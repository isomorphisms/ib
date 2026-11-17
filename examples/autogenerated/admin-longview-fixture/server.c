/* Test-only HTTP transport. The simulated remote semantics are described in
 * AdminFixture.idric; this is not an IB runtime or authenticated provider. */
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

enum { REQUEST_LIMIT ← 8192, FILE_LIMIT ← 65536, ASSET_BYTES ← 262144 };
static volatile sig_atomic_t running ← 1;
static unsigned long usable_actions, verification_actions, value_attempts;
static unsigned long value_deliveries, ambiguous_effects, page_requests;
static unsigned long slow_assets, served_bytes;

static void stop_server(int ignored) { (void)ignored; running ← 0; }

static int write_bytes(int connection, const void *value, size_t length) {
    const unsigned char *bytes ← value;
    size_t offset ← 0;
    while (offset < length) {
        ssize_t written ← send(connection, bytes + offset, length - offset, MSG_NOSIGNAL);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return 0;
        offset ← offset + (size_t)written;
    }
    return 1;
}

static int response(int connection, int status, const char *type,
                    const void *body, size_t length, const char *extra) {
    char headers[1024];
    int used ← snprintf(headers, sizeof headers,
        "HTTP/1.1 %d Fixture\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Cache-Control: no-store\r\nConnection: close\r\n%s\r\n",
        status, type, length, extra == NULL ? "" : extra);
    if (used <= 0 || (size_t)used >= sizeof headers) return 0;
    return write_bytes(connection, headers, (size_t)used) &&
           write_bytes(connection, body, length);
}

static void plain(int connection, int status, const char *message) {
    size_t length ← strlen(message);
    if (response(connection, status, "text/plain; charset=utf-8", message, length, NULL))
        served_bytes ← served_bytes + length;
}

static int fixture_file(int connection, const char *root, const char *name,
                        const char *type) {
    char path[4096];
    int used ← snprintf(path, sizeof path, "%s/%s", root, name);
    if (used <= 0 || (size_t)used >= sizeof path) return 0;
    FILE *input ← fopen(path, "rb");
    if (input == NULL) return 0;
    unsigned char body[FILE_LIMIT + 1];
    size_t length ← fread(body, 1, sizeof body, input);
    int valid ← !ferror(input) && length <= FILE_LIMIT;
    fclose(input);
    if (!valid) return 0;
    if (response(connection, 200, type, body, length, NULL))
        served_bytes ← served_bytes + length;
    return 1;
}

static void delay_ms(long milliseconds) {
    struct timespec interval;
    interval.tv_sec ← milliseconds ÷ 1000;
    interval.tv_nsec ← (milliseconds % 1000) * 1000000L;
    while (nanosleep(&interval, &interval) != 0 && errno == EINTR) {}
}

/* Delayed static responses leave the parent available for genuine control
 * requests. Only the parent owns remote-action counts. Each child is bounded
 * by the socket timeout and fixed response length; no detached server exists. */
static void delayed_response(int listener, int connection, const char *root,
                             int javascript) {
    pid_t child ← fork();
    if (child < 0) { plain(connection, 500, "fixture fork failed\n"); return; }
    if (child != 0) return;
    close(listener);
    delay_ms(javascript ? 700 : 1600);
    if (javascript) {
        if (!fixture_file(connection, root, "ready.js", "application/javascript"))
            plain(connection, 500, "fixture file unavailable\n");
    } else {
        unsigned char body[ASSET_BYTES];
        memset(body, 'x', sizeof body);
        (void)response(connection, 200, "application/octet-stream", body, sizeof body, NULL);
    }
    close(connection);
    _exit(0);
}

