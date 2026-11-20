#define _POSIX_C_SOURCE 200809L
#include "ib_shizuku_ingress_binder.h"
#include "ib_binder_flags.h"

#if defined(__ANDROID__) && __ANDROID_API__ >= 29
#include <android/binder_ibinder_jni.h>
#include <android/binder_status.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define INGRESS_DESCRIPTOR "org.isomorphisms.ib.ShizukuArenaIngress.v1"
#define SDK_CLASS "org/isomorphisms/ib/PrivateIngress"
#define MAX_REQUEST_PARCEL_BYTES 512

struct ib_ingress_capability {
    atomic_uint references;
    ib_shizuku_arena_ingress session;
};

ib_ingress_capability *ib_ingress_capability_new(ib_ingress_publish publish, void *owner) {
    ib_ingress_capability *capability = calloc(1, sizeof(*capability));
    if (!capability) return NULL;
    if (ib_ingress_init(&capability->session, publish, owner) != 0) {
        free(capability);
        return NULL;
    }
    atomic_init(&capability->references, 1);
    return capability;
}

ib_shizuku_arena_ingress *ib_ingress_capability_session(ib_ingress_capability *capability) {
    return capability ? &capability->session : NULL;
}

void ib_ingress_capability_retain(ib_ingress_capability *capability) {
    if (capability) (void)atomic_fetch_add(&capability->references, 1);
}

void ib_ingress_capability_release(ib_ingress_capability *capability) {
    if (capability && atomic_fetch_sub(&capability->references, 1) == 1) {
        ib_ingress_dispose(&capability->session);
        free(capability);
    }
}

/* The opaque native anchor owns memory, not an RPC surface. A private field
 * in the SDK Binder retains it until remote references and calls have gone. */
static void *anchor_created(void *opaque) { return opaque; }
static void anchor_destroyed(void *opaque) { ib_ingress_capability_release(opaque); }
static binder_status_t anchor_refuses(AIBinder *self, transaction_code_t code,
                                      const AParcel *in, AParcel *out) {
    (void)self; (void)code; (void)in; (void)out;
    return STATUS_UNKNOWN_TRANSACTION;
}
static pthread_once_t anchor_once = PTHREAD_ONCE_INIT;
static AIBinder_Class *anchor_class;
static void initialize_anchor(void) {
    anchor_class = AIBinder_Class_define("org.isomorphisms.ib.IngressStorageAnchor.v1",
        anchor_created, anchor_destroyed, anchor_refuses);
}
static const AIBinder_Class *get_anchor_class(void) {
    (void)pthread_once(&anchor_once, initialize_anchor);
    return anchor_class;
}

static jmethodID method(JNIEnv *e, const char *type, const char *name, const char *signature) {
    if ((*e)->ExceptionCheck(e)) return NULL;
    jclass klass = (*e)->FindClass(e, type);
    return klass ? (*e)->GetMethodID(e, klass, name, signature) : NULL;
}

static int ascii(JNIEnv *e, jstring value, char *destination, size_t capacity) {
    if (!value || (*e)->ExceptionCheck(e) || capacity < 2) return 0;
    jsize length = (*e)->GetStringLength(e, value);
    if (length <= 0 || (size_t)length >= capacity) return 0;
    const jchar *characters = (*e)->GetStringChars(e, value, NULL);
    if (!characters) return 0;
    int ok = 1;
    for (jsize i = 0; i < length; ++i) {
        if (characters[i] < 33 || characters[i] > 126) { ok = 0; break; }
        destination[i] = (char)characters[i];
    }
    (*e)->ReleaseStringChars(e, value, characters);
    if (ok) destination[length] = '\0';
    return ok;
}

