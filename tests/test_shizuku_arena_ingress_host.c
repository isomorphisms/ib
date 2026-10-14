#define _GNU_SOURCE
#include "../native/android/ib_shizuku_arena_ingress.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static const uid_t bridge_uid = (uid_t)2000;
static const char *example_url = "https://example.com/";
static const unsigned char body[] = {'I', 'B', 0, 'C', '6', '7', 255, '\n'};

typedef struct {
    int publications;
    int deny_publication;
    unsigned char last[sizeof(body)];
    size_t count;
    char id[IB_READER_REQUEST_ID_CAP + 1U];
    char digest[65];
} fake_arena;

static int publish(void *opaque, const char *id, const char *url,
                   const unsigned char *bytes, size_t n,
                   const char sha[65]) {
    fake_arena *store = opaque;
    assert(strcmp(url, example_url) == 0);
    if (store->deny_publication) return -1;
    if (n != sizeof(body) || memcmp(body, bytes, n) != 0) return -1;
    char computed[65];
    ib_reader_sha256_hex(bytes, n, computed);
    if (strcmp(computed, sha) != 0) return -1;
    memcpy(store->last, bytes, n);
    store->count = n;
    memcpy(store->id, id, strlen(id) + 1U);
    memcpy(store->digest, sha, 65U);
    store->publications += 1;
    return 0;
}

static int transmit_descriptor(int socket, int descriptor) {
    char payload = 'P';
    struct iovec segment = {&payload, 1U};
    union {struct cmsghdr align; unsigned char body[CMSG_SPACE(sizeof(int))];} control = {0};
    struct msghdr message = {0};
    message.msg_iov = &segment;
    message.msg_iovlen = 1;
    message.msg_control = control.body;
    message.msg_controllen = sizeof(control.body);
    struct cmsghdr *header = CMSG_FIRSTHDR(&message);
    if (!header) return -1;
    header->cmsg_level = SOL_SOCKET;
    header->cmsg_type = SCM_RIGHTS;
    header->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(header), &descriptor, sizeof(descriptor));
    return sendmsg(socket, &message, 0) == 1 ? 0 : -1;
}

static int receive_descriptor(int socket) {
    char payload = '\0';
    struct iovec segment = {&payload, 1U};
    union {struct cmsghdr align; unsigned char body[CMSG_SPACE(sizeof(int))];} control = {0};
    struct msghdr message = {0};
    message.msg_iov = &segment;
    message.msg_iovlen = 1;
    message.msg_control = control.body;
    message.msg_controllen = sizeof(control.body);
    if (recvmsg(socket, &message, 0) != 1 || payload != 'P' ||
        (message.msg_flags & MSG_CTRUNC) != 0) return -1;
    struct cmsghdr *header = CMSG_FIRSTHDR(&message);
    if (!header || header->cmsg_level != SOL_SOCKET ||
        header->cmsg_type != SCM_RIGHTS ||
        header->cmsg_len != CMSG_LEN(sizeof(int))) return -1;
    int fd = -1;
    memcpy(&fd, CMSG_DATA(header), sizeof(fd));
    return fd;
}

static void write_source(int fd, const unsigned char *bytes, size_t count) {
    size_t sent = 0;
    while (sent < count) {
        ssize_t n = write(fd, bytes + sent, count - sent);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) _exit(38);
        sent += (size_t)n;
    }
}

static int exchange(ib_shizuku_arena_ingress *session,
                    const char *id, uint64_t generation,
                    const unsigned char *bytes, size_t count,
                    int producer_status, int stale_during_transfer) {
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    /* Fork BEFORE the source-owner creates the pipe. The bridge process
     * cannot inherit an app-private Arena FD or source-reader pipe end. */
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        signal(SIGPIPE, SIG_IGN);
        close(sockets[0]);
        int transferred = receive_descriptor(sockets[1]);
        if (transferred < 0) _exit(41);
        write_source(transferred, bytes, count);
        close(transferred);
        close(sockets[1]);
        _exit(producer_status);
    }
    close(sockets[1]);
    int write_fd = -1;
    assert(ib_ingress_begin(session, (uid_t)10001, id, generation,
                            &write_fd) != 0);
    assert(write_fd == -1);
    assert(ib_ingress_begin(session, bridge_uid, "wrong", generation,
                            &write_fd) != 0);
    assert(write_fd == -1);
    assert(ib_ingress_begin(session, bridge_uid, id, generation + 1U,
                            &write_fd) != 0);
    assert(write_fd == -1);
    assert(ib_ingress_begin(session, bridge_uid, id, generation,
                            &write_fd) == 0);
    assert(write_fd >= 0);
    int repeated_fd = -1;
    assert(ib_ingress_begin(session, bridge_uid, id, generation,
                            &repeated_fd) != 0 && repeated_fd == -1);
    assert(transmit_descriptor(sockets[0], write_fd) == 0);
    close(write_fd);
    close(sockets[0]);
    if (stale_during_transfer)
        ib_ingress_want(session, generation + 1U);
    int wait_status = 0;
    assert(waitpid(child, &wait_status, 0) == child);
    int exit_status = WIFEXITED(wait_status) ? WEXITSTATUS(wait_status) : 128;
    assert(ib_ingress_finish(session, (uid_t)10001, id,
                             generation, exit_status) != 0);
    int result = ib_ingress_finish(session, bridge_uid, id,
                                   generation, exit_status);
    assert(ib_ingress_finish(session, bridge_uid, id,
                             generation, exit_status) != 0);
    return result;
}

