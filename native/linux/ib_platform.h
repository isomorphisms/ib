#ifndef IB_LINUX_PLATFORM_H
#define IB_LINUX_PLATFORM_H

/* Borrowed strings are used only during the call. No browser state lives here. */
int ib_linux_abi_version(void);
int ib_linux_pointer_bits(void);
int ib_linux_utf8_bytes(const char *text);

#endif
