#ifndef IB_ADMIN_TRANSACTION_H
#define IB_ADMIN_TRANSACTION_H

/* Issue #100: browser-owned, non-secret administrative mutation journal.
 * This is an action journal, not a credential vault or another source store.
 * The existing Arena owner must admit/read back ordinary source results.
 * All operations are fail-closed. The caller owns the authentication and
 * actual browser/renderer boundary; page text may not authorize an action. */

#include <stdint.h>

#define IB_ADMIN_TASK_CAP 48
#define IB_ADMIN_LABEL_CAP 48
#define IB_ADMIN_OPERATION_CAP 64
#define IB_ADMIN_MAX_EVENTS 64

typedef enum {
    IB_ADMIN_NEW = 0,
    IB_ADMIN_DRAFT = 1,
    IB_ADMIN_PREPARED = 2,
    IB_ADMIN_MAY_HAVE_APPLIED = 3,
    IB_ADMIN_OBSERVED_APPLIED = 4
} ib_admin_phase;

typedef struct {
    int directory_fd; /* owned openat() capability; never renderer-owned */
    uint32_t revision;
    ib_admin_phase phase;
    char ordinary_label[IB_ADMIN_LABEL_CAP + 1];
    char operation_id[IB_ADMIN_OPERATION_CAP + 1];
} ib_admin_transaction;

/* app_files_directory must be the trusted app-private Context.getFilesDir().
 * task_id is an IB-minted opaque identity, not a URL or user input.
 * Create/open 0700 action-journal directory without symlink traversal.
 * A missing, corrupted, out-of-order or partial event refuses recovery. */
int ib_admin_open(ib_admin_transaction *transaction, const char *app_files_directory,
                  const char *task_id);
void ib_admin_close(ib_admin_transaction *transaction);

/* Only a strictly allowlisted, ordinary label is checkpointed.
 * Passwords, cookies, API keys, verification codes, URLs and form bodies
 * are not accepted by this API. The checked caller chooses the field. */
int ib_admin_checkpoint_label(ib_admin_transaction *transaction, const char *ordinary_label);

/* Prepared intent is durable BEFORE any mutation. The opaque operation ID
 * must not itself be a secret. There is no implicit authorization: the
 * browser-owned UI must require explicit user approval before dispatch. */
int ib_admin_prepare(ib_admin_transaction *transaction, const char *operation_id);

/* Durable MAY_HAVE_APPLIED precedes the HTTP POST. Crash, timeout, app switch
 * or lost response may NOT cause a second dispatch, even if the first POST
 * never reached the server. No automatic transitions out of this phase. */
int ib_admin_mark_may_have_applied(ib_admin_transaction *transaction);
int ib_admin_can_dispatch(const ib_admin_transaction *transaction);

/* ONLY after an independently authenticated read-after-write observation
 * binds the same operation ID. A page-supplied success string is insufficient.
 * The returned one-time secret is NEVER passed to this API or persisted. */
int ib_admin_observe_applied(ib_admin_transaction *transaction, const char *operation_id);

const char *ib_admin_phase_name(ib_admin_phase phase);

#endif
