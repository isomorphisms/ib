/* Android process/file/JNI adapter. Checked Idriç owns URL policy, extraction,
 * layout and the decision to acknowledge a presented document. */
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include "ib_shizuku_client.h"
#include "ib_shizuku_worker.h"
#include "ib_shizuku_ingress_binder.h"
#include "ib_shizuku_arena_publish.h"
#include "ib_arena_source_owner.h"
#include "appendfat_arena.h"
#include "ib_reader_index.h"

#include <android/binder_ibinder_jni.h>
#include <android/binder_status.h>
#include <android/log.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <jni.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <unistd.h>

#define BODY_VIEW 2130903040
#define INITIAL_ARENA_BYTES (4U * 1024U * 1024U)
#define AVAILABLE_RESERVE_BYTES (8U * 1024U * 1024U)

typedef struct {
    atomic_uint references;
    uint32_t navigation, source_generation;
    uint64_t epoch;
    int retained, result;
    char id[IB_READER_REQUEST_ID_CAP + 1U], url[2049];
    ib_reader_index index;
    ib_reader_source source;
    ib_reader_source_ticket ticket;
    ib_arena_source_observations observations;
} reader_job;

static struct {
    pthread_mutex_t mutex;
    int ready;
    uint32_t navigation;
    char root[4096];
    jobject context;
    jweak activity;
    jclass generated, acquire_task, completion_task;
    jmethodID ready_callback, error_callback;
    reader_job *active;
    ib_ingress_capability *capability;
    uint64_t capability_epoch;
    ib_ingress_arena arena;
    ib_arena_source_owner sources;
} application = { .mutex = PTHREAD_MUTEX_INITIALIZER };

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
    int ok = 1;
    for (jsize i = 0; i < size; ++i) {
        if (units[i] < 33 || units[i] > 126) { ok = 0; break; }
        out[i] = (char)units[i];
    }
    (*e)->ReleaseStringChars(e, text, units);
    if (ok) out[size] = 0;
    return ok;
}

