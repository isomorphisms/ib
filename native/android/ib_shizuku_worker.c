#define _GNU_SOURCE
#include "ib_shizuku_worker.h"
#include "ib_binder_flags.h"
#include "ib_shizuku_ingress_runner.h"
#include "ib_reader_pfd.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/binder_ibinder.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PACKAGE "org.isomorphisms.ib.prepaint"
#define DESCRIPTOR "org.isomorphisms.ib.AcquisitionWorker.v1"
#define INGRESSTYPE "org.isomorphisms.ib.ShizukuArenaIngress.v1"
#define PATH_CAP 4096
#define PAYLOAD_CAP (32U * 1024U * 1024U)

#if defined(__aarch64__)
#define CATALOG "worker-arm64-v8a.sha256"
#elif defined(__arm__)
#define CATALOG "worker-armeabi-v7a.sha256"
#elif defined(__x86_64__)
#define CATALOG "worker-x86_64.sha256"
#else
#error No independently qualified worker ABI profile
#endif

static struct {
    pthread_mutex_t busy;
    int ready;
    uid_t app_uid;
    char root[PATH_CAP], grease[PATH_CAP], library[PATH_CAP];
    char dex[PATH_CAP], script[PATH_CAP];
} worker = { .busy = PTHREAD_MUTEX_INITIALIZER };

static jmethodID method(JNIEnv *e, const char *type, const char *name, const char *signature) {
    if ((*e)->ExceptionCheck(e)) return NULL;
    jclass klass = (*e)->FindClass(e, type);
    return klass ? (*e)->GetMethodID(e, klass, name, signature) : NULL;
}

static int ascii(JNIEnv *e, jstring text, char *out, size_t capacity) {
    if (!text || (*e)->ExceptionCheck(e)) return 0;
    jsize size = (*e)->GetStringLength(e, text);
    if (size < 1 || (size_t)size >= capacity) return 0;
    const jchar *units = (*e)->GetStringChars(e, text, NULL);
    if (!units) return 0;
    int valid = 1;
    for (jsize i = 0; i < size; ++i) {
        if (units[i] < 33 || units[i] > 126) { valid = 0; break; }
        out[i] = (char)units[i];
    }
    (*e)->ReleaseStringChars(e, text, units);
    if (valid) out[size] = 0;
    return valid;
}

static int digest_matches(const unsigned char *bytes, size_t size, const char *expected) {
    char actual[65];
    ib_reader_sha256_hex(bytes, size, actual);
    return !memcmp(actual, expected, 64);
}

static int verified_library(const char *path, const char *expected) {
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return 0;
    struct stat info;
    int ok = fstat(fd, &info) == 0 && S_ISREG(info.st_mode) &&
        (info.st_uid == 0 || info.st_uid == 1000) && (info.st_mode & 022U) == 0 &&
        (info.st_mode & 0111U) != 0 && info.st_size > 0 && info.st_size <= PAYLOAD_CAP;
    unsigned char *bytes = ok ? malloc((size_t)info.st_size) : NULL;
    ok = ok && bytes;
    size_t used = 0;
    while (ok && used < (size_t)info.st_size) {
        ssize_t count = read(fd, bytes + used, (size_t)info.st_size - used);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = 0; break; }
        used += (size_t)count;
    }
    unsigned char extra;
    if (ok) ok = read(fd, &extra, 1) == 0 && digest_matches(bytes, used, expected);
    free(bytes);
    if (close(fd) != 0) ok = 0;
    return ok;
}

