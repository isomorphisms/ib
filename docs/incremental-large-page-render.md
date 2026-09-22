# Incremental large-page render

This branch uses a large authenticated web application as a stress case for IB's existing rule: expose the cheapest useful representation first, then continue expensive acquisition, reconstruction, and rendering without making the user stare at a blank surface.

The immediate manual target is Google Cloud Agent Platform Studio. The repository records only the stable page identity and non-secret project/model selectors. Account-selector query state, cookies, credentials, form values, and authentication tokens are not part of the committed fixture.

The experiment is renderer-neutral. `android-webview/` supplies the first diagnostic adapter because it already has renderer-death and protected-form acceptance machinery. Success does not make WebView the browser model or the required final renderer.

## There is no single `loaded` bit

A large application can reach these states at different times:

1. navigation identity is known;
2. DNS, connection, TLS, redirects, and authentication have progressed far enough to identify the response;
3. initial response bytes are available;
4. HTML or another source representation has exposed useful structure;
5. CSS, script, font, image, worker, module, and other dependencies have been discovered;
6. required code has been downloaded, decompressed, parsed, and compiled;
7. application code has executed enough to create or hydrate controls;
8. application RPC/API responses needed for the current task have arrived;
9. style, layout, paint, and compositing have produced useful pixels;
10. forms and controls are safely interactive;
11. optional background work has settled enough for the current task.

The final item may never become globally true. Long polling, telemetry, service workers, periodic refreshes, and deferred modules can keep a modern application active indefinitely. IB therefore records capabilities and milestones rather than waiting for a universal `page complete` state.

A 100-second apparent load can be caused by more than transfer size. Serial dependency discovery, authentication redirects, large JavaScript bundles, decompression, parse/compile CPU, main-thread execution, repeated bootstrap RPCs, layout churn, custom-element or shadow-DOM construction, fonts/images, GPU work, and device memory pressure can all contribute. The useful question is which dependency chain blocks the task the user actually wants to perform.

## Foreground and background split

The foreground path should do only enough to provide a useful surface:

- retain tab/resource/history identity immediately;
- show any cached last-complete or partial representation immediately;
- expose source-backed headings, controls, forms, status values, or structured responses as soon as they are trustworthy;
- attach a live renderer when interaction requires one;
- report what is still incomplete instead of keeping the viewport blank.

Expensive work belongs in a restartable background generation. Each successful step should leave enough evidence on disk that the next process can continue from a stage boundary instead of beginning from zero. Examples include response bodies, immutable resource hashes, dependency manifests, extracted source representations, compiled or renderer-specific disposable caches when safe, and milestone journals.

The browser-owned tab and task remain the identity. The background worker, renderer process, cache directory, and current generation are replaceable machinery.

An exact JavaScript heap is not generally serializable browser state. If an application requires replaying its JavaScript bootstrap after renderer death, IB should say so. Disk persistence can avoid repeating many network, extraction, and indexing steps without pretending arbitrary live application state can be reconstructed exactly.

## Forms are the hard boundary

Incremental replacement is easy for read-only text and much harder once the user edits a form.

IB should keep at least three things distinct:

- source/default control state supplied by the page or extracted representation;
- user-edited ordinary state that may be eligible for an explicit protected-transaction checkpoint;
- sensitive state such as passwords, authentication material, account numbers, or other fields that must not be copied into ordinary cache or diagnostic logs.

Once a control is dirty, a later source/render revision must not overwrite the user's edit merely because more of the page finished loading. Revisions update clean controls and surrounding structure; user-edited values win until the user resets, submits, or explicitly discards them.

A background worker must never submit or replay a form merely to make progress. GET reconstruction, POST replay, and application actions are different operations. Submission remains an explicit user action unless a separately defined task policy says otherwise.

Renderer death is also different from form reconstruction. If a renderer disappears while a form is dirty and IB cannot prove a safe reconstruction, automatic reload is blocked and the loss boundary is reported. Secret values are not silently copied into a generic disk checkpoint to make recovery appear stronger than it is.

## First diagnostic adapter

`IncrementalLargePageActivity` is intentionally narrow. It:

- runs in a separate Android process from the existing protected-transaction fixture;
- loads an HTTPS target with JavaScript and DOM storage enabled;
- records navigation start, coarse WebView progress, first committed visible content, page-finished callbacks, renderer death, and compact Performance API samples;
- records counts and timings, not form values or resource URLs;
- installs an input/change listener that records only `dirty=true`;
- starts a foreground data-sync service in the same `:incremental` process so the host remains active while the task is off-screen;
- requests important renderer priority even when the WebView is hidden;
- keeps the WebView visibly attached in Picture-in-Picture when the device supports it;
- treats launcher and notification re-entry as a request to expose and sample the existing page, never as an implicit reload;
- appends progress to an app-private on-disk journal;
- copies that bounded journal to the clipboard on explicit **Copy receipt** so the
  physical-phone run can be reviewed without ADB or Wireless debugging;
