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

/* Observations copied by the one maintained receipt parser after committed
 * source checks. Original representation admission remains an observation,
 * separate from the decoder-input contract. No Intent supplies this object. */
typedef struct {
    char source_key[IB_READER_REQUEST_ID_CAP + 7U];
    uint64_t source_generation;
    char requested_url[2049], resolved_url[2049];
    char mime_type[64], charset[64], http_status[4];
    char redirect_chain[8193];
    /* Empty means absent in a retained legacy receipt, never inferred identity. */
    char transfer_decoding[8];
    int utf8_valid, navigation_admitted;
} ib_arena_source_observations;

/* The trusted application host passes its existing app-private Arena root
 * (e.g. Context.getFilesDir()/arena). This does not choose/create a store. */
int ib_arena_source_owner_init(ib_arena_source_owner *owner,
                               const char *trusted_private_root);
void ib_arena_source_owner_want(ib_arena_source_owner *owner,
                                uint64_t navigation_generation);

/* All-or-nothing admitted-source open. Requires IB ICU/Arena v2 with
 * observed HTTP 200, HTML/UTF-8, completed acquisition, positive admission
 * and a receipt source_generation matching navigation_generation. Legacy
 * body-only v1 receipts never reach the reader. Independently reopen the
 * canonical committed byte range and check SHA-256 before returning a
 * read-only anonymous pipe. The browser still controls navigation ordering
 * and the reader checks the ticket, stream and digest again.
 * Caller owns *read_fd. */
int ib_arena_source_owner_open(ib_arena_source_owner *owner,
                               const char *request_id,
                               uint64_t navigation_generation,
                               ib_reader_source_ticket *ticket,
                               int *read_fd);

/* Reopen an already committed source selected by trusted browser history.
 * retained_source_generation is the immutable generation in that source's
 * receipt; navigation_generation is the current disposable-reader request.
 * Validate both independently. Never rewrite a source receipt to make Back,
 * Forward or process recreation appear to be a new acquisition. The trusted
 * application callback supplies the retained generation, not a Binder parcel
 * or a renderer. The existing open entrypoint keeps its stricter initial-
 * acquisition contract where both generations must coincide. */
int ib_arena_source_owner_open_retained(ib_arena_source_owner *owner,
                                       const char *request_id,
                                       uint64_t retained_source_generation,
                                       uint64_t navigation_generation,
                                       ib_reader_source_ticket *ticket,
                                       int *read_fd);

int ib_arena_source_owner_open_observed(ib_arena_source_owner *owner,
                                       const char *request_id,
                                       uint64_t retained_source_generation,
                                       uint64_t navigation_generation,
                                       ib_reader_source_ticket *ticket,
                                       ib_arena_source_observations *observations,
                                       int *read_fd);

/* Original-byte input to a checked decoder, not HTML reader admission.
 * Validate the same response identity, generation, complete acquisition,
 * canonical range and independently reopened digest. Preserve the original
 * charset, status and admission observation even when the representation is
 * unsupported. The checked reader must then admit the decoded representation;
 * successfully opening this pipe alone never authorizes visible content.
 * The older admitted-source APIs retain their strict UTF-8 contract. */
int ib_arena_source_owner_open_decode_input(ib_arena_source_owner *owner,
                                       const char *request_id,
                                       uint64_t retained_source_generation,
                                       uint64_t navigation_generation,
                                       ib_reader_source_ticket *ticket,
                                       ib_arena_source_observations *observations,
                                       int *read_fd);

/* Adapter for ib_arena_pfd_provider.open_verified_fd. */
int ib_arena_source_owner_open_binder(void *owner, const char *request_id,
                                      uint64_t navigation_generation,
                                      ib_reader_source_ticket *verified_ticket,
                                      int *read_fd);

#endif
