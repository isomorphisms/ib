#ifndef IB_SHIZUKU_CLIENT_H
#define IB_SHIZUKU_CLIENT_H

#include <jni.h>
#include <stdint.h>
#include <sys/types.h>

/* OS capability state only. Browser navigation/source/history remain Idriç
 * values. None of these states grants a document admission or network result. */
typedef enum {
    IB_SHIZUKU_UNAVAILABLE = 0,
    IB_SHIZUKU_ATTACHING = 1,
    IB_SHIZUKU_PERMISSION_REQUIRED = 2,
    IB_SHIZUKU_AUTHORIZED = 3,
    IB_SHIZUKU_PLATFORM_ERROR = 4
} ib_shizuku_capability_state;

typedef void (*ib_shizuku_revocation_handler)(void *owner, uint64_t revoked_through_epoch);

/* Unregister waits for an already running handler. A handler must not call
 * this registration function; revoke only ingress instances minted no later
 * than revoked_through_epoch, promptly. A late old event cannot revoke a new
 * legitimate connection's instances. */
void ib_shizuku_client_set_revocation_handler(ib_shizuku_revocation_handler handler,
                                             void *owner);

/* Every use rechecks actual Shizuku permission after a validated application
 * attachment. A cached permission callback is never sufficient authority. */
ib_shizuku_capability_state ib_shizuku_client_authorization(JNIEnv *environment,
                                                            uid_t *server_uid,
                                                            uint64_t *capability_epoch);
int ib_shizuku_client_request_permission(JNIEnv *environment);

/* App-owned native acquisition adapter only; never return this broad service
 * handle to document code or distribute it as the private ingress capability.
 * The caller owns the returned local JNI reference. */
jobject ib_shizuku_client_authorized_server(JNIEnv *environment,
                                            uid_t *server_uid,
                                            uint64_t *capability_epoch);

#endif
