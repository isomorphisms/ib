#ifndef IB_LONGVIEW_STORE_H
#define IB_LONGVIEW_STORE_H

#include <stddef.h>

/* This adapter stores opaque checked Idriç records. It does not decode tasks,
 * infer authentication, retain page URLs, or decide a lifecycle transition. */
#define IB_LONGVIEW_ID_CAP 64U
#define IB_LONGVIEW_RECORD_CAP 16384U

typedef struct {
    int state;
    int tasks;
    int tabs;
} ib_longview_store;

void ib_longview_store_close(ib_longview_store *store);
int ib_longview_store_open(ib_longview_store *store, const char *trusted_files_root);
/* 0: selected record, 1: no prior state, -1: invalid/incomplete/refused state. */
int ib_longview_store_read(ib_longview_store *store,
    char task[IB_LONGVIEW_ID_CAP + 1U], char tab[IB_LONGVIEW_ID_CAP + 1U],
    char navigation[IB_LONGVIEW_ID_CAP + 1U],
    unsigned char record[IB_LONGVIEW_RECORD_CAP + 1U], size_t *record_bytes);
/* New selection is committed only after task, tab and history are durable.
 * Existing checkpoints may only update the currently selected task. */
int ib_longview_store_commit(ib_longview_store *store, const char *task,
    const char *tab, const char *navigation, const unsigned char *record,
    size_t record_bytes, int new_selection);

#endif
