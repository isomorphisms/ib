package org.isomorphisms.ib.webview;

import java.util.UUID;

/** One identity per IB host-process lifetime; never persisted as a credential. */
final class HostProcessIdentity {
    static final String VALUE = "host-" + UUID.randomUUID();

    private HostProcessIdentity() {
    }
}
