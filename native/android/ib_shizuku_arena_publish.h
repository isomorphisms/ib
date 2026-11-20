#ifndef IB_SHIZUKU_ARENA_PUBLISH_H
#define IB_SHIZUKU_ARENA_PUBLISH_H

#include <stddef.h>
#include <stdint.h>

#define IB_INGRESS_ARENA_ROOT_CAP 4096U

typedef struct {
    char private_root[IB_INGRESS_ARENA_ROOT_CAP];
} ib_ingress_arena;

/* App-owned, preinitialized 0700 Arena root. Existing responses.arena,
 * .used and .lock files, receipts/ and locks/ directories must already
 * exist. Never create another canonical source store or let Shizuku open it. */
int ib_ingress_arena_init(ib_ingress_arena *arena, const char *private_root);

/* Implements ib_ingress_publish. Uses the canonical appendfat_arena C API
 * pinned by the application. Commit and independent readback precede the
 * immutable, sync'd v1 source receipt. Unsupported HTTP metadata remains
 * explicitly UNOBSERVED; navigation_admitted remains NO. */
int ib_ingress_arena_publish(void *arena, const char *request_id,
                             uint64_t navigation_generation,
                             const char *requested_url,
                             const unsigned char *exact_bytes,
                             size_t byte_count,
                             const char sha256_hex[65]);

/* Paired ICU capture only: validates the exact IBICU01 frame and observed
 * metadata before appending its body. Publishes the existing Arena v2 receipt.
 * A body-only stream is never promoted to an admitted response. */
int ib_ingress_arena_publish_frame(void *arena, const char *request_id,
                                   uint64_t navigation_generation,
                                   const char *requested_url,
                                   const unsigned char *frame,
                                   size_t frame_bytes,
                                   const char frame_sha256_hex[65]);

#endif
