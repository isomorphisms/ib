# Program attempt: Pensieve Quick Settings executable bindings

Status: `PARTIAL` — consumer source checking passes against the published
backend candidate. This change records that candidate in the gitlink for hosted
verification; DEX emission and runtime acceptance are pending.

## Purpose and ownership

Give IB/Pensieve an executable acceptance source for the shared Quick Settings
bindings while the tile's purpose remains undecided. This continues the
declaration-access work already merged in
[isomorphisms/ib #131 — Expose Quick Settings bindings to Pensieve](https://github.com/isomorphisms/ib/pull/131).
The existing signature-only
fixture and its historical source-check receipt remain unchanged.

The source directly imports `Android.QuickSettings`. The dependency owns all
platform declarations and reference types. This example is an external-test
entry point, with an empty `main`; it does not connect a binding to browser
startup, a user action, a service, or a manifest.

## Type-system sketch

### Values and domains

`TileService`, `Tile`, `StatusBarManager`, `ComponentName`, `CharSequence`,
`Icon`, `Executor`, `TileAddResultConsumer`, and `BoxedInteger` remain distinct
external reference types from the dependency. `TileState` and `AddTileResult`
remain the dependency's domain data. Framework references include Java null;
their types do not prove API availability, non-nullness, or service lifecycle.

### Types and signatures

| Export | Checked Idriç signature | Observation |
| --- | --- | --- |
| `service_tile` | `TileService → IO Tile` | Forward the service's actual framework reference. |
| `tile_state_code` | `Tile → IO Int32` | Project the public `Maybe TileState` internally; unknown state becomes `-1`. |
| `set_active_then_update` | `Tile → IO ()` | Sequence the public state setter and updater. |
| `submit_add_request` | `StatusBarManager → ComponentName → CharSequence → Icon → Executor → TileAddResultConsumer → IO ()` | Preserve all six framework arguments in order. |
| `callback_result_code` | `BoxedInteger → IO Int32` | Read through the public binding and project every declared result case; preserve unknown codes. |
| `callback_result_tag` | `BoxedInteger → IO Int32` | Observe known constructors as distinct tags 0–8 and unknown as 9. |

### Actions and effects

Platform operations stay in `IO`; result projection is pure. The setter must
precede the updater. The request call must retain the virtual receiver and
all five arguments. World-token erasure and argument staging belong to the
checked direct DEX backend, not to the consumer.

`main` performs no platform action. A future valid caller must supply actual
framework references and lifecycle ownership. No Tile constructor is invented
to make the example executable without that caller.

### Invariants and failures

The result observation must preserve codes `0`, `1`, `2`, and `1000` through
`1005`. Every `UnrecognizedAddResult code` returns that same code. Neither an
error nor an unknown payload becomes addition success. Framework exceptions
remain failures. A valid Tile and listening interval remain prerequisites for
an actual update; compiling this example cannot establish them.

The runner must check both code and constructor tag. Code identity alone
would allow a broken decoder that always chose `UnrecognizedAddResult` to
pass; distinct known tags expose that failure without assigning tile behavior.

### Execution boundary

The candidate route is IB-owned `.idric` source → actual Idriç checked ANF →
the dependency's direct DEX backend → emitted DEX → an independent ART caller.
The exported ABI names match the shared upstream `QuickSettingsRunner`, so an
external caller can pass real `Integer.valueOf` inputs to
`callback_result_code(Ljava/lang/Integer;)I` without a second binding facade.

Each generated fixture uses `LIdric/Generated;`. Run this candidate alone with
its matching runner; do not combine it with another fixture's candidate DEX.
The runner is an observation tool. All binding calls under test must come from
this checked source and the pinned dependency.

## Attempt and source identity

This worktree starts from the live `0.2/pensieve-arxiv` commit
`d10eeda153dae4be25fd7902338e8b8443e0666e`, verified on 2026-10-09 UTC.
The base commit's android-NDK gitlink is
`65769fd68a30a26e0dacd2d0f9ba182850857e86`. Initial source checking kept that
declaration-only pin in the index while checking the isolated submodule at
candidate
[`3fb6a75d8cffe2f9bff77ae58754e7344d65e20a`](https://github.com/isomorphisms/android-NDK/commit/3fb6a75d8cffe2f9bff77ae58754e7344d65e20a),
from [isomorphisms/android-NDK #20 — Lower checked framework calls and IO directly to DEX](https://github.com/isomorphisms/android-NDK/pull/20).
This draft change records the candidate in the gitlink so its hosted checks
can run in parallel with upstream verification. Final acceptance requires
both upstream and consumer evidence for the recorded revision.

Source: `PensieveQuickSettingsRuntime.idric`.
This attempt and type sketch were written before the source.
Its SHA-256 is
`f5c3980c6a471188dbe4a437d97fec9de73da61b7e5f215c01b192707c54ecc9`.

## Result or failure

| Boundary | Current result |
| --- | --- |
| Historical signature-only access | Preserved in `../quick-settings-bindings/` |
| Existing public vocabulary check, core `src` and this new example | `PASS` on 2026-10-09 UTC |
| This consumer's source check against candidate `3fb6a75` | `PASS` on 2026-10-09 UTC |
| Candidate dependency pin | `3fb6a75`; runtime acceptance `PENDING` |
| This consumer's direct DEX emission | `NOT_RUN` |
| This consumer's ART execution | `NOT_RUN` |
| Upstream candidate ART execution | `PENDING`; no runtime result claimed here |
| Tile lifecycle, user approval, registration, app behavior, physical device | `NOT_RUN` |

Upstream fixture emission or ART evidence must be recorded separately from
this IB-owned source. The consumer needs its own source and emitted-artifact
identities; an upstream pass alone is not an IB execution receipt.

The fresh source check used actual Idriç
`ff4d852862a3942592f8ade9afde8d409d9803be`, explicit checked prelude/base module
paths, and an isolated prefix with no installed packages. The candidate's
binding package was checked first into a fresh output directory, then this
consumer was checked with that directory added to `IDRIS2_PATH`.
Both commands returned status 0. [source-check.txt](source-check.txt) records
the exact commands, dependency tree, source hashes, and compiler identity.
No backend build, DEX emission, or Android operation was performed by these
source checks.

The vocabulary checks used the repository's existing
`tests/idric-public-vocabulary.sh`, once with its default `src` root and once
with `IB_IDRIC_SOURCE_ROOT=examples/autogenerated/quick-settings-runtime`.
Both returned status 0 and `public Idriç vocabulary PASS`; neither compiled
the program or ran an Android operation.

## Fallback

None. No Java, JNI, Gradle, host implementation, or handwritten candidate DEX
replaces the checked Idriç program. The shared external smali runner is an
independent caller and does not provide candidate semantics.

## Verification orchestration sketch

This sketch precedes authoring `verify.grease` and the Pensieve Quick Settings
workflow. Grease owns process orchestration and evidence collection; the
Idriç fixture continues to own the candidate behavior. The workflow checks
out the exact IB pull-request head and initializes only its canonical
`vendor/android-NDK` gitlink. It uses the upstream workflow's pinned Idriç,
Cat Food, Grease, Android setup, emulator, and artifact actions.

The host stage first invokes the dependency's reusable
`dex/idric/verify-effects.grease`. The IB stage consumes that fresh driver and
checked binding modules, then compiles this IB source into its own fresh
evidence directory. Compiler errors, missing or empty DEX/plan/checked-ANF/
smali artifacts, failed independent `dexdump` parsing, and a byte difference
from upstream `quick-settings.dex` all refuse acceptance. Source, compiler,
dependency, IB revision, and artifact hashes belong in the receipt.

The ART stage invokes the dependency's unchanged four-candidate runner.
Only after that script succeeds may the IB receipt record that its emitted
DEX is byte-identical to the upstream Quick Settings candidate observed by
that runner. It must also compare the IB DEX with the candidate bytes pulled
back from the emulator. This is shared execution evidence for identical
bytes; it is not a separate IB Android lifecycle, callback dispatch, or tile
registration test. Existing source-only receipts remain historical.

The authored `verify.grease` passes syntax checking through actual pinned
Grease on 2026-10-09 UTC; `git diff --check` also passes. The exact command and
script identity are in `orchestration-check.txt`. Local syntax checks do not
stand in for hosted builds, byte comparison, or ART execution; those results
remain pending until observed.

## Language work exposed

Acceptance depends on checked external references, IO and void results,
ordered effects, internal algebraic projection, imported public wrappers, and
six-word virtual-call placement. A failure in any of those layers is retained
as the actual compiler or runtime result; it is not hidden behind a fabricated
Android object or an assigned tile purpose.