static int valid_name(const char *name) {
    size_t size = name ? strnlen(name, IB_READER_REQUEST_ID_CAP + 1U) : 0;
    if (!size || size > IB_READER_REQUEST_ID_CAP || !strcmp(name, ".") || !strcmp(name, "..")) return 0;
    for (size_t i = 0; i < size; ++i) {
        unsigned char c = (unsigned char)name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}

static int has_descriptors(JNIEnv *e, jobject data) {
    jmethodID check = method(e, "android/os/Parcel", "hasFileDescriptors", "()Z");
    return !check || (*e)->CallBooleanMethod(e, data, check) || (*e)->ExceptionCheck(e);
}

static int consumed(JNIEnv *e, jobject data) {
    jmethodID available = method(e, "android/os/Parcel", "dataAvail", "()I");
    return available && (*e)->CallIntMethod(e, data, available) == 0 && !(*e)->ExceptionCheck(e);
}

static int bounded_reply(JNIEnv *e, jobject data, jint maximum) {
    jmethodID size = method(e, "android/os/Parcel", "dataSize", "()I");
    if (!size) return 0;
    jint bytes = (*e)->CallIntMethod(e, data, size);
    return !(*e)->ExceptionCheck(e) && bytes > 0 && bytes <= maximum;
}

static void recycle(JNIEnv *e, jobject data, jobject reply, jobject descriptor) {
    jthrowable first = (*e)->ExceptionOccurred(e);
    if (first) (*e)->ExceptionClear(e);
    jmethodID release = method(e, "android/os/Parcel", "recycle", "()V");
    jobject parcels[] = {data, reply};
    for (unsigned i = 0; i < 2; ++i) {
        if (release && parcels[i]) (*e)->CallVoidMethod(e, parcels[i], release);
        if ((*e)->ExceptionCheck(e)) {
            if (!first) first = (*e)->ExceptionOccurred(e);
            (*e)->ExceptionClear(e);
        }
    }
    if (descriptor) {
        jmethodID close = method(e, "android/os/ParcelFileDescriptor", "close", "()V");
        if (close) (*e)->CallVoidMethod(e, descriptor, close);
        if ((*e)->ExceptionCheck(e)) {
            if (!first) first = (*e)->ExceptionOccurred(e);
            (*e)->ExceptionClear(e);
        }
    }
    if (first) (*e)->Throw(e, first);
}

jobject ib_shizuku_ingress_binder_new(JNIEnv *e, ib_ingress_capability *capability) {
    if (!e || !capability || !get_anchor_class()) return NULL;
    jclass type = (*e)->FindClass(e, SDK_CLASS);
    if (!type) return NULL;
    jmethodID constructor = (*e)->GetMethodID(e, type, "<init>", "()V");
    if (!constructor) return NULL;
    jfieldID field = (*e)->GetFieldID(e, type, "storageAnchor", "Landroid/os/IBinder;");
    if (!field) return NULL;
    jobject result = (*e)->NewObject(e, type, constructor);
    if (!result) return NULL;
    ib_ingress_capability_retain(capability);
    AIBinder *anchor = AIBinder_new(get_anchor_class(), capability);
    if (!anchor) { ib_ingress_capability_release(capability); return NULL; }
    jobject java_anchor = AIBinder_toJavaBinder(e, anchor);
    if (java_anchor && !(*e)->ExceptionCheck(e)) (*e)->SetObjectField(e, result, field, java_anchor);
    AIBinder_decStrong(anchor);
    if (!java_anchor || (*e)->ExceptionCheck(e)) return NULL;
    jmethodID attach = method(e, "android/os/Binder", "attachInterface", "(Landroid/os/IInterface;Ljava/lang/String;)V");
    if (!attach) return NULL;
    jstring descriptor = (*e)->NewStringUTF(e, INGRESS_DESCRIPTOR);
    if (!descriptor) return NULL;
    (*e)->CallVoidMethod(e, result, attach, NULL, descriptor);
    return (*e)->ExceptionCheck(e) ? NULL : result;
}

static AIBinder *owned_anchor(JNIEnv *e, jobject self) {
    jclass type = (*e)->FindClass(e, SDK_CLASS);
    if (!type) return NULL;
    jfieldID field = (*e)->GetFieldID(e, type, "storageAnchor", "Landroid/os/IBinder;");
    if (!field) return NULL;
    jobject value = (*e)->GetObjectField(e, self, field);
    if (!value || (*e)->ExceptionCheck(e)) return NULL;
    AIBinder *anchor = AIBinder_fromJavaBinder(e, value);
    if (anchor && AIBinder_getClass(anchor) != get_anchor_class()) {
        AIBinder_decStrong(anchor);
        return NULL;
    }
    return anchor;
}

static int write_integer(JNIEnv *e, jobject reply, jint value) {
    jmethodID write = method(e, "android/os/Parcel", "writeInt", "(I)V");
    if (!write) return 0;
    (*e)->CallVoidMethod(e, reply, write, value);
    return !(*e)->ExceptionCheck(e);
}

static int write_pipe(JNIEnv *e, jobject reply, int *owned_fd) {
    jclass type = (*e)->FindClass(e, "android/os/ParcelFileDescriptor");
    if (!type) return 0;
    jmethodID adopt = (*e)->GetStaticMethodID(e, type, "adoptFd", "(I)Landroid/os/ParcelFileDescriptor;");
    if (!adopt) return 0;
    jobject descriptor = (*e)->CallStaticObjectMethod(e, type, adopt, *owned_fd);
    if (!descriptor || (*e)->ExceptionCheck(e)) return 0;
    *owned_fd = -1;
    jmethodID write = method(e, "android/os/ParcelFileDescriptor", "writeToParcel", "(Landroid/os/Parcel;I)V");
    if (write) (*e)->CallVoidMethod(e, descriptor, write, reply, 0);
    int ok = write && !(*e)->ExceptionCheck(e);
    recycle(e, NULL, NULL, descriptor);
    return ok && !(*e)->ExceptionCheck(e);
}

JNIEXPORT jboolean JNICALL
Java_org_isomorphisms_ib_PrivateIngress_onTransact(JNIEnv *e, jobject self, jint code,
                                                   jobject data, jobject reply, jint flags) {
    if ((*e)->PushLocalFrame(e, 128) != 0) return JNI_FALSE;
    if (code == 0x5f4e5446 && ib_binder_synchronous(flags) && reply) {
        jmethodID write = method(e, "android/os/Parcel", "writeString", "(Ljava/lang/String;)V");
        jstring descriptor = write ? (*e)->NewStringUTF(e, INGRESS_DESCRIPTOR) : NULL;
        if (descriptor) (*e)->CallVoidMethod(e, reply, write, descriptor);
        int ok = descriptor && !(*e)->ExceptionCheck(e);
        (*e)->PopLocalFrame(e, NULL);
        return ok ? JNI_TRUE : JNI_FALSE;
    }
    if (!ib_binder_synchronous(flags) || !data || !reply ||
        (code != IB_SHIZUKU_INGRESS_BEGIN && code != IB_SHIZUKU_INGRESS_FINISH)) {
        (*e)->PopLocalFrame(e, NULL);
        return JNI_FALSE;
    }
    AIBinder *anchor = owned_anchor(e, self);
    ib_ingress_capability *capability = anchor ? AIBinder_getUserData(anchor) : NULL;
    ib_shizuku_arena_ingress *session = ib_ingress_capability_session(capability);
    uid_t uid = (uid_t)AIBinder_getCallingUid();
    int accepted = 0, fd = -1;
    char name[IB_READER_REQUEST_ID_CAP + 1U] = {0};
    jlong generation = 0;
    jint exit_status = -1;
    int valid = session && uid == session->permitted_bridge_uid;
    jmethodID size = method(e, "android/os/Parcel", "dataSize", "()I");
    if (valid && size) {
        jint bytes = (*e)->CallIntMethod(e, data, size);
        valid = !(*e)->ExceptionCheck(e) && bytes > 0 && bytes <= MAX_REQUEST_PARCEL_BYTES && !has_descriptors(e, data);
    } else valid = 0;
    jmethodID enforce = method(e, "android/os/Parcel", "enforceInterface", "(Ljava/lang/String;)V");
    jmethodID read_name = method(e, "android/os/Parcel", "readString", "()Ljava/lang/String;");
    jmethodID read_generation = method(e, "android/os/Parcel", "readLong", "()J");
    if (valid && enforce && read_name && read_generation) {
        jstring descriptor = (*e)->NewStringUTF(e, INGRESS_DESCRIPTOR);
        if (descriptor) (*e)->CallVoidMethod(e, data, enforce, descriptor);
        valid = descriptor && !(*e)->ExceptionCheck(e);
        jstring text = valid ? (*e)->CallObjectMethod(e, data, read_name) : NULL;
        valid = valid && ascii(e, text, name, sizeof(name)) && valid_name(name);
        if (valid) generation = (*e)->CallLongMethod(e, data, read_generation);
        valid = valid && !(*e)->ExceptionCheck(e) && generation > 0;
    } else valid = 0;
    if (valid && code == IB_SHIZUKU_INGRESS_FINISH) {
        jmethodID read = method(e, "android/os/Parcel", "readInt", "()I");
        if (read) exit_status = (*e)->CallIntMethod(e, data, read);
        valid = read && !(*e)->ExceptionCheck(e) && exit_status >= 0 && exit_status <= 255;
    }
    valid = valid && consumed(e, data);
    if (valid && code == IB_SHIZUKU_INGRESS_BEGIN) {
        accepted = ib_ingress_begin(session, uid, name, (uint64_t)generation, &fd) == 0 && fd >= 0;
        if (accepted) {
            jmethodID write = method(e, "android/os/Parcel", "writeString", "(Ljava/lang/String;)V");
            jstring url = write ? (*e)->NewStringUTF(e, session->requested_url) : NULL;
            accepted = write && url && write_integer(e, reply, 0);
            if (accepted) (*e)->CallVoidMethod(e, reply, write, url);
            accepted = accepted && !(*e)->ExceptionCheck(e) && write_pipe(e, reply, &fd);
            if (!accepted) ib_ingress_revoke(session);
        }
    } else if (valid) {
        int finished = ib_ingress_finish(session, uid, name, (uint64_t)generation, exit_status);
        accepted = write_integer(e, reply, finished == 0 ? 0 : -1);
    }
    if (fd >= 0) close(fd);
    if (!accepted) {
        if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
        accepted = write_integer(e, reply, -1);
    }
    if (anchor) AIBinder_decStrong(anchor);
    (*e)->PopLocalFrame(e, NULL);
    return accepted ? JNI_TRUE : JNI_FALSE;
}

static jobject new_parcel(JNIEnv *e) {
    if ((*e)->ExceptionCheck(e)) return NULL;
    jclass type = (*e)->FindClass(e, "android/os/Parcel");
    if (!type) return NULL;
    jmethodID obtain = (*e)->GetStaticMethodID(e, type, "obtain", "()Landroid/os/Parcel;");
    return obtain ? (*e)->CallStaticObjectMethod(e, type, obtain) : NULL;
}

static int prepare(JNIEnv *e, jobject binder, const char *id, uint64_t generation,
                    jobject *data, jobject *reply) {
    *data = *reply = NULL;
    if (!binder || !valid_name(id) || generation == 0 || generation > INT64_MAX) return 0;
    jmethodID describe = method(e, "android/os/IBinder", "getInterfaceDescriptor", "()Ljava/lang/String;");
    if (!describe) return 0;
    jstring descriptor = (*e)->CallObjectMethod(e, binder, describe);
    char actual[sizeof(INGRESS_DESCRIPTOR)] = {0};
    if (!ascii(e, descriptor, actual, sizeof(actual)) || strcmp(actual, INGRESS_DESCRIPTOR)) return 0;
    *data = new_parcel(e);
    if (!*data) return 0;
    *reply = new_parcel(e);
    if (!*reply) return 0;
    jmethodID token = method(e, "android/os/Parcel", "writeInterfaceToken", "(Ljava/lang/String;)V");
    jmethodID name = method(e, "android/os/Parcel", "writeString", "(Ljava/lang/String;)V");
    jmethodID number = method(e, "android/os/Parcel", "writeLong", "(J)V");
    if (!token || !name || !number) return 0;
    jstring text = (*e)->NewStringUTF(e, id);
    if (!text) return 0;
    (*e)->CallVoidMethod(e, *data, token, descriptor);
    if (!(*e)->ExceptionCheck(e)) (*e)->CallVoidMethod(e, *data, name, text);
    if (!(*e)->ExceptionCheck(e)) (*e)->CallVoidMethod(e, *data, number, (jlong)generation);
    return !(*e)->ExceptionCheck(e);
}

static int transact(JNIEnv *e, jobject binder, jint code, jobject data, jobject reply) {
    jmethodID send = method(e, "android/os/IBinder", "transact", "(ILandroid/os/Parcel;Landroid/os/Parcel;I)Z");
    return send && (*e)->CallBooleanMethod(e, binder, send, code, data, reply, 0) && !(*e)->ExceptionCheck(e);
}

static jobject read_descriptor(JNIEnv *e, jobject data) {
    jclass type = (*e)->FindClass(e, "android/os/ParcelFileDescriptor");
    if (!type) return NULL;
    jfieldID field = (*e)->GetStaticFieldID(e, type, "CREATOR", "Landroid/os/Parcelable$Creator;");
    if (!field) return NULL;
    jobject creator = (*e)->GetStaticObjectField(e, type, field);
    if (!creator || (*e)->ExceptionCheck(e)) return NULL;
    jmethodID read = method(e, "android/os/Parcelable$Creator", "createFromParcel", "(Landroid/os/Parcel;)Ljava/lang/Object;");
    jobject descriptor = read ? (*e)->CallObjectMethod(e, creator, read, data) : NULL;
    return descriptor && !(*e)->ExceptionCheck(e) && (*e)->IsInstanceOf(e, descriptor, type) ? descriptor : NULL;
}

int ib_shizuku_ingress_begin_remote(JNIEnv *e, jobject binder, const char *id, uint64_t generation,
    char authorized_url[IB_INGRESS_URL_CAP + 1U], int *write_fd) {
    if (write_fd) *write_fd = -1;
    if (authorized_url) authorized_url[0] = '\0';
    if (!e || !write_fd || !authorized_url) return -1;
    jobject data = NULL, reply = NULL, descriptor = NULL;
    int fd = -1, ok = prepare(e, binder, id, generation, &data, &reply);
    ok = ok && transact(e, binder, IB_SHIZUKU_INGRESS_BEGIN, data, reply) && bounded_reply(e, reply, 8192);
    jmethodID read = method(e, "android/os/Parcel", "readInt", "()I");
    if (ok && read) ok = (*e)->CallIntMethod(e, reply, read) == 0 && !(*e)->ExceptionCheck(e);
    else ok = 0;
    jmethodID text = method(e, "android/os/Parcel", "readString", "()Ljava/lang/String;");
    jstring url = ok && text ? (*e)->CallObjectMethod(e, reply, text) : NULL;
    ok = ok && ascii(e, url, authorized_url, IB_INGRESS_URL_CAP + 1U);
    ok = ok && (!strncmp(authorized_url, "https://", 8) || !strncmp(authorized_url, "http://", 7));
    if (ok) descriptor = read_descriptor(e, reply);
    ok = ok && descriptor && !(*e)->ExceptionCheck(e) && consumed(e, reply);
    if (ok) {
        jmethodID detach = method(e, "android/os/ParcelFileDescriptor", "detachFd", "()I");
        if (detach) fd = (*e)->CallIntMethod(e, descriptor, detach);
        ok = detach && !(*e)->ExceptionCheck(e) && fd >= 0;
    }
    if (ok) {
        struct stat info;
        int flags = fcntl(fd, F_GETFL);
        ok = flags >= 0 && (flags & O_ACCMODE) == O_WRONLY && fstat(fd, &info) == 0 &&
            S_ISFIFO(info.st_mode) && fcntl(fd, F_SETFD, FD_CLOEXEC) == 0;
    }
    recycle(e, data, reply, descriptor);
    if ((*e)->ExceptionCheck(e)) { (*e)->ExceptionClear(e); ok = 0; }
    if (!ok) { if (fd >= 0) close(fd); authorized_url[0] = '\0'; return -1; }
    *write_fd = fd;
    return 0;
}

int ib_shizuku_ingress_finish_remote(JNIEnv *e, jobject binder, const char *id,
                                     uint64_t generation, int exit_status) {
    if (!e || exit_status < 0 || exit_status > 255) return -1;
    jobject data = NULL, reply = NULL;
    int ok = prepare(e, binder, id, generation, &data, &reply);
    ok = ok && write_integer(e, data, exit_status) &&
        transact(e, binder, IB_SHIZUKU_INGRESS_FINISH, data, reply) && bounded_reply(e, reply, 4);
    jmethodID read = method(e, "android/os/Parcel", "readInt", "()I");
    ok = ok && read && (*e)->CallIntMethod(e, reply, read) == 0 &&
        !(*e)->ExceptionCheck(e) && consumed(e, reply) && !has_descriptors(e, reply);
    recycle(e, data, reply, NULL);
    if ((*e)->ExceptionCheck(e)) { (*e)->ExceptionClear(e); ok = 0; }
    return ok ? 0 : -1;
}
#endif
