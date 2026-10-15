/* CI-only narrow native harness for *actual* ICU #35 transport.c. It is not
 * an Android app_process/Idriç replacement or a browser URL parser. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int icu_send_http(const char *, int, const char *, const char *, int);
int icu_send_https(const char *, int, const char *, const char *, int);

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    const char *url = argv[1];
    int tls = 0;
    if (strncmp(url, "http://", 7) == 0) url += 7;
    else if (strncmp(url, "https://", 8) == 0) {
        url += 8;
        tls = 1;
    } else return 2;
    const char *colon = strchr(url, ':');
    const char *slash = strchr(url, '/');
    if (!colon || !slash || colon >= slash || colon == url) return 2;
    if ((size_t)(colon - url) >= 128U) return 2;
    char host[128];
    memcpy(host, url, (size_t)(colon - url));
    host[colon - url] = '\0';
    if (strcmp(host, "localhost") != 0 &&
        strcmp(host, "127.0.0.1") != 0) return 2;
    errno = 0;
    char *end = NULL;
    long port_long = strtol(colon + 1, &end, 10);
    if (errno || end != slash || port_long < 1 || port_long > 65535)
        return 2;
    int port = (int)port_long;
    for (const unsigned char *p = (const unsigned char *)slash; *p; ++p)
        if (*p <= 32U || *p >= 127U) return 2;
    char request[4096];
    int count = snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\nHost: %s:%d\r\nConnection: close\r\n\r\n",
        slash, host, port);
    if (count <= 0 || (size_t)count >= sizeof(request)) return 2;
    return tls
        ? icu_send_https(host, port, request, NULL, 0)
        : icu_send_http(host, port, request, NULL, 0);
}
