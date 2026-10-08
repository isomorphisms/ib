# IB-B03/B06/B09 execution receipt

Base: B01 published `5205209f189cb3813d18ec251a273d59f369e50e`, preserving
Longview `843c7bcf5336c685c4a96a4b4bc28b96fbc7a5dc` and merged E2/E3 work.
This child does not restack or replace closed Longview PR #114.

## B06 bounded trusted-operation capture

`ib_shizuku_capture SECONDS STDOUT_BYTES STDERR_BYTES COMMAND` captures both
streams before exposing output. A native Grease helper runs within timeout's
process group; FIFO readers store at most each budget plus one overflow byte.
The helper waits for stream closure, so a child retaining descriptors is also
subject to the elapsed limit. A one-second kill-after allowance covers a local
TERM-resistant helper. No new native helper/compiler was introduced.

Within budgets the exact producer status and stderr survive. Nonzero status
withholds stdout. Overflow returns 65, missing timeout/Grease/helper returns69,
bad budgets return2, timeout returns124 (137 for kill-after escalation). Local
timeout/overflow reports remote lifetime unknown. Positive canonical decimal
limits have ceilings:60s,1MiB stdout,64KiB stderr. Ordinary observations use5s;
UID256B, identity8KiB, process/capability snapshots256KiB, process identity512B,
package PID4KiB, with8KiB stderr. Raw `ib_shizuku_run` remains a trusted debug
edge and is no longer used for production observations.

Actual native Grease capture acceptance: PASS. Success0; plausible UID text
then denied exit37 gives no stdout and exact denial; stdout/stderr overflow65;
hang, partial hang and orphan-held descriptors124; missing timeout69; later
success0. Both direct capture files were exactly129 bytes for128-byte limits.
Local timeout fixtures had no delayed survival write after3s. Replacing both
bounded readers with `cat` fails the real suite with exit1 at the129-byte
assertion; restored source passes. This prevents post-hoc overflow checking
from masquerading as bounded capture. Legacy host and E3 fixture suites PASS.

## B03 durable phases

The acceptance entrypoint now supports `start RECEIPT WORK RESULT LAUNCH
HOST_GEN ATTEMPT ATTEMPT_NO immediate|delayed`, and `observe|resume|signal
RECEIPT`. It publishes a <=2048-byte exact eleven-field handoff with source
head, commit marker and digest using no-replace link. Later callers take a
bounded snapshot, reject wrong schema/count/source/digest before provider
contact, and use the existing exact-attempt APIs. Resume only reads: it does
not restart or decide policy. `IB.WorkerLifecycle` remains unchanged.

Start inputs have a128-character transport limit before any provider contact;
the built receipt is capped2048 bytes before publication. These are harness
limits, not a new browser identity policy. Runtime fallback in the legacy entry
is explicitly forwarded to the native phase process. Actual native Grease
transport-mock suite PASS: independent start/later
observe/read calls; running vs uncommitted read68; denial37 and restoration;
simulated changed boot75; committed read; truncation/tampering76 before any
provider call; exact signal refusal78; duplicate start76; later committed read;
oversized input76 before contact; malformed provider UID2 and oversized remote
boot76 without receipt publication; runtime fallback with no configured env.
These are transport mocks, not actual worker or physical outcomes. Digest is
not authentication against a writer able to recompute it. Launch-before-receipt
interruption, same-UID concurrent path replacement, power-loss durability and
physical service/reboot outcomes remain unproven.

## B09 refusal until identity-bound signaling exists

Both owned-signal APIs previously checked `/proc` identity and then used numeric
kill. Same-shell sequencing and attempt locks cannot bind the signal to the
checked process if it exits and its PID is reused. This is source evidence,
not demonstrated exploitation or a physical failure. The child now validates
arguments and returns78 before provider contact or attempt mutation. Startup
identity-read failure also no longer numerically kills an unidentified child;
the existing gate wait is bounded and publication requires its attempt.

