#define _GNU_SOURCE
#include "../native/android/ib_shizuku_arena_ingress.h"
#include "../native/android/ib_shizuku_arena_publish.h"
#include "../native/android/ib_arena_source_owner.h"
#include "appendfat_arena.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *url = "https://example.com/";
static const unsigned char html[] = "<h1>Reading</h1><p>caf\303\251 \360\237\214\215</p>\n";

static uint64_t committed(const char *root) {
    char path[4096];
    int n = snprintf(path, sizeof(path), "%s/responses.arena", root);
    assert(n > 0 && (size_t)n < sizeof(path));
    appendfat_arena arena;
    assert(appendfat_arena_open(&arena, path, 0) == 0);
    uint64_t count = appendfat_arena_committed(&arena);
    appendfat_arena_close(&arena);
    return count;
}

static size_t frame(unsigned char *out, size_t capacity, const char *metadata,
                     const unsigned char *body, size_t count) {
    size_t m = strlen(metadata);
    assert(m <= 8192U && m + count + 27U <= capacity);
    int n = snprintf((char *)out, capacity, "IBICU01 %08zu %08zu\n", m, count);
    assert(n == 26);
    memcpy(out + 26U, metadata, m);
    memcpy(out + 26U + m, body, count);
    return 26U + m + count;
}

static void metadata(char *out, size_t capacity, const char *type,
                      const char *resolved, unsigned redirects,
                      const char *utf8, size_t count) {
    int n = snprintf(out, capacity,
        "http_status\t200\ncontent_type\t%s\ncontent_encoding\tidentity\n"
        "declared_length\t%zu\nreceived_length\t%zu\nutf8\t%s\n"
        "resolved_url\t%s\nredirect_count\t%u\n",
        type, count, count, utf8, resolved, redirects);
    assert(n > 0 && (size_t)n < capacity);
}

static int send_frame(ib_ingress_arena *publisher, const char *id,
                       const unsigned char *bytes, size_t count,
                       int worker_status, int stale) {
    ib_shizuku_arena_ingress session;
    assert(ib_ingress_init(&session, ib_ingress_arena_publish_frame, publisher) == 0);
    assert(ib_ingress_arm(&session, id, 19, url, (uid_t)2000) == 0);
    int fd = -1;
    assert(ib_ingress_begin(&session, (uid_t)2000, id, 19, &fd) == 0);
    size_t sent = 0;
    while (sent < count) {
        size_t chunk = count - sent;
        if (chunk > 13U) chunk = 13U; /* Split the header and metadata too. */
        ssize_t n = write(fd, bytes + sent, chunk);
        if (n < 0 && errno == EINTR) continue;
        assert(n > 0);
        sent += (size_t)n;
    }
    assert(close(fd) == 0);
    if (stale) ib_ingress_want(&session, 20);
    int result = ib_ingress_finish(&session, (uid_t)2000, id, 19, worker_status);
    ib_ingress_dispose(&session);
    return result;
}

static int reopen(const char *root, const char *id, uint64_t generation) {
    ib_arena_source_owner owner;
    ib_reader_source_ticket ticket = {0};
    ib_reader_source reader;
    int fd = -1;
    assert(ib_arena_source_owner_init(&owner, root) == 0);
    ib_arena_source_owner_want(&owner, generation);
    int result = ib_arena_source_owner_open(&owner, id, generation, &ticket, &fd);
    if (result != 0) { assert(fd == -1); return result; }
    assert(ticket.navigation_generation == generation);
    assert(ticket.byte_count == sizeof(html) - 1U);
    ib_reader_source_init(&reader);
    ib_reader_source_want(&reader, generation);
    assert(ib_reader_source_accept_fd(&reader, &ticket, fd) == IB_READER_PFD_ACCEPTED);
    assert(close(fd) == 0);
    assert(reader.byte_count == sizeof(html) - 1U);
    assert(memcmp(reader.bytes, html, sizeof(html) - 1U) == 0);
    ib_reader_source_clear(&reader);
    return 0;
}

