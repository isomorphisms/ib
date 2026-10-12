# Long View: restore the chosen Idriç / Android boundary

The 2026-10-09 C67 startup failure came from the separate Java Long View harness.
Its path-validation patch is not acceptance of the selected Idriç architecture.
Keep that failure as regression evidence; do not clear user data, uninstall, or
ask for another test of that harness as an Idriç implementation.

## Current scope and pins — 2026-10-12

The existing `IB.LongViewLifecycle` delegates to `IB.LongViewTask`. It distinguishes
same-host Activity recreation from process restart and checks renderer observations
against task/tab/navigation/host/renderer identities and generations. This
integration does not change either production module, its serializer, storage,
worker or UI. `LongViewAndroid.idric` remains the small diagnostic export taking
five runtime identity strings and returning the resulting host generation.

The current Git submodule and verifiers require:

- checked DEX backend `d0080ff0ad938680d319519bf484f98f540d3e89` from android-NDK
  PR #24, **Lower checked unpack-derived character views and tail scans to DEX**;
- actual Idriç compiler `ff4d852862a3942592f8ade9afde8d409d9803be`;
- the same inherited Cat Food/Grease bootstrap interfaces and compiler API cache.

The backend's [exact-head run 38216174983](https://github.com/isomorphisms/android-NDK/actions/runs/38216174983)
passed all required steps, including the previously failing final receipt gate.
[Artifact 11732488764](https://github.com/isomorphisms/android-NDK/actions/runs/38216174983/artifacts/11732488764)
has ZIP SHA-256 `a8dfa65f2684c1d6a1744fd4e796daf730160dd48b00365cc76207ab5d614e12`.
It was independently inspected: 18 host assertions, 27 standalone unpack
observations, nine original IB observations and the deliberately wrong programs
have their expected results; candidate bytes match the emulator-returned copies.
The original IB DEX is 2180 bytes, SHA-256
`25dcf1daee87868e0cea43c3eb254f1e0534880432c254c6e0c97c10243e74d6`.
This prior consumer used IB `446a183e245632608bf2868d4a34eb2f5225bd67` as an unchanged
read-only input. That external success is not automatic acceptance of a later IB head.

## IB-owned execution and acceptance

The Long View workflow now qualifies its own exact checked-out source in order:

1. `verify.grease` executes the production Idriç lifecycle on the host, rejects
   wrong-host/stale-callback/separator production mutations, and independently
   compiles and parses both the text probe and actual LongViewAndroid DEX.
   Backend rejection still fails even if the compiler process exits zero.
2. `compile-task-controls.grease` compiles the two relevant production variants
   already made and rejected by the host suite. Their diagnostic wrapper is
   unchanged, each DEX must differ from the original, and compiler errors do
   not count as a successful negative test.
3. The pinned backend's finalized-layout verifier compares its nine source/encoder
   fixtures with actual DEX and rejects stale plan, stale smali and jointly stale
   reports. This remains compiler-layout evidence, not browser lifecycle evidence.
4. `verify-text-art.grease` retains the earlier separate text probe. In the same
   SDK34 x86_64 emulator, `verify-task-art.grease` executes this branch's actual
   LongViewAndroid and both production variants. It reuses the qualified pinned
   upstream `IBUnpackRunner.smali` solely as an external observer. Original
   execution must exit zero with its success marker; each mutant must exit 73
   with its exact intended assertion failure. Timeout, loader and verifier errors
   cannot count as mutation rejection. Each candidate and runner is pulled back
   and byte-compared before invocation.
5. `require-task-acceptance.grease` independently requires the exact head/backend,
   host and text results, original task result, both mutation statuses/markers,
   source digests and received-byte digests. It writes the final diagnostic
   acceptance receipt only after every condition holds.

The consumer changes require their own exact-head run. At source publication
that new run is NOT_RUN; the backend's completed run is only its starting evidence.
Do not inherit green status from it or rename diagnostic success as full persistence.

## Remaining boundaries

The diagnostic constructs initial task values. It does not yet establish arbitrary
restored task/counter inputs, every renderer transition on ART, platform lifecycle
callbacks, disk reopen, authenticated website continuation, Shizuku, an APK or
physical A1/C67 behavior. The broader stale-callback cases currently remain host
execution, not newly claimed Android coverage. Backend unpack-derived views and
known constructor specialization are not a general list/record heap or dynamic
arbitrary-precision counter runtime.

Retained Longview/Binder/store/worker work under #95–#105/#120 and the other IB
threads remains separately owned. Integrate through their agreed interfaces;
do not create another store, task record, serializer or source model. Preserve
package/signer/version and user data for any future installable replacement.

[Flexible Pipes SUN IB-C01 #76](https://github.com/isomorphisms/flexible-pipes/issues/76)
and [android-NDK #22](https://github.com/isomorphisms/android-NDK/issues/22) retain
the wider unfinished scope. The recorded envelope and draft CI are not registered
FP dispatch, independent ai-ci promotion or merge/release authorization. Bootstrap
build-policy debt remains separate from the successful checked-Dex target tests.

## Preserved failure history

- [Run 37996968726](https://github.com/isomorphisms/ib/actions/runs/37996968726),
  source `a7c6768d9b2c52e728b2fcee91f471cd381a72ad`: host lifecycle and two mutants
  passed; the actual task first failed at `Prelude.Types.fastUnpack`.
- [Run 38103345521](https://github.com/isomorphisms/ib/actions/runs/38103345521):
  the separate checked text probe reached a 149-code-unit jump exceeding 10t.
- [Run 38107554419](https://github.com/isomorphisms/ib/actions/runs/38107554419):
  widened DEX ran the text probe on ART, but retained smali still described a
  short jump; original task compilation still failed at fastUnpack.
- [Backend run 38205035202](https://github.com/isomorphisms/android-NDK/actions/runs/38205035202),
  head `940cf63f655ae9e2031e03a574b2ce22176af155`: one finalized layout repaired
  binary/plan/smali consistency. Its first ART attempt had an emulator-memory
  failure; the same-head second attempt passed. [IB run 38205552843](https://github.com/isomorphisms/ib/actions/runs/38205552843)
  consumed that layout repair but still explicitly recorded BLOCKED_COMPILER_GAP.
- [Backend run 38215145255](https://github.com/isomorphisms/android-NDK/actions/runs/38215145255),
  head `6c41e7febb2950f1f2f1181ee804c66306135972`: original IB diagnostic and two
  production mutants passed their ART conditions, then an undefined environment
  variable broke the final receipt check. The current d0080ff0 run repairs and
  passes that check using an explicit evidence-directory argument.

The earlier [dependency closure](longview-dex-dependency-closure.md) records the
then-first failure and other analyzed reachable forms. Its fastUnpack rejection
is historical for the diagnostic with the new pin, not proof that arbitrary
runtime task values have since become supported.
