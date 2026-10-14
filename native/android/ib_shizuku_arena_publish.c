#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include "ib_shizuku_arena_publish.h"
#include "ib_shizuku_arena_ingress.h"
#include "ib_reader_pfd.h"
#include "appendfat_arena.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define RECEIPT_MAX_BYTES 8192U
#define READBACK_CHUNK 65536U

static int valid_id(const char *id) {
    if (!id) return 0;
    size_t n = strnlen(id, IB_READER_REQUEST_ID_CAP + 1U);
    if (!n || n > IB_READER_REQUEST_ID_CAP ||
        strcmp(id, ".") == 0 || strcmp(id, "..") == 0) return 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)id[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              c == '.' || c == '-' || c == '_')) return 0;
    }
    return 1;
}

static int valid_url(const char *url) {
    if (!url) return 0;
    size_t n = strnlen(url, IB_INGRESS_URL_CAP + 1U);
    if (n < 9 || n > IB_INGRESS_URL_CAP ||
        (strncmp(url, "https://", 8) != 0 &&
         strncmp(url, "http://", 7) != 0)) return 0;
    size_t authority = strncmp(url, "https://", 8) == 0 ? 8U : 7U;
    if (!url[authority] || url[authority] == '/' ||
        url[authority] == '?' || url[authority] == '#') return 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)url[i];
        if (c < 33U || c > 126U || c == '\'') return 0;
        if (i >= authority && c == '@') {
            size_t j = i;
            while (j > authority && url[j-1] != '/' &&
                   url[j-1] != '?' && url[j-1] != '#') --j;
            if (j == authority) return 0;
        }
    }
    return 1;
}

static int owned_directory(int fd) {
    struct stat info;
    return fd >= 0 && fstat(fd, &info) == 0 &&
           S_ISDIR(info.st_mode) && info.st_uid == geteuid() &&
           (info.st_mode & 077U) == 0;
}

