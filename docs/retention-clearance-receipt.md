# Retention clearance — IB-S11

The existing scientific navigation producer called acquired source bytes
clearable solely because it placed them below the prefetch cache. The new
Idriç retention boundary preserves canonical information, acquired bytes,
accepted decisions, last-complete results and unknown retention. Only an
explicitly requested derived projection with complete offline-rebuild evidence
may receive the `clear_qualified_projection` result. Temperature, pathname and
placement are absent from that permission input.

`prefetch_storage_retention` maps either inherited prefetch placement to
`acquired_information`. The actual Grease scientific producer now records
acquisition and unavailable rebuild evidence instead of transient-clearability.
Harvest failure preserves its status, creates no complete result and leaves
previous complete bytes intact. No eviction service or real cache-clear was
added. `RebuildEvidence` contains already-observed facts; the later store
consumer must verify identities/hashes against actual retained bytes before
acting. The pure policy is not a verifier for arbitrary caller assertions.

Actual Idriç typechecker and compiler-evaluator evidence:
`ff4d852862a3942592f8ade9afde8d409d9803be`, clean compiler source; wrapper
SHA-256 `55bd2f65fde3fd6093048e75369002d14f12db8f12258778d197376e5f37c0bf`;
image SHA-256 `10002074cfae31a15e6f136cd191b6abe9f8d58efcec438d21145e72802b2819`.
`RetentionCases.all_expected` evaluates True. Changing the prefetch mapping
to derived information makes both `all_expected` and
`prefetch_placement_cannot_authorize_clearance` evaluate False. Restoring it
restores True. Cases preserve canonical/source/decision/last-complete data and
refuse missing inputs/decisions, unavailable media, unobserved offline rebuild,
wrong output and unrequested clearance.

Actual Grease producer fixture execution uses
`dilapidated-shed/grease@ba869518c7d850de6c47d8c6234654575e264e6c`, pinned
implementation `5651cf97a1b5042f24f14112a7ade9a1518eb0bc`.
Wrapper SHA-256 `37088097c36a2f82cf45aaea9392e4a4cf16155fecb9da87d08a9c7fb4d92e4a`;
image SHA-256 `7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`.
The test uses explicit `+O ysh:all` for the inherited shell subset and
`ASAN_OPTIONS=detect_leaks=0` for this ptraced host. It exercises the actual
producer with a harvest double: successful publication, identical replay,
failure status 37, no false completion, subsequent successful publication and
unchanged prior bytes. Restoring the old transient trigger makes the test
exit 1; restoring acquisition makes it exit 0. This is producer transport
evidence, not actual extraction or Idriç filesystem-consumer execution.

First blocker: the qualified ICK/NDK consumer executable and kernel-enforced
store evidence consumer do not exist for this slice. Actual offline rebuild
and clearance, filesystem crash guarantees, automated CI, APK/device/media
acceptance remain NOT_RUN. The inherited broad network fixture was updated
but not re-executed here; the focused producer fixture needs no network.