static int random_identity(char out[IB_READER_REQUEST_ID_CAP + 1U]) {
    unsigned char bytes[24];
    size_t used = 0;
    while (used < sizeof(bytes)) {
        ssize_t count = getrandom(bytes + used, sizeof(bytes) - used, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return -1;
        used += (size_t)count;
    }
    static const char hex[] = "0123456789abcdef";
    for (unsigned i = 0; i < sizeof(bytes); ++i) {
        out[2*i] = hex[bytes[i] >> 4];
        out[2*i+1] = hex[bytes[i] & 15];
    }
    out[48] = 0;
    return 0;
}

static int private_directory(int fd) {
    struct stat info;
    return fd >= 0 && fstat(fd, &info) == 0 && S_ISDIR(info.st_mode) &&
        info.st_uid == geteuid() && (info.st_mode & 077U) == 0;
}

static int application_directory(int fd) {
    struct stat info;
    /* Android can give files/ group access for the application's own GID.
     * The newly selected Arena itself must still be strictly private. */
    return fd >= 0 && fstat(fd, &info) == 0 && S_ISDIR(info.st_mode) &&
        info.st_uid == geteuid() && (info.st_mode & 0002U) == 0 &&
        ((info.st_mode & 0020U) == 0 || info.st_gid == getegid());
}

static int initialize_arena(const char *files, char root[4096]) {
    int files_fd = open(files, O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
    if (!application_directory(files_fd)) { if (files_fd >= 0) close(files_fd); return -1; }
    int count = snprintf(root, 4096, "%s/arena", files);
    if (count < 1 || count >= 4096) { close(files_fd); return -1; }
    int existing = openat(files_fd, "arena", O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
    if (existing >= 0) {
        int ok = private_directory(existing);
        close(existing); close(files_fd);
        return ok ? 0 : -1;
    }
    if (errno != ENOENT) { close(files_fd); return -1; }
    struct statvfs space;
    if (fstatvfs(files_fd, &space) != 0 || !space.f_frsize ||
        space.f_bavail < (INITIAL_ARENA_BYTES + AVAILABLE_RESERVE_BYTES) / space.f_frsize + 1U) {
        close(files_fd); return -1;
    }
    char identity[IB_READER_REQUEST_ID_CAP + 1U], leaf[80], temporary[4096], arena[4096];
    if (random_identity(identity) != 0) { close(files_fd); return -1; }
    int a = snprintf(leaf, sizeof(leaf), "arena-initializing-%s", identity);
    int b = snprintf(temporary, sizeof(temporary), "%s/%s", files, leaf);
    int c = snprintf(arena, sizeof(arena), "%s/responses.arena", temporary);
    int ok = a > 0 && (size_t)a < sizeof(leaf) && b > 0 && (size_t)b < sizeof(temporary) &&
        c > 0 && (size_t)c < sizeof(arena) && mkdirat(files_fd, leaf, 0700) == 0;
    int directory = ok ? openat(files_fd, leaf, O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW) : -1;
    ok = ok && private_directory(directory) && mkdirat(directory, "receipts", 0700) == 0 &&
        mkdirat(directory, "locks", 0700) == 0 && appendfat_arena_create(arena, INITIAL_ARENA_BYTES) == 0 &&
        fsync(directory) == 0;
    /* Never replace an existing source directory, even if another initializer
     * won the race. An interrupted unselected directory is retained for audit. */
    if (ok) ok = syscall(SYS_renameat2, files_fd, leaf, files_fd, "arena", 1U) == 0 && fsync(files_fd) == 0;
    if (directory >= 0) close(directory);
    close(files_fd);
    return ok ? 0 : -1;
}

static void job_retain(reader_job *job) { atomic_fetch_add(&job->references, 1); }
static void job_release(reader_job *job) {
    if (job && atomic_fetch_sub(&job->references, 1) == 1) {
        ib_reader_source_clear(&job->source);
        free(job);
    }
}
static void *job_created(void *opaque) { return opaque; }
static void job_destroyed(void *opaque) { job_release(opaque); }
static binder_status_t job_refuses(AIBinder *self, transaction_code_t code, const AParcel *in, AParcel *out) {
    (void)self; (void)code; (void)in; (void)out; return STATUS_UNKNOWN_TRANSACTION;
}
static pthread_once_t job_once = PTHREAD_ONCE_INIT;
static AIBinder_Class *job_class;
static void initialize_job_class(void) {
    job_class = AIBinder_Class_define("org.isomorphisms.ib.ReaderJobStorage.v1", job_created, job_destroyed, job_refuses);
}

static jobject task(JNIEnv *e, reader_job *job, jclass type) {
    (void)pthread_once(&job_once, initialize_job_class);
    if (!job_class || (*e)->ExceptionCheck(e)) return NULL;
    jmethodID constructor = (*e)->GetMethodID(e, type, "<init>", "()V");
    jfieldID field = constructor ? (*e)->GetFieldID(e, type, "storageAnchor", "Landroid/os/IBinder;") : NULL;
    jobject runnable = field ? (*e)->NewObject(e, type, constructor) : NULL;
    if (!runnable) return NULL;
    job_retain(job);
    AIBinder *anchor = AIBinder_new(job_class, job);
    if (!anchor) { job_release(job); return NULL; }
    jobject java_anchor = AIBinder_toJavaBinder(e, anchor);
    if (java_anchor && !(*e)->ExceptionCheck(e)) (*e)->SetObjectField(e, runnable, field, java_anchor);
    AIBinder_decStrong(anchor);
    return java_anchor && !(*e)->ExceptionCheck(e) ? runnable : NULL;
}

static AIBinder *task_anchor(JNIEnv *e, jobject self) {
    jclass type = (*e)->GetObjectClass(e, self);
    jfieldID field = type ? (*e)->GetFieldID(e, type, "storageAnchor", "Landroid/os/IBinder;") : NULL;
    jobject value = field ? (*e)->GetObjectField(e, self, field) : NULL;
    AIBinder *anchor = value && !(*e)->ExceptionCheck(e) ? AIBinder_fromJavaBinder(e, value) : NULL;
    if (anchor && AIBinder_getClass(anchor) != job_class) { AIBinder_decStrong(anchor); return NULL; }
    return anchor;
}

static void authority_revoked(void *owner, uint64_t through) {
    (void)owner;
    pthread_mutex_lock(&application.mutex);
    if (application.capability && application.capability_epoch <= through)
        ib_ingress_revoke(ib_ingress_capability_session(application.capability));
    pthread_mutex_unlock(&application.mutex);
}

static int start_job(JNIEnv *e, reader_job *job) {
    jobject runnable = task(e, job, application.acquire_task);
    jclass type = (*e)->FindClass(e, "java/lang/Thread");
    jmethodID constructor = type ? (*e)->GetMethodID(e, type, "<init>", "(Ljava/lang/Runnable;)V") : NULL;
    jobject thread = runnable && constructor ? (*e)->NewObject(e, type, constructor, runnable) : NULL;
    jmethodID start = method(e, "java/lang/Thread", "start", "()V");
    if (thread && start) (*e)->CallVoidMethod(e, thread, start);
    return thread && start && !(*e)->ExceptionCheck(e) ? 0 : -1;
}

/* The index stores opaque Idriç history references only; Arena owns source
 * identity, admission and all original bytes. */
static int last_source(reader_job *job, unsigned first) {
    int root = open(application.root, O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
    int result = root >= 0 ? ib_reader_index_read(root, first, &job->index) : -1;
    if (root >= 0) close(root);
    if (result == 0) {
        strcpy(job->id, job->index.source_id);
        job->source_generation = job->index.source_generation;
    }
    return result;
}

static int journal_text(JNIEnv *e, jstring text, char out[IB_READER_JOURNAL_CAP + 1U]) {
    if (!text || (*e)->ExceptionCheck(e)) return 0;
    jsize size = (*e)->GetStringLength(e, text);
    if (size < 1 || (size_t)size > IB_READER_JOURNAL_CAP) return 0;
    const jchar *units = (*e)->GetStringChars(e, text, NULL);
    if (!units) return 0;
    int valid = 1;
    for (jsize i = 0; i < size; ++i) {
        if (units[i] != '\t' && units[i] != '\n' && (units[i] < 32 || units[i] > 126)) { valid = 0; break; }
        out[i] = (char)units[i];
    }
    (*e)->ReleaseStringChars(e, text, units);
    if (valid) out[size] = 0;
    return valid;
}

JNIEXPORT jstring JNICALL
Java_org_isomorphisms_ib_ReaderBridge_history(JNIEnv *e, jclass type) {
    (void)type;
    ib_reader_index index;
    int root = open(application.root, O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
    int result = root >= 0 ? ib_reader_index_read(root, 0, &index) : -1;
    if (root >= 0) close(root);
    return (*e)->NewStringUTF(e, result == 0 ? index.journal : result == 1 ? "" : "INVALID\n");
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderBridge_acknowledge(JNIEnv *e, jclass type, jstring id_text,
                                                 jint navigation, jstring journal) {
    (void)type;
    char id[65] = {0};
    ib_reader_index index = {0};
    if (!ascii(e, id_text, id, sizeof(id)) || !journal_text(e, journal, index.journal)) return -1;
    pthread_mutex_lock(&application.mutex);
    reader_job *job = application.active;
    int ok = job && job->result == 0 && job->navigation == (uint32_t)navigation && !strcmp(job->id, id) &&
        application.navigation == job->navigation;
    if (ok) {
        strcpy(index.source_id, job->id);
        index.source_generation = job->source_generation;
        index.previous = job->index.previous;
        int root = open(application.root, O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
        ok = root >= 0 && ib_reader_index_publish(root, &index) == 0;
        if (root >= 0) close(root);
    }
    pthread_mutex_unlock(&application.mutex);
    return ok ? 0 : -1;
}

static int enqueue(JNIEnv *e, jstring url_text, int retained, jstring id, jint generation, jstring journal) {
    reader_job *job = calloc(1, sizeof(*job));
    if (!job) return -1;
    atomic_init(&job->references, 1);
    ib_reader_source_init(&job->source);
    job->retained = retained;
    int ok = retained == 1 ? last_source(job, 0) == 0 : retained == 2 ?
        generation > 0 && ascii(e, id, job->id, sizeof(job->id)) &&
        journal_text(e, journal, job->index.journal) :
        ascii(e, url_text, job->url, sizeof(job->url)) && random_identity(job->id) == 0;
    if (retained == 2) job->source_generation = (uint32_t)generation;
    pthread_mutex_lock(&application.mutex);
    ok = ok && application.ready && !application.active && application.navigation < INT32_MAX;
    if (ok) {
        job->navigation = ++application.navigation;
        if (!retained) job->source_generation = job->navigation;
        application.active = job;
        ib_arena_source_owner_want(&application.sources, job->navigation);
        ib_reader_source_want(&job->source, job->navigation);
    }
    pthread_mutex_unlock(&application.mutex);
    if (ok && start_job(e, job) == 0) return 0;
    pthread_mutex_lock(&application.mutex);
    if (application.active == job) application.active = NULL;
    pthread_mutex_unlock(&application.mutex);
    job_release(job);
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    return -1;
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderBridge_navigate(JNIEnv *e, jclass type, jstring url) {
    (void)type; return enqueue(e, url, 0, NULL, 0, NULL);
}
JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderBridge_restore(JNIEnv *e, jclass type) {
    (void)type; return enqueue(e, NULL, 1, NULL, 0, NULL);
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderBridge_openRetained(JNIEnv *e, jclass type, jstring id,
                                                  jint generation, jstring journal) {
    (void)type; return enqueue(e, NULL, 2, id, generation, journal);
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderBridge_attach(JNIEnv *e, jclass type, jobject activity) {
    (void)type;
    if (!activity || getuid() < 10000 || (*e)->PushLocalFrame(e, 64) != 0) return -1;
    pthread_mutex_lock(&application.mutex);
    int ready = application.ready;
    pthread_mutex_unlock(&application.mutex);
    int ok = 1;
    if (!ready) {
        jmethodID get_context = method(e, "android/content/Context", "getApplicationContext", "()Landroid/content/Context;");
        jobject context = get_context ? (*e)->CallObjectMethod(e, activity, get_context) : NULL;
        jmethodID get_files = method(e, "android/content/Context", "getFilesDir", "()Ljava/io/File;");
        jobject files = context && get_files ? (*e)->CallObjectMethod(e, context, get_files) : NULL;
        jmethodID path = method(e, "java/io/File", "getCanonicalPath", "()Ljava/lang/String;");
        jstring text = files && path ? (*e)->CallObjectMethod(e, files, path) : NULL;
        char directory[4096];
        ok = ascii(e, text, directory, sizeof(directory)) && initialize_arena(directory, application.root) == 0 &&
            ib_ingress_arena_init(&application.arena, application.root) == 0 &&
            ib_arena_source_owner_init(&application.sources, application.root) == 0;
        jclass generated = ok ? (*e)->FindClass(e, "Idric/Generated") : NULL;
        jclass acquire = generated ? (*e)->FindClass(e, "org/isomorphisms/ib/ReaderAcquireTask") : NULL;
        jclass completion = acquire ? (*e)->FindClass(e, "org/isomorphisms/ib/ReaderCompletionTask") : NULL;
        application.ready_callback = completion ? (*e)->GetStaticMethodID(e, generated, "reader_acquisition_ready",
            "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;I)V") : NULL;
        application.error_callback = application.ready_callback ? (*e)->GetStaticMethodID(e, generated, "reader_acquisition_failed",
            "(Landroid/app/Activity;Ljava/lang/String;)V") : NULL;
        ok = ok && application.error_callback && !(*e)->ExceptionCheck(e);
        if (ok) {
            application.context = (*e)->NewGlobalRef(e, context);
            application.generated = (*e)->NewGlobalRef(e, generated);
            application.acquire_task = (*e)->NewGlobalRef(e, acquire);
            application.completion_task = (*e)->NewGlobalRef(e, completion);
            ok = application.context && application.generated && application.acquire_task && application.completion_task && !(*e)->ExceptionCheck(e);
        }
        if (ok) ib_shizuku_client_set_revocation_handler(authority_revoked, NULL);
    }
    jweak weak = ok ? (*e)->NewWeakGlobalRef(e, activity) : NULL;
    ok = ok && weak && !(*e)->ExceptionCheck(e);
    if (ok) {
        pthread_mutex_lock(&application.mutex);
        jweak old = application.activity;
        application.activity = weak;
        application.ready = 1;
        pthread_mutex_unlock(&application.mutex);
        if (old) (*e)->DeleteWeakGlobalRef(e, old);
        jclass listener_type = (*e)->FindClass(e, "org/isomorphisms/ib/ReaderLayoutListener");
        jmethodID constructor = listener_type ? (*e)->GetMethodID(e, listener_type, "<init>", "()V") : NULL;
        jobject listener = constructor ? (*e)->NewObject(e, listener_type, constructor) : NULL;
        jmethodID find = method(e, "android/app/Activity", "findViewById", "(I)Landroid/view/View;");
        jobject body = find ? (*e)->CallObjectMethod(e, activity, find, BODY_VIEW) : NULL;
        jmethodID add = method(e, "android/view/View", "addOnLayoutChangeListener", "(Landroid/view/View$OnLayoutChangeListener;)V");
        if (body && listener && add) (*e)->CallVoidMethod(e, body, add, listener);
        ok = body && listener && add && !(*e)->ExceptionCheck(e);
    }
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    (*e)->PopLocalFrame(e, NULL);
    return ok ? 0 : -1;
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_ReaderLayoutListener_onLayoutChange(JNIEnv *e, jobject self, jobject view,
    jint left, jint top, jint right, jint bottom, jint old_left, jint old_top, jint old_right, jint old_bottom) {
    (void)self; (void)top; (void)bottom; (void)old_top; (void)old_bottom;
    if (right <= left || right - left == old_right - old_left) return;
    jmethodID context = method(e, "android/view/View", "getContext", "()Landroid/content/Context;");
    jobject activity = context ? (*e)->CallObjectMethod(e, view, context) : NULL;
    pthread_mutex_lock(&application.mutex);
    int current = activity && application.activity && (*e)->IsSameObject(e, activity, application.activity) && !application.active;
    pthread_mutex_unlock(&application.mutex);
    if (current) (void)enqueue(e, NULL, 1, NULL, 0, NULL);
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_ReaderAcquireTask_run(JNIEnv *e, jobject self) {
    if ((*e)->PushLocalFrame(e, 128) != 0) return;
    AIBinder *anchor = task_anchor(e, self);
    reader_job *job = anchor ? AIBinder_getUserData(anchor) : NULL;
    if (!job) { if (anchor) AIBinder_decStrong(anchor); (*e)->PopLocalFrame(e, NULL); return; }
    job->result = -1;
    int acquired = job->retained;
    if (!acquired) {
        uid_t uid;
        uint64_t epoch;
        if (ib_shizuku_client_authorization(e, &uid, &epoch) == IB_SHIZUKU_AUTHORIZED && uid == 2000) {
            ib_ingress_capability *capability = ib_ingress_capability_new(ib_ingress_arena_publish_frame, &application.arena);
            ib_shizuku_arena_ingress *session = ib_ingress_capability_session(capability);
            int armed = session && ib_ingress_arm(session, job->id, job->navigation, job->url, uid) == 0;
            jobject ingress = armed ? ib_shizuku_ingress_binder_new(e, capability) : NULL;
            if (ingress) {
                pthread_mutex_lock(&application.mutex);
                application.capability = capability;
                application.capability_epoch = epoch;
                pthread_mutex_unlock(&application.mutex);
                acquired = ib_shizuku_worker_acquire(e, application.context, ingress, job->id, job->navigation, epoch) == 0;
                pthread_mutex_lock(&application.mutex);
                application.capability = NULL;
                pthread_mutex_unlock(&application.mutex);
            }
            if (session) ib_ingress_revoke(session);
            if (capability) ib_ingress_capability_release(capability);
        }
    }
    int fd = -1, open_status = -1, stream_status = -1;
    if (acquired) open_status = ib_arena_source_owner_open_decode_input(&application.sources, job->id,
        job->source_generation, job->navigation, &job->ticket, &job->observations, &fd);
    if (open_status == 0) {
        stream_status = ib_reader_source_accept_fd(&job->source, &job->ticket, fd);
        job->result = stream_status == IB_READER_PFD_ACCEPTED ? 0 : -1;
        close(fd);
    }
    if (job->retained == 1 && job->result != 0 && last_source(job, 1) == 0 &&
        ib_arena_source_owner_open_decode_input(&application.sources, job->id,
            job->source_generation, job->navigation, &job->ticket, &job->observations, &fd) == 0) {
        job->result = ib_reader_source_accept_fd(&job->source, &job->ticket, fd) == IB_READER_PFD_ACCEPTED ? 0 : -1;
        close(fd);
    }
    if ((*e)->ExceptionCheck(e)) { (*e)->ExceptionClear(e); job->result = -1; }
    __android_log_print(ANDROID_LOG_INFO, "IBReader", "committed source acquired=%d open=%d stream=%d result=%d bytes=%zu navigation=%u source_generation=%u",
        acquired, open_status, stream_status, job->result, job->source.byte_count, job->navigation, job->source_generation);
    jobject completion = task(e, job, application.completion_task);
    pthread_mutex_lock(&application.mutex);
    jobject activity = application.activity ? (*e)->NewLocalRef(e, application.activity) : NULL;
    pthread_mutex_unlock(&application.mutex);
    jmethodID post = method(e, "android/app/Activity", "runOnUiThread", "(Ljava/lang/Runnable;)V");
    int posted = activity && completion && post;
    if (posted) (*e)->CallVoidMethod(e, activity, post, completion);
    __android_log_print(ANDROID_LOG_INFO, "IBReader", "source completion posted=%d exception=%d", posted, (*e)->ExceptionCheck(e));
    if (!posted || (*e)->ExceptionCheck(e)) {
        if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
        pthread_mutex_lock(&application.mutex);
        if (application.active == job) application.active = NULL;
        pthread_mutex_unlock(&application.mutex);
        job_release(job);
    }
    AIBinder_decStrong(anchor);
    (*e)->PopLocalFrame(e, NULL);
}

static jstring source_text(JNIEnv *e, const unsigned char *bytes, size_t count, int utf8) {
    if (count > IB_READER_SOURCE_MAX_BYTES || count > INT32_MAX) return NULL;
    jbyteArray array = (*e)->NewByteArray(e, (jsize)count);
    if (!array) return NULL;
    (*e)->SetByteArrayRegion(e, array, 0, (jsize)count, (const jbyte *)bytes);
    if (!utf8) {
        jclass type = (*e)->FindClass(e, "java/lang/String");
        jmethodID constructor = type ? (*e)->GetMethodID(e, type, "<init>", "([BLjava/lang/String;)V") : NULL;
        jstring charset = constructor ? (*e)->NewStringUTF(e, "ISO-8859-1") : NULL;
        /* A one-octet/one-scalar carrier, never HTML charset inference. The
         * checked Idriç decoder interprets these original octets and admits
         * the derivative against the unchanged response observations. */
        return charset && !(*e)->ExceptionCheck(e) ? (*e)->NewObject(e, type, constructor, array, charset) : NULL;
    }
    jclass charset_type = (*e)->FindClass(e, "java/nio/charset/Charset");
    jmethodID for_name = charset_type ? (*e)->GetStaticMethodID(e, charset_type, "forName", "(Ljava/lang/String;)Ljava/nio/charset/Charset;") : NULL;
    jstring encoding = for_name ? (*e)->NewStringUTF(e, "UTF-8") : NULL;
    jobject charset = encoding ? (*e)->CallStaticObjectMethod(e, charset_type, for_name, encoding) : NULL;
    jmethodID new_decoder = method(e, "java/nio/charset/Charset", "newDecoder", "()Ljava/nio/charset/CharsetDecoder;");
    jobject decoder = charset && new_decoder ? (*e)->CallObjectMethod(e, charset, new_decoder) : NULL;
    jclass buffer_type = (*e)->FindClass(e, "java/nio/ByteBuffer");
    jmethodID wrap = buffer_type ? (*e)->GetStaticMethodID(e, buffer_type, "wrap", "([B)Ljava/nio/ByteBuffer;") : NULL;
    jobject buffer = wrap ? (*e)->CallStaticObjectMethod(e, buffer_type, wrap, array) : NULL;
    /* CharsetDecoder defaults to REPORT for malformed/unmappable input. Do
     * not use String's replacement decoder even when a receipt says valid. */
    jmethodID decode = method(e, "java/nio/charset/CharsetDecoder", "decode", "(Ljava/nio/ByteBuffer;)Ljava/nio/CharBuffer;");
    jobject decoded = decoder && buffer && decode ? (*e)->CallObjectMethod(e, decoder, decode, buffer) : NULL;
    jmethodID text = method(e, "java/nio/CharBuffer", "toString", "()Ljava/lang/String;");
    return decoded && text && !(*e)->ExceptionCheck(e) ? (*e)->CallObjectMethod(e, decoded, text) : NULL;
}

/* Record only exception types and generated method names. Never log Throwable
 * messages, source text, URLs or request/capability values. This diagnostic
 * consumes the pending exception before the existing recoverable error path. */
static void log_reader_exception(JNIEnv *e) {
    jthrowable failure = (*e)->ExceptionOccurred(e);
    if (!failure) return;
    (*e)->ExceptionClear(e);
    if ((*e)->PushLocalFrame(e, 24) != 0) {
        (*e)->ExceptionClear(e);
        (*e)->DeleteLocalRef(e, failure);
        return;
    }
    jclass failure_type = (*e)->GetObjectClass(e, failure);
    jclass class_type = (*e)->FindClass(e, "java/lang/Class");
    jmethodID class_name = class_type ? (*e)->GetMethodID(e, class_type, "getName", "()Ljava/lang/String;") : NULL;
    jstring name = class_name ? (*e)->CallObjectMethod(e, failure_type, class_name) : NULL;
    const char *type = name && !(*e)->ExceptionCheck(e) ? (*e)->GetStringUTFChars(e, name, NULL) : NULL;
    if (type) {
        __android_log_print(ANDROID_LOG_ERROR, "IBReader", "checked renderer exception type=%.192s", type);
        (*e)->ReleaseStringUTFChars(e, name, type);
    }
    if (!(*e)->ExceptionCheck(e)) {
        jmethodID stack = method(e, "java/lang/Throwable", "getStackTrace", "()[Ljava/lang/StackTraceElement;");
        jobjectArray frames = stack ? (*e)->CallObjectMethod(e, failure, stack) : NULL;
        jmethodID owner = method(e, "java/lang/StackTraceElement", "getClassName", "()Ljava/lang/String;");
        jmethodID member = method(e, "java/lang/StackTraceElement", "getMethodName", "()Ljava/lang/String;");
        if (frames && owner && member && !(*e)->ExceptionCheck(e)) {
            jsize count = (*e)->GetArrayLength(e, frames);
            if (count > 12) count = 12;
            for (jsize i = 0; i < count && !(*e)->ExceptionCheck(e); ++i) {
                if ((*e)->PushLocalFrame(e, 8) != 0) break;
                jobject frame = (*e)->GetObjectArrayElement(e, frames, i);
                jstring owner_name = frame ? (*e)->CallObjectMethod(e, frame, owner) : NULL;
                jstring member_name = frame && !(*e)->ExceptionCheck(e) ? (*e)->CallObjectMethod(e, frame, member) : NULL;
                const char *owner_text = owner_name && !(*e)->ExceptionCheck(e) ? (*e)->GetStringUTFChars(e, owner_name, NULL) : NULL;
                const char *member_text = member_name && !(*e)->ExceptionCheck(e) ? (*e)->GetStringUTFChars(e, member_name, NULL) : NULL;
                if (owner_text && member_text)
                    __android_log_print(ANDROID_LOG_ERROR, "IBReader", "checked renderer frame=%d %.192s.%.192s", i, owner_text, member_text);
                if (member_text) (*e)->ReleaseStringUTFChars(e, member_name, member_text);
                if (owner_text) (*e)->ReleaseStringUTFChars(e, owner_name, owner_text);
                (*e)->PopLocalFrame(e, NULL);
            }
        }
    }
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    (*e)->PopLocalFrame(e, NULL);
    (*e)->DeleteLocalRef(e, failure);
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_ReaderCompletionTask_run(JNIEnv *e, jobject self) {
    if ((*e)->PushLocalFrame(e, 64) != 0) return;
    AIBinder *anchor = task_anchor(e, self);
    reader_job *job = anchor ? AIBinder_getUserData(anchor) : NULL;
    pthread_mutex_lock(&application.mutex);
    int current = job && application.active == job && application.navigation == job->navigation;
    jobject activity = current && application.activity ? (*e)->NewLocalRef(e, application.activity) : NULL;
    pthread_mutex_unlock(&application.mutex);
    __android_log_print(ANDROID_LOG_INFO, "IBReader", "source completion current=%d activity=%d result=%d bytes=%zu",
        current, activity != NULL, job ? job->result : -1, job ? job->source.byte_count : 0U);
    if (current && activity && job->result == 0) {
        char fields[5000], observed[65];
        ib_reader_sha256_hex(job->source.bytes, job->source.byte_count, observed);
        int count = snprintf(fields, sizeof(fields), "%s\t%u\t%s\t%s\t%u\t%zu\t%s\t%s\t%s\t%s\t%s\t%s\tcomplete",
            job->observations.source_key, job->source_generation, job->ticket.sha256_hex, observed,
            job->ticket.byte_count, job->source.byte_count, job->observations.requested_url,
            job->observations.resolved_url, job->observations.mime_type, job->observations.charset, job->observations.http_status,
            job->observations.utf8_valid ? "verified-utf8" : "invalid-utf8");
        jstring evidence = count > 0 && (size_t)count < sizeof(fields) ? (*e)->NewStringUTF(e, fields) : NULL;
        int utf8 = job->observations.utf8_valid &&
            (strcmp(job->observations.charset, "utf-8") == 0 || strcmp(job->observations.charset, "UTF-8") == 0);
        jstring body = evidence ? source_text(e, job->source.bytes, job->source.byte_count, utf8) : NULL;
        jstring id = body ? (*e)->NewStringUTF(e, job->id) : NULL;
        jstring journal = id ? (*e)->NewStringUTF(e, job->index.journal) : NULL;
        if (journal) {
            __android_log_print(ANDROID_LOG_INFO, "IBReader", "checked renderer callback begin navigation=%u source_generation=%u", job->navigation, job->source_generation);
            (*e)->CallStaticVoidMethod(e, application.generated, application.ready_callback,
                activity, evidence, body, id, (jint)job->navigation, journal, (jint)job->retained);
            __android_log_print(ANDROID_LOG_INFO, "IBReader", "checked renderer callback returned exception=%d", (*e)->ExceptionCheck(e));
        }
        if (!journal || (*e)->ExceptionCheck(e)) {
            log_reader_exception(e);
            jstring reason = (*e)->NewStringUTF(e, "The checked renderer failed; the last committed readable state is retained.");
            if (reason) (*e)->CallStaticVoidMethod(e, application.generated, application.error_callback, activity, reason);
            if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
        }
    } else if (current && activity) {
        jstring reason = (*e)->NewStringUTF(e, "Acquisition or verified source readback failed. The previous document is retained.");
        if (reason) (*e)->CallStaticVoidMethod(e, application.generated, application.error_callback, activity, reason);
    }
    pthread_mutex_lock(&application.mutex);
    if (current && application.active == job) application.active = NULL;
    pthread_mutex_unlock(&application.mutex);
    if (current) job_release(job);
    if (anchor) AIBinder_decStrong(anchor);
    (*e)->PopLocalFrame(e, NULL);
}

JNIEXPORT jobject JNICALL
Java_org_isomorphisms_ib_ReaderBridge_metrics(JNIEnv *e, jclass type, jobject activity) {
    (void)type;
    jmethodID find = method(e, "android/app/Activity", "findViewById", "(I)Landroid/view/View;");
    jobject body = find ? (*e)->CallObjectMethod(e, activity, find, BODY_VIEW) : NULL;
    jmethodID get_paint = method(e, "android/widget/TextView", "getPaint", "()Landroid/text/TextPaint;");
    jobject current = body && get_paint ? (*e)->CallObjectMethod(e, body, get_paint) : NULL;
    jclass paint_type = (*e)->FindClass(e, "android/graphics/Paint");
    jmethodID paint_copy = paint_type ? (*e)->GetMethodID(e, paint_type, "<init>", "(Landroid/graphics/Paint;)V") : NULL;
    jobject paint = current && paint_copy ? (*e)->NewObject(e, paint_type, paint_copy, current) : NULL;
    jmethodID get_width = method(e, "android/view/View", "getWidth", "()I");
    jint width = body && get_width ? (*e)->CallIntMethod(e, body, get_width) : 0;
    jmethodID left = method(e, "android/view/View", "getPaddingLeft", "()I");
    jmethodID right = method(e, "android/view/View", "getPaddingRight", "()I");
    if (body && left && right) width -= (*e)->CallIntMethod(e, body, left) + (*e)->CallIntMethod(e, body, right);
    jclass integer_type = (*e)->FindClass(e, "java/lang/Integer");
    jmethodID integer_constructor = integer_type ? (*e)->GetMethodID(e, integer_type, "<init>", "(I)V") : NULL;
    jobject boxed = integer_constructor ? (*e)->NewObject(e, integer_type, integer_constructor,
        width > 0 && width <= INT32_MAX / 64 ? width * 64 : 0) : NULL;
    jclass metrics_type = (*e)->FindClass(e, "org/isomorphisms/ib/ReaderMetrics");
    jmethodID constructor = metrics_type ? (*e)->GetMethodID(e, metrics_type, "<init>", "()V") : NULL;
    jobject metrics = constructor && paint && boxed ? (*e)->NewObject(e, metrics_type, constructor) : NULL;
    jfieldID paint_field = metrics ? (*e)->GetFieldID(e, metrics_type, "paint", "Landroid/graphics/Paint;") : NULL;
    jfieldID width_field = paint_field ? (*e)->GetFieldID(e, metrics_type, "widthValue", "Ljava/lang/Integer;") : NULL;
    if (width_field) { (*e)->SetObjectField(e, metrics, paint_field, paint); (*e)->SetObjectField(e, metrics, width_field, boxed); }
    return width_field && !(*e)->ExceptionCheck(e) ? metrics : NULL;
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderMetrics_width(JNIEnv *e, jobject self) {
    if (!self || (*e)->ExceptionCheck(e)) return 0;
    jclass type = (*e)->GetObjectClass(e, self);
    jfieldID field = type ? (*e)->GetFieldID(e, type, "widthValue", "Ljava/lang/Integer;") : NULL;
    jobject boxed = field ? (*e)->GetObjectField(e, self, field) : NULL;
    jmethodID value = method(e, "java/lang/Integer", "intValue", "()I");
    return boxed && value ? (*e)->CallIntMethod(e, boxed, value) : 0;
}
JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderMetrics_measure(JNIEnv *e, jobject self, jstring text) {
    if (!self || (*e)->ExceptionCheck(e)) return INT32_MAX;
    jclass type = (*e)->GetObjectClass(e, self);
    jfieldID field = type ? (*e)->GetFieldID(e, type, "paint", "Landroid/graphics/Paint;") : NULL;
    jobject paint = field ? (*e)->GetObjectField(e, self, field) : NULL;
    jmethodID measure = method(e, "android/graphics/Paint", "measureText", "(Ljava/lang/String;)F");
    float width = paint && measure && text ? (*e)->CallFloatMethod(e, paint, measure, text) : NAN;
    return !(*e)->ExceptionCheck(e) && isfinite(width) && width >= 0 && width < (float)INT32_MAX / 64.0f ?
        (jint)ceilf(width * 64.0f) : INT32_MAX;
}
