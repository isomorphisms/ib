/* SDK-only qualification boundary. Query and main-frame policy are checked
 * Idric. No result semantics, HTTP implementation, challenge solver or durable
 * navigation state lives here. This file is not the IB production adapter. */
#define _GNU_SOURCE
#include <jni.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static jobject browser;
static jclass generated;
static jmethodID allowed;
static unsigned polls, snapshots;
static char directory[4096];

static jmethodID method(JNIEnv *e, const char *type, const char *name, const char *signature) {
    jclass klass = (*e)->FindClass(e, type);
    return klass ? (*e)->GetMethodID(e, klass, name, signature) : NULL;
}
static jobject construct(JNIEnv *e, const char *name) {
    jclass type = (*e)->FindClass(e, name);
    jmethodID init = type ? (*e)->GetMethodID(e, type, "<init>", "()V") : NULL;
    return init ? (*e)->NewObject(e, type, init) : NULL;
}
static int permitted(JNIEnv *e, jstring url) {
    return url && generated && allowed && !(*e)->ExceptionCheck(e) &&
        (*e)->CallStaticIntMethod(e, generated, allowed, url) == 1 && !(*e)->ExceptionCheck(e);
}
static void record(JNIEnv *e, const char *name, jstring text) {
    if (!text || !directory[0] || (*e)->ExceptionCheck(e)) return;
    jsize length = (*e)->GetStringLength(e, text);
    if (length < 1 || length > 2097152) return;
    const jchar *units = (*e)->GetStringChars(e, text, NULL);
    if (!units) return;
    int valid = 1;
    for (jsize i = 0; i < length; ++i) {
        if (units[i] >= 0xd800 && units[i] <= 0xdbff) {
            if (++i >= length || units[i] < 0xdc00 || units[i] > 0xdfff) { valid = 0; break; }
        } else if (units[i] >= 0xdc00 && units[i] <= 0xdfff) { valid = 0; break; }
    }
    (*e)->ReleaseStringChars(e, text, units);
    if (!valid) return;
    jmethodID get_bytes = method(e, "java/lang/String", "getBytes", "(Ljava/lang/String;)[B");
    jstring encoding = (*e)->NewStringUTF(e, "UTF-8");
    jbyteArray bytes = get_bytes && encoding ? (*e)->CallObjectMethod(e, text, get_bytes, encoding) : NULL;
    if (!bytes || (*e)->ExceptionCheck(e)) return;
    jsize count = (*e)->GetArrayLength(e, bytes);
    if (count < 1 || count > 2097152) return;
    jbyte *value = (*e)->GetByteArrayElements(e, bytes, NULL);
    char path[4200];
    int size = snprintf(path, sizeof(path), "%s/%s", directory, name);
    int fd = value && size > 0 && (size_t)size < sizeof(path) ?
        open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600) : -1;
    if (fd >= 0) {
        size_t written = 0;
        while (written < (size_t)count) {
            ssize_t used = write(fd, value + written, (size_t)count - written);
            if (used <= 0) break;
            written += (size_t)used;
        }
        if (written == (size_t)count) (void)fsync(fd);
        close(fd);
    }
    if (value) (*e)->ReleaseByteArrayElements(e, bytes, value, JNI_ABORT);
}
static void stopped(JNIEnv *e, const char *reason) {
    if (!browser) return;
    jmethodID stop = method(e, "android/webkit/WebView", "stopLoading", "()V");
    if (stop) (*e)->CallVoidMethod(e, browser, stop);
    record(e, "terminal.txt", (*e)->NewStringUTF(e, reason));
    polls = 32;
}

JNIEXPORT void JNICALL Java_org_isomorphisms_ib_googleprobe_Bridge_stop(JNIEnv *e, jclass type) {
    (void)type;
    if (browser) {
        stopped(e, "ACTIVITY_DESTROYED");
        jmethodID destroy = method(e, "android/webkit/WebView", "destroy", "()V");
        if (destroy) (*e)->CallVoidMethod(e, browser, destroy);
        (*e)->DeleteGlobalRef(e, browser);
        browser = NULL;
    }
    if (generated) { (*e)->DeleteGlobalRef(e, generated); generated = NULL; }
    directory[0] = 0;
}

