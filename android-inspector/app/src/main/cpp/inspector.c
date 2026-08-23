#define _GNU_SOURCE
#include <android/log.h>
#include <android/native_activity.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <jni.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define LOG_TAG "ib-inspector"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define MAX_SCAN_ROWS 512
#define MAX_DISPLAY_FILES 120
#define MAX_TAB_NAMES 128
#define MAX_DISPLAY_TABS 20
#define MAX_TAB_BYTES (64u * 1024u)
#define MAX_SUMMARY_BYTES (64u * 1024u)
#define MAX_TAIL_BYTES (1024u * 1024u)
#define MAX_TAIL_RECORDS 20
#define MAX_PATH_TEXT 768
#define MAX_DEPTH 32

typedef enum {
    KIND_CANONICAL = 0,
    KIND_DERIVED,
    KIND_SNAPSHOT,
    KIND_CACHE,
    KIND_SECRET,
    KIND_UNKNOWN,
    KIND_COUNT
} StoreKind;

typedef enum {
    FILE_REGULAR = 0,
    FILE_SYMLINK,
    FILE_OTHER
} FileType;

typedef struct {
    char path[MAX_PATH_TEXT];
    uint64_t bytes;
    StoreKind kind;
    FileType type;
    bool readable;
} FileRow;

typedef struct {
    uint64_t file_count;
    uint64_t byte_count;
    uint64_t tab_records;
    uint64_t files_by_kind[KIND_COUNT];
    uint64_t bytes_by_kind[KIND_COUNT];
    FileRow rows[MAX_SCAN_ROWS];
    size_t row_count;
    bool rows_truncated;
} ScanResult;

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} TextBuffer;

typedef struct {
    ANativeActivity *activity;
    jobject text_view;
} InspectorActivity;

static const char *kind_name(StoreKind kind) {
    static const char *const names[KIND_COUNT] = {
        "canonical", "derived", "snapshot", "cache", "secret", "unknown"
    };
    return (kind >= 0 && kind < KIND_COUNT) ? names[kind] : "unknown";
}

static const char *type_name(FileType type) {
    switch (type) {
        case FILE_REGULAR: return "file";
        case FILE_SYMLINK: return "symlink";
        case FILE_OTHER: return "other";
    }
    return "other";
}

static bool text_reserve(TextBuffer *text, size_t extra) {
    if (extra > SIZE_MAX - text->len - 1) {
        return false;
    }
    size_t needed = text->len + extra + 1;
    if (needed <= text->cap) {
        return true;
    }
    size_t cap = text->cap ? text->cap : 4096;
    while (cap < needed) {
        if (cap > SIZE_MAX / 2) {
            cap = needed;
            break;
        }
        cap *= 2;
    }
    char *grown = realloc(text->data, cap);
    if (!grown) {
        return false;
    }
    text->data = grown;
    text->cap = cap;
    return true;
}

static bool text_append_n(TextBuffer *text, const char *value, size_t length) {
    if (!text_reserve(text, length)) {
        return false;
    }
    memcpy(text->data + text->len, value, length);
    text->len += length;
    text->data[text->len] = '\0';
    return true;
}

static bool text_append(TextBuffer *text, const char *value) {
    return text_append_n(text, value, strlen(value));
}

static bool text_appendf(TextBuffer *text, const char *format, ...) {
    va_list args;
    va_start(args, format);
    va_list copy;
    va_copy(copy, args);
    int size = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (size < 0) {
        va_end(args);
        return false;
    }
    if (!text_reserve(text, (size_t)size)) {
        va_end(args);
        return false;
    }
    vsnprintf(text->data + text->len, text->cap - text->len, format, args);
    va_end(args);
    text->len += (size_t)size;
    return true;
}

static bool text_append_escaped(TextBuffer *text, const unsigned char *value, size_t length, size_t max_bytes) {
    size_t shown = length < max_bytes ? length : max_bytes;
    for (size_t i = 0; i < shown; ++i) {
        unsigned char ch = value[i];
        if (ch >= 0x20 && ch <= 0x7e) {
            char one = (char)ch;
            if (!text_append_n(text, &one, 1)) {
                return false;
            }
        } else if (ch == '\t') {
            if (!text_append(text, "\\t")) {
                return false;
            }
        } else {
            if (!text_appendf(text, "\\x%02X", (unsigned int)ch)) {
                return false;
            }
        }
    }
    if (shown < length) {
        return text_append(text, "...");
    }
    return true;
}

static bool text_append_escaped_cstr(TextBuffer *text, const char *value, size_t max_bytes) {
    return text_append_escaped(text, (const unsigned char *)value, strlen(value), max_bytes);
}

