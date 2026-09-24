# IB WebView protected transactions

The application launcher is now **IB Long View**, the first live browser path
whose protected task survives both renderer replacement and IB host-process
restart.  It uses browser-owned task, tab, and navigation identities under the
app-private `state/` tree.  The task and security model are documented in
[`docs/long-view-protected-tasks.md`](../docs/long-view-protected-tasks.md).

The original issue #59 loopback harness remains in `WebViewActivity` as test
equipment.  It is no longer the launcher and does not own the product model.

## Long-view behavior

The launcher opens a switchable real URL.  The complete URL is used only by the
live renderer; durable state retains only a neutral scheme/host/port origin and
records whether path, query, or fragment material was removed.  On renderer death the
activity attaches a replacement WebView to the same task.  **Kill IB host**
provides the separate whole-process acceptance leg; after relaunch IB discovers
the same task, advances the host generation, and performs safe reconstruction.

WebView's app profile owns cookies.  The task file does not copy them, and a
reloaded page remains `authenticated=not-proven` until **Session works** is
pressed.  **Needs repeat** records an explicit incomplete remote-site state.

The path does not require Picture-in-Picture or a foreground service.  Important
renderer priority remains a survival optimization, not the correctness model.

**Copy receipt** produces a phone-visible receipt without query strings, field
values, cookies, authorization codes, or heap-canary values.  Inspect the copied
text for unexpected secrets before sharing it.


## Issue #84 durable-result provider experiment

This branch adds a deliberately tiny test of a different lifetime boundary:
whether an app-private durable result can outlive the IB process and later be
read by another Android application without moving the result into shared
storage.

The IB package commits exactly one immutable fixture result:

```text
result-id: hello-v1
bytes: hello\n
storage: app-private files/durable-results/
```

`DurableResultProvider` is not the store. It is a non-exported Android
`ContentProvider` that can hand a fresh read-only `ParcelFileDescriptor` to
a package holding an explicit URI grant. A separate APK,
`org.isomorphisms.ib.resultreader`, is the caller shim. It has a distinct
Android UID and reads through `ContentResolver`, not through IB's private
filesystem.

The provider increments a durable provider-generation counter every time a new
provider process is created. The reader queries that metadata before opening
the result, so a physical receipt can distinguish "same provider process" from
"new provider process reading the same stored result."

### MIRO A1 run

Install both APKs from the same workflow artifact. No ADB is required for the
phone run.

From Termux:

```sh
termux-open-url ib://durable-result-fixture
```

The fixture commits the result and grants the reader package access. Then:

```sh
termux-open-url content://org.isomorphisms.ib.webview.results/result/hello-v1
```

The reader opens the result through Android IPC and records its own UID plus
the provider PID, UID, process identity, generation, and calling package/UID.
**Open two independent readers** obtains two descriptors before reading either
one and verifies that both see the complete `hello\n` bytes.

For the process-death leg, read once, reopen the IB fixture, tap **Kill IB
host**, then invoke the content URI again. A successful late read with a higher
provider generation proves the stored result did not depend on the old provider
process.

Force-stop and reboot are deliberately physical questions. If Android revokes
the URI grant, the reader records `permission-denied` and tells the user to
reopen `ib://durable-result-fixture`; do not turn that permission lifetime
into a claim that the result bytes were lost.

The two receipts are independently copyable. CI proves only that the packages
build, use different application UIDs in an emulator install, and preserve the
static storage/descriptor boundary. It does not claim MIRO A1 acceptance.

## Issue #59 fixture

This is a deliberately separate Android fixture for issue #59. It is not the
prepaint viewer and it does not change the prepaint boundary.

The app starts a loopback HTTP server that establishes an HttpOnly authenticated
session, then displays a tiny form in WebView. The form has one ordinary field
that the host may checkpoint and one synthetic password field that the host must
not checkpoint.

The host records only:

- current fixture URL;
- scroll position;
- ordinary-field value;
- the renderer heap canary used only as an acceptance observation.

It does not call `WebView.saveState()`, persist a DOM, persist the synthetic
password field, or claim JavaScript heap continuity.

## Manual phone run

1. Install and launch the debug APK.
2. Wait for `Protected transaction fixture` to appear.
3. Tap **Seed fields** (or type your own ordinary and synthetic-secret values).
4. Tap **Protect**.
5. Tap **Pressure**, allocate memory in the separate `:pressure` process, and
   return to the WebView.
6. Tap **Check live**. The receipt reports authenticated-session survival,
   ordinary-form survival, whether the synthetic secret is still live, renderer
   death events, and whether the same live document/heap canary survived.
7. On API 29+, tap **Kill renderer**. If the WebView provider exposes an
   isolated renderer process, `WebViewRenderProcess.terminate()` requests a
   deterministic renderer death. The `onRenderProcessGone` callback destroys
   the dead WebView, creates a new one, reloads the same URL, and restores only
   the ordinary field and scroll position.
8. Tap **Check recovery**. A successful narrow recovery shows session survival,
   ordinary-field reconstruction, the synthetic secret left blank, a new heap
   canary, and a renderer-death event.

If `getWebViewRenderProcess()` returns null, the receipt says isolated renderer
recovery is unavailable on that provider/device. The fixture does not substitute
`chrome://crash` or pretend host-process death is renderer-process death.

## Update identity

The package name and version code are part of the acceptance boundary. CI signs
the installable debug artifact with the repository's stable public test signer
and verifies repeated `adb install -r` replacement without uninstalling. A
local build without the configured stable signer is deliberately left unsigned
rather than receiving a machine-local Gradle debug identity. The test signer is
not a production or store signing identity.

## Evidence boundary

This first slice can provide four separate observations:

- cookie/session survival;
- form-state survival while the renderer remains live;
- renderer survival or renderer-death observation;
- reconstruction after renderer death.

It does **not** yet prove recovery after the IB host process itself is killed.
The in-memory checkpoint is intentional so renderer-death recovery is not
silently conflated with durable host-process recovery.

`android:usesCleartextTraffic="true"` is present only because the deterministic
fixture server is HTTP on `127.0.0.1`. The harness does not browse arbitrary
cleartext sites.
