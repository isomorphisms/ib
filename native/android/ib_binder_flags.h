#ifndef IB_BINDER_FLAGS_H
#define IB_BINDER_FLAGS_H

/* libbinder IPCThreadState::transact adds TF_ACCEPT_FDS (0x10) before
 * dispatching Java Binder.onTransact. It describes reply transport, not an
 * authorization to carry descriptors in this request. Each owner separately
 * checks Parcel.hasFileDescriptors and the exact private request schema.
 * AOSP frameworks/native e59fde00320dcb3a8c927eaa84ab5d9fed003164,
 * libs/binder/IPCThreadState.cpp. Reject all other unexpected flag bits. */
static inline int ib_binder_synchronous(int flags) {
    return (flags & ~0x10) == 0;
}

static inline int ib_binder_oneway(int flags) {
    return (flags & ~0x10) == 1;
}

#endif
