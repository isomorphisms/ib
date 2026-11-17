#define _POSIX_C_SOURCE 200809L
#include "../../native/android/ib_admin_transaction.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void reopen(ib_admin_transaction *state, const char *root, const char *task,
                   ib_admin_phase expected, unsigned revision) {
    ib_admin_close(state);
    assert(ib_admin_open(state, root, task) == 0);
    assert(state->phase == expected && state->revision == revision);
}

int main(void) {
    char root[] = "/tmp/ib-admin-native-test-XXXXXX";
    assert(mkdtemp(root));
    assert(chmod(root, 0700) == 0);
    ib_admin_transaction tx;
    assert(ib_admin_open(&tx, root, "taskOne") == 0);
    assert(tx.phase == IB_ADMIN_NEW);
    assert(ib_admin_prepare(&tx, "opOne") != 0);
    assert(ib_admin_checkpoint_label(&tx, "labelOne") == 0);
    assert(ib_admin_checkpoint_label(&tx, "labelTwo") == 0);
    assert(ib_admin_checkpoint_label(&tx, "bad/label") != 0);
    assert(ib_admin_checkpoint_label(&tx, "key\nredacted") != 0);
    assert(ib_admin_prepare(&tx, "opOne") == 0);
    assert(ib_admin_can_dispatch(&tx));
    assert(ib_admin_prepare(&tx, "opTwo") != 0);
    reopen(&tx, root, "taskOne", IB_ADMIN_PREPARED, 3);
    assert(ib_admin_can_dispatch(&tx));
    /* MUST be durably marked before issuing the POST. Conservatively mark
     * uncertain even if the network request ultimately never left the host. */
    assert(ib_admin_mark_may_have_applied(&tx) == 0);
    assert(!ib_admin_can_dispatch(&tx));
    assert(ib_admin_mark_may_have_applied(&tx) != 0);
    assert(ib_admin_checkpoint_label(&tx, "another") != 0);
    assert(ib_admin_observe_applied(&tx, "wrongOperation") != 0);
    reopen(&tx, root, "taskOne", IB_ADMIN_MAY_HAVE_APPLIED, 4);
    assert(!ib_admin_can_dispatch(&tx));
    assert(ib_admin_observe_applied(&tx, "opOne") == 0);
    reopen(&tx, root, "taskOne", IB_ADMIN_OBSERVED_APPLIED, 5);
    assert(ib_admin_mark_may_have_applied(&tx) != 0);
    ib_admin_close(&tx);

    /* A second actor cannot silently write an event with the same revision. */
    ib_admin_transaction first, second;
    assert(ib_admin_open(&first, root, "race") == 0);
    assert(ib_admin_open(&second, root, "race") == 0);
    assert(ib_admin_checkpoint_label(&first, "safe") == 0);
    assert(ib_admin_checkpoint_label(&second, "stale") != 0);
    ib_admin_close(&first);
    ib_admin_close(&second);

    /* An event gap/corruption is fail-closed; no reconstruction from page text. */
    assert(ib_admin_open(&tx, root, "corrupt") == 0);
    assert(ib_admin_checkpoint_label(&tx, "safe") == 0);
    ib_admin_close(&tx);
    char filename[512];
    snprintf(filename, sizeof(filename), "%s/admin-action-journal/corrupt/00000001.event", root);
    int fd = open(filename, O_WRONLY | O_TRUNC);
    assert(fd >= 0);
    assert(write(fd, "oops\n", 5) == 5);
    close(fd);
    assert(ib_admin_open(&tx, root, "corrupt") != 0);

    char link_name[512];
    snprintf(link_name, sizeof(link_name), "%s/admin-action-journal/fake", root);
    assert(symlink("taskOne", link_name) == 0);
    assert(ib_admin_open(&tx, root, "fake") != 0);

    /* Cleanup intentionally left to private disposable test directory; the
     * hosted runner deletes the whole temporary root after inspection. */
    printf("PASS native admin action journal: recovery, no replay, stale writer, corruption, symlink\n");
    return 0;
}
