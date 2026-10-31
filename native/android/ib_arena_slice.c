#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "ib_redirect_observations.h"

#ifndef O_NOFOLLOW
#error "This reader requires O_NOFOLLOW"
#endif
#ifndef O_DIRECTORY
#error "This reader requires O_DIRECTORY"
#endif

#define MAX_SOURCE_BYTES (4U * 1024U * 1024U)
#define BUFFER_BYTES 65536U

static int error(const char *what) {
    fprintf(stderr, "ib-arena-slice: %s: %s\n", what, strerror(errno));
    return 1;
}

static int number(const char *text, uint64_t *out) {
    char *end;
    uintmax_t parsed;
    if (*text == '\0' || *text == '-' || *text == '+') return -1;
    errno = 0;
    parsed = strtoumax(text, &end, 10);
    if (errno != 0 || *end != '\0' || parsed > INT64_MAX) return -1;
    *out = (uint64_t)parsed;
    return 0;
}

static char *suffix(const char *path, const char *extension) {
    size_t a = strlen(path), b = strlen(extension);
    if (a > SIZE_MAX - b - 1) return NULL;
    char *name = malloc(a + b + 1);
    if (!name) return NULL;
    memcpy(name, path, a);
    memcpy(name + a, extension, b + 1);
    return name;
}

static int read_used(int fd, uint64_t *used) {
    char buffer[64];
    ssize_t n;
    do { n = read(fd, buffer, sizeof(buffer) - 1); }
    while (n < 0 && errno == EINTR);
    if (n <= 0 || n >= (ssize_t)sizeof(buffer) - 1) return -1;
    buffer[n] = '\0';
    if (buffer[n - 1] != '\n') return -1;
    buffer[n - 1] = '\0';
    return number(buffer, used);
}

static int write_all(const unsigned char *bytes, size_t length) {
    size_t offset = 0;
    while (offset < length) {
        ssize_t n = write(STDOUT_FILENO, bytes + offset, length - offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        offset += (size_t)n;
    }
    return 0;
}

static int dump_slice(const char *arena, uint64_t offset, uint64_t length) {
    char *lock_path = suffix(arena, ".lock");
    char *used_path = suffix(arena, ".used");
    int lock = -1, descriptor = -1, used_fd = -1, result = 1;
    unsigned char buffer[BUFFER_BYTES];
    struct stat info;
    uint64_t committed;
    if (!lock_path || !used_path) goto end;
    lock = open(lock_path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (lock < 0 || flock(lock, LOCK_SH) != 0) goto end;
    descriptor = open(arena, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    used_fd = open(used_path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (descriptor < 0 || used_fd < 0) goto end;
    if (fstat(descriptor, &info) != 0 || !S_ISREG(info.st_mode) ||
        read_used(used_fd, &committed) != 0 ||
        info.st_size < 0 || committed > (uint64_t)info.st_size ||
        length > MAX_SOURCE_BYTES || offset > committed || length > committed - offset) {
        errno = EINVAL;
        goto end;
    }
    while (length) {
        size_t take = length < BUFFER_BYTES ? (size_t)length : BUFFER_BYTES;
        ssize_t n = pread(descriptor, buffer, take, (off_t)offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0 || write_all(buffer, (size_t)n) != 0) {
            errno = EIO;
            goto end;
        }
        offset += (uint64_t)n;
        length -= (uint64_t)n;
    }
    result = 0;
end:
    if (result) error("invalid or unreadable committed arena slice");
    if (used_fd >= 0) close(used_fd);
    if (descriptor >= 0) close(descriptor);
    if (lock >= 0) close(lock);
    free(lock_path);
    free(used_path);
    return result;
}

static int sync_path_and_directory(const char *path) {
    int fd = -1, directory_fd = -1, result = 1;
    char *parent = strdup(path);
    if (!parent) return error("could not allocate parent pathname");
    char *slash = strrchr(parent, '/');
    if (slash == parent) parent[1] = '\0';
    else if (slash) *slash = '\0';
    else { free(parent); parent = strdup("."); }
    if (!parent) return error("could not allocate current directory");
    fd = open(path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0 || fsync(fd) != 0) goto done;
    directory_fd = open(parent, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (directory_fd < 0 || fsync(directory_fd) != 0) goto done;
    result = 0;
done:
    if (result) error("cannot sync file and its parent directory");
    if (fd >= 0) close(fd);
    if (directory_fd >= 0) close(directory_fd);
    free(parent);
    return result;
}

int main(int argc, char **argv) {
    if (argc == 6 && strcmp(argv[1], "redirects") == 0) {
        uint64_t count = 0;
        if (number(argv[4], &count) != 0 || count > 8U ||
            !ib_redirect_chain_valid(argv[5], argv[2], argv[3], (unsigned)count)) {
            fprintf(stderr, "ib-arena-slice: refused redirect observation chain\n");
            return 1;
        }
        return 0;
    }
    uint64_t offset, length;
    if (argc == 3 && strcmp(argv[1], "sync") == 0)
        return sync_path_and_directory(argv[2]);
    if (argc == 5 && strcmp(argv[1], "slice") == 0 &&
        number(argv[3], &offset) == 0 && number(argv[4], &length) == 0)
        return dump_slice(argv[2], offset, length);
    fprintf(stderr, "usage: ib-arena-slice slice ARENA OFFSET LENGTH | sync PATH | redirects REQUESTED RESOLVED COUNT CHAIN\n");
    return 2;
}
