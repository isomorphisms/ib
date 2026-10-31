#define _POSIX_C_SOURCE 200809L
#include "ib_keep_snapshot.h"
#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static atomic_uint next_temp = 1;

static int good_name(const char *name)
{
    size_t n = 0;
    if (!name || !*name || !strcmp(name, ".") || !strcmp(name, ".."))
        return 0;
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p) {
        unsigned char c = *p;
        if (++n > 80 || !((c >= 'a' && c <= 'z') ||
                          (c >= 'A' && c <= 'Z') ||
                          (c >= '0' && c <= '9') || c == '_' ||
                          c == '-' || c == '.'))
            return 0;
    }
    return 1;
}

static int write_all(int fd, const unsigned char *bytes, size_t length)
{
    size_t at = 0;
    while (at < length) {
        ssize_t n = write(fd, bytes + at, length - at);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        at += (size_t)n;
    }
    return 0;
}

int ib_keep_snapshot_replace(int dirfd, const char *name,
                             const unsigned char *bytes, size_t length)
{
    char tmp[128];
    int fd = -1, saved = 0, created = 0;
    struct stat folder;
    if (dirfd < 0 || !good_name(name) || !bytes || !length ||
        length > IB_KEEP_SNAPSHOT_MAX || fstat(dirfd, &folder) ||
        !S_ISDIR(folder.st_mode) || (folder.st_mode & 0077)) {
        errno = EINVAL;
        return -1;
    }
    for (unsigned i = 0; i < 32; ++i) {
        unsigned serial = atomic_fetch_add(&next_temp, 1);
        int n = snprintf(tmp, sizeof tmp, ".%s.%ld.%u.tmp", name,
                         (long)getpid(), serial);
        if (n < 0 || (size_t)n >= sizeof tmp) { errno = ENAMETOOLONG; return -1; }
        fd = openat(dirfd, tmp, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (fd >= 0) { created = 1; break; }
        if (errno != EEXIST) return -1;
    }
    if (!created) { errno = EEXIST; return -1; }
    if (write_all(fd, bytes, length) || fsync(fd)) goto failed;
    if (close(fd)) { fd = -1; goto failed; }
    fd = -1;
    if (renameat(dirfd, tmp, dirfd, name)) goto failed;
    /* Failure here means the new snapshot may exist but is not known durable. */
    if (fsync(dirfd)) return -1;
    return 0;
failed:
    saved = errno;
    if (fd >= 0) close(fd);
    unlinkat(dirfd, tmp, 0);
    errno = saved;
    return -1;
}

int ib_keep_snapshot_read(int dirfd, const char *name,
                          unsigned char *bytes, size_t capacity, size_t *length)
{
    struct stat folder, file;
    int fd, saved;
    size_t at = 0;
    if (dirfd < 0 || !good_name(name) || !bytes || !length ||
        capacity > IB_KEEP_SNAPSHOT_MAX || fstat(dirfd, &folder) ||
        !S_ISDIR(folder.st_mode) || (folder.st_mode & 0077)) {
        errno = EINVAL;
        return -1;
    }
    fd = openat(dirfd, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -1;
    if (fstat(fd, &file) || !S_ISREG(file.st_mode) || file.st_size < 1 ||
        (uintmax_t)file.st_size > capacity || (file.st_mode & 0077)) {
        saved = EINVAL;
        goto fail;
    }
    while (at < (size_t)file.st_size) {
        ssize_t n = read(fd, bytes + at, (size_t)file.st_size - at);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { saved = EIO; goto fail; }
        at += (size_t)n;
    }
    unsigned char excess;
    ssize_t extra = read(fd, &excess, 1);
    if (extra != 0) { saved = EIO; goto fail; }
    if (close(fd)) return -1;
    *length = at;
    return 0;
fail:
    close(fd);
    errno = saved;
    return -1;
}