No qualified identity-bound Android signaling primitive is present in these
Grease/Binder adapters. Genuine installed NDK r27c27.2.12479018 `sys/pidfd.h`
guards pidfd_open/getfd/send_signal at API>=31, with `__INTRODUCED_IN(31)`;
header SHA256 `a055da168cd91e8c61f9837b9406ebf239f9b4d900b05170104f06d627ce8501`.
ARM32 API31 libc exposes these symbols; API28 libc has none. This proves only
the API surface, with no MIRO kernel/syscall/SELinux qualification. Grease/Oils
5651's `builtin/process_osh.py` blob `64abe579aed72b77bc24bb616b30eca1c26a9e02`
still calls posix.kill(pid,sig_num); inspected cpp/libc.h has no pidfd edge.
Idriç ff4d852 `System.Signal.prim__sendSignal` binds the C numeric kill wrapper;
its SubProcess pointer is a popen2 handle without an identity-bound signal
contract. No helper was compiled and no Android API level was guessed. A Linux
header or hosted pidfd availability would not qualify actual device support. Exact tuples,
stale tuples and unavailable authority send no signal; mock state stays
running and later committed reads recover. Actual owned-signal delivery and
controlled kernel exit/PID-reuse barrier cases remain BLOCKED, not passed.

The unchanged B01 baseline actual-worker-program test and this child both
exit44 at the first `/proc/$pid` identity read. This executor's `/bin/sh` reports
PID2 while `/proc/self/stat` reports an outer Linux PID (observed203500).
Consequently this environment cannot qualify the fixed worker's process
identity or kernel reuse test. We did not replace that limitation with a fake
physical row. The updated actual-program test source expects refusal without
mutation, but its execution is NOT_RUN beyond the same early refusal.

| Layer | Result |
|---|---|
| Grease native capture and handoff orchestration | PASS hosted |
| inherited provider/Binder/E3 mock behavior | PASS in named compatibility mode |
| Idriç lifecycle policy | unchanged; no new runtime claim |
| actual fixed background worker process identity | BLOCKED exit44 on unchanged baseline and child |
| identity-bound Android signaling | BLOCKED, fail-closed78 |
| qualified Android/ICK/NDK consumer build | NOT_RUN |
| MIRO A1 service-stop, reboot, kill, latency | NOT_RUN |

## Exact runtime boundary

Grease repository `ba869518c7d850de6c47d8c6234654575e264e6c`, implementation
`5651cf97a1b5042f24f14112a7ade9a1518eb0bc`, version0.37.0. Runtime launcher
`/workspace/scratch/4cbbf3939bb5/il0/qualification/grease` SHA256
`37088097c36a2f82cf45aaea9392e4a4cf16155fecb9da87d08a9c7fb4d92e4a`; underlying
implementation `7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`.
`ASAN_OPTIONS=detect_leaks=0` is required for this ptraced executor. New helpers
and suites use native Grease; inherited adapters execute in separately named
`+O ysh:all` compatibility processes. No stock interpreter/runtime acceptance.

Existing host coreutils used as orchestration primitives, not newly compiled:

| Executable | SHA256 |
|---|---|
| timeout | 375eaa8774baf7667515932c4d6fa2e31a2c21e9c50f152a27c4c6a718374ebe |
| head | 8a0e39c0c595570deebf85d24dc651ef14164046db88d1323d75126f1ce9ef6a |
| mkfifo | 3504240e1f81c9b130c63545d3581a2d52ffdd9817db56aafe886b29eff72409 |
| mktemp | 7258721fb887f4cef21e7013b0f6d970ed4ca58390ac5116d426679d13a80806 |
| wc | 672e0eba574b7b50607ef04d81fa1a6abc6e7a2b928740f2cf582645edb0da1f |
| cat | 90c9437a02857838ccc0ce1ff8652691181bfb67135a1173dd276f91fa57d7ec |
| rm | 42d99e8ead91f30586a0f689178e8bd3db5fadb1e995681aaf4b418cbeffab99 |

The qualified capture script is source, not a durable Linux Grease distribution
or an Android installation receipt. Hosted tests require explicit runtime
configuration. No CI installer, merge, release, device operation or automatic
retry was added. Outer caller death may leave the bounded temporary capture
directory behind; this is not a remote cancellation or caller-death cleanup
claim. New source is reviewable on its branch without starting inherited stock
shell/Chez/Java workflows via another PR.
