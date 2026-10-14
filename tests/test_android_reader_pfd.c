#define _POSIX_C_SOURCE 200809L
#include "../native/android/ib_reader_pfd.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static const char *abc_digest =
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
static const char *hello_digest =
    "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";
static const char *binary_digest =
    "c8ac851bcdb2785ab14adb0b61e4743865e6999e03481b9409b67de2cdc29c4a";

static void must_write(int fd, const unsigned char *bytes, size_t count) {
    while (count != 0) {
        ssize_t written = write(fd, bytes, count);
        if (written < 0 && errno == EINTR) continue;
        assert(written > 0);
        bytes += (size_t)written;
        count -= (size_t)written;
    }
}

/* Host simulation only: SCM_RIGHTS carries an anonymous pipe between two
 * independent processes. Android uses Binder's ParcelFileDescriptor instead. */
static void send_descriptor(int socket, int descriptor) {
    char byte = 'F';
    struct iovec segment = { &byte, sizeof(byte) };
    union {
        struct cmsghdr align;
        unsigned char space[CMSG_SPACE(sizeof(int))];
    } control;
    memset(&control, 0, sizeof(control));
    struct msghdr message = {0};
    message.msg_iov = &segment;
    message.msg_iovlen = 1;
    message.msg_control = control.space;
    message.msg_controllen = sizeof(control.space);
    struct cmsghdr *header = CMSG_FIRSTHDR(&message);
    assert(header != NULL);
    header->cmsg_level = SOL_SOCKET;
    header->cmsg_type = SCM_RIGHTS;
    header->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(header), &descriptor, sizeof(descriptor));
    assert(sendmsg(socket, &message, 0) == 1);
}

static int receive_descriptor(int socket) {
    char byte = '\0';
    struct iovec segment = { &byte, sizeof(byte) };
    union {
        struct cmsghdr align;
        unsigned char space[CMSG_SPACE(sizeof(int))];
    } control;
    memset(&control, 0, sizeof(control));
    struct msghdr message = {0};
    message.msg_iov = &segment;
    message.msg_iovlen = 1;
    message.msg_control = control.space;
    message.msg_controllen = sizeof(control.space);
    assert(recvmsg(socket, &message, 0) == 1);
    assert(byte == 'F' && (message.msg_flags & MSG_CTRUNC) == 0);
    struct cmsghdr *header = CMSG_FIRSTHDR(&message);
    assert(header != NULL && header->cmsg_level == SOL_SOCKET);
    assert(header->cmsg_type == SCM_RIGHTS);
    assert(header->cmsg_len == CMSG_LEN(sizeof(int)));
    int descriptor = -1;
    memcpy(&descriptor, CMSG_DATA(header), sizeof(descriptor));
    assert(descriptor >= 0);
    return descriptor;
}

static int transferred_stream(const unsigned char *data, size_t size) {
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        close(sockets[0]);
        int stream[2];
        if (pipe(stream) != 0) _exit(33);
        send_descriptor(sockets[1], stream[0]);
        close(stream[0]);
        if (size != 0) must_write(stream[1], data, size);
        close(stream[1]);
        close(sockets[1]);
        _exit(0);
    }
    close(sockets[1]);
    int descriptor = receive_descriptor(sockets[0]);
    close(sockets[0]);
    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    return descriptor;
}

static ib_reader_source_ticket ticket(const char *name, uint64_t generation,
                                      uint32_t count, const char *digest) {
    ib_reader_source_ticket value = {0};
    assert(strlen(name) <= IB_READER_REQUEST_ID_CAP);
    memcpy(value.request_id, name, strlen(name) + 1U);
    value.navigation_generation = generation;
    value.byte_count = count;
    assert(strlen(digest) == 64U);
    memcpy(value.sha256_hex, digest, 65U);
    return value;
}

static void intact(const ib_reader_source *source, const unsigned char *bytes,
                   size_t count, const char *request, uint64_t generation) {
    assert(source->bytes != NULL);
    assert(source->byte_count == count);
    assert(memcmp(source->bytes, bytes, count) == 0);
    assert(strcmp(source->request_id, request) == 0);
    assert(source->published_navigation_generation == generation);
}

