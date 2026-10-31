#define _POSIX_C_SOURCE 200809L
#include "ib_shizuku_ingress_runner.h"

#if defined(__ANDROID__) && __ANDROID_API__ >= 29
#include "ib_shizuku_ingress_binder.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
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
        (info.st_uid != geteuid() && info.st_uid != 0))
        return 0;
    return !executable || (info.st_mode & 0111U) != 0;
}

static int run_process(const char *qualified_grease, const char *worker_script,
                       const char *authorized_url, int write_fd) {
    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) return 127;
    int actions_status =
        posix_spawn_file_actions_adddup2(&actions, write_fd, STDOUT_FILENO);
    if (actions_status == 0 && write_fd != STDOUT_FILENO)
        actions_status = posix_spawn_file_actions_addclose(&actions, write_fd);

    pid_t pid = -1;
    int spawned = actions_status;
    if (spawned == 0) {
        char *const argv[] = {
            (char *)qualified_grease, (char *)worker_script,
            (char *)authorized_url, NULL
        };
        spawned = posix_spawn(&pid, qualified_grease, &actions,
                              NULL, argv, environ);
    }
    (void)posix_spawn_file_actions_destroy(&actions);
    if (spawned != 0 || pid <= 0) return 127;

    int64_t start = monotonic_milliseconds();
    int64_t deadline = start < 0 ? 0 : start + BROKER_PROCESS_DEADLINE_MS;
    for (;;) {
        int status = 0;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
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
            (void)kill(pid, SIGKILL);
            while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) { }
            return 124;
        }
        struct timespec interval = {0, 50000000L};
        while (nanosleep(&interval, &interval) != 0 && errno == EINTR) { }
    }
}

int ib_shizuku_ingress_run_icu(AIBinder *private_ingress,
                               const char *request_id,
                               uint64_t navigation_generation,
                               const char *qualified_grease,
                               const char *trusted_worker_script) {
    if (!private_ingress || !request_id || navigation_generation == 0)
        return -1;

    char authorized_url[IB_INGRESS_URL_CAP + 1U] = {0};
    int write_fd = -1;
    if (ib_shizuku_ingress_begin_remote(
            private_ingress, request_id, navigation_generation,
            authorized_url, &write_fd) != 0 || write_fd < 0)
        return -1;

    int result = 127;
    if (approved_executable_path(qualified_grease, 1) &&
        approved_executable_path(trusted_worker_script, 0))
        result = run_process(qualified_grease, trusted_worker_script,
                             authorized_url, write_fd);

    /* Always close the only broker-side copy BEFORE FINISH. Otherwise a
     * held write reference prevents the app from observing the exact EOF. */
    close(write_fd);
    int published = ib_shizuku_ingress_finish_remote(
        private_ingress, request_id, navigation_generation, result);
    return result == 0 && published == 0 ? 0 : -1;
}
#endif
