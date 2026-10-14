#include "../ib_platform.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>

int main(void)
{
    const char utf8[] = "A東京🙂éB";
    assert(ib_macos_abi_version() == 1);
    assert(ib_macos_pointer_bits() == (int)(sizeof(void *) * CHAR_BIT));
    assert(ib_macos_pointer_bits() == 64);
    assert(ib_macos_utf8_bytes(utf8) == (int)strlen(utf8));
    assert(ib_macos_utf8_bytes("A") == 1);
    assert(ib_macos_utf8_bytes("") == 0);
    assert(ib_macos_utf8_bytes(NULL) == -1);
    return 0;
}
