#ifndef IB_SHIZUKU_INGRESS_RUNNER_H
#define IB_SHIZUKU_INGRESS_RUNNER_H

#include <stdint.h>

#if defined(__ANDROID__) && __ANDROID_API__ >= 29
#include <android/binder_ibinder.h>

/* Run in the explicitly authorized broker process (for example, an
 * app-approved Termux/Shizuku broker), never in the display reader.
 * The app-private Binder object is passed to this process out-of-band.
 *
 * qualified_grease and trusted_worker_script are immutable, Cat Food
 * qualified absolute paths supplied by the broker/host, never by a page.
 * The URL comes only from the app's BEGIN reply; no caller URL is used.
 * No stdout body is captured as a String or on disk. */
int ib_shizuku_ingress_run_icu(AIBinder *private_ingress,
                               const char *request_id,
                               uint64_t navigation_generation,
                               const char *qualified_grease,
                               const char *trusted_worker_script);
#endif
#endif
