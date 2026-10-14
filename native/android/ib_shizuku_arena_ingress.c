#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "ib_shizuku_arena_ingress.h"

#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define INGEST_POLL_SLICE_MS 250
#define INGEST_BUFFER_CAP (IB_READER_SOURCE_MAX_BYTES + 1U)

static int64_t monotonic_milliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return -1;
    return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static int valid_request_id(const char *request_id) {
    if (!request_id) return 0;
    size_t size = strnlen(request_id, IB_READER_REQUEST_ID_CAP + 1U);
    if (size == 0 || size > IB_READER_REQUEST_ID_CAP ||
        (size == 1 && request_id[0] == '.') ||
        (size == 2 && request_id[0] == '.' && request_id[1] == '.'))
        return 0;
    for (size_t i = 0; i < size; ++i) {
        unsigned char c = (unsigned char)request_id[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
              c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}

static int valid_requested_url(const char *url) {
    if (!url) return 0;
    size_t size = strnlen(url, IB_INGRESS_URL_CAP + 1U);
    if (size <= 8 || size > IB_INGRESS_URL_CAP ||
        (strncmp(url, "http://", 7) != 0 &&
         strncmp(url, "https://", 8) != 0))
        return 0;
    /* Match the pinned Shizuku ICU shell quoting contract: no apostrophes,
     * whitespace, controls, non-ASCII or shell metacharacter expansion via
     * the command adapter. HTTP URL syntax is still owned by ICU. */
    for (size_t i = 0; i < size; ++i) {
        unsigned char c = (unsigned char)url[i];
        if (c < 33 || c > 126 || c == '\'') return 0;
    }
    size_t authority = strncmp(url, "https://", 8) == 0 ? 8U : 7U;
    if (url[authority] == '\0' || url[authority] == '/' ||
        url[authority] == '?' || url[authority] == '#') return 0;
    /* No credentials forwarded to redirects or included in receipts. */
    for (size_t i = authority; i < size && url[i] != '/' &&
                          url[i] != '?' && url[i] != '#'; ++i)
        if (url[i] == '@') return 0;
    return 1;
}

static int allowed_broker_uid(uid_t uid) {
    /* Only an app-approved explicit UID, never a wildcard. Shizuku's shell
     * UID is 2000, but a Termux broker may use a different app UID. */
    return uid == (uid_t)2000 || uid >= (uid_t)10000;
}

static int matching_session(const ib_shizuku_arena_ingress *session,
                            uid_t caller, const char *id, uint64_t generation) {
    return session && caller == session->permitted_bridge_uid &&
           valid_request_id(id) &&
           strcmp(id, session->request_id) == 0 &&
           generation != 0 && generation == session->request_generation &&
           generation ==
               atomic_load(&session->wanted_navigation_generation);
}

static void *consume_pipe(void *opaque) {
    ib_shizuku_arena_ingress *session = opaque;
    unsigned char *bytes = malloc(INGEST_BUFFER_CAP);
    size_t count = 0;
    int complete = 0;
    int fd = session->source_fd;
    int64_t start = monotonic_milliseconds();
    int64_t deadline = start < 0 ? 0 : start + IB_INGRESS_TIMEOUT_MILLISECONDS;
    if (!bytes || start < 0) goto done;

    for (;;) {
        if (atomic_load(&session->cancelled) ||
            session->request_generation !=
                atomic_load(&session->wanted_navigation_generation))
            goto done;
        int64_t now = monotonic_milliseconds();
        if (now < 0 || now >= deadline) goto done;
        int timeout = (int)(deadline - now);
        if (timeout > INGEST_POLL_SLICE_MS) timeout = INGEST_POLL_SLICE_MS;
        struct pollfd descriptor = {fd, POLLIN | POLLHUP, 0};
        int observed = poll(&descriptor, 1, timeout);
        if (observed < 0 && errno == EINTR) continue;
        if (observed < 0 || (descriptor.revents & (POLLERR | POLLNVAL)))
            goto done;
        if (observed == 0) continue;
        size_t available = INGEST_BUFFER_CAP - count;
        if (available == 0) goto done;
        if (available > 65536U) available = 65536U;
        ssize_t amount = read(fd, bytes + count, available);
        if (amount < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        if (amount < 0) goto done;
        if (amount == 0) {
            if (count == 0 || count > IB_READER_SOURCE_MAX_BYTES) goto done;
            complete = 1;
            break;
        }
        count += (size_t)amount;
        if (count > IB_READER_SOURCE_MAX_BYTES) goto done;
    }

done:
    close(fd);
    session->source_fd = -1;
    if (!complete) {
        free(bytes);
        bytes = NULL;
        count = 0;
    }
    session->bytes = bytes;
    session->byte_count = count;
    session->stream_complete = complete;
    return NULL;
}

int ib_ingress_init(ib_shizuku_arena_ingress *session,
                    ib_ingress_publish publish, void *source_owner) {
    if (!session || !publish || !source_owner) return -1;
    memset(session, 0, sizeof(*session));
    if (pthread_mutex_init(&session->mutex, NULL) != 0) return -1;
    atomic_init(&session->wanted_navigation_generation, 0);
    atomic_init(&session->cancelled, false);
    session->source_fd = -1;
    session->publish = publish;
    session->source_owner = source_owner;
    session->state = IB_INGRESS_IDLE;
    return 0;
}

int ib_ingress_arm(ib_shizuku_arena_ingress *session,
                   const char *request_id, uint64_t navigation_generation,
                   const char *requested_url, uid_t explicitly_allowed_uid) {
    if (!session || !valid_request_id(request_id) ||
        !valid_requested_url(requested_url) ||
        !allowed_broker_uid(explicitly_allowed_uid) ||
        navigation_generation == 0) return -1;
    pthread_mutex_lock(&session->mutex);
    if (session->state != IB_INGRESS_IDLE) {
        pthread_mutex_unlock(&session->mutex);
        return -1;
    }
    memcpy(session->request_id, request_id, strlen(request_id) + 1U);
    memcpy(session->requested_url, requested_url, strlen(requested_url) + 1U);
    session->request_generation = navigation_generation;
    session->permitted_bridge_uid = explicitly_allowed_uid;
    atomic_store(&session->wanted_navigation_generation, navigation_generation);
    session->state = IB_INGRESS_ARMED;
    pthread_mutex_unlock(&session->mutex);
    return 0;
}

void ib_ingress_want(ib_shizuku_arena_ingress *session,
                     uint64_t navigation_generation) {
    if (session)
        atomic_store(&session->wanted_navigation_generation,
                     navigation_generation);
}

int ib_ingress_begin(ib_shizuku_arena_ingress *session,
                     uid_t calling_uid, const char *request_id,
                     uint64_t generation, int *owned_write_fd) {
    if (owned_write_fd) *owned_write_fd = -1;
    if (!session || !owned_write_fd) return -1;
    pthread_mutex_lock(&session->mutex);
    if (session->state != IB_INGRESS_ARMED ||
        !matching_session(session, calling_uid, request_id, generation)) {
        pthread_mutex_unlock(&session->mutex);
        return -1;
    }
    int pair[2] = {-1, -1};
    if (pipe2(pair, O_CLOEXEC | O_NONBLOCK) != 0) {
        pthread_mutex_unlock(&session->mutex);
        return -1;
    }
    int flags = fcntl(pair[1], F_GETFL);
    if (flags < 0 || fcntl(pair[1], F_SETFL, flags & ~O_NONBLOCK) != 0) {
        close(pair[0]);
        close(pair[1]);
        pthread_mutex_unlock(&session->mutex);
        return -1;
    }
    session->source_fd = pair[0];
    if (pthread_create(&session->read_thread, NULL, consume_pipe, session) != 0) {
        session->source_fd = -1;
        close(pair[0]);
        close(pair[1]);
        pthread_mutex_unlock(&session->mutex);
        return -1;
    }
    session->thread_started = 1;
    session->state = IB_INGRESS_STREAMING;
    *owned_write_fd = pair[1];
    pthread_mutex_unlock(&session->mutex);
    return 0;
}

int ib_ingress_finish(ib_shizuku_arena_ingress *session,
                      uid_t calling_uid, const char *request_id,
                      uint64_t generation, int producer_exit_status) {
    if (!session) return -1;
    pthread_mutex_lock(&session->mutex);
    if (session->state != IB_INGRESS_STREAMING ||
        !matching_session(session, calling_uid, request_id, generation)) {
        pthread_mutex_unlock(&session->mutex);
        return -1;
    }
    session->state = IB_INGRESS_FINISHING;
    if (producer_exit_status != 0)
        atomic_store(&session->cancelled, true);
    pthread_mutex_unlock(&session->mutex);

    if (session->thread_started) {
        (void)pthread_join(session->read_thread, NULL);
        session->thread_started = 0;
    }
    int result = -1;
    if (producer_exit_status == 0 &&
        session->stream_complete && session->bytes &&
        session->byte_count != 0 &&
        session->byte_count <= IB_READER_SOURCE_MAX_BYTES &&
        !atomic_load(&session->cancelled) &&
        generation == atomic_load(&session->wanted_navigation_generation)) {
        char digest[65];
        ib_reader_sha256_hex(session->bytes, session->byte_count, digest);
        /* The source owner independently read-back verifies the exact
         * canonical append before any receipt becomes visible. */
        result = session->publish(session->source_owner, session->request_id,
                                  session->requested_url, session->bytes,
                                  session->byte_count, digest);
    }
    free(session->bytes);
    session->bytes = NULL;
    session->byte_count = 0;
    session->stream_complete = 0;
    pthread_mutex_lock(&session->mutex);
    session->state = IB_INGRESS_CLOSED;
    pthread_mutex_unlock(&session->mutex);
    return result;
}

void ib_ingress_dispose(ib_shizuku_arena_ingress *session) {
    if (!session) return;
    atomic_store(&session->cancelled, true);
    if (session->thread_started) {
        (void)pthread_join(session->read_thread, NULL);
        session->thread_started = 0;
    }
    free(session->bytes);
    session->bytes = NULL;
    session->byte_count = 0;
    session->source_fd = -1;
    session->state = IB_INGRESS_CLOSED;
    pthread_mutex_destroy(&session->mutex);
}
