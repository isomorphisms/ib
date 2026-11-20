#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "ib_shizuku_ingress_runner.h"
#include <android/log.h>

#if defined(__ANDROID__) && __ANDROID_API__ >= 29
#include "ib_shizuku_ingress_binder.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
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

#define STDERR_LIMIT 65536U

static int above_standard(int fd) {
    if (fd < 0 || fd > STDERR_FILENO) return fd;
    int moved = fcntl(fd, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
    close(fd);
    return moved;
}

static void drain_stderr(int stream, int output, size_t *saved, int *failed) {
    /* Drain after the cap too: a closed diagnostic pipe must not terminate the
     * acquisition. Bounded draining keeps the timeout/cancellation loop live. */
    for (unsigned attempt = 0; attempt < 4; ++attempt) {
        char bytes[8192];
        ssize_t count = read(stream, bytes, sizeof(bytes));
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return;
        size_t amount = (size_t)count;
        if (amount > STDERR_LIMIT - *saved) amount = STDERR_LIMIT - *saved;
        size_t used = 0;
        while (!*failed && used < amount) {
            ssize_t wrote = write(output, bytes + used, amount - used);
            if (wrote < 0 && errno == EINTR) continue;
            if (wrote <= 0) { *failed = 1; break; }
            used += (size_t)wrote;
        }
        *saved += used;
    }
}

static int run_process(const ib_icu_worker_runtime *runtime,
                       const char *authorized_url, int write_fd) {
    /* Shizuku may close the read side of the service starter's stderr pipe.
     * Never inherit it into Grease. Preserve only bounded private diagnostics;
     * no diagnostic text, URL, token or source bytes go into Android logcat. */
    char diagnostic[4096];
    int length = snprintf(diagnostic, sizeof(diagnostic), "%s.stderr", runtime->script);
    if (length < 1 || (size_t)length >= sizeof(diagnostic)) return 127;
    int output = above_standard(open(diagnostic, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600));
    if (output < 0) return 127;
    int pair[2] = {-1, -1};
    if (pipe2(pair, O_CLOEXEC | O_NONBLOCK) != 0) {
        close(output); (void)unlink(diagnostic); return 127;
    }
    pair[0] = above_standard(pair[0]);
    pair[1] = above_standard(pair[1]);
    int flags = pair[1] >= 0 ? fcntl(pair[1], F_GETFL) : -1;
    if (pair[0] < 0 || flags < 0 || fcntl(pair[1], F_SETFL, flags & ~O_NONBLOCK) != 0) {
        if (pair[0] >= 0) close(pair[0]);
        if (pair[1] >= 0) close(pair[1]);
        close(output); (void)unlink(diagnostic); return 127;
    }
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
            dup2(write_fd, STDOUT_FILENO) < 0 || fcntl(STDOUT_FILENO, F_SETFD, 0) != 0) _exit(127);
        if (write_fd != STDOUT_FILENO) close(write_fd);
        if (dup2(pair[1], STDERR_FILENO) < 0) _exit(127);
        close(pair[0]);
        if (pair[1] != STDERR_FILENO) close(pair[1]);
        close(output);
        execve(runtime->grease, argv, environ);
        _exit(127);
    }
    close(pair[1]);
    int result = 127, failed = 0;
    size_t saved = 0;
    if (pid < 0) goto finished;
    if (setpgid(pid, pid) != 0 && errno != EACCES && errno != ESRCH) {
        (void)kill(pid, SIGKILL);
        while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) { }
        goto finished;
    }
    int64_t start = monotonic_milliseconds();
    int64_t deadline = start < 0 ? 0 : start + BROKER_PROCESS_DEADLINE_MS;
    for (;;) {
        drain_stderr(pair[0], output, &saved, &failed);
        int status = 0;
        siginfo_t child = {0};
        int observed = waitid(P_PID, (id_t)pid, &child, WEXITED | WNOHANG | WNOWAIT);
        if (observed == 0 && child.si_pid == pid) {
            /* Reap only after group cleanup so this PID cannot be reused. */
            (void)kill(-pid, SIGKILL);
            while (waitpid(pid, &status, 0) < 0) {
                if (errno != EINTR) { result = 125; goto finished; }
            }
            if (WIFEXITED(status)) result = WEXITSTATUS(status);
            else if (WIFSIGNALED(status)) result = 128 + WTERMSIG(status);
            else result = 125;
            break;
        }
        if (observed < 0 && errno != EINTR) {
            (void)kill(-pid, SIGKILL);
            (void)kill(pid, SIGKILL);
            while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) { }
            result = 125;
            break;
        }
        int64_t now = monotonic_milliseconds();
        if (now < 0 || now >= deadline) {
            (void)kill(-pid, SIGKILL);
            (void)kill(pid, SIGKILL);
            while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) { }
            result = 124;
            break;
        }
        struct timespec interval = {0, 50000000L};
        while (nanosleep(&interval, &interval) != 0 && errno == EINTR) { }
    }
finished:
    drain_stderr(pair[0], output, &saved, &failed);
    close(pair[0]);
    if (close(output) != 0) failed = 1;
    if (result == 0 && failed) result = 127;
    if (result == 0) (void)unlink(diagnostic); /* Exact own temporary leaf. */
    __android_log_print(ANDROID_LOG_INFO, "IBReader", "private worker diagnostics bytes=%zu io_failure=%d", saved, failed);
    return result > 255 ? 255 : result;
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
