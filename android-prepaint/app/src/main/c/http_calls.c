#include <android/looper.h>
#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

struct ib_http_result {
    int curl_code;
    long http_status;
    size_t length;
    int truncated;
    char error[192];
};

struct ib_body_sink {
    unsigned char *bytes;
    size_t capacity;
    size_t length;
    int truncated;
};

static int notification_pipe[2] = {-1, -1};
static void (*main_ready_callback)(void);
static atomic_int transfer_busy;

static size_t receive_body(char *incoming, size_t size, size_t count,
                           void *opaque) {
    struct ib_body_sink *sink = opaque;
    if (size != 0 && count > SIZE_MAX / size) {
        sink->truncated = 1;
        return 0;
    }
    size_t amount = size * count;
    size_t remaining = sink->capacity - sink->length;
    if (amount > remaining) {
        if (remaining != 0) {
            memcpy(sink->bytes + sink->length, incoming, remaining);
            sink->length += remaining;
        }
        sink->truncated = 1;
        return 0; /* Bound both memory and network use. */
    }
    memcpy(sink->bytes + sink->length, incoming, amount);
    sink->length += amount;
    return amount;
}

int ib_http_get(const char *url, const void *ca_data, size_t ca_length,
                unsigned char *body, size_t body_capacity,
                struct ib_http_result *result) {
    CURL *request;
    CURLcode code;
    struct curl_blob authorities;
    struct ib_body_sink sink;

    if (url == NULL || ca_data == NULL || ca_length == 0 || body == NULL ||
        body_capacity == 0 || result == NULL) {
        return (int) CURLE_BAD_FUNCTION_ARGUMENT;
    }

    memset(result, 0, sizeof(*result));
    memset(&sink, 0, sizeof(sink));
    sink.bytes = body;
    sink.capacity = body_capacity;
    authorities.data = (void *) ca_data;
    authorities.len = ca_length;
    authorities.flags = CURL_BLOB_NOCOPY;

    request = curl_easy_init();
    if (request == NULL) {
        result->curl_code = (int) CURLE_FAILED_INIT;
        return result->curl_code;
    }

#define IB_SETOPT(option, value)                                                \
    do {                                                                         \
        code = curl_easy_setopt(request, (option), (value));                     \
        if (code != CURLE_OK) goto complete;                                     \
    } while (0)

    code = CURLE_OK;
    IB_SETOPT(CURLOPT_URL, url);
    IB_SETOPT(CURLOPT_PROTOCOLS_STR, "https");
    IB_SETOPT(CURLOPT_REDIR_PROTOCOLS_STR, "https");
    IB_SETOPT(CURLOPT_FOLLOWLOCATION, 1L);
    IB_SETOPT(CURLOPT_MAXREDIRS, 5L);
    IB_SETOPT(CURLOPT_CONNECTTIMEOUT_MS, 10000L);
    IB_SETOPT(CURLOPT_TIMEOUT_MS, 30000L);
    IB_SETOPT(CURLOPT_NOSIGNAL, 1L);
    IB_SETOPT(CURLOPT_USERAGENT, "IB-D/0.4");
    IB_SETOPT(CURLOPT_SSL_VERIFYPEER, 1L);
    IB_SETOPT(CURLOPT_SSL_VERIFYHOST, 2L);
    IB_SETOPT(CURLOPT_CAINFO_BLOB, &authorities);
    IB_SETOPT(CURLOPT_WRITEFUNCTION, receive_body);
    IB_SETOPT(CURLOPT_WRITEDATA, &sink);
    IB_SETOPT(CURLOPT_ERRORBUFFER, result->error);
    code = curl_easy_perform(request);

complete:
    result->curl_code = (int) code;
    result->length = sink.length;
    result->truncated = sink.truncated;
    (void) curl_easy_getinfo(request, CURLINFO_RESPONSE_CODE,
                             &result->http_status);
    if (result->error[0] == '\0' && code != CURLE_OK) {
        const char *description = curl_easy_strerror(code);
        if (description != NULL) {
            strncpy(result->error, description, sizeof(result->error) - 1);
            result->error[sizeof(result->error) - 1] = '\0';
        }
    }
    curl_easy_cleanup(request);
    return result->curl_code;
#undef IB_SETOPT
}

static int notification_ready(int descriptor, int events, void *opaque) {
    unsigned char bytes[32];
    (void) events;
    (void) opaque;
    while (read(descriptor, bytes, sizeof(bytes)) > 0) {
    }
    if (main_ready_callback != NULL) {
        main_ready_callback();
    }
    return 1;
}

int ib_network_prepare(void (*ready_callback)(void)) {
    ALooper *looper;
    int flags;
    if (notification_pipe[0] >= 0) {
        main_ready_callback = ready_callback;
        return 0;
    }
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        return -1;
    }
    if (pipe(notification_pipe) != 0) {
        return -2;
    }
    flags = fcntl(notification_pipe[0], F_GETFL, 0);
    if (flags >= 0) {
        (void) fcntl(notification_pipe[0], F_SETFL, flags | O_NONBLOCK);
    }
    looper = ALooper_forThread();
    if (looper == NULL ||
        ALooper_addFd(looper, notification_pipe[0], ALOOPER_POLL_CALLBACK,
                      ALOOPER_EVENT_INPUT, notification_ready, NULL) < 0) {
        close(notification_pipe[0]);
        close(notification_pipe[1]);
        notification_pipe[0] = -1;
        notification_pipe[1] = -1;
        return -3;
    }
    main_ready_callback = ready_callback;
    return 0;
}

void ib_network_signal_main(void) {
    const unsigned char ready = 1;
    if (notification_pipe[1] >= 0) {
        (void) write(notification_pipe[1], &ready, sizeof(ready));
    }
}

int ib_network_try_begin(void) {
    int expected = 0;
    return atomic_compare_exchange_strong_explicit(
        &transfer_busy, &expected, 1, memory_order_acq_rel,
        memory_order_acquire);
}

void ib_network_end(void) {
    atomic_store_explicit(&transfer_busy, 0, memory_order_release);
}

int ib_start_detached_thread(void *(*entry)(void *), void *argument) {
    pthread_t thread;
    int result = pthread_create(&thread, NULL, entry, argument);
    if (result == 0) {
        (void) pthread_detach(thread);
    }
    return result;
}
