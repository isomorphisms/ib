#ifndef IB_ANDROID_ARENA_PFD_PROVIDER_H
#define IB_ANDROID_ARENA_PFD_PROVIDER_H

#include <stdint.h>
#include <sys/types.h>

#if defined(__ANDROID__) && __ANDROID_API__ >= 29
#include <android/binder_ibinder.h>

/* Source owner, not the shell process, supplies an already checked fresh FD
 * containing exactly one committed source. open_verified_fd may NOT return a
 * raw descriptor of the entire canonical Arena, a path or a network socket. */
typedef int (*ib_open_committed_source)(void *source_owner,
                                        const char *request_id,
                                        uint64_t navigation_generation,
                                        int *owned_read_fd);

typedef struct {
    uid_t allowed_reader_uid;             /* checked application UID, not shell */
    ib_open_committed_source open_verified_fd;
    void *source_owner;
} ib_arena_pfd_provider;

/* Construct a private app-owned Binder object. The app's trusted host must
 * deliver this capability to its allowed reader; it is NOT registered with
 * the global Android service manager. Provider configuration must outlive
 * the returned Binder object. Caller releases returned Binder reference. */
AIBinder *ib_arena_pfd_provider_new(ib_arena_pfd_provider *provider);
#endif
#endif
