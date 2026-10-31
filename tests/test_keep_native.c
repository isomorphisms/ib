#include "../native/android/ib_keep_share.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    uint16_t out[20] = {0};
    size_t used = 0;
    const unsigned char valid[] = "Merhaba Tu\304\237ba! \360\237\214\215\n";
    assert(ib_keep_utf8_to_utf16(valid, sizeof(valid) - 1, out, 20, &used));
    assert(used == 18);
    assert(out[10] == 0x011f); /* Turkish g breve */
    assert(out[15] == 0xd83c && out[16] == 0xdf0d); /* Earth emoji */
    const unsigned char bad_overlong[] = {0xc0, 0x80};
    const unsigned char bad_surrogate[] = {0xed, 0xa0, 0x80};
    const unsigned char bad_truncated[] = {0xe2, 0x82};
    const unsigned char nul[] = {'a', 0, 'b'};
    assert(!ib_keep_utf8_to_utf16(bad_overlong, sizeof bad_overlong, out, 20, &used));
    assert(!ib_keep_utf8_to_utf16(bad_surrogate, sizeof bad_surrogate, out, 20, &used));
    assert(!ib_keep_utf8_to_utf16(bad_truncated, sizeof bad_truncated, out, 20, &used));
    assert(!ib_keep_utf8_to_utf16(nul, sizeof nul, out, 20, &used));
    assert(!ib_keep_utf8_to_utf16(valid, sizeof(valid) - 1, out, 1, &used));
    puts("keep-native-utf8=PASS");
    return 0;
}
