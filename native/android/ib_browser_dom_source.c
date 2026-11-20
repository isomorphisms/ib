#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include "ib_browser_dom_source.h"
#include "appendfat_arena.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#define DOM_RECEIPT_MAX 32768U
static int dom_text(const char *value, size_t cap) {
    if (!value || !*value || strnlen(value, cap) >= cap) return 0;
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p)
        if (*p < 32U || *p > 126U || *p == '\t') return 0;
    return 1;
}
static int dom_id(const char *value) {
    if (!dom_text(value, IB_READER_REQUEST_ID_CAP + 1U) || !strcmp(value, ".") || !strcmp(value, "..")) return 0;
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p)
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9') || *p == '-' || *p == '_' || *p == '.')) return 0;
    return 1;
}
static int dom_hash(const char *value) {
    if (!value || strnlen(value, 65U) != 64U) return 0;
    for (unsigned i = 0; i < 64U; ++i)
        if (!((value[i] >= '0' && value[i] <= '9') || (value[i] >= 'a' && value[i] <= 'f'))) return 0;
    return 1;
}
static int dom_number(const char *value, uint64_t *out) {
    if (!value || !*value || (value[0] == '0' && value[1])) return 0;
    uint64_t n = 0;
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p) {
        if (*p < '0' || *p > '9' || n > ((uint64_t)INT64_MAX - (*p - '0')) / 10U) return 0;
        n = n * 10U + (*p - '0');
    }
    *out = n;
    return 1;
}
static int dom_directory(int fd) {
    struct stat s;
    return fd >= 0 && fstat(fd, &s) == 0 && S_ISDIR(s.st_mode) &&
        s.st_uid == geteuid() && (s.st_mode & 077U) == 0;
}
static int dom_regular(int root, const char *name) {
    int fd = openat(root, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    struct stat s;
    int ok = fd >= 0 && fstat(fd, &s) == 0 && S_ISREG(s.st_mode) && s.st_uid == geteuid();
    if (fd >= 0) close(fd);
    return ok;
}
static int dom_basic(const ib_browser_dom_metadata *m, int complete) {
    if (!m || !dom_id(m->request_id) || !dom_id(m->parent_id) ||
        !strcmp(m->request_id, m->parent_id) || !m->source_generation ||
        m->source_generation > INT32_MAX || m->parent_generation != m->source_generation ||
        !dom_hash(m->parent_sha256) || !dom_text(m->requested_url, sizeof(m->requested_url)) ||
        !dom_text(m->resolved_url, sizeof(m->resolved_url)) ||
        !dom_text(m->provider_package, sizeof(m->provider_package)) ||
        !dom_text(m->provider_version, sizeof(m->provider_version))) return 0;
    if (!complete) return 1;
    if (!m->byte_count || m->byte_count > IB_DOM_MAX_BYTES || !dom_hash(m->sha256) ||
        !m->target_count || m->target_count > IB_DOM_MAX_TARGETS) return 0;
    for (unsigned i = 0; i < m->target_count; ++i) {
        const ib_dom_target_reference *t = &m->targets[i];
        if (!dom_id(t->request_id) || !strcmp(t->request_id, m->request_id) ||
            !strcmp(t->request_id, m->parent_id) ||
            !dom_text(t->original_href, sizeof(t->original_href)) ||
            !dom_text(t->requested_url, sizeof(t->requested_url))) return 0;
        for (unsigned j = 0; j < i; ++j)
            if (t->occurrence == m->targets[j].occurrence ||
                !strcmp(t->request_id, m->targets[j].request_id)) return 0;
    }
    return 1;
}

static int dom_parent(ib_arena_source_owner *owner, uint64_t navigation,
                      const ib_browser_dom_metadata *m) {
    ib_reader_source source;
    ib_reader_source_init(&source);
    ib_reader_source_want(&source, navigation);
    ib_reader_source_ticket ticket;
    ib_arena_source_observations observations;
    int fd = -1;
    int ok = navigation == atomic_load(&owner->wanted_navigation_generation) &&
        ib_arena_source_owner_open_decode_input(owner, m->parent_id,
            m->parent_generation, navigation, &ticket, &observations, &fd) == 0;
    if (ok) ok = ib_reader_source_accept_fd(&source, &ticket, fd) == IB_READER_PFD_ACCEPTED &&
        !strcmp(ticket.sha256_hex, m->parent_sha256) &&
        !strcmp(observations.requested_url, m->requested_url) &&
        navigation == atomic_load(&owner->wanted_navigation_generation);
    if (fd >= 0) close(fd);
    ib_reader_source_clear(&source);
    return ok;
}

int ib_browser_dom_append(ib_arena_source_owner *owner, uint64_t navigation,
                         ib_browser_dom_metadata *m,
                         const unsigned char *bytes, size_t count) {
    if (!owner || !dom_basic(m, 0) || !bytes || !count || count > IB_DOM_MAX_BYTES ||
        !dom_parent(owner, navigation, m)) return -1;
    int root = -1, locks = -1, result = -1, opened = 0;
    appendfat_arena arena = {0};
    char path[IB_ARENA_PRIVATE_ROOT_CAP + 32U];
    int n = snprintf(path, sizeof(path), "%s/responses.arena", owner->private_root);
    if (n <= 0 || (size_t)n >= sizeof(path)) return -1;
    root = open(owner->private_root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (!dom_directory(root) || !dom_regular(root, "responses.arena") ||
        !dom_regular(root, "responses.arena.used") || !dom_regular(root, "responses.arena.lock")) goto done;
    locks = openat(root, "locks", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (!dom_directory(locks) || mkdirat(locks, m->request_id, 0700) != 0) goto done;
    if (appendfat_arena_open(&arena, path, 1) != 0) goto done;
    opened = 1;
    uint64_t offset = appendfat_arena_committed(&arena);
    if (offset > appendfat_arena_capacity(&arena) ||
        count > appendfat_arena_capacity(&arena) - offset ||
        navigation != atomic_load(&owner->wanted_navigation_generation)) goto done;
    if (appendfat_arena_append(&arena, bytes, count) != 0) {
        appendfat_arena_discard_uncommitted(&arena);
        goto done;
    }
    if (appendfat_arena_commit(&arena) != 0 || appendfat_arena_committed(&arena) - offset != count) goto done;
    appendfat_arena_close(&arena);
    opened = 0;
    m->arena_offset = offset;
    m->byte_count = (uint32_t)count;
    ib_reader_sha256_hex(bytes, count, m->sha256);
    ib_reader_source_ticket ticket;
    ib_reader_source readback;
    ib_reader_source_init(&readback);
    ib_reader_source_want(&readback, navigation);
    int fd = -1;
    int ok = ib_arena_source_owner_open_representation_range(owner, m->request_id,
        navigation, offset, (uint32_t)count, m->sha256, &ticket, &fd) == 0;
    if (ok) ok = ib_reader_source_accept_fd(&readback, &ticket, fd) == IB_READER_PFD_ACCEPTED &&
        readback.byte_count == count && !memcmp(readback.bytes, bytes, count);
    if (fd >= 0) close(fd);
    ib_reader_source_clear(&readback);
    if (ok) result = 0;
done:
    if (opened) appendfat_arena_close(&arena);
    if (locks >= 0) close(locks);
    if (root >= 0) close(root);
    return result;
}

static int dom_write_receipt(FILE *f, const ib_browser_dom_metadata *m) {
    int n = fprintf(f,
        "schema\tib-browser-dom-arena-v1\nrequest_id\t%s\nsource_generation\t%u\n"
        "requested_url\t%s\nresolved_url\t%s\nparent_id\t%s\nparent_generation\t%u\n"
        "parent_sha256\t%s\nprovider_package\t%s\nprovider_version\t%s\n"
        "serialization\twebview-content-outerHTML-utf8-v1\narena_offset\t%" PRIu64 "\n"
        "byte_count\t%u\nsha256\t%s\ntarget_count\t%u\ncomplete\tYES\n",
        m->request_id, m->source_generation, m->requested_url, m->resolved_url,
        m->parent_id, m->parent_generation, m->parent_sha256, m->provider_package,
        m->provider_version, m->arena_offset, m->byte_count, m->sha256, m->target_count);
    if (n < 0) return 0;
    for (unsigned i = 0; i < m->target_count; ++i) {
        const ib_dom_target_reference *t = &m->targets[i];
        if (fprintf(f, "target%u_occurrence\t%u\ntarget%u_href\t%s\n"
            "target%u_requested\t%s\ntarget%u_id\t%s\n",
            i + 1U, t->occurrence, i + 1U, t->original_href,
            i + 1U, t->requested_url, i + 1U, t->request_id) < 0) return 0;
    }
    return 1;
}

static int dom_bound_targets(ib_arena_source_owner *owner, uint64_t navigation,
                             const ib_browser_dom_metadata *m) {
    for (unsigned i = 0; i < m->target_count; ++i) {
        ib_reader_source source;
        ib_reader_source_init(&source);
        ib_reader_source_want(&source, navigation);
        ib_reader_source_ticket ticket;
        ib_arena_source_observations observations;
        int fd = -1;
        int ok = ib_arena_source_owner_open_decode_input(owner, m->targets[i].request_id,
            m->source_generation, navigation, &ticket, &observations, &fd) == 0;
        if (ok) ok = ib_reader_source_accept_fd(&source, &ticket, fd) == IB_READER_PFD_ACCEPTED &&
            !strcmp(observations.requested_url, m->targets[i].requested_url) &&
            !strcmp(observations.http_status, "200") && !strcmp(observations.mime_type, "text/html") &&
            navigation == atomic_load(&owner->wanted_navigation_generation);
        if (fd >= 0) close(fd);
        ib_reader_source_clear(&source);
        if (!ok) return 0;
    }
    return 1;
}

int ib_browser_dom_seal(ib_arena_source_owner *owner, uint64_t navigation,
                       const ib_browser_dom_metadata *m) {
    if (!owner || !dom_basic(m, 1) || !dom_parent(owner, navigation, m) ||
        !dom_bound_targets(owner, navigation, m)) return -1;
    ib_reader_source_ticket verified;
    int verified_fd = -1;
    if (ib_arena_source_owner_open_representation_range(owner, m->request_id, navigation,
        m->arena_offset, m->byte_count, m->sha256, &verified, &verified_fd) != 0) return -1;
    close(verified_fd);
    int root = -1, receipts = -1, fd = -1, result = -1;
    FILE *file = NULL;
    char pending[100], published[100];
    (void)snprintf(pending, sizeof(pending), ".%s.dom.pending", m->request_id);
    (void)snprintf(published, sizeof(published), "%s.dom.tsv", m->request_id);
    root = open(owner->private_root, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (!dom_directory(root)) goto done;
    receipts = openat(root, "receipts", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (!dom_directory(receipts)) goto done;
    fd = openat(receipts, pending, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    file = fd >= 0 ? fdopen(fd, "w") : NULL;
    if (!file) goto done;
    if (!dom_write_receipt(file, m) || fflush(file) != 0 || fsync(fd) != 0) goto done;
    struct stat info;
    if (fstat(fd, &info) != 0 || info.st_size <= 0 || info.st_size > DOM_RECEIPT_MAX) goto done;
    if (fclose(file) != 0) { file = NULL; fd = -1; goto done; }
    file = NULL; fd = -1;
    if (navigation != atomic_load(&owner->wanted_navigation_generation) ||
        syscall(SYS_renameat2, receipts, pending, receipts, published, 1U) != 0 ||
        fsync(receipts) != 0) goto done;
    result = 0;
done:
    if (file) { (void)fclose(file); fd = -1; }
    if (fd >= 0) close(fd);
    if (receipts >= 0) close(receipts);
    if (root >= 0) close(root);
    return result;
}

static int dom_copy(char *out, size_t capacity, const char *value) {
    size_t n = strlen(value);
    if (!n || n >= capacity) return 0;
    memcpy(out, value, n + 1U);
    return 1;
}

static int dom_parse(char *text, size_t length, const char *id,
                     uint32_t generation, ib_browser_dom_metadata *m) {
    static const char *const names[] = {
        "schema", "request_id", "source_generation", "requested_url", "resolved_url",
        "parent_id", "parent_generation", "parent_sha256", "provider_package", "provider_version",
        "serialization", "arena_offset", "byte_count", "sha256", "target_count", "complete",
        "target1_occurrence", "target1_href", "target1_requested", "target1_id",
        "target2_occurrence", "target2_href", "target2_requested", "target2_id",
        "target3_occurrence", "target3_href", "target3_requested", "target3_id"
    };
    char *fields[28] = {0};
    char *position = text, *end = text + length;
    while (position < end) {
        char *newline = memchr(position, '\n', (size_t)(end - position));
        char *tab = newline ? memchr(position, '\t', (size_t)(newline - position)) : NULL;
        if (!newline || !tab || tab == position || tab + 1 == newline) return 0;
        for (char *p = position; p < newline; ++p)
            if (p != tab && ((unsigned char)*p < 32U || (unsigned char)*p > 126U)) return 0;
        *tab = 0; *newline = 0;
        unsigned key = 0;
        while (key < 28U && strcmp(position, names[key])) ++key;
        if (key == 28U || fields[key]) return 0;
        fields[key] = tab + 1;
        position = newline + 1;
    }
    for (unsigned i = 0; i < 16U; ++i) if (!fields[i]) return 0;
    uint64_t source, parent, offset, bytes, targets;
    if (strcmp(fields[0], "ib-browser-dom-arena-v1") || strcmp(fields[1], id) ||
        !dom_number(fields[2], &source) || source != generation ||
        !dom_number(fields[6], &parent) || parent != source ||
        !dom_number(fields[11], &offset) || !dom_number(fields[12], &bytes) ||
        !bytes || bytes > IB_DOM_MAX_BYTES || !dom_number(fields[14], &targets) ||
        !targets || targets > IB_DOM_MAX_TARGETS || strcmp(fields[15], "YES") ||
        strcmp(fields[10], "webview-content-outerHTML-utf8-v1")) return 0;
    memset(m, 0, sizeof(*m));
    m->source_generation = generation; m->parent_generation = generation;
    m->arena_offset = offset; m->byte_count = (uint32_t)bytes; m->target_count = (unsigned)targets;
    if (!dom_copy(m->request_id, sizeof(m->request_id), id) ||
        !dom_copy(m->requested_url, sizeof(m->requested_url), fields[3]) ||
        !dom_copy(m->resolved_url, sizeof(m->resolved_url), fields[4]) ||
        !dom_copy(m->parent_id, sizeof(m->parent_id), fields[5]) ||
        !dom_copy(m->parent_sha256, sizeof(m->parent_sha256), fields[7]) ||
        !dom_copy(m->provider_package, sizeof(m->provider_package), fields[8]) ||
        !dom_copy(m->provider_version, sizeof(m->provider_version), fields[9]) ||
        !dom_copy(m->sha256, sizeof(m->sha256), fields[13])) return 0;
    for (unsigned i = 0; i < IB_DOM_MAX_TARGETS; ++i) {
        unsigned first = 16U + i * 4U;
        for (unsigned k = 0; k < 4U; ++k) if ((fields[first + k] != NULL) != (i < targets)) return 0;
        if (i >= targets) continue;
        uint64_t occurrence;
        if (!dom_number(fields[first], &occurrence) || occurrence > UINT32_MAX ||
            !dom_copy(m->targets[i].original_href, sizeof(m->targets[i].original_href), fields[first + 1U]) ||
            !dom_copy(m->targets[i].requested_url, sizeof(m->targets[i].requested_url), fields[first + 2U]) ||
            !dom_copy(m->targets[i].request_id, sizeof(m->targets[i].request_id), fields[first + 3U])) return 0;
        m->targets[i].occurrence = (uint32_t)occurrence;
    }
    return dom_basic(m, 1);
}

int ib_browser_dom_open(ib_arena_source_owner *owner, const char *id,
                       uint32_t generation, uint64_t navigation,
                       ib_browser_dom_metadata *m,
                       ib_reader_source_ticket *ticket, int *read_fd) {
    if (read_fd) *read_fd = -1;
    if (!owner || !dom_id(id) || !generation || generation > INT32_MAX || !m || !ticket || !read_fd ||
        navigation != atomic_load(&owner->wanted_navigation_generation)) return -1;
    int root = -1, receipts = -1, fd = -1, result = -1;
    char name[100];
    (void)snprintf(name, sizeof(name), "%s.dom.tsv", id);
    root = open(owner->private_root, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (!dom_directory(root)) goto done;
    receipts = openat(root, "receipts", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (!dom_directory(receipts)) goto done;
    fd = openat(receipts, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    struct stat info;
    if (fd < 0 || fstat(fd, &info) != 0 || !S_ISREG(info.st_mode) || info.st_uid != geteuid() ||
        (info.st_mode & 077U) != 0 || info.st_size <= 0 || info.st_size > DOM_RECEIPT_MAX) goto done;
    size_t length = (size_t)info.st_size, used = 0;
    char *text = malloc(length + 1U);
    if (!text) goto done;
    while (used < length) {
        ssize_t n = read(fd, text + used, length - used);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        used += (size_t)n;
    }
    text[used] = 0;
    int valid = used == length && dom_parse(text, length, id, generation, m);
    free(text);
    if (!valid || !dom_parent(owner, navigation, m)) goto done;
    result = ib_arena_source_owner_open_representation_range(owner, id, navigation,
        m->arena_offset, m->byte_count, m->sha256, ticket, read_fd);
done:
    if (fd >= 0) close(fd);
    if (receipts >= 0) close(receipts);
    if (root >= 0) close(root);
    return result;
}
