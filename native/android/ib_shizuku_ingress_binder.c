#define _POSIX_C_SOURCE 200809L
#include "ib_shizuku_ingress_binder.h"

#if defined(__ANDROID__) && __ANDROID_API__ >= 29

#include <android/binder_parcel.h>
#include <android/binder_status.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char id[IB_READER_REQUEST_ID_CAP + 1U];
    int32_t capacity;
} request_name;

static bool allocate_name(void *opaque, int32_t length, char **out) {
    request_name *name = opaque;
    if (!name || !out || length <= 1 ||
        length > (int32_t)sizeof(name->id)) return false;
    name->capacity = length;
    *out = name->id;
    return true;
}

static int valid_name(const request_name *name) {
    if (!name || name->capacity < 2) return 0;
    size_t n = strnlen(name->id, sizeof(name->id));
    if (n + 1U != (size_t)name->capacity || !n ||
        (n == 1U && name->id[0] == '.') ||
        (n == 2U && name->id[0] == '.' && name->id[1] == '.'))
        return 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)name->id[i];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}

typedef struct {
    char url[IB_INGRESS_URL_CAP + 1U];
    int32_t capacity;
} authorized_url_reply;

static bool allocate_url(void *opaque, int32_t length, char **out) {
    authorized_url_reply *reply = opaque;
    if (!reply || !out || length < 10 ||
        length > (int32_t)sizeof(reply->url)) return false;
    reply->capacity = length;
    *out = reply->url;
    return true;
}

static void *created(void *user_data) { return user_data; }
static void destroyed(void *user_data) { (void)user_data; }

static binder_status_t serve(AIBinder *binder, transaction_code_t code,
                             const AParcel *in, AParcel *out) {
    if (code != IB_SHIZUKU_INGRESS_BEGIN &&
        code != IB_SHIZUKU_INGRESS_FINISH) return STATUS_UNKNOWN_TRANSACTION;
    ib_shizuku_arena_ingress *session =
        (ib_shizuku_arena_ingress *)AIBinder_getUserData(binder);
    if (!session) return AParcel_writeInt32(out, -1);

    request_name name = {{0}, 0};
    int64_t generation = 0;
    binder_status_t status = AParcel_readString(in, &name, allocate_name);
    if (status != STATUS_OK || !valid_name(&name))
        return AParcel_writeInt32(out, -1);
    status = AParcel_readInt64(in, &generation);
    if (status != STATUS_OK || generation <= 0)
        return AParcel_writeInt32(out, -1);

    uid_t calling_uid = (uid_t)AIBinder_getCallingUid();
    if (code == IB_SHIZUKU_INGRESS_BEGIN) {
        int fd = -1;
        int accepted = ib_ingress_begin(session, calling_uid, name.id,
                                        (uint64_t)generation, &fd);
        if (accepted != 0 || fd < 0) {
            if (fd >= 0) close(fd);
            return AParcel_writeInt32(out, -1);
        }
        status = AParcel_writeInt32(out, 0);
        if (status == STATUS_OK)
            status = AParcel_writeString(
                out, session->requested_url,
                (int32_t)strlen(session->requested_url));
        if (status == STATUS_OK)
            status = AParcel_writeParcelFileDescriptor(out, fd);
        close(fd);
        if (status != STATUS_OK)
            ib_ingress_want(session, 0);
        return status;
    }

    int32_t process_exit_status = -1;
    status = AParcel_readInt32(in, &process_exit_status);
    if (status != STATUS_OK || process_exit_status < 0 ||
        process_exit_status > 255)
        return AParcel_writeInt32(out, -1);
    int result = ib_ingress_finish(session, calling_uid, name.id,
                                   (uint64_t)generation, process_exit_status);
    return AParcel_writeInt32(out, result == 0 ? 0 : -1);
}

static pthread_once_t binder_once = PTHREAD_ONCE_INIT;
static AIBinder_Class *ingress_class = NULL;

