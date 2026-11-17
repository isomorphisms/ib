#define _POSIX_C_SOURCE 200809L
#include "ib_admin_transaction.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef O_CLOEXEC
#error "Administrative journal requires close-on-exec descriptors"
#endif
#ifndef O_NOFOLLOW
#error "Administrative journal requires symlink-safe openat"
#endif

static int safe_token(const char *value, size_t maximum) {
    if (!value || !value[0]) return 0;
    size_t length = strnlen(value, maximum + 1);
    if (length > maximum || !length) return 0;
    for (size_t index = 0; index < length; ++index) {
        unsigned char c = (unsigned char)value[index];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_')) return 0;
    }
    return 1;
}

static int private_directory(int fd) {
    struct stat st;
    return fd >= 0 && fstat(fd, &st) == 0 && S_ISDIR(st.st_mode) &&
           st.st_uid == geteuid() && (st.st_mode & 077U) == 0;
}

static int open_private_child(int parent, const char *name) {
    if (mkdirat(parent, name, 0700) != 0 && errno != EEXIST) return -1;
    int child = openat(parent, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (!private_directory(child)) {
        if (child >= 0) close(child);
        return -1;
    }
    return child;
}

static int event_name(uint32_t revision, char name[32]) {
    int count = snprintf(name, 32, "%08u.event", revision);
    return count == 14 ? 0 : -1;
}

/* Exactly one event per file. No plaintext remote source, credential, key or
 * response body belongs in these files. The file never gets rewritten. */
static int format_event(uint32_t revision, char code, const char *label,
                        const char *operation, char data[256]) {
    int count = snprintf(data, 256, "IBADMIN1\t%u\t%c\t%s\t%s\n",
                         revision, code, label ? label : "-", operation ? operation : "-");
    return count > 0 && count < 256 ? count : -1;
}

static int parse_event(const char *data, size_t count, uint32_t revision,
                       char *code, char label[IB_ADMIN_LABEL_CAP + 1],
                       char operation[IB_ADMIN_OPERATION_CAP + 1]) {
    if (count < 17 || count >= 256 || data[count - 1] != '\n') return -1;
    char copy[256];
    memcpy(copy, data, count);
    copy[count - 1] = '\0';
    char *save = NULL;
    char *schema = strtok_r(copy, "\t", &save);
    char *revision_text = strtok_r(NULL, "\t", &save);
    char *event = strtok_r(NULL, "\t", &save);
    char *field = strtok_r(NULL, "\t", &save);
    char *identity = strtok_r(NULL, "\t", &save);
    if (!schema || !revision_text || !event || !field || !identity ||
        strtok_r(NULL, "\t", &save) != NULL ||
        strcmp(schema, "IBADMIN1") || strlen(event) != 1) return -1;
    char number[32];
    if (snprintf(number, sizeof(number), "%u", revision) < 1 ||
        strcmp(number, revision_text)) return -1;
    if (strlen(field) > IB_ADMIN_LABEL_CAP ||
        strlen(identity) > IB_ADMIN_OPERATION_CAP) return -1;
    char canonical[256];
    int canonical_length = format_event(revision, event[0], field, identity, canonical);
    if (canonical_length < 0 || (size_t)canonical_length != count ||
        memcmp(canonical, data, count)) return -1;
    if (strcmp(field, "-") && !safe_token(field, IB_ADMIN_LABEL_CAP)) return -1;
    if (strcmp(identity, "-") && !safe_token(identity, IB_ADMIN_OPERATION_CAP)) return -1;
    *code = event[0];
    strcpy(label, field);
    strcpy(operation, identity);
    return 0;
}

static int load_event(ib_admin_transaction *transaction, uint32_t revision) {
    char filename[32];
    if (event_name(revision, filename) != 0) return -1;
    int fd = openat(transaction->directory_fd, filename,
                    O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return errno == ENOENT ? 1 : -1;
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != geteuid() ||
        (st.st_mode & 077U) || st.st_size < 17 || st.st_size >= 256) {
        close(fd);
        return -1;
    }
    char data[256];
    ssize_t count = read(fd, data, sizeof(data));
    int ok = count == st.st_size;
    if (close(fd) != 0 || !ok) return -1;
    char code = 0, label[IB_ADMIN_LABEL_CAP + 1], op[IB_ADMIN_OPERATION_CAP + 1];
    if (parse_event(data, (size_t)count, revision, &code, label, op) != 0) return -1;
    switch (code) {
        case 'D':
            if (transaction->phase != IB_ADMIN_NEW && transaction->phase != IB_ADMIN_DRAFT) return -1;
            if (!strcmp(label, "-") || strcmp(op, "-")) return -1;
            transaction->phase = IB_ADMIN_DRAFT;
            strcpy(transaction->ordinary_label, label);
            break;
        case 'P':
            if (transaction->phase != IB_ADMIN_DRAFT || strcmp(label, transaction->ordinary_label) ||
                !strcmp(op, "-")) return -1;
            transaction->phase = IB_ADMIN_PREPARED;
            strcpy(transaction->operation_id, op);
            break;
        case 'S':
            if (transaction->phase != IB_ADMIN_PREPARED ||
                strcmp(label, transaction->ordinary_label) ||
                strcmp(op, transaction->operation_id)) return -1;
            transaction->phase = IB_ADMIN_MAY_HAVE_APPLIED;
            break;
        case 'O':
            if (transaction->phase != IB_ADMIN_MAY_HAVE_APPLIED ||
                strcmp(label, transaction->ordinary_label) ||
                strcmp(op, transaction->operation_id)) return -1;
            transaction->phase = IB_ADMIN_OBSERVED_APPLIED;
            break;
        default: return -1;
    }
    transaction->revision = revision;
    return 0;
}

int ib_admin_open(ib_admin_transaction *transaction, const char *app_files_directory,
                  const char *task_id) {
    if (!transaction || !app_files_directory || !safe_token(task_id, IB_ADMIN_TASK_CAP)) return -1;
    memset(transaction, 0, sizeof(*transaction));
    transaction->directory_fd = -1;
    int root = open(app_files_directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (!private_directory(root)) { if (root >= 0) close(root); return -1; }
    int journals = open_private_child(root, "admin-action-journal");
    close(root);
    if (journals < 0) return -1;
    int task = open_private_child(journals, task_id);
    close(journals);
    if (task < 0) return -1;
    transaction->directory_fd = task;
    for (uint32_t index = 1; index <= IB_ADMIN_MAX_EVENTS; ++index) {
        int status = load_event(transaction, index);
        if (status == 1) {
            /* A missing event inside the bounded journal may not conceal a
             * higher numbered event after a crash or hostile substitution. */
            for (uint32_t next = index + 1; next <= IB_ADMIN_MAX_EVENTS + 1; ++next) {
                char filename[32];
                if (event_name(next, filename) != 0) goto fail;
                struct stat st;
                if (fstatat(task, filename, &st, AT_SYMLINK_NOFOLLOW) == 0) goto fail;
                if (errno != ENOENT) goto fail;
            }
            return 0;
        }
        if (status != 0) goto fail;
    }
    /* Do not silently accept an unbounded continuation after the cap. */
    char extra[32];
    struct stat extra_stat;
    if (event_name(IB_ADMIN_MAX_EVENTS + 1, extra) != 0 ||
        fstatat(task, extra, &extra_stat, AT_SYMLINK_NOFOLLOW) == 0 || errno != ENOENT)
        goto fail;
    return transaction->revision == IB_ADMIN_MAX_EVENTS ? 0 : -1;
fail:
    ib_admin_close(transaction);
    return -1;
}

void ib_admin_close(ib_admin_transaction *transaction) {
    if (!transaction) return;
    if (transaction->directory_fd >= 0) close(transaction->directory_fd);
    transaction->directory_fd = -1;
    memset(transaction->ordinary_label, 0, sizeof(transaction->ordinary_label));
    memset(transaction->operation_id, 0, sizeof(transaction->operation_id));
    transaction->revision = 0;
    transaction->phase = IB_ADMIN_NEW;
}

static int append_event(ib_admin_transaction *transaction, char code,
                        const char *label, const char *operation) {
    if (!transaction || transaction->directory_fd < 0 ||
        transaction->revision >= IB_ADMIN_MAX_EVENTS) return -1;
    uint32_t revision = transaction->revision + 1;
    char filename[32], data[256];
    int count = format_event(revision, code, label, operation, data);
    if (event_name(revision, filename) != 0 || count < 0) return -1;
    int fd = openat(transaction->directory_fd, filename,
                    O_CREAT | O_EXCL | O_WRONLY | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (fd < 0) return -1;
    ssize_t written = write(fd, data, (size_t)count);
    int ok = written == count && fsync(fd) == 0;
    if (close(fd) != 0 || !ok || fsync(transaction->directory_fd) != 0) return -1;
    /* Read our own immutable durable event through the exact replay validator. */
    return load_event(transaction, revision);
}

int ib_admin_checkpoint_label(ib_admin_transaction *transaction, const char *ordinary_label) {
    if (!transaction || (transaction->phase != IB_ADMIN_NEW && transaction->phase != IB_ADMIN_DRAFT) ||
        !safe_token(ordinary_label, IB_ADMIN_LABEL_CAP)) return -1;
    return append_event(transaction, 'D', ordinary_label, "-");
}

int ib_admin_prepare(ib_admin_transaction *transaction, const char *operation_id) {
    if (!transaction || transaction->phase != IB_ADMIN_DRAFT ||
        !safe_token(operation_id, IB_ADMIN_OPERATION_CAP)) return -1;
    return append_event(transaction, 'P', transaction->ordinary_label, operation_id);
}

int ib_admin_can_dispatch(const ib_admin_transaction *transaction) {
    return transaction && transaction->directory_fd >= 0 && transaction->phase == IB_ADMIN_PREPARED;
}

int ib_admin_mark_may_have_applied(ib_admin_transaction *transaction) {
    if (!ib_admin_can_dispatch(transaction)) return -1;
    return append_event(transaction, 'S', transaction->ordinary_label, transaction->operation_id);
}

int ib_admin_observe_applied(ib_admin_transaction *transaction, const char *operation_id) {
    if (!transaction || transaction->phase != IB_ADMIN_MAY_HAVE_APPLIED ||
        !operation_id || strcmp(operation_id, transaction->operation_id)) return -1;
    return append_event(transaction, 'O', transaction->ordinary_label, transaction->operation_id);
}

const char *ib_admin_phase_name(ib_admin_phase phase) {
    switch (phase) {
        case IB_ADMIN_NEW: return "NEW";
        case IB_ADMIN_DRAFT: return "DRAFT";
        case IB_ADMIN_PREPARED: return "PREPARED";
        case IB_ADMIN_MAY_HAVE_APPLIED: return "MAY_HAVE_APPLIED";
        case IB_ADMIN_OBSERVED_APPLIED: return "OBSERVED_APPLIED";
        default: return "INVALID";
    }
}
