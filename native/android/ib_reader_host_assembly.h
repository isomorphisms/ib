#ifndef IB_ANDROID_READER_HOST_ASSEMBLY_H
#define IB_ANDROID_READER_HOST_ASSEMBLY_H

#include "ib_arena_source_owner.h"
#include "ib_arena_pfd_provider.h"

#if defined(__ANDROID__) && __ANDROID_API__ >= 29

/* App host owns the memory and must keep it stable until the local Binder
 * object and all in-flight calls are gone. Browser event dispatch, not the
 * renderer or Shizuku, advances the expected source generation. */
typedef struct {
    ib_arena_source_owner source;
    ib_arena_pfd_provider descriptor_provider;
    AIBinder *binder;
} ib_reader_host_assembly;

/* The source owner runs at the app UID and uses its existing private Arena.
 * allowed_reader_uid must be obtained from trusted Android package identity.
 * Neither shell UID 2000 nor a caller-controlled UID is accepted. */
int ib_reader_host_assembly_init(ib_reader_host_assembly *assembly,
                                 const char *trusted_private_arena_root,
                                 uid_t allowed_reader_uid);
void ib_reader_host_assembly_want(ib_reader_host_assembly *assembly,
                                  uint64_t navigation_generation);

/* Host must first stop Binder calls and release its external references.
 * Caller must not free assembly until the Binder object's final destruction. */
void ib_reader_host_assembly_release_local_reference(
    ib_reader_host_assembly *assembly);
#endif

#endif
