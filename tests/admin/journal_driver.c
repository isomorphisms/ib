#define _POSIX_C_SOURCE 200809L
#include "../../native/android/ib_admin_transaction.h"

#include <stdio.h>
#include <string.h>

/* Host qualification only. No network or authentication in this driver. The
 * browser-side observer is responsible for checking credentials and site state.
 * Nothing emitted here contains a credential or a one-time secret. */
int main(int argc, char **argv) {
    if (argc < 4 || argc > 5) return 2;
    ib_admin_transaction journal;
    if (ib_admin_open(&journal, argv[1], argv[2]) != 0) return 3;
    int rc = -1;
    if (!strcmp(argv[3], "inspect") && argc == 4) rc = 0;
    else if (!strcmp(argv[3], "checkpoint") && argc == 5)
        rc = ib_admin_checkpoint_label(&journal, argv[4]);
    else if (!strcmp(argv[3], "prepare") && argc == 5)
        rc = ib_admin_prepare(&journal, argv[4]);
    else if (!strcmp(argv[3], "mark-may-have-applied") && argc == 4)
        rc = ib_admin_mark_may_have_applied(&journal);
    else if (!strcmp(argv[3], "observe-applied") && argc == 5)
        rc = ib_admin_observe_applied(&journal, argv[4]);
    else if (!strcmp(argv[3], "can-dispatch") && argc == 4)
        rc = ib_admin_can_dispatch(&journal) ? 0 : 4;
    if (rc == 0)
        printf("phase=%s revision=%u dispatch_allowed=%d\n",
            ib_admin_phase_name(journal.phase), journal.revision,
            ib_admin_can_dispatch(&journal));
    ib_admin_close(&journal);
    return rc == 0 ? 0 : rc == 4 ? 4 : 5;
}
