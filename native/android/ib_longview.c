/* Narrow Android attachment for IB.LongViewTask. All task transitions and
 * record decoding execute the checked Idriç exports in the real Activity. */
#define _GNU_SOURCE
#include "ib_longview.h"
#include "ib_longview_store.h"

#include <android/log.h>
#include <errno.h>
#include <inttypes.h>
#include <jni.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>
#include <time.h>
#include <unistd.h>

#define LV_PANEL 2130903057
#define LV_STATUS 2130903058
#define LV_WEBVIEW 2130903059
#define LV_READER_SCROLL 2130903054
#define LV_GOOGLE_PANEL 2130903052
#define LV_SOURCES_PANEL 2130903042
#define LV_READER_STATUS 2130903045
#define LV_REFLOW_BUTTON 2130903047
#define LV_BACK_BUTTON 2130903048
#define LV_FORWARD_BUTTON 2130903049

/* Framework entrypoints and WebView callbacks are confined to the main
 * Looper. No worker thread may manipulate these attachment references. */
static struct {
    int ready, broken, visible, opened, recovering;
    unsigned replacements;
    jint document_epoch;
    int64_t requested_at, document_at;
    char host[49];
    ib_longview_store store;
    jweak activity;
    jobject browser;
    jstring record;
    jclass generated, client, timing, recovery;
    jmethodID start, open, attach, observe, field, counter;
} protected_view ← { .store ← { -1, -1, -1 } };

static jmethodID lv_method(JNIEnv *env, const char *owner, const char *name, const char *signature) {
    if ((*env)->ExceptionCheck(env)) return NULL;
    jclass type ← (*env)->FindClass(env, owner);
    return type ? (*env)->GetMethodID(env, type, name, signature) : NULL;
}

static int lv_main_thread(JNIEnv *env) {
    if ((*env)->ExceptionCheck(env)) return 0;
    jclass type ← (*env)->FindClass(env, "android/os/Looper");
    jmethodID current ← type ? (*env)->GetStaticMethodID(env, type, "myLooper", "()Landroid/os/Looper;") : NULL;
    jmethodID main ← current ? (*env)->GetStaticMethodID(env, type, "getMainLooper", "()Landroid/os/Looper;") : NULL;
    jobject observed ← main ? (*env)->CallStaticObjectMethod(env, type, current) : NULL;
    jobject expected ← observed ? (*env)->CallStaticObjectMethod(env, type, main) : NULL;
    return expected && !(*env)->ExceptionCheck(env) && (*env)->IsSameObject(env, observed, expected);
}

int ib_longview_surface_visible(JNIEnv *env, jobject activity) {
    return activity && lv_main_thread(env) && protected_view.visible && protected_view.activity &&
        (*env)->IsSameObject(env, protected_view.activity, activity);
}

static int64_t lv_clock(void) {
    struct timespec value;
    return clock_gettime(CLOCK_MONOTONIC, &value) == 0 ?
        (int64_t)value.tv_sec * 1000 + value.tv_nsec / 1000000 : -1;
}

static int64_t lv_elapsed(int64_t started) {
    int64_t now ← lv_clock();
    return started >= 0 && now >= started ? now - started : -1;
}

static int lv_ascii(JNIEnv *env, jstring text, char *out, size_t capacity) {
    if (!text || (*env)->ExceptionCheck(env)) return 0;
    jsize length ← (*env)->GetStringLength(env, text);
    if (length < 1 || (size_t)length >= capacity) return 0;
    const jchar *units ← (*env)->GetStringChars(env, text, NULL);
    if (!units) return 0;
    int ok ← 1;
    for (jsize i ← 0; i < length; ++i) {
        if (units[i] < 33 || units[i] > 126) { ok ← 0; break; }
        out[i] ← (char)units[i];
    }
    (*env)->ReleaseStringChars(env, text, units);
    if (ok) out[length] ← 0;
    return ok;
}

static int lv_random(char out[49]) {
    unsigned char bytes[24];
    size_t count ← 0;
    while (count < sizeof(bytes)) {
        ssize_t used ← getrandom(bytes + count, sizeof(bytes) - count, 0);
        if (used < 0 && errno == EINTR) continue;
        if (used <= 0) return -1;
        count += (size_t)used;
    }
    static const char hex[] ← "0123456789abcdef";
    for (size_t i ← 0; i < sizeof(bytes); ++i) {
        out[2U*i] ← hex[bytes[i] >> 4];
        out[2U*i+1U] ← hex[bytes[i] & 15U];
    }
    out[48] ← 0;
    return 0;
}

static jobject lv_activity(JNIEnv *env) {
    return protected_view.activity ? (*env)->NewLocalRef(env, protected_view.activity) : NULL;
}

static jobject lv_view(JNIEnv *env, jobject activity, jint id) {
    jmethodID find ← lv_method(env, "android/app/Activity", "findViewById", "(I)Landroid/view/View;");
    return activity && find ? (*env)->CallObjectMethod(env, activity, find, id) : NULL;
}

static void lv_visibility(JNIEnv *env, jobject view, jint value) {
    jmethodID set ← lv_method(env, "android/view/View", "setVisibility", "(I)V");
    if (view && set) (*env)->CallVoidMethod(env, view, set, value);
}

