/* Narrow filesystem/byte ABI for the checked reader's opaque history. */
#define _GNU_SOURCE
#include "ib_reader_index.h"
#include "ib_reader_pfd.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <unistd.h>

#define INDEX_CAP (IB_READER_JOURNAL_CAP + 256U)
static const char *const names[] = {"last-source", "last-source.previous"};

static int private_root(int fd) {
    struct stat s;
    return fd >= 0 && fstat(fd, &s) == 0 && S_ISDIR(s.st_mode) &&
        s.st_uid == geteuid() && (s.st_mode & 077U) == 0;
}

static int valid_identity(const char *id) {
    size_t n = strnlen(id, 65);
    if (!n || n > 64 || !strcmp(id, ".") || !strcmp(id, "..")) return 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}

static int valid_journal(const char *journal) {
    size_t n = strnlen(journal, IB_READER_JOURNAL_CAP + 1U);
    if (!n || n > IB_READER_JOURNAL_CAP || journal[n - 1] != '\n') return 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)journal[i];
        if (c != '\t' && c != '\n' && (c < 32 || c > 126)) return 0;
    }
    return 1;
}

/* Invalid ordinary contents differ from unsafe file authority. A torn owned
 * head can recover; an alias, symlink or permissive file cannot be replaced
 * as though it were a valid current head. 1 means absent, -2 unsafe. */
static int read_leaf(int root, const char *name, ib_reader_index *out,
                     char bytes[INDEX_CAP + 1U], size_t *size) {
    memset(out, 0, sizeof(*out));
    *size = 0;
    int fd = openat(root, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return errno == ENOENT ? 1 : -2;
    struct stat s;
    int safe = fstat(fd, &s) == 0 && S_ISREG(s.st_mode) && s.st_uid == geteuid() &&
        s.st_nlink == 1 && (s.st_mode & 077U) == 0;
    if (!safe) { close(fd); return -2; }
    int ok = s.st_size > 0 && (uint64_t)s.st_size <= INDEX_CAP;
    size_t used = 0;
    while (ok && used < (size_t)s.st_size) {
        ssize_t n = read(fd, bytes + used, (size_t)s.st_size - used);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { ok = 0; break; }
        used += (size_t)n;
    }
    char extra;
    if (ok) ok = read(fd, &extra, 1) == 0;
    if (close(fd) != 0) ok = 0;
    if (!ok || used < 77 || memchr(bytes, 0, used) || bytes[used - 1] != '\n') return -1;
    bytes[used] = 0;
    size_t prefix = used - 65U;
    if (!prefix || bytes[prefix - 1] != '\n') return -1;
    char actual[65];
    ib_reader_sha256_hex((unsigned char *)bytes, prefix, actual);
    if (memcmp(actual, bytes + prefix, 64)) return -1;
    int legacy = !memcmp(bytes, "IBLAST1\n", 8);
    if (!legacy && memcmp(bytes, "IBLAST2\n", 8)) return -1;
    char *id_end = memchr(bytes + 8, '\n', prefix - 8U);
    if (!id_end || id_end - bytes - 8 < 1 || id_end - bytes - 8 > 64) return -1;
    memcpy(out->source_id, bytes + 8, (size_t)(id_end - bytes - 8));
    if (!valid_identity(out->source_id)) goto invalid;
    char *generation = id_end + 1;
    char *generation_end = memchr(generation, '\n', prefix - (size_t)(generation - bytes));
    if (!generation_end || generation_end == generation || *generation == '0' || generation_end - generation > 10) goto invalid;
    uint64_t value = 0;
    for (char *p = generation; p < generation_end; ++p) {
        if (*p < '0' || *p > '9') goto invalid;
        value = value * 10U + (unsigned)(*p - '0');
    }
    if (!value || value > INT32_MAX) goto invalid;
    out->source_generation = (uint32_t)value;
    size_t journal_size = prefix - (size_t)(generation_end + 1 - bytes);
    if (legacy ? journal_size != 0 : journal_size < 1 || journal_size > IB_READER_JOURNAL_CAP) goto invalid;
    if (!legacy) {
        memcpy(out->journal, generation_end + 1, journal_size);
        if (!valid_journal(out->journal)) goto invalid;
    }
    *size = used;
    return 0;
invalid:
    memset(out, 0, sizeof(*out));
    return -1;
}

int ib_reader_index_read(int root, unsigned first, ib_reader_index *out) {
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    if (!private_root(root) || first > 1) return -1;
    int absent = 1;
    char bytes[INDEX_CAP + 1U];
    size_t size;
    for (unsigned i = first; i < 2; ++i) {
        ib_reader_index candidate;
        int result = read_leaf(root, names[i], &candidate, bytes, &size);
        if (result == 0) { *out = candidate; out->previous = i == 1; return 0; }
        if (result != 1) absent = 0;
    }
    return absent ? 1 : -1;
}

static int write_all(int fd, const char *bytes, size_t size) {
    size_t used = 0;
    while (used < size) {
        ssize_t n = write(fd, bytes + used, size - used);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        used += (size_t)n;
    }
    return 0;
}

static int atomic_leaf(int root, const char *name, const char *bytes, size_t size) {
    unsigned char random[24];
    size_t used = 0;
    while (used < sizeof(random)) {
        ssize_t n = getrandom(random + used, sizeof(random) - used, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        used += (size_t)n;
    }
    static const char hex[] = "0123456789abcdef";
    char identity[49], temporary[96];
    for (unsigned i = 0; i < sizeof(random); ++i) {
        identity[2*i] = hex[random[i] >> 4];
        identity[2*i+1] = hex[random[i] & 15];
    }
    identity[48] = 0;
    int n = snprintf(temporary, sizeof(temporary), "%s.new.%s", name, identity);
    if (n < 1 || (size_t)n >= sizeof(temporary)) return -1;
    int fd = openat(root, temporary, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return -1;
    int ok = write_all(fd, bytes, size) == 0 && fsync(fd) == 0;
    if (close(fd) != 0) ok = 0;
    if (ok) ok = renameat(root, temporary, root, name) == 0 && fsync(root) == 0;
    if (!ok) (void)unlinkat(root, temporary, 0); /* Only this operation's leaf. */
    return ok ? 0 : -1;
}

int ib_reader_index_publish(int root, const ib_reader_index *index) {
    if (!index || !private_root(root) || !valid_identity(index->source_id) ||
        !index->source_generation || index->source_generation > INT32_MAX || !valid_journal(index->journal)) return -1;
    char bytes[INDEX_CAP + 1U], old_bytes[INDEX_CAP + 1U], digest[65];
    int prefix = snprintf(bytes, sizeof(bytes), "IBLAST2\n%s\n%u\n%s", index->source_id,
        index->source_generation, index->journal);
    if (prefix < 1 || (size_t)prefix > sizeof(bytes) - 66U) return -1;
    ib_reader_sha256_hex((unsigned char *)bytes, (size_t)prefix, digest);
    memcpy(bytes + prefix, digest, 64); bytes[prefix + 64] = '\n';
    ib_reader_index old;
    size_t old_size;
    int previous = read_leaf(root, names[0], &old, old_bytes, &old_size);
    if (previous == -2) return -1;
    if (previous == 0 && !index->previous &&
        (old_size != (size_t)prefix + 65U || memcmp(old_bytes, bytes, old_size)) &&
        atomic_leaf(root, names[1], old_bytes, old_size) != 0) return -1;
    return atomic_leaf(root, names[0], bytes, (size_t)prefix + 65U);
}
