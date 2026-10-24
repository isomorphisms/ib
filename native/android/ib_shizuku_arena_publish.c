#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include "ib_shizuku_arena_publish.h"
#include "ib_shizuku_arena_ingress.h"
#include "ib_reader_pfd.h"
#include "appendfat_arena.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define RECEIPT_MAX_BYTES 8192U
#define READBACK_CHUNK 65536U

typedef struct {
    unsigned status;
    char resolved_url[IB_INGRESS_URL_CAP + 1U];
    char mime_type[256];
    char charset[64];
    char content_encoding[256];
    int utf8_valid;
    unsigned redirects;
    int64_t declared_length;
    uint64_t received_length;
    int admitted;
} observed_response;

static int decimal(const char *text, uint64_t *value) {
    if (!text || !*text) return -1;
    uint64_t result = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (*p < '0' || *p > '9' ||
            result > ((uint64_t)INT64_MAX - (*p - '0')) / 10U)
            return -1;
        result = result * 10U + (*p - '0');
    }
    *value = result;
    return 0;
}

static int copied(char *out, size_t capacity, const char *input) {
    size_t n = strlen(input);
    if (n == 0 || n >= capacity) return -1;
    memcpy(out, input, n + 1U);
    return 0;
}

/* Check code points independently of the worker's UTF-8 observation. */
static int valid_utf8(const unsigned char *bytes, size_t length) {
    size_t i = 0;
    while (i < length) {
        unsigned first = bytes[i++];
        if (first < 0x80U) continue;
        unsigned remaining, point, minimum;
        if (first >= 0xc2U && first <= 0xdfU) {
            remaining = 1; point = first & 0x1fU; minimum = 0x80U;
        } else if (first >= 0xe0U && first <= 0xefU) {
            remaining = 2; point = first & 0x0fU; minimum = 0x800U;
        } else if (first >= 0xf0U && first <= 0xf4U) {
            remaining = 3; point = first & 0x07U; minimum = 0x10000U;
        } else return 0;
        if (remaining > length - i) return 0;
        while (remaining--) {
            unsigned next = bytes[i++];
            if ((next & 0xc0U) != 0x80U) return 0;
            point = (point << 6U) | (next & 0x3fU);
        }
        if (point < minimum || point > 0x10ffffU ||
            (point >= 0xd800U && point <= 0xdfffU)) return 0;
    }
    return 1;
}

static int media_type(observed_response *response, const char *value) {
    char normalized[512];
    size_t used = 0;
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p) {
        if (used + 1U >= sizeof(normalized)) return -1;
        normalized[used++] = (char)(*p >= 'A' && *p <= 'Z' ? *p + ('a' - 'A') : *p);
    }
    normalized[used] = '\0';
    char *media = normalized;
    while (*media == ' ') ++media;
    char *parameter = strchr(media, ';');
    if (parameter) *parameter++ = '\0';
    size_t media_length = strlen(media);
    while (media_length && media[media_length-1U] == ' ') media[--media_length] = '\0';
    if (!strchr(media, '/') && strcmp(media, "unknown") != 0)
        return -1;
    for (const char *p = media; *p; ++p)
        if (!( (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') ||
                strchr(".+/_-", *p))) return -1;
    if (copied(response->mime_type, sizeof(response->mime_type), media) != 0)
        return -1;
    memcpy(response->charset, "UNOBSERVED", sizeof("UNOBSERVED"));
    if (parameter) {
        while (*parameter == ' ') ++parameter;
        size_t parameter_length = strlen(parameter);
        while (parameter_length && parameter[parameter_length-1U] == ' ')
            parameter[--parameter_length] = '\0';
    }
    if (parameter && (strcmp(parameter, "charset=utf-8") == 0 ||
                       strcmp(parameter, "charset=\"utf-8\"") == 0))
        memcpy(response->charset, "utf-8", sizeof("utf-8"));
    return 0;
}