static int extract(AAssetManager *assets, const char *name, const char *destination,
                     const char *expected, size_t maximum) {
    AAsset *asset = AAssetManager_open(assets, name, AASSET_MODE_BUFFER);
    if (!asset) return 0;
    off64_t size = AAsset_getLength64(asset);
    const unsigned char *bytes = AAsset_getBuffer(asset);
    int ok = size > 0 && (uint64_t)size <= maximum && bytes &&
        digest_matches(bytes, (size_t)size, expected);
    int fd = ok ? open(destination, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0400) : -1;
    ok = ok && fd >= 0;
    size_t used = 0;
    while (ok && used < (size_t)size) {
        ssize_t count = write(fd, bytes + used, (size_t)size - used);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ok = 0; break; }
        used += (size_t)count;
    }
    if (fd >= 0 && close(fd) != 0) ok = 0;
    AAsset_close(asset);
    return ok;
}

static void cleanup(void) {
    /* Only exact temporary leaves minted by this process, never app content. */
    if (worker.dex[0]) (void)unlink(worker.dex);
    if (worker.script[0]) (void)unlink(worker.script);
    if (worker.root[0]) (void)rmdir(worker.root);
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_ShizukuWorker_initialize(JNIEnv *e, jobject self, jobject context) {
    if ((*e)->PushLocalFrame(e, 64) != 0) return;
    int ok = getuid() == 2000 && !worker.ready && context;
    jmethodID package = method(e, "android/content/Context", "getPackageName", "()Ljava/lang/String;");
    jstring name = ok && package ? (*e)->CallObjectMethod(e, context, package) : NULL;
    char actual[sizeof(PACKAGE)] = {0};
    ok = ok && ascii(e, name, actual, sizeof(actual)) && !strcmp(actual, PACKAGE);
    jmethodID get = method(e, "android/content/Context", "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
    jobject app = ok && get ? (*e)->CallObjectMethod(e, context, get) : NULL;
    jclass type = (*e)->FindClass(e, "android/content/pm/ApplicationInfo");
    jfieldID uid = type ? (*e)->GetFieldID(e, type, "uid", "I") : NULL;
    jfieldID directory = type ? (*e)->GetFieldID(e, type, "nativeLibraryDir", "Ljava/lang/String;") : NULL;
    jint owner = app && uid ? (*e)->GetIntField(e, app, uid) : -1;
    jstring directory_text = app && directory ? (*e)->GetObjectField(e, app, directory) : NULL;
    char native[PATH_CAP] = {0};
    ok = ok && owner >= 10000 && ascii(e, directory_text, native, sizeof(native)) &&
        !strncmp(native, "/data/app/", 10) && !strstr(native, "/../") && !strstr(native, "/./");
    jmethodID get_assets = method(e, "android/content/Context", "getAssets", "()Landroid/content/res/AssetManager;");
    jobject asset_object = ok && get_assets ? (*e)->CallObjectMethod(e, context, get_assets) : NULL;
    AAssetManager *assets = asset_object ? AAssetManager_fromJava(e, asset_object) : NULL;
    AAsset *catalog = assets ? AAssetManager_open(assets, CATALOG, AASSET_MODE_BUFFER) : NULL;
    const char *contents = catalog ? AAsset_getBuffer(catalog) : NULL;
    char hashes[4][65] = {{0}};
    ok = ok && catalog && AAsset_getLength64(catalog) == 270 && contents &&
        !memcmp(contents, "IBWORKER1\n", 10);
    for (unsigned i = 0; ok && i < 4; ++i) {
        const char *hash = contents + 10 + 65 * i;
        for (unsigned j = 0; j < 64; ++j)
            if (!((hash[j] >= '0' && hash[j] <= '9') || (hash[j] >= 'a' && hash[j] <= 'f'))) ok = 0;
        ok = ok && hash[64] == '\n';
        memcpy(hashes[i], hash, 64);
    }
    if (catalog) AAsset_close(catalog);
    if (ok) {
        int a = snprintf(worker.grease, sizeof(worker.grease), "%s/libib_grease.so", native);
        int b = snprintf(worker.library, sizeof(worker.library), "%s/libicu_transport.so", native);
        ok = a > 0 && (size_t)a < sizeof(worker.grease) && b > 0 && (size_t)b < sizeof(worker.library) &&
            verified_library(worker.grease, hashes[0]) && verified_library(worker.library, hashes[1]);
    }
    if (ok) {
        int count = snprintf(worker.root, sizeof(worker.root), "/data/local/tmp/ib-reader-%u.XXXXXX", (unsigned)owner);
        ok = count > 0 && (size_t)count < sizeof(worker.root) && mkdtemp(worker.root) && chmod(worker.root, 0700) == 0;
    }
    if (ok) {
        int a = snprintf(worker.dex, sizeof(worker.dex), "%s/icu.dex", worker.root);
        int b = snprintf(worker.script, sizeof(worker.script), "%s/worker.grease", worker.root);
        ok = a > 0 && (size_t)a < sizeof(worker.dex) && b > 0 && (size_t)b < sizeof(worker.script) &&
            extract(assets, "icu-worker.dex", worker.dex, hashes[2], PAYLOAD_CAP) &&
            extract(assets, "ib-worker.grease", worker.script, hashes[3], 65536);
    }
    jmethodID attach = method(e, "android/os/Binder", "attachInterface", "(Landroid/os/IInterface;Ljava/lang/String;)V");
    jstring descriptor = ok && attach ? (*e)->NewStringUTF(e, DESCRIPTOR) : NULL;
    if (descriptor) (*e)->CallVoidMethod(e, self, attach, NULL, descriptor);
    ok = ok && descriptor && !(*e)->ExceptionCheck(e);
    if (ok) { worker.app_uid = (uid_t)owner; worker.ready = 1; }
    else {
        cleanup();
        if (!(*e)->ExceptionCheck(e)) {
            jclass exception = (*e)->FindClass(e, "java/lang/SecurityException");
            if (exception) (*e)->ThrowNew(e, exception, "IB worker identity or source-bound payload refused");
        }
    }
    (*e)->PopLocalFrame(e, NULL);
}

static int write_status(JNIEnv *e, jobject reply, jint status) {
    jmethodID write = method(e, "android/os/Parcel", "writeInt", "(I)V");
    if (write) (*e)->CallVoidMethod(e, reply, write, status);
    return write && !(*e)->ExceptionCheck(e);
}

JNIEXPORT jboolean JNICALL
Java_org_isomorphisms_ib_ShizukuWorker_onTransact(JNIEnv *e, jobject self, jint code,
                                                 jobject data, jobject reply, jint flags) {
    if ((*e)->PushLocalFrame(e, 64) != 0) return JNI_FALSE;
    uid_t caller = (uid_t)AIBinder_getCallingUid();
    if (code == 16777115 && ib_binder_synchronous(flags) && worker.ready &&
        (caller == worker.app_uid || caller == 2000 || caller == 0)) {
        /* Shizuku's documented non-daemon destruction boundary. Parent death
         * wakes the Grease group trap before this process can lose authority. */
        cleanup();
        _exit(0);
    }
    if (code == 0x5f4e5446 && ib_binder_synchronous(flags) && reply) {
        jmethodID write = method(e, "android/os/Parcel", "writeString", "(Ljava/lang/String;)V");
        jstring descriptor = write ? (*e)->NewStringUTF(e, DESCRIPTOR) : NULL;
        if (descriptor) (*e)->CallVoidMethod(e, reply, write, descriptor);
        int ok = descriptor && !(*e)->ExceptionCheck(e);
        (*e)->PopLocalFrame(e, NULL);
        return ok ? JNI_TRUE : JNI_FALSE;
    }
    int ok = worker.ready && caller == worker.app_uid && code == 2 && ib_binder_synchronous(flags) && data && reply;
    jmethodID size = method(e, "android/os/Parcel", "dataSize", "()I");
    jmethodID fds = method(e, "android/os/Parcel", "hasFileDescriptors", "()Z");
    jint bytes = ok && size ? (*e)->CallIntMethod(e, data, size) : -1;
    ok = ok && size && fds && bytes > 0 && bytes <= 1024 &&
        !(*e)->CallBooleanMethod(e, data, fds) && !(*e)->ExceptionCheck(e);
    jmethodID enforce = method(e, "android/os/Parcel", "enforceInterface", "(Ljava/lang/String;)V");
    jmethodID read_binder = method(e, "android/os/Parcel", "readStrongBinder", "()Landroid/os/IBinder;");
    jmethodID read_name = method(e, "android/os/Parcel", "readString", "()Ljava/lang/String;");
    jmethodID read_generation = method(e, "android/os/Parcel", "readLong", "()J");
    jmethodID available = method(e, "android/os/Parcel", "dataAvail", "()I");
    jstring descriptor = ok && enforce ? (*e)->NewStringUTF(e, DESCRIPTOR) : NULL;
    if (descriptor) (*e)->CallVoidMethod(e, data, enforce, descriptor);
    ok = ok && descriptor && read_binder && read_name && read_generation && available && !(*e)->ExceptionCheck(e);
    jobject ingress = ok ? (*e)->CallObjectMethod(e, data, read_binder) : NULL;
    jstring id_text = ok && !(*e)->ExceptionCheck(e) ? (*e)->CallObjectMethod(e, data, read_name) : NULL;
    char id[IB_READER_REQUEST_ID_CAP + 1U] = {0};
    ok = ok && ingress && ascii(e, id_text, id, sizeof(id));
    jlong generation = ok ? (*e)->CallLongMethod(e, data, read_generation) : 0;
    ok = ok && generation > 0 && !(*e)->ExceptionCheck(e) &&
        (*e)->CallIntMethod(e, data, available) == 0 && !(*e)->ExceptionCheck(e);
    jmethodID describe = method(e, "android/os/IBinder", "getInterfaceDescriptor", "()Ljava/lang/String;");
    jstring actual_text = ok && describe ? (*e)->CallObjectMethod(e, ingress, describe) : NULL;
    char actual[sizeof(INGRESSTYPE)] = {0};
    ok = ok && ascii(e, actual_text, actual, sizeof(actual)) && !strcmp(actual, INGRESSTYPE);
    int locked = ok && pthread_mutex_trylock(&worker.busy) == 0;
    int result = -1;
    if (locked) {
        jmethodID link = method(e, "android/os/IBinder", "linkToDeath", "(Landroid/os/IBinder$DeathRecipient;I)V");
        jmethodID unlink = method(e, "android/os/IBinder", "unlinkToDeath", "(Landroid/os/IBinder$DeathRecipient;I)Z");
        if (link && unlink) (*e)->CallVoidMethod(e, ingress, link, self, 0);
        ok = link && unlink && !(*e)->ExceptionCheck(e);
        char capture_directory[PATH_CAP] = {0};
        int count = ok ? snprintf(capture_directory, sizeof(capture_directory), "%s/capture.XXXXXX", worker.root) : -1;
        ok = ok && count > 0 && (size_t)count < sizeof(capture_directory) && mkdtemp(capture_directory) &&
            chmod(capture_directory, 0700) == 0;
        if (ok) {
            ib_icu_worker_runtime runtime = {worker.grease, worker.script, worker.dex, worker.library, capture_directory};
            result = ib_shizuku_ingress_run_icu(e, ingress, id, (uint64_t)generation, &runtime);
        }
        if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
        if (unlink) (void)(*e)->CallBooleanMethod(e, ingress, unlink, self, 0);
        if (capture_directory[0]) (void)rmdir(capture_directory);
        pthread_mutex_unlock(&worker.busy);
    }
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    int answered = reply && ib_binder_synchronous(flags) && write_status(e, reply, result);
    (*e)->PopLocalFrame(e, NULL);
    return answered ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_ShizukuWorker_binderDied(JNIEnv *e, jobject self) {
    (void)e; (void)self;
    /* Only the app capability is linked. No shell daemon survives app death. */
    _exit(125);
}
