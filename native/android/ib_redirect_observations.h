#ifndef IB_REDIRECT_OBSERVATIONS_H
#define IB_REDIRECT_OBSERVATIONS_H

#include <stddef.h>
#include <string.h>

#define IB_REDIRECT_OBSERVATION_CAP 8192U
#define IB_OBSERVED_URL_CAP 2048U

/* The response owner emits ASCII length-framed R1 observations, not a shell
 * command, JSON, or a delimiter-separated list of URLs. This validator owns
 * only the bounded wire boundary; ICU owns acquisition and selected endpoints. */
static inline int ib_observed_url(const char *url, size_t length) {
    size_t authority;
    if (length > IB_OBSERVED_URL_CAP) return 0;
    if (length > 8U && memcmp(url, "https://", 8U) == 0) authority = 8U;
    else if (length > 7U && memcmp(url, "http://", 7U) == 0) authority = 7U;
    else return 0;
    if (url[authority] == '/' || url[authority] == '?' || url[authority] == '#') return 0;
    int in_authority = 1;
    for (size_t i = 0; i < length; ++i) {
        unsigned char octet = (unsigned char)url[i];
        if (octet <= 32U || octet >= 127U || octet == '\'') return 0;
        if (i >= authority) {
            if (octet == '/' || octet == '?' || octet == '#') in_authority = 0;
            if (in_authority && octet == '@') return 0;
        }
    }
    return 1;
}

static inline int ib_redirect_url(const char **cursor, const char *end,
                                  const char **url, size_t *length) {
    const char *position = *cursor;
    size_t count = 0, digits = 0;
    while (position < end && *position >= '0' && *position <= '9') {
        if (++digits > 4U || (digits == 1U && *position == '0')) return 0;
        count = count * 10U + (unsigned)(*position++ - '0');
    }
    if (!digits || position == end || *position++ != ' ' ||
        count > IB_OBSERVED_URL_CAP || (size_t)(end - position) < count ||
        !ib_observed_url(position, count)) return 0;
    *url = position;
    *length = count;
    *cursor = position + count;
    return 1;
}

/* This is a boundary comparison, not URL resolution. ICU observes its socket
 * endpoint after the checked request has supplied a default path and port.
 * Permit only that explicit projection; never resolve a relative URL here. */
static inline int ib_initial_endpoint_matches(const char *requested,
                                               const char *observed, size_t observed_length) {
    size_t length = strlen(requested);
    if (!ib_observed_url(requested, length)) return 0;
    size_t scheme = memcmp(requested, "https://", 8U) == 0 ? 8U : 7U;
    size_t authority_end = scheme;
    while (authority_end < length && requested[authority_end] != '/' &&
           requested[authority_end] != '?' && requested[authority_end] != '#') ++authority_end;
    size_t host_end = authority_end;
    for (size_t i = scheme; i < authority_end; ++i) {
        if (requested[i] == ':') { host_end = i; break; }
    }
    if (host_end == scheme) return 0;
    unsigned port = scheme == 8U ? 443U : 80U;
    if (host_end < authority_end) {
        port = 0;
        if (host_end + 1U == authority_end) return 0;
        for (size_t i = host_end + 1U; i < authority_end; ++i) {
            if (requested[i] < '0' || requested[i] > '9') return 0;
            port = port * 10U + (unsigned)(requested[i] - '0');
            if (port > 65535U) return 0;
        }
        if (!port) return 0;
    }
    char canonical[IB_OBSERVED_URL_CAP + 2U];
    size_t used = host_end;
    memcpy(canonical, requested, host_end);
    if (port != (scheme == 8U ? 443U : 80U)) {
        char digits[5];
        size_t count = 0;
        for (unsigned value = port; value; value /= 10U) digits[count++] = (char)('0' + value % 10U);
        if (used + count + 1U > IB_OBSERVED_URL_CAP) return 0;
        canonical[used++] = ':';
        while (count) canonical[used++] = digits[--count];
    }
    size_t target_end = authority_end;
    while (target_end < length && requested[target_end] != '#') ++target_end;
    if (authority_end == target_end || requested[authority_end] == '?') canonical[used++] = '/';
    size_t target_length = target_end - authority_end;
    if (used + target_length > IB_OBSERVED_URL_CAP) return 0;
    memcpy(canonical + used, requested + authority_end, target_length);
    used += target_length;
    return used == observed_length && memcmp(canonical, observed, used) == 0;
}

static inline int ib_redirect_chain_valid(const char *wire,
                                           const char *requested,
                                           const char *resolved,
                                           unsigned redirects) {
    if (!wire || !requested || !resolved || redirects > 8U) return 0;
    size_t size = 0;
    while (size <= IB_REDIRECT_OBSERVATION_CAP && wire[size]) ++size;
    if (size < 4U || size > IB_REDIRECT_OBSERVATION_CAP ||
        memcmp(wire, "R1 ", 3U) != 0) return 0;
    const char *position = wire + 3U, *end = wire + size, *url = NULL;
    size_t length = 0;
    if (!ib_redirect_url(&position, end, &url, &length) ||
        !ib_initial_endpoint_matches(requested, url, length))
        return 0;
    for (unsigned hop = 0; hop < redirects; ++hop) {
        if (end - position < 4 || position[3] != ' ') return 0;
        if (!(memcmp(position, "301", 3U) == 0 || memcmp(position, "302", 3U) == 0 ||
              memcmp(position, "303", 3U) == 0 || memcmp(position, "307", 3U) == 0 ||
              memcmp(position, "308", 3U) == 0)) return 0;
        position += 4U;
        if (!ib_redirect_url(&position, end, &url, &length)) return 0;
    }
    return position == end && strlen(resolved) == length &&
           memcmp(resolved, url, length) == 0;
}

#endif
