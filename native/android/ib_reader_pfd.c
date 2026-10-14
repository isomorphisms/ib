#define _POSIX_C_SOURCE 200809L
#include "ib_reader_pfd.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define IB_PFD_READ_DEADLINE_MS 5000

typedef struct {
    uint32_t word[8];
    unsigned char block[64];
    size_t used;
    uint64_t processed_bits;
} sha256_state;

static uint32_t rotate(uint32_t value, unsigned int count) {
    return (value >> count) | (value << (32U - count));
}

static const uint32_t sha256_constants[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

static void sha256_transform(sha256_state *state,
                             const unsigned char input[64]) {
    uint32_t words[64], registers[8];
    for (size_t index = 0; index < 16; ++index) {
        size_t at = index * 4U;
        words[index] = ((uint32_t)input[at] << 24U) |
                       ((uint32_t)input[at + 1U] << 16U) |
                       ((uint32_t)input[at + 2U] << 8U) |
                       (uint32_t)input[at + 3U];
    }
    for (size_t index = 16; index < 64; ++index) {
        uint32_t a = words[index - 15U], b = words[index - 2U];
        uint32_t s0 = rotate(a, 7U) ^ rotate(a, 18U) ^ (a >> 3U);
        uint32_t s1 = rotate(b, 17U) ^ rotate(b, 19U) ^ (b >> 10U);
        words[index] = words[index - 16U] + s0 +
                       words[index - 7U] + s1;
    }
    memcpy(registers, state->word, sizeof(registers));
    for (size_t index = 0; index < 64; ++index) {
        uint32_t s1 = rotate(registers[4], 6U) ^
                      rotate(registers[4], 11U) ^
                      rotate(registers[4], 25U);
        uint32_t choose = (registers[4] & registers[5]) ^
                          (~registers[4] & registers[6]);
        uint32_t first = registers[7] + s1 + choose +
                         sha256_constants[index] + words[index];
        uint32_t s0 = rotate(registers[0], 2U) ^
                      rotate(registers[0], 13U) ^
                      rotate(registers[0], 22U);
        uint32_t majority = (registers[0] & registers[1]) ^
                            (registers[0] & registers[2]) ^
                            (registers[1] & registers[2]);
        uint32_t second = s0 + majority;
        for (size_t at = 7; at > 0; --at) registers[at] = registers[at - 1U];
        registers[4] += first;
        registers[0] = first + second;
    }
    for (size_t index = 0; index < 8; ++index)
        state->word[index] += registers[index];
}

static void sha256_init(sha256_state *state) {
    static const uint32_t initial[8] = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U
    };
    memcpy(state->word, initial, sizeof(initial));
    state->used = 0;
    state->processed_bits = 0;
}

static void sha256_update(sha256_state *state, const unsigned char *body,
                          size_t count) {
    for (size_t index = 0; index < count; ++index) {
        state->block[state->used++] = body[index];
        if (state->used == sizeof(state->block)) {
            sha256_transform(state, state->block);
            state->processed_bits += 512U;
            state->used = 0;
        }
    }
}

static void sha256_finish(sha256_state *state, unsigned char digest[32]) {
    uint64_t bits = state->processed_bits + (uint64_t)state->used * 8U;
    state->block[state->used++] = 0x80U;
    if (state->used > 56U) {
        memset(state->block + state->used, 0, 64U - state->used);
        sha256_transform(state, state->block);
        state->used = 0;
    }
    memset(state->block + state->used, 0, 56U - state->used);
    for (size_t index = 0; index < 8; ++index)
        state->block[56U + index] =
            (unsigned char)(bits >> (56U - 8U * index));
    sha256_transform(state, state->block);
    for (size_t index = 0; index < 32; ++index)
        digest[index] = (unsigned char)
            (state->word[index / 4U] >> (24U - 8U * (index % 4U)));
}

