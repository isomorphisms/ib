#include "ib_platform.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#if !defined(__APPLE__) || !defined(__MACH__) || \
    !(defined(__arm64__) || defined(__aarch64__) || defined(__x86_64__))
#error IB_MACOS_ARM64_OR_X86_64_REQUIRED
#endif

int ib_macos_abi_version(void)
{
    return 1;
}

int ib_macos_pointer_bits(void)
{
    return (int)(sizeof(void *) * CHAR_BIT);
}

int ib_macos_utf8_bytes(const char *text)
{
    if (text == NULL) {
        return -1;
    }
    size_t bytes = strlen(text);
    if (bytes > INT_MAX) {
        return -1;
    }
    return (int)bytes;
}
