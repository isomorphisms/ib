# Bounded Longview worker lifecycle

This is the functioning worker boundary under Longview. It remains browser-owned
policy above Binder rather than a second browser/task framework or a
provider-specific Shizuku model.

## Identity

The browser-owned worker record separates:

- durable work identity;
- durable result identity;
- operation/replay class;
- monotone attempt count and restart limit;
- current lifecycle phase;
- the exact current process attachment.

One process attachment binds all of the following to one execution attempt:

- launch identity;
- Android/Linux boot identity;
- UID;
- PID;
- process-start identity from `/proc/<pid>/stat`;
- process name;
- Longview host generation;
- attempt identity;
- monotone attempt number.

PID, process-start identity, and the remaining attachment fields never replace
work or result identity.

A running attempt cannot be replaced. A retry can attach only after a terminal
or authority-unavailable phase and must advance the attempt number. Completion,
exit, kill, and authority-loss transitions compare the supplied attachment with
the current attachment before mutating task state. A stale launch, boot, UID,
PID, process start, process name, host generation, attempt identity, or attempt
number therefore cannot complete or mutate the current attempt.

Authority loss retains the process attachment as evidence. It does not turn a
committed result into loss. A frontend or initiating Binder-provider caller
disconnect likewise does not mutate the durable worker task.

## Recovery policy

The policy remains deliberately conservative:

- a committed result is never replayed merely because the worker later exits or
  authority disappears;
- a bounded observation may be automatically restarted while attempts remain;
- unavailable privileged Binder authority waits for authority rather than
  becoming success; the selected provider today is Shizuku;
- a state-changing/nonreplayable operation requires manual recovery rather than
  automatic replay.

E3 does not add automatic state-changing replay.

## Binder and Shizuku attempt boundary

`lib/android_longview_worker.grease` is Binder-facing. Longview supplies the
browser-owned work/result, launch, host-generation, attempt identity, and
attempt number. The current provider observes boot identity, UID, PID,
process-start identity, and process name and returns them in the launch receipt.

The receipt shape is:

```text
worker<TAB>launch_id<TAB>boot_id<TAB>uid<TAB>pid<TAB>start_ticks<TAB>process_name<TAB>host_generation<TAB>attempt_id<TAB>attempt_number
```

Every authoritative worker status/read/observe/signal operation carries that
same complete tuple. The provider compares it with the current attempt record
and the current boot/UID before proceeding. Worker-specific signaling then
rechecks PID/start/name immediately before `kill`. The generic PID/start/name
signal primitive remains a lower Binder facility, but Longview E3 does not use
it as worker authority.

The provider serializes launch decisions for one work identity and refuses:

- replacement while the current attempt is running;
- restart after a committed result;
- non-monotone attempt numbers;
- any stale or ambiguous attempt tuple.

Provider calls remain bounded through the existing bounded Shizuku invocation;
a stalled rish call does not become success.

## E2 publication binding

E2 still owns storage admission and immutable publication.
`IB.StorageAdmission.publication_for` produces the already-admitted action with
its unchanged result identity, byte limit, backend/media identity, observation
identity, reservation receipt, and reserved ledger.

E3 does not admit work or reserve capacity again.
`ib_publish_owned_admitted_result` checks a bounded worker completion record
against the exact expected attempt and checks that the admitted action names the
same result identity. Only then does it call the existing E2
`ib_publish_admitted_result` path. A stale completion returns a worker-identity
mismatch before the immutable-store adapter is invoked.

This keeps the E4 boundary explicit: E4 must supply a real retained-result
admission/action. E3 does not invent retained-page admission or double-charge a
reservation.

## Android shell fixture

The current Shizuku fixture still uses:

```text
/data/local/tmp/ib-longview-fixture-v1
  .ib-backend-id
  .ib-staging/
  objects/
  work/
```

This directory is intentionally shell-owned and the payload is deliberately
non-sensitive. It is not selected product storage.

The result fixture is bounded to 1024 bytes because the v2 record now carries
the full attempt evidence. Publication remains staged and synchronized, and
committed results are immutable by result identity. The attempt/status files
are control evidence rather than result storage.

## Hosted acceptance

The deterministic E3 suites exercise actual policy and adapter calls for:

- running-attempt replacement;
- stale launch/boot/UID/PID/process-start/host-generation/attempt identity and
  attempt-number refusal;
- stale mutating signal refusal;
- exact owned signal;
- monotone retry and rejection of completion from the old attempt;
- authority loss with retained attempt evidence;
- committed result under later authority loss;
- bounded output refusal;
- hung provider calls and stalled peers;
- E2 immutable publication races, source growth, ENOSPC, bounded reads,
  backend/media/observation mismatch, repeated reads, and exact exit-37
  propagation.

Hosted Android acceptance remains separate from the shell-worker fixture and
continues to exercise retained app-private results, cross-UID provider reads,
live PFD behavior, renderer loss, and whole-host loss on an emulator.

## Physical acceptance still required

The MIRO/Shizuku path remains a separate physical gate. At one exact source
head, test immediate completion, delayed caller loss, host loss, exact owned
kill, authority stop/restart, reboot behavior, committed-result recovery, and
stale-attempt refusal. Hosted success is not physical MIRO evidence.
