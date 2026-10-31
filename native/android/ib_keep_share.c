#include "ib_keep_share.h"

#include <stdlib.h>

int ib_keep_utf8_to_utf16(const unsigned char *src, size_t count,
                          uint16_t *dst, size_t capacity, size_t *written)
{
    size_t i = 0, o = 0;
    if (!src || !dst || !written || !count || count > IB_KEEP_SHARE_MAX_BYTES)
        return 0;
    while (i < count) {
        uint32_t cp = 0;
        unsigned char a = src[i];
        unsigned length = 0;
        if (a < 0x80) { cp = a; length = 1; }
        else if (a >= 0xc2 && a <= 0xdf) { cp = a & 0x1fU; length = 2; }
        else if (a >= 0xe0 && a <= 0xef) { cp = a & 0x0fU; length = 3; }
        else if (a >= 0xf0 && a <= 0xf4) { cp = a & 0x07U; length = 4; }
        else return 0;
        if (count - i < length) return 0;
        for (unsigned j = 1; j < length; ++j) {
            unsigned char b = src[i + j];
            if ((b & 0xc0U) != 0x80U) return 0;
            cp = (cp << 6) | (b & 0x3fU);
        }
        if (!cp || (length == 2 && cp < 0x80U) ||
            (length == 3 && cp < 0x800U) ||
            (length == 4 && cp < 0x10000U) ||
            (cp >= 0xd800U && cp <= 0xdfffU) || cp > 0x10ffffU)
            return 0;
        if (cp <= 0xffffU) {
            if (o >= capacity) return 0;
            dst[o++] = (uint16_t)cp;
        } else {
            if (capacity - o < 2) return 0;
            cp -= 0x10000U;
            dst[o++] = (uint16_t)(0xd800U + (cp >> 10));
            dst[o++] = (uint16_t)(0xdc00U + (cp & 0x3ffU));
        }
        i += length;
    }
    *written = o;
    return 1;
}

#ifdef __ANDROID__
#include <jni.h>

/* Fixed framework names are ASCII. User text goes through NewString, never
 * NewStringUTF (which expects modified UTF-8 and changes NUL/supplementary). */
static jstring fixed_string(JNIEnv *env, const char *ascii)
{
    return (*env)->NewStringUTF(env, ascii);
}

static int pending(JNIEnv *env) { return (*env)->ExceptionCheck(env) == JNI_TRUE; }

static int intent_string_call(JNIEnv *env, jobject intent, jclass intent_class,
                              const char *method, jstring value)
{
    jmethodID id = (*env)->GetMethodID(env, intent_class, method,
                                      "(Ljava/lang/String;)Landroid/content/Intent;");
    if (!id || pending(env)) return 0;
    jobject returned = (*env)->CallObjectMethod(env, intent, id, value);
    if (pending(env)) return 0;
    if (returned) (*env)->DeleteLocalRef(env, returned);
    return 1;
}

enum ib_keep_share_result ib_keep_start_share(JNIEnv *env, jobject activity,
                                               const unsigned char *utf8,
                                               size_t count)
{
    enum ib_keep_share_result result = IB_KEEP_START_FAILED_OR_UNKNOWN;
    uint16_t *buffer = NULL;
    size_t units = 0;
    jclass intent_class = NULL, activity_class = NULL;
    jobject intent = NULL;
    jstring action = NULL, mime = NULL, target = NULL, key = NULL, payload = NULL;
    if (!env || !activity || !utf8 || !count || count > IB_KEEP_SHARE_MAX_BYTES)
        return IB_KEEP_BAD_TEXT;
    buffer = malloc((count + 1U) * sizeof(*buffer));
    if (!buffer) return IB_KEEP_START_FAILED_OR_UNKNOWN;
    if (!ib_keep_utf8_to_utf16(utf8, count, buffer, count + 1U, &units)) {
        result = IB_KEEP_BAD_TEXT;
        goto done;
    }
    intent_class = (*env)->FindClass(env, "android/content/Intent");
    if (!intent_class || pending(env)) goto done;
    jmethodID ctor = (*env)->GetMethodID(env, intent_class, "<init>",
                                         "(Ljava/lang/String;)V");
    if (!ctor || pending(env)) goto done;
    action = fixed_string(env, "android.intent.action.SEND");
    mime = fixed_string(env, "text/plain");
    target = fixed_string(env, "com.google.android.keep");
    key = fixed_string(env, "android.intent.extra.TEXT");
    payload = (*env)->NewString(env, (const jchar *)buffer, (jsize)units);
    if (pending(env) || !action || !mime || !target || !key || !payload) goto done;
    intent = (*env)->NewObject(env, intent_class, ctor, action);
    if (pending(env) || !intent) goto done;
    if (!intent_string_call(env, intent, intent_class, "setType", mime) ||
        !intent_string_call(env, intent, intent_class, "setPackage", target)) goto done;
    jmethodID put = (*env)->GetMethodID(env, intent_class, "putExtra",
                 "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;");
    if (!put || pending(env)) goto done;
    jobject returned = (*env)->CallObjectMethod(env, intent, put, key, payload);
    if (pending(env)) goto done;
    if (returned) (*env)->DeleteLocalRef(env, returned);
    activity_class = (*env)->GetObjectClass(env, activity);
    if (!activity_class || pending(env)) goto done;
    jmethodID start = (*env)->GetMethodID(env, activity_class, "startActivity",
                                          "(Landroid/content/Intent;)V");
    if (!start || pending(env)) goto done;
    (*env)->CallVoidMethod(env, activity, start, intent);
    if (!pending(env)) result = IB_KEEP_INTENT_STARTED_UNCONFIRMED;

done:
    if (pending(env)) {
        jthrowable error = (*env)->ExceptionOccurred(env);
        (*env)->ExceptionClear(env);
        jclass no_handler = (*env)->FindClass(env, "android/content/ActivityNotFoundException");
        if (no_handler && !pending(env) && error &&
            (*env)->IsInstanceOf(env, error, no_handler))
            result = IB_KEEP_NO_HANDLER;
        if (pending(env)) (*env)->ExceptionClear(env);
        if (no_handler) (*env)->DeleteLocalRef(env, no_handler);
        if (error) (*env)->DeleteLocalRef(env, error);
    }
    if (payload) (*env)->DeleteLocalRef(env, payload);
    if (key) (*env)->DeleteLocalRef(env, key);
    if (target) (*env)->DeleteLocalRef(env, target);
    if (mime) (*env)->DeleteLocalRef(env, mime);
    if (action) (*env)->DeleteLocalRef(env, action);
    if (intent) (*env)->DeleteLocalRef(env, intent);
    if (activity_class) (*env)->DeleteLocalRef(env, activity_class);
    if (intent_class) (*env)->DeleteLocalRef(env, intent_class);
    free(buffer);
    return result;
}
#endif
