#define _POSIX_C_SOURCE 200809L
#include "../native/android/ib_arena_source_owner.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static unsigned char expected_source[] = {'<','p','>','I','B',' ','C','6','7','<','/','p','>','\n'};
static const char *request_id = "verified";
static const char *name = "receipts/verified.tsv";

static void full_file(const char *path, unsigned char **bytes, size_t *length) {
    FILE *file = fopen(path, "rb");
    assert(file != NULL);
    assert(fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file);
    assert(size > 0 && size < 8192);
    assert(fseek(file, 0, SEEK_SET) == 0);
    *bytes = malloc((size_t)size + 1U);
    assert(*bytes != NULL);
    assert(fread(*bytes, 1, (size_t)size, file) == (size_t)size);
    (*bytes)[size] = '\0';
    assert(fclose(file) == 0);
    *length = (size_t)size;
}

static void overwrite(const char *path, const unsigned char *bytes, size_t count) {
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    assert(fwrite(bytes, 1, count, file) == count);
    assert(fflush(file) == 0);
    assert(fclose(file) == 0);
}

static void preserved(const ib_reader_source *reader,
                      const char *id, uint64_t generation) {
    assert(reader->bytes != NULL);
    assert(reader->byte_count == sizeof(expected_source));
    assert(memcmp(reader->bytes, expected_source, sizeof(expected_source)) == 0);
    assert(reader->published_navigation_generation == generation);
    assert(strcmp(reader->request_id, id) == 0);
}

