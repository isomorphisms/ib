#ifndef IB_SHIZUKU_INGRESS_BINDER_H
#define IB_SHIZUKU_INGRESS_BINDER_H

#include "ib_shizuku_arena_ingress.h"

#if defined(__ANDROID__) && __ANDROID_API__ >= 29
#include <android/binder_ibinder.h>
#include <jni.h>

typedef struct ib_ingress_capability ib_ingress_capability;

/* The source owner outlives all capabilities (the app's process-owned Arena
 * owner). The caller owns one reference. The private SDK Binder retains the
 * session until the final remote reference and in-flight call have gone. */
ib_ingress_capability *ib_ingress_capability_new(ib_ingress_publish publish,
                                                void *source_owner);
ib_shizuku_arena_ingress *ib_ingress_capability_session(ib_ingress_capability *capability);
void ib_ingress_capability_retain(ib_ingress_capability *capability);
void ib_ingress_capability_release(ib_ingress_capability *capability);

/* Private app-created Binder capability; never register it with Android's
 * global service manager. App host supplies it only to its explicitly
 * authorized Shizuku/rish bridge. The caller never gets an Arena pathname. */
/* Caller owns the returned local JNI reference. SDK Parcel validation is used
 * on API 29 too; no dependency on API 30/31 Parcel bridge/size additions. The
 * opaque native anchor in the Binder is never distributed separately. */
jobject ib_shizuku_ingress_binder_new(JNIEnv *environment,
                                     ib_ingress_capability *capability);

/* Transaction 2: BEGIN(request_id, generation) -> (status, URL, WRITE PFD).
 * Transaction 3: FINISH(request_id, generation, ICU process exit status)
 *                -> (status).
 * Status zero is permitted only after the app independently committed and
 * read-back verified the canonical Arena source receipt. */
enum {
    IB_SHIZUKU_INGRESS_BEGIN = 2,
    IB_SHIZUKU_INGRESS_FINISH = 3
};

/* The bridge uses the privately passed Binder handle. It must close the
 * returned FD before reporting ICU completion. Only app-owned code arms
 * the request identity and approved caller UID. */
int ib_shizuku_ingress_begin_remote(JNIEnv *environment, jobject binder,
                                    const char *request_id,
                                    uint64_t navigation_generation,
                                    char app_authorized_url[IB_INGRESS_URL_CAP + 1U],
                                    int *write_fd);
int ib_shizuku_ingress_finish_remote(JNIEnv *environment, jobject binder,
                                     const char *request_id,
                                     uint64_t navigation_generation,
                                     int icu_exit_status);
#endif
#endif
