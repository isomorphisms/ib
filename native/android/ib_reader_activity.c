/* Android process/file/JNI adapter. Checked Idriç owns URL policy, extraction,
 * layout and the decision to acknowledge a presented document. */
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include "ib_shizuku_client.h"
#include "ib_shizuku_worker.h"
#include "ib_shizuku_ingress_binder.h"
#include "ib_shizuku_arena_publish.h"
#include "ib_arena_source_owner.h"
#include "ib_browser_dom_source.h"
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
#include <time.h>
#include <unistd.h>

#define BODY_VIEW 2130903040
#define INITIAL_ARENA_BYTES (4U * 1024U * 1024U)
#define AVAILABLE_RESERVE_BYTES (8U * 1024U * 1024U)
#define GOOGLE_PANEL_VIEW 2130903052
#define READER_SCROLL_VIEW 2130903054

typedef struct {
    ib_reader_source source;
    ib_reader_source_ticket ticket;
    ib_arena_source_observations observations;
} reader_target;

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
    int cancelled, is_dom, browser_started, browser_pending, browser_checking, browser_committing, browser_error;
    unsigned browser_polls;
    int64_t browser_deadline;
    unsigned char *pending_dom;
    size_t pending_dom_bytes;
    char pending_dom_url[2049];
    ib_browser_dom_metadata dom;
    reader_target targets[IB_DOM_MAX_TARGETS];
} reader_job;

static struct {
    pthread_mutex_t mutex;
    int ready;
    uint32_t navigation;
    char root[4096];
    jobject context;
    jweak activity;
    jclass generated, acquire_task, completion_task;
    jmethodID ready_callback, error_callback, history_validator;
    jclass google_client, google_dom, google_poll;
    jmethodID dom_ready_callback, browser_needed, browser_allowed, snapshot_ready, candidate_wire, target_admitted, progress_callback;
    jobject browser;
    reader_job *browser_job;
    reader_job *active;
    ib_ingress_capability *capability;
    uint64_t capability_epoch, activity_epoch;
    int restore_after_retirement;
    ib_ingress_arena arena;
    ib_arena_source_owner sources;
} application = { .mutex = PTHREAD_MUTEX_INITIALIZER };

static void browser_cleanup(JNIEnv *e, reader_job *job);
static void browser_cancel_owner(JNIEnv *e, reader_job *job);
static int browser_begin(JNIEnv *e, reader_job *job, jobject activity);
static void browser_snapshot_worker(JNIEnv *e, reader_job *job);
static jstring source_text(JNIEnv *e, const unsigned char *bytes, size_t count, int utf8);

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
        for (unsigned i = 0; i < IB_DOM_MAX_TARGETS; ++i) ib_reader_source_clear(&job->targets[i].source);
        free(job->pending_dom);
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

/* Called with application.mutex held. Provider authorization and Activity
 * ownership are separate generations; neither substitutes for the other. */
static int current_job_locked(const reader_job *job) {
    return job && !job->cancelled && application.active == job && application.navigation == job->navigation &&
        application.activity && job->epoch == application.activity_epoch;
}

