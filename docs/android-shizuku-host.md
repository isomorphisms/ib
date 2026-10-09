# Shizuku provider for the Android Binder host

## Current decision

Binder is IB's Android process/service architecture. Shizuku is the current
provider used to obtain that Binder authority on stock experimental Android.

The canonical boundary is documented in `docs/android-binder-host.md`.
Longview and browser semantics must depend on Binder capabilities rather than
on Shizuku, `rish`, or shell command syntax.

Crawl Space remains a possible later provider/variant. It is not an additional
mandatory hop in front of Shizuku.

```text
IB / Longview
     |
     v
Binder capability boundary
     |
     v
Shizuku provider today
     |
     +--> current rish/shell lowering
     +--> direct Binder lowering as it is physically proved
```

## First implemented surface

`lib/android_shizuku_host.grease` currently provides:

- discovery of the `rish` executable, with `IB_RISH_COMMAND` as an explicit
  override;
- execution of one trusted Grease command through `rish -c`, preserving the
  remote exit status and stderr;
- effective-UID / authority inspection;
- an identity receipt including the remote `id` result and SELinux context
  when Android exposes it;
- a raw process snapshot through `ps -A`;
- validated PID/signal forwarding;
- package PID lookup through `pidof`;
- deliberate Android package stop through ActivityManager's `am force-stop`.

`bin/ib_android_shizuku.grease` exposes those operations for development.

Authority inspection preserves a failed UID query's exact nonzero status and
emits no authority on failure. A successful query must contain decimal digits
only: `0` means `root`, `2000` means `shell`, and other numeric values retain
the `uid:VALUE` classification. Empty or malformed output is rejected with
status 2 rather than being classified as authority. The Binder-facing caller
preserves the same result.

This is deliberately a small mechanism surface. Browser tasks, tab identity,
restart policy, durable results, and renderer policy remain owned by the Idriç
core.

## Bounded provider capture

Every `ib_shizuku_run` operation now uses the same bounded capture as
`ib_shizuku_run_bounded`. The default provider budget is five seconds, 65,536
stdout bytes and 16,384 stderr bytes. `IB_SHIZUKU_TIMEOUT_SECONDS`,
`IB_SHIZUKU_STDOUT_LIMIT_BYTES` and `IB_SHIZUKU_STDERR_LIMIT_BYTES` configure
these budgets. Short identity queries and bounded process/capability snapshots
share conservative limits; a larger acquisition uses its own explicit budget.

The provider-neutral `lib/bounded_process_capture.grease` takes
`SECONDS STDOUT_BYTES STDERR_BYTES EXECUTABLE [ARGUMENT ...]`. It does not
evaluate command text. Call it with the exact qualified absolute Grease path
also supplied as `IB_GREASE_COMMAND`. The existing host library uses inherited
Grease shell compatibility; the capture helper and its adversarial suite use
Grease default mode. A stock-shell invocation is not a supported substitute.

Time must be 1–300 seconds; each stream budget is 0–1,048,576 bytes. Two FIFO
readers retain at most each budget plus one detection byte, so the temporary
stream files cannot grow without bound before a post-run size check. The
private directory is created under `ENV.TMPDIR` (default `/tmp`). Producer FIFO
opens occur inside its deadline. Reader deadlines add two seconds, with a
one-second kill grace; completion can therefore take up to the requested
deadline plus three seconds, apart from scheduler/filesystem delays.

Successful bounded output is emitted only after all three directly started
helpers have been waited for. Within limits, the exact producer status and
stderr survive; every failure suppresses stdout. Overflow returns 65, incomplete
reader capture 74, invalid budgets 2, and missing runtime/primitives 127.
Overflow/incomplete-capture diagnostics are short local messages, separate from
the provider stderr budget. A producer's own status 124 or 137 is preserved
without inventing a timeout cause. A local timeout is never remote-process-death
evidence.

This is trusted synchronous-operation capture, not a new process supervisor.
It does not contain arbitrary daemonizing providers, descendants that detach
after their monitored parent exits, or remote Android workers. Cleanup waits
for the directly started finite-deadline helpers and removes the FIFO directory;
the GNU timeout process-group behavior applies while its producer remains
monitored. Exact executable/primitives and Android behavior require separate
qualification. See `docs/shizuku-bounded-capture-receipt.md` for tested limits.

The package-stop operation is immediately useful to the protected long-view
work: physical acceptance can kill the actual IB package through Shizuku rather
than requiring an ADB session, then verify that the durable task survives the
new host generation. The stop mechanism remains distinct from the durable task
semantics being tested.

## Current rish lowering

Upstream rish is specifically a shell whose process is created by the
high-privilege Shizuku/Sui daemon. It passes shell arguments to the remote
Android shell; for example, `rish -c 'ls'` executes `/system/bin/sh -c 'ls'`
remotely.

That makes it a useful first backend for IB process and operating-system work.
If command startup or text parsing later becomes measurable overhead, a direct
Shizuku Binder adapter can replace individual hot operations behind the same
Binder boundary.

Long-running work should eventually use maintained remote workers rather than a
large number of tiny `rish -c` calls. Those worker processes/threads remain
normal kernel-scheduled work; Shizuku supplies authority, not CPU scheduling.

## Evidence boundary

`tests/test_android_shizuku_host.grease` uses a fake rish executable only to
test local adapter behavior: argument forwarding, exit-status preservation,
identity classification, process/signal dispatch, and rejection of an unsafe
PID string.

The authority cases exercise both the Shizuku adapter and real Binder delegation
with root, shell, and ordinary UID controls; provider exits 37, 124, and a
permission denial; missing/nonexecutable providers; and malformed UID output.
`tests/test_android_binder_host.grease` also checks status forwarding at the
isolated provider boundary.

That test is **not** Shizuku acceptance. Physical MIRO A1 acceptance still must
show the exact branch revision running against real Shizuku/rish and record the
remote UID, SELinux context, process visibility, signal behavior, and exact
denials.

Upstream rish reference:
https://github.com/RikkaApps/Shizuku-API/tree/master/rish
