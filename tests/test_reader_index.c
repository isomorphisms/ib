/* Real ordinary-file metadata owner with compiled write/sync/rename faults.
 * No simulated Arena publication or Android/physical claim is made here. */
#define _GNU_SOURCE
#include "ib_reader_index.h"
#include "ib_reader_pfd.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum fault { NONE, SHORT_WRITES, FULL, PARTIAL_FULL, FILE_SYNC, DIRECTORY_SYNC, RENAME_DENIED };
static enum fault fault;
static unsigned writes;
ssize_t __real_write(int, const void *, size_t);
int __real_fsync(int);
int __real_renameat(int, const char *, int, const char *);
ssize_t __wrap_write(int fd, const void *bytes, size_t size) {
    if (fault == FULL || (fault == PARTIAL_FULL && writes++ > 0)) { errno = ENOSPC; return -1; }
    if (fault == SHORT_WRITES && size > 7) size = 7;
    if (fault == PARTIAL_FULL && size > 3) size = 3;
    return __real_write(fd, bytes, size);
}
int __wrap_fsync(int fd) {
    struct stat s;
    assert(fstat(fd, &s) == 0);
    if ((fault == FILE_SYNC && S_ISREG(s.st_mode)) || (fault == DIRECTORY_SYNC && S_ISDIR(s.st_mode))) {
        errno = EIO; return -1;
    }
    return __real_fsync(fd);
}
int __wrap_renameat(int oldfd, const char *old, int newfd, const char *name) {
    if (fault == RENAME_DENIED) { errno = EACCES; return -1; }
    return __real_renameat(oldfd, old, newfd, name);
}

