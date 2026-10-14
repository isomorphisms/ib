#include "ib_reader_host_assembly.h"

#if defined(__ANDROID__) && __ANDROID_API__ >= 29

#include <string.h>

int ib_reader_host_assembly_init(ib_reader_host_assembly *assembly,
                                 const char *trusted_private_arena_root,
                                 uid_t allowed_reader_uid) {
    if (!assembly || allowed_reader_uid < (uid_t)10000)
        return -1;
    memset(assembly, 0, sizeof(*assembly));
    if (ib_arena_source_owner_init(&assembly->source,
                                   trusted_private_arena_root) != 0)
        return -1;
    assembly->descriptor_provider.allowed_reader_uid = allowed_reader_uid;
    assembly->descriptor_provider.open_verified_fd =
        ib_arena_source_owner_open_binder;
    assembly->descriptor_provider.source_owner = &assembly->source;
    assembly->binder =
        ib_arena_pfd_provider_new(&assembly->descriptor_provider);
    return assembly->binder ? 0 : -1;
}

void ib_reader_host_assembly_want(ib_reader_host_assembly *assembly,
                                  uint64_t navigation_generation) {
    if (assembly)
        ib_arena_source_owner_want(&assembly->source, navigation_generation);
}

void ib_reader_host_assembly_release_local_reference(
    ib_reader_host_assembly *assembly) {
    if (!assembly || !assembly->binder) return;
    AIBinder_decStrong(assembly->binder);
    assembly->binder = NULL;
}
#endif
