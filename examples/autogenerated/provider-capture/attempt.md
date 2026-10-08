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