static int valid_ticket(const ib_reader_source_ticket *ticket) {
    if (!ticket || ticket->navigation_generation == 0 ||
        ticket->byte_count == 0 ||
        ticket->byte_count > IB_READER_SOURCE_MAX_BYTES)
        return 0;
    size_t length = strnlen(ticket->request_id,
                            sizeof(ticket->request_id));
    if (length == 0 || length > IB_READER_REQUEST_ID_CAP)
        return 0;
    for (size_t index = 0; index < length; ++index) {
        unsigned char ch = (unsigned char)ticket->request_id[index];
        if (!((ch >= 'a' && ch <= 'z') ||
              (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') ||
              ch == '.' || ch == '_' || ch == '-'))
            return 0;
    }
    if (strnlen(ticket->sha256_hex, sizeof(ticket->sha256_hex)) != 64U)
        return 0;
    for (size_t index = 0; index < 64; ++index) {
        char ch = ticket->sha256_hex[index];
        if (!((ch >= '0' && ch <= '9') ||
              (ch >= 'a' && ch <= 'f')))
            return 0;
    }
    return 1;
}

void ib_reader_sha256_hex(const unsigned char *bytes, size_t count,
                          char hex[65]) {
    static const char alphabet[] = "0123456789abcdef";
    sha256_state hash;
    unsigned char digest[32];
    sha256_init(&hash);
    if (count != 0) sha256_update(&hash, bytes, count);
    sha256_finish(&hash, digest);
    for (size_t index = 0; index < 32; ++index) {
        hex[index * 2U] = alphabet[digest[index] >> 4U];
        hex[index * 2U + 1U] = alphabet[digest[index] & 15U];
    }
    hex[64] = '\0';
}

static int matching_digest(const unsigned char digest[32],
                           const char expected[65]) {
    static const char alphabet[] = "0123456789abcdef";
    unsigned int differences = 0;
    for (size_t index = 0; index < 32; ++index) {
        differences |= (unsigned int)
            (alphabet[digest[index] >> 4U] != expected[index * 2U]);
        differences |= (unsigned int)
            (alphabet[digest[index] & 15U] != expected[index * 2U + 1U]);
    }
    return differences == 0;
}

static int64_t monotonic_milliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return -1;
    return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static ssize_t read_bounded(int fd, void *buffer, size_t capacity,
                            int64_t deadline) {
    for (;;) {
        int64_t now = monotonic_milliseconds();
        if (now < 0 || now >= deadline) return -1;
        int64_t remaining = deadline - now;
        struct pollfd descriptor = { fd, POLLIN | POLLHUP, 0 };
        int status = poll(&descriptor, 1, remaining > INT_MAX ?
                         INT_MAX : (int)remaining);
        if (status < 0 && errno == EINTR) continue;
        if (status <= 0 || (descriptor.revents & (POLLERR | POLLNVAL)))
            return -1;
        ssize_t count = read(fd, buffer, capacity);
        if (count < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        return count;
    }
}

void ib_reader_source_init(ib_reader_source *source) {
    if (!source) return;
    atomic_init(&source->wanted_navigation_generation, 0);
    source->bytes = NULL;
    source->byte_count = 0;
    source->published_navigation_generation = 0;
    source->request_id[0] = '\0';
}

void ib_reader_source_clear(ib_reader_source *source) {
    if (!source) return;
    free(source->bytes);
    source->bytes = NULL;
    source->byte_count = 0;
    source->published_navigation_generation = 0;
    source->request_id[0] = '\0';
    atomic_store(&source->wanted_navigation_generation, 0);
}

void ib_reader_source_want(ib_reader_source *source, uint64_t generation) {
    if (source) atomic_store(&source->wanted_navigation_generation, generation);
}

int ib_reader_source_accept_fd(ib_reader_source *source,
                               const ib_reader_source_ticket *ticket, int fd) {
    if (!source || !valid_ticket(ticket) || fd < 0)
        return IB_READER_PFD_BAD_TICKET;
    if (ticket->navigation_generation !=
        atomic_load(&source->wanted_navigation_generation))
        return IB_READER_PFD_STALE;
    /* A bounded source pipe, never the backing Arena file or a writable FD. */
    struct stat info;
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0 || fstat(fd, &info) != 0 ||
        !S_ISFIFO(info.st_mode) || (flags & O_ACCMODE) != O_RDONLY)
        return IB_READER_PFD_BAD_TICKET;

    unsigned char *candidate = malloc((size_t)ticket->byte_count + 1U);
    if (!candidate) return IB_READER_PFD_IO;
    sha256_state hash;
    sha256_init(&hash);
    size_t received = 0;
    int result = IB_READER_PFD_IO;
    int64_t start = monotonic_milliseconds();
    if (start < 0 || start > INT64_MAX - IB_PFD_READ_DEADLINE_MS)
        goto failed;
    int64_t deadline = start + IB_PFD_READ_DEADLINE_MS;
    while (received < ticket->byte_count) {
        size_t wanted = ticket->byte_count - received;
        if (wanted > 65536U) wanted = 65536U;
        ssize_t count = read_bounded(fd, candidate + received, wanted, deadline);
        if (count <= 0) goto failed;
        sha256_update(&hash, candidate + received, (size_t)count);
        received += (size_t)count;
    }
    /* A source capability must end here: never silently accept a prefix
     * from a larger or still-live stream. */
    unsigned char extra;
    if (read_bounded(fd, &extra, 1, deadline) != 0) goto failed;
    unsigned char digest[32];
    sha256_finish(&hash, digest);
    if (!matching_digest(digest, ticket->sha256_hex)) {
        result = IB_READER_PFD_DIGEST;
        goto failed;
    }
    if (ticket->navigation_generation !=
        atomic_load(&source->wanted_navigation_generation)) {
        result = IB_READER_PFD_STALE;
        goto failed;
    }
    candidate[received] = '\0'; /* sentinel; body remains binary */
    unsigned char *previous = source->bytes;
    source->bytes = candidate;
    source->byte_count = received;
    source->published_navigation_generation = ticket->navigation_generation;
    memcpy(source->request_id, ticket->request_id,
           strlen(ticket->request_id) + 1U);
    free(previous);
    return IB_READER_PFD_ACCEPTED;

failed:
    free(candidate);
    return result;
}

#if defined(__ANDROID__) && __ANDROID_API__ >= 29
#include <android/binder_parcel.h>
#include "ib_arena_pfd_provider.h"

/* The private provider owns authorization and obtains the descriptor from a
 * committed, receipt-verified source. The Binder transaction contains only a
 * request identity and epoch; the reply contains a PFD, never a path. */
int ib_reader_source_fetch_binder(ib_reader_source *source,
                                  const ib_reader_source_ticket *ticket,
                                  AIBinder *provider) {
    if (!source || !valid_ticket(ticket) || !provider)
        return IB_READER_PFD_BAD_TICKET;
    if (ticket->navigation_generation !=
        atomic_load(&source->wanted_navigation_generation))
        return IB_READER_PFD_STALE;

    /* A binder proxy requires typed class association before transaction.
     * Check the exact private interface descriptor; never send to an
     * unassociated or unrelated Binder endpoint. */
    const AIBinder_Class *clazz = ib_arena_pfd_provider_class();
    if (!clazz || !AIBinder_associateClass(provider, clazz))
        return IB_READER_PFD_BINDER;

    AParcel *request = NULL, *reply = NULL;
    binder_status_t status = AIBinder_prepareTransaction(provider, &request);
    if (status != STATUS_OK || !request)
        return IB_READER_PFD_BINDER;
    status = AParcel_writeString(request, ticket->request_id,
                                 (int32_t)strlen(ticket->request_id));
    if (status == STATUS_OK)
        status = AParcel_writeInt64(request,
                                   (int64_t)ticket->navigation_generation);
    if (status != STATUS_OK) {
        AParcel_delete(request);
        return IB_READER_PFD_BINDER;
    }
    /* request ownership transfers to AIBinder_transact. */
    status = AIBinder_transact(provider, 1U, &request, &reply, 0);
    if (status != STATUS_OK || !reply) {
        if (reply) AParcel_delete(reply);
        return IB_READER_PFD_BINDER;
    }
    int32_t provider_status = -1;
    int descriptor = -1;
    status = AParcel_readInt32(reply, &provider_status);
    if (status == STATUS_OK && provider_status == 0)
        status = AParcel_readParcelFileDescriptor(reply, &descriptor);
    AParcel_delete(reply);
    if (status != STATUS_OK || provider_status != 0 || descriptor < 0) {
        if (descriptor >= 0) close(descriptor);
        return IB_READER_PFD_BINDER;
    }
    int result = ib_reader_source_accept_fd(source, ticket, descriptor);
    close(descriptor);
    return result;
}
#endif