static void accept(ib_arena_source_owner *owner, ib_reader_source *reader,
                   const char *id, uint64_t generation) {
    ib_reader_source_ticket ticket = {0};
    int fd = -1;
    assert(ib_arena_source_owner_open(owner, id, generation,
                                     &ticket, &fd) == 0);
    assert(fd >= 0);
    assert(ticket.byte_count == sizeof(expected_source));
    assert(ticket.navigation_generation == generation);
    assert(strcmp(ticket.request_id, id) == 0);
    char local_digest[65];
    ib_reader_sha256_hex(expected_source, sizeof(expected_source), local_digest);
    assert(strcmp(ticket.sha256_hex, local_digest) == 0);
    assert(ib_reader_source_accept_fd(reader, &ticket, fd)
           == IB_READER_PFD_ACCEPTED);
    close(fd);
    preserved(reader, id, generation);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    char private_root[4096], receipt[4096], arena[4096], pointer[4096];
    assert(snprintf(private_root, sizeof(private_root), "%s", argv[1])
           < (int)sizeof(private_root));
    assert(snprintf(receipt, sizeof(receipt), "%s/%s", private_root, name)
           < (int)sizeof(receipt));
    assert(snprintf(arena, sizeof(arena), "%s/responses.arena", private_root)
           < (int)sizeof(arena));
    assert(snprintf(pointer, sizeof(pointer), "%s/responses.arena.used", private_root)
           < (int)sizeof(pointer));

    ib_arena_source_owner owner;
    assert(ib_arena_source_owner_init(&owner, private_root) == 0);
    ib_reader_source reader;
    ib_reader_source_init(&reader);
    ib_arena_source_owner_want(&owner, 1);
    ib_reader_source_want(&reader, 1);
    accept(&owner, &reader, request_id, 1);
    int fd = -1;
    /* A legacy v1 receipt has matching bytes but no observed admission. */
    assert(ib_arena_source_owner_open(&owner, "legacy", 1, NULL, &fd) != 0);
    assert(fd == -1);

    ib_reader_source_ticket binder_ticket = {0};
    assert(ib_arena_source_owner_open_binder(&owner, request_id, 2, &binder_ticket, &fd) != 0);
    assert(fd == -1);
    assert(ib_arena_source_owner_open(&owner, "../bad", 1, NULL, &fd) != 0);
    preserved(&reader, request_id, 1);

    unsigned char *original = NULL;
    size_t count = 0;
    full_file(receipt, &original, &count);
    unsigned char *damaged = malloc(count + 1U);
    assert(damaged != NULL);
    memcpy(damaged, original, count + 1U);
    char *digest = strstr((char *)damaged, "sha256\t");
    assert(digest != NULL);
    digest += strlen("sha256\t");
    *digest = *digest == '0' ? '1' : '0';
    overwrite(receipt, damaged, count);
    assert(ib_arena_source_owner_open(&owner, request_id, 1, NULL, &fd) != 0);
    assert(fd == -1);
    preserved(&reader, request_id, 1);
    overwrite(receipt, original, count);

    /* Every observed source/admission field participates in PFD authority:
     * the byte hash alone is insufficient for an admitted navigation. */
    const char *mutations[][2] = {
        {"source_generation\t1", "source_generation\t2"},
        {"source_key\tarena:verified", "source_key\tarena:verifieX"},
        {"http_status\t200", "http_status\t404"},
        {"mime_type\ttext/html", "mime_type\ttext/junk"},
        {"charset\tutf-8", "charset\tutf-7"},
        {"content_encoding\tidentity", "content_encoding\tidentitx"},
        {"utf8\tvalid", "utf8\twrong"},
        {"received_length\t14", "received_length\t13"},
        {"declared_length\t14", "declared_length\t13"},
        {"acquisition_complete\tYES", "acquisition_complete\tNAY"},
        {"navigation_admitted\tYES", "navigation_admitted\tNAY"}
    };
    for (size_t i = 0; i < sizeof(mutations) / sizeof(mutations[0]); ++i) {
        const char *before = mutations[i][0], *after = mutations[i][1];
        assert(strlen(before) == strlen(after));
        memcpy(damaged, original, count + 1U);
        char *location = strstr((char *)damaged, before);
        assert(location != NULL);
        memcpy(location, after, strlen(after));
        overwrite(receipt, damaged, count);
        assert(ib_arena_source_owner_open(&owner, request_id, 1,
                                          NULL, &fd) != 0);
        assert(fd == -1);
        preserved(&reader, request_id, 1);
    }
    overwrite(receipt, original, count);

    /* Last-good retained when Arena source bytes no longer match receipt. */
    int file = open(arena, O_RDWR);
    assert(file >= 0);
    unsigned char first = 0;
    assert(pread(file, &first, 1, 0) == 1);
    unsigned char changed = (unsigned char)(first ^ 0xffU);
    assert(pwrite(file, &changed, 1, 0) == 1);
    assert(ib_arena_source_owner_open(&owner, request_id, 1, NULL, &fd) != 0);
    assert(fd == -1);
    preserved(&reader, request_id, 1);
    assert(pwrite(file, &first, 1, 0) == 1);
    close(file);

    unsigned char *saved_pointer = NULL;
    size_t used_count = 0;
    full_file(pointer, &saved_pointer, &used_count);
    overwrite(pointer, (const unsigned char *)"999999999\n", 10);
    assert(ib_arena_source_owner_open(&owner, request_id, 1, NULL, &fd) != 0);
    assert(fd == -1);
    overwrite(pointer, saved_pointer, used_count);
    free(saved_pointer);

    /* A substituted symlink or world-readable private root is refused. */
    char alias[4096];
    assert(snprintf(alias, sizeof(alias), "%s-symlink", private_root)
           < (int)sizeof(alias));
    assert(symlink(private_root, alias) == 0);
    ib_arena_source_owner linked_owner;
    assert(ib_arena_source_owner_init(&linked_owner, alias) == 0);
    ib_arena_source_owner_want(&linked_owner, 1);
    assert(ib_arena_source_owner_open(&linked_owner, request_id, 1, NULL, &fd) != 0);
    assert(unlink(alias) == 0);
    assert(chmod(private_root, 0755) == 0);
    assert(ib_arena_source_owner_open(&owner, request_id, 1, NULL, &fd) != 0);
    assert(chmod(private_root, 0700) == 0);

    ib_arena_source_owner_want(&owner, 2);
    ib_reader_source_want(&reader, 2);
    /* Replaying the same admitted generation as a newer navigation refuses. */
    assert(ib_arena_source_owner_open(&owner, request_id, 2, NULL, &fd) != 0);
    assert(fd == -1);
    preserved(&reader, request_id, 1);
    accept(&owner, &reader, "verified2", 2);
    preserved(&reader, "verified2", 2);

    /* Back selects immutable source generation 1 while advancing the reader's
     * navigation epoch to 3. It must not relabel or refetch those source bytes. */
    ib_arena_source_owner_want(&owner, 3);
    ib_reader_source_want(&reader, 3);
    assert(ib_arena_source_owner_open(&owner, request_id, 3, NULL, &fd) != 0);
    assert(fd == -1);
    assert(ib_arena_source_owner_open_retained(&owner, request_id, 2, 3, NULL, &fd) != 0);
    assert(fd == -1);
    assert(ib_arena_source_owner_open_retained(&owner, request_id, 1, 2, NULL, &fd) != 0);
    assert(fd == -1);
    assert(ib_arena_source_owner_open_retained(&owner, request_id, 0, 3, NULL, &fd) != 0);
    assert(fd == -1);
    preserved(&reader, "verified2", 2);
    ib_reader_source_ticket retained_ticket = {0};
    puts("STAGE retained source generation 1 at navigation generation 3");
    fflush(stdout);
    assert(ib_arena_source_owner_open_retained(&owner, request_id, 1, 3,
                                               &retained_ticket, &fd) == 0);
    assert(retained_ticket.navigation_generation == 3);
    assert(strcmp(retained_ticket.request_id, request_id) == 0);
    assert(ib_reader_source_accept_fd(&reader, &retained_ticket, fd)
           == IB_READER_PFD_ACCEPTED);
    close(fd);
    preserved(&reader, request_id, 3);

    /* A newer navigation invalidates an in-flight retained read as before. */
    ib_arena_source_owner_want(&owner, 4);
    ib_reader_source_want(&reader, 4);
    assert(ib_arena_source_owner_open_retained(&owner, request_id, 1, 3, NULL, &fd) != 0);
    assert(fd == -1);
    assert(ib_arena_source_owner_open_retained(&owner, request_id, 1, 4,
                                               &retained_ticket, &fd) == 0);
    ib_arena_source_owner_want(&owner, 5);
    ib_reader_source_want(&reader, 5);
    assert(ib_reader_source_accept_fd(&reader, &retained_ticket, fd)
           == IB_READER_PFD_STALE);
    close(fd);
    preserved(&reader, request_id, 3);

    unsigned char *retained_receipt = NULL;
    size_t retained_count = 0;
    full_file(receipt, &retained_receipt, &retained_count);
    assert(retained_count == count && memcmp(retained_receipt, original, count) == 0);
    free(retained_receipt);
    ib_reader_source_clear(&reader);
    free(original);
    free(damaged);
    puts("PASS: v2 HTML receipt admission, matching navigation generation, pipe/double SHA, v1/stale/forged-metadata/byte/permission/symlink/corrupt-pointer refusal and recovery");
    puts("PASS: retained source generation stays immutable across a newer navigation; stale retained reads preserve the last good document");
    puts("NOT_RUN: Android private Binder transaction, actual Shizuku/ICU network, Idric reader screen or physical TAB_P10");
    return 0;
}
