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

static unsigned char expected_source[] = {'I','B',0,'C','6','7',255,'\n'};
static const char *request_id = "verified";
static const char *name = "receipts/verified.tsv";

static void full_file(const char *path, unsigned char **bytes, size_t *length) {
    FILE *file = fopen(path, "rb");
    assert(file != NULL);
    assert(fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file);
    assert(size > 0 && size < 8192);
    assert(fseek(file, 0, SEEK_SET) == 0);
    *bytes = malloc((size_t)size);
    assert(*bytes != NULL);
    assert(fread(*bytes, 1, (size_t)size, file) == (size_t)size);
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
                   uint64_t generation) {
    ib_reader_source_ticket ticket = {0};
    int fd = -1;
    assert(ib_arena_source_owner_open(owner, request_id, generation,
                                     &ticket, &fd) == 0);
    assert(fd >= 0);
    assert(ticket.byte_count == sizeof(expected_source));
    assert(ticket.navigation_generation == generation);
    assert(strcmp(ticket.request_id, request_id) == 0);
    char local_digest[65];
    ib_reader_sha256_hex(expected_source, sizeof(expected_source), local_digest);
    assert(strcmp(ticket.sha256_hex, local_digest) == 0);
    assert(ib_reader_source_accept_fd(reader, &ticket, fd)
           == IB_READER_PFD_ACCEPTED);
    close(fd);
    preserved(reader, request_id, generation);
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
    accept(&owner, &reader, 1);

    int fd = -1;
    assert(ib_arena_source_owner_open_binder(&owner, request_id, 2, &fd) != 0);
    assert(fd == -1);
    assert(ib_arena_source_owner_open(&owner, "../bad", 1, NULL, &fd) != 0);
    preserved(&reader, request_id, 1);

    unsigned char *original = NULL;
    size_t count = 0;
    full_file(receipt, &original, &count);
    unsigned char *damaged = malloc(count);
    assert(damaged != NULL);
    memcpy(damaged, original, count);
    char *digest = strstr((char *)damaged, "sha256\t");
    assert(digest != NULL);
    digest += strlen("sha256\t");
    *digest = *digest == '0' ? '1' : '0';
    overwrite(receipt, damaged, count);
    assert(ib_arena_source_owner_open(&owner, request_id, 1, NULL, &fd) != 0);
    assert(fd == -1);
    preserved(&reader, request_id, 1);
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
    accept(&owner, &reader, 2);
    preserved(&reader, request_id, 2);
    ib_reader_source_clear(&reader);
    free(original);
    free(damaged);
    puts("PASS: canonical appendfat receipt + commit pointer, binary PFD, end-to-end double SHA, stale/URL/digest/byte/permission/symlink/corrupt-pointer refusal and recovery");
    puts("NOT_RUN: Android private Binder transaction, actual Shizuku/ICU network, Idric reader screen or physical C67");
    return 0;
}
