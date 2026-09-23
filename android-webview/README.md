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

## Issue #86 cross-UID Unix-domain socket experiment

This fixture asks a narrow mechanism question: what actually crosses the
MIRO A1 Android application UID boundary between IB and an ordinary Termux
process? It is not an IB transport choice.

CI builds two pieces from the same source head:

- the IB APK, with a launchable `ib://uds-probe` activity;
- `ib-uds-probe-armv7a`, a tiny ARMv7 Termux executable. It is compiled in CI;
  no compiler or NDK is required on the phone.

The fixed abstract names are:

```text
IB listener:     ib-longview-86
Termux listener: termux-longview-86
```

The Termux probe syntax is:

```sh
./ib-uds-probe-armv7a listen  abstract NAME [hold]
./ib-uds-probe-armv7a connect abstract NAME [hold]
./ib-uds-probe-armv7a listen  path PATH [hold]
./ib-uds-probe-armv7a connect path PATH [hold]
```

Every successful connection reports process PID/UID and Linux `SO_PEERCRED`
peer PID/UID. The Android side reports the same boundary through
`LocalSocket.getPeerCredentials()`.

### Physical MIRO A1 sequence

Launch the Android fixture from Termux without ADB:

```sh
termux-open-url ib://uds-probe
```

**Termux listener → IB connector**

```sh
./ib-uds-probe-armv7a listen abstract termux-longview-86
```

Then tap **IB connect to Termux abstract once**. Both sides must report the
`ping\n` / `pong\n` exchange and each other's UID.

**IB listener → Termux connector**

Tap **IB listen abstract once**, then:

```sh
./ib-uds-probe-armv7a connect abstract ib-longview-86
```

Again retain both receipts. After the listener exits, repeat the connector
command. A failed connect is the name-disappearance observation; do not
replace it with a filesystem check.

For process-death behavior, use `hold` on the Termux side and the corresponding
**+ hold** Android button. Once both sides report the completed exchange and
`waiting-for-peer-close`, kill one endpoint. The survivor must record EOF or
an explicit socket error. **Kill IB host** exists for the IB-dies-first leg.

For pathname comparison, first prove the creator can bind its own private path.
The Android fixture exposes its exact app-private pathname on screen. From
Termux, try:

```sh
./ib-uds-probe-armv7a connect path /data/user/0/org.isomorphisms.ib.webview/files/ib-longview-86.sock
```

Retain the exact errno. In the other direction, create a Termux-private
listener:

```sh
./ib-uds-probe-armv7a listen path "$TMPDIR/ib-longview-86.sock"
```

Paste that pathname into the Android fixture if it differs from the prefilled
Termux path, then tap **IB connect to Termux-private pathname**. Retain the
exact Android exception or successful exchange.

The expected filesystem isolation is only a hypothesis until the phone
produces the receipt. Likewise, success or failure of abstract sockets is
evidence about this Android/SELinux boundary, not a proposal that Grease or IB
should expose Unix socket names.

Hosted CI checks the probe logic on Linux, compiles the ARMv7 executable,
builds/signs the APK, and verifies replacement installation. It does not claim
cross-UID MIRO A1 acceptance.
