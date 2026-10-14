#ifndef IB_MACOS_PLATFORM_H
#define IB_MACOS_PLATFORM_H

/* Borrowed UTF-8: no allocation, browser state, or retained caller pointers. */
int ib_macos_abi_version(void);
int ib_macos_pointer_bits(void);
int ib_macos_utf8_bytes(const char *text);

#endif