static void initialize_class(void) {
    ingress_class = AIBinder_Class_define(
        "org.isomorphisms.ib.ShizukuArenaIngress.v1",
        created, destroyed, serve
    );
}
static const AIBinder_Class *get_ingress_class(void) {
    (void)pthread_once(&binder_once, initialize_class);
    return ingress_class;
}

AIBinder *ib_shizuku_ingress_binder_new(ib_shizuku_arena_ingress *session) {
    if (!session || !get_ingress_class()) return NULL;
    return AIBinder_new(get_ingress_class(), session);
}

static int prepare(AIBinder *binder, const char *id, uint64_t generation,
                   AParcel **request) {
    if (!binder || !id || !generation ||
        !get_ingress_class() ||
        !AIBinder_associateClass(binder, get_ingress_class()))
        return -1;
    size_t len = strnlen(id, IB_READER_REQUEST_ID_CAP + 1U);
    if (len == 0 || len > IB_READER_REQUEST_ID_CAP) return -1;
    binder_status_t status = AIBinder_prepareTransaction(binder, request);
    if (status != STATUS_OK || !*request) return -1;
    status = AParcel_writeString(*request, id, (int32_t)len);
    if (status == STATUS_OK)
        status = AParcel_writeInt64(*request, (int64_t)generation);
    if (status != STATUS_OK) {
        AParcel_delete(*request);
        *request = NULL;
        return -1;
    }
    return 0;
}

int ib_shizuku_ingress_begin_remote(AIBinder *binder, const char *id,
                                    uint64_t generation,
                                    char app_authorized_url[IB_INGRESS_URL_CAP + 1U],
                                    int *write_fd) {
    if (write_fd) *write_fd = -1;
    if (app_authorized_url) app_authorized_url[0] = '\0';
    if (!write_fd || !app_authorized_url) return -1;
    AParcel *request = NULL, *reply = NULL;
    if (prepare(binder, id, generation, &request) != 0) return -1;
    binder_status_t status = AIBinder_transact(
        binder, IB_SHIZUKU_INGRESS_BEGIN, &request, &reply, 0);
    if (status != STATUS_OK || !reply) {
        if (reply) AParcel_delete(reply);
        return -1;
    }
    int32_t remote_status = -1;
    int descriptor = -1;
    authorized_url_reply authorized_url = {{0}, 0};
    status = AParcel_readInt32(reply, &remote_status);
    if (status == STATUS_OK && remote_status == 0)
        status = AParcel_readString(reply, &authorized_url, allocate_url);
    if (status == STATUS_OK && remote_status == 0) {
        if (authorized_url.capacity < 10 ||
            strnlen(authorized_url.url, sizeof(authorized_url.url)) + 1U !=
                (size_t)authorized_url.capacity)
            status = STATUS_BAD_VALUE;
    }
    if (status == STATUS_OK && remote_status == 0)
        status = AParcel_readParcelFileDescriptor(reply, &descriptor);
    AParcel_delete(reply);
    if (status != STATUS_OK || remote_status != 0 || descriptor < 0) {
        if (descriptor >= 0) close(descriptor);
        return -1;
    }
    memcpy(app_authorized_url, authorized_url.url,
           (size_t)authorized_url.capacity);
    *write_fd = descriptor;
    return 0;
}

int ib_shizuku_ingress_finish_remote(AIBinder *binder, const char *id,
                                     uint64_t generation, int exit_status) {
    if (exit_status < 0 || exit_status > 255) return -1;
    AParcel *request = NULL, *reply = NULL;
    if (prepare(binder, id, generation, &request) != 0) return -1;
    binder_status_t status = AParcel_writeInt32(request, exit_status);
    if (status != STATUS_OK) {
        AParcel_delete(request);
        return -1;
    }
    status = AIBinder_transact(
        binder, IB_SHIZUKU_INGRESS_FINISH, &request, &reply, 0);
    if (status != STATUS_OK || !reply) {
        if (reply) AParcel_delete(reply);
        return -1;
    }
    int32_t result = -1;
    status = AParcel_readInt32(reply, &result);
    AParcel_delete(reply);
    return status == STATUS_OK && result == 0 ? 0 : -1;
}
#endif
