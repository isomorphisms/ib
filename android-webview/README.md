# IB WebView protected-transaction acceptance harness

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

## Incremental large-page experiment

The branch `incremental-large-page-render` also registers
`IncrementalLargePageActivity` in a separate `:incremental` process. This is a
diagnostic adapter for `docs/incremental-large-page-render.md`; it does not
replace the protected-transaction fixture above.

The APK exposes a second launcher entry named **IB Large Page**. Tap that entry
to start the exact Google Cloud Studio target with `authuser=5`. This avoids
relying on `/system/bin/am`, which some Android/Unisoc builds reject when it is
invoked from an application UID such as Termux.

The adapter records coarse load progress, first committed visible content,
`onPageFinished`, compact Performance API timing/count samples, and renderer
death to an app-private disk journal. **Background** moves the task out of the
foreground without deliberately pausing WebView timers. A foreground data-sync
service in the same `:incremental` process keeps the host process active and
shows an ongoing notification while that page remains open. The WebView also
requests important renderer priority while visible or hidden.

Opening **IB Large Page** again brings the existing incremental Activity to the
front. It records `launcher-reentry existing-page-preserved` and samples the
existing document; it does not create a second WebView or call `loadUrl` again.
The notification follows the same re-entry path. Its **Stop** action removes the
extra process protection without claiming that Android or WebView preserved the
page.

The foreground service reduces host- and renderer-process eviction. It does not
make a hidden document visible to Chromium, disable hidden-page JavaScript
throttling, or prove that Google Cloud Console continues task-relevant work.
Those remain physical-device observations.

For forms it records only whether an input/change event made the page dirty; it
does not read the value. Renderer death never automatically reloads this real
page. **Reload** is an explicit user action because replaying or discarding an
in-progress form is not a renderer-recovery implementation detail.

The unattended-load acceptance run is ten minutes in another app. Returning
through both Android Recents and the **IB Large Page** icon must retain the same
run and JavaScript heap, show no second navigation start, and show whether the
five-second resource/control samples advanced while IB was off-screen. Build,
installation, and notification presence do not substitute for that run.
