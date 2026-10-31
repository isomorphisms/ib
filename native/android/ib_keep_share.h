#ifndef IB_KEEP_SHARE_H
#define IB_KEEP_SHARE_H

#include <stddef.h>
#include <stdint.h>

/* Status from starting an editor, not evidence of a saved Google Keep note. */
enum ib_keep_share_result {
    IB_KEEP_INTENT_STARTED_UNCONFIRMED = 0,
    IB_KEEP_BAD_TEXT = 1,
    IB_KEEP_NO_HANDLER = 2,
    IB_KEEP_START_FAILED_OR_UNKNOWN = 3
};

#define IB_KEEP_SHARE_MAX_BYTES 32768U

/* Strict UTF-8 (not JNI modified UTF-8). On failure, output is not usable. */
int ib_keep_utf8_to_utf16(const unsigned char *src, size_t count,
                          uint16_t *dst, size_t capacity, size_t *written);

#ifdef __ANDROID__
#include <jni.h>
/* Invoke on a thread attached to Activity's JVM. Neither callback completion
 * nor successful startActivity() may be recorded as a saved note. */
enum ib_keep_share_result ib_keep_start_share(JNIEnv *env, jobject activity,
                                               const unsigned char *utf8,
                                               size_t count);
#endif
#endif
