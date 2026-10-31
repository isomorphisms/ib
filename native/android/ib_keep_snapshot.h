#ifndef IB_KEEP_SNAPSHOT_H
#define IB_KEEP_SNAPSHOT_H
#include <stddef.h>

/* Opaque bytes. Idriç validates their schema and controls every state change.
 * Directory must already be a private, opened, trusted storage directory. */
#define IB_KEEP_SNAPSHOT_MAX 512U
int ib_keep_snapshot_replace(int private_dir_fd, const char *basename,
                             const unsigned char *bytes, size_t length);
int ib_keep_snapshot_read(int private_dir_fd, const char *basename,
                          unsigned char *bytes, size_t capacity,
                          size_t *length);
#endif