JNIEXPORT void JNICALL Java_org_isomorphisms_ib_googleprobe_Client_onPageStarted(
    JNIEnv *e, jobject self, jobject view, jstring url, jobject icon) {
    (void)self; (void)view; (void)icon;
    if (!permitted(e, url)) stopped(e, "REFUSED_MAIN_FRAME_OR_CONSENT_OR_VERIFICATION");
}
JNIEXPORT jboolean JNICALL Java_org_isomorphisms_ib_googleprobe_Client_shouldOverrideUrlLoading(
    JNIEnv *e, jobject self, jobject view, jobject request) {
    (void)self; (void)view;
    jmethodID main_frame = method(e, "android/webkit/WebResourceRequest", "isForMainFrame", "()Z");
    if (main_frame && !(*e)->CallBooleanMethod(e, request, main_frame)) return JNI_FALSE;
    jmethodID uri = method(e, "android/webkit/WebResourceRequest", "getUrl", "()Landroid/net/Uri;");
    jobject address = uri ? (*e)->CallObjectMethod(e, request, uri) : NULL;
    jmethodID text = method(e, "android/net/Uri", "toString", "()Ljava/lang/String;");
    jstring url = address && text ? (*e)->CallObjectMethod(e, address, text) : NULL;
    if (permitted(e, url)) return JNI_FALSE;
    stopped(e, "REFUSED_MAIN_FRAME_OR_CONSENT_OR_VERIFICATION");
    return JNI_TRUE;
}
JNIEXPORT void JNICALL Java_org_isomorphisms_ib_googleprobe_Snapshot_onReceiveValue(
    JNIEnv *e, jobject self, jobject supplied) {
    (void)self;
    if (!browser || polls > 31 || !supplied || snapshots >= 30) return;
    jclass type = (*e)->FindClass(e, "org/json/JSONArray");
    jmethodID init = type ? (*e)->GetMethodID(e, type, "<init>", "(Ljava/lang/String;)V") : NULL;
    jobject array = init ? (*e)->NewObject(e, type, init, supplied) : NULL;
    jmethodID get = type ? (*e)->GetMethodID(e, type, "getString", "(I)Ljava/lang/String;") : NULL;
    jstring url = array && get ? (*e)->CallObjectMethod(e, array, get, 0) : NULL;
    jstring dom = url ? (*e)->CallObjectMethod(e, array, get, 1) : NULL;
    if (!(*e)->ExceptionCheck(e) && permitted(e, url)) {
        char name[80];
        ++snapshots;
        (void)snprintf(name, sizeof(name), "snapshot-%02u.dom.html", snapshots);
        record(e, name, dom);
        (void)snprintf(name, sizeof(name), "snapshot-%02u.url.txt", snapshots);
        record(e, name, url);
    }
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
}
JNIEXPORT void JNICALL Java_org_isomorphisms_ib_googleprobe_Tick_run(JNIEnv *e, jobject self) {
    (void)self;
    if (!browser || polls >= 30) { stopped(e, "BOUNDED_OBSERVATION_COMPLETE"); return; }
    ++polls;
    if ((*e)->PushLocalFrame(e, 16) != 0) return;
    jobject callback = construct(e, "org/isomorphisms/ib/googleprobe/Snapshot");
    jmethodID evaluate = method(e, "android/webkit/WebView", "evaluateJavascript", "(Ljava/lang/String;Landroid/webkit/ValueCallback;)V");
    /* Read only: normal first-party scripts run unmodified. No generated
     * challenge answers, cookie replay, DOM rewrite or access-control bypass. */
    jstring script = (*e)->NewStringUTF(e,
        "[location.href,document.documentElement?document.documentElement.outerHTML:'']");
    if (callback && evaluate && script) (*e)->CallVoidMethod(e, browser, evaluate, script, callback);
    jobject tick = construct(e, "org/isomorphisms/ib/googleprobe/Tick");
    jmethodID post = method(e, "android/view/View", "postDelayed", "(Ljava/lang/Runnable;J)Z");
    if (tick && post && !(*e)->ExceptionCheck(e)) (*e)->CallBooleanMethod(e, browser, post, tick, (jlong)1000);
    if ((*e)->ExceptionCheck(e)) { (*e)->ExceptionClear(e); stopped(e, "SDK_CALLBACK_FAILED"); }
    (*e)->PopLocalFrame(e, NULL);
}
JNIEXPORT void JNICALL Java_org_isomorphisms_ib_googleprobe_Bridge_start(
    JNIEnv *e, jclass ignored, jobject activity, jstring url) {
    (void)ignored;
    if (browser) return;
    jclass candidate = (*e)->FindClass(e, "Idric/Generated");
    generated = candidate ? (*e)->NewGlobalRef(e, candidate) : NULL;
    allowed = candidate ? (*e)->GetStaticMethodID(e, candidate, "probe_google_url", "(Ljava/lang/String;)I") : NULL;
    if (!permitted(e, url)) return;
    jmethodID files = method(e, "android/content/Context", "getFilesDir", "()Ljava/io/File;");
    jobject file = files ? (*e)->CallObjectMethod(e, activity, files) : NULL;
    jmethodID absolute = method(e, "java/io/File", "getAbsolutePath", "()Ljava/lang/String;");
    jstring path = file && absolute ? (*e)->CallObjectMethod(e, file, absolute) : NULL;
    const char *root = path ? (*e)->GetStringUTFChars(e, path, NULL) : NULL;
    int size = root ? snprintf(directory, sizeof(directory), "%s/browser-probe", root) : -1;
    if (root) (*e)->ReleaseStringUTFChars(e, path, root);
    if (size <= 0 || (size_t)size >= sizeof(directory) || mkdir(directory, 0700) != 0) return;
    record(e, "provenance.tsv", (*e)->NewStringUTF(e,
        "representation\tbrowser-rendered-dom\noriginal_network_bytes\tNOT_CAPTURED\n"
        "ICU_body_identity\tNOT_APPLICABLE\nsource_offsets\tUTF8_DOM_SNAPSHOT_ONLY\n"
        "cookie_policy\tordinary_private_WebView_profile_no_cookie_copy\n"
        "TLS_policy\tdefault_strict_no_SSL_override\n"));
    record(e, "requested-url.txt", url);
    jclass web = (*e)->FindClass(e, "android/webkit/WebView");
    jmethodID constructor = web ? (*e)->GetMethodID(e, web, "<init>", "(Landroid/content/Context;)V") : NULL;
    jobject view = constructor ? (*e)->NewObject(e, web, constructor, activity) : NULL;
    if (!view || (*e)->ExceptionCheck(e)) return;
    browser = (*e)->NewGlobalRef(e, view);
    jmethodID settings_method = method(e, "android/webkit/WebView", "getSettings", "()Landroid/webkit/WebSettings;");
    jobject settings = settings_method ? (*e)->CallObjectMethod(e, view, settings_method) : NULL;
    const char *flags[] = {"setJavaScriptEnabled", "setDomStorageEnabled", "setAllowFileAccess", "setAllowContentAccess", "setSupportMultipleWindows"};
    for (unsigned i = 0; i < sizeof(flags)/sizeof(flags[0]); ++i) {
        jmethodID set = method(e, "android/webkit/WebSettings", flags[i], "(Z)V");
        if (settings && set) (*e)->CallVoidMethod(e, settings, set, i < 2 ? JNI_TRUE : JNI_FALSE);
    }
    jmethodID mixed = method(e, "android/webkit/WebSettings", "setMixedContentMode", "(I)V");
    if (settings && mixed) (*e)->CallVoidMethod(e, settings, mixed, (jint)1);
    jobject client = construct(e, "org/isomorphisms/ib/googleprobe/Client");
    jmethodID set_client = method(e, "android/webkit/WebView", "setWebViewClient", "(Landroid/webkit/WebViewClient;)V");
    if (set_client && client) (*e)->CallVoidMethod(e, view, set_client, client);
    jmethodID display = method(e, "android/app/Activity", "setContentView", "(Landroid/view/View;)V");
    if (display) (*e)->CallVoidMethod(e, activity, display, view);
    jmethodID load = method(e, "android/webkit/WebView", "loadUrl", "(Ljava/lang/String;)V");
    if (load && !(*e)->ExceptionCheck(e)) (*e)->CallVoidMethod(e, view, load, url);
    polls = snapshots = 0;
    Java_org_isomorphisms_ib_googleprobe_Tick_run(e, NULL);
}
