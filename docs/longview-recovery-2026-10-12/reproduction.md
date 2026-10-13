# Controlled recovery reproduction: required, not executed

Status: **BLOCKED**. This is a reproduction protocol, not an executable device
harness or an acceptance receipt. No physical connection was available during
the source investigation. Reuse the Flexible Pipes Android diagnostic capture,
Kitchen capture, Cat Food exact-artifact/profile and ai-ci evidence owners once
qualified for IB on C67. Do not fabricate a device channel or substitute A1.

## Controlled session

Use a dedicated test origin and synthetic account, hosted outside the Android
browser process. It must survive a killed browser. Do not start with real Google
credentials. The test server must offer:

- distinguishable `/first` and `/second` pages on the same origin;
- an authenticated endpoint returning a boolean test outcome without disclosing
  its cookie or token;
- a synthetic persisted-cookie case and a separate session-cookie case;
- an ordinary text field explicitly permitted for restoration, an excluded
  synthetic secret field, and a known scroll target;
- an interaction that increments a server-observed counter and changes the
  visible page, allowing the observer to verify actual usability;
- a server-side request history using test case IDs, never authentication values.

Use the installed application entry point and its actual store/renderer adapter.
The separate WebViewActivity fixture is not a substitute. Prepare the controlled
origin through the actual navigation path. At the baseline, cold onCreate ignores
a supplied URL extra when it discovers an existing task; this must not lead a
harness to test the wrong page or delete user data to force a fresh start.

Before each trial bind the exact APK digest, source, package, signer/version,
device model/ABI/Android/WebView, device instance, boot identity, process UUID,
PID/start time, host/activity/renderer generations, and task/tab/navigation ID.
Record test page IDs and booleans rather than raw private URLs or state values.

## Cases

1. Navigate first → second, authenticate, set permitted field and scroll target,
   and perform a visible interaction. Save the intended destination and cursor.
2. Ordinary switching: leave through Android's normal app switch and return ten
   times. Observe each before/after pair, including whether the host or renderer
   changed. Switching itself does not establish a kill.
3. Extended background: one separately timed interval, recording actual duration.
   Do not infer termination from duration or from a low-RAM classification.
4. Background host termination: background IB, use the qualified Android
   background-kill operation, and independently verify the **old** PID/start-time
   instance is gone before relaunch. If the command is ignored or the host is
   still alive, record failed preparation, not a recovery pass. Run five times.
5. Renderer termination: invoke the actual renderer's termination API, require
   the matching renderer-gone observation and unchanged host identity, then
   check replacement rendering and interaction.
6. Cold launch: discover an existing committed task; verify disk-derived target,
   selected history position, same page and action. Do not pass only because IDs
   survived or a root page rendered.
7. Activity recreation: independently demonstrate unchanged process UUID and a
   new Activity; check the same destination and state. Keep this separate from
   host death.
8. Repeat the matrix on A1 after C67 is understood, with a separately verified
   artifact and device profile. C67 evidence cannot fill A1 rows.

The app's **Kill IB host** button is a deliberate self-kill test. `am force-stop`
is a separate stopped-package test. Neither should be labeled a spontaneous
low-memory death or silently substituted for a background-kill case. A deliberate
background kill tests reconstruction but does not prove Android memory pressure
caused the historical incident.

## Oracle and negative controls

Require all of: intended page equality, visible content, successful interaction,
navigation identity/order, recoverable Back/Forward behavior, and valid test
authentication when the service/profile supports it. Observe the old process
death separately from new-host startup. Preserve every failed trial and every
failed prerequisite. Missing evidence is NOT_VERIFIED, never PASS.

The independent acceptance must reject:

- same task ID with `/` instead of `/second`;
- rendered login/home page instead of the authenticated intended page;
- first-paint or document.readyState without successful interaction;
- stale callbacks from the previous navigation or renderer generation;
- an ignored kill, a reused PID without process-start comparison, or force-stop
  mislabeled as ordinary background death;
- a Session works click used as proof of history or permitted-field restoration;
- cookie/profile availability used as proof of service acceptance;
- a host/emulator/fixture receipt offered for physical C67 execution;
- discarded failures or fewer than ten switch and five verified-death trials.

Real-site reproduction follows only after the synthetic session. Keep credentials,
tokens, cookies, secret fields, raw private URLs and private content out of logs.
Public receipts should contain opaque test IDs, identity bindings and outcomes.
No new persistent store or whole-heap snapshot promise is needed to test this.