static ib_reader_index occurrence(const char *identity, unsigned generation) {
    ib_reader_index result = {0};
    assert(strlen(identity) < sizeof(result.source_id));
    strcpy(result.source_id, identity);
    result.source_generation = generation;
    int n = snprintf(result.journal, sizeof(result.journal), "IBH1\n0\n%s\t%u\n", identity, generation);
    assert(n > 0 && (size_t)n < sizeof(result.journal));
    return result;
}
static void current(int root, const char *identity, unsigned generation, int previous) {
    ib_reader_index observed;
    assert(ib_reader_index_read(root, 0, &observed) == 0);
    assert(!strcmp(observed.source_id, identity));
    assert(observed.source_generation == generation);
    assert(observed.previous == previous);
}
static void leaf(int root, const char *name, const char *bytes, size_t size) {
    int fd = openat(root, name, O_WRONLY | O_TRUNC | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
    assert(fd >= 0);
    assert(__real_write(fd, bytes, size) == (ssize_t)size);
    assert(close(fd) == 0);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    char directory[] = "/tmp/ib-reader-index-test.XXXXXX";
    assert(mkdtemp(directory));
    int root = open(directory, O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
    assert(root >= 0 && fchmod(root, 0700) == 0);
    ib_reader_index observed;
    assert(ib_reader_index_read(root, 0, &observed) == 1);
    ib_reader_index first = occurrence("first", 1), second = occurrence("second", 2), third = occurrence("third", 3);
    puts("STAGE index short-write completion");
    fault = SHORT_WRITES;
    assert(ib_reader_index_publish(root, &first) == 0);
    fault = NONE;
    current(root, "first", 1, 0);
    assert(ib_reader_index_publish(root, &second) == 0);
    assert(ib_reader_index_read(root, 1, &observed) == 0 && !strcmp(observed.source_id, "first"));
    puts("STAGE index no-space interrupted-write sync and rename refusal");
    const enum fault faults[] = {FULL, PARTIAL_FULL, FILE_SYNC, RENAME_DENIED};
    for (size_t i = 0; i < sizeof(faults)/sizeof(faults[0]); ++i) {
        fault = faults[i]; writes = 0;
        assert(ib_reader_index_publish(root, &third) == -1);
        fault = NONE;
        current(root, "second", 2, 0);
    }
    puts("STAGE index digest refusal and previous recovery");
    int fd = openat(root, "last-source", O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    assert(fd >= 0);
    /* Change a valid generation without repairing its full-prefix SHA. */
    char changed = '3';
    assert(pwrite(fd, &changed, 1, 8 + (off_t)strlen("second") + 1) == 1);
    assert(close(fd) == 0);
    assert(ib_reader_index_read(root, 0, &observed) == 0);
    assert(observed.previous == 1 && !strcmp(observed.source_id, "first") && observed.source_generation == 1);
    observed.journal[0] = 'I';
    assert(ib_reader_index_publish(root, &observed) == 0);
    current(root, "first", 1, 0);
    assert(ib_reader_index_publish(root, &second) == 0);
    puts("STAGE index unsafe aliases and permissions refused");
    assert(linkat(root, "last-source", root, "alias", 0) == 0);
    current(root, "first", 1, 1);
    assert(ib_reader_index_publish(root, &third) == -1);
    assert(unlinkat(root, "alias", 0) == 0);
    assert(fchmodat(root, "last-source", 0644, 0) == 0);
    current(root, "first", 1, 1);
    assert(ib_reader_index_publish(root, &third) == -1);
    assert(fchmodat(root, "last-source", 0600, 0) == 0);
    assert(renameat(root, "last-source", root, "saved") == 0);
    assert(symlinkat("last-source.previous", root, "last-source") == 0);
    current(root, "first", 1, 1);
    assert(ib_reader_index_publish(root, &third) == -1);
    assert(unlinkat(root, "last-source", 0) == 0);
    assert(renameat(root, "saved", root, "last-source") == 0);
    assert(fchmod(root, 0755) == 0);
    assert(ib_reader_index_read(root, 0, &observed) == -1);
    assert(ib_reader_index_publish(root, &third) == -1);
    assert(fchmod(root, 0700) == 0);
    puts("STAGE index directory sync failure is an uncertain commit");
    /* Keep the same body occurrence, change only the opaque cursor journal.
     * A failure after backup rename leaves current intact. If the head rename
     * already happened, both complete digests still verify independently. */
    first.previous = 1;
    fault = DIRECTORY_SYNC;
    assert(ib_reader_index_publish(root, &first) == -1);
    fault = NONE;
    current(root, "first", 1, 0);
    assert(ib_reader_index_read(root, 1, &observed) == 0 && !strcmp(observed.source_id, "first"));
    puts("STAGE index torn metadata and unpublished temporary ignored");
    leaf(root, "last-source", "torn", 4);
    leaf(root, "last-source.new.unpublished", "not a committed head", 20);
    current(root, "first", 1, 1);
    leaf(root, "last-source.previous", "torn", 4);
    memset(&observed, 0xab, sizeof(observed));
    assert(ib_reader_index_read(root, 0, &observed) == -1);
    for (size_t i = 0; i < sizeof(observed); ++i) assert(((unsigned char *)&observed)[i] == 0);
    puts("STAGE index legacy migration");
    char legacy[200], digest[65];
    int count = snprintf(legacy, sizeof(legacy), "IBLAST1\nlegacy\n1\n");
    ib_reader_sha256_hex((unsigned char *)legacy, (size_t)count, digest);
    memcpy(legacy + count, digest, 64); legacy[count + 64] = '\n';
    leaf(root, "last-source", legacy, (size_t)count + 65U);
    assert(ib_reader_index_read(root, 0, &observed) == 0 && !strcmp(observed.source_id, "legacy") && !observed.journal[0]);
    ib_reader_index migrated = occurrence("legacy", 1);
    assert(ib_reader_index_publish(root, &migrated) == 0);
    assert(ib_reader_index_read(root, 0, &observed) == 0 && !strcmp(observed.journal, migrated.journal));
    assert(ib_reader_index_read(root, 1, &observed) == 0 && !observed.journal[0]);
    close(root);
    puts("PASS actual reader index durability faults and previous-head recovery");
    return 0;
}