- records the host PID with samples so process continuity is observable inside
  the same receipt;
- requires an explicit user reload after renderer death instead of silently replaying an edited form.

This adapter does **not** yet prove:

- reuse of the user's Chrome/Google authenticated session;
- successful Google sign-in inside WebView;
- durable continuation after Android kills the incremental process;
- unthrottled hidden-page JavaScript or equivalence to a visible Chromium page;
- Picture-in-Picture availability on Android low-RAM devices;
- exact reconstruction of a JavaScript heap;
- safe generic persistence of arbitrary form values;
- that `onPageFinished` means the application is task-ready.

Those are separate observations.

## Manual target

The committed default omits the browser-specific `authuser` selector:

```text
https://console.cloud.google.com/agent-platform/studio/multimodal?project=isomorphismes-youtube-shorts&supportedpurview=project&model=gemini-3.7-flash&region=global
```

A caller can supply another URL, including the exact current browser URL, through the Activity's `url` string extra. The journal intentionally strips query parameters from recorded target identity because arbitrary query strings can contain secrets.

Example explicit launch after installing the branch APK:

```sh
/system/bin/am start \
  -n org.isomorphisms.ib.webview/.IncrementalLargePageActivity \
  --es url 'https://console.cloud.google.com/agent-platform/studio/multimodal?authuser=5&project=isomorphismes-youtube-shorts&supportedpurview=project&model=gemini-3.7-flash&region=global'
```

Tap **Keep loading** as soon as waiting becomes pointless. On a device that
supports Picture-in-Picture, the WebView remains visibly attached in a pinned
window while another app is foreground. Android 12+ also auto-enters this mode
when the user leaves the activity. **Sample** records a fresh compact
timing/count snapshot. **Reload** is explicit because a reload can destroy live
form state.

Android may disable Picture-in-Picture on a low-RAM device. The adapter records
`picture-in-picture-available=false` in that case and does not present ordinary
hidden execution as reliable renderer protection.

The foreground notification is the user-visible lifetime of this experiment.
On Android 13+ the activity requests notification permission before relying on
that re-entry path. Its **Stop** action removes the foreground service.
Expanding the pinned window, opening the **IB Large Page** launcher, or using
the notification must expose the same Activity and WebView. Launcher and
notification re-entry record `launcher-reentry existing-page-preserved` and
must not emit a second `run` or `navigation started` entry.

After the Picture-in-Picture interval and re-entry legs, **Copy receipt** places
the bounded journal on the clipboard for direct paste into the review
conversation. The acceptance flow must not depend on ADB, same-phone Wireless
debugging, `run-as`, or another screen-switch-sensitive extraction mechanism.

## Acceptance questions

The first real run should answer these separately:

1. Does the target reach the expected authenticated application, a Google login boundary, or an embedded-browser refusal?
2. How long to first committed visible content?
3. How long until `document.readyState`, DOMContentLoaded, and load-event milestones?
4. How many resource entries exist at each five-second sample, and does that count continue growing after the task is backgrounded?
5. When do ordinary form/control elements first exist in the document visible to the adapter?
6. Does editing a form set `dirty=true` without logging the value?
7. Do later load milestones leave the edit intact while the renderer remains live?
8. If the renderer dies after an edit, does IB refuse automatic replay and report the boundary?
9. If the Android process is killed, which disk artifacts remain useful and which stages must be recomputed?
10. Which of the observed long stages can be moved into a restartable Grease worker or satisfied by an extracted representation without waiting for the full application?

The first hidden-mode physical run failed. Google Cloud Console reached
`ready=interactive`, grew from 61 to 65 resource entries, and then reported
`renderer-gone didCrash=false priority=2 form-dirty=false` when the activity
returned at 1,097,066 ms (18 minutes 17.066 seconds). Priority 2 is WebView's
highest public renderer priority. The receipt proves that a foreground service
and that requested priority did not preserve a hidden renderer on the MIRO A1;
it does not establish the exact off-screen instant at which Android killed it.

The replacement unattended-load receipt uses Picture-in-Picture for at least
25 minutes in another app, long enough to exceed the failed interval.
It must bind the before/after samples to one journal and report separately:

- whether the host process survived;
- whether the same WebView/JavaScript heap survived;
- whether resource, document, or control milestones advanced;
- whether Picture-in-Picture remained present and pinned-window, launcher, and notification re-entry preserved the page;
- whether Android or the site throttled progress despite process survival.

A green APK build establishes none of those physical-device results.

The point of the experiment is not to declare the page fast after one run. It is to identify the blocking chain, make useful partial state visible, and progressively move non-interactive work out of the user's critical path.
