#ifndef IB_SHIZUKU_WORKER_H
#define IB_SHIZUKU_WORKER_H

#include <jni.h>
#include <stdint.h>

/* Call from the app's acquisition thread. No UI-thread wait and no broad
 * Shizuku capability reaches checked document code. The private ingress is
 * already armed for this request, navigation generation and current epoch. */
int ib_shizuku_worker_acquire(JNIEnv *environment, jobject context,
                              jobject private_ingress, const char *request_id,
                              uint64_t navigation_generation,
                              uint64_t authorized_epoch);

#endif
