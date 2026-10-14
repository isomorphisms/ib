#define _POSIX_C_SOURCE 200809L
#include "ib_arena_pfd_provider.h"

#if defined(__ANDROID__) && __ANDROID_API__ >= 29

#include <android/binder_parcel.h>
#include <android/binder_status.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define REQUEST_MAX_BYTES 64
#define OPEN_COMMITTED_SOURCE_TRANSACTION 1U

typedef struct {
    char request_id[REQUEST_MAX_BYTES + 1U];
    int32_t provided_capacity;
} parsed_request;

static bool allocate_request(void *userdata, int32_t length, char **buffer) {
    parsed_request *request = (parsed_request *)userdata;
    if (!request || !buffer || length <= 0 ||
        length > (int32_t)sizeof(request->request_id))
        return false;
    request->provided_capacity = length;
    *buffer = request->request_id;
    return true;
}

static bool valid_id(const parsed_request *request) {
    if (!request || request->provided_capacity <= 1) return false;
    size_t length = strnlen(request->request_id,
                           sizeof(request->request_id));
    if (length == 0 || length >= sizeof(request->request_id) ||
        length + 1U != (size_t)request->provided_capacity)
        return false;
    for (size_t index = 0; index < length; ++index) {
        unsigned char c = (unsigned char)request->request_id[index];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              c == '.' || c == '_' || c == '-'))
            return false;
    }
    return true;
}

static void *created(void *userdata) {
    return userdata;
}

static void destroyed(void *userdata) {
    /* The trusted application host owns this configuration throughout the
     * lifetime of the bound private Binder service. */
    (void)userdata;
}

static binder_status_t respond(AParcel *out, int32_t error) {
    return AParcel_writeInt32(out, error);
}

static binder_status_t transact(AIBinder *binder, transaction_code_t code,
                                const AParcel *in, AParcel *out) {
    if (code != OPEN_COMMITTED_SOURCE_TRANSACTION)
        return STATUS_UNKNOWN_TRANSACTION;
    ib_arena_pfd_provider *provider =
        (ib_arena_pfd_provider *)AIBinder_getUserData(binder);
    if (!provider || !provider->open_verified_fd)
        return respond(out, 4);

    /* Read access is explicitly app-scoped; UID 2000 running ICU is not a
     * reader and has no implicit authority over private retained sources. */
    if (AIBinder_getCallingUid() != provider->allowed_reader_uid)
        return respond(out, 1);

    parsed_request request = {{0}, 0};
    int64_t generation = 0;
    binder_status_t status = AParcel_readString(in, &request,
                                                allocate_request);
    if (status != STATUS_OK)
        return respond(out, 2);
    status = AParcel_readInt64(in, &generation);
    if (status != STATUS_OK || generation <= 0 || !valid_id(&request))
        return respond(out, 2);

    int descriptor = -1;
    int result = provider->open_verified_fd(provider->source_owner,
                                           request.request_id,
                                           (uint64_t)generation,
                                           &descriptor);
    if (result != 0 || descriptor < 0) {
        if (descriptor >= 0) close(descriptor);
        return respond(out, 3);
    }
    status = respond(out, 0);
    if (status == STATUS_OK)
        status = AParcel_writeParcelFileDescriptor(out, descriptor);
    close(descriptor);
    return status;
}

AIBinder *ib_arena_pfd_provider_new(ib_arena_pfd_provider *provider) {
    if (!provider || !provider->open_verified_fd ||
        provider->allowed_reader_uid == (uid_t)-1)
        return NULL;
    /* Called at single-threaded host initialization, not once per request. */
    static AIBinder_Class *binder_class = NULL;
    if (!binder_class) {
        binder_class = AIBinder_Class_define(
            "org.isomorphisms.ib.ArenaCommittedSource.v1",
            created, destroyed, transact
        );
    }
    if (!binder_class) return NULL;
    return AIBinder_new(binder_class, provider);
}
#endif
