# E2 replacement publication source receipt

E2_SOURCE_SHA=`b731444de8bed8455f36a56998a10cbb38cabe97`

[Exact implementation source](https://github.com/isomorphisms/ib/commit/b731444de8bed8455f36a56998a10cbb38cabe97)
is contained in [isomorphisms/IB PR #115, “Reconstruct immutable-result publication acceptance on Longview”](https://github.com/isomorphisms/ib/pull/115).
This is a contract-derived replacement, not recovery of historical repairs.

The verified starting head and current actual Android program acceptance
baseline is `ea322cd1c2dcc9021efddd089b2593bd4b40dd6b`, the unchanged head of
[isomorphisms/IB PR #114, “Consolidate Longview through the Binder boundary with retained acceptance work”](https://github.com/isomorphisms/ib/pull/114).
GitHub comparison identifies it as the merge base of the replacement source.
The hosted builds explicitly checked out the exact E2 source, not a synthetic
merge commit. Subsequent receipt/handoff edits do not change implementation.

## Exact-source acceptance

- [Foundation run 37235523461](https://github.com/isomorphisms/ib/actions/runs/37235523461): PASS. Actual pinned Chez Idriç compiled production admission/worker policy, emitted actions and drove the real ordinary-file adapter. Both exact exit-37 probes passed. Existing admission, worker, Binder, work-bound and paint assertions remain present.
- [Universal gate 37235523534](https://github.com/isomorphisms/ib/actions/runs/37235523534): PASS.
- [Protected Android run 37235523508](https://github.com/isomorphisms/ib/actions/runs/37235523508): PASS. Build job 111533720609 passed production adapter unit tests, lint, package boundaries and stable signer verification. Renderer-loss job 111533935352, whole-host-loss job 111533935323 and cross-UID/Binder/live-PFD job 111533935373 passed on API 29 x86_64 emulators. Independent descriptors, late reads through a new provider generation and caller/provider death over the live channel remain verified.

## Acceptance surfaces

| Contract case | Execution and assertion |
| --- | --- |
| Concurrent unequal bytes | Two real staged writers, ordered release; exactly one winner, loser conflict 70 in ordinary adapter and IOException in Android adapter. |
| Equal-byte idempotence | Both concurrent writers succeed; later independent reads return the same complete bytes. |
| Source growth | Growth inside the actual copy after size observation; exit 73, no published prefix. |
| Interrupted staging | Actual disposable writer terminated at staging barrier; exit 143; read refuses absent object 68. Android stalled staging is also unreadable. |
| Reader bound | Ordinary reader emits no output on refusal 69; Android returns no byte array on oversized reads. |
| Simulated ENOSPC | Partial stage plus injected I/O failure; ordinary exit 72 / Android IOException, no result and failed staging removed. |
| Immutable committed bytes | Unequal retries cannot replace the winner; known-bad regular-file atomic rename is deterministically rejected by the same race assertion. |
| Backend/media/stale identity | Production Idriç refuses mismatched location; changed transport identities return 75 without an adapter call. Replacement media fails ordinary lookup 67. |
| Unavailable observations | Actual compiled unknown, stale, denied, read-only and absent plans cause zero publication invocations. |
| Committed versus authority | Existing worker committed state remains committed/no-restart under later authority loss; already committed ordinary bytes remain readable after every refusal. |
| Baseline retained result | Legacy Android flat files remain readable/idempotent; unequal retry refuses without migration. Hosted app reopens committed bytes after host/renderer loss without reacquisition. |
| Failed children | Partial policy output followed by exit 37 reaches both orchestration and logging as exactly 37 before publication. |

Ordinary acceptance is `tests/test_durable_object_store.grease`; the compiled
policy bridge is `src/ResultPublicationAcceptance.idric` and
`tests/test_result_publication.grease`; native adapter tests are
`DurableResultStoreTest.java`. The ordinary adapter uses the inherited POSIX
compatibility interpreter. This is real filesystem implementation evidence;
actual Grease runtime execution remains unverified. Admission evidence comes
from the compiled production Idriç program.

The earlier Android hard-link candidate is rejected: diagnostic source
`b3de3e3f80b4b684d557eb27e672b5c447464070` recorded AccessDeniedException
at publication in run 37235216146. The accepted candidate uses atomic
nonempty-directory publication and preserves existing flat-file results.

No physical MIRO/Shizuku acceptance, complete E1 worker-authority acceptance,
or whole-device power-loss guarantee is claimed. E3 launch/attempt binding and
actual E4 retained-result admission integration remain separately scoped.
Idriç retains policy and reservation accounting, Grease retains orchestration,
and adapters do not admit work. No merge, physical-device mutation, signer
change, new provisioning or automatic state-changing replay occurred.

The bounded downstream handoffs are `docs/jobs/moon-e2-b-publication-mapping.txt`
and `docs/jobs/earth-e3-worker-publication.txt`; both use this exact E2 source.