static int bind_activity(JNIEnv *e, jobject activity) {
    jweak weak = (*e)->NewWeakGlobalRef(e, activity);
    if (!weak || (*e)->ExceptionCheck(e)) return -1;
    if (application.browser_job &&
        (!application.activity || !(*e)->IsSameObject(e, activity, application.activity)))
        browser_cancel_owner(e, application.browser_job);
    pthread_mutex_lock(&application.mutex);
    if (application.activity && (*e)->IsSameObject(e, activity, application.activity)) {
        pthread_mutex_unlock(&application.mutex);
        (*e)->DeleteWeakGlobalRef(e, weak);
        return 0;
    }
    if (application.activity_epoch == UINT64_MAX) {
        pthread_mutex_unlock(&application.mutex);
        (*e)->DeleteWeakGlobalRef(e, weak);
        return -1;
    }
    jweak previous = application.activity;
    application.activity = weak;
    ++application.activity_epoch;
    application.restore_after_retirement = application.active != NULL;
    if (application.capability)
        ib_ingress_revoke(ib_ingress_capability_session(application.capability));
    pthread_mutex_unlock(&application.mutex);
    if (previous) (*e)->DeleteWeakGlobalRef(e, previous);
    return 1;
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


/* Byte-verified index contents are still untrusted navigation semantics.
 * Invoke the maintained checked owner; never parse an IBH journal in C. */
static int checked_retained_history(JNIEnv *e, const char *id, uint32_t generation,
                                    const char *journal) {
    if (!application.ready || !application.generated || !application.history_validator ||
        !id || !journal || !generation || generation > INT32_MAX ||
        (*e)->ExceptionCheck(e) || (*e)->PushLocalFrame(e, 8) != 0) return 0;
    char generation_text[16];
    int count = snprintf(generation_text, sizeof(generation_text), "%u", generation);
    jstring source = count > 0 && (size_t)count < sizeof(generation_text) ?
        (*e)->NewStringUTF(e, id) : NULL;
    jstring number = source ? (*e)->NewStringUTF(e, generation_text) : NULL;
    jstring history = number ? (*e)->NewStringUTF(e, journal) : NULL;
    jint accepted = history && !(*e)->ExceptionCheck(e) ?
        (*e)->CallStaticIntMethod(e, application.generated, application.history_validator,
            source, number, history) : 0;
    if ((*e)->ExceptionCheck(e)) { (*e)->ExceptionClear(e); accepted = 0; }
    (*e)->PopLocalFrame(e, NULL);
    return accepted == 1;
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
    if (result == 0 && !checked_retained_history(e, index.source_id, index.source_generation, index.journal)) {
        result = !index.previous ? ib_reader_index_read(root, 1, &index) : -1;
        if (result == 0 && !checked_retained_history(e, index.source_id, index.source_generation, index.journal))
            result = -1;
    }
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
        current_job_locked(job);
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
    for (unsigned i = 0; i < IB_DOM_MAX_TARGETS; ++i) ib_reader_source_init(&job->targets[i].source);
    job->retained = retained;
    int ok = retained == 1 ? last_source(job, 0) == 0 : retained == 2 ?
        generation > 0 && ascii(e, id, job->id, sizeof(job->id)) &&
        journal_text(e, journal, job->index.journal) :
        ascii(e, url_text, job->url, sizeof(job->url)) && random_identity(job->id) == 0;
    if (retained == 2) job->source_generation = (uint32_t)generation;
    pthread_mutex_lock(&application.mutex);
    ok = ok && application.ready && application.activity && !application.active &&
        application.navigation < INT32_MAX && application.activity_epoch < UINT64_MAX;
    if (ok) {
        job->navigation = ++application.navigation;
        job->epoch = application.activity_epoch;
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

/* One application-owned reference is retired exactly once, including stale
 * completions. Task Binder anchors retain their own references independently. */
static void retire_job(JNIEnv *e, reader_job *job, int ui_thread) {
    if (ui_thread) browser_cleanup(e, job);
    pthread_mutex_lock(&application.mutex);
    int owned = job && application.active == job;
    int restore = owned && application.restore_after_retirement && application.activity;
    if (owned) {
        application.active = NULL;
        application.restore_after_retirement = 0;
    }
    pthread_mutex_unlock(&application.mutex);
    if (owned) job_release(job);
    if (restore && !(*e)->ExceptionCheck(e) && enqueue(e, NULL, 1, NULL, 0, NULL) != 0) {
        __android_log_print(ANDROID_LOG_INFO, "IBReader", "Activity return has no committed source to restore");
        if (ui_thread) {
            pthread_mutex_lock(&application.mutex);
            jobject activity = application.activity ? (*e)->NewLocalRef(e, application.activity) : NULL;
            pthread_mutex_unlock(&application.mutex);
            jstring reason = activity && !(*e)->ExceptionCheck(e) ? (*e)->NewStringUTF(e,
                "The earlier navigation ended before a readable source was committed. Enter an HTTP(S) URL.") : NULL;
            if (reason) (*e)->CallStaticVoidMethod(e, application.generated, application.error_callback, activity, reason);
            if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
        }
    }
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
        application.history_validator = application.error_callback ?
            (*e)->GetStaticMethodID(e, generated, "reader_retained_history_valid",
                "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I") : NULL;
        application.dom_ready_callback = application.history_validator ? (*e)->GetStaticMethodID(e, generated,
            "reader_dom_acquisition_ready", "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;I)V") : NULL;
        application.browser_needed = application.dom_ready_callback ? (*e)->GetStaticMethodID(e, generated,
            "reader_google_browser_needed", "(Ljava/lang/String;Ljava/lang/String;)I") : NULL;
        application.browser_allowed = application.browser_needed ? (*e)->GetStaticMethodID(e, generated,
            "reader_browser_url_allowed", "(Ljava/lang/String;Ljava/lang/String;)I") : NULL;
        application.snapshot_ready = application.browser_allowed ? (*e)->GetStaticMethodID(e, generated,
            "reader_google_snapshot_ready", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I") : NULL;
        application.candidate_wire = application.snapshot_ready ? (*e)->GetStaticMethodID(e, generated,
            "reader_google_candidate_wire", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;") : NULL;
        application.target_admitted = application.candidate_wire ? (*e)->GetStaticMethodID(e, generated,
            "reader_google_target_admitted", "(Ljava/lang/String;Ljava/lang/String;)I") : NULL;
        application.progress_callback = application.target_admitted ? (*e)->GetStaticMethodID(e, generated,
            "reader_acquisition_progress", "(Landroid/app/Activity;Ljava/lang/String;)V") : NULL;
        jclass google_client = application.progress_callback ? (*e)->FindClass(e, "org/isomorphisms/ib/GoogleAcquisitionClient") : NULL;
        jclass google_dom = google_client ? (*e)->FindClass(e, "org/isomorphisms/ib/GoogleDomCallback") : NULL;
        jclass google_poll = google_dom ? (*e)->FindClass(e, "org/isomorphisms/ib/GooglePollTask") : NULL;
        ok = ok && google_poll && !(*e)->ExceptionCheck(e);
        if (ok) {
            application.context = (*e)->NewGlobalRef(e, context);
            application.generated = (*e)->NewGlobalRef(e, generated);
            application.acquire_task = (*e)->NewGlobalRef(e, acquire);
            application.completion_task = (*e)->NewGlobalRef(e, completion);
            application.google_client = (*e)->NewGlobalRef(e, google_client);
            application.google_dom = (*e)->NewGlobalRef(e, google_dom);
            application.google_poll = (*e)->NewGlobalRef(e, google_poll);
            ok = application.context && application.generated && application.acquire_task && application.completion_task &&
                application.google_client && application.google_dom && application.google_poll && !(*e)->ExceptionCheck(e);
        }
        if (ok) ib_shizuku_client_set_revocation_handler(authority_revoked, NULL);
    }
    if (ok) {
        jclass listener_type = (*e)->FindClass(e, "org/isomorphisms/ib/ReaderLayoutListener");
        jmethodID constructor = listener_type ? (*e)->GetMethodID(e, listener_type, "<init>", "()V") : NULL;
        jobject listener = constructor ? (*e)->NewObject(e, listener_type, constructor) : NULL;
        jmethodID find = method(e, "android/app/Activity", "findViewById", "(I)Landroid/view/View;");
        jobject body = find ? (*e)->CallObjectMethod(e, activity, find, BODY_VIEW) : NULL;
        jmethodID add = method(e, "android/view/View", "addOnLayoutChangeListener", "(Landroid/view/View$OnLayoutChangeListener;)V");
        if (body && listener && add) (*e)->CallVoidMethod(e, body, add, listener);
        ok = body && listener && add && !(*e)->ExceptionCheck(e);
    }
    if (ok) {
        pthread_mutex_lock(&application.mutex);
        application.ready = 1;
        pthread_mutex_unlock(&application.mutex);
        ok = bind_activity(e, activity) >= 0;
    }
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    (*e)->PopLocalFrame(e, NULL);
    return ok ? 0 : -1;
}

/* Framework onResume reclaims an existing Activity without registering another
 * layout listener or granting a provider capability. Only a changed owner
 * reopens committed state; returning from a permission dialog is a no-op. */
JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderBridge_resume(JNIEnv *e, jclass type, jobject activity) {
    (void)type;
    if (!activity || !application.ready || (*e)->ExceptionCheck(e)) return -1;
    int changed = bind_activity(e, activity);
    int result = changed;
    if (changed > 0) {
        pthread_mutex_lock(&application.mutex);
        int busy = application.active != NULL;
        pthread_mutex_unlock(&application.mutex);
        if (!busy && enqueue(e, NULL, 1, NULL, 0, NULL) != 0) result = 2;
    }
    if ((*e)->ExceptionCheck(e)) { (*e)->ExceptionClear(e); return -1; }
    return result;
}

/* A late destroy callback from an older Activity cannot detach a newer owner. */
JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_ReaderBridge_detach(JNIEnv *e, jclass type, jobject activity) {
    (void)type;
    if (!activity || (*e)->ExceptionCheck(e)) return;
    if (application.browser_job && application.activity && (*e)->IsSameObject(e, activity, application.activity))
        browser_cancel_owner(e, application.browser_job);
    pthread_mutex_lock(&application.mutex);
    int current = application.activity && (*e)->IsSameObject(e, activity, application.activity);
    jweak previous = NULL;
    if (current) {
        previous = application.activity;
        application.activity = NULL;
        if (application.activity_epoch < UINT64_MAX) ++application.activity_epoch;
        application.restore_after_retirement = 0;
        if (application.capability)
            ib_ingress_revoke(ib_ingress_capability_session(application.capability));
    }
    pthread_mutex_unlock(&application.mutex);
    if (previous) (*e)->DeleteWeakGlobalRef(e, previous);
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

static int acquire_network_source(JNIEnv *e, reader_job *job, const char *id, const char *url) {
    uid_t uid;
    uint64_t epoch;
    if (ib_shizuku_client_authorization(e, &uid, &epoch) != IB_SHIZUKU_AUTHORIZED || uid != 2000) return 0;
    ib_ingress_capability *capability = ib_ingress_capability_new(ib_ingress_arena_publish_frame, &application.arena);
    ib_shizuku_arena_ingress *session = ib_ingress_capability_session(capability);
    int armed = session && ib_ingress_arm(session, id, job->navigation, url, uid) == 0;
    jobject ingress = armed ? ib_shizuku_ingress_binder_new(e, capability) : NULL;
    int acquired = 0;
    if (ingress) {
        pthread_mutex_lock(&application.mutex);
        int owned = current_job_locked(job);
        if (owned) { application.capability = capability; application.capability_epoch = epoch; }
        pthread_mutex_unlock(&application.mutex);
        acquired = owned && ib_shizuku_worker_acquire(e, application.context, ingress, id, job->navigation, epoch) == 0;
        pthread_mutex_lock(&application.mutex);
        if (application.capability == capability) application.capability = NULL;
        pthread_mutex_unlock(&application.mutex);
    }
    if (session) ib_ingress_revoke(session);
    if (capability) ib_ingress_capability_release(capability);
    return acquired;
}

static jstring http_source_fields(JNIEnv *e, const ib_reader_source_ticket *ticket,
                                  const ib_arena_source_observations *observations,
                                  const ib_reader_source *source) {
    char fields[5000], observed[65];
    ib_reader_sha256_hex(source->bytes, source->byte_count, observed);
    int count = snprintf(fields, sizeof(fields), "%s\t%" PRIu64 "\t%s\t%s\t%u\t%zu\t%s\t%s\t%s\t%s\t%s\t%s\tcomplete",
        observations->source_key, observations->source_generation, ticket->sha256_hex, observed,
        ticket->byte_count, source->byte_count, observations->requested_url, observations->resolved_url,
        observations->mime_type, observations->charset, observations->http_status,
        observations->utf8_valid ? "verified-utf8" : "invalid-utf8");
    return count > 0 && (size_t)count < sizeof(fields) ? (*e)->NewStringUTF(e, fields) : NULL;
}

static jstring http_source_body(JNIEnv *e, const ib_arena_source_observations *observations,
                                const ib_reader_source *source) {
    int utf8 = observations->utf8_valid &&
        (!strcmp(observations->charset, "utf-8") || !strcmp(observations->charset, "UTF-8"));
    return source_text(e, source->bytes, source->byte_count, utf8);
}

static jstring dom_source_fields(JNIEnv *e, const reader_job *job) {
    char fields[6000], observed[65];
    const ib_browser_dom_metadata *m = &job->dom;
    ib_reader_sha256_hex(job->source.bytes, job->source.byte_count, observed);
    int count = snprintf(fields, sizeof(fields),
        "IBDOM1\tarena-dom:%s\t%u\t%s\t%s\t%u\t%zu\t%s\t%s\tarena:%s\t%u\t%s\t%s\t%s\twebview-outerHTML-utf8-v1\tverified-utf8\tcomplete",
        m->request_id, m->source_generation, m->sha256, observed, m->byte_count, job->source.byte_count,
        m->requested_url, m->resolved_url, m->parent_id, m->parent_generation, m->parent_sha256,
        m->provider_package, m->provider_version);
    return count > 0 && (size_t)count < sizeof(fields) ? (*e)->NewStringUTF(e, fields) : NULL;
}

static int load_target_source(JNIEnv *e, reader_job *job, unsigned index) {
    reader_target *target = &job->targets[index];
    const ib_dom_target_reference *reference = &job->dom.targets[index];
    ib_reader_source_want(&target->source, job->navigation);
    int fd = -1;
    int ok = ib_arena_source_owner_open_decode_input(&application.sources, reference->request_id,
        job->dom.source_generation, job->navigation, &target->ticket, &target->observations, &fd) == 0;
    if (ok) ok = ib_reader_source_accept_fd(&target->source, &target->ticket, fd) == IB_READER_PFD_ACCEPTED &&
        !strcmp(reference->requested_url, target->observations.requested_url);
    if (fd >= 0) close(fd);
    if (!ok || (*e)->PushLocalFrame(e, 8) != 0) return 0;
    jstring fields = http_source_fields(e, &target->ticket, &target->observations, &target->source);
    jstring body = fields ? http_source_body(e, &target->observations, &target->source) : NULL;
    jint admitted = body && !(*e)->ExceptionCheck(e) ? (*e)->CallStaticIntMethod(e,
        application.generated, application.target_admitted, fields, body) : 0;
    if ((*e)->ExceptionCheck(e)) { (*e)->ExceptionClear(e); admitted = 0; }
    (*e)->PopLocalFrame(e, NULL);
    return admitted == 1;
}

static int open_job_source(JNIEnv *e, reader_job *job) {
    job->is_dom = 0;
    int fd = -1;
    int opened = ib_arena_source_owner_open_decode_input(&application.sources, job->id,
        job->source_generation, job->navigation, &job->ticket, &job->observations, &fd);
    if (opened != 0) {
        opened = ib_browser_dom_open(&application.sources, job->id, job->source_generation,
            job->navigation, &job->dom, &job->ticket, &fd);
        if (opened == 0) job->is_dom = 1;
    }
    int ok = opened == 0 && ib_reader_source_accept_fd(&job->source, &job->ticket, fd) == IB_READER_PFD_ACCEPTED;
    if (fd >= 0) close(fd);
    if (ok && job->is_dom)
        for (unsigned i = 0; i < job->dom.target_count; ++i) if (!load_target_source(e, job, i)) { ok = 0; break; }
    return ok;
}

static void post_job_completion(JNIEnv *e, reader_job *job) {
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
        retire_job(e, job, 0);
    }
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_ReaderAcquireTask_run(JNIEnv *e, jobject self) {
    if ((*e)->PushLocalFrame(e, 128) != 0) return;
    AIBinder *anchor = task_anchor(e, self);
    reader_job *job = anchor ? AIBinder_getUserData(anchor) : NULL;
    if (!job) { if (anchor) AIBinder_decStrong(anchor); (*e)->PopLocalFrame(e, NULL); return; }
    pthread_mutex_lock(&application.mutex);
    int checking_browser = job->browser_checking;
    pthread_mutex_unlock(&application.mutex);
    if (checking_browser) {
        browser_snapshot_worker(e, job);
        AIBinder_decStrong(anchor);
        (*e)->PopLocalFrame(e, NULL);
        return;
    }
    job->result = -1;
    pthread_mutex_lock(&application.mutex);
    int current = current_job_locked(job);
    pthread_mutex_unlock(&application.mutex);
    int acquired = current && job->retained ?
        checked_retained_history(e, job->id, job->source_generation, job->index.journal) : 0;
    if (current && !job->retained) acquired = acquire_network_source(e, job, job->id, job->url);
    job->result = acquired && open_job_source(e, job) ? 0 : -1;
    if (job->retained == 1 && job->result != 0 && !job->index.previous && last_source(job, 1) == 0 &&
        checked_retained_history(e, job->id, job->source_generation, job->index.journal) && open_job_source(e, job))
        job->result = 0;
    if ((*e)->ExceptionCheck(e)) { (*e)->ExceptionClear(e); job->result = -1; }
    __android_log_print(ANDROID_LOG_INFO, "IBReader", "committed source acquired=%d result=%d bytes=%zu navigation=%u source_generation=%u rendered_dom=%d",
        acquired, job->result, job->source.byte_count, job->navigation, job->source_generation, job->is_dom);
    post_job_completion(e, job);
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
    int current = current_job_locked(job);
    jobject activity = current && application.activity ? (*e)->NewLocalRef(e, application.activity) : NULL;
    pthread_mutex_unlock(&application.mutex);
    __android_log_print(ANDROID_LOG_INFO, "IBReader", "source completion current=%d activity=%d result=%d bytes=%zu",
        current, activity != NULL, job ? job->result : -1, job ? job->source.byte_count : 0U);
    if (current && activity && job->result == 0) {
        jstring evidence = job->is_dom ? dom_source_fields(e, job) :
            http_source_fields(e, &job->ticket, &job->observations, &job->source);
        jstring body = evidence ? (job->is_dom ? source_text(e, job->source.bytes, job->source.byte_count, 1) :
            http_source_body(e, &job->observations, &job->source)) : NULL;
        jint needs_browser = !job->retained && !job->is_dom && !job->browser_started && body &&
            !(*e)->ExceptionCheck(e) ? (*e)->CallStaticIntMethod(e, application.generated,
                application.browser_needed, evidence, body) : 0;
        if (needs_browser == 1 && !(*e)->ExceptionCheck(e) && browser_begin(e, job, activity) == 0) {
            if (anchor) AIBinder_decStrong(anchor);
            (*e)->PopLocalFrame(e, NULL);
            return;
        }
        browser_cleanup(e, job);
        jstring id = body ? (*e)->NewStringUTF(e, job->id) : NULL;
        jstring journal = id ? (*e)->NewStringUTF(e, job->index.journal) : NULL;
        if (journal) {
            __android_log_print(ANDROID_LOG_INFO, "IBReader", "checked renderer callback begin navigation=%u source_generation=%u", job->navigation, job->source_generation);
            (*e)->CallStaticVoidMethod(e, application.generated,
                job->is_dom ? application.dom_ready_callback : application.ready_callback,
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
        jstring reason = (*e)->NewStringUTF(e, job->browser_started ?
            "Google acquisition did not produce verified public results. The previous document is retained." :
            "Acquisition or verified source readback failed. The previous document is retained.");
        if (reason) (*e)->CallStaticVoidMethod(e, application.generated, application.error_callback, activity, reason);
    }
    retire_job(e, job, 1);
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

/* The WebView is an explicitly identified first-party Google acquisition
 * adapter. It never supplies ordinary document bytes, HTTP observations or
 * native-reader history. Checked Idric owns its URL and result policies. */
static int64_t browser_clock(void) {
    struct timespec value;
    return clock_gettime(CLOCK_MONOTONIC, &value) == 0 ?
        (int64_t)value.tv_sec * 1000 + value.tv_nsec / 1000000 : -1;
}

static jobject browser_activity(JNIEnv *e, reader_job *job) {
    pthread_mutex_lock(&application.mutex);
    jobject result = current_job_locked(job) && application.activity ?
        (*e)->NewLocalRef(e, application.activity) : NULL;
    pthread_mutex_unlock(&application.mutex);
    return result;
}

static int browser_current(reader_job *job) {
    pthread_mutex_lock(&application.mutex);
    int result = current_job_locked(job) && job->browser_pending;
    pthread_mutex_unlock(&application.mutex);
    return result;
}

static void browser_progress(JNIEnv *e, reader_job *job, const char *message) {
    jobject activity = browser_activity(e, job);
    jstring text = activity ? (*e)->NewStringUTF(e, message) : NULL;
    if (text) (*e)->CallStaticVoidMethod(e, application.generated, application.progress_callback, activity, text);
}

static jobject browser_view(JNIEnv *e, jobject activity, jint id) {
    jmethodID find = method(e, "android/app/Activity", "findViewById", "(I)Landroid/view/View;");
    return activity && find ? (*e)->CallObjectMethod(e, activity, find, id) : NULL;
}

static void browser_visibility(JNIEnv *e, jobject view, jint visibility) {
    jmethodID set = method(e, "android/view/View", "setVisibility", "(I)V");
    if (view && set) (*e)->CallVoidMethod(e, view, set, visibility);
}

static void browser_cleanup(JNIEnv *e, reader_job *job) {
    if (!job || application.browser_job != job) return;
    jobject browser = application.browser;
    application.browser = NULL;
    application.browser_job = NULL;
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    pthread_mutex_lock(&application.mutex);
    jobject activity = application.activity && job->epoch == application.activity_epoch ?
        (*e)->NewLocalRef(e, application.activity) : NULL;
    pthread_mutex_unlock(&application.mutex);
    jobject panel = browser_view(e, activity, GOOGLE_PANEL_VIEW);
    jmethodID remove = method(e, "android/view/ViewGroup", "removeAllViews", "()V");
    if (panel && remove) (*e)->CallVoidMethod(e, panel, remove);
    browser_visibility(e, panel, 8);
    browser_visibility(e, browser_view(e, activity, READER_SCROLL_VIEW), 0);
    jmethodID stop = method(e, "android/webkit/WebView", "stopLoading", "()V");
    if (browser && stop) (*e)->CallVoidMethod(e, browser, stop);
    jmethodID destroy = method(e, "android/webkit/WebView", "destroy", "()V");
    if (browser && destroy) (*e)->CallVoidMethod(e, browser, destroy);
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    if (browser) (*e)->DeleteGlobalRef(e, browser);
    job_release(job); /* the UI-owned WebView reference */
}

static void browser_cancel_owner(JNIEnv *e, reader_job *job) {
    if (!job) return;
    job_retain(job);
    pthread_mutex_lock(&application.mutex);
    int owned = application.active == job;
    if (owned) {
        job->cancelled = 1;
        job->browser_pending = 0;
        application.restore_after_retirement = 0;
        ib_arena_source_owner_want(&application.sources, 0);
        ib_reader_source_want(&job->source, 0);
        if (application.capability) ib_ingress_revoke(ib_ingress_capability_session(application.capability));
    }
    pthread_mutex_unlock(&application.mutex);
    if (owned) retire_job(e, job, 1);
    job_release(job);
}

static void browser_abort(JNIEnv *e, reader_job *job, const char *message) {
    jobject activity = browser_activity(e, job);
    jstring reason = activity ? (*e)->NewStringUTF(e, message) : NULL;
    if (reason) (*e)->CallStaticVoidMethod(e, application.generated, application.error_callback, activity, reason);
    if ((*e)->ExceptionCheck(e)) log_reader_exception(e);
    browser_cancel_owner(e, job);
}

static int browser_url_allowed(JNIEnv *e, reader_job *job, jstring actual) {
    jstring requested = (*e)->NewStringUTF(e, job->url);
    jint allowed = requested && actual && !(*e)->ExceptionCheck(e) ?
        (*e)->CallStaticIntMethod(e, application.generated, application.browser_allowed, requested, actual) : 0;
    if ((*e)->ExceptionCheck(e)) { (*e)->ExceptionClear(e); return 0; }
    return allowed == 1;
}

static int browser_post_poll(JNIEnv *e, reader_job *job) {
    jobject poll = task(e, job, application.google_poll);
    jmethodID post = method(e, "android/view/View", "postDelayed", "(Ljava/lang/Runnable;J)Z");
    return application.browser && poll && post &&
        (*e)->CallBooleanMethod(e, application.browser, post, poll, (jlong)1000) && !(*e)->ExceptionCheck(e);
}

static int browser_begin(JNIEnv *e, reader_job *job, jobject activity) {
    if (application.browser || !browser_activity(e, job)) return -1;
    job->browser_started = 1;
    memset(&job->dom, 0, sizeof(job->dom));
    memcpy(job->dom.parent_id, job->id, sizeof(job->id));
    memcpy(job->dom.requested_url, job->url, sizeof(job->url));
    memcpy(job->dom.parent_sha256, job->ticket.sha256_hex, sizeof(job->dom.parent_sha256));
    job->dom.source_generation = job->dom.parent_generation = job->source_generation;
    int64_t now = browser_clock();
    if (now < 0) return -1;
    job->browser_deadline = now + 120000;
    job->browser_polls = 0;
    jclass web_type = (*e)->FindClass(e, "android/webkit/WebView");
    jmethodID constructor = web_type ? (*e)->GetMethodID(e, web_type, "<init>", "(Landroid/content/Context;)V") : NULL;
    jobject web = constructor ? (*e)->NewObject(e, web_type, constructor, activity) : NULL;
    jobject client = web ? task(e, job, application.google_client) : NULL;
    jmethodID get_settings = method(e, "android/webkit/WebView", "getSettings", "()Landroid/webkit/WebSettings;");
    jobject settings = web && get_settings ? (*e)->CallObjectMethod(e, web, get_settings) : NULL;
    static const char *const settings_names[] = {
        "setJavaScriptEnabled", "setDomStorageEnabled", "setAllowFileAccess", "setAllowContentAccess",
        "setSupportMultipleWindows", "setJavaScriptCanOpenWindowsAutomatically", "setSafeBrowsingEnabled"
    };
    for (unsigned i = 0; settings && i < 7U && !(*e)->ExceptionCheck(e); ++i) {
        jmethodID set = method(e, "android/webkit/WebSettings", settings_names[i], "(Z)V");
        if (set) (*e)->CallVoidMethod(e, settings, set, (jboolean)(i == 0U || i == 1U || i == 6U));
    }
    jmethodID mixed = method(e, "android/webkit/WebSettings", "setMixedContentMode", "(I)V");
    if (settings && mixed) (*e)->CallVoidMethod(e, settings, mixed, 1); /* MIXED_CONTENT_NEVER_ALLOW */
    jmethodID provider_method = web_type ? (*e)->GetStaticMethodID(e, web_type, "getCurrentWebViewPackage", "()Landroid/content/pm/PackageInfo;") : NULL;
    jobject provider = provider_method ? (*e)->CallStaticObjectMethod(e, web_type, provider_method) : NULL;
    jclass package_type = provider ? (*e)->GetObjectClass(e, provider) : NULL;
    jfieldID package_field = package_type ? (*e)->GetFieldID(e, package_type, "packageName", "Ljava/lang/String;") : NULL;
    jfieldID version_field = package_field ? (*e)->GetFieldID(e, package_type, "versionName", "Ljava/lang/String;") : NULL;
    jstring package_text = version_field ? (*e)->GetObjectField(e, provider, package_field) : NULL;
    jstring version_text = version_field ? (*e)->GetObjectField(e, provider, version_field) : NULL;
    int identified = ascii(e, package_text, job->dom.provider_package, sizeof(job->dom.provider_package)) &&
        ascii(e, version_text, job->dom.provider_version, sizeof(job->dom.provider_version));
    jmethodID set_client = method(e, "android/webkit/WebView", "setWebViewClient", "(Landroid/webkit/WebViewClient;)V");
    if (web && client && set_client) (*e)->CallVoidMethod(e, web, set_client, client);
    jobject panel = browser_view(e, activity, GOOGLE_PANEL_VIEW);
    jmethodID add = method(e, "android/view/ViewGroup", "addView", "(Landroid/view/View;II)V");
    if (web && panel && add) (*e)->CallVoidMethod(e, panel, add, web, -1, -1);
    int ok = identified && web && client && settings && panel && add && set_client && !(*e)->ExceptionCheck(e);
    if (!ok) {
        if ((*e)->ExceptionCheck(e)) log_reader_exception(e);
        return -1;
    }
    application.browser = (*e)->NewGlobalRef(e, web);
    if (!application.browser || (*e)->ExceptionCheck(e)) return -1;
    job_retain(job);
    application.browser_job = job;
    pthread_mutex_lock(&application.mutex);
    job->browser_pending = 1;
    pthread_mutex_unlock(&application.mutex);
    browser_visibility(e, browser_view(e, activity, READER_SCROLL_VIEW), 8);
    browser_visibility(e, panel, 0);
    browser_progress(e, job, "Google needs browser rendering. Loading results…");
    jstring url = (*e)->NewStringUTF(e, job->url);
    jmethodID load = method(e, "android/webkit/WebView", "loadUrl", "(Ljava/lang/String;)V");
    if (url && load) (*e)->CallVoidMethod(e, web, load, url);
    ok = url && load && !(*e)->ExceptionCheck(e) && browser_post_poll(e, job);
    if (!ok) { if ((*e)->ExceptionCheck(e)) log_reader_exception(e); browser_cleanup(e, job); }
    return ok ? 0 : -1;
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_GoogleAcquisitionClient_onPageStarted(JNIEnv *e, jobject self,
    jobject web, jstring url, jobject icon) {
    (void)icon;
    if ((*e)->PushLocalFrame(e, 16) != 0) return;
    AIBinder *anchor = task_anchor(e, self);
    reader_job *job = anchor ? AIBinder_getUserData(anchor) : NULL;
    if (job && application.browser_job == job && application.browser &&
        (*e)->IsSameObject(e, application.browser, web) && browser_current(job) &&
        !browser_url_allowed(e, job, url))
        browser_abort(e, job, "Google requested a consent, challenge or unsupported navigation. The previous document is retained.");
    if (anchor) AIBinder_decStrong(anchor);
    (*e)->PopLocalFrame(e, NULL);
}

JNIEXPORT jboolean JNICALL
Java_org_isomorphisms_ib_GoogleAcquisitionClient_shouldOverrideUrlLoading(JNIEnv *e, jobject self,
    jobject web, jobject request) {
    if ((*e)->PushLocalFrame(e, 16) != 0) return JNI_TRUE;
    AIBinder *anchor = task_anchor(e, self);
    reader_job *job = anchor ? AIBinder_getUserData(anchor) : NULL;
    jmethodID main_frame = method(e, "android/webkit/WebResourceRequest", "isForMainFrame", "()Z");
    int main = request && main_frame && (*e)->CallBooleanMethod(e, request, main_frame);
    int allowed = 0;
    if (job && application.browser_job == job && application.browser &&
        (*e)->IsSameObject(e, application.browser, web) && browser_current(job)) {
        if (!main) allowed = 1;
        else {
            jmethodID get_url = method(e, "android/webkit/WebResourceRequest", "getUrl", "()Landroid/net/Uri;");
            jobject uri = get_url ? (*e)->CallObjectMethod(e, request, get_url) : NULL;
            jmethodID to_string = method(e, "android/net/Uri", "toString", "()Ljava/lang/String;");
            jstring url = uri && to_string ? (*e)->CallObjectMethod(e, uri, to_string) : NULL;
            allowed = url && browser_url_allowed(e, job, url);
            if (!allowed) browser_abort(e, job, "Google requested a consent, challenge or unsupported navigation. The previous document is retained.");
        }
    }
    if ((*e)->ExceptionCheck(e)) { (*e)->ExceptionClear(e); allowed = 0; }
    if (anchor) AIBinder_decStrong(anchor);
    (*e)->PopLocalFrame(e, NULL);
    return allowed ? JNI_FALSE : JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_GoogleAcquisitionClient_onReceivedError(JNIEnv *e, jobject self,
    jobject web, jobject request, jobject error) {
    (void)web; (void)error;
    if ((*e)->PushLocalFrame(e, 16) != 0) return;
    AIBinder *anchor = task_anchor(e, self);
    reader_job *job = anchor ? AIBinder_getUserData(anchor) : NULL;
    jmethodID main_frame = method(e, "android/webkit/WebResourceRequest", "isForMainFrame", "()Z");
    if (job && browser_current(job) && request && main_frame && (*e)->CallBooleanMethod(e, request, main_frame))
        browser_abort(e, job, "Google browser acquisition failed. The previous document is retained.");
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    if (anchor) AIBinder_decStrong(anchor);
    (*e)->PopLocalFrame(e, NULL);
}

JNIEXPORT jboolean JNICALL
Java_org_isomorphisms_ib_GoogleAcquisitionClient_onRenderProcessGone(JNIEnv *e, jobject self,
    jobject web, jobject detail) {
    (void)web; (void)detail;
    if ((*e)->PushLocalFrame(e, 16) != 0) return JNI_TRUE;
    AIBinder *anchor = task_anchor(e, self);
    reader_job *job = anchor ? AIBinder_getUserData(anchor) : NULL;
    if (job && browser_current(job)) browser_abort(e, job, "Google browser process ended. The previous document is retained.");
    if (anchor) AIBinder_decStrong(anchor);
    (*e)->PopLocalFrame(e, NULL);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_GooglePollTask_run(JNIEnv *e, jobject self) {
    if ((*e)->PushLocalFrame(e, 24) != 0) return;
    AIBinder *anchor = task_anchor(e, self);
    reader_job *job = anchor ? AIBinder_getUserData(anchor) : NULL;
    if (job && browser_current(job) && application.browser_job == job && application.browser) {
        int64_t now = browser_clock();
        pthread_mutex_lock(&application.mutex);
        int checking = job->browser_checking, committing = job->browser_committing;
        pthread_mutex_unlock(&application.mutex);
        if (now < 0 || now >= job->browser_deadline || (!checking && job->browser_polls >= 30U)) {
            browser_abort(e, job, "Google did not produce verified results within the acquisition deadline. The previous document is retained.");
        } else if (!committing) {
            if (!checking) {
                jobject callback = task(e, job, application.google_dom);
                jstring script = callback ? (*e)->NewStringUTF(e,
                    "[location.href,document.documentElement?document.documentElement.outerHTML:'']") : NULL;
                jmethodID evaluate = method(e, "android/webkit/WebView", "evaluateJavascript", "(Ljava/lang/String;Landroid/webkit/ValueCallback;)V");
                if (script && evaluate) {
                    ++job->browser_polls;
                    (*e)->CallVoidMethod(e, application.browser, evaluate, script, callback);
                }
                if (!script || !evaluate || (*e)->ExceptionCheck(e)) {
                    if ((*e)->ExceptionCheck(e)) log_reader_exception(e);
                    browser_abort(e, job, "Google DOM acquisition failed. The previous document is retained.");
                }
            }
            if (browser_current(job) && !browser_post_poll(e, job))
                browser_abort(e, job, "Google acquisition scheduling failed. The previous document is retained.");
        } else {
            browser_progress(e, job, "Verifying Google result links through ICU…");
            /* Keep the deadline and cancellation active while network work is off-thread. */
            if (!browser_post_poll(e, job)) browser_abort(e, job, "Google acquisition scheduling failed. The previous document is retained.");
        }
    }
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    if (anchor) AIBinder_decStrong(anchor);
    (*e)->PopLocalFrame(e, NULL);
}

static unsigned char *browser_utf8(JNIEnv *e, jstring text, size_t *byte_count) {
    if (!text || (*e)->ExceptionCheck(e)) return NULL;
    jsize length = (*e)->GetStringLength(e, text);
    if (length < 1 || (uint64_t)length > IB_DOM_MAX_BYTES) return NULL;
    const jchar *units = (*e)->GetStringChars(e, text, NULL);
    if (!units) return NULL;
    int valid = 1;
    for (jsize i = 0; i < length; ++i) {
        if (!units[i]) { valid = 0; break; }
        if (units[i] >= 0xd800U && units[i] <= 0xdbffU) {
            if (i + 1 >= length || units[i + 1] < 0xdc00U || units[i + 1] > 0xdfffU) { valid = 0; break; }
            ++i;
        } else if (units[i] >= 0xdc00U && units[i] <= 0xdfffU) { valid = 0; break; }
    }
    (*e)->ReleaseStringChars(e, text, units);
    if (!valid) return NULL;
    jmethodID encode = method(e, "java/lang/String", "getBytes", "(Ljava/lang/String;)[B");
    jstring charset = encode ? (*e)->NewStringUTF(e, "UTF-8") : NULL;
    jbyteArray bytes = charset ? (*e)->CallObjectMethod(e, text, encode, charset) : NULL;
    if (!bytes || (*e)->ExceptionCheck(e)) return NULL;
    jsize count = (*e)->GetArrayLength(e, bytes);
    if (count < 1 || (uint64_t)count > IB_DOM_MAX_BYTES) return NULL;
    unsigned char *copy = malloc((size_t)count);
    if (!copy) return NULL;
    (*e)->GetByteArrayRegion(e, bytes, 0, count, (jbyte *)copy);
    if ((*e)->ExceptionCheck(e)) { free(copy); return NULL; }
    *byte_count = (size_t)count;
    return copy;
}

JNIEXPORT void JNICALL
Java_org_isomorphisms_ib_GoogleDomCallback_onReceiveValue(JNIEnv *e, jobject self, jobject value) {
    if ((*e)->PushLocalFrame(e, 32) != 0) return;
    AIBinder *anchor = task_anchor(e, self);
    reader_job *job = anchor ? AIBinder_getUserData(anchor) : NULL;
    int accept = job && browser_current(job) && application.browser_job == job;
    pthread_mutex_lock(&application.mutex);
    if (accept) accept = !job->browser_checking && !job->browser_committing;
    pthread_mutex_unlock(&application.mutex);
    if (accept) {
        jclass string_type = (*e)->FindClass(e, "java/lang/String");
        jclass array_type = string_type && value && (*e)->IsInstanceOf(e, value, string_type) ?
            (*e)->FindClass(e, "org/json/JSONArray") : NULL;
        jmethodID constructor = array_type ? (*e)->GetMethodID(e, array_type, "<init>", "(Ljava/lang/String;)V") : NULL;
        jobject array = constructor ? (*e)->NewObject(e, array_type, constructor, value) : NULL;
        jmethodID count = method(e, "org/json/JSONArray", "length", "()I");
        jmethodID get = method(e, "org/json/JSONArray", "get", "(I)Ljava/lang/Object;");
        jobject url = array && count && get && (*e)->CallIntMethod(e, array, count) == 2 ?
            (*e)->CallObjectMethod(e, array, get, 0) : NULL;
        jobject dom = url && !(*e)->ExceptionCheck(e) ? (*e)->CallObjectMethod(e, array, get, 1) : NULL;
        char actual[2049];
        int valid = url && dom && (*e)->IsInstanceOf(e, url, string_type) &&
            (*e)->IsInstanceOf(e, dom, string_type) && ascii(e, url, actual, sizeof(actual)) &&
            browser_url_allowed(e, job, url);
        size_t bytes = 0;
        unsigned char *copy = valid ? browser_utf8(e, dom, &bytes) : NULL;
        if (copy && !(*e)->ExceptionCheck(e)) {
            pthread_mutex_lock(&application.mutex);
            int owned = current_job_locked(job) && job->browser_pending && !job->browser_checking && !job->browser_committing;
            if (owned) {
                job->pending_dom = copy;
                job->pending_dom_bytes = bytes;
                memcpy(job->pending_dom_url, actual, strlen(actual) + 1U);
                job->browser_checking = 1;
            }
            pthread_mutex_unlock(&application.mutex);
            if (!owned) free(copy);
            else if (start_job(e, job) != 0) browser_abort(e, job, "Google result verification could not start. The previous document is retained.");
        }
        /* A transient empty/oversized serialization is not a completed source.
         * Polling remains bounded; no result or source identity is fabricated. */
    }
    if ((*e)->ExceptionCheck(e)) (*e)->ExceptionClear(e);
    if (anchor) AIBinder_decStrong(anchor);
    (*e)->PopLocalFrame(e, NULL);
}

static int candidate_rows(JNIEnv *e, jstring wire, ib_browser_dom_metadata *metadata) {
    if (!wire || (*e)->ExceptionCheck(e)) return 0;
    jsize length = (*e)->GetStringLength(e, wire);
    if (length < 11 || length > 16384) return 0;
    const jchar *units = (*e)->GetStringChars(e, wire, NULL);
    if (!units) return 0;
    char *text = malloc((size_t)length + 1U);
    int valid = text != NULL;
    for (jsize i = 0; valid && i < length; ++i) {
        if ((units[i] < 32U || units[i] > 126U) && units[i] != '\t' && units[i] != '\n') valid = 0;
        else text[i] = (char)units[i];
    }
    (*e)->ReleaseStringChars(e, wire, units);
    if (!valid) { free(text); return 0; }
    text[length] = 0;
    char *row = text + 10;
    valid = !strncmp(text, "IBGOOGLE1\n", 10U);
    unsigned count = 0;
    while (valid && *row) {
        char *first = strchr(row, '\t'), *end = strchr(row, '\n');
        char *second = first ? strchr(first + 1, '\t') : NULL;
        if (count >= IB_DOM_MAX_TARGETS || !first || !second || !end || first >= second || second >= end ||
            memchr(second + 1, '\t', (size_t)(end - second - 1))) { valid = 0; break; }
        *first = 0; *second = 0; *end = 0;
        uint64_t occurrence = 0;
        if (!*row || (row[0] == '0' && row[1])) { valid = 0; break; }
        for (char *p = row; *p; ++p) {
            if (*p < '0' || *p > '9' || occurrence > (UINT32_MAX - (unsigned)(*p - '0')) / 10U) { valid = 0; break; }
            occurrence = occurrence * 10U + (unsigned)(*p - '0');
        }
        ib_dom_target_reference *target = &metadata->targets[count];
        size_t href = strlen(first + 1), requested = strlen(second + 1);
        if (!valid || !href || href >= sizeof(target->original_href) || !requested || requested >= sizeof(target->requested_url)) { valid = 0; break; }
        for (unsigned i = 0; i < count; ++i) if (metadata->targets[i].occurrence == occurrence) valid = 0;
        if (!valid) break;
        target->occurrence = (uint32_t)occurrence;
        memcpy(target->original_href, first + 1, href + 1U);
        memcpy(target->requested_url, second + 1, requested + 1U);
        ++count;
        row = end + 1;
    }
    free(text);
    metadata->target_count = valid ? count : 0;
    return valid && count > 0;
}

static void browser_snapshot_worker(JNIEnv *e, reader_job *job) {
    pthread_mutex_lock(&application.mutex);
    unsigned char *bytes = job->pending_dom;
    size_t count = job->pending_dom_bytes;
    char actual[2049];
    memcpy(actual, job->pending_dom_url, sizeof(actual));
    job->pending_dom = NULL; job->pending_dom_bytes = 0;
    int current = current_job_locked(job) && job->browser_pending;
    pthread_mutex_unlock(&application.mutex);
    jstring requested = current && bytes ? (*e)->NewStringUTF(e, job->url) : NULL;
    jstring resolved = requested ? (*e)->NewStringUTF(e, actual) : NULL;
    jstring dom = resolved ? source_text(e, bytes, count, 1) : NULL;
    jint ready = dom && !(*e)->ExceptionCheck(e) ? (*e)->CallStaticIntMethod(e,
        application.generated, application.snapshot_ready, requested, resolved, dom) : -1;
    int64_t now = browser_clock();
    if (ready == 0 && !(*e)->ExceptionCheck(e) && now >= 0 && now < job->browser_deadline && browser_current(job)) {
        free(bytes);
        pthread_mutex_lock(&application.mutex);
        job->browser_checking = 0;
        pthread_mutex_unlock(&application.mutex);
        return;
    }
    int ok = ready > 0 && !(*e)->ExceptionCheck(e) && now >= 0 && now < job->browser_deadline && browser_current(job);
    if (ok) {
        pthread_mutex_lock(&application.mutex);
        job->browser_committing = 1;
        pthread_mutex_unlock(&application.mutex);
        memcpy(job->dom.resolved_url, actual, sizeof(actual));
        ok = random_identity(job->dom.request_id) == 0 &&
            ib_browser_dom_append(&application.sources, job->navigation, &job->dom, bytes, count) == 0;
    }
    free(bytes);
    if (ok) {
        ib_reader_source_clear(&job->source);
        ib_reader_source_want(&job->source, job->navigation);
        memcpy(job->id, job->dom.request_id, sizeof(job->id));
        job->is_dom = 1;
        int fd = -1;
        ok = ib_arena_source_owner_open_representation_range(&application.sources, job->id,
            job->navigation, job->dom.arena_offset, job->dom.byte_count, job->dom.sha256, &job->ticket, &fd) == 0;
        if (ok) ok = ib_reader_source_accept_fd(&job->source, &job->ticket, fd) == IB_READER_PFD_ACCEPTED;
        if (fd >= 0) close(fd);
        jstring fields = ok ? dom_source_fields(e, job) : NULL;
        jstring reopened = fields ? source_text(e, job->source.bytes, job->source.byte_count, 1) : NULL;
        jstring wire = reopened && !(*e)->ExceptionCheck(e) ? (*e)->CallStaticObjectMethod(e,
            application.generated, application.candidate_wire, fields, reopened) : NULL;
        ok = wire && !(*e)->ExceptionCheck(e) && candidate_rows(e, wire, &job->dom);
    }
    if (ok) {
        unsigned candidates = job->dom.target_count, accepted = 0;
        ib_dom_target_reference references[IB_DOM_MAX_TARGETS];
        memcpy(references, job->dom.targets, sizeof(references));
        job->dom.target_count = 0;
        for (unsigned i = 0; i < candidates && browser_current(job); ++i) {
            now = browser_clock();
            if (now < 0 || now >= job->browser_deadline) break;
            job->dom.targets[accepted] = references[i];
            ib_dom_target_reference *target = &job->dom.targets[accepted];
            if (random_identity(target->request_id) != 0) break;
            int acquired = acquire_network_source(e, job, target->request_id, target->requested_url);
            if (acquired && load_target_source(e, job, accepted)) ++accepted;
            else ib_reader_source_clear(&job->targets[accepted].source);
            if ((*e)->ExceptionCheck(e)) { (*e)->ExceptionClear(e); break; }
        }
        job->dom.target_count = accepted;
        ok = accepted > 0 && browser_current(job) && browser_clock() < job->browser_deadline &&
            ib_browser_dom_seal(&application.sources, job->navigation, &job->dom) == 0;
    }
    if (ok) ok = open_job_source(e, job);
    if ((*e)->ExceptionCheck(e)) { log_reader_exception(e); ok = 0; }
    pthread_mutex_lock(&application.mutex);
    job->browser_pending = 0;
    job->browser_checking = 0;
    job->result = ok && current_job_locked(job) ? 0 : -1;
    pthread_mutex_unlock(&application.mutex);
    __android_log_print(ANDROID_LOG_INFO, "IBReader", "browser-derived source complete=%d verified_targets=%u navigation=%u source_generation=%u",
        job->result == 0, job->dom.target_count, job->navigation, job->source_generation);
    post_job_completion(e, job);
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderBridge_cancel(JNIEnv *e, jclass type, jobject activity) {
    (void)type;
    pthread_mutex_lock(&application.mutex);
    reader_job *job = application.active;
    int owned = job && application.activity && (*e)->IsSameObject(e, activity, application.activity);
    if (owned) job_retain(job);
    pthread_mutex_unlock(&application.mutex);
    if (!owned) return 0;
    browser_cancel_owner(e, job);
    job_release(job);
    return 1;
}

static reader_job *target_job(JNIEnv *e, jstring id, jint navigation, jint index) {
    char raw[IB_READER_REQUEST_ID_CAP + 1U];
    if (navigation < 1 || !ascii(e, id, raw, sizeof(raw))) return NULL;
    pthread_mutex_lock(&application.mutex);
    reader_job *job = application.active;
    int valid = current_job_locked(job) && job->is_dom && job->result == 0 && !job->browser_pending &&
        !job->browser_checking && job->navigation == (uint32_t)navigation && !strcmp(job->id, raw) &&
        index >= -1 && (index < 0 || (unsigned)index < job->dom.target_count);
    if (valid) job_retain(job);
    pthread_mutex_unlock(&application.mutex);
    return valid ? job : NULL;
}

JNIEXPORT jint JNICALL
Java_org_isomorphisms_ib_ReaderBridge_googleTargetCount(JNIEnv *e, jclass type, jstring id, jint navigation) {
    (void)type;
    reader_job *job = target_job(e, id, navigation, -1);
    jint result = job ? (jint)job->dom.target_count : -1;
    if (job) job_release(job);
    return result;
}

JNIEXPORT jstring JNICALL
Java_org_isomorphisms_ib_ReaderBridge_googleTargetReference(JNIEnv *e, jclass type,
    jstring id, jint navigation, jint index) {
    (void)type;
    reader_job *job = target_job(e, id, navigation, index);
    char reference[2080];
    int count = job ? snprintf(reference, sizeof(reference), "%u\t%s", job->dom.targets[index].occurrence,
        job->dom.targets[index].original_href) : -1;
    jstring result = count > 0 && (size_t)count < sizeof(reference) ? (*e)->NewStringUTF(e, reference) : (*e)->NewStringUTF(e, "");
    if (job) job_release(job);
    return result;
}

JNIEXPORT jstring JNICALL
Java_org_isomorphisms_ib_ReaderBridge_googleTargetEvidence(JNIEnv *e, jclass type,
    jstring id, jint navigation, jint index) {
    (void)type;
    reader_job *job = target_job(e, id, navigation, index);
    jstring result = job ? http_source_fields(e, &job->targets[index].ticket,
        &job->targets[index].observations, &job->targets[index].source) : (*e)->NewStringUTF(e, "");
    if (job) job_release(job);
    return result;
}

JNIEXPORT jstring JNICALL
Java_org_isomorphisms_ib_ReaderBridge_googleTargetBody(JNIEnv *e, jclass type,
    jstring id, jint navigation, jint index) {
    (void)type;
    reader_job *job = target_job(e, id, navigation, index);
    jstring result = job ? http_source_body(e, &job->targets[index].observations, &job->targets[index].source) :
        (*e)->NewStringUTF(e, "");
    if (job) job_release(job);
    return result;
}
