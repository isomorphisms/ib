#ifndef IB_SHIZUKU_INGRESS_RUNNER_H
#define IB_SHIZUKU_INGRESS_RUNNER_H

#include <stdint.h>

#if defined(__ANDROID__) && __ANDROID_API__ >= 29
#include <android/binder_ibinder.h>
#include <jni.h>

/* Run in the explicitly authorized broker process (for example, an
 * app-approved Termux/Shizuku broker), never in the display reader.
 * The app-private Binder object is passed to this process out-of-band.
 *
 * Runtime paths are immutable inputs verified against the signed app's
 * source-bound payload catalog by the broker, never supplied by a page.
 * The URL comes only from the app's BEGIN reply; no caller URL is used.
 * ICU's private capture files are framed to the WRITE PFD and removed by the
 * supervisor. No app Arena pathname or publication authority crosses here. */
typedef struct {
    const char *grease;
    const char *script;
    const char *icu_dex;
    const char *icu_library;
    const char *capture_directory;
} ib_icu_worker_runtime;

int ib_shizuku_ingress_run_icu(JNIEnv *environment, jobject private_ingress,
                               const char *request_id,
                               uint64_t navigation_generation,
                               const ib_icu_worker_runtime *runtime);
#endif
#endif
