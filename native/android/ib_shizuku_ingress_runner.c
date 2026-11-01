#define _POSIX_C_SOURCE 200809L
#include "ib_shizuku_ingress_runner.h"
#include <android/log.h>

#if defined(__ANDROID__) && __ANDROID_API__ >= 29
#include "ib_shizuku_ingress_binder.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char **environ;

#define BROKER_PROCESS_DEADLINE_MS 35000

static int64_t monotonic_milliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return -1;
    return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static int approved_executable_path(const char *path, int executable) {
    if (!path || path[0] != '/') return 0;
    size_t n = strnlen(path, 4097U);
    if (n < 2 || n > 4096) return 0;
    /* No path substitution via relative traversal or writable leaf. The
     * independent Cat Food digest/executable pin must also be checked by the
     * caller; a same-UID attacker could otherwise replace ancestor paths. */
    if (strstr(path, "/../") != NULL || strstr(path, "/./") != NULL ||
        (n >= 3 && strcmp(path + n - 3, "/..") == 0) ||
        (n >= 2 && strcmp(path + n - 2, "/.") == 0))
        return 0;
    struct stat info;
    if (lstat(path, &info) != 0 || !S_ISREG(info.st_mode) ||
        (info.st_mode & 022U) != 0 ||
        (info.st_uid != geteuid() && info.st_uid != 0 && info.st_uid != 1000))
        return 0;
    return !executable || (info.st_mode & 0111U) != 0;
}

static int run_process(const ib_icu_worker_runtime *runtime,
                       const char *authorized_url, int write_fd) {
    /* Prepare all memory before fork in the ART-hosted user-service process.
     * The child performs only fixed native syscalls before exec. The Grease
     * supervisor installs its TERM trap before starting ICU; parent death
     * therefore terminates its whole acquisition group, not just a wrapper. */
    char *const argv[] = {
        (char *)"ysh", (char *)runtime->script, (char *)authorized_url,
        (char *)runtime->icu_dex, (char *)runtime->icu_library,
        (char *)runtime->capture_directory, NULL
    };
    pid_t parent = getpid();
    pid_t pid = fork();
    if (pid == 0) {
        if (setpgid(0, 0) != 0 || prctl(PR_SET_PDEATHSIG, SIGTERM) != 0 ||
            getppid() != parent) _exit(127);
        sigset_t unblocked;
        if (sigemptyset(&unblocked) != 0 || sigprocmask(SIG_SETMASK, &unblocked, NULL) != 0 ||
            dup2(write_fd, STDOUT_FILENO) < 0) _exit(127);
        if (write_fd != STDOUT_FILENO) close(write_fd);
        execve(runtime->grease, argv, environ);
        _exit(127);
    }
    if (pid < 0) return 127;
    /* Either side may win group creation; exec may already have happened. */
    if (setpgid(pid, pid) != 0 && errno != EACCES && errno != ESRCH) {
        (void)kill(pid, SIGKILL);
        while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) { }
        return 127;
    }

    int64_t start = monotonic_milliseconds();
    int64_t deadline = start < 0 ? 0 : start + BROKER_PROCESS_DEADLINE_MS;
    for (;;) {
        int status = 0;
        siginfo_t child = {0};
        int result = waitid(P_PID, (id_t)pid, &child, WEXITED | WNOHANG | WNOWAIT);
        if (result == 0 && child.si_pid == pid) {
            /* Keep the leader unreaped until group cleanup, so its PID cannot
             * be reused as an unrelated process/group between these calls. */
            (void)kill(-pid, SIGKILL);
            while (waitpid(pid, &status, 0) < 0) {
                if (errno != EINTR) return 125;
            }
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            if (WIFSIGNALED(status)) {
                int code = 128 + WTERMSIG(status);
                return code > 255 ? 255 : code;
            }
            return 125;
        }
        if (result < 0 && errno != EINTR) return 125;
        int64_t now = monotonic_milliseconds();
        if (now < 0 || now >= deadline) {
            (void)kill(-pid, SIGKILL);
            (void)kill(pid, SIGKILL);
            while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) { }
            return 124;
        }
        struct timespec interval = {0, 50000000L};
        while (nanosleep(&interval, &interval) != 0 && errno == EINTR) { }
    }
}

int ib_shizuku_ingress_run_icu(JNIEnv *environment, jobject private_ingress,
                               const char *request_id,
                               uint64_t navigation_generation,
                               const ib_icu_worker_runtime *runtime) {
    if (!environment || !private_ingress || !request_id || !runtime || navigation_generation == 0)
        return -1;

    char authorized_url[IB_INGRESS_URL_CAP + 1U] = {0};
    int write_fd = -1;
    if (ib_shizuku_ingress_begin_remote(
            environment, private_ingress, request_id, navigation_generation,
            authorized_url, &write_fd) != 0 || write_fd < 0) {
        __android_log_print(ANDROID_LOG_WARN, "IBReader", "private acquisition BEGIN refused");
        return -1;
    }

    int result = 127;
    struct stat directory;
    if (approved_executable_path(runtime->grease, 1) &&
        approved_executable_path(runtime->script, 0) &&
        approved_executable_path(runtime->icu_dex, 0) &&
        approved_executable_path(runtime->icu_library, 0) &&
        runtime->capture_directory &&
        lstat(runtime->capture_directory, &directory) == 0 &&
        S_ISDIR(directory.st_mode) && directory.st_uid == geteuid() &&
        (directory.st_mode & 0777U) == 0700U)
        result = run_process(runtime, authorized_url, write_fd);

    /* Always close the only broker-side copy BEFORE FINISH. Otherwise a
     * held write reference prevents the app from observing the exact EOF. */
    close(write_fd);
    int published = ib_shizuku_ingress_finish_remote(
        environment, private_ingress, request_id, navigation_generation, result);
    __android_log_print(ANDROID_LOG_INFO, "IBReader", "Grease supervisor exit=%d publication=%d", result, published);
    return result == 0 && published == 0 ? 0 : -1;
}
#endif
