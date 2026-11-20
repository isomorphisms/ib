#define _GNU_SOURCE
#include "../native/android/ib_shizuku_arena_ingress.h"
#include "../native/android/ib_shizuku_arena_publish.h"
#include "../native/android/ib_arena_source_owner.h"
#include "../native/android/ib_redirect_observations.h"
#include "appendfat_arena.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

static void immutable_receipt_case(ib_ingress_arena *publisher, const char *root,
                                   const unsigned char *packet, size_t count) {
    char path[4096];
    int n = snprintf(path, sizeof(path), "%s/receipts/immutable-collision.tsv", root);
    assert(n > 0 && (size_t)n < sizeof(path));
    static const char original_receipt[] = "previous immutable receipt\n";
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    assert(fd >= 0);
    assert(write(fd, original_receipt, sizeof(original_receipt)-1U) == sizeof(original_receipt)-1U);
    assert(fsync(fd) == 0 && close(fd) == 0);
    puts("STAGE immutable receipt no-replace preserves previous bytes");
    assert(fflush(stdout) == 0);
    int publication = send_frame(publisher, "immutable-collision", packet, count, 0, 0);
    assert(publication != 0);
    fd = open(path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    assert(fd >= 0);
    char retained[sizeof(original_receipt) + 1U];
    ssize_t retained_length = read(fd, retained, sizeof(retained));
    struct stat info;
    assert(fstat(fd, &info) == 0 && info.st_nlink == 1);
    assert(close(fd) == 0);
    assert(retained_length == sizeof(original_receipt)-1U &&
        memcmp(retained, original_receipt, sizeof(original_receipt)-1U) == 0);
    assert(reopen(root, "observed", 19) == 0);
    puts("PASS: immutable receipt collision refuses replacement and preserves previous source");
}

static void current_metadata_cases(ib_ingress_arena *publisher, const char *root) {
    unsigned char packet[16384];
    char base[4096], fields[8193];
    metadata(base, sizeof(base), "text/html; charset=utf-8", url, 0,
             "valid", sizeof(html)-1U);
    const char *valid[] = {"identity", "chunked"};
    const char *identities[] = {"current-identity", "current-chunked"};
    for (size_t i = 0; i < 2U; ++i) {
        int n = snprintf(fields, sizeof(fields),
                          "%scontent_range\tabsent\ntransfer_decoding\t%s\n", base, valid[i]);
        assert(n > 0 && (size_t)n < sizeof(fields));
        size_t count = frame(packet, sizeof(packet), fields, html, sizeof(html)-1U);
        assert(send_frame(publisher, identities[i], packet, count, 0, 0) == 0);
        assert(reopen(root, identities[i], 19) == 0);
        ib_arena_source_owner owner;
        assert(ib_arena_source_owner_init(&owner, root) == 0);
        ib_arena_source_owner_want(&owner, 19);
        ib_reader_source_ticket ticket;
        ib_arena_source_observations observed;
        int fd = -1;
        assert(ib_arena_source_owner_open_observed(&owner, identities[i], 19, 19,
            &ticket, &observed, &fd) == 0 && fd >= 0);
        puts("STAGE retained transfer-decoding observation");
        assert(fflush(stdout) == 0);
        assert(strcmp(observed.transfer_decoding, valid[i]) == 0);
        ib_reader_source source;
        ib_reader_source_init(&source);
        ib_reader_source_want(&source, 19);
        assert(ib_reader_source_accept_fd(&source, &ticket, fd) == IB_READER_PFD_ACCEPTED);
        assert(close(fd) == 0);
        ib_reader_source_clear(&source);
    }
    const char *invalid[] = {
        "content_range\tabsent\n",
        "transfer_decoding\tidentity\n",
        "content_range\tbytes 0-1/2\ntransfer_decoding\tidentity\n",
        "content_range\tabsent\ntransfer_decoding\tgzip\n",
        "content_range\tabsent\ntransfer_decoding\tchunked\ncontent_range\tabsent\n"
    };
    for (size_t i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
        char identity[64];
        int n = snprintf(identity, sizeof(identity), "current-refusal-%zu", i);
        assert(n > 0 && (size_t)n < sizeof(identity));
        n = snprintf(fields, sizeof(fields), "%s%s", base, invalid[i]);
        assert(n > 0 && (size_t)n < sizeof(fields));
        size_t count = frame(packet, sizeof(packet), fields, html, sizeof(html)-1U);
        reject(publisher, root, identity, packet, count);
    }
    puts("PASS: current ICU ten-field identity/chunked captures and paired-field/range/decoder/duplicate refusals");
}

static void http_failure_cases(ib_ingress_arena *publisher, const char *root) {
    char base[4096], fields[8193], identity[64];
    unsigned char packet[16384];
    metadata(base, sizeof(base), "text/html; charset=utf-8", url, 0,
             "valid", sizeof(html)-1U);
    const char *statuses[] = {"300", "400", "404", "429", "500", "599"};
    for (size_t i = 0; i < sizeof(statuses)/sizeof(statuses[0]); ++i) {
        int n = snprintf(identity, sizeof(identity), "http-error-%s", statuses[i]);
        assert(n > 0 && (size_t)n < sizeof(identity));
        n = snprintf(fields, sizeof(fields), "http_status\t%s\n%s",
                     statuses[i], strchr(base, '\n') + 1);
        assert(n > 0 && (size_t)n < sizeof(fields));
        size_t count = frame(packet, sizeof(packet), fields, html, sizeof(html)-1U);
        uint64_t before = committed(root);
        assert(send_frame(publisher, identity, packet, count, 0, 0) == 0);
        assert(committed(root) == before + sizeof(html)-1U);
        assert(reopen(root, identity, 19) != 0);
        ib_arena_source_owner owner;
        assert(ib_arena_source_owner_init(&owner, root) == 0);
        ib_arena_source_owner_want(&owner, 20);
        ib_reader_source_ticket ticket;
        ib_arena_source_observations observed;
        int fd = -1;
        assert(ib_arena_source_owner_open_decode_input(&owner, identity, 19, 20,
            &ticket, &observed, &fd) == 0 && fd >= 0);
        puts("STAGE complete HTTP error remains reader-ineligible");
        assert(fflush(stdout) == 0);
        assert(observed.navigation_admitted == 0);
        assert(strcmp(observed.http_status, statuses[i]) == 0);
        ib_reader_source source;
        ib_reader_source_init(&source);
        ib_reader_source_want(&source, 20);
        assert(ib_reader_source_accept_fd(&source, &ticket, fd) == IB_READER_PFD_ACCEPTED);
        assert(close(fd) == 0);
        assert(source.byte_count == sizeof(html)-1U &&
               memcmp(source.bytes, html, sizeof(html)-1U) == 0);
        ib_reader_source_clear(&source);
        assert(reopen(root, "observed", 19) == 0);
    }
    const char *invalid[] = {"100", "199", "600", "999", "0200"};
    for (size_t i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
        int n = snprintf(identity, sizeof(identity), "invalid-status-%zu", i);
        assert(n > 0 && (size_t)n < sizeof(identity));
        n = snprintf(fields, sizeof(fields), "http_status\t%s\n%s",
                     invalid[i], strchr(base, '\n') + 1);
        assert(n > 0 && (size_t)n < sizeof(fields));
        size_t count = frame(packet, sizeof(packet), fields, html, sizeof(html)-1U);
        reject(publisher, root, identity, packet, count);
    }
    puts("PASS: complete HTTP error bytes and exact status retained without admission; malformed status and prior-source controls");
}

static void redirect_observation_cases(ib_ingress_arena *publisher, const char *root) {
    assert(ib_redirect_chain_valid("R1 20 https://example.com/", "https://example.com", "https://example.com/", 0));
    assert(ib_redirect_chain_valid("R1 22 https://example.com/?q", "https://example.com:0443?q#anchor", "https://example.com/?q", 0));
    assert(!ib_redirect_chain_valid("R1 20 https://example.com/", "http://example.com", "https://example.com/", 0));
    assert(!ib_redirect_chain_valid("R1 20 https://example.com/", "https://example.com:444", "https://example.com/", 0));
    unsigned char packet[16384];
    char base[4096], fields[8193], chain[4096];
    const char *final = "https://example.org/final?x=302;next";
    metadata(base, sizeof(base), "text/html; charset=utf-8", final, 2,
             "valid", sizeof(html)-1U);
    int n = snprintf(chain, sizeof(chain), "R1 %zu %s302 %zu %s307 %zu %s",
                     strlen(url), url, strlen("https://other.example/path"),
                     "https://other.example/path", strlen(final), final);
    assert(n > 0 && (size_t)n < sizeof(chain));
    n = snprintf(fields, sizeof(fields),
                  "%scontent_range\tabsent\ntransfer_decoding\tidentity\nredirect_chain\t%s\n",
                  base, chain);
    assert(n > 0 && (size_t)n < sizeof(fields));
    size_t count = frame(packet, sizeof(packet), fields, html, sizeof(html)-1U);
    assert(send_frame(publisher, "proven-redirect", packet, count, 0, 0) == 0);
    assert(reopen(root, "proven-redirect", 19) == 0);

    char receipt_path[4096], original[16384];
    n = snprintf(receipt_path, sizeof(receipt_path), "%s/receipts/proven-redirect.tsv", root);
    assert(n > 0 && (size_t)n < sizeof(receipt_path));
    FILE *receipt = fopen(receipt_path, "rb");
    assert(receipt != NULL);
    size_t length = fread(original, 1, sizeof(original)-1U, receipt);
    assert(length > 0 && length < sizeof(original)-1U && !ferror(receipt));
    assert(fclose(receipt) == 0);
    original[length] = '\0';
    char *retained = strstr(original, "redirect_chain\tR1 ");
    assert(retained != NULL && strstr(retained, chain) != NULL);
    /* A valid body/digest cannot hide a corrupt persisted observation. */
    char changed[16384];
    memcpy(changed, original, length);
    size_t position = (size_t)(retained - original) + strlen("redirect_chain\tR1 ");
    changed[position] = '0';
    receipt = fopen(receipt_path, "wb");
    assert(receipt != NULL && fwrite(changed, 1, length, receipt) == length);
    assert(fclose(receipt) == 0);
    assert(reopen(root, "proven-redirect", 19) != 0);
    assert(reopen(root, "observed", 19) == 0);
    receipt = fopen(receipt_path, "wb");
    assert(receipt != NULL && fwrite(original, 1, length, receipt) == length);
    assert(fclose(receipt) == 0);
    assert(reopen(root, "proven-redirect", 19) == 0);

    const char *invalid[] = {
        "R1 20 https://example.com/200 20 https://example.org/",
        "R1 20 https://example.net/302 20 https://example.org/",
        "R1 20 https://example.com/302 20 https://example.net/",
        "R1 20 https://example.com/302 19 https://example.org/",
        "R1 020 https://example.com/302 20 https://example.org/",
        "R1 20 https://example.com/302 20 https://example.org/trailing",
        "R1 20 https://example.com/302 29 https://user:secret@test.org/"
    };
    metadata(base, sizeof(base), "text/html; charset=utf-8", "https://example.org/", 1,
             "valid", sizeof(html)-1U);
    for (size_t i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
        char identity[64];
        n = snprintf(identity, sizeof(identity), "redirect-refusal-%zu", i);
        assert(n > 0 && (size_t)n < sizeof(identity));
        n = snprintf(fields, sizeof(fields),
                      "%scontent_range\tabsent\ntransfer_decoding\tidentity\nredirect_chain\t%s\n",
                      base, invalid[i]);
        assert(n > 0 && (size_t)n < sizeof(fields));
        count = frame(packet, sizeof(packet), fields, html, sizeof(html)-1U);
        reject(publisher, root, identity, packet, count);
    }
    /* The chain says two followed responses; the independent count says one. */
    n = snprintf(fields, sizeof(fields),
                  "%scontent_range\tabsent\ntransfer_decoding\tidentity\nredirect_chain\t%s\n",
                  base, chain);
    assert(n > 0 && (size_t)n < sizeof(fields));
    count = frame(packet, sizeof(packet), fields, html, sizeof(html)-1U);
    reject(publisher, root, "redirect-wrong-count", packet, count);
    puts("PASS: ordered response-owner redirect chain, canonical retention/readback and origin/final/count/status/length/corruption refusals");
}

static unsigned char *read_observation(const char *filename, size_t bound, size_t *length) {
    FILE *stream = fopen(filename, "rb");
    assert(stream != NULL);
    unsigned char *bytes = malloc(bound + 1U);
    assert(bytes != NULL);
    *length = fread(bytes, 1, bound + 1U, stream);
    assert(!ferror(stream) && *length > 0 && *length <= bound);
    assert(fclose(stream) == 0);
    bytes[*length] = 0;
    return bytes;
}

static void decoded_input_case(ib_ingress_arena *publisher, const char *root) {
    const unsigned char original[] = "<p>caf\351 \200 \223source\224</p>";
    char fields[8193];
    unsigned char packet[16384];
    metadata(fields, sizeof(fields), "text/html; profile=reader; charset = \"Windows-1252\"",
        url, 0, "invalid", sizeof(original)-1U);
    size_t count = frame(packet, sizeof(packet), fields, original, sizeof(original)-1U);
    uint64_t original_offset = committed(root);
    assert(send_frame(publisher, "decode-input", packet, count, 0, 0) == 0);
    /* Original UTF-8 admission remains refused. The decoder input contract
     * must preserve octets and observations without pretending admission. */
    assert(reopen(root, "decode-input", 19) != 0);
    ib_arena_source_owner owner;
    ib_reader_source_ticket ticket;
    ib_arena_source_observations observed;
    ib_reader_source reader;
    int fd = -1;
    assert(ib_arena_source_owner_init(&owner, root) == 0);
    ib_arena_source_owner_want(&owner, 20);
    assert(ib_arena_source_owner_open_decode_input(&owner, "decode-input", 19, 20,
        &ticket, &observed, &fd) == 0);
    assert(!observed.utf8_valid && !observed.navigation_admitted);
    assert(!strcmp(observed.charset, "windows-1252"));
    assert(!strcmp(observed.source_key, "arena:decode-input") && observed.source_generation == 19);
    assert(ticket.navigation_generation == 20 && ticket.byte_count == sizeof(original)-1U);
    char digest[65];
    ib_reader_sha256_hex(original, sizeof(original)-1U, digest);
    assert(!strcmp(ticket.sha256_hex, digest));
    ib_reader_source_init(&reader);
    ib_reader_source_want(&reader, 20);
    assert(ib_reader_source_accept_fd(&reader, &ticket, fd) == IB_READER_PFD_ACCEPTED);
    assert(close(fd) == 0);
    assert(reader.byte_count == sizeof(original)-1U && !memcmp(reader.bytes, original, sizeof(original)-1U));
    fd = -1;
    char path[4096];
    assert(snprintf(path, sizeof(path), "%s/responses.arena", root) < (int)sizeof(path));
    int storage = open(path, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    assert(storage >= 0);
    const unsigned char changed = 0;
    assert(pwrite(storage, &changed, 1, (off_t)original_offset) == 1 && fsync(storage) == 0);
    puts("STAGE original decoder input digest refusal");
    assert(ib_arena_source_owner_open_decode_input(&owner, "decode-input", 19, 20,
        &ticket, &observed, &fd) != 0 && fd == -1);
    assert(observed.source_key[0] == 0);
    assert(pwrite(storage, original, 1, (off_t)original_offset) == 1 && fsync(storage) == 0);
    assert(close(storage) == 0);
    assert(ib_arena_source_owner_open_decode_input(&owner, "decode-input", 20, 20,
        &ticket, &observed, &fd) != 0 && fd == -1);
    assert(observed.source_key[0] == 0);
    ib_arena_source_owner_want(&owner, 21);
    assert(ib_arena_source_owner_open_decode_input(&owner, "decode-input", 19, 20,
        &ticket, &observed, &fd) != 0 && fd == -1);
    ib_reader_source_clear(&reader);
    /* No duplicate charset may silently win, even when each label is known. */
    metadata(fields, sizeof(fields), "text/html; charset=windows-1252; charset=utf-8",
        url, 0, "invalid", sizeof(original)-1U);
    count = frame(packet, sizeof(packet), fields, original, sizeof(original)-1U);
    reject(publisher, root, "ambiguous-charset", packet, count);
    puts("PASS: original non-UTF-8 canonical bytes and unchanged observations reach only the decoder input; corruption, duplicate charset and stale generations refuse");
}

static void retain_live_observation(ib_ingress_arena *publisher, const char *root,
                                    const char *requested, const char *metadata_file,
                                    const char *body_file) {
    size_t metadata_length, body_length;
    unsigned char *fields = read_observation(metadata_file, 8192U, &metadata_length);
    unsigned char *body = read_observation(body_file, IB_READER_SOURCE_MAX_BYTES, &body_length);
    assert(strlen((const char *)fields) == metadata_length);
    assert(strstr((const char *)fields, "charset=ISO-8859-1\n") != NULL);
    unsigned char *packet = malloc(27U + metadata_length + body_length);
    assert(packet != NULL);
    size_t count = frame(packet, 27U + metadata_length + body_length,
                         (const char *)fields, body, body_length);
    uint64_t before = committed(root);
    url = requested;
    assert(send_frame(publisher, "live-google", packet, count, 0, 0) == 0);
    assert(committed(root) == before + body_length);
    assert(reopen(root, "live-google", 19) != 0);
    char arena_path[4096];
    int n = snprintf(arena_path, sizeof(arena_path), "%s/responses.arena", root);
    assert(n > 0 && (size_t)n < sizeof(arena_path));
    appendfat_arena arena;
    assert(appendfat_arena_open(&arena, arena_path, 0) == 0);
    unsigned char *reopened = malloc(body_length);
    assert(reopened != NULL);
    size_t got = 0;
    assert(appendfat_arena_read(&arena, before, reopened, body_length, &got) == 0);
    assert(got == body_length && memcmp(reopened, body, body_length) == 0);
    appendfat_arena_close(&arena);
    assert(reopen(root, "observed", 19) == 0);
    free(reopened); free(packet); free(body); free(fields);
    puts("PASS: actual ICU Google body retained in canonical Arena and byte-verified; unsupported charset/redirect remains reader-ineligible");
}

int main(int argc, char **argv) {
    assert(argc == 2 || argc == 5);
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
    immutable_receipt_case(&publisher, root, packet, count);
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
    current_metadata_cases(&publisher, root);
    redirect_observation_cases(&publisher, root);
    decoded_input_case(&publisher, root);
    http_failure_cases(&publisher, root);
    if (argc == 5) retain_live_observation(&publisher, root, argv[2], argv[3], argv[4]);
    puts("PASS: paired ICU frame through private ingress into real canonical Arena v2 and exact reopened reader bytes");
    puts("PASS: body-only, partial, trailing, header, size, metadata, UTF-8, credentials, digest, worker and stale refusals preserve prior content");
    puts("PASS: incomplete redirect provenance and unobserved charset cannot admit navigation");
    puts("NOT_RUN: real Shizuku/Binder, checked Idric view, Android ART and physical TAB_P10");
    if (argc == 2) puts("NOT_RUN: ICU HTTPS; this invocation used synthetic captures only");
    return 0;
}
