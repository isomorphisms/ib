#ifndef IB_ANDROID_READER_PFD_H
#define IB_ANDROID_READER_PFD_H

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

/* Reader accepts one complete bounded snapshot, never an Arena pathname.
 * This C boundary consumes a descriptor received through Android Binder/PFD.
 * Browser/navigation ownership remains with the checked Idric core. */
#define IB_READER_SOURCE_MAX_BYTES (4U * 1024U * 1024U)
#define IB_READER_REQUEST_ID_CAP 64U

typedef struct {
    char request_id[IB_READER_REQUEST_ID_CAP + 1U];
    uint64_t navigation_generation;
    uint32_t byte_count;
    char sha256_hex[65];
} ib_reader_source_ticket;

typedef struct {
    atomic_uint_fast64_t wanted_navigation_generation;
    unsigned char *bytes;
    size_t byte_count;
    uint64_t published_navigation_generation;
    char request_id[IB_READER_REQUEST_ID_CAP + 1U];
} ib_reader_source;

enum {
    IB_READER_PFD_ACCEPTED = 0,
    IB_READER_PFD_BAD_TICKET = 1,
    IB_READER_PFD_STALE = 2,
    IB_READER_PFD_IO = 3,
    IB_READER_PFD_DIGEST = 4,
    IB_READER_PFD_BINDER = 5
};

/* Initialize once, use in a serialized reader-owned publication queue.
 * Navigation may invalidate an in-flight read from another thread.
 * Published bytes remain owned by the reader until a successful replacement. */
void ib_reader_source_init(ib_reader_source *source);
void ib_reader_source_clear(ib_reader_source *source);
void ib_reader_source_want(ib_reader_source *source, uint64_t generation);

/* Same checked SHA-256 implementation for reader and source owner; never
 * substitute textual CLI output for the captured binary response. */
void ib_reader_sha256_hex(const unsigned char *bytes, size_t count,
                          char hex[65]);

/* Does NOT close fd. Requires a finite stream ending at exactly byte_count.
 * Checks deadline, length, SHA-256 and generation before exposing any byte. */
int ib_reader_source_accept_fd(ib_reader_source *source,
                               const ib_reader_source_ticket *ticket,
                               int fd);

#if defined(__ANDROID__) && __ANDROID_API__ >= 29
#include <android/binder_ibinder.h>

/* provider is an already authenticated private Binder capability, NOT an
 * Android service-manager name or a Shizuku command. On transaction 1 it
 * receives request_id and navigation_generation, then returns status=0 and
 * a ParcelFileDescriptor of the already committed immutable source bytes.
 * No untrusted URL, pathname, or body travels through a Parcel. */
int ib_reader_source_fetch_binder(ib_reader_source *source,
                                  const ib_reader_source_ticket *ticket,
                                  AIBinder *provider);
#endif
#endif
