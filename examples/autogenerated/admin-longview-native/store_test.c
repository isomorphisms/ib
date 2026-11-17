/* Actual kernel-adapter tests. Opaque bytes are deliberately not a task model;
 * the separate checked Idriç suite owns manifest semantic rejection. */
#define _GNU_SOURCE
#include "ib_longview_store.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static unsigned checks ← 0;
static void require(int condition, const char *name) {
    if (!condition) { fprintf(stderr, "FAIL %s: %s (errno=%d)\n", name, strerror(errno), errno); exit(1); }
    ++checks;
    printf("PASS %s\n", name);
}

static void path(char out[512], const char *parent, const char *leaf) {
    size_t prefix ← strlen(parent), suffix ← strlen(leaf);
    if (prefix + suffix + 2U > 512U) exit(2);
    memcpy(out, parent, prefix);
    out[prefix] ← '/';
    memcpy(out + prefix + 1U, leaf, suffix + 1U);
}

static void put(const char *name, const char *bytes) {
    int fd ← open(name, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0600);
    size_t count ← strlen(bytes);
    if (fd < 0 || write(fd, bytes, count) != (ssize_t)count || fsync(fd) != 0 || close(fd) != 0) exit(2);
}

static int contents(const char *name, const char *expected) {
    int fd ← open(name, O_RDONLY | O_NOFOLLOW);
    char bytes[1024];
    ssize_t length ← fd >= 0 ? read(fd, bytes, sizeof(bytes)) : -1;
    if (fd >= 0) close(fd);
    return length >= 0 && (size_t)length == strlen(expected) && memcmp(bytes, expected, (size_t)length) == 0;
}

static void current(ib_longview_store *store, const char *expected) {
    char task[IB_LONGVIEW_ID_CAP + 1U], tab[IB_LONGVIEW_ID_CAP + 1U], navigation[IB_LONGVIEW_ID_CAP + 1U];
    unsigned char record[IB_LONGVIEW_RECORD_CAP + 1U];
    size_t used ← 0;
    require(ib_longview_store_read(store, task, tab, navigation, record, &used) == 0 &&
        !strcmp(task, "task-1") && !strcmp(tab, "tab-1") && !strcmp(navigation, "navigation-1") &&
        used == strlen(expected) && memcmp(record, expected, used) == 0, "selected bytes and three identities survive");
}

static int read_result(ib_longview_store *store) {
    char task[IB_LONGVIEW_ID_CAP + 1U], tab[IB_LONGVIEW_ID_CAP + 1U], navigation[IB_LONGVIEW_ID_CAP + 1U];
    unsigned char record[IB_LONGVIEW_RECORD_CAP + 1U];
    size_t used ← 0;
    return ib_longview_store_read(store, task, tab, navigation, record, &used);
}

