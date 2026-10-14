#ifndef IB_ANDROID_VERIFIED_ARENA_SOURCE_H
#define IB_ANDROID_VERIFIED_ARENA_SOURCE_H

#include "ib_reader_pfd.h"
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#define IB_ARENA_PRIVATE_ROOT_CAP 4096U

typedef struct {
    char private_root[IB_ARENA_PRIVATE_ROOT_CAP];
    atomic_uint_fast64_t wanted_navigation_generation;
} ib_arena_source_owner;

/* The trusted application host passes its existing app-private Arena root
 * (e.g. Context.getFilesDir()/arena). This does not choose/create a store. */
int ib_arena_source_owner_init(ib_arena_source_owner *owner,
                               const char *trusted_private_root);
void ib_arena_source_owner_want(ib_arena_source_owner *owner,
                                uint64_t navigation_generation);

/* All-or-nothing source-open. The already committed canonical appendfat
 * range and private receipt are re-read and SHA-256 checked before returning
 * an independent, read-only anonymous pipe. The returned receipt-derived
 * ticket is the sole expected metadata for the reader; navigation itself is
 * NOT admitted by this byte-transfer function. Caller owns *read_fd. */
int ib_arena_source_owner_open(ib_arena_source_owner *owner,
                               const char *request_id,
                               uint64_t navigation_generation,
                               ib_reader_source_ticket *ticket,
                               int *read_fd);

/* Adapter for ib_arena_pfd_provider.open_verified_fd. */
int ib_arena_source_owner_open_binder(void *owner, const char *request_id,
                                      uint64_t navigation_generation,
                                      ib_reader_source_ticket *verified_ticket,
                                      int *read_fd);

#endif
