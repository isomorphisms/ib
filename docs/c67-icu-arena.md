# C67 ICU → Shizuku → Arena experimental slice

Status: **host-checked staging only; not a URL-capable IB APK**.

The physical MIRO C67 launches the checked native Material reader in IB PR #129, but that reader only paints a fixed README. This branch adds a separate source-byte path:

    browser-owned request [future Android Binder command]
      → Binder authority / Shizuku provider (rish -c today)
      → ICU's checked Android app_process DEX + native TLS library
      → bounded temporary response; require producer exit 0
      → preinitialized canonical appendfat userspace Arena
      → verify independently reopened committed offset + length
      → immutable private receipt (requested URL, SHA256, range)
      → Idriç extraction → prepaint → native reader [NOT WIRED]

The storage backend is the **userspace** arena in isomorphisms/sd-card-append-fat, not an assertion that kernel appendfat exists. The native reader's CLI has no socket operations or separate source store. No direct app-side HTTP, WebView fetch, or Java application is introduced.

## How the bounded stage works

The owning process must select an existing **0700** private Arena directory and create the canonical arena first, for example:

    appendfat_arena create PRIVATE_DIR/responses.arena 16777216

The runtime requires the real pinned Android ICU DEX and ARM32 native library visible to Shizuku's shell as /data/local/tmp/ib-icu/classes.dex and libicu_transport.so. Set IB_C67_ICU_DEX, IB_C67_ICU_SO, IB_C67_ICU_DEX_SHA256, IB_C67_ICU_SO_SHA256; set IB_APPEND_ARENA and IB_ARENA_SLICE to exact qualified executables. Shizuku must be started and authorized. These artifacts and permissions are NOT YET bundled into a C67 app.

    sh bin/ib_android_icu_arena.grease REQUEST_ID https://example.com/ PRIVATE_DIR

This is a standalone development invocation, **not a suggested usable phone command yet**: the command's owning process must have access to the private Arena, and cross-UID publication must eventually use scoped Binder/PFD or a qualified provider rather than a shared pathname. The shell provider is invoked through a Binder-named capability, not directly by the display reader.

A request ID is single-use. A failed request retains its lock for investigation, requiring a fresh ID to retry; it does not publish a receipt. The cap is 4 MiB. Failure after append but before receipt publication leaves an unreferenced committed tail, which cannot be mistaken for accepted browser history. An admitted source must require the receipt plus a byte-range hash check; it must never trust merely observing a large .arena file.

ICU presently emits the body without reliable final resolved URL, HTTP status or MIME/charset observations. Accordingly the receipt marks these UNOBSERVED and navigation_admitted=NO. Untrusted body bytes cannot update source identity, history or tab state. Private receipt text contains the requested URL; no authenticated URLs should be used until credential redaction policy is qualified.

## Evidence and remaining work

The host test uses an explicitly **fake** Shizuku provider with real canonical appendfat_arena bytes and real C read/sync helper. It checks binary fidelity, duplicate IDs, denial, partial exit, overrun, unsafe URL, corrupt .used pointer and replay. The GitHub Actions lane additionally cross-builds the read helper for Android ARM32 and ARM64; these are not Android network or GUI tests.

Not done: producing/distributing exact Idriç ICU Android DEX plus ARM native runtime; real C67 Shizuku/ICU HTTPS; private Binder/PFD source delivery to the app; ICU metadata extension; source-linked HTML extraction and link handling; C67 APK with URL entry. Track IB #151, #142, #141, #97 and native reader PRs #129 and #147.
