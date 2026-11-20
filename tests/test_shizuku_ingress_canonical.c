#define _GNU_SOURCE
#include "../native/android/ib_shizuku_arena_ingress.h"
#include "../native/android/ib_shizuku_arena_publish.h"
#include "../native/android/ib_arena_source_owner.h"
#include "appendfat_arena.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const unsigned char first[] = {'I', 'B', 0, 'C', '6', '7', 255, '\n'};
static const char *url = "https://example.com/";

static void path(char *out, size_t capacity, const char *root,
                 const char *name) {
    int n = snprintf(out, capacity, "%s/%s", root, name);
    assert(n > 0 && (size_t)n < capacity);
}

static int commit_attempt(ib_ingress_arena *publisher,
                          const char *id, uint64_t generation,
                          const unsigned char *bytes, size_t byte_count,
                          int worker_exit, int invalidate) {
    ib_shizuku_arena_ingress session;
    assert(ib_ingress_init(&session, ib_ingress_arena_publish, publisher) == 0);
    assert(ib_ingress_arm(&session, id, generation, url, (uid_t)2000) == 0);
    int fd = -1;
    assert(ib_ingress_begin(&session, (uid_t)2000, id, generation, &fd) == 0);
    assert(fd >= 0);
    if (byte_count) {
        size_t sent = 0;
        while (sent < byte_count) {
            ssize_t count = write(fd, bytes + sent, byte_count - sent);
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) break;
            sent += (size_t)count;
        }
    }
    assert(close(fd) == 0);
    if (invalidate)
        ib_ingress_want(&session, generation + 1U);
    int status = ib_ingress_finish(&session, (uid_t)2000,
                                   id, generation, worker_exit);
    ib_ingress_dispose(&session);
    return status;
}

static uint64_t committed_count(const char *root) {
    char arena_path[4096];
    path(arena_path, sizeof(arena_path), root, "responses.arena");
    appendfat_arena arena;
    assert(appendfat_arena_open(&arena, arena_path, 0) == 0);
    uint64_t result = appendfat_arena_committed(&arena);
    appendfat_arena_close(&arena);
    return result;
}

static void retained_matches(const char *root, const char *request_id,
                           uint64_t generation, uint64_t offset,
                           const unsigned char *expected, size_t count) {
    /* Body-only ingress has no observed HTTP metadata. Check retained bytes
     * through the canonical store, and require the v2 HTML owner to refuse it.
     * A valid body digest must never upgrade this v1 receipt to navigation. */
    char filename[4096];
    path(filename, sizeof(filename), root, "responses.arena");
    appendfat_arena arena;
    assert(appendfat_arena_open(&arena, filename, 0) == 0);
    unsigned char retained[sizeof(first)];
    size_t got = 0;
    assert(count == sizeof(retained));
    assert(appendfat_arena_read(&arena, offset, retained, count, &got) == 0);
    assert(got == count && memcmp(retained, expected, count) == 0);
    appendfat_arena_close(&arena);
    ib_arena_source_owner source;
    ib_reader_source_ticket ticket = {0};
    int fd = -1;
    assert(ib_arena_source_owner_init(&source, root) == 0);
    ib_arena_source_owner_want(&source, generation);
    assert(ib_arena_source_owner_open(&source, request_id, generation,
                                      &ticket, &fd) != 0);
    assert(fd == -1);
}

static void source_absent(const char *root, const char *id,
                          uint64_t generation) {
    ib_arena_source_owner source;
    ib_reader_source_ticket ticket = {0};
    int fd = -1;
    assert(ib_arena_source_owner_init(&source, root) == 0);
    ib_arena_source_owner_want(&source, generation);
    assert(ib_arena_source_owner_open(&source, id, generation, &ticket, &fd) != 0);
    assert(fd == -1);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    const char *root = argv[1];
    ib_ingress_arena publisher;
    assert(ib_ingress_arena_init(&publisher, root) == 0);
    assert(committed_count(root) == 0);

    assert(commit_attempt(&publisher, "real-first", 1,
                          first, sizeof(first), 0, 0) == 0);
    assert(committed_count(root) == sizeof(first));
    retained_matches(root, "real-first", 1, 0, first, sizeof(first));

    uint64_t good = committed_count(root);
    assert(commit_attempt(&publisher, "failed-icu", 2,
                          first, sizeof(first), 29, 0) != 0);
    assert(committed_count(root) == good);
    source_absent(root, "failed-icu", 2);

    assert(commit_attempt(&publisher, "stale-page", 3,
                          first, sizeof(first), 0, 1) != 0);
    assert(committed_count(root) == good);
    source_absent(root, "stale-page", 3);

    assert(commit_attempt(&publisher, "empty-body", 4,
                          first, 0, 0, 0) != 0);
    assert(committed_count(root) == good);
    source_absent(root, "empty-body", 4);

    assert(commit_attempt(&publisher, "real-first", 5,
                          first, sizeof(first), 0, 0) != 0);
    assert(committed_count(root) == good);
    retained_matches(root, "real-first", 1, 0, first, sizeof(first));

    char fake_hash[65];
    memset(fake_hash, '0', 64);
    fake_hash[64] = '\0';
    assert(ib_ingress_arena_publish(&publisher, "invalid-hash", 6,
                                    url, first, sizeof(first),
                                    fake_hash) != 0);
    assert(committed_count(root) == good);

    /* No shared writable path: this app-owned directory is private. */
    assert(chmod(root, 0755) == 0);
    assert(commit_attempt(&publisher, "world-readable", 6,
                          first, sizeof(first), 0, 0) != 0);
    assert(chmod(root, 0700) == 0);
    assert(committed_count(root) == good);
    source_absent(root, "world-readable", 6);

    assert(commit_attempt(&publisher, "real-second", 7,
                          first, sizeof(first), 0, 0) == 0);
    assert(committed_count(root) == 2U * sizeof(first));
    retained_matches(root, "real-second", 7, sizeof(first), first, sizeof(first));
    retained_matches(root, "real-first", 1, 0, first, sizeof(first));
    puts("PASS: real pinned appendfat Arena stream commit, canonical byte readback, body-only v1 navigation refusal, duplicate/failed producer/stale/empty/denied-root/hash refusal and prior-byte reopening");
    puts("NOT_RUN: physical Android Binder, authorized Shizuku, ICU HTTPS, C67 app-private hosting");
    return 0;
}