static int owned_regular(int parent, const char *name) {
    int fd = openat(parent, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -1;
    struct stat info;
    int result = fstat(fd, &info) == 0 && S_ISREG(info.st_mode) &&
                 info.st_uid == geteuid() && info.st_nlink == 1;
    close(fd);
    return result ? 0 : -1;
}

static int write_all(int fd, const char *text, size_t count) {
    size_t written = 0;
    while (written < count) {
        ssize_t got = write(fd, text + written, count - written);
        if (got < 0 && errno == EINTR) continue;
        if (got <= 0) return -1;
        written += (size_t)got;
    }
    return 0;
}

static int receipt_publish(int receipts_dir, const char *id,
                           const char *url, uint64_t offset,
                           size_t length, const char digest[65]) {
    char receipt_name[IB_READER_REQUEST_ID_CAP + 5U];
    char temporary_name[IB_READER_REQUEST_ID_CAP + 48U];
    char content[RECEIPT_MAX_BYTES];
    int n = snprintf(receipt_name, sizeof(receipt_name), "%s.tsv", id);
    int t = snprintf(temporary_name, sizeof(temporary_name),
                     ".%s.%ld.pending", id, (long)getpid());
    if (n <= 0 || (size_t)n >= sizeof(receipt_name) ||
        t <= 0 || (size_t)t >= sizeof(temporary_name)) return -1;
    n = snprintf(content, sizeof(content),
                 "schema\tib-icu-arena-v1\n"
                 "request_id\t%s\n"
                 "requested_url\t%s\n"
                 "resolved_url\tUNOBSERVED\n"
                 "http_status\tUNOBSERVED\n"
                 "mime_type\tUNOBSERVED\n"
                 "transport\ticu-app-process-via-binder-shizuku\n"
                 "arena_name\tresponses.arena\n"
                 "arena_offset\t%" PRIu64 "\n"
                 "arena_end\t%" PRIu64 "\n"
                 "byte_count\t%zu\n"
                 "sha256\t%s\n"
                 "reopen_check\tMATCH\n"
                 "navigation_admitted\tNO\n",
                 id, url, offset, offset + (uint64_t)length, length, digest);
    if (n <= 0 || (size_t)n >= sizeof(content)) return -1;

    int fd = openat(receipts_dir, temporary_name,
                    O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (fd < 0) return -1;
    int ok = write_all(fd, content, (size_t)n) == 0 && fsync(fd) == 0;
    if (close(fd) != 0) ok = 0;
    if (!ok) {
        (void)unlinkat(receipts_dir, temporary_name, 0);
        return -1;
    }
    /* Immutable source receipt appears atomically only after source readback
     * has succeeded and the complete staged receipt was synced. */
    int linked = linkat(receipts_dir, temporary_name,
                        receipts_dir, receipt_name, 0) == 0;
    (void)unlinkat(receipts_dir, temporary_name, 0);
    if (!linked) return -1;
    if (fsync(receipts_dir) != 0) {
        (void)unlinkat(receipts_dir, receipt_name, 0);
        (void)fsync(receipts_dir);
        return -1;
    }
    return 0;
}

int ib_ingress_arena_init(ib_ingress_arena *arena, const char *private_root) {
    if (!arena || !private_root || private_root[0] != '/') return -1;
    size_t n = strnlen(private_root, IB_INGRESS_ARENA_ROOT_CAP);
    if (!n || n >= IB_INGRESS_ARENA_ROOT_CAP) return -1;
    memset(arena, 0, sizeof(*arena));
    memcpy(arena->private_root, private_root, n + 1U);
    return 0;
}

int ib_ingress_arena_publish(void *opaque, const char *request_id,
                             const char *requested_url,
                             const unsigned char *exact_bytes,
                             size_t byte_count,
                             const char sha256_hex[65]) {
    ib_ingress_arena *publisher = opaque;
    if (!publisher || !valid_id(request_id) || !valid_url(requested_url) ||
        !exact_bytes || !sha256_hex || byte_count == 0 ||
        byte_count > IB_READER_SOURCE_MAX_BYTES ||
        strnlen(sha256_hex, 65U) != 64U) return -1;
    char computed[65];
    ib_reader_sha256_hex(exact_bytes, byte_count, computed);
    if (memcmp(sha256_hex, computed, 65U) != 0) return -1;

    int root = -1, locks = -1, receipts = -1, result = -1;
    int arena_is_open = 0;
    char arena_path[IB_INGRESS_ARENA_ROOT_CAP + 24U];
    appendfat_arena canonical = {0};
    struct stat info;
    int n = snprintf(arena_path, sizeof(arena_path),
                     "%s/responses.arena", publisher->private_root);
    if (n <= 0 || (size_t)n >= sizeof(arena_path)) return -1;

    root = open(publisher->private_root,
                O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (!owned_directory(root)) goto done;
    locks = openat(root, "locks",
                   O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    receipts = openat(root, "receipts",
                      O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (!owned_directory(locks) || !owned_directory(receipts) ||
        owned_regular(root, "responses.arena") != 0 ||
        owned_regular(root, "responses.arena.used") != 0 ||
        owned_regular(root, "responses.arena.lock") != 0)
        goto done;

    /* Bind request identity before changing the canonical commit pointer.
     * Lock remains after failure for audit and prevents retries with same ID. */
    if (mkdirat(locks, request_id, 0700) != 0) goto done;
    if (appendfat_arena_open(&canonical, arena_path, 1) != 0) goto done;
    arena_is_open = 1;
    uint64_t old_used = appendfat_arena_committed(&canonical);
    if (old_used > appendfat_arena_capacity(&canonical) ||
        byte_count > appendfat_arena_capacity(&canonical) - old_used)
        goto done;
    if (appendfat_arena_append(&canonical, exact_bytes, byte_count) != 0) {
        appendfat_arena_discard_uncommitted(&canonical);
        goto done;
    }
    if (appendfat_arena_commit(&canonical) != 0) goto done;
    if (appendfat_arena_committed(&canonical) - old_used != byte_count)
        goto done;
    appendfat_arena_close(&canonical);
    arena_is_open = 0;

    if (appendfat_arena_open(&canonical, arena_path, 0) != 0) goto done;
    arena_is_open = 1;
    if (appendfat_arena_committed(&canonical) <
        old_used + (uint64_t)byte_count) goto done;
    size_t read_back = 0;
    unsigned char block[READBACK_CHUNK];
    while (read_back < byte_count) {
        size_t max = byte_count - read_back, amount = 0;
        if (max > sizeof(block)) max = sizeof(block);
        if (appendfat_arena_read(&canonical,
                                 old_used + (uint64_t)read_back,
                                 block, max, &amount) != 0 ||
            amount == 0 ||
            memcmp(block, exact_bytes + read_back, amount) != 0)
            goto done;
        read_back += amount;
    }
    appendfat_arena_close(&canonical);
    arena_is_open = 0;
    if (fstat(receipts, &info) != 0 || !S_ISDIR(info.st_mode))
        goto done;
    if (receipt_publish(receipts, request_id, requested_url,
                        old_used, byte_count, computed) != 0)
        goto done;
    result = 0;
done:
    if (arena_is_open) appendfat_arena_close(&canonical);
    if (receipts >= 0) close(receipts);
    if (locks >= 0) close(locks);
    if (root >= 0) close(root);
    return result;
}
