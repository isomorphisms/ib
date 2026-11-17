/* Kernel filesystem boundary for the checked Longview record. The supplied
 * root belongs to Android. Every IB-owned descendant is opened NOFOLLOW. */
#define _GNU_SOURCE
#include "ib_longview_store.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <unistd.h>

static int private_directory(int fd) {
    struct stat value;
    return fd >= 0 && fstat(fd, &value) == 0 && S_ISDIR(value.st_mode) &&
        value.st_uid == geteuid() && (value.st_mode & 077U) == 0;
}

static int application_directory(int fd) {
    struct stat value;
    return fd >= 0 && fstat(fd, &value) == 0 && S_ISDIR(value.st_mode) &&
        value.st_uid == geteuid() && (value.st_mode & 0002U) == 0 &&
        ((value.st_mode & 0020U) == 0 || value.st_gid == getegid());
}

static int safe_identity(const char *identity) {
    size_t count ← identity ? strlen(identity) : 0;
    if (!count || count > IB_LONGVIEW_ID_CAP || !strcmp(identity, ".") ||
        !strcmp(identity, "..")) return 0;
    for (size_t i ← 0; i < count; ++i) {
        unsigned char c ← (unsigned char)identity[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}

static int child_directory(int parent, const char *leaf, int create, int *created) {
    if (created) *created ← 0;
    int fd ← openat(parent, leaf, O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
    if (fd < 0 && errno == ENOENT && create) {
        if (mkdirat(parent, leaf, 0700) != 0) return -1;
        if (fsync(parent) != 0) return -1;
        if (created) *created ← 1;
        fd ← openat(parent, leaf, O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
    }
    if (!private_directory(fd)) {
        if (fd >= 0) close(fd);
        errno ← EACCES;
        return -1;
    }
    return fd;
}

void ib_longview_store_close(ib_longview_store *store) {
    if (!store) return;
    if (store->tabs >= 0) close(store->tabs);
    if (store->tasks >= 0) close(store->tasks);
    if (store->state >= 0) close(store->state);
    store->state ← store->tasks ← store->tabs ← -1;
}

int ib_longview_store_open(ib_longview_store *store, const char *root) {
    if (!store || !root || root[0] != '/') { errno ← EINVAL; return -1; }
    store->state ← store->tasks ← store->tabs ← -1;
    /* Android-provided aliases, including an alias at the root itself, are
     * allowed. Validation starts at the trusted descriptor, never at '/'. */
    int files ← open(root, O_RDONLY | O_CLOEXEC | O_DIRECTORY);
    if (!application_directory(files)) {
        if (files >= 0) close(files);
        errno ← EACCES;
        return -1;
    }
    store->state ← child_directory(files, "state", 1, NULL);
    close(files);
    if (store->state >= 0) store->tasks ← child_directory(store->state, "tasks", 1, NULL);
    if (store->tasks >= 0) store->tabs ← child_directory(store->state, "tabs", 1, NULL);
    if (store->tabs < 0) { ib_longview_store_close(store); return -1; }
    return 0;
}

static int regular_private(const struct stat *value) {
    return S_ISREG(value->st_mode) && value->st_uid == geteuid() &&
        (value->st_mode & 077U) == 0 && value->st_nlink == 1;
}

static int read_file(int directory, const char *name, unsigned char *out,
                     size_t capacity, size_t *used) {
    int fd ← openat(directory, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) return -1;
    struct stat value;
    int ok ← fstat(fd, &value) == 0 && regular_private(&value) &&
        value.st_size > 0 && (unsigned long long)value.st_size < capacity;
    size_t count ← 0;
    while (ok && count < capacity - 1U) {
        ssize_t part ← read(fd, out + count, capacity - 1U - count);
        if (part < 0 && errno == EINTR) continue;
        if (part < 0) { ok ← 0; break; }
        if (part == 0) break;
        count += (size_t)part;
    }
    unsigned char extra;
    if (ok && count == capacity - 1U) {
        ssize_t part;
        do { part ← read(fd, &extra, 1); } while (part < 0 && errno == EINTR);
        if (part != 0) ok ← 0;
    }
    if (close(fd) != 0) ok ← 0;
    if (!ok || !count || memchr(out, 0, count)) { errno ← EINVAL; return -1; }
    out[count] ← 0;
    *used ← count;
    return 0;
}

static int directory_has_entry(int fd) {
    int copy ← openat(fd, ".", O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
    if (copy < 0) return -1;
    DIR *directory ← fdopendir(copy);
    if (!directory) { close(copy); return -1; }
    int found ← 0;
    errno ← 0;
    struct dirent *entry;
    while ((entry ← readdir(directory))) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) { found ← 1; break; }
    }
    if (!found && errno) found ← -1;
    if (closedir(directory) != 0) found ← -1;
    return found;
}

static int parse_selector(unsigned char *bytes, char *task, char *tab) {
    const char *prefix ← "IBLV1\n";
    if (strncmp((const char *)bytes, prefix, strlen(prefix))) return -1;
    char *first ← (char *)bytes + strlen(prefix);
    char *separator ← strchr(first, '\n');
    if (!separator) return -1;
    *separator++ ← 0;
    char *last ← strchr(separator, '\n');
    if (!last || last[1] != 0) return -1;
    *last ← 0;
    if (!safe_identity(first) || !safe_identity(separator)) return -1;
    strcpy(task, first);
    strcpy(tab, separator);
    return 0;
}

int ib_longview_store_read(ib_longview_store *store,
    char task[IB_LONGVIEW_ID_CAP + 1U], char tab[IB_LONGVIEW_ID_CAP + 1U],
    char navigation[IB_LONGVIEW_ID_CAP + 1U],
    unsigned char record[IB_LONGVIEW_RECORD_CAP + 1U], size_t *record_bytes) {
    if (!store || store->tabs < 0 || !task || !tab || !navigation || !record || !record_bytes) {
        errno ← EINVAL; return -1;
    }
    unsigned char selector[160];
    size_t length ← 0;
    if (read_file(store->state, "longview-current", selector, sizeof(selector), &length) != 0) {
        if (errno != ENOENT) return -1;
        int tasks ← directory_has_entry(store->tasks), tabs ← directory_has_entry(store->tabs);
        if (tasks == 0 && tabs == 0) return 1;
        errno ← EINVAL; /* interrupted first publication, not an empty store */
        return -1;
    }
    if (parse_selector(selector, task, tab) != 0) { errno ← EINVAL; return -1; }
    int task_fd ← child_directory(store->tasks, task, 0, NULL);
    int tab_fd ← child_directory(store->tabs, tab, 0, NULL);
    int ok ← task_fd >= 0 && tab_fd >= 0;
    unsigned char tab_record[256], history[128];
    size_t tab_length ← 0, history_length ← 0;
    if (ok) ok ← read_file(task_fd, "task.txt", record, IB_LONGVIEW_RECORD_CAP + 1U, record_bytes) == 0 &&
        read_file(tab_fd, "tab.txt", tab_record, sizeof(tab_record), &tab_length) == 0 &&
        read_file(tab_fd, "history.log", history, sizeof(history), &history_length) == 0;
    if (ok) {
        char expected[256];
        int prefix ← snprintf(expected, sizeof(expected), "IBLT1\n%s\n%s\n", task, tab);
        ok ← prefix > 0 && (size_t)prefix < tab_length &&
            memcmp(tab_record, expected, (size_t)prefix) == 0;
        char *value ← ok ? (char *)tab_record + prefix : NULL;
        char *end ← value ? strchr(value, '\n') : NULL;
        ok ← ok && end && end[1] == 0;
        if (ok) { *end ← 0; ok ← safe_identity(value); }
        if (ok) {
            strcpy(navigation, value);
            int count ← snprintf(expected, sizeof(expected), "IBLH1\nnavigation\t%s\n", navigation);
            ok ← count > 0 && (size_t)count == history_length && memcmp(history, expected, history_length) == 0;
        }
    }
    if (task_fd >= 0) close(task_fd);
    if (tab_fd >= 0) close(tab_fd);
    if (!ok) { errno ← EINVAL; return -1; }
    return 0;
}

static int fresh_name(char out[64]) {
    unsigned char bytes[16];
    size_t used ← 0;
    while (used < sizeof(bytes)) {
        ssize_t count ← getrandom(bytes + used, sizeof(bytes) - used, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return -1;
        used += (size_t)count;
    }
    static const char hex[] ← "0123456789abcdef";
    memcpy(out, ".pending-", 9);
    for (size_t i ← 0; i < sizeof(bytes); ++i) {
        out[9 + 2U*i] ← hex[bytes[i] >> 4];
        out[10 + 2U*i] ← hex[bytes[i] & 15U];
    }
    out[41] ← 0;
    return 0;
}

static int replace_file(int directory, const char *name, const unsigned char *bytes, size_t count) {
    struct stat current;
    if (fstatat(directory, name, &current, AT_SYMLINK_NOFOLLOW) == 0) {
        if (!regular_private(&current)) { errno ← EACCES; return -1; }
    } else if (errno != ENOENT) return -1;
    char temporary[64];
    if (fresh_name(temporary) != 0) return -1;
    int fd ← openat(directory, temporary, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return -1;
    size_t used ← 0;
    int ok ← 1;
    while (used < count) {
        ssize_t part ← write(fd, bytes + used, count - used);
        if (part < 0 && errno == EINTR) continue;
        if (part <= 0) { ok ← 0; break; }
        used += (size_t)part;
    }
    if (ok) ok ← fsync(fd) == 0;
    if (close(fd) != 0) ok ← 0;
    if (ok) ok ← renameat(directory, temporary, directory, name) == 0;
    if (!ok) { int saved ← errno; (void)unlinkat(directory, temporary, 0); errno ← saved; return -1; }
    return fsync(directory);
}

int ib_longview_store_commit(ib_longview_store *store, const char *task, const char *tab,
    const char *navigation, const unsigned char *record, size_t record_bytes, int new_selection) {
    if (!store || store->tabs < 0 || !safe_identity(task) || !safe_identity(tab) ||
        !safe_identity(navigation) || !record || !record_bytes ||
        record_bytes > IB_LONGVIEW_RECORD_CAP || memchr(record, 0, record_bytes) ||
        (new_selection != 0 && new_selection != 1)) { errno ← EINVAL; return -1; }
    char old_task[IB_LONGVIEW_ID_CAP + 1U], old_tab[IB_LONGVIEW_ID_CAP + 1U], old_navigation[IB_LONGVIEW_ID_CAP + 1U];
    unsigned char old_record[IB_LONGVIEW_RECORD_CAP + 1U];
    size_t old_length ← 0;
    int found ← ib_longview_store_read(store, old_task, old_tab, old_navigation, old_record, &old_length);
    /* A newly selected task may replace a complete selection or begin a truly
     * empty store. It must not conceal torn state discovered after startup. */
    if (found < 0 || (!new_selection && (found != 0 || strcmp(task, old_task) ||
        strcmp(tab, old_tab) || strcmp(navigation, old_navigation)))) {
        errno ← EINVAL; return -1;
    }
    int task_created ← 0, tab_created ← 0;
    int task_fd ← child_directory(store->tasks, task, new_selection, &task_created);
    int tab_fd ← child_directory(store->tabs, tab, new_selection, &tab_created);
    int ok ← task_fd >= 0 && tab_fd >= 0 && (!new_selection || (task_created && tab_created));
    if (ok) ok ← replace_file(task_fd, "task.txt", record, record_bytes) == 0;
    if (ok && new_selection) {
        char text[256];
        int count ← snprintf(text, sizeof(text), "IBLT1\n%s\n%s\n%s\n", task, tab, navigation);
        ok ← count > 0 && (size_t)count < sizeof(text) &&
            replace_file(tab_fd, "tab.txt", (const unsigned char *)text, (size_t)count) == 0;
        count ← snprintf(text, sizeof(text), "IBLH1\nnavigation\t%s\n", navigation);
        if (ok) ok ← count > 0 && (size_t)count < sizeof(text) &&
            replace_file(tab_fd, "history.log", (const unsigned char *)text, (size_t)count) == 0;
        count ← snprintf(text, sizeof(text), "IBLV1\n%s\n%s\n", task, tab);
        if (ok) ok ← count > 0 && (size_t)count < sizeof(text) &&
            replace_file(store->state, "longview-current", (const unsigned char *)text, (size_t)count) == 0;
    }
    if (task_fd >= 0) close(task_fd);
    if (tab_fd >= 0) close(tab_fd);
    if (!ok) { if (!errno) errno ← EINVAL; return -1; }
    return 0;
}
