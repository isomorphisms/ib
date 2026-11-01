#ifndef IB_READER_INDEX_H
#define IB_READER_INDEX_H
#include <stdint.h>

#define IB_READER_JOURNAL_CAP 12288U
typedef struct {
    char source_id[65];
    uint32_t source_generation;
    char journal[IB_READER_JOURNAL_CAP + 1U];
    int previous;
} ib_reader_index;

/* Ordinary-file metadata only. Idriç owns the journal's navigation semantics;
 * canonical Arena receipts and independent byte readback grant admission. */
/* Return 0 for a verified head, 1 if both heads are absent, -1 on refusal. */
int ib_reader_index_read(int root_fd, unsigned first, ib_reader_index *out);
/* A reported sync failure can leave a valid but durability-uncertain head.
 * Previous committed sources are never removed by this adapter. */
int ib_reader_index_publish(int root_fd, const ib_reader_index *index);
#endif