static void arm(ib_shizuku_arena_ingress *session,
                fake_arena *store, const char *id, uint64_t generation) {
    assert(ib_ingress_init(session, publish, store) == 0);
    assert(ib_ingress_arm(session, id, generation, example_url, bridge_uid) == 0);
}

int main(void) {
    fake_arena store = {0};
    ib_shizuku_arena_ingress session;
    assert(ib_ingress_init(&session, publish, &store) == 0);
    assert(ib_ingress_arm(&session, "bad", 1, "file:///etc/passwd", bridge_uid) != 0);
    assert(ib_ingress_arm(&session, "bad", 1,
                          "https://user:password@example.com/", bridge_uid) != 0);
    assert(ib_ingress_arm(&session, "../bad", 1, example_url, bridge_uid) != 0);
    assert(ib_ingress_arm(&session, "bad", 1, example_url, (uid_t)0) != 0);
    assert(ib_ingress_arm(&session, "good", 1, example_url, bridge_uid) == 0);
    assert(ib_ingress_arm(&session, "second", 2, example_url, bridge_uid) != 0);
    assert(exchange(&session, "good", 1, body, sizeof(body), 0, 0) == 0);
    assert(store.publications == 1);
    assert(store.count == sizeof(body));
    assert(memcmp(store.last, body, sizeof(body)) == 0);
    assert(strcmp(store.id, "good") == 0);
    ib_ingress_dispose(&session);

    arm(&session, &store, "exit29", 2);
    assert(exchange(&session, "exit29", 2, body, sizeof(body), 29, 0) != 0);
    assert(store.publications == 1);
    ib_ingress_dispose(&session);

    arm(&session, &store, "empty", 3);
    assert(exchange(&session, "empty", 3, body, 0, 0, 0) != 0);
    assert(store.publications == 1);
    ib_ingress_dispose(&session);

    arm(&session, &store, "stale", 4);
    assert(exchange(&session, "stale", 4, body, sizeof(body), 0, 1) != 0);
    assert(store.publications == 1);
    ib_ingress_dispose(&session);

    arm(&session, &store, "denied", 5);
    store.deny_publication = 1;
    assert(exchange(&session, "denied", 5, body, sizeof(body), 0, 0) != 0);
    assert(store.publications == 1);
    ib_ingress_dispose(&session);
    store.deny_publication = 0;

    arm(&session, &store, "too-large", 6);
    unsigned char *oversized = malloc(IB_READER_SOURCE_MAX_BYTES + 1U);
    assert(oversized != NULL);
    memset(oversized, 'Z', IB_READER_SOURCE_MAX_BYTES + 1U);
    assert(exchange(&session, "too-large", 6, oversized,
                    IB_READER_SOURCE_MAX_BYTES + 1U, 0, 0) != 0);
    assert(store.publications == 1);
    free(oversized);
    ib_ingress_dispose(&session);

    arm(&session, &store, "recovery", 7);
    assert(exchange(&session, "recovery", 7, body, sizeof(body), 0, 0) == 0);
    assert(store.publications == 2);
    ib_ingress_dispose(&session);

    puts("PASS: cross-process anonymous FD delivery, app-owned source publish, binary bytes, UID/ID/generation/URL refusal, nonzero ICU exit, stale, empty, oversized, duplicate and recovery");
    puts("NOT_RUN: actual Android Binder, real Shizuku/ICU HTTPS and physical C67");
    return 0;
}
