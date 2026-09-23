#define _GNU_SOURCE
#include <errno.h>
#include <stddef.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static const char ping_bytes[] = "ping\n";
static const char pong_bytes[] = "pong\n";

static void usage(const char *program)
{
    fprintf(stderr,
        "usage: %s <listen|connect> <abstract|path> <name-or-path> [hold]\n",
        program);
}

static void print_errno_result(const char *stage)
{
    printf("status=error\tstage=%s\terrno=%d\tmessage=%s\n",
        stage, errno, strerror(errno));
    fflush(stdout);
}

static int write_all(int fd, const void *data, size_t size)
{
    const unsigned char *bytes = data;
    size_t offset = 0;
    while (offset < size) {
        ssize_t wrote = write(fd, bytes + offset, size - offset);
        if (wrote < 0) {
            return -1;
        }
        offset += (size_t) wrote;
    }
    return 0;
}

static int read_exact(int fd, void *data, size_t size)
{
    unsigned char *bytes = data;
    size_t offset = 0;
    while (offset < size) {
        ssize_t got = read(fd, bytes + offset, size - offset);
        if (got == 0) {
            errno = ECONNRESET;
            return -1;
        }
        if (got < 0) {
            return -1;
        }
        offset += (size_t) got;
    }
    return 0;
}

static int make_address(
    struct sockaddr_un *address,
    socklen_t *address_length,
    const char *namespace_name,
    const char *name)
{
    size_t name_length = strlen(name);
    memset(address, 0, sizeof(*address));
    address->sun_family = AF_UNIX;

    if (strcmp(namespace_name, "abstract") == 0) {
        if (name_length + 1 > sizeof(address->sun_path)) {
            errno = ENAMETOOLONG;
            return -1;
        }
        address->sun_path[0] = '\0';
        memcpy(address->sun_path + 1, name, name_length);
        *address_length = (socklen_t)
            (offsetof(struct sockaddr_un, sun_path) + 1 + name_length);
        return 0;
    }

    if (strcmp(namespace_name, "path") == 0) {
        if (name_length + 1 > sizeof(address->sun_path)) {
            errno = ENAMETOOLONG;
            return -1;
        }
        memcpy(address->sun_path, name, name_length + 1);
        *address_length = (socklen_t)
            (offsetof(struct sockaddr_un, sun_path) + name_length + 1);
        return 0;
    }

    errno = EINVAL;
    return -1;
}

static void print_self(void)
{
    printf("self_pid=%ld\tself_uid=%ld\n",
        (long) getpid(), (long) getuid());
}

static void print_peer(int fd)
{
    struct ucred credentials;
    socklen_t length = sizeof(credentials);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &credentials, &length) == 0) {
        printf("peer_pid=%ld\tpeer_uid=%ld\n",
            (long) credentials.pid, (long) credentials.uid);
    } else {
        print_errno_result("peer-credentials");
    }
    fflush(stdout);
}

static int exchange_as_listener(int fd)
{
    char bytes[sizeof(ping_bytes) - 1];
    if (read_exact(fd, bytes, sizeof(bytes)) < 0) {
        print_errno_result("read-ping");
        return -1;
    }
    if (memcmp(bytes, ping_bytes, sizeof(bytes)) != 0) {
        printf("status=error\tstage=read-ping\tmessage=wrong-payload\n");
        return -1;
    }
    if (write_all(fd, pong_bytes, sizeof(pong_bytes) - 1) < 0) {
        print_errno_result("write-pong");
        return -1;
    }
    printf("exchange=pass\trole=listener\n");
    fflush(stdout);
    return 0;
}

