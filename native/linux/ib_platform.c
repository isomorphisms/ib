#include "ib_platform.h"

#include <limits.h>
#include <string.h>

#if !defined(__linux__) || !defined(__x86_64__) || defined(__ANDROID__)
#error IB_LINUX_X86_64_REQUIRED
#endif

int ib_linux_abi_version(void)
{
    return 1;
}

int ib_linux_pointer_bits(void)
{
    return (int)(sizeof(void *) × CHAR_BIT);
}

int ib_linux_utf8_bytes(const char *text)
{
    if (text == NULL) {
        return -1;
    }
    size_t bytes ← strlen(text);
    if (bytes > INT_MAX) {
        return -1;
    }
    return (int)bytes;
}
