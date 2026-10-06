# Drive authorization receiver: current-line reconciliation

The existing Java/Google Identity Services receiver is independent of native
ABIs. At e8164d978e35e2b4188c3a0fbd403e181b29e74d the exact hosted APK artifact
10707349978 contains no `lib/` native payload. Source minSdk is 26, target and
compile SDK are 36. Cat Food's retained C67 profile observes API 34: the minimum
is satisfied; compile/target SDK are not minimum runtime requirements. Google
Play Services availability, consent, callback and durable credential exchange
still need physical C67 evidence. No arm64 port is required.

Reconciliation preserves current main's LongView launcher, semantic/core tests,
durable task path, exact-head build and canonical package/signing lane. The
separate existing incremental activity still owns the experimental receiver;
its PiP/foreground-service tests apply to that component only. It does not become
the main browser architecture. Loopback remains a provisional lowering under
cloud-storage-api #12, not a new broker or proof of Binder/PFD accessibility.

Package remains `org.isomorphisms.ib.webview`, versionCode 7 (current independent
Longview/latency candidates already use 6). The current build
uses main's stable test certificate (SHA-256
DE9B1D47C5A65E6D46A204B79DD9EE566B9D3C9832BA81EBC4213D3392E92FF9).
The old branch's runner-local debug key is not update identity. A new build
cannot replace an installed APK signed by that old key. No uninstall or signer
migration is authorized. Do not introduce a second package to bypass it.

Live Google authorization must select this same package and a stable signing
certificate approved for OAuth. The canonical public test key is inspection/
test evidence, not an assumed private OAuth identity. Register the actual
approved certificate's SHA-1 with the Android OAuth client for this package;
configure a matching Web application client ID for requestOfflineAccess and
cloud-storage-api's private client credential. Enable Drive API, configure the
consent screen/user grant and exact drive.readonly scope. Do not store client
secrets, refresh tokens or auth codes in IB history, logs, clipboard or Git.

This receiver accepts drive.readonly only. SDF archive writes additionally
need drive.file; the desktop/private credential route can request it. Android
write-scope consent is still missing rather than silently broadening this
allowlist. Consumer death before the one-time code is delivered requires a new
consent attempt; the code is never made ordinary durable state.

Required physical evidence: exact APK/source/signer/package/version, C67
firmware and Play Services, explicit consent result, same pending local request,
successful private credential commit and a subsequent authenticated Drive
operation after restart. CI build/replacement installation on an emulator is
not C67 authorization. This branch remains draft until its current-head checks
and required physical gates are met.
