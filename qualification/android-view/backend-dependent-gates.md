# Android backend gates consuming this qualification

Source/run refresh on 2026-10-08 for SUN IB-B04, B07, B05 and B08. This
bounded receipt changes no provider, Activity, workflow, private configuration
or device state. The full job contracts remain in STAR IB-R1's recovery owner.

## B04 — reliable PFD session bounds

Live `longview-renderer-loss-acceptance` remains
`843c7bcf5336c685c4a96a4b4bc28b96fbc7a5dc`. Exact inspected source:

| Source | Git blob | Observation |
| --- | --- | --- |
| `android-webview/app/src/main/java/org/isomorphisms/ib/webview/LiveChannelProvider.java` | `8f63cfd3bcbb47902fc1f91d67aa12313b1660a9` | `openFile` creates a reliable socket pair and one daemon thread for every accepted open. No active-session admission/cap or deadline. |
| `android-webview/result-reader/src/main/java/org/isomorphisms/ib/resultreader/LiveChannelActivity.java` | `61c0b367b88a63ea519d4455e40d57986f87d6fb` | Descriptor is local to `exchange`; `onDestroy` calls `executor.shutdownNow` but does not own an explicit descriptor-close cancellation path. |

Provider `read_exact` waits for the five ping bytes with blocking reads;
after pong, a separate `input.read()` waits for caller EOF. Both can remain
pending for a silent/partial peer. The request buffer is already fixed-size;
that fact does not bound session duration or concurrent descriptor/thread
residency. There is no source-defined waitable cancel/deadline result.
Interrupting an executor has not been qualified here as closing an Android
PFD blocked read. Reader queued exchanges also have no explicit bounded
admission contract.

The manifest marks the provider `exported=false`, enables URI grants and
restricts their path prefix to `/channel/`. Calling UID/package are recorded;
the URI and descriptor modes are checked. This source has an authorization
boundary; no missing-grant bypass was inferred. The retained workflow tests
ordinary ping/pong and both process-death/EOF directions, but no missing/revoked
grant, partial-frame stall, session exhaustion or bounded reconnect/resource
baseline assertions. Those cases must execute through the real Android grant
boundary; a host mock would not establish them.

**First blocker: BLOCKED_QUALIFIED_ANDROID_ADAPTER_STAGE.** The maintained
workflow compiles the Java ContentProvider/Activity with JDK 17 and Gradle 8.13.
That inherited stage has no ICK/NDK producer declaration. New Java application
code cannot be built through it as a fallback. The available installed Idriç
compiler lacks `dex`; the separately checked android-NDK driver exports
Int32/Text but has no generic ContentProvider subclass, framework callback or
object ABI. NDK alone supplies no checked lowering of these existing managed
objects. No compiled qualified substitute adapter was observed.

An owned source-to-framework or explicit qualified narrow adapter stage is
required before implementing this slice. Afterward, retain the same PFD and
URI contract, enforce session admission and waitable cancellation/deadlines,
and test successful, malformed, unauthorized, revoked, stalled and exhausted
peers plus EOF/reconnect/resource recovery. No speculative session patch was
made while the adapter stage is blocked.

## B07 — preserved latency correction