static int frame_response(const unsigned char *frame, size_t length,
                           const char *requested_url, observed_response *response,
                           const unsigned char **body, size_t *body_bytes) {
    static const char *const names[] = {
        "http_status", "content_type", "content_encoding", "declared_length",
        "received_length", "utf8", "resolved_url", "redirect_count"
    };
    if (length < IB_INGRESS_FRAME_HEADER_BYTES ||
        memcmp(frame, "IBICU01 ", 8U) != 0 || frame[16] != ' ' ||
        frame[25] != '\n') return -1;
    size_t metadata_length = 0, source_length = 0;
    for (size_t i = 0; i < 8U; ++i) {
        unsigned char m = frame[8U + i], b = frame[17U + i];
        if (m < '0' || m > '9' || b < '0' || b > '9') return -1;
        metadata_length = metadata_length * 10U + (m - '0');
        source_length = source_length * 10U + (b - '0');
    }
    if (!metadata_length || metadata_length > IB_INGRESS_METADATA_MAX_BYTES ||
        !source_length || source_length > IB_READER_SOURCE_MAX_BYTES ||
        length != IB_INGRESS_FRAME_HEADER_BYTES + metadata_length + source_length)
        return -1;
    char metadata[IB_INGRESS_METADATA_MAX_BYTES + 1U];
    memcpy(metadata, frame + IB_INGRESS_FRAME_HEADER_BYTES, metadata_length);
    metadata[metadata_length] = '\0';
    char *fields[8] = {0};
    char *position = metadata, *end = metadata + metadata_length;
    while (position < end) {
        char *newline = memchr(position, '\n', (size_t)(end - position));
        if (!newline) return -1;
        char *tab = memchr(position, '\t', (size_t)(newline - position));
        if (!tab || tab == position || tab + 1 == newline) return -1;
        for (char *p = position; p < newline; ++p)
            if (p != tab && ((unsigned char)*p < 32U || (unsigned char)*p > 126U))
                return -1;
        *tab = '\0'; *newline = '\0';
        size_t key = 0;
        while (key < 8U && strcmp(position, names[key]) != 0) ++key;
        if (key == 8U || fields[key]) return -1;
        fields[key] = tab + 1;
        position = newline + 1;
    }
    for (size_t i = 0; i < 8U; ++i) if (!fields[i]) return -1;
    uint64_t status, received, redirects, declared = 0;
    if (decimal(fields[0], &status) != 0 || status < 200U || status > 299U ||
        decimal(fields[4], &received) != 0 || received != source_length ||
        decimal(fields[7], &redirects) != 0 || redirects > 8U ||
        (strcmp(fields[3], "-1") != 0 &&
         (decimal(fields[3], &declared) != 0 || declared != source_length)))
        return -1;
    if (copied(response->resolved_url, sizeof(response->resolved_url), fields[6]) != 0 ||
        copied(response->content_encoding, sizeof(response->content_encoding), fields[2]) != 0 ||
        media_type(response, fields[1]) != 0)
        return -1;
    if (strcmp(fields[5], "valid") != 0 && strcmp(fields[5], "invalid") != 0)
        return -1;
    *body = frame + IB_INGRESS_FRAME_HEADER_BYTES + metadata_length;
    *body_bytes = source_length;
    response->utf8_valid = valid_utf8(*body, source_length);
    if (response->utf8_valid != (strcmp(fields[5], "valid") == 0)) return -1;
    response->status = (unsigned)status;
    response->received_length = received;
    response->declared_length = strcmp(fields[3], "-1") == 0 ? -1 : (int64_t)declared;
    response->redirects = (unsigned)redirects;
    /* The current ICU capture reports only a redirect count, not the chain.
     * Retain those bytes, but do not admit redirected content on that evidence. */
    response->admitted = status == 200U && response->utf8_valid &&
        strcmp(response->mime_type, "text/html") == 0 &&
        strcmp(response->charset, "utf-8") == 0 &&
        strcmp(response->content_encoding, "identity") == 0 &&
        redirects == 0 && strcmp(requested_url, response->resolved_url) == 0;
    return 0;
}

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
                           size_t length, const char digest[65],
                           uint64_t generation, const observed_response *response) {
    char receipt_name[IB_READER_REQUEST_ID_CAP + 5U];
    char temporary_name[IB_READER_REQUEST_ID_CAP + 48U];
    char content[RECEIPT_MAX_BYTES];
    int n = snprintf(receipt_name, sizeof(receipt_name), "%s.tsv", id);
    int t = snprintf(temporary_name, sizeof(temporary_name),
                     ".%s.%ld.pending", id, (long)getpid());
    if (n <= 0 || (size_t)n >= sizeof(receipt_name) ||
        t <= 0 || (size_t)t >= sizeof(temporary_name)) return -1;
    if (response) {
        n = snprintf(content, sizeof(content),
                     "schema\tib-icu-arena-v2\nrequest_id\t%s\n"
                     "source_key\tarena:%s\nsource_generation\t%" PRIu64 "\n"
                     "requested_url\t%s\nresolved_url\t%s\nhttp_status\t%u\n"
                     "mime_type\t%s\ncharset\t%s\ncontent_encoding\t%s\n"
                     "utf8\t%s\nredirect_count\t%u\ndeclared_length\t%" PRId64 "\n"
                     "received_length\t%" PRIu64 "\n"
                     "transport\ticu-app-process-via-binder-shizuku\n"
                     "arena_name\tresponses.arena\narena_offset\t%" PRIu64 "\n"
                     "arena_end\t%" PRIu64 "\nbyte_count\t%zu\nsha256\t%s\n"
                     "reopen_check\tMATCH\nacquisition_complete\tYES\n"
                     "navigation_admitted\t%s\n",
                     id, id, generation, url, response->resolved_url, response->status,
                     response->mime_type, response->charset, response->content_encoding,
                     response->utf8_valid ? "valid" : "invalid", response->redirects,
                     response->declared_length, response->received_length,
                     offset, offset + (uint64_t)length, length, digest,
                     response->admitted ? "YES" : "NO");
    } else n = snprintf(content, sizeof(content),
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

static int publish_response(void *opaque, const char *request_id,
                             uint64_t generation,
                             const char *requested_url,
                             const unsigned char *exact_bytes,
                             size_t byte_count,
                             const char sha256_hex[65],
                             const observed_response *response) {
    ib_ingress_arena *publisher = opaque;
    if (!publisher || !generation || !valid_id(request_id) || !valid_url(requested_url) ||
        (response && !valid_url(response->resolved_url)) ||
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
                        old_used, byte_count, computed, generation, response) != 0)
        goto done;
    result = 0;
done:
    if (arena_is_open) appendfat_arena_close(&canonical);
    if (receipts >= 0) close(receipts);
    if (locks >= 0) close(locks);
    if (root >= 0) close(root);
    return result;
}

int ib_ingress_arena_publish(void *opaque, const char *request_id,
                             uint64_t generation, const char *requested_url,
                             const unsigned char *exact_bytes, size_t byte_count,
                             const char sha256_hex[65]) {
    return publish_response(opaque, request_id, generation, requested_url,
                             exact_bytes, byte_count, sha256_hex, NULL);
}

int ib_ingress_arena_publish_frame(void *opaque, const char *request_id,
                                   uint64_t generation, const char *requested_url,
                                   const unsigned char *frame, size_t length,
                                   const char expected[65]) {
    if (!frame || !expected || !valid_url(requested_url) ||
        !length || length > IB_INGRESS_STREAM_MAX_BYTES ||
        strnlen(expected, 65U) != 64U) return -1;
    char computed[65];
    ib_reader_sha256_hex(frame, length, computed);
    if (memcmp(expected, computed, 65U) != 0) return -1;
    observed_response response = {0};
    const unsigned char *body = NULL;
    size_t body_bytes = 0;
    if (frame_response(frame, length, requested_url, &response,
                        &body, &body_bytes) != 0) return -1;
    ib_reader_sha256_hex(body, body_bytes, computed);
    return publish_response(opaque, request_id, generation, requested_url,
                             body, body_bytes, computed, &response);
}
