#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include "ib_arena_source_owner.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Source-only receipts from PR #152. No parsing or storage authority here:
 * the canonical appendfat .used pointer is the authoritative commit boundary.
 * The receipt supplies the exact source offset, size and digest. */
enum receipt_key {
    RK_SCHEMA, RK_REQUEST, RK_REQUESTED_URL, RK_RESOLVED_URL, RK_STATUS,
    RK_MIME, RK_TRANSPORT, RK_ARENA, RK_OFFSET, RK_END, RK_LENGTH,
    RK_SHA256, RK_REOPEN, RK_NAVIGATION, RK_COUNT
};
static const char *const receipt_names[RK_COUNT] = {
    "schema", "request_id", "requested_url", "resolved_url", "http_status",
    "mime_type", "transport", "arena_name", "arena_offset", "arena_end",
    "byte_count", "sha256", "reopen_check", "navigation_admitted"
};
#define RECEIPT_MAX_BYTES 8192U
#define SOURCE_WRITER_DEADLINE_MS 5000
#define SOURCE_MAX_CONCURRENT_WRITERS 2U
static atomic_uint source_writers = 0;

typedef struct {
    unsigned char *bytes;
    uint32_t length;
    int writer_fd;
} source_writer;

static int64_t time_milliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return -1;
    return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static int safe_request_id(const char *id) {
    if (!id) return 0;
    size_t length = strnlen(id, IB_READER_REQUEST_ID_CAP + 1U);
    if (length == 0 || length > IB_READER_REQUEST_ID_CAP) return 0;
    for (size_t i = 0; i < length; ++i) {
        unsigned char ch = (unsigned char)id[i];
        if (!((ch >= 'a' && ch <= 'z') ||
              (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') ||
              ch == '.' || ch == '_' || ch == '-')) return 0;
    }
    return strcmp(id, ".") != 0 && strcmp(id, "..") != 0;
}

static int parse_uint(const char *value, uint64_t *out) {
    if (!value || !*value || !out) return -1;
    for (const char *ch = value; *ch; ++ch) {
        if (*ch < '0' || *ch > '9') return -1;
    }
    errno = 0;
    char *end = NULL;
    uintmax_t parsed = strtoumax(value, &end, 10);
    if (errno != 0 || !end || *end != '\0' || parsed > INT64_MAX)
        return -1;
    *out = (uint64_t)parsed;
    return 0;
}

static int parse_receipt(char *text, size_t size, const char *request_id,
                         uint64_t *offset, uint32_t *length,
                         char sha256[65]) {
    char *fields[RK_COUNT] = {0};
    size_t found = 0;
    char *position = text, *end = text + size;
    while (position < end) {
        char *newline = memchr(position, '\n', (size_t)(end - position));
        if (!newline) return -1;
        char *separator = memchr(position, '\t',
                                 (size_t)(newline - position));
        if (!separator || separator == position ||
            separator == newline - 1) return -1;
        if (memchr(separator + 1, '\t',
                   (size_t)(newline - separator - 1)) != NULL)
            return -1;
        if (memchr(position, '\r', (size_t)(newline - position)) ||
            memchr(position, '\0', (size_t)(newline - position)))
            return -1;
        *separator = '\0';
        *newline = '\0';
        int matched = 0;
        for (size_t i = 0; i < RK_COUNT; ++i) {
            if (strcmp(receipt_names[i], position) == 0) {
                if (fields[i]) return -1;
                fields[i] = separator + 1;
                matched = 1;
                break;
            }
        }
        if (!matched || ++found > RK_COUNT) return -1;
        position = newline + 1;
    }
    if (found != RK_COUNT ||
        strcmp(fields[RK_SCHEMA], "ib-icu-arena-v1") != 0 ||
        strcmp(fields[RK_REQUEST], request_id) != 0 ||
        strcmp(fields[RK_TRANSPORT],
               "icu-app-process-via-binder-shizuku") != 0 ||
        strcmp(fields[RK_ARENA], "responses.arena") != 0 ||
        strcmp(fields[RK_REOPEN], "MATCH") != 0 ||
        strcmp(fields[RK_NAVIGATION], "NO") != 0)
        return -1;

    /* v1 knows only that ICU emitted body bytes, not that HTML/navigation
     * may be displayed as a verified HTTP response. */
    if (strcmp(fields[RK_RESOLVED_URL], "UNOBSERVED") != 0 ||
        strcmp(fields[RK_STATUS], "UNOBSERVED") != 0 ||
        strcmp(fields[RK_MIME], "UNOBSERVED") != 0)
        return -1;

    uint64_t begin, finish, amount;
    if (parse_uint(fields[RK_OFFSET], &begin) != 0 ||
        parse_uint(fields[RK_END], &finish) != 0 ||
        parse_uint(fields[RK_LENGTH], &amount) != 0 ||
        amount == 0 || amount > IB_READER_SOURCE_MAX_BYTES ||
        begin > finish || finish - begin != amount)
        return -1;

    const char *hash = fields[RK_SHA256];
    if (strlen(hash) != 64U) return -1;
    for (size_t i = 0; i < 64; ++i)
        if (!((hash[i] >= '0' && hash[i] <= '9') ||
              (hash[i] >= 'a' && hash[i] <= 'f'))) return -1;

    *offset = begin;
    *length = (uint32_t)amount;
    memcpy(sha256, hash, 65U);
    return 0;
}

static int read_manifest(int root_fd, const char *request_id,
                         uint64_t *offset, uint32_t *length, char sha256[65]) {
    int dir = -1, manifest = -1, result = -1;
    char filename[IB_READER_REQUEST_ID_CAP + 5U];
    struct stat info;
    char content[RECEIPT_MAX_BYTES + 1U];
    int n = snprintf(filename, sizeof(filename), "%s.tsv", request_id);
    if (n < 1 || (size_t)n >= sizeof(filename)) return -1;
    dir = openat(root_fd, "receipts",
                 O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dir < 0) goto done;
    manifest = openat(dir, filename, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (manifest < 0 || fstat(manifest, &info) != 0 ||
        !S_ISREG(info.st_mode) || info.st_size <= 0 ||
        info.st_size > RECEIPT_MAX_BYTES ||
        info.st_uid != geteuid() || info.st_nlink != 1)
        goto done;
    size_t size = (size_t)info.st_size;
    size_t got = 0;
    while (got < size) {
        ssize_t amount = pread(manifest, content + got, size - got, (off_t)got);
        if (amount < 0 && errno == EINTR) continue;
        if (amount <= 0) goto done;
        got += (size_t)amount;
    }
    content[size] = '\0';
    result = parse_receipt(content, size, request_id, offset, length, sha256);
done:
    if (manifest >= 0) close(manifest);
    if (dir >= 0) close(dir);
    return result;
}

static int read_committed_bytes(int root_fd, uint64_t offset, uint32_t length,
                                unsigned char *bytes) {
    int lock = -1, used = -1, arena = -1, result = -1;
    struct stat info;
    char pointer[64];
    lock = openat(root_fd, "responses.arena.lock",
                  O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (lock < 0 || flock(lock, LOCK_SH | LOCK_NB) != 0) goto done;
    used = openat(root_fd, "responses.arena.used",
                  O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    arena = openat(root_fd, "responses.arena",
                   O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (used < 0 || arena < 0 ||
        fstat(used, &info) != 0 || !S_ISREG(info.st_mode))
        goto done;
    ssize_t pointer_bytes = pread(used, pointer, sizeof(pointer) - 1U, 0);
    if (pointer_bytes <= 1 || pointer_bytes >= (ssize_t)sizeof(pointer) - 1 ||
        pointer[pointer_bytes - 1] != '\n')
        goto done;
    pointer[pointer_bytes - 1] = '\0';
    uint64_t committed = 0;
    if (parse_uint(pointer, &committed) != 0 ||
        fstat(arena, &info) != 0 || !S_ISREG(info.st_mode) ||
        info.st_uid != geteuid() || info.st_size < 0 ||
        committed > (uint64_t)info.st_size ||
        offset > committed || length > committed - offset)
        goto done;

    size_t filled = 0;
    while (filled < length) {
        size_t remaining = (size_t)length - filled;
        if (remaining > 65536U) remaining = 65536U;
        ssize_t amount = pread(arena, bytes + filled, remaining,
                               (off_t)(offset + filled));
        if (amount < 0 && errno == EINTR) continue;
        if (amount <= 0) goto done;
        filled += (size_t)amount;
    }
    result = 0;
done:
    if (arena >= 0) close(arena);
    if (used >= 0) close(used);
    if (lock >= 0) close(lock);
    return result;
}

static void *write_source(void *argument) {
    source_writer *writer = argument;
    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGPIPE);
    (void)pthread_sigmask(SIG_BLOCK, &blocked, NULL);

    int64_t now = time_milliseconds();
    int64_t deadline = now < 0 ? 0 : now + SOURCE_WRITER_DEADLINE_MS;
    size_t sent = 0;
    while (sent < writer->length) {
        now = time_milliseconds();
        if (now < 0 || now >= deadline) break;
        struct pollfd poll_fd = { writer->writer_fd, POLLOUT, 0 };
        int remaining = (int)(deadline - now);
        int status = poll(&poll_fd, 1, remaining);
        if (status < 0 && errno == EINTR) continue;
        if (status <= 0 || (poll_fd.revents & (POLLERR | POLLNVAL | POLLHUP)))
            break;
        size_t chunk = writer->length - sent;
        if (chunk > 65536U) chunk = 65536U;
        ssize_t count = write(writer->writer_fd, writer->bytes + sent, chunk);
        if (count < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        if (count <= 0) break;
        sent += (size_t)count;
    }
    close(writer->writer_fd);
    free(writer->bytes);
    free(writer);
    atomic_fetch_sub(&source_writers, 1U);
    return NULL;
}

int ib_arena_source_owner_init(ib_arena_source_owner *owner,
                               const char *trusted_private_root) {
    if (!owner || !trusted_private_root || trusted_private_root[0] != '/')
        return -1;
    size_t length = strnlen(trusted_private_root,
                            IB_ARENA_PRIVATE_ROOT_CAP);
    if (length == 0 || length >= IB_ARENA_PRIVATE_ROOT_CAP)
        return -1;
    memcpy(owner->private_root, trusted_private_root, length + 1U);
    atomic_init(&owner->wanted_navigation_generation, 0);
    return 0;
}

void ib_arena_source_owner_want(ib_arena_source_owner *owner,
                                uint64_t navigation_generation) {
    if (owner)
        atomic_store(&owner->wanted_navigation_generation,
                     navigation_generation);
}

int ib_arena_source_owner_open(ib_arena_source_owner *owner,
                               const char *request_id,
                               uint64_t navigation_generation,
                               ib_reader_source_ticket *ticket,
                               int *read_fd) {
    if (read_fd) *read_fd = -1;
    if (!owner || !read_fd || !safe_request_id(request_id) ||
        navigation_generation == 0 ||
        navigation_generation !=
            atomic_load(&owner->wanted_navigation_generation))
        return -1;

    int root = -1, result = -1, descriptor[2] = {-1, -1};
    uint64_t offset = 0;
    uint32_t length = 0;
    char expected[65], observed[65];
    unsigned char *bytes = NULL;
    source_writer *writer = NULL;
    pthread_t thread;
    struct stat info;

    root = open(owner->private_root,
                O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (root < 0 || fstat(root, &info) != 0 ||
        !S_ISDIR(info.st_mode) || info.st_uid != geteuid() ||
        (info.st_mode & 077U) != 0) goto done;
    if (read_manifest(root, request_id, &offset, &length, expected) != 0)
        goto done;
    bytes = malloc(length);
    if (!bytes || read_committed_bytes(root, offset, length, bytes) != 0)
        goto done;
    ib_reader_sha256_hex(bytes, length, observed);
    if (strcmp(expected, observed) != 0) goto done;
    if (navigation_generation !=
        atomic_load(&owner->wanted_navigation_generation)) goto done;

    if (atomic_fetch_add(&source_writers, 1U) >=
        SOURCE_MAX_CONCURRENT_WRITERS) {
        atomic_fetch_sub(&source_writers, 1U);
        goto done;
    }
    if (pipe2(descriptor, O_CLOEXEC | O_NONBLOCK) != 0) {
        atomic_fetch_sub(&source_writers, 1U);
        goto done;
    }
    /* The consumer receives a blocking read descriptor but the writer
     * stays nonblocking so unresponsive readers cannot accumulate threads. */
    int flags = fcntl(descriptor[0], F_GETFL);
    if (flags < 0 ||
        fcntl(descriptor[0], F_SETFL, flags & ~O_NONBLOCK) != 0) {
        atomic_fetch_sub(&source_writers, 1U);
        goto done;
    }
    writer = calloc(1, sizeof(*writer));
    if (!writer) {
        atomic_fetch_sub(&source_writers, 1U);
        goto done;
    }
    writer->bytes = bytes;
    writer->length = length;
    writer->writer_fd = descriptor[1];
    if (pthread_create(&thread, NULL, write_source, writer) != 0) {
        writer->bytes = NULL; /* caller still owns bytes */
        free(writer);
        writer = NULL;
        atomic_fetch_sub(&source_writers, 1U);
        goto done;
    }
    (void)pthread_detach(thread);
    bytes = NULL;
    writer = NULL;
    descriptor[1] = -1;
    if (ticket) {
        memset(ticket, 0, sizeof(*ticket));
        memcpy(ticket->request_id, request_id, strlen(request_id) + 1U);
        ticket->navigation_generation = navigation_generation;
        ticket->byte_count = length;
        memcpy(ticket->sha256_hex, expected, sizeof(expected));
    }
    *read_fd = descriptor[0];
    descriptor[0] = -1;
    result = 0;
done:
    if (descriptor[0] >= 0) close(descriptor[0]);
    if (descriptor[1] >= 0) close(descriptor[1]);
    if (root >= 0) close(root);
    free(bytes);
    return result;
}

int ib_arena_source_owner_open_binder(void *owner, const char *request_id,
                                      uint64_t navigation_generation,
                                      int *read_fd) {
    return ib_arena_source_owner_open((ib_arena_source_owner *)owner,
                                      request_id, navigation_generation,
                                      NULL, read_fd);
}