static void lv_status(JNIEnv *env, const char *message) {
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    jobject activity ← lv_activity(env);
    jobject view ← lv_view(env, activity, LV_STATUS);
    jmethodID set ← lv_method(env, "android/widget/TextView", "setText", "(Ljava/lang/CharSequence;)V");
    jstring text ← view && set ? (*env)->NewStringUTF(env, message) : NULL;
    if (text) (*env)->CallVoidMethod(env, view, set, text);
    lv_visibility(env, view, 0);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
}

static jstring lv_field(JNIEnv *env, jstring record, jint field) {
    return record && protected_view.field && !(*env)->ExceptionCheck(env) ?
        (*env)->CallStaticObjectMethod(env, protected_view.generated, protected_view.field, record, field) : NULL;
}

static jint lv_counter(JNIEnv *env, jstring record, jint field) {
    return record && protected_view.counter && !(*env)->ExceptionCheck(env) ?
        (*env)->CallStaticIntMethod(env, protected_view.generated, protected_view.counter, record, field) : -1;
}

static int lv_nonempty(JNIEnv *env, jstring value) {
    return value && !(*env)->ExceptionCheck(env) && (*env)->GetStringLength(env, value) > 0;
}

static void lv_event(JNIEnv *env, int event, int code, int64_t elapsed) {
    jint host ← lv_counter(env, protected_view.record, 0);
    jint activity ← lv_counter(env, protected_view.record, 1);
    jint renderer ← lv_counter(env, protected_view.record, 2);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    __android_log_print(ANDROID_LOG_INFO, "IBLongView",
        "event=%d host_pid=%ld host_generation=%d activity_generation=%d renderer_generation=%d document_epoch=%d code=%d elapsed_ms=%" PRId64,
        event, (long)getpid(), host, activity, renderer, protected_view.document_epoch, code, elapsed);
}

static int lv_utf8(JNIEnv *env, jstring text, unsigned char out[IB_LONGVIEW_RECORD_CAP + 1U], size_t *length) {
    jmethodID encode ← lv_method(env, "java/lang/String", "getBytes", "(Ljava/lang/String;)[B");
    jstring encoding ← encode ? (*env)->NewStringUTF(env, "UTF-8") : NULL;
    jbyteArray bytes ← text && encoding ? (*env)->CallObjectMethod(env, text, encode, encoding) : NULL;
    if (!bytes || (*env)->ExceptionCheck(env)) return -1;
    jsize size ← (*env)->GetArrayLength(env, bytes);
    if (size < 1 || size > (jint)IB_LONGVIEW_RECORD_CAP) return -1;
    (*env)->GetByteArrayRegion(env, bytes, 0, size, (jbyte *)out);
    if ((*env)->ExceptionCheck(env) || memchr(out, 0, (size_t)size)) return -1;
    out[size] ← 0;
    *length ← (size_t)size;
    return 0;
}

static jstring lv_text(JNIEnv *env, const unsigned char *bytes, size_t length) {
    if (!length || length > IB_LONGVIEW_RECORD_CAP) return NULL;
    jclass type ← (*env)->FindClass(env, "java/lang/String");
    jmethodID constructor ← type ? (*env)->GetMethodID(env, type, "<init>", "([BLjava/lang/String;)V") : NULL;
    jbyteArray input ← constructor ? (*env)->NewByteArray(env, (jsize)length) : NULL;
    if (input) (*env)->SetByteArrayRegion(env, input, 0, (jsize)length, (const jbyte *)bytes);
    jstring encoding ← input ? (*env)->NewStringUTF(env, "UTF-8") : NULL;
    jstring text ← encoding ? (*env)->NewObject(env, type, constructor, input, encoding) : NULL;
    /* Java's decoder replaces malformed input. Refuse that lossy conversion. */
    unsigned char encoded[IB_LONGVIEW_RECORD_CAP + 1U];
    size_t encoded_length ← 0;
    if (!text || lv_utf8(env, text, encoded, &encoded_length) != 0 ||
        encoded_length != length || memcmp(encoded, bytes, length)) return NULL;
    return text;
}

static int lv_save(JNIEnv *env, jstring next, int initial) {
    if (!lv_nonempty(env, next) || lv_counter(env, next, 0) < 1) return -1;
    char task[IB_LONGVIEW_ID_CAP + 1U], tab[IB_LONGVIEW_ID_CAP + 1U], navigation[IB_LONGVIEW_ID_CAP + 1U];
    unsigned char bytes[IB_LONGVIEW_RECORD_CAP + 1U];
    size_t count ← 0;
    int ok ← lv_ascii(env, lv_field(env, next, 0), task, sizeof(task)) &&
        lv_ascii(env, lv_field(env, next, 1), tab, sizeof(tab)) &&
        lv_ascii(env, lv_field(env, next, 2), navigation, sizeof(navigation)) &&
        lv_utf8(env, next, bytes, &count) == 0;
    jstring retained ← ok ? (*env)->NewGlobalRef(env, next) : NULL;
    int64_t started ← lv_clock();
    if (retained) ok ← ib_longview_store_commit(&protected_view.store, task, tab,
        navigation, bytes, count, initial) == 0;
    else ok ← 0;
    if (ok) {
        if (protected_view.record) (*env)->DeleteGlobalRef(env, protected_view.record);
        protected_view.record ← retained;
    } else {
        if (retained) (*env)->DeleteGlobalRef(env, retained);
        protected_view.broken ← 1;
    }
    lv_event(env, 4, ok ? 0 : 1, lv_elapsed(started));
    return ok ? 0 : -1;
}

