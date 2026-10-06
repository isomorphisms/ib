# Protected long-view tasks

IB treats a slow authenticated workflow as a browser-owned task with a renderer
attached.  The task, tab, and current navigation identities survive Activity,
renderer, and host-process loss.  A `WebView` and its renderer process do not.

The first integrated Android path is `LongViewActivity`.  It is the application
launcher, not a standalone fixture.  The issue #59 loopback fixture remains in
`WebViewActivity` as test equipment.

## Ownership and durable record

`IB.LongViewTask` owns the semantic states and transitions.  The Android
`DurableTaskRecord` and `DurableTaskStore` are the platform representation and
app-private filesystem adapter for the same `ib-long-view-task-v1` record.

The ordinary durable record contains:

- task, tab, and current navigation identities;
- a neutral origin URL;
- whether path, query, or fragment material had to be removed;
- the last distinguished continuity case;
- separate session, form, JavaScript-heap, and reconstruction results;
- renderer and host-process identities and generations;
- optional provider-neutral authorization-adapter metadata and references to
  protected protocol state.

It does not contain passwords, cookies, page field values, authorization codes,
access or refresh tokens, raw OAuth state or nonce values, or PKCE verifiers.
The generic authorization record can name references to protected state; it
does not make the ordinary task file secret storage.

The store uses the existing conceptual paths from `docs/storage-model.md`:

```text
state/tasks/<task-id>/task.txt
state/tabs/<tab-id>/tab.txt
state/tabs/<tab-id>/history.log
```

Task and tab manifests are replaced by a synced same-directory atomic rename.
Navigation history is append-only and synced.  Ordinary unprotected tabs remain
renderer-lifetime state in this slice; the store refuses to create a durable
task file for them.

## Navigation and authentication

The live renderer may initially receive the complete user-entered HTTP(S) URL.
The durable record never does.  It stores only scheme, host, and port, with `/`
as the restart path.  User-info is rejected; path, query, and fragment are
removed.  This is deliberately conservative: after restart, a Google Cloud
page may require the console path, project, or account choice to be repeated.
IB reports `path-query-or-fragment-redacted` rather than hiding that loss or
copying a path-embedded token or one-time code into a restart URL.

WebView's normal app profile remains the platform owner of cookies and related
site-session material across renderer replacement and host restart.  IB does
not copy those cookies into its task files or receipts.  Profile availability
is not proof that an authenticated session is usable.  Every reconstruction
starts at `session=not-proven`; the user can record `Session works`, or mark the
remote step as needing repetition.

PR #79, “Receive local Drive authorization handoffs,” remains a separate native
authorization-adapter experiment.  Such an adapter may populate the generic
authorization continuation metadata and protected-state references.  It does
not own task, tab, navigation, renderer, or host-process identity, and its
one-time authorization code must still stay out of ordinary IB state.

## Distinct continuity cases

The record and receipt do not collapse recovery into one status:

| Case | Evidence | Durable transition |
| --- | --- | --- |
| Live renderer return | same Activity/WebView and same in-memory heap canary | `live-renderer-survival` |
| Activity recreation | same host-process identity, new Activity attachment | `activity-recreation` |
| Renderer death | `onRenderProcessGone`, old renderer detached, replacement attached | `renderer-replacement` |
| Host-process restart | a different per-process identity discovers the same task file | `host-process-restart` |
| Remote state cannot resume | user or adapter reports the remaining site step | `remote-site-blocked` plus `incomplete` |

The JavaScript heap canary is never written to disk.  It can prove a live heap
only while the observing host remains alive.  A changed in-memory canary proves
a recreated heap after renderer replacement.  After host restart, the old
canary is unavailable, so the receipt cannot claim either equality or change.

## Forms

The integrated path observes only whether page input became dirty.  It never
reads or persists page field values.  After renderer loss, a dirty generic form
therefore becomes `repeat-required`.  The Idriç model has separate states for a
future explicitly permitted ordinary-form checkpoint, but this change does not
pretend that arbitrary third-party form fields have been classified safely.

That leaves the ordinary permitted-form portion of issue #71 incomplete for the
real path.  Passwords and other arbitrary sensitive fields are excluded now,
rather than being captured by a heuristic in order to claim fuller recovery.

## Why renderer survival is only an optimization

The physical MIRO A1 result from PR #74, “Keep incremental WebView loads alive
while using another app,” remains the relevant failure evidence.  Google Cloud
Console reached an interactive state, but returning at 1,097,066 ms produced a
renderer-gone event.  The foreground service kept the host important and the
renderer already had WebView's highest public priority.  Those mechanisms did
not make the hidden renderer durable.

`LongViewActivity` still requests important renderer priority because survival
is useful when Android grants it.  It does not require Picture-in-Picture, a
foreground service, foreground residence, or a kept-awake screen.  Correctness
comes from rediscovering the durable task and attaching a replacement renderer.

## Deterministic evidence

The Idriç smoke path exercises both required transitions:

- protected task with live renderer → renderer loss → same identities with a
  replacement renderer;
- protected task → different host process → same durable task discovered with
  advanced host and Activity generations.

Android unit tests additionally cover record round trips, atomic discovery,
Activity-versus-host distinction, lack of false authenticated or heap
continuity, explicit incomplete remote state, query/code exclusion,
provider-neutral authorization references, unsafe path refusal, and refusal to
persist ordinary tabs.

These are semantic and host-side Android build tests.  They are not MIRO A1
physical acceptance.

## MIRO A1 acceptance still required

The exact CI-built APK must be replacement-installed without uninstalling the
existing package.  Through the normal **IB Long View** launcher:

1. start a slow authenticated/admin page and note the task, navigation, run,
   host, and renderer identities;
2. leave IB longer than the prior failure interval and return;
3. record whether the receipt says live survival or reconstruction;
4. use **Kill renderer** and verify the same task/tab/navigation identities with
   a replacement renderer and no heap-continuity claim;
5. use **Kill IB host**, relaunch, and verify the same durable task is discovered
   with an advanced host generation;
6. complete an app-switch or device-verification detour;
7. use **Session works** only if the authenticated state is genuinely usable,
   otherwise use **Needs repeat**;
8. copy and inspect the receipt for any unexpected secret before sharing it.

The receipt identifies source head, APK version, Android and WebView versions,
run/task/tab/navigation identities, host PID and generations, renderer identity
and generation, elapsed time, session result, reconstruction result, URL
redaction status, and any user step that must be repeated.
