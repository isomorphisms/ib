# B06 bounded provider capture

Type sketch before implementation: a trusted operation consists of validated
command text and positive elapsed/stdout/stderr limits. Its result is either
the exact producer status plus bounded bytes, or a local refusal/timeout/output
overflow with no success payload. A timeout never asserts remote termination.
Grease owns process/file orchestration; no browser policy is added here.

Implementation choice: one native Grease helper inside the existing external
`timeout` process group. Two FIFO readers retain at most each byte limit plus
one overflow sentinel. The helper waits for both readers before returning;
descendants that retain descriptors therefore also meet the elapsed limit.
No compiler or new native executable is introduced. The inherited provider
module remains a named Grease compatibility boundary.

Refusal codes: 2 invalid input, 69 missing capture prerequisites, 65 output
overflow, 124 timeout (137 if the kill-after escalation is required). Within
budgets, the original producer status and stderr remain visible. For every
nonzero result stdout is withheld so a plausible prefix cannot claim success.

The hosted recipe must record the actual Grease/coreutils executable identities
and exercise flood, partial hang, denial, exact exit 37, descriptor-retaining
children and later recovery. Android Shizuku and device evidence remain NOT_RUN.

## 2026-10-12 reconciliation

The separate capture above is historical. B03 now delegates its existing
operation-specific budgets to B06's `lib/bounded_process_capture.grease`,
reused byte-for-byte from `6b3350acccb22ba27a3546a7ea56fc672313cf2a`.
`lib/android_shizuku_capture.grease` is removed; no production caller uses it.
The shared helper starts each finite deadline before FIFO open, including a
reader that fails before opening. Missing shared primitives now return 127;
an incomplete stream returns 74. B03's missing entrypoint still returns 69.
Exact producer failures remain unchanged and never expose success stdout.

Actual native Grease 5651cf97 executes the shared positive/adversarial capture
suite, including the uncapped-reader mutant and failed-reader case, plus B03's
handoff suite. Existing host and worker adapter tests pass in their explicit
compatibility mode. Actual fixed-worker identity remains blocked at exit 44;
Android cancellation and physical-device results remain unrun. See
`docs/ib-b01-b09-resumption.md` for exact inputs and evidence boundaries.