static void lv_destroy_browser(JNIEnv *env) {
    jobject browser ← protected_view.browser;
    protected_view.browser ← NULL;
    protected_view.recovering ← 0;
    protected_view.document_epoch ← 0;
    protected_view.document_at ← -1;
    if (!browser) return;
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    jmethodID parent_method ← lv_method(env, "android/view/View", "getParent", "()Landroid/view/ViewParent;");
    jobject parent ← parent_method ? (*env)->CallObjectMethod(env, browser, parent_method) : NULL;
    jmethodID remove ← lv_method(env, "android/view/ViewGroup", "removeView", "(Landroid/view/View;)V");
    if (parent && remove) (*env)->CallVoidMethod(env, parent, remove, browser);
    jmethodID stop ← lv_method(env, "android/webkit/WebView", "stopLoading", "()V");
    jmethodID destroy ← lv_method(env, "android/webkit/WebView", "destroy", "()V");
    if (stop) (*env)->CallVoidMethod(env, browser, stop);
    if (destroy) (*env)->CallVoidMethod(env, browser, destroy);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->DeleteGlobalRef(env, browser);
}

static void lv_failure(JNIEnv *env) {
    protected_view.broken ← 1;
    lv_destroy_browser(env);
    lv_status(env, "Protected task state is unavailable or incomplete. Existing records were retained; no remote action was repeated.");
    lv_event(env, 12, 1, -1);
}