int main(void) {
    static const unsigned char abc[] = "abc";
    static const unsigned char hello[] = "hello";
    static const unsigned char binary[] = {'b','i','n',0,'a','r','y',255,'\n'};
    ib_reader_source source;
    ib_reader_source_init(&source);
    ib_reader_source_want(&source, 7U);

    ib_reader_source_ticket first = ticket("first", 7, 3, abc_digest);
    int fd = transferred_stream(abc, 3);
    assert(ib_reader_source_accept_fd(&source, &first, fd) == IB_READER_PFD_ACCEPTED);
    close(fd);
    intact(&source, abc, 3, "first", 7);
    assert(source.bytes[3] == '\0');

    /* Corrupt stream and metadata never evict the last published source. */
    ib_reader_source_ticket invalid_digest = ticket("broken", 7, 5, hello_digest);
    invalid_digest.sha256_hex[0] = '0';
    fd = transferred_stream(hello, 5);
    assert(ib_reader_source_accept_fd(&source, &invalid_digest, fd) == IB_READER_PFD_DIGEST);
    close(fd);
    intact(&source, abc, 3, "first", 7);

    fd = transferred_stream(abc, 2);
    assert(ib_reader_source_accept_fd(&source, &first, fd) == IB_READER_PFD_IO);
    close(fd);
    intact(&source, abc, 3, "first", 7);

    static const unsigned char longer[] = "abcX";
    fd = transferred_stream(longer, 4);
    assert(ib_reader_source_accept_fd(&source, &first, fd) == IB_READER_PFD_IO);
    close(fd);
    intact(&source, abc, 3, "first", 7);

    ib_reader_source_ticket invalid_name = ticket("..bad", 7, 3, abc_digest);
    invalid_name.request_id[0] = '/';
    fd = transferred_stream(abc, 3);
    assert(ib_reader_source_accept_fd(&source, &invalid_name, fd) == IB_READER_PFD_BAD_TICKET);
    close(fd);
    intact(&source, abc, 3, "first", 7);

    ib_reader_source_ticket too_large = ticket("large", 7, IB_READER_SOURCE_MAX_BYTES+1,
                                               abc_digest);
    fd = transferred_stream(abc, 3);
    assert(ib_reader_source_accept_fd(&source, &too_large, fd) == IB_READER_PFD_BAD_TICKET);
    close(fd);
    intact(&source, abc, 3, "first", 7);

    /* New navigation invalidates an old PFD even if its hash is valid. */
    ib_reader_source_want(&source, 8U);
    fd = transferred_stream(abc, 3);
    assert(ib_reader_source_accept_fd(&source, &first, fd) == IB_READER_PFD_STALE);
    close(fd);
    intact(&source, abc, 3, "first", 7);

    ib_reader_source_ticket second = ticket("second", 8, 5, hello_digest);
    fd = transferred_stream(hello, 5);
    assert(ib_reader_source_accept_fd(&source, &second, fd) == IB_READER_PFD_ACCEPTED);
    close(fd);
    intact(&source, hello, 5, "second", 8);

    ib_reader_source_want(&source, 9U);
    ib_reader_source_ticket third = ticket("binary-source", 9, sizeof(binary), binary_digest);
    fd = transferred_stream(binary, sizeof(binary));
    assert(ib_reader_source_accept_fd(&source, &third, fd) == IB_READER_PFD_ACCEPTED);
    close(fd);
    intact(&source, binary, sizeof(binary), "binary-source", 9);

    ib_reader_source_clear(&source);
    assert(source.bytes == NULL && source.byte_count == 0U);
    puts("PASS: two-process descriptor transfer, SHA-256, bounded exact binary, stale/partial/extra/corrupt/oversized refusal, last-good preservation");
    puts("NOT_RUN: Android Binder service, Shizuku, native Idric screen wiring, actual C67");
    return 0;
}
