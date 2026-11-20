#include "ib_binder_flags.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    assert(ib_binder_synchronous(0));
    assert(ib_binder_synchronous(16));
    puts("STAGE framework AppOps synchronous flag");
    assert(ib_binder_synchronous(2));
    assert(ib_binder_synchronous(18));
    assert(ib_binder_oneway(1));
    assert(ib_binder_oneway(17));
    assert(!ib_binder_synchronous(1));
    assert(!ib_binder_synchronous(3));
    assert(!ib_binder_synchronous(17));
    assert(!ib_binder_synchronous(19));
    assert(!ib_binder_synchronous(32));
    assert(!ib_binder_synchronous(48));
    assert(!ib_binder_synchronous(64));
    assert(!ib_binder_synchronous(-1));
    assert(!ib_binder_oneway(0));
    assert(!ib_binder_oneway(2));
    assert(!ib_binder_oneway(3));
    assert(!ib_binder_oneway(16));
    assert(!ib_binder_oneway(18));
    assert(!ib_binder_oneway(19));
    assert(!ib_binder_oneway(33));
    assert(!ib_binder_oneway(-1));
    puts("PASS exact Binder direction and framework transport flag controls");
    return 0;
}