static int lv_setup(JNIEnv *env, jobject activity) {
    if (!activity || !lv_main_thread(env)) return -1;
    if (protected_view.ready) return protected_view.broken ? -1 : 0;
    if (protected_view.broken) return -1;
    jclass generated ← (*env)->FindClass(env, "Idric/Generated");
    protected_view.generated ← generated ? (*env)->NewGlobalRef(env, generated) : NULL;
    protected_view.start ← generated ? (*env)->GetStaticMethodID(env, generated, "longview_start",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;") : NULL;
    protected_view.open ← protected_view.start ? (*env)->GetStaticMethodID(env, generated, "longview_open",
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;") : NULL;
    protected_view.attach ← protected_view.open ? (*env)->GetStaticMethodID(env, generated, "longview_attach",
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;") : NULL;
    protected_view.observe ← protected_view.attach ? (*env)->GetStaticMethodID(env, generated, "longview_observe",
        "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;III)Ljava/lang/String;") : NULL;
    protected_view.field ← protected_view.observe ? (*env)->GetStaticMethodID(env, generated, "longview_field",
        "(Ljava/lang/String;I)Ljava/lang/String;") : NULL;
    protected_view.counter ← protected_view.field ? (*env)->GetStaticMethodID(env, generated, "longview_counter",
        "(Ljava/lang/String;I)I") : NULL;
    jclass client ← protected_view.counter ? (*env)->FindClass(env, "org/isomorphisms/ib/LongViewClient") : NULL;
    jclass timing ← client ? (*env)->FindClass(env, "org/isomorphisms/ib/LongViewTimingCallback") : NULL;
    jclass recovery ← timing ? (*env)->FindClass(env, "org/isomorphisms/ib/LongViewRecoveryTask") : NULL;
    protected_view.client ← client ? (*env)->NewGlobalRef(env, client) : NULL;
    protected_view.timing ← timing ? (*env)->NewGlobalRef(env, timing) : NULL;
    protected_view.recovery ← recovery ? (*env)->NewGlobalRef(env, recovery) : NULL;
    jmethodID files_method ← lv_method(env, "android/content/Context", "getFilesDir", "()Ljava/io/File;");
    jobject files ← files_method ? (*env)->CallObjectMethod(env, activity, files_method) : NULL;
    jmethodID canonical ← lv_method(env, "java/io/File", "getCanonicalPath", "()Ljava/lang/String;");
    jstring path ← files && canonical ? (*env)->CallObjectMethod(env, files, canonical) : NULL;
    char directory[4096];
    int ok ← protected_view.generated && protected_view.client && protected_view.timing && protected_view.recovery &&
        lv_ascii(env, path, directory, sizeof(directory)) && lv_random(protected_view.host) == 0 &&
        ib_longview_store_open(&protected_view.store, directory) == 0;
    if (ok) {
        char task[IB_LONGVIEW_ID_CAP + 1U], tab[IB_LONGVIEW_ID_CAP + 1U], navigation[IB_LONGVIEW_ID_CAP + 1U];
        unsigned char record[IB_LONGVIEW_RECORD_CAP + 1U];
        size_t length ← 0;
        int found ← ib_longview_store_read(&protected_view.store, task, tab, navigation, record, &length);
        if (found == 0) {
            jstring decoded ← lv_text(env, record, length);
            char actual[IB_LONGVIEW_ID_CAP + 1U];
            ok ← decoded && lv_counter(env, decoded, 0) > 0 &&
                lv_ascii(env, lv_field(env, decoded, 0), actual, sizeof(actual)) && !strcmp(actual, task) &&
                lv_ascii(env, lv_field(env, decoded, 1), actual, sizeof(actual)) && !strcmp(actual, tab) &&
                lv_ascii(env, lv_field(env, decoded, 2), actual, sizeof(actual)) && !strcmp(actual, navigation);
            if (ok) protected_view.record ← (*env)->NewGlobalRef(env, decoded);
            ok ← ok && protected_view.record;
        } else ok ← found == 1;
    }
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); ok ← 0; }
    protected_view.ready ← ok;
    protected_view.broken ← !ok;
    return ok ? 0 : -1;
}

static int lv_bind(JNIEnv *env, jobject activity) {
    if (protected_view.activity && (*env)->IsSameObject(env, protected_view.activity, activity)) return 0;
    lv_destroy_browser(env);
    if (protected_view.activity) (*env)->DeleteWeakGlobalRef(env, protected_view.activity);
    protected_view.activity ← (*env)->NewWeakGlobalRef(env, activity);
    return protected_view.activity ? 1 : -1;
}

static int lv_set_document_epoch(JNIEnv *env, jobject observer, jint value) {
    jclass type ← observer ? (*env)->GetObjectClass(env, observer) : NULL;
    jfieldID field ← type ? (*env)->GetFieldID(env, type, "documentEpoch", "Ljava/lang/Integer;") : NULL;
    jclass integer ← field ? (*env)->FindClass(env, "java/lang/Integer") : NULL;
    jmethodID boxed ← integer ? (*env)->GetStaticMethodID(env, integer, "valueOf", "(I)Ljava/lang/Integer;") : NULL;
    jobject number ← boxed ? (*env)->CallStaticObjectMethod(env, integer, boxed, value) : NULL;
    if (number && !(*env)->ExceptionCheck(env)) (*env)->SetObjectField(env, observer, field, number);
    return number && !(*env)->ExceptionCheck(env);
}

static jobject lv_observer(JNIEnv *env, jclass type) {
    jmethodID constructor ← (*env)->GetMethodID(env, type, "<init>", "()V");
    jobject result ← constructor ? (*env)->NewObject(env, type, constructor) : NULL;
    jfieldID snapshot ← result ? (*env)->GetFieldID(env, type, "observationRecord", "Ljava/lang/String;") : NULL;
    if (snapshot) (*env)->SetObjectField(env, result, snapshot, protected_view.record);
    return snapshot && lv_set_document_epoch(env, result, protected_view.document_epoch) ? result : NULL;
}

static jint lv_document_epoch(JNIEnv *env, jobject observer) {
    jclass type ← observer ? (*env)->GetObjectClass(env, observer) : NULL;
    jfieldID field ← type ? (*env)->GetFieldID(env, type, "documentEpoch", "Ljava/lang/Integer;") : NULL;
    jobject number ← field ? (*env)->GetObjectField(env, observer, field) : NULL;
    jmethodID value ← number ? lv_method(env, "java/lang/Integer", "intValue", "()I") : NULL;
    return value ? (*env)->CallIntMethod(env, number, value) : -1;
}

static jstring lv_snapshot(JNIEnv *env, jobject observer) {
    jclass type ← observer ? (*env)->GetObjectClass(env, observer) : NULL;
    jfieldID field ← type ? (*env)->GetFieldID(env, type, "observationRecord", "Ljava/lang/String;") : NULL;
    return field ? (*env)->GetObjectField(env, observer, field) : NULL;
}

static jstring lv_observed(JNIEnv *env, jstring snapshot, jint kind) {
    if (!lv_nonempty(env, snapshot) || !protected_view.record) return NULL;
    jstring task ← lv_field(env, snapshot, 0), tab ← lv_field(env, snapshot, 1);
    jstring navigation ← lv_field(env, snapshot, 2), host ← lv_field(env, snapshot, 4);
    jstring renderer ← lv_field(env, snapshot, 5);
    jint host_generation ← lv_counter(env, snapshot, 0), activity_generation ← lv_counter(env, snapshot, 1);
    jint renderer_generation ← lv_counter(env, snapshot, 2);
    return task && tab && navigation && host && renderer && !(*env)->ExceptionCheck(env) ?
        (*env)->CallStaticObjectMethod(env, protected_view.generated, protected_view.observe,
            protected_view.record, kind, task, tab, navigation, host, renderer,
            host_generation, activity_generation, renderer_generation) : NULL;
}

static int lv_current(JNIEnv *env, jobject self, jobject web) {
    if (protected_view.broken || !protected_view.visible || !lv_main_thread(env) || !protected_view.browser ||
        (web && !(*env)->IsSameObject(env, web, protected_view.browser))) return 0;
    return lv_nonempty(env, lv_observed(env, lv_snapshot(env, self), 1));
}

static int lv_record_loss(JNIEnv *env) {
    if (!protected_view.browser || !protected_view.record) return 0;
    jstring next ← lv_observed(env, protected_view.record, 0);
    return lv_nonempty(env, next) ? lv_save(env, next, 0) : -1;
}

static void lv_surfaces(JNIEnv *env, jobject activity, int visible) {
    lv_visibility(env, lv_view(env, activity, LV_PANEL), visible ? 0 : 8);
    lv_visibility(env, lv_view(env, activity, LV_STATUS), visible ? 0 : 8);
    lv_visibility(env, lv_view(env, activity, LV_READER_SCROLL), visible ? 8 : 0);
    lv_visibility(env, lv_view(env, activity, LV_READER_STATUS), visible ? 8 : 0);
    lv_visibility(env, lv_view(env, activity, LV_REFLOW_BUTTON), visible ? 8 : 0);
    lv_visibility(env, lv_view(env, activity, LV_BACK_BUTTON), visible ? 8 : 0);
    lv_visibility(env, lv_view(env, activity, LV_FORWARD_BUTTON), visible ? 8 : 0);
    if (visible) {
        lv_visibility(env, lv_view(env, activity, LV_GOOGLE_PANEL), 8);
        lv_visibility(env, lv_view(env, activity, LV_SOURCES_PANEL), 8);
    }
}

static int lv_allowed(JNIEnv *env, jstring url) {
    if (!protected_view.record || !url) return 0;
    jstring task ← lv_field(env, protected_view.record, 0), tab ← lv_field(env, protected_view.record, 1);
    jstring navigation ← lv_field(env, protected_view.record, 2), host ← lv_field(env, protected_view.record, 4);
    jstring checked ← task && tab && navigation && host && !(*env)->ExceptionCheck(env) ?
        (*env)->CallStaticObjectMethod(env, protected_view.generated, protected_view.start,
            task, tab, navigation, host, url) : NULL;
    return lv_nonempty(env, checked);
}

static int lv_start_renderer(JNIEnv *env, jobject activity, jstring url) {
    if (protected_view.broken || protected_view.browser || !lv_allowed(env, url)) return -1;
    char renderer[49];
    if (lv_random(renderer) != 0) return -1;
    jstring renderer_id ← (*env)->NewStringUTF(env, renderer);
    jstring next ← renderer_id ? (*env)->CallStaticObjectMethod(env, protected_view.generated,
        protected_view.attach, protected_view.record, renderer_id) : NULL;
    if (!lv_nonempty(env, next)) return -1;
    int64_t before ← lv_clock();
    jclass web_type ← (*env)->FindClass(env, "android/webkit/WebView");
    jmethodID constructor ← web_type ? (*env)->GetMethodID(env, web_type, "<init>", "(Landroid/content/Context;)V") : NULL;
    jobject browser ← constructor ? (*env)->NewObject(env, web_type, constructor, activity) : NULL;
    if (!browser || (*env)->ExceptionCheck(env)) return -1;
    protected_view.document_epoch ← 0;
    protected_view.document_at ← -1;
    protected_view.browser ← (*env)->NewGlobalRef(env, browser);
    if (!protected_view.browser || lv_save(env, next, 0) != 0) {
        lv_destroy_browser(env);
        return -1;
    }
    jobject client ← protected_view.browser ? lv_observer(env, protected_view.client) : NULL;
    jmethodID set_id ← lv_method(env, "android/view/View", "setId", "(I)V");
    if (set_id) (*env)->CallVoidMethod(env, browser, set_id, LV_WEBVIEW);
    jmethodID save ← lv_method(env, "android/view/View", "setSaveEnabled", "(Z)V");
    if (save) (*env)->CallVoidMethod(env, browser, save, JNI_FALSE);
    jmethodID settings_method ← lv_method(env, "android/webkit/WebView", "getSettings", "()Landroid/webkit/WebSettings;");
    jobject settings ← settings_method ? (*env)->CallObjectMethod(env, browser, settings_method) : NULL;
    static const char *const names[] ← {
        "setJavaScriptEnabled", "setDomStorageEnabled", "setAllowFileAccess", "setAllowContentAccess",
        "setSupportMultipleWindows", "setJavaScriptCanOpenWindowsAutomatically", "setSafeBrowsingEnabled"
    };
    for (unsigned i ← 0; settings && i < 7U && !(*env)->ExceptionCheck(env); ++i) {
        jmethodID method ← lv_method(env, "android/webkit/WebSettings", names[i], "(Z)V");
        if (method) (*env)->CallVoidMethod(env, settings, method, (jboolean)(i == 0U || i == 1U || i == 6U));
    }
    jmethodID mixed ← lv_method(env, "android/webkit/WebSettings", "setMixedContentMode", "(I)V");
    if (settings && mixed) (*env)->CallVoidMethod(env, settings, mixed, 1);
    jmethodID priority ← lv_method(env, "android/webkit/WebView", "setRendererPriorityPolicy", "(IZ)V");
    if (priority) (*env)->CallVoidMethod(env, browser, priority, 2, JNI_FALSE);
    jmethodID set_client ← lv_method(env, "android/webkit/WebView", "setWebViewClient", "(Landroid/webkit/WebViewClient;)V");
    if (client && set_client) (*env)->CallVoidMethod(env, browser, set_client, client);
    jobject panel ← lv_view(env, activity, LV_PANEL);
    jmethodID add ← lv_method(env, "android/view/ViewGroup", "addView", "(Landroid/view/View;II)V");
    if (panel && add) (*env)->CallVoidMethod(env, panel, add, browser, -1, -1);
    int ok ← protected_view.browser && client && settings && mixed && priority && set_client && panel && add &&
        !(*env)->ExceptionCheck(env);
    if (!ok) { (void)lv_record_loss(env); lv_destroy_browser(env); return -1; }
    protected_view.visible ← 1;
    lv_surfaces(env, activity, 1);
    lv_status(env, "Protected page loading. Session usability is unconfirmed; recovery retains only its origin.");
    lv_event(env, 2, 0, lv_elapsed(before));
    protected_view.requested_at ← lv_clock();
    jmethodID load ← lv_method(env, "android/webkit/WebView", "loadUrl", "(Ljava/lang/String;)V");
    if (load) (*env)->CallVoidMethod(env, browser, load, url);
    if (!load || (*env)->ExceptionCheck(env)) { (void)lv_record_loss(env); lv_destroy_browser(env); return -1; }
    lv_event(env, 1, 0, 0);
    return 0;
}

static int lv_open_host(JNIEnv *env) {
    jstring host ← (*env)->NewStringUTF(env, protected_view.host);
    jstring next ← host ? (*env)->CallStaticObjectMethod(env, protected_view.generated,
        protected_view.open, protected_view.record, host) : NULL;
    if (!lv_nonempty(env, next) || lv_save(env, next, 0) != 0) return -1;
    protected_view.opened ← 1;
    return 0;
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_LongViewBridge_open(JNIEnv *env, jclass type, jobject activity, jstring url) {
    (void)type;
    if ((*env)->PushLocalFrame(env, 96) != 0) return -1;
    int ok ← lv_setup(env, activity) == 0 && lv_bind(env, activity) >= 0;
    char task[49], tab[49], navigation[49];
    jstring next ← NULL;
    if (ok) ok ← lv_random(task) == 0 && lv_random(tab) == 0 && lv_random(navigation) == 0;
    if (ok) {
        jstring task_id ← (*env)->NewStringUTF(env, task), tab_id ← (*env)->NewStringUTF(env, tab);
        jstring navigation_id ← (*env)->NewStringUTF(env, navigation), host_id ← (*env)->NewStringUTF(env, protected_view.host);
        next ← task_id && tab_id && navigation_id && host_id ? (*env)->CallStaticObjectMethod(env,
            protected_view.generated, protected_view.start, task_id, tab_id, navigation_id, host_id, url) : NULL;
        ok ← lv_nonempty(env, next);
    }
    if (ok) {
        ok ← lv_record_loss(env) == 0;
        lv_destroy_browser(env);
        if (ok) ok ← lv_save(env, next, 1) == 0;
        protected_view.opened ← ok;
        protected_view.replacements ← 0;
        if (ok) ok ← lv_start_renderer(env, activity, url) == 0;
    }
    if (!ok) {
        if (protected_view.broken) lv_failure(env);
        else lv_status(env, "Protected navigation refused. Enter an ordinary HTTP(S) address without user information or control characters.");
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
    return ok ? 0 : -1;
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_LongViewBridge_reopen(JNIEnv *env, jclass type, jobject activity) {
    (void)type;
    if ((*env)->PushLocalFrame(env, 96) != 0) return -1;
    int ok ← lv_setup(env, activity) == 0;
    int changed ← ok ? lv_bind(env, activity) : -1;
    ok ← ok && changed >= 0 && protected_view.record;
    if (ok && (!protected_view.opened || changed > 0)) ok ← lv_open_host(env) == 0;
    if (ok && !protected_view.browser) ok ← lv_start_renderer(env, activity, lv_field(env, protected_view.record, 3)) == 0;
    if (ok) {
        protected_view.visible ← 1;
        lv_surfaces(env, activity, 1);
        lv_event(env, 9, 0, -1);
    } else if (protected_view.broken) lv_failure(env);
    else lv_status(env, "No complete protected task is available. Open a protected page to begin one.");
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
    return ok ? 0 : -1;
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_LongViewBridge_resume(JNIEnv *env, jclass type, jobject activity) {
    (void)type;
    if ((*env)->PushLocalFrame(env, 96) != 0) return -1;
    int ok ← lv_setup(env, activity) == 0;
    int was_visible ← protected_view.visible;
    int changed ← ok ? lv_bind(env, activity) : -1;
    ok ← ok && changed >= 0;
    /* Hidden protected work still observes a genuine Activity replacement;
     * view visibility controls attachment, not browser-owned generations. */
    if (ok && changed > 0 && protected_view.record && protected_view.opened)
        ok ← lv_open_host(env) == 0;
    if (ok && was_visible && changed > 0 && protected_view.record) {
        if (!protected_view.opened) ok ← lv_open_host(env) == 0;
        if (ok) ok ← lv_start_renderer(env, activity, lv_field(env, protected_view.record, 3)) == 0;
    } else if (ok && was_visible && protected_view.browser) {
        lv_surfaces(env, activity, 1);
        lv_event(env, 9, 0, -1);
    }
    if (!ok) lv_failure(env);
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
    return ok ? (protected_view.visible && protected_view.browser ? 1 : 0) : -1;
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_LongViewBridge_hide(JNIEnv *env, jclass type, jobject activity) {
    (void)type;
    if ((*env)->PushLocalFrame(env, 64) != 0) return;
    if (lv_main_thread(env) && protected_view.activity && (*env)->IsSameObject(env, protected_view.activity, activity)) {
        int ok ← lv_record_loss(env) == 0;
        lv_destroy_browser(env);
        protected_view.visible ← 0;
        lv_surfaces(env, activity, 0);
        if (!ok) lv_failure(env);
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_LongViewBridge_detach(JNIEnv *env, jclass type, jobject activity) {
    (void)type;
    if ((*env)->PushLocalFrame(env, 64) != 0) return;
    if (lv_main_thread(env) && protected_view.activity && (*env)->IsSameObject(env, protected_view.activity, activity)) {
        int ok ← lv_record_loss(env) == 0;
        lv_destroy_browser(env);
        (*env)->DeleteWeakGlobalRef(env, protected_view.activity);
        protected_view.activity ← NULL;
        if (!ok) protected_view.broken ← 1;
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
}

static void lv_measure(JNIEnv *env) {
    jobject callback ← lv_observer(env, protected_view.timing);
    jstring script ← callback ? (*env)->NewStringUTF(env,
        "(()=>{const n=performance.getEntriesByType('navigation')[0];if(!n)return[-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1];"
        "const q=v=>Number.isFinite(v)&&v>=0&&v<=1000000000000?Math.floor(v):-1;"
        "const p=name=>{const e=performance.getEntriesByName(name,'paint')[0];return e?e.startTime:-1};"
        "const r=performance.getEntriesByType('resource');const b=r.reduce((sum,e)=>sum+e.transferSize,0);"
        "return[n.domainLookupEnd-n.domainLookupStart,n.connectEnd-n.connectStart,"
        "n.secureConnectionStart?n.connectEnd-n.secureConnectionStart:-1,n.responseStart-n.requestStart,"
        "n.responseEnd-n.responseStart,n.domContentLoadedEventEnd||-1,n.loadEventEnd||-1,n.transferSize,"
        "p('first-paint'),p('first-contentful-paint'),r.length,b].map(q)})()") : NULL;
    jmethodID evaluate ← lv_method(env, "android/webkit/WebView", "evaluateJavascript", "(Ljava/lang/String;Landroid/webkit/ValueCallback;)V");
    if (script && evaluate && protected_view.browser) (*env)->CallVoidMethod(env, protected_view.browser, evaluate, script, callback);
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_LongViewClient_onPageStarted(JNIEnv *env, jobject self, jobject web, jstring url, jobject icon) {
    (void)icon;
    if ((*env)->PushLocalFrame(env, 96) != 0) return;
    if (lv_current(env, self, web)) {
        if (!lv_allowed(env, url)) {
            jmethodID stop ← lv_method(env, "android/webkit/WebView", "stopLoading", "()V");
            if (stop) (*env)->CallVoidMethod(env, web, stop);
            lv_status(env, "This navigation requires a supported external authorization step or is not an ordinary web address.");
            lv_event(env, 7, 1, -1);
        } else if (protected_view.document_epoch < INT32_MAX) {
            ++protected_view.document_epoch;
            protected_view.document_at ← lv_clock();
            if (lv_set_document_epoch(env, self, protected_view.document_epoch))
                lv_event(env, 3, 0, protected_view.document_epoch == 1 && protected_view.document_at >= 0 &&
                    protected_view.requested_at >= 0 ? protected_view.document_at - protected_view.requested_at : 0);
            else lv_failure(env);
        } else lv_failure(env);
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
}

JNIEXPORT jboolean JNICALL
Java_org_isomorphisms_ib_LongViewClient_shouldOverrideUrlLoading(JNIEnv *env, jobject self, jobject web, jobject request) {
    if ((*env)->PushLocalFrame(env, 96) != 0) return JNI_TRUE;
    int allowed ← 0;
    if (lv_current(env, self, web)) {
        jmethodID get ← lv_method(env, "android/webkit/WebResourceRequest", "getUrl", "()Landroid/net/Uri;");
        jobject uri ← request && get ? (*env)->CallObjectMethod(env, request, get) : NULL;
        jmethodID text ← lv_method(env, "android/net/Uri", "toString", "()Ljava/lang/String;");
        jstring url ← uri && text ? (*env)->CallObjectMethod(env, uri, text) : NULL;
        allowed ← lv_allowed(env, url);
        if (!allowed) lv_status(env, "This navigation requires a supported external authorization step or is not an ordinary web address.");
    }
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); allowed ← 0; }
    (*env)->PopLocalFrame(env, NULL);
    return allowed ? JNI_FALSE : JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_LongViewClient_onPageCommitVisible(JNIEnv *env, jobject self, jobject web, jstring url) {
    (void)url;
    if ((*env)->PushLocalFrame(env, 96) != 0) return;
    if (lv_current(env, self, web)) {
        jstring next ← lv_observed(env, lv_snapshot(env, self), 1);
        if (lv_nonempty(env, next) && lv_save(env, next, 0) == 0) {
            lv_status(env, "Page visible. Confirm the needed control yourself; authentication and unsaved form recovery remain unproven.");
            lv_event(env, 5, 0, lv_elapsed(protected_view.document_at));
            lv_measure(env);
        } else lv_failure(env);
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_LongViewClient_onPageFinished(JNIEnv *env, jobject self, jobject web, jstring url) {
    (void)url;
    if ((*env)->PushLocalFrame(env, 96) != 0) return;
    if (lv_current(env, self, web)) {
        lv_event(env, 6, 0, lv_elapsed(protected_view.document_at));
        lv_measure(env);
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_LongViewClient_onReceivedError(JNIEnv *env, jobject self, jobject web, jobject request, jobject error) {
    if ((*env)->PushLocalFrame(env, 96) != 0) return;
    jmethodID main ← lv_method(env, "android/webkit/WebResourceRequest", "isForMainFrame", "()Z");
    if (lv_current(env, self, web) && request && main && (*env)->CallBooleanMethod(env, request, main)) {
        jmethodID code ← lv_method(env, "android/webkit/WebResourceError", "getErrorCode", "()I");
        jint value ← error && code ? (*env)->CallIntMethod(env, error, code) : 0;
        lv_event(env, 7, value, lv_elapsed(protected_view.document_at));
        lv_status(env, "The main page did not load. Protected identity is retained; no remote action was automatically repeated.");
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
}

JNIEXPORT jboolean JNICALL
Java_org_isomorphisms_ib_LongViewClient_onRenderProcessGone(JNIEnv *env, jobject self, jobject web, jobject detail) {
    (void)detail;
    if ((*env)->PushLocalFrame(env, 96) != 0) return JNI_TRUE;
    if (lv_current(env, self, web)) {
        jstring next ← lv_observed(env, lv_snapshot(env, self), 0);
        int ok ← lv_nonempty(env, next) && lv_save(env, next, 0) == 0;
        lv_destroy_browser(env);
        lv_event(env, 8, ok ? 0 : 1, -1);
        if (ok && protected_view.replacements < 8U) {
            ++protected_view.replacements;
            jobject recovery ← lv_observer(env, protected_view.recovery);
            jobject activity ← lv_activity(env);
            jobject panel ← lv_view(env, activity, LV_PANEL);
            jmethodID post ← lv_method(env, "android/view/View", "post", "(Ljava/lang/Runnable;)Z");
            protected_view.recovering ← 1;
            ok ← recovery && panel && post && (*env)->CallBooleanMethod(env, panel, post, recovery);
            if (!ok) protected_view.recovering ← 0;
        } else if (ok) {
            lv_status(env, "The renderer repeatedly stopped. Use Resume protected page to retry its origin; any remote step may need repetition.");
        }
        if (!ok) lv_failure(env);
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_LongViewRecoveryTask_run(JNIEnv *env, jobject self) {
    if ((*env)->PushLocalFrame(env, 96) != 0) return;
    jstring snapshot ← lv_snapshot(env, self);
    jmethodID equals ← lv_method(env, "java/lang/String", "equals", "(Ljava/lang/Object;)Z");
    int current ← lv_main_thread(env) && !protected_view.broken && protected_view.visible &&
        protected_view.recovering && !protected_view.browser && snapshot && equals &&
        (*env)->CallBooleanMethod(env, snapshot, equals, protected_view.record);
    if (current) {
        protected_view.recovering ← 0;
        jobject activity ← lv_activity(env);
        if (!activity || lv_start_renderer(env, activity, lv_field(env, protected_view.record, 3)) != 0) lv_failure(env);
        else lv_status(env, "Renderer replaced at the retained origin. Re-enter the remote step if needed; authentication and form recovery are unproven.");
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
}

static int lv_numbers(const char *text, int64_t values[12]) {
    const char *cursor ← text;
    if (*cursor++ != '[') return 0;
    for (unsigned i ← 0; i < 12U; ++i) {
        while (*cursor == ' ') ++cursor;
        int negative ← *cursor == '-';
        if (negative) ++cursor;
        if (*cursor < '0' || *cursor > '9') return 0;
        int64_t value ← 0;
        while (*cursor >= '0' && *cursor <= '9') {
            value ← value * 10 + (*cursor++ - '0');
            if (value > INT64_C(1000000000000)) return 0;
        }
        if (negative && value != 1) return 0;
        values[i] ← negative ? -value : value;
        while (*cursor == ' ') ++cursor;
        if (*cursor++ != (i == 11U ? ']' : ',')) return 0;
    }
    return *cursor == 0;
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_LongViewTimingCallback_onReceiveValue(JNIEnv *env, jobject self, jobject value) {
    if ((*env)->PushLocalFrame(env, 96) != 0) return;
    if (lv_current(env, self, NULL) && lv_document_epoch(env, self) == protected_view.document_epoch) {
        char payload[256];
        int64_t metrics[12];
        if (lv_ascii(env, value, payload, sizeof(payload)) && lv_numbers(payload, metrics)) {
            jint host ← lv_counter(env, protected_view.record, 0), activity ← lv_counter(env, protected_view.record, 1);
            jint renderer ← lv_counter(env, protected_view.record, 2);
            __android_log_print(ANDROID_LOG_INFO, "IBLongView",
                "event=10 host_pid=%ld host_generation=%d activity_generation=%d renderer_generation=%d document_epoch=%d dns_ms=%" PRId64
                " connect_ms=%" PRId64 " tls_ms=%" PRId64 " first_byte_ms=%" PRId64 " transfer_ms=%" PRId64
                " dom_ms=%" PRId64 " load_ms=%" PRId64 " transfer_bytes=%" PRId64,
                (long)getpid(), host, activity, renderer, protected_view.document_epoch,
                metrics[0], metrics[1], metrics[2], metrics[3], metrics[4], metrics[5], metrics[6], metrics[7]);
            __android_log_print(ANDROID_LOG_INFO, "IBLongView",
                "event=11 host_pid=%ld host_generation=%d activity_generation=%d renderer_generation=%d document_epoch=%d first_paint_ms=%" PRId64
                " first_contentful_paint_ms=%" PRId64 " observed_resource_count=%" PRId64 " observed_resource_transfer_bytes=%" PRId64,
                (long)getpid(), host, activity, renderer, protected_view.document_epoch,
                metrics[8], metrics[9], metrics[10], metrics[11]);
        } else lv_event(env, 10, 1, -1);
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->PopLocalFrame(env, NULL);
}