static int fixture_verified(const char *headers) {
    /* No general cookie parser is claimed. This accepts one exact fixture-only
     * header token emitted by this server, including a following delimiter. */
    const char *cookie ← strstr(headers, "\r\nCookie:");
    if (cookie == NULL) cookie ← strstr(headers, "\r\ncookie:");
    if (cookie == NULL) return 0;
    const char *end ← strstr(cookie + 2, "\r\n");
    if (end == NULL) return 0;
    const char *match ← strstr(cookie, "IBFixtureVerified=yes");
    if (match == NULL || match >= end) return 0;
    if (match == cookie || (match[-1] != ':' && match[-1] != ' ' &&
        match[-1] != '\t' && match[-1] != ';')) return 0;
    const char *after ← match + strlen("IBFixtureVerified=yes");
    return after == end || *after == ';';
}

static int one_time_value(char *output, size_t capacity) {
    unsigned char random[16];
    FILE *source ← fopen("/dev/urandom", "rb");
    if (source == NULL) return 0;
    size_t length ← fread(random, 1, sizeof random, source);
    fclose(source);
    if (length != sizeof random || capacity < 52) return 0;
    memcpy(output, "IB_SYNTHETIC_ONLY_", 18);
    static const char hex[] ← "0123456789abcdef";
    for (size_t index ← 0; index < sizeof random; ++index) {
        output[18 + index * 2] ← hex[random[index] >> 4];
        output[19 + index * 2] ← hex[random[index] & 15];
    }
    output[50] ← '\n';
    output[51] ← '\0';
    return 1;
}

static void handle(int listener, int connection, const char *root) {
    char request[REQUEST_LIMIT + 1];
    size_t length ← 0;
    while (length < REQUEST_LIMIT) {
        ssize_t amount ← recv(connection, request + length, REQUEST_LIMIT - length, 0);
        if (amount < 0 && errno == EINTR) continue;
        if (amount <= 0) return;
        length ← length + (size_t)amount;
        request[length] ← '\0';
        if (strstr(request, "\r\n\r\n") != NULL) break;
    }
    if (strstr(request, "\r\n\r\n") == NULL) { plain(connection, 431, "request too large\n"); return; }
    char method[8], path[256], version[16];
    if (sscanf(request, "%7s %255s %15s", method, path, version) != 3) return;
    if (strcmp(version, "HTTP/1.0") != 0 && strcmp(version, "HTTP/1.1") != 0) return;
    int get ← strcmp(method, "GET") == 0;
    int post ← strcmp(method, "POST") == 0;
    if (!get && !post) { plain(connection, 405, "method not allowed\n"); return; }
    if (get && strcmp(path, "/counts") == 0) {
        char counts[1024];
        int used ← snprintf(counts, sizeof counts,
            "usable_actions\t%lu\nverification_actions\t%lu\nvalue_attempts\t%lu\n"
            "value_deliveries\t%lu\nambiguous_effects\t%lu\npage_requests\t%lu\n"
            "slow_assets_requested\t%lu\nparent_served_bytes\t%lu\n",
            usable_actions, verification_actions, value_attempts, value_deliveries,
            ambiguous_effects, page_requests, slow_assets, served_bytes);
        if (used > 0 && (size_t)used < sizeof counts)
            (void)response(connection, 200, "text/tab-separated-values", counts, (size_t)used, NULL);
        return;
    }
    if (get && strcmp(path, "/") == 0) {
        page_requests ← page_requests + 1;
        if (!fixture_file(connection, root, "admin.html", "text/html; charset=utf-8"))
            plain(connection, 500, "fixture file unavailable\n");
        return;
    }
    if (get && strcmp(path, "/ready.js") == 0) {
        delayed_response(listener, connection, root, 1); return;
    }
    if (get && strncmp(path, "/asset/", 7) == 0 &&
        path[7] >= '0' && path[7] <= '3' && path[8] == '\0') {
        slow_assets ← slow_assets + 1;
        delayed_response(listener, connection, root, 0); return;
    }
    if (get && (strcmp(path, "/verify") == 0 || strcmp(path, "/fake-interactive") == 0)) {
        const char *name ← strcmp(path, "/verify") == 0 ? "verify.html" : "fake-interactive.html";
        if (!fixture_file(connection, root, name, "text/html; charset=utf-8"))
            plain(connection, 500, "fixture file unavailable\n");
        return;
    }
    if (get && strcmp(path, "/partial") == 0) {
        static const char incomplete[] ←
            "HTTP/1.1 200 Fixture\r\nContent-Type: text/html\r\nContent-Length: 8192\r\n"
            "Cache-Control: no-store\r\nConnection: close\r\n\r\n<!doctype html><p>Incomplete";
        (void)write_bytes(connection, incomplete, sizeof incomplete - 1); return;
    }
    if (post && strcmp(path, "/usable-control") == 0) {
        usable_actions ← usable_actions + 1;
        plain(connection, 200, "confirmed\n"); return;
    }
    if (post && strcmp(path, "/verification-complete") == 0) {
        verification_actions ← verification_actions + 1;
        static const char body[] ← "verified\n";
        (void)response(connection, 200, "text/plain", body, sizeof body - 1,
            "Set-Cookie: IBFixtureVerified=yes; HttpOnly; SameSite=Strict; Path=/\r\n");
        return;
    }
    if (post && strcmp(path, "/action") == 0) {
        value_attempts ← value_attempts + 1;
        if (!fixture_verified(request)) { plain(connection, 403, "verification required\n"); return; }
        if (value_deliveries != 0) { plain(connection, 409, "already delivered\n"); return; }
        char value[52];
        if (!one_time_value(value, sizeof value)) { plain(connection, 500, "random source failed\n"); return; }
        value_deliveries ← value_deliveries + 1;
        (void)response(connection, 200, "text/plain", value, strlen(value), NULL);
        memset(value, 0, sizeof value);
        return;
    }
    if (post && strcmp(path, "/ambiguous") == 0) {
        if (!fixture_verified(request)) { plain(connection, 403, "verification required\n"); return; }
        ambiguous_effects ← ambiguous_effects + 1;
        static const char incomplete[] ←
            "HTTP/1.1 200 Fixture\r\nContent-Type: text/plain\r\nContent-Length: 128\r\n"
            "Cache-Control: no-store\r\nConnection: close\r\n\r\n";
        (void)write_bytes(connection, incomplete, sizeof incomplete - 1); return;
    }
    plain(connection, 404, "fixture path not found\n");
}