static unsigned char ascii_lower(unsigned char ch) {
    return (ch >= 'A' && ch <= 'Z') ? (unsigned char)(ch + ('a' - 'A')) : ch;
}

static bool ascii_equal_n(const char *value, size_t length, const char *literal) {
    size_t literal_length = strlen(literal);
    if (length != literal_length) {
        return false;
    }
    for (size_t i = 0; i < length; ++i) {
        if (ascii_lower((unsigned char)value[i]) != ascii_lower((unsigned char)literal[i])) {
            return false;
        }
    }
    return true;
}

static bool ascii_equal(const char *left, const char *right) {
    return ascii_equal_n(left, strlen(left), right);
}

static bool top_component_is(const char *path, const char *literal) {
    const char *slash = strchr(path, '/');
    size_t length = slash ? (size_t)(slash - path) : strlen(path);
    return ascii_equal_n(path, length, literal);
}

static bool path_is_visits(const char *path) {
    return ascii_equal(path, "visits.jsonl");
}

static bool path_is_tab_record(const char *path) {
    if (strncasecmp(path, "tabs/", 5) != 0) {
        return false;
    }
    const char *id = path + 5;
    const char *slash = strchr(id, '/');
    if (!slash || slash == id || strchr(slash + 1, '/')) {
        return false;
    }
    const char *name = slash + 1;
    return ascii_equal(name, "tab.txt") ||
           ascii_equal(name, "history.log") ||
           ascii_equal(name, "view.txt");
}

static bool path_is_tab_manifest(const char *path) {
    if (!path_is_tab_record(path)) {
        return false;
    }
    const char *name = strrchr(path, '/');
    return name && ascii_equal(name + 1, "tab.txt");
}

static bool component_looks_secret(const char *component, size_t length) {
    static const char *const secret_words[] = {
        "auth", "authentication", "bearer", "cookie", "cookies",
        "credential", "credentials", "keychain", "keystore", "login",
        "logins", "oauth", "passwd", "password", "passwords", "secret",
        "secrets", "session", "sessions", "token", "tokens"
    };

    char normalized[96];
    size_t out = 0;
    bool separator = false;
    for (size_t i = 0; i < length && out + 1 < sizeof(normalized); ++i) {
        unsigned char ch = ascii_lower((unsigned char)component[i]);
        bool alnum = (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9');
        if (alnum) {
            if (separator && out > 0 && normalized[out - 1] != '_') {
                normalized[out++] = '_';
            }
            normalized[out++] = (char)ch;
            separator = false;
        } else {
            separator = true;
        }
    }
    while (out > 0 && normalized[out - 1] == '_') {
        --out;
    }
    normalized[out] = '\0';
    if (strcmp(normalized, "login_data") == 0 || strcmp(normalized, "web_data") == 0) {
        return true;
    }

    size_t start = 0;
    while (start < length) {
        while (start < length) {
            unsigned char ch = (unsigned char)component[start];
            if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
                break;
            }
            ++start;
        }
        size_t end = start;
        while (end < length) {
            unsigned char ch = (unsigned char)component[end];
            if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))) {
                break;
            }
            ++end;
        }
        if (end > start) {
            for (size_t i = 0; i < sizeof(secret_words) / sizeof(secret_words[0]); ++i) {
                if (ascii_equal_n(component + start, end - start, secret_words[i])) {
                    return true;
                }
            }
        }
        start = end + 1;
    }
    return false;
}

static bool path_looks_secret(const char *path) {
    const char *start = path;
    while (*start) {
        const char *slash = strchr(start, '/');
        size_t length = slash ? (size_t)(slash - start) : strlen(start);
        if (component_looks_secret(start, length)) {
            return true;
        }
        if (!slash) {
            break;
        }
        start = slash + 1;
    }
    return false;
}

static StoreKind classify_path(const char *path) {
    if (path_is_visits(path) || path_is_tab_record(path)) {
        return KIND_CANONICAL;
    }
    if (path_looks_secret(path)) {
        return KIND_SECRET;
    }
    if (top_component_is(path, "snapshots")) {
        return KIND_SNAPSHOT;
    }
    if (top_component_is(path, "cache") ||
        top_component_is(path, "caches") ||
        top_component_is(path, "renderer-cache") ||
        top_component_is(path, "renderer_cache") ||
        top_component_is(path, "scratch") ||
        top_component_is(path, "tmp")) {
        return KIND_CACHE;
    }
    if (top_component_is(path, "indexes")) {
        return KIND_DERIVED;
    }
    return KIND_UNKNOWN;
}