[PR #91, Account for page-load latency without UI-thread receipt fsync](https://github.com/isomorphisms/ib/pull/91)
remains closed, draft and unmerged; its preserved branch
`perf/page-load-accounting` is still
`f3533211bc0eca556b3ad58a3ca64ecaaea1d618`. No replacement owner or reopen
was created. Longview has not absorbed this independent correction:

| Source | Git blob | Observation |
| --- | --- | --- |
| Longview `LongViewActivity.java` | `bc263268109d00ef17f319689d559df4174ef763` | Ordinary `record` appends and calls `output.getFD().sync()` on the caller/UI path. |
| Preserved latency `LongViewActivity.java` | `08d0e929cfb0fadaa0ba9d0af1274a993bde3c02` | Single receipt writer queues ordinary writes; navigation/checkpoint timing source is present; deliberate host kill queues a sync after prior writes. |
| Preserved `.github/workflows/android-webview.yml` | `24dad55ed356b779a39f3f360b3daa3f8280d0f6` | Real-page gate retains Android API 34, 1 GiB RAM, 192 MiB heap, one core and 45 attempts. |

Fresh inspection of the existing exact-head hosted run
[37468854379](https://github.com/isomorphisms/ib/actions/runs/37468854379)
finds build/lint/signer job `112286743843` and replacement-install job
`112287232415` successful. These are historical Java/Gradle and emulator
receipts, not build-policy qualification or physical MIRO acceptance.
Low-memory real-page job `112287232621` failed on 2026-10-06. Its actual log
records an emulator boot of **789960 ms**, followed by:
`adb: failed to install downloaded-apk/app-debug.apk: cmd: Failure calling service package: Broken pipe (32)`.
The action's `/usr/bin/sh` exited 1. Evidence upload found no files.
The repaired harness ran as one fail-fast command, but installation failed
before Activity launch, receipt polling, navigation or page timing. This is
the current preserved-head infrastructure failure, distinct from the older
`Can't find service: package` failure in the branch's retirement note.

The source measures recovery/checkpoint elapsed time, page-start/commit/finish
and Navigation Timing DNS/connect/TLS/first-byte/response/DOM/load fields.
It does not establish physical values, separate actual construction cost or
a completed visible-paint timing trace merely by naming those events.
Target receipts use `safe_target_identity`; query/fragment exclusion and
producer failure must remain in any integration's acceptance.

There is one concrete failure-propagation gap to test once its stage is
qualified: `sync_receipt()` catches `IOException`, posts status and returns;
`kill_host_process()` then calls `Process.killProcess` unconditionally.
`persist_or_block` likewise records a checkpoint failure rather than
propagating success to the kill path. The source contains a queued ordering
barrier, but it does not refuse deliberate host death when that barrier or
checkpoint fails. This diagnosis is source evidence, not a simulated failure
receipt or implemented repair.

**First execution blocker remains BLOCKED_QUALIFIED_ANDROID_ADAPTER_STAGE.**
Do not repeat the inherited Gradle build to manufacture new acceptance.
Once qualified, resume only the retained delta, serialize LongView edits with
S03/D02, retain durable checkpoints and require successful queue/sync refusal
behavior before deliberate host death. A wholesale branch merge would also
remove later Longview features and provider workflow cases; it is not the
required delta reconciliation. Keep the emulator's memory/attempt limits and
the separate physical timing gate unchanged.

## External gates unchanged

**B05: BLOCKED_EXTERNAL.** No verified connected MIRO A1 or admitted exact
installed provider/reader artifacts were supplied in this execution. Real
app/reader/Termux identity, independent read-only descriptors, late read after
host death, live peer-death/denial/reconnect and grant-loss behavior still need
physical receipts. Only the separate Android shim currently receives the PFD;
no Termux POSIX descriptor transfer is claimed. Force-stop/reboot are not
authorized by this preparation. No private bytes were moved to shared storage.

**B08: BLOCKED_EXTERNAL.** Live
[PR #79, Receive local Drive authorization handoffs](https://github.com/isomorphisms/ib/pull/79)
is closed, draft and unmerged at
`0b0a7bed15cb668a00ba11147618e3d353d58f1b`. The retained
`docs/drive-authorization-c67.md` confirms MIRO C67 scope, package
`org.isomorphisms.ib.webview`, default read-only scopes and separate private
signer/Google registration/physical consent gates. Its historical artifact
10707349978 predates private stable signing and is not an admitted update.
No new parser defect was established and no generic OAuth rewrite was made.
Approved private certificate, installed-package match, Android/Web client and
consent registration, physical Play Services/account state, code return,
durable refresh state and one authorized Drive request remain required.
No certificate, client, consent, account or cloud-console state was changed.