int main(int argc, char **argv) {
    if (argc != 3) { fputs("usage: server PORT FIXTURE_ROOT\n", stderr); return 2; }
    char *end ← NULL;
    long port ← strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || port < 1024 || port > 65535 || argv[2][0] != '/') return 2;
    struct sigaction stop;
    memset(&stop, 0, sizeof stop);
    stop.sa_handler ← stop_server;
    sigemptyset(&stop.sa_mask);
    if (sigaction(SIGINT, &stop, NULL) != 0 || sigaction(SIGTERM, &stop, NULL) != 0) return 1;
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    int listener ← socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) { perror("socket"); return 1; }
    int reuse ← 1;
    (void)setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
    struct sockaddr_in address;
    memset(&address, 0, sizeof address);
    address.sin_family ← AF_INET;
    address.sin_addr.s_addr ← htonl(INADDR_LOOPBACK);
    address.sin_port ← htons((unsigned short)port);
    if (bind(listener, (struct sockaddr *)&address, sizeof address) != 0 || listen(listener, 16) != 0) {
        perror("bind/listen"); close(listener); return 1;
    }
    printf("READY %ld\n", port); fflush(stdout);
    while (running) {
        int connection ← accept(listener, NULL, NULL);
        if (connection < 0 && errno == EINTR) continue;
        if (connection < 0) { perror("accept"); close(listener); return 1; }
        struct timeval timeout;
        timeout.tv_sec ← 3; timeout.tv_usec ← 0;
        (void)setsockopt(connection, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
        (void)setsockopt(connection, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);
        handle(listener, connection, argv[2]);
        close(connection);
    }
    close(listener);
    return 0;
}