static bool transparent_derived(const char *path) {
    if (strncasecmp(path, "indexes/", 8) != 0 || strchr(path + 8, '/')) {
        return false;
    }
    const char *name = path + 8;
    return ascii_equal(name, "chronology.tsv") ||
           ascii_equal(name, "urls.tsv") ||
           ascii_equal(name, "hosts.tsv") ||
           ascii_equal(name, "sources.tsv") ||
           ascii_equal(name, "days.tsv") ||
           ascii_equal(name, "queries.tsv") ||
           ascii_equal(name, "terms.tsv") ||
           ascii_equal(name, "summary.json");
}

static bool is_transparent_text(const char *path) {
    return path_is_visits(path) || path_is_tab_record(path) || transparent_derived(path);
}

static int row_compare(const void *left, const void *right) {
    const FileRow *a = left;
    const FileRow *b = right;
    return strcmp(a->path, b->path);
}

static void scan_directory(int directory_fd, const char *prefix, int depth, ScanResult *scan) {
    if (depth > MAX_DEPTH) {
        return;
    }

    int duplicate = dup(directory_fd);
    if (duplicate < 0) {
        return;
    }
    DIR *directory = fdopendir(duplicate);
    if (!directory) {
        close(duplicate);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        struct stat metadata;
        if (fstatat(directory_fd, entry->d_name, &metadata, AT_SYMLINK_NOFOLLOW) != 0) {
            continue;
        }

        char relative[MAX_PATH_TEXT];
        int written = prefix[0]
            ? snprintf(relative, sizeof(relative), "%s/%s", prefix, entry->d_name)
            : snprintf(relative, sizeof(relative), "%s", entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(relative)) {
            continue;
        }

        if (S_ISDIR(metadata.st_mode)) {
            int child = openat(directory_fd, entry->d_name,
                               O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (child >= 0) {
                scan_directory(child, relative, depth + 1, scan);
                close(child);
            }
            continue;
        }

        StoreKind kind = classify_path(relative);
        FileType type = S_ISREG(metadata.st_mode)
            ? FILE_REGULAR
            : (S_ISLNK(metadata.st_mode) ? FILE_SYMLINK : FILE_OTHER);
        uint64_t bytes = metadata.st_size > 0 ? (uint64_t)metadata.st_size : 0u;

        scan->file_count += 1;
        scan->byte_count += bytes;
        scan->files_by_kind[kind] += 1;
        scan->bytes_by_kind[kind] += bytes;
        if (type == FILE_REGULAR && path_is_tab_manifest(relative)) {
            scan->tab_records += 1;
        }

        if (scan->row_count < MAX_SCAN_ROWS) {
            FileRow *row = &scan->rows[scan->row_count++];
            snprintf(row->path, sizeof(row->path), "%s", relative);
            row->bytes = bytes;
            row->kind = kind;
            row->type = type;
            row->readable = type == FILE_REGULAR &&
                            kind != KIND_SECRET &&
                            is_transparent_text(relative);
        } else {
            scan->rows_truncated = true;
        }
    }
    closedir(directory);
}

static int open_storage_root(const char *root_path) {
    return open(root_path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
}

static int split_relative_path(char *path, char **parts, size_t max_parts) {
    if (!path[0] || path[0] == '/') {
        return -1;
    }
    size_t count = 0;
    char *save = NULL;
    char *part = strtok_r(path, "/", &save);
    while (part) {
        if (!part[0] || strcmp(part, ".") == 0 || strcmp(part, "..") == 0) {
            return -1;
        }
        if (count >= max_parts) {
            return -1;
        }
        parts[count++] = part;
        part = strtok_r(NULL, "/", &save);
    }
    return count > 0 ? (int)count : -1;
}

static int open_regular_nofollow(int root_fd, const char *relative_path) {
    if (strlen(relative_path) >= MAX_PATH_TEXT) {
        errno = ENAMETOOLONG;
        return -1;
    }

    char path[MAX_PATH_TEXT];
    snprintf(path, sizeof(path), "%s", relative_path);
    char *parts[64];
    int count = split_relative_path(path, parts, sizeof(parts) / sizeof(parts[0]));
    if (count < 1) {
        errno = EINVAL;
        return -1;
    }

    int directory_fd = dup(root_fd);
    if (directory_fd < 0) {
        return -1;
    }

    for (int i = 0; i < count - 1; ++i) {
        int next = openat(directory_fd, parts[i],
                          O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (next < 0) {
            close(directory_fd);
            return -1;
        }
        close(directory_fd);
        directory_fd = next;
    }

    int file_fd = openat(directory_fd, parts[count - 1],
                         O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    close(directory_fd);
    if (file_fd < 0) {
        return -1;
    }

    struct stat metadata;
    if (fstat(file_fd, &metadata) != 0 || !S_ISREG(metadata.st_mode)) {
        close(file_fd);
        errno = EINVAL;
        return -1;
    }
    return file_fd;
}

static unsigned char *read_limited(int root_fd, const char *path, size_t max_bytes,
                                   size_t *length_out, bool *truncated_out) {
    *length_out = 0;
    *truncated_out = false;

    int fd = open_regular_nofollow(root_fd, path);
    if (fd < 0) {
        return NULL;
    }

    unsigned char *buffer = malloc(max_bytes + 2);
    if (!buffer) {
        close(fd);
        errno = ENOMEM;
        return NULL;
    }

    size_t used = 0;
    while (used < max_bytes + 1) {
        ssize_t got = read(fd, buffer + used, max_bytes + 1 - used);
        if (got < 0) {
            if (errno == EINTR) {
                continue;
            }
            free(buffer);
            close(fd);
            return NULL;
        }
        if (got == 0) {
            break;
        }
        used += (size_t)got;
    }
    close(fd);

    *truncated_out = used > max_bytes;
    if (used > max_bytes) {
        used = max_bytes;
    }
    buffer[used] = '\0';
    *length_out = used;
    return buffer;
}

static int64_t indexed_visit_count(int root_fd) {
    size_t length = 0;
    bool truncated = false;
    unsigned char *data = read_limited(root_fd, "indexes/summary.json",
                                       MAX_SUMMARY_BYTES, &length, &truncated);
    if (!data || truncated) {
        free(data);
        return -1;
    }

    const char *text = (const char *)data;
    const char *key = strstr(text, "\"entries\"");
    if (!key) {
        free(data);
        return -1;
    }
    const char *colon = strchr(key + 9, ':');
    if (!colon) {
        free(data);
        return -1;
    }
    const char *number = colon + 1;
    while (*number == ' ' || *number == '\t' || *number == '\r' || *number == '\n') {
        ++number;
    }
    if (*number < '0' || *number > '9') {
        free(data);
        return -1;
    }

    errno = 0;
    char *end = NULL;
    long long value = strtoll(number, &end, 10);
    bool valid = errno == 0 && end != number && value >= 0;
    free(data);
    return valid ? (int64_t)value : -1;
}

static int name_compare(const void *left, const void *right) {
    return strcmp((const char *)left, (const char *)right);
}

static size_t collect_tab_names(int root_fd, char names[MAX_TAB_NAMES][NAME_MAX + 1], bool *truncated) {
    *truncated = false;
    int tabs_fd = openat(root_fd, "tabs", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (tabs_fd < 0) {
        return 0;
    }

    int duplicate = dup(tabs_fd);
    if (duplicate < 0) {
        close(tabs_fd);
        return 0;
    }
    DIR *directory = fdopendir(duplicate);
    if (!directory) {
        close(duplicate);
        close(tabs_fd);
        return 0;
    }

    size_t count = 0;
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        struct stat directory_metadata;
        if (fstatat(tabs_fd, entry->d_name, &directory_metadata, AT_SYMLINK_NOFOLLOW) != 0 ||
            !S_ISDIR(directory_metadata.st_mode)) {
            continue;
        }

        int tab_fd = openat(tabs_fd, entry->d_name,
                            O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (tab_fd < 0) {
            continue;
        }
        struct stat manifest_metadata;
        bool manifest = fstatat(tab_fd, "tab.txt", &manifest_metadata, AT_SYMLINK_NOFOLLOW) == 0 &&
                        S_ISREG(manifest_metadata.st_mode);
        close(tab_fd);
        if (!manifest) {
            continue;
        }

        if (count < MAX_TAB_NAMES) {
            snprintf(names[count], NAME_MAX + 1, "%s", entry->d_name);
            ++count;
        } else {
            *truncated = true;
        }
    }

    closedir(directory);
    close(tabs_fd);
    qsort(names, count, sizeof(names[0]), name_compare);
    return count;
}

typedef struct {
    char id[160];
    char state[160];
    char current[200];
    char last_visit[200];
    char renderer[160];
    char labels[320];
} TabFields;

static void copy_value(char *destination, size_t capacity, const char *value, size_t length) {
    if (capacity == 0) {
        return;
    }
    while (length > 0 && (value[length - 1] == '\r' || value[length - 1] == ' ' || value[length - 1] == '\t')) {
        --length;
    }
    size_t copy = length < capacity - 1 ? length : capacity - 1;
    memcpy(destination, value, copy);
    destination[copy] = '\0';
}

static void append_label_value(TabFields *fields, const char *value, size_t length) {
    size_t used = strlen(fields->labels);
    if (used && used + 1 < sizeof(fields->labels)) {
        fields->labels[used++] = ',';
        fields->labels[used] = '\0';
    }
    if (used >= sizeof(fields->labels) - 1) {
        return;
    }
    size_t available = sizeof(fields->labels) - used;
    copy_value(fields->labels + used, available, value, length);
}

static void parse_tab_fields(unsigned char *data, size_t length, TabFields *fields) {
    memset(fields, 0, sizeof(*fields));
    size_t start = 0;
    while (start < length) {
        size_t end = start;
        while (end < length && data[end] != '\n') {
            ++end;
        }

        size_t line_start = start;
        while (line_start < end && (data[line_start] == ' ' || data[line_start] == '\t' || data[line_start] == '\r')) {
            ++line_start;
        }
        size_t line_end = end;
        while (line_end > line_start &&
               (data[line_end - 1] == ' ' || data[line_end - 1] == '\t' || data[line_end - 1] == '\r')) {
            --line_end;
        }

        if (line_end > line_start && data[line_start] != '#') {
            size_t key_end = line_start;
            while (key_end < line_end && data[key_end] != ' ' && data[key_end] != '\t') {
                ++key_end;
            }
            size_t value_start = key_end;
            while (value_start < line_end && (data[value_start] == ' ' || data[value_start] == '\t')) {
                ++value_start;
            }

            const char *key = (const char *)data + line_start;
            size_t key_length = key_end - line_start;
            const char *value = (const char *)data + value_start;
            size_t value_length = line_end - value_start;

            if (ascii_equal_n(key, key_length, "id") && !fields->id[0]) {
                copy_value(fields->id, sizeof(fields->id), value, value_length);
            } else if (ascii_equal_n(key, key_length, "state") && !fields->state[0]) {
                copy_value(fields->state, sizeof(fields->state), value, value_length);
            } else if (ascii_equal_n(key, key_length, "current_history") && !fields->current[0]) {
                copy_value(fields->current, sizeof(fields->current), value, value_length);
            } else if ((ascii_equal_n(key, key_length, "last_visit") ||
                        ascii_equal_n(key, key_length, "last_visited")) &&
                       !fields->last_visit[0]) {
                copy_value(fields->last_visit, sizeof(fields->last_visit), value, value_length);
            } else if ((ascii_equal_n(key, key_length, "preferred_renderer") ||
                        ascii_equal_n(key, key_length, "renderer")) &&
                       !fields->renderer[0]) {
                copy_value(fields->renderer, sizeof(fields->renderer), value, value_length);
            } else if (ascii_equal_n(key, key_length, "label")) {
                append_label_value(fields, value, value_length);
            }
        }

        start = end < length ? end + 1 : length;
    }
}

static void render_tabs(int root_fd, TextBuffer *text, uint64_t tab_count) {
    text_append(text, "\nDURABLE TAB RECORDS\n");
    if (tab_count == 0) {
        text_append(text, "(none)\n");
        return;
    }

    char names[MAX_TAB_NAMES][NAME_MAX + 1];
    bool names_truncated = false;
    size_t count = collect_tab_names(root_fd, names, &names_truncated);
    size_t shown = count < MAX_DISPLAY_TABS ? count : MAX_DISPLAY_TABS;

    for (size_t i = 0; i < shown; ++i) {
        char path[MAX_PATH_TEXT];
        int written = snprintf(path, sizeof(path), "tabs/%s/tab.txt", names[i]);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            continue;
        }

        size_t length = 0;
        bool manifest_truncated = false;
        unsigned char *data = read_limited(root_fd, path, MAX_TAB_BYTES,
                                           &length, &manifest_truncated);
        if (!data) {
            text_append(text, "- ");
            text_append_escaped_cstr(text, names[i], 120);
            text_append(text, " [manifest unavailable]\n");
            continue;
        }

        if (manifest_truncated && length > 0 && data[length - 1] != '\n') {
            while (length > 0 && data[length - 1] != '\n') {
                --length;
            }
            if (length > 0) {
                --length;
            }
        }

        TabFields fields;
        parse_tab_fields(data, length, &fields);
        free(data);

        text_append(text, "- ");
        if (fields.id[0]) {
            text_append_escaped_cstr(text, fields.id, 120);
        } else {
            text_append_escaped_cstr(text, names[i], 120);
        }
        if (fields.state[0]) {
            text_append(text, " state=");
            text_append_escaped_cstr(text, fields.state, 100);
        }
        if (fields.current[0]) {
            text_append(text, " current=");
            text_append_escaped_cstr(text, fields.current, 120);
        }
        if (fields.last_visit[0]) {
            text_append(text, " last=");
            text_append_escaped_cstr(text, fields.last_visit, 120);
        }
        if (fields.renderer[0]) {
            text_append(text, " renderer=");
            text_append_escaped_cstr(text, fields.renderer, 100);
        }
        if (fields.labels[0]) {
            text_append(text, " labels=");
            text_append_escaped_cstr(text, fields.labels, 180);
        }
        if (manifest_truncated) {
            text_append(text, " [manifest truncated]");
        }
        text_append(text, "\n");
    }

    if ((uint64_t)shown < tab_count || names_truncated) {
        text_appendf(text, "... showing %zu of %" PRIu64 " manifests\n", shown, tab_count);
    }
}

static bool line_has_nonspace(const unsigned char *data, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
        unsigned char ch = data[i];
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
            return true;
        }
    }
    return false;
}

typedef struct {
    size_t start;
    size_t length;
    uint64_t offset;
} TailLine;

static void render_visit_tail(int root_fd, TextBuffer *text) {
    text_append(text, "\nRECENT CANONICAL VISITS\n");

    int fd = open_regular_nofollow(root_fd, "visits.jsonl");
    if (fd < 0) {
        text_append(text, "(none)\n");
        return;
    }

    struct stat metadata;
    if (fstat(fd, &metadata) != 0 || metadata.st_size <= 0) {
        close(fd);
        text_append(text, "(none)\n");
        return;
    }

    uint64_t file_size = (uint64_t)metadata.st_size;
    size_t to_read = file_size < MAX_TAIL_BYTES ? (size_t)file_size : MAX_TAIL_BYTES;
    uint64_t file_start = file_size - to_read;

    unsigned char *data = malloc(to_read + 1);
    if (!data) {
        close(fd);
        text_append(text, "(memory unavailable)\n");
        return;
    }

    size_t used = 0;
    while (used < to_read) {
        ssize_t got = pread(fd, data + used, to_read - used, (off_t)(file_start + used));
        if (got < 0) {
            if (errno == EINTR) {
                continue;
            }
            free(data);
            close(fd);
            text_append(text, "(read unavailable)\n");
            return;
        }
        if (got == 0) {
            break;
        }
        used += (size_t)got;
    }
    close(fd);

    size_t base = 0;
    if (file_start > 0) {
        while (base < used && data[base] != '\n') {
            ++base;
        }
        if (base < used) {
            ++base;
        }
    }

    TailLine ring[MAX_TAIL_RECORDS];
    size_t ring_count = 0;
    size_t ring_next = 0;

    size_t start = base;
    while (start < used) {
        size_t end = start;
        while (end < used && data[end] != '\n') {
            ++end;
        }
        size_t trimmed = end;
        while (trimmed > start && (data[trimmed - 1] == '\r' || data[trimmed - 1] == '\n')) {
            --trimmed;
        }
        if (line_has_nonspace(data, start, trimmed)) {
            ring[ring_next].start = start;
            ring[ring_next].length = trimmed - start;
            ring[ring_next].offset = file_start + start;
            ring_next = (ring_next + 1) % MAX_TAIL_RECORDS;
            if (ring_count < MAX_TAIL_RECORDS) {
                ++ring_count;
            }
        }
        start = end < used ? end + 1 : used;
    }

    if (ring_count == 0) {
        text_append(text, "(none within byte budget)\n");
        free(data);
        return;
    }

    size_t first = ring_count == MAX_TAIL_RECORDS ? ring_next : 0;
    for (size_t i = 0; i < ring_count; ++i) {
        TailLine *line = &ring[(first + i) % MAX_TAIL_RECORDS];
        text_appendf(text, "@%" PRIu64 " ", line->offset);
        text_append_escaped(text, data + line->start, line->length, 220);
        text_append(text, "\n");
    }

    if (file_start > 0) {
        text_appendf(text, "(tail bounded to last %u bytes)\n", (unsigned int)MAX_TAIL_BYTES);
    }
    free(data);
}

static void render_files(const ScanResult *scan, TextBuffer *text) {
    text_append(text, "\nPHYSICAL FILES\n");
    size_t shown = scan->row_count < MAX_DISPLAY_FILES ? scan->row_count : MAX_DISPLAY_FILES;
    for (size_t i = 0; i < shown; ++i) {
        const FileRow *row = &scan->rows[i];
        text_appendf(text, "%-9s %-4s %-7s %9" PRIu64 " ",
                     kind_name(row->kind),
                     row->readable ? "read" : "meta",
                     type_name(row->type),
                     row->bytes);
        text_append_escaped_cstr(text, row->path, 260);
        text_append(text, "\n");
    }
    if ((uint64_t)shown < scan->file_count || scan->rows_truncated) {
        text_appendf(text, "... showing %zu of %" PRIu64 " files", shown, scan->file_count);
        if (scan->rows_truncated) {
            text_appendf(text, " (row capture capped at %u)", (unsigned int)MAX_SCAN_ROWS);
        }
        text_append(text, "\n");
    }
}

static char *build_inspector_text(ANativeActivity *activity) {
    TextBuffer text = {0};
    text_append(&text, "IB STORAGE INSPECTOR\n");
    text_append(&text, "read-only; resume app to refresh\n\n");

    const char *internal = activity->internalDataPath ? activity->internalDataPath : "";
    char root_path[PATH_MAX];
    int written = snprintf(root_path, sizeof(root_path), "%s/state", internal);
    if (written < 0 || (size_t)written >= sizeof(root_path)) {
        text_append(&text, "root path unavailable\n");
        return text.data;
    }

    text_append(&text, "root ");
    text_append_escaped_cstr(&text, root_path, sizeof(root_path));
    text_append(&text, "\n");

    int root_fd = open_storage_root(root_path);
    if (root_fd < 0) {
        text_append(&text, "state directory absent or not safely openable\n");
        text_append(&text, "\nThe inspector does not create storage.\n");
        return text.data;
    }

    ScanResult scan;
    memset(&scan, 0, sizeof(scan));
    scan_directory(root_fd, "", 0, &scan);
    qsort(scan.rows, scan.row_count, sizeof(scan.rows[0]), row_compare);

    int64_t indexed = indexed_visit_count(root_fd);
    text_appendf(&text, "files %" PRIu64 "\n", scan.file_count);
    text_appendf(&text, "bytes %" PRIu64 "\n", scan.byte_count);
    text_appendf(&text, "tab_records %" PRIu64 "\n", scan.tab_records);
    if (indexed >= 0) {
        text_appendf(&text, "indexed_visits %" PRId64 "\n", indexed);
    } else {
        text_append(&text, "indexed_visits -\n");
    }

    for (int kind = 0; kind < KIND_COUNT; ++kind) {
        if (scan.files_by_kind[kind] == 0) {
            continue;
        }
        text_appendf(&text, "%-9s %" PRIu64 " files %" PRIu64 " bytes\n",
                     kind_name((StoreKind)kind),
                     scan.files_by_kind[kind],
                     scan.bytes_by_kind[kind]);
    }

    render_tabs(root_fd, &text, scan.tab_records);
    render_files(&scan, &text);
    render_visit_tail(root_fd, &text);
    close(root_fd);

    text_append(&text, "\nInspection never wakes renderers and never mutates state.\n");
    return text.data;
}

static bool jni_ok(JNIEnv *env, const char *where) {
    if (!(*env)->ExceptionCheck(env)) {
        return true;
    }
    LOGE("JNI exception at %s", where);
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
    return false;
}

static void update_text(ANativeActivity *activity) {
    InspectorActivity *state = activity->instance;
    if (!state || !state->text_view) {
        return;
    }

    char *content = build_inspector_text(activity);
    if (!content) {
        return;
    }

    JNIEnv *env = activity->env;
    jclass text_class = (*env)->GetObjectClass(env, state->text_view);
    if (!text_class || !jni_ok(env, "GetObjectClass(TextView)")) {
        free(content);
        return;
    }
    jmethodID set_text = (*env)->GetMethodID(
        env, text_class, "setText", "(Ljava/lang/CharSequence;)V");
    if (!set_text || !jni_ok(env, "TextView.setText id")) {
        (*env)->DeleteLocalRef(env, text_class);
        free(content);
        return;
    }

    jstring java_text = (*env)->NewStringUTF(env, content);
    free(content);
    if (!java_text || !jni_ok(env, "NewStringUTF")) {
        (*env)->DeleteLocalRef(env, text_class);
        return;
    }

    (*env)->CallVoidMethod(env, state->text_view, set_text, java_text);
    jni_ok(env, "TextView.setText");
    (*env)->DeleteLocalRef(env, java_text);
    (*env)->DeleteLocalRef(env, text_class);
}

static bool create_view(ANativeActivity *activity, InspectorActivity *state) {
    JNIEnv *env = activity->env;

    jclass scroll_class = (*env)->FindClass(env, "android/widget/ScrollView");
    jclass text_class = (*env)->FindClass(env, "android/widget/TextView");
    jclass activity_class = (*env)->FindClass(env, "android/app/Activity");
    jclass typeface_class = (*env)->FindClass(env, "android/graphics/Typeface");
    if (!scroll_class || !text_class || !activity_class || !typeface_class ||
        !jni_ok(env, "FindClass")) {
        return false;
    }

    jmethodID scroll_ctor = (*env)->GetMethodID(
        env, scroll_class, "<init>", "(Landroid/content/Context;)V");
    jmethodID text_ctor = (*env)->GetMethodID(
        env, text_class, "<init>", "(Landroid/content/Context;)V");
    jmethodID add_view = (*env)->GetMethodID(
        env, scroll_class, "addView", "(Landroid/view/View;)V");
    jmethodID set_fill = (*env)->GetMethodID(
        env, scroll_class, "setFillViewport", "(Z)V");
    jmethodID set_content = (*env)->GetMethodID(
        env, activity_class, "setContentView", "(Landroid/view/View;)V");
    jmethodID set_text_size = (*env)->GetMethodID(
        env, text_class, "setTextSize", "(F)V");
    jmethodID set_text_color = (*env)->GetMethodID(
        env, text_class, "setTextColor", "(I)V");
    jmethodID set_background = (*env)->GetMethodID(
        env, text_class, "setBackgroundColor", "(I)V");
    jmethodID set_padding = (*env)->GetMethodID(
        env, text_class, "setPadding", "(IIII)V");
    jmethodID set_selectable = (*env)->GetMethodID(
        env, text_class, "setTextIsSelectable", "(Z)V");
    jmethodID set_typeface = (*env)->GetMethodID(
        env, text_class, "setTypeface", "(Landroid/graphics/Typeface;)V");
    jfieldID monospace_field = (*env)->GetStaticFieldID(
        env, typeface_class, "MONOSPACE", "Landroid/graphics/Typeface;");
    if (!scroll_ctor || !text_ctor || !add_view || !set_fill || !set_content ||
        !set_text_size || !set_text_color || !set_background || !set_padding ||
        !set_selectable || !set_typeface || !monospace_field ||
        !jni_ok(env, "method lookup")) {
        return false;
    }

    jobject scroll = (*env)->NewObject(env, scroll_class, scroll_ctor, activity->clazz);
    jobject text = (*env)->NewObject(env, text_class, text_ctor, activity->clazz);
    jobject monospace = (*env)->GetStaticObjectField(env, typeface_class, monospace_field);
    if (!scroll || !text || !monospace || !jni_ok(env, "view construction")) {
        return false;
    }

    (*env)->CallVoidMethod(env, scroll, set_fill, JNI_TRUE);
    (*env)->CallVoidMethod(env, text, set_text_size, 13.0f);
    (*env)->CallVoidMethod(env, text, set_text_color, (jint)0xffeeeeeeu);
    (*env)->CallVoidMethod(env, text, set_background, (jint)0xff111111u);
    (*env)->CallVoidMethod(env, text, set_padding, 24, 24, 24, 24);
    (*env)->CallVoidMethod(env, text, set_selectable, JNI_TRUE);
    (*env)->CallVoidMethod(env, text, set_typeface, monospace);
    (*env)->CallVoidMethod(env, scroll, add_view, text);
    (*env)->CallVoidMethod(env, activity->clazz, set_content, scroll);
    if (!jni_ok(env, "view setup")) {
        return false;
    }

    state->text_view = (*env)->NewGlobalRef(env, text);
    if (!state->text_view || !jni_ok(env, "NewGlobalRef(TextView)")) {
        return false;
    }

    (*env)->DeleteLocalRef(env, monospace);
    (*env)->DeleteLocalRef(env, text);
    (*env)->DeleteLocalRef(env, scroll);
    (*env)->DeleteLocalRef(env, typeface_class);
    (*env)->DeleteLocalRef(env, activity_class);
    (*env)->DeleteLocalRef(env, text_class);
    (*env)->DeleteLocalRef(env, scroll_class);
    return true;
}

static void on_resume(ANativeActivity *activity) {
    update_text(activity);
}

static void on_destroy(ANativeActivity *activity) {
    InspectorActivity *state = activity->instance;
    if (!state) {
        return;
    }
    if (state->text_view) {
        (*activity->env)->DeleteGlobalRef(activity->env, state->text_view);
    }
    free(state);
    activity->instance = NULL;
}

JNIEXPORT void ANativeActivity_onCreate(
    ANativeActivity *activity,
    void *saved_state,
    size_t saved_state_size
) {
    (void)saved_state;
    (void)saved_state_size;

    InspectorActivity *state = calloc(1, sizeof(*state));
    if (!state) {
        ANativeActivity_finish(activity);
        return;
    }
    state->activity = activity;
    activity->instance = state;
    activity->callbacks->onResume = on_resume;
    activity->callbacks->onDestroy = on_destroy;

    if (!create_view(activity, state)) {
        LOGE("failed to create inspector view");
        ANativeActivity_finish(activity);
        return;
    }
    update_text(activity);
}
