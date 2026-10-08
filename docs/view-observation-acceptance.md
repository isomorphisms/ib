# View-observation journal acceptance, 2026-10-08

IB-S07 is a child of the implemented source contract in isomorphisms/ib
PR #123, “Persist source-bound fragments and checked reading traversals”
(https://github.com/isomorphisms/ib/pull/123), remote dependency
a3bbcb5741d34318d85ab32ad9627abddb6f4a62. This dependency has semantic and OS
boundary evidence but remains PARTIAL at qualified executable generation.

`src/IB/ViewObservations.idric` records explicit adapter and observation IDs,
monotonic clock session, exact source-revision binding, canonical fragment and
Unicode code-point range, interval, kind, foreground and occlusion. It records
what an adapter reported; it does not infer that a person read or understood
the content. An equal observation identity and equal facts replay idempotently.
Conflicting facts or overlapping intervals for the same adapter/clock/fragment
refuse. Different adapters retain separate observations.

The journal is private ordinary TSV. Its header pins S06's source binding.
A newline commits each row. Replay preserves earlier valid observations,
reports refused complete rows by position and reports an incomplete final tail.
The version-one derived duration counts unoccluded foreground visibility per
fragment. It is rebuildable and is never a canonical psychological label.
Append normalization refuses to silently rewrite a corrupt or torn prior
journal; the preceding complete immutable generation remains available.

`lib/view_observation_store.grease` snapshots bounded source, graph, prior
journal and proposed-row files. It verifies source SHA-256 and UTF-8, preserves
normalizer status, and uses the existing Longview immutable publication adapter
for each accepted journal generation. It does not replace that adapter or
modify an existing committed inode.

## Evidence

The exact current Idriç checker is the clean compiler checkout
ff4d852862a3942592f8ade9afde8d409d9803be, implementation SHA-256
10002074cfae31a15e6f136cd191b6abe9f8d58efcec438d21145e72802b2819.
Actual `--check` passes for the core, fake-clock acceptance and append command.
The compiler's `--client acceptance_results ViewObservationAcceptance.idric`
evaluates the actual pure Idriç semantics without consumer code generation.
All seventeen results are True.
The acceptance program covers distinct slow/fast/return/occluded/background
facts, foreground duration, duplicate callback/replay, conflicting identity,
overlapping interval, backwards clock, stale source, alternate view range,
complete round-trip, torn tail and invalid complete row recovery, source-bound
replay, derived duration rebuild, duplicate append and torn-prior append refusal.

The known-bad mutation removing the foreground condition makes
`foreground-duration` and `torn-final-append-preserves-prior` False. The original
condition is restored in delivered source. Typechecking and compiler
normalization establish neither packaged runtime nor physical visibility.

Actual current Grease source 5651cf97a1b5042f24f14112a7ade9a1518eb0bc, binary
SHA-256 7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c,
passes `tests/test_view_observation_store.grease`: immutable journal generation
and independent reopen; plausible partial producer bytes followed by exit 7
do not publish; the prior complete generation survives; changed source bytes
refuse. The controlled producer is expressly an OS fixture. It does not
substitute for the Idriç normalizer executable.

The unchanged Longview adapter hash and inherited Grease compatibility options
are documented in `fragment-store-acceptance.md`: it requires
`+O no_parse_sh_arith +O strict_errexit +O no_osh_builtins`. The sanitized
container runtime uses `ASAN_OPTIONS=detect_leaks=0` because its process-task
inspection is unavailable. Public Idriç vocabulary and whitespace checks pass.

## Remaining acceptance

S07 is PARTIAL. Integrated Idriç→Grease execution still depends on the same
qualified ICK/NDK consumer executable gate as S06. No consumer compile/link,
stock runtime, RefC or alternative backend was used to bypass it. Physical
Android visibility/occlusion, replacement installation and a real private
corpus are NOT_RUN. D06 owns actual native view-observation wiring.
