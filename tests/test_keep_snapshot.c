#define _POSIX_C_SOURCE 200809L
#include "../native/android/ib_keep_snapshot.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
    char dirname[] = "/tmp/ib-keep-snapshot-XXXXXX";
    char *dir = mkdtemp(dirname);
    assert(dir);
    assert(chmod(dir, 0700) == 0);
    int fd = open(dir, O_RDONLY | O_DIRECTORY);
    assert(fd >= 0);
    const unsigned char pending[] = "ib-keep-delivery-v1\tkeep.decision-1\tin-flight\t\n";
    const unsigned char delivered[] = "ib-keep-delivery-v1\tkeep.decision-1\tawaiting-user\t\n";
    unsigned char readback[IB_KEEP_SNAPSHOT_MAX];
    size_t length = 0;
    assert(!ib_keep_snapshot_replace(fd, "delivery.tsv", pending, sizeof pending - 1));
    assert(!ib_keep_snapshot_read(fd, "delivery.tsv", readback, sizeof readback, &length));
    assert(length == sizeof pending - 1);
    assert(!memcmp(readback, pending, length));
    assert(close(fd) == 0); /* simulate process restart */
    fd = open(dir, O_RDONLY | O_DIRECTORY);
    assert(fd >= 0);
    assert(!ib_keep_snapshot_read(fd, "delivery.tsv", readback, sizeof readback, &length));
    assert(length == sizeof pending - 1);
    assert(!memcmp(readback, pending, length));
    assert(!ib_keep_snapshot_replace(fd, "delivery.tsv", delivered, sizeof delivered - 1));
    assert(!ib_keep_snapshot_read(fd, "delivery.tsv", readback, sizeof readback, &length));
    assert(length == sizeof delivered - 1);
    assert(!memcmp(readback, delivered, length));
    assert(ib_keep_snapshot_replace(fd, "../unsafe", pending, sizeof pending - 1) < 0);
    assert(ib_keep_snapshot_replace(fd, "delivery.tsv", pending, 513) < 0);
    assert(ib_keep_snapshot_read(fd, "delivery.tsv", readback, 1, &length) < 0);
    assert(unlinkat(fd, "delivery.tsv", 0) == 0);
    assert(symlinkat("/etc/passwd", fd, "delivery.tsv") == 0);
    assert(ib_keep_snapshot_read(fd, "delivery.tsv", readback, sizeof readback, &length) < 0);
    assert(unlinkat(fd, "delivery.tsv", 0) == 0);
    assert(close(fd) == 0);
    assert(rmdir(dir) == 0);
    puts("keep-snapshot-atomic-recovery=PASS");
    return 0;
}
