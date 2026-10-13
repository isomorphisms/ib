# STAR IB-LONGVIEW-RECOVERY-01: C67 recovery investigation

Status: **BLOCKED; source diagnosis established, product repair and physical acceptance NOT_RUN.**
Investigation date: 2026-10-12 America/Detroit (2026-10-13 UTC).

## Answer

The reported APK does not implement the Shizuku-backed end-to-end recovery
being assumed. At its reported source revision,
`d05807236c2a8891195d057405a0bd4050313a2b`, LongViewActivity uses an app-private
Java task store. No Shizuku/Binder worker is invoked by this Android application.
It preserves task/tab/navigation identifiers and an **origin-only restart URL**,
then creates a new WebView and loads that origin. It does not reconstruct the
original page address, browser history, scroll position, DOM or arbitrary form
state. Successful discovery therefore cannot establish the requested browsing
continuity. This is an incomplete implementation, not evidence that Android
prevented a complete implemented recovery mechanism from working.

Separately, `page-not-confirmed` is not a load-failure diagnostic. It is assigned
at replacement attachment, retained even after `onPageCommitVisible`, and changed
to `confirmed` by the human's **Session works** button. The receipt conflates page
confirmation with authentication confirmation. `available-to-replacement-renderer=true`
is a literal string printed unconditionally; it is not a cookie, login or profile
restoration test.

The cause and exact wall-clock time of the preceding host exit remain UNKNOWN.
The supplied 33,326 ms off-screen interval is within the later run, with PID 4701
on both sides. It cannot be used to date or explain the earlier host restart.

## Inspected identities and ownership

| Work | Exact source | Relationship to the observed APK |
| --- | --- | --- |
| C67 baseline | `d05807236c2a8891195d057405a0bd4050313a2b` | Reported in user receipts; local checkout inspected; installed bytes not independently pulled from C67 |
| Retained Longview/Binder/E2/E3/E4 branch | `3524599f53e4028957d9d9580079a36760dd9b3f` | Separate unmerged branch, not an ancestor of the baseline |
| Chosen Idriç Android lifecycle branch | `41d4cae9c15e42d3b0b4ed4ed957ba86a7ac396a` | Diagnostic source-to-DEX/ART work; not an installable recovered browser |
| Flexible Pipes main | `def2a66fe61a2b23440f98031f1defa738a0d1ee` | No Longview recovery pipeline |
| Flexible Pipes registered Android candidate | `86c2d16f634eb73e0e338f2531827d1ea040b037` | Client explicitly inactive; Android operations unqualified; documented consumer is Crystal/A1 |
| ai-ci | `3d174000649f9f828c29e6be981cfd0e60646014` | Shared requirements and independent merge-state verifier |

Relevant existing work:

- [isomorphisms/IB PR #133, “Fix C67 Long View startup on Android-managed directory symlinks”](https://github.com/isomorphisms/ib/pull/133)
- [isomorphisms/IB PR #114, “Consolidate Longview through the Binder boundary with retained acceptance work”](https://github.com/isomorphisms/ib/pull/114)
- [isomorphisms/IB PR #134, “Move Long View lifecycle dispatch into Idriç and exercise its direct-DEX path”](https://github.com/isomorphisms/ib/pull/134)
- [Flexible Pipes IB-C01 control record](https://github.com/isomorphisms/flexible-pipes/issues/76)

The explicit current boundary in `docs/idric-longview-android-repair.md` on the
chosen branch rejects presenting the Java harness as the Idriç implementation.
This investigation does not revive that rejected implementation, build another
Java APK, or create a competing browser/task/store. Existing workers, admission,
Cauldron source storage, publication and lifecycle owners remain the integration
path. The task model itself is byte-identical between the baseline and the
inspected Idriç lifecycle branch; compiling it does not repair missing page state.

## Exact baseline source trace

All line numbers in this section refer to the baseline above. Java paths share
`android-webview/app/src/main/java/org/isomorphisms/ib/webview/`.

| Stage | Source location | What actually happens |
| --- | --- | --- |
| Live navigation | `LongViewActivity.java:402–423` | Address-bar/gesture navigation gets an ID; observed URLs replace the stored neutral navigation |
| Information loss | `DurableNavigation.java:39–83` | Every non-root path, query and fragment is discarded; resulting URL is scheme/host/port plus `/` |
| Disk publication | `DurableTaskStore.java:73–108,318–347` | Task and tab files each use file force plus atomic rename; journal is appended and forced |
| Cold launch | `LongViewActivity.java:69–107` | Discovers latest task; advances host/activity state; takes only `task.navigation.neutral_url` |
| Discovery | `DurableTaskStore.java:110–175` | Reads and validates task files; does not read the navigation journal into a history stack |
| Replacement | `LongViewActivity.java:132–137,266–384` | Allocates a new WebView and calls loadUrl with the neutral URL |
| Renderer death | `LongViewActivity.java:351–369,426–443` | Destroys dead WebView, attaches replacement and loads neutral URL again |
| Visible commit | `LongViewActivity.java:321–336`; `DurableTaskRecord.java:335–349` | Records paint, but replacement reconstruction remains `PAGE_NOT_CONFIRMED` |
| Authentication | `LongViewActivity.java:123–129,283–285,463–466`; `DurableTaskRecord.java:371–379` | Reuses default WebView profile configuration; only human confirmation claims authentication and reconstruction |
| Interaction sample | `LongViewActivity.java:490–531` | Reads heap canary and counts resources/forms/controls; never verifies original page identity or performs a useful action |

Specific defects and limits:

1. **Origin-only reconstruction loses the destination.** Two ordinary pages such
   as `https://example.test/first` and `https://example.test/second` have the same
   restart target. A perfect disk write cannot recover the discarded distinction.
2. **Redaction provenance can be overwritten.** Loading the saved root calls
   `onPageStarted` → `observe_neutral_navigation` → `observe_current_navigation`.
   The freshly parsed root is `exact-neutral`; it replaces the prior redacted
   navigation under the same navigation ID. This is source-derived behavior,
   not a claim that the supplied truncated receipt contains that later event.
3. **No recovered history.** `history.log` is write-only in this application;
   no `saveState`, `restoreState`, back/forward reconstruction or browser back
   handler exists in LongViewActivity. A new renderer starts a new WebView list.
4. **Activity recreation also reloads.** The saved Bundle is not used to restore
   browsing state. A same-process recreation still destroys/replaces the view
   and uses the neutral target. No process kill is necessary for that loss.
5. **Receipt labels overstate what was measured.** Profile availability is
   asserted, not observed. `onNewIntent` prints `case=live-renderer-survival`
   before its asynchronous sample. Renderer generation counts attachments,
   not verified renderer-process deaths. Initial attachment labels `live-page`
   before a visible commit. Authentication confirmation marks reconstruction
   confirmed without comparing the recovered page to the original destination.
6. **Storage errors do not stop navigation.** `persist_or_block` catches an
   IOException, records it, and returns to its caller. Its name does not mean
   publication failure prevented the subsequent load. No such error was supplied
   in these observations, so this is not the established cause of this incident.
7. **Synchronous I/O and unguarded callbacks are additional risks, not diagnosed
   causes.** Task and receipt syncs run in UI callbacks. Callback results are not
   bound to captured task/navigation/renderer generations in this Java path.
   The Idriç lifecycle branch has explicit observation-identity checks. No ANR,
   stale-callback race or latency measurement was captured in this investigation.

## Receipt timeline and comparison

`reported-observations.tsv` transcribes only the provided fields. It is explicitly
a partial user report, **not a captured device receipt**. Attempts to retrieve the
full original conversations did not recover either complete receipt.

| Evidence | Established | Not established |
| --- | --- | --- |
| Earlier run `run-794c2e8c-165c-4a0b-91b0-10cd4bd27084`, `valid-records=0`, PID 28677 | No record was found at discovery. Source then creates a task and still labels its first task event `point=discovered` | That record was recovered, any later process death, its cause, or usable authentication |
| Later run `run-acbc62cc-bbc1-4ada-9569-e0fbfeab2f68`, `valid-records=1`, PID 4701, host generation 2 | A valid task record was reopened under a different per-process identity before the discovery receipt | The exact exit time/reason, low-memory kill, crash, voluntary kill or reboot |
| `continuity=host-process-restart`, `reconstruction=pending` at discovery | `opened_by_host` classified the new host before renderer attachment | A new restart during a later off-screen interval |
| Replacement attachment, renderer generation 3 | Third persisted attachment count | Three renderer kills, isolated renderer PID, or same-page rendering |
| `page-not-confirmed`, `session=not-proven` | No positive confirmation at that recorded point | A blank page, failed load, invalid cookie, or successful recovery |
| 33,326 ms off-screen and return with the same PID 4701 in the later run | That foreground/background pair belongs to the already restarted host | That Android killed the host during those 33 seconds |

The current receipt elapsed clock starts in Activity.onCreate, not at the prior
process exit. The baseline records neither boot/process-start identity nor Android
historical exit reason. `HostProcessIdentity.VALUE` is a static random UUID, not
an Android death-cause report. `low-ram=false` is a device classification, not
proof that the device experienced no memory pressure.

## Competing mechanisms

| Mechanism | Classification | Evidence / next discriminator |
| --- | --- | --- |
| Required navigation information absent | **BROKEN, source demonstrated** | Non-injective origin reduction and root load above |
| History/scroll/form reconstruction absent | **BROKEN / explicitly excluded in baseline** | No consumers/checkpoint path; history writes do not restore a stack |
| Inadequate page confirmation | **BROKEN instrumentation** | Commit and non-commit both remain page-not-confirmed |
| Android background memory kill | **UNPROVEN** | Need old process identity and Android exit reason; no LMK evidence supplied |
| Crash, ANR, explicit Kill IB host, external force-stop | **UNPROVEN** | Need prior receipt tail and independently scoped exit evidence |
| Activity recreation | **POSSIBLE independent loss path** | Supported by source; not established as this reported restart |
| Renderer termination | **UNPROVEN for this interval** | Need onRenderProcessGone plus renderer/host identities; attachment count is insufficient |
| Task disk flush/reopen failure | **Not demonstrated for the recovered record** | One valid task reopened; this does not prove complete multi-file/power-loss atomicity |
| Shizuku/Binder permission or worker death | **Not on this APK's recovery path** | Baseline Android source has no corresponding calls or integration |
| Cookie/sessionStorage loss or server invalidation | **UNPROVEN** | Default profile reuse and control counts are not authenticated endpoint results |

## What Shizuku contributes

The separate retained branch implements Binder-neutral operations lowered through
Shizuku/rish, capability checks, worker launch/attachment identity, bounded work
and admitted immutable publication. `lib/android_shizuku_worker_provider.grease`
binds launch/boot/UID/PID/start time/attempt; `lib/result_publication.grease` and
`lib/durable_object_store.grease` bind completed results to storage.
None is a WebView heap/session snapshot mechanism. The inspected baseline's
`lib/` contains only scientific/prefetch helpers; its Android manifest and source
have no Shizuku or Binder integration.

The retained branch's old Android heavy-page tests verify a small
`longview-heavy-v1` result marker after renderer loss/force-stop. Its newer
`docs/longview-source-backed-result.md` correctly limits source-byte recovery and
host text projection to backend evidence and explicitly excludes live authenticated
Android continuation. Both are useful, but neither proves the installed browser
resumes the user's page. The host-loss test's force-stop is not an ordinary
Android background-kill test.

## Platform boundaries

Android's [WebView state API](https://developer.android.com/reference/android/webkit/WebView#saveState(android.os.Bundle))
can return a saved/restored back-forward list; it does not save/restore display
data. This is a potential adapter aid, not an entire browsing-state model or a
guarantee of live heap, form or service continuity. Those remain separate claims.

[CookieManager.flush](https://developer.android.com/reference/android/webkit/CookieManager#flush())
persists accessible cookies. The baseline sets cookie acceptance but does not
call flush. That omission does **not** prove cookies were lost: WebView may have
persisted them, and the server may accept, expire or invalidate them independently.
Do not log cookie values to investigate this.

[ActivityManager.getHistoricalProcessExitReasons](https://developer.android.com/reference/android/app/ActivityManager#getHistoricalProcessExitReasons(java.lang.String,int,int))
provides recent exit records from API 30, including the app's own UID without
another app's DUMP permission. Its history is bounded and low-memory reporting
support must be checked. Bind numeric reason/time/status to the old process;
do not copy arbitrary descriptions, traces or sensitive URLs into public logs.

[Shizuku's own documentation](https://github.com/RikkaApps/Shizuku#attention)
states that ADB permissions are limited and version dependent. Its presence
cannot establish immunity from Android termination or restoration of state the
application never checkpointed.

## Repair boundary and acceptance

A one-line change retaining all URLs in ordinary task records would reverse the
existing privacy policy and still omit history, view state and authentication
evidence. Relabeling page-not-confirmed or keeping the task ID would repair none
of those omissions. No speculative production patch was made.

The smallest complete repair must extend the **existing Idriç-owned navigation
and view state**, connect it to existing admitted durable publication/reopen,
and consume it through the chosen Android renderer adapter. It must retain a
privacy-classified resumable destination (or an explicit protected reference),
the history cursor/entries where supported, and permitted view checkpoints.
Keep actual destination and recovery observation separate so a root fallback
cannot overwrite the intended page. A blocked private continuation must remain
explicit; never silently turn it into home-page success. The WebView profile
continues to own cookies. Authentication, same-page rendering and an actual
interactive action require separate identity-bound observations.

The existing Idriç branch's own scope states that platform callbacks, arbitrary
restored task inputs, disk reopen, authenticated navigation, Shizuku and APK
integration are unfinished. Finishing that integration is a real prerequisite,
not a compiler success already equivalent to a repaired browser.

`reproduction.md` specifies the controlled test and required observations;
`trials.tsv` preserves all required C67 and A1 trials as **NOT_RUN**. No adb
executable or C67/A1 connection is exposed in this environment. No APK was built,
installed, launched, killed, or accepted. No full physical receipt was captured.

The inspected Flexible Pipes client is inactive and no qualified C67/IB operation
was found. The control record is an authoring/investigation record, not dispatch.
Independent ai-ci must reject promotion while physical and integration claims
remain NOT_VERIFIED. This report/PR does not bypass that boundary or authorize
a merge. The job is incomplete until the actual repaired browser passes the
required device trials.