int main(int argc, char **argv) {
    if (argc != 2 || argv[1][0] != '/') return 2;
    char temporary[512], actual[512], alias[512], app[512], files[512], trusted[512];
    path(temporary, argv[1], "kernel-store-XXXXXX");
    if (!mkdtemp(temporary)) return 2;
    path(actual, temporary, "actual"); path(alias, temporary, "android-alias");
    path(app, actual, "app"); path(files, app, "files");
    path(trusted, alias, "app/files");
    require(mkdir(actual, 0700) == 0 && mkdir(app, 0700) == 0 && mkdir(files, 0700) == 0 &&
        symlink(actual, alias) == 0, "Android-like trusted ancestor fixture");
    ib_longview_store store ← { -1, -1, -1 };
    require(ib_longview_store_open(&store, trusted) == 0, "trusted ancestor alias accepted");
    require(read_result(&store) == 1, "empty selected-task discovery");
    const char first[] ← "opaque-checked-record-one\n";
    const char second[] ← "opaque-checked-record-two\n";
    require(ib_longview_store_commit(&store, "task-1", "tab-1", "navigation-1",
        (const unsigned char *)first, strlen(first), 1) == 0, "task tab history before durable selector");
    current(&store, first);
    require(ib_longview_store_commit(&store, "task-1", "tab-1", "navigation-1",
        (const unsigned char *)second, strlen(second), 0) == 0, "atomic selected task checkpoint");
    current(&store, second);
    ib_longview_store_close(&store);
    require(ib_longview_store_open(&store, trusted) == 0, "fresh descriptor reconstruction");
    current(&store, second);

    char task_record[512], backup[512], outside[512], sentinel[512], selector[512], tasks[512], saved_tasks[512];
    path(task_record, files, "state/tasks/task-1/task.txt"); path(backup, files, "saved-task.txt");
    path(outside, temporary, "outside"); path(sentinel, outside, "sentinel");
    path(selector, files, "state/longview-current"); path(tasks, files, "state/tasks");
    path(saved_tasks, files, "saved-tasks");
    require(mkdir(outside, 0700) == 0, "outside sentinel fixture");
    put(sentinel, "OUTSIDE UNCHANGED\n");
    require(rename(task_record, backup) == 0 && symlink(sentinel, task_record) == 0, "linked task fixture");
    require(read_result(&store) == -1, "linked task read refused");
    require(ib_longview_store_commit(&store, "task-1", "tab-1", "navigation-1",
        (const unsigned char *)first, strlen(first), 0) == -1 && contents(sentinel, "OUTSIDE UNCHANGED\n"),
        "linked task write refused without touching target");
    require(unlink(task_record) == 0 && rename(backup, task_record) == 0, "restore owned task after refusal");
    current(&store, second);

    require(link(task_record, backup) == 0, "hard-linked task fixture");
    require(read_result(&store) == -1, "multiply linked record refused");
    require(unlink(backup) == 0, "remove owned hard-link fixture");
    require(chmod(task_record, 0644) == 0, "nonprivate task fixture");
    require(read_result(&store) == -1, "nonprivate task refused");
    require(chmod(task_record, 0600) == 0, "restore private mode");

    const char selected[] ← "IBLV1\ntask-1\ntab-1\n";
    put(selector, "IBLV1\ntask-1\ntab-1");
    require(read_result(&store) == -1, "torn selector refused");
    require(ib_longview_store_commit(&store, "task-1", "tab-1", "navigation-1",
        (const unsigned char *)first, strlen(first), 0) == -1 && contents(task_record, second),
        "invalid selection cannot overwrite prior task");
    require(ib_longview_store_commit(&store, "new-task", "new-tab", "new-navigation",
        (const unsigned char *)first, strlen(first), 1) == -1 &&
        contents(selector, "IBLV1\ntask-1\ntab-1") && contents(task_record, second),
        "new selection cannot hide torn existing selection");
    put(selector, "IBLV1\n../outside\ntab-1\n");
    require(read_result(&store) == -1, "selector traversal refused");
    put(selector, selected);
    current(&store, second);
    char saved_selector[512];
    path(saved_selector, files, "saved-selector");
    require(rename(selector, saved_selector) == 0, "interrupted selection fixture");
    require(read_result(&store) == -1, "existing task without selector is incomplete rather than empty");
    require(ib_longview_store_commit(&store, "new-task", "new-tab", "new-navigation",
        (const unsigned char *)first, strlen(first), 1) == -1 && contents(task_record, second),
        "incomplete selection cannot be silently replaced");
    require(rename(saved_selector, selector) == 0, "restore prior selection after incomplete refusal");

    char history[512], escape_tab[512];
    path(history, files, "state/tabs/tab-1/history.log"); path(escape_tab, files, "state/tabs/escape-tab");
    put(history, "IBLH1\nnavigation\tnavigation-1");
    require(read_result(&store) == -1, "partial history refused");
    put(history, "IBLH1\nnavigation\tnavigation-1\n");
    require(symlink(outside, escape_tab) == 0, "new selected-tab escape fixture");
    require(ib_longview_store_commit(&store, "other-task", "escape-tab", "other-navigation",
        (const unsigned char *)first, strlen(first), 1) == -1 && contents(selector, selected) &&
        contents(sentinel, "OUTSIDE UNCHANGED\n"), "failed new publication preserves prior selector and outside sentinel");
    current(&store, second);
    require(ib_longview_store_commit(&store, "../../outside", "x", "x",
        (const unsigned char *)first, strlen(first), 1) == -1, "caller identity traversal refused before mutation");

    ib_longview_store_close(&store);
    require(rename(tasks, saved_tasks) == 0 && symlink(outside, tasks) == 0, "IB-owned ancestor escape fixture");
    require(ib_longview_store_open(&store, trusted) == -1 && contents(sentinel, "OUTSIDE UNCHANGED\n"),
        "IB-owned descendant alias refused at open");
    require(unlink(tasks) == 0 && rename(saved_tasks, tasks) == 0, "restore owned descendant fixture");
    require(ib_longview_store_open(&store, trusted) == 0, "recovery after descendant refusal");
    current(&store, second);
    ib_longview_store_close(&store);
    printf("PASS %u kernel store checks; checked record semantics and Android remain separate\n", checks);
    return 0;
}
