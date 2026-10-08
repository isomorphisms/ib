# IB-R1 recovery — 2026-10-08

Owner: [IB issue 120](https://github.com/isomorphisms/ib/issues/120).

This pass recovers existing obligations, emits bounded SUN assignments, and
executes a first independent wave. Queue composition, dispatch, source changes,
host execution, CI, and physical-device acceptance are separate outcomes.

## Recovered intent and coverage

The accessible evidence consists of the current issue assignment, Git refs,
source trees, issue/PR metadata, and targeted prior-conversation retrieval.
It is not an exhaustive transcript of every IB discussion. Retrieved historical
file excerpts were leads; current source and live trackers determine current
state. User corrections outrank older implementation plans.

Recovered direct decisions:

| Date (UTC) | Decision | Consequence for this queue |
| --- | --- | --- |
| 2026-08-24/25 | IB owns browser resource, memory and disk management; views do not own the corpus. | No parallel browser model or UI-owned history. |
| 2026-09-11 02:11 | Prepaint was expedient scaffolding; Material 3 supplies the intended rendering path. | Preserve acquisition/extraction semantics; do not commission expansion of obsolete prepaint rendering. |
| 2026-09-11 | Exact reading search returns source fragment IDs and neighboring context, preserving original-language source authority. | Reading semantics remain testable without a UI or model. |
| 2026-09-16 | Substantive IB behavior should remain terminal-testable; Material 3 is an Android adapter. | Separate semantic and presentation acceptance. |
| 2026-10-01 | UI/renderer may die while durable work and information survive; inspect disk/SD/internal admission. | Background work cannot depend on a kept-alive presentation surface. |
| 2026-10-01 | Inventory Binder workarounds before restructuring; reacquire authority after reboot. | Do not assume persistent shell authority from durable client files. |
| 2026-10-02 | Binder is the boundary; Shizuku is the current replaceable provider. | Keep browser policy and provider command lowering separate. |
| 2026-10-05 | Continue the repaired Longview line; do not create a competing worker-identity implementation. | Reuse merged attempt-binding work. |
| 2026-10-08 | Recover already-requested unfinished IB work and execute independent slices without routine oversight. | Concrete assignments and execution receipts, with real external blockers isolated. |

FastChat-only decisions are comparison material, not authority to replace IB's
semantics or runtime. The optional userspace append arena does not make kernel
appendFAT, raw SD writes, or a mount a prerequisite for ordinary-file IB work.

## Exact inspected anchors

| Owner | Ref | Inspected revision |
| --- | --- | --- |
| IB | main | `10fa7bbe93ff9ffa1ba7b9f62e0dd8f6c10a86bc` |
| IB | longview-renderer-loss-acceptance | `843c7bcf5336c685c4a96a4b4bc28b96fbc7a5dc` |
| IB | 0.2/pensieve-arxiv | `b4d93c47327f112433b90c0a77a00b3ddcba0af4` |
| IB | style/current-idric-ib | `631052fbeccd183c4fd409428180b3eab824c481` |
| ai-ci | main | `01608a2493fa409463f70e8fbfd8a123ef59ee85` |
| Flexible Pipes | main | `9775aa324f523cbc6c758e89461915484a5a0fc7` |

Recheck mutable refs before every job. Main is already an ancestor of the
retained Longview head; the comparison is 0 commits unique to main and 216
unique to Longview. A restack is not currently a missing implementation.

[isomorphisms/ib PR #114, “Consolidate Longview through the Binder boundary with retained acceptance work”](https://github.com/isomorphisms/ib/pull/114)
is closed and unmerged, with its branch preserved. It remains the Longview
integration owner. Neither this recovery nor its first wave reopens it.

[isomorphisms/ib PR #115, “Reconstruct immutable-result publication acceptance on Longview”](https://github.com/isomorphisms/ib/pull/115)
and [isomorphisms/ib PR #117, “Bind Longview worker completion to exact execution attempts”](https://github.com/isomorphisms/ib/pull/117)
landed on the retained Longview line. They are not on main merely because they
are merged, and they must not be regenerated as new E2/E3 jobs.

## Execution path

The verified execution host is the disposable Ubuntu 24.04.3 x86_64 container.
No physical MIRO A1 session or live Shizuku authority was observed. Git access
and connected GitHub reads work; the local `gh` executable is absent.

Flexible Pipes' current `docs/job-delivery.md` states that no deployed ChatGPT
visible-sink adapter exists. Its maintained runner stops with
`AWAITING_HANDOFF_DELIVERY`; neither a local response file nor fixture receipt
establishes a real UI delivery or dispatch. No automated FP/SUN dispatch or
paid external model run is claimed.

The first wave uses the actual in-session execution workers. Full assignments
were shown in chat before their dispatch. Worker task identifiers and results
are recorded in `execution.tsv`. These identifiers are session dispatch receipts,
not Flexible Pipes receipts or proof of a particular commercial model tier.

## Worktree rules

Keep all feature work isolated. The recovery branch owns only this directory.
Q01 starts from main and owns the exact-text workflow/regression. B01 starts
from the retained Longview head and owns authority-status handling plus its
host/Binder tests. No other worker may edit those scopes concurrently.

The display, backend and storage job files carry source-specific prerequisites.
Canonical Idriç, retained-content bridges, navigation, and UI projections need
dependency order even when their initial source inspection can run in parallel.
`READY_CI` means a job has an independent hosted starting slice, not that its
toolchain, tests, or physical behavior have already passed.

No merge, closure, release, device mutation, credential enrollment, destructive
storage change, or provider substitution is authorized by this queue.

## Genuine remaining human boundaries

Physical A1/live Shizuku acceptance and any real account enrollment require the
corresponding device/account context. Those boundaries do not block unrelated
hosted work. Resolve missing implementation qualification through the existing
ICK/NDK/Idriç owners; do not silently compile via a conventional fallback.

No immediate product-choice question is needed for the first execution wave.
