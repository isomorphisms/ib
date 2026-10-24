#ifndef IB_SHIZUKU_ARENA_INGRESS_H
#define IB_SHIZUKU_ARENA_INGRESS_H

#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "ib_reader_pfd.h"

#define IB_INGRESS_URL_CAP 2048U
#define IB_INGRESS_TIMEOUT_MILLISECONDS 35000
/* The paired ICU frame carries a 26-byte header and bounded metadata in
 * addition to the body. The publisher validates the body limit separately. */
#define IB_INGRESS_FRAME_HEADER_BYTES 26U
#define IB_INGRESS_METADATA_MAX_BYTES 8192U
#define IB_INGRESS_STREAM_MAX_BYTES \
    (IB_READER_SOURCE_MAX_BYTES + IB_INGRESS_METADATA_MAX_BYTES + \
     IB_INGRESS_FRAME_HEADER_BYTES)

/* Storage owner is the only component permitted to append the source.
 * Exactly one call follows a fully received stream, a confirmed worker exit
 * status of zero, and a matching current browser navigation generation.
 * Publisher checks duplicate request IDs and independently read-back verifies
 * the canonical userspace appendfat Arena before publishing a receipt. */
typedef int (*ib_ingress_publish)(void *source_owner,
                                  const char *request_id,
                                  uint64_t navigation_generation,
                                  const char *requested_url,
                                  const unsigned char *exact_bytes,
                                  size_t byte_count,
                                  const char sha256_hex[65]);

typedef enum {
    IB_INGRESS_IDLE,
    IB_INGRESS_ARMED,
    IB_INGRESS_STREAMING,
    IB_INGRESS_FINISHING,
    IB_INGRESS_CLOSED
} ib_ingress_state;

typedef struct {
    pthread_mutex_t mutex;
    atomic_uint_fast64_t wanted_navigation_generation;
    atomic_bool cancelled;
    uid_t permitted_bridge_uid;
    uint64_t request_generation;
    char request_id[IB_READER_REQUEST_ID_CAP + 1U];
    char requested_url[IB_INGRESS_URL_CAP + 1U];
    ib_ingress_state state;
    int source_fd;
    pthread_t read_thread;
    int thread_started;
    unsigned char *bytes;
    size_t byte_count;
    int stream_complete;
    ib_ingress_publish publish;
    void *source_owner;
} ib_shizuku_arena_ingress;

/* The app/browser owner calls arm with the current accepted navigation and
 * a verified, explicitly authorized bridge process UID. No token or URL from
 * page content is accepted as Binder authority. One request per instance. */
int ib_ingress_init(ib_shizuku_arena_ingress *session,
                    ib_ingress_publish publish, void *source_owner);
int ib_ingress_arm(ib_shizuku_arena_ingress *session,
                   const char *request_id, uint64_t navigation_generation,
                   const char *requested_url, uid_t explicitly_allowed_uid);
void ib_ingress_want(ib_shizuku_arena_ingress *session,
                     uint64_t navigation_generation);

/* Returns an owned blocking WRITE pipe descriptor suitable for Binder PFD.
 * The app retains only the READ end and consumes in a bounded worker thread.
 * No app-private Arena pathname or FD is passed to Shizuku. */
int ib_ingress_begin(ib_shizuku_arena_ingress *session,
                     uid_t calling_uid,
                     const char *request_id, uint64_t generation,
                     int *owned_write_fd);

/* The authorized bridge calls finish only after ICU exits and the transferred
 * write PFD is closed; nonzero/timeout producer exit is a mandatory refusal.
 * No source becomes visible until verified canonical commit and receipt. */
int ib_ingress_finish(ib_shizuku_arena_ingress *session,
                      uid_t calling_uid,
                      const char *request_id, uint64_t generation,
                      int producer_exit_status);
void ib_ingress_dispose(ib_shizuku_arena_ingress *session);

#endif