static int exchange_as_connector(int fd)
{
    char bytes[sizeof(pong_bytes) - 1];
    if (write_all(fd, ping_bytes, sizeof(ping_bytes) - 1) < 0) {
        print_errno_result("write-ping");
        return -1;
    }
    if (read_exact(fd, bytes, sizeof(bytes)) < 0) {
        print_errno_result("read-pong");
        return -1;
    }
    if (memcmp(bytes, pong_bytes, sizeof(bytes)) != 0) {
        printf("status=error\tstage=read-pong\tmessage=wrong-payload\n");
        return -1;
    }
    printf("exchange=pass\trole=connector\n");
    fflush(stdout);
    return 0;
}

static int wait_for_peer_close(int fd)
{
    unsigned char byte;
    printf("hold=waiting-for-peer-close\n");
    fflush(stdout);
    for (;;) {
        ssize_t got = read(fd, &byte, 1);
        if (got == 0) {
            printf("peer_close=eof\n");
            fflush(stdout);
            return 0;
        }
        if (got < 0) {
            print_errno_result("hold-read");
            return -1;
        }
        printf("peer_close=unexpected-byte-%u\n", (unsigned) byte);
        fflush(stdout);
    }
}

static int run_listener(
    const char *namespace_name,
    const char *name,
    int hold)
{
    struct sockaddr_un address;
    socklen_t address_length;
    if (make_address(&address, &address_length, namespace_name, name) < 0) {
        print_errno_result("address");
        return 1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        print_errno_result("socket");
        return 1;
    }

    if (strcmp(namespace_name, "path") == 0) {
        if (unlink(name) < 0 && errno != ENOENT) {
            print_errno_result("unlink-before-bind");
            close(fd);
            return 1;
        }
    }

    if (bind(fd, (struct sockaddr *) &address, address_length) < 0) {
        print_errno_result("bind");
        close(fd);
        return 1;
    }
    if (listen(fd, 4) < 0) {
        print_errno_result("listen");
        close(fd);
        return 1;
    }

    print_self();
    printf("ready=true\tnamespace=%s\taddress=%s\thold=%s\n",
        namespace_name, name, hold ? "true" : "false");
    fflush(stdout);

    int peer = accept4(fd, NULL, NULL, SOCK_CLOEXEC);
    if (peer < 0) {
        print_errno_result("accept");
        close(fd);
        return 1;
    }

    print_peer(peer);
    int result = exchange_as_listener(peer);
    if (result == 0 && hold) {
        result = wait_for_peer_close(peer);
    }

    close(peer);
    close(fd);
    if (strcmp(namespace_name, "path") == 0) {
        if (unlink(name) < 0 && errno != ENOENT) {
            print_errno_result("unlink-after-close");
            return 1;
        }
    }
    return result == 0 ? 0 : 1;
}

static int run_connector(
    const char *namespace_name,
    const char *name,
    int hold)
{
    struct sockaddr_un address;
    socklen_t address_length;
    if (make_address(&address, &address_length, namespace_name, name) < 0) {
        print_errno_result("address");
        return 1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        print_errno_result("socket");
        return 1;
    }
    if (connect(fd, (struct sockaddr *) &address, address_length) < 0) {
        print_self();
        print_errno_result("connect");
        close(fd);
        return 1;
    }

    print_self();
    print_peer(fd);
    int result = exchange_as_connector(fd);
    if (result == 0 && hold) {
        result = wait_for_peer_close(fd);
    }
    close(fd);
    return result == 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);

    if (argc != 4 && argc != 5) {
        usage(argv[0]);
        return 2;
    }
    if (strcmp(argv[2], "abstract") != 0 && strcmp(argv[2], "path") != 0) {
        usage(argv[0]);
        return 2;
    }

    int hold = argc == 5 && strcmp(argv[4], "hold") == 0;
    if (argc == 5 && !hold) {
        usage(argv[0]);
        return 2;
    }

    if (strcmp(argv[1], "listen") == 0) {
        return run_listener(argv[2], argv[3], hold);
    }
    if (strcmp(argv[1], "connect") == 0) {
        return run_connector(argv[2], argv[3], hold);
    }

    usage(argv[0]);
    return 2;
}
