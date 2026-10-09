# Long View: restore the chosen Idric / Android boundary

The 2026-10-09 C67 test exposed a startup error in the Java Long View harness.
The subsequent Java fix in PR #133 does **not** repair the architectural error:
that harness implements browser task transitions and serialization separately
from Idric. Do not deliver it as acceptance of the intended IB implementation.
Keep its source and physical failure as regression evidence; do not ask the
user to clear data, uninstall, or repeat that harness test for this migration.

## Implemented first slice

`IB.LongViewLifecycle` receives host/renderer observations and dispatches to
**existing `IB.LongViewTask` transitions**. No second task record, serializer,
storage engine, or Android-specific browser model is introduced. It distinguishes
same-host Activity recreation from a new host and refuses renderer observations
for another task, tab, navigation, host, host generation, renderer, or renderer
generation. A duplicate renderer-loss notice cannot become another transition.
A normal foreground return is not an Activity creation and must not call
`open_in_host`.

`LongViewAndroid.idric` supplies runtime identity inputs to that same core and
exports a host-generation observation through the **existing checked direct-DEX
backend**. This is a backend consumer probe, not an Activity or an APK.
`LongViewLifecycleCheck.idric` exercises the actual production modules and
checks both useful operations and stale/invalid inputs. The Grease verifier
executes deliberately broken host-dispatch and stale-callback versions and
requires their assertions to fail, after successful compilation.

## Existing owners and pins

This repair is based on PR #132, **Verify Pensieve Quick Settings bindings through
direct DEX**, head `edeb7a3b91c821910e5524002f8548ec2052dd42`. It reuses:

- `src/IB/LongViewTask.idric`: task identity, transitions, ordinary serialization;
- `vendor/android-NDK@7ea62a3e3cd19f2ab04a27f72ace1ae9e8b1fe66`: checked DEX driver;
- `isomorphisms/Idric@ff4d852862a3942592f8ade9afde8d409d9803be`: actual compiler;
- the existing Cat Food/Grease bootstrap entry points and compiler API cache.

Retained Longview/Binder store and worker work remains under #95–#105 and #120.
Do not copy those implementations into this probe or replace Pensieve with a
new viewer. Material design remains a reference, not permission to import
Compose, Gradle, Java application source, or duplicate browser logic.

## Evidence boundaries

The workflow attempts these stages in order, preserving their separate logs:

1. Actual Idric source compilation and host execution, with failed-mutation
   controls. Chez is diagnostic host execution, not an Android implementation.
2. Actual checked source-to-DEX generation and independent AOSP DEX parsing.
   Missing/unsupported target code is a failure, never a successful fallback.
3. **Not implemented here:** ART execution of this consumer, Activity callbacks,
   visible rendering, disk recovery, Shizuku calls, APK packaging, physical C67.

At initial publication both compiler execution and DEX generation are **NOT_RUN**.
An empty `main`, emitted DEX, successful framework-binding fixture, or a Java
APK is not evidence that the Idric browser runs on the phone.

## Remaining work before another IB phone test

Wire the actual Idric task state to generated Android lifecycle callbacks and
views; use Grease and the existing store boundary for I/O, with restored records
validated in Idric. Keep JNI/NDK only for facilities that need that boundary.
Port the observed linked-system-ancestor counterexample into that real store
adapter without weakening refusal of links in browser-owned descendants.

Then prove source-to-artifact provenance, real event/state/visible-output
relationships, durable reopen after host death, denial handling, and independent
failure controls. Preserve the established package, persistent signer, and
nondecreasing version code. Do not call a replacement phone-ready until its
intended implementation actually executes in the delivered artifact.