static void reject(ib_ingress_arena *publisher, const char *root,
                    const char *id, const unsigned char *bytes, size_t count) {
    uint64_t before = committed(root);
    assert(send_frame(publisher, id, bytes, count, 0, 0) != 0);
    assert(committed(root) == before);
    assert(reopen(root, id, 19) != 0);
    assert(reopen(root, "observed", 19) == 0);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    const char *root = argv[1];
    ib_ingress_arena publisher;
    assert(ib_ingress_arena_init(&publisher, root) == 0);
    assert(committed(root) == 0);
    unsigned char packet[16384], bad[16384];
    char fields[8193], altered[8193];
    metadata(fields, sizeof(fields), "text/html; charset=utf-8", url, 0, "valid", sizeof(html)-1U);
    size_t count = frame(packet, sizeof(packet), fields, html, sizeof(html)-1U);
    assert(send_frame(&publisher, "observed", packet, count, 0, 0) == 0);
    assert(committed(root) == sizeof(html)-1U); /* Frame/metadata are not page bytes. */
    assert(reopen(root, "observed", 19) == 0);
    assert(reopen(root, "observed", 20) != 0);
    reject(&publisher, root, "body-only", html, sizeof(html)-1U);
    reject(&publisher, root, "incomplete", packet, count-1U);
    memcpy(bad, packet, count); bad[count] = 'x';
    reject(&publisher, root, "trailing", bad, count+1U);
    memcpy(bad, packet, count); bad[25] = 'x';
    reject(&publisher, root, "header", bad, count);
    memcpy(bad, packet, count); memcpy(bad+8, "99999999", 8);
    reject(&publisher, root, "oversized", bad, count);
    int n = snprintf(altered, sizeof(altered), "%shttp_status\t200\n", fields);
    assert(n > 0 && (size_t)n < sizeof(altered));
    size_t bad_count = frame(bad, sizeof(bad), altered, html, sizeof(html)-1U);
    reject(&publisher, root, "duplicate", bad, bad_count);
    n = snprintf(altered, sizeof(altered), "%sunknown\tvalue\n", fields);
    assert(n > 0 && (size_t)n < sizeof(altered));
    bad_count = frame(bad, sizeof(bad), altered, html, sizeof(html)-1U);
    reject(&publisher, root, "unknown", bad, bad_count);
    memcpy(bad, packet, count); bad[26U] = 0;
    reject(&publisher, root, "metadata-nul", bad, count);
    metadata(altered, sizeof(altered), "text/html; charset=utf-8", url, 0, "valid", sizeof(html));
    bad_count = frame(bad, sizeof(bad), altered, html, sizeof(html)-1U);
    reject(&publisher, root, "wrong-length", bad, bad_count);
    metadata(altered, sizeof(altered), "text/html; charset=utf-8", url, 0, "invalid", sizeof(html)-1U);
    bad_count = frame(bad, sizeof(bad), altered, html, sizeof(html)-1U);
    reject(&publisher, root, "false-utf8", bad, bad_count);
    metadata(altered, sizeof(altered), "text/html; charset=utf-8", "https://user:secret@example.com/", 0, "valid", sizeof(html)-1U);
    bad_count = frame(bad, sizeof(bad), altered, html, sizeof(html)-1U);
    reject(&publisher, root, "credentials", bad, bad_count);
    uint64_t before = committed(root);
    assert(send_frame(&publisher, "worker-failed", packet, count, 29, 0) != 0);
    assert(send_frame(&publisher, "stale", packet, count, 0, 1) != 0);
    assert(committed(root) == before);
    assert(send_frame(&publisher, "observed", packet, count, 0, 0) != 0);
    assert(committed(root) == before);
    char digest[65];
    ib_reader_sha256_hex(packet, count, digest);
    digest[0] = digest[0] == '0' ? '1' : '0';
    assert(ib_ingress_arena_publish_frame(&publisher, "bad-digest", 19, url,
                                          packet, count, digest) != 0);
    assert(committed(root) == before);
    metadata(altered, sizeof(altered), "text/html; charset=utf-8", "https://example.org/", 1, "valid", sizeof(html)-1U);
    bad_count = frame(bad, sizeof(bad), altered, html, sizeof(html)-1U);
    assert(send_frame(&publisher, "unproven-redirect", bad, bad_count, 0, 0) == 0);
    assert(reopen(root, "unproven-redirect", 19) != 0);
    metadata(altered, sizeof(altered), "text/html", url, 0, "valid", sizeof(html)-1U);
    bad_count = frame(bad, sizeof(bad), altered, html, sizeof(html)-1U);
    assert(send_frame(&publisher, "unknown-charset", bad, bad_count, 0, 0) == 0);
    assert(reopen(root, "unknown-charset", 19) != 0);
    assert(reopen(root, "observed", 19) == 0);
    puts("PASS: paired ICU frame through private ingress into real canonical Arena v2 and exact reopened reader bytes");
    puts("PASS: body-only, partial, trailing, header, size, metadata, UTF-8, credentials, digest, worker and stale refusals preserve prior content");
    puts("PASS: incomplete redirect provenance and unobserved charset cannot admit navigation");
    puts("NOT_RUN: real Shizuku/Binder/ICU HTTPS, checked Idric view, Android ART and physical C67");
    return 0;
}
