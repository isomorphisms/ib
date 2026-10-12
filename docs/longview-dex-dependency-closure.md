# Checked Long View export: observed dependency closure

This is the compiler dependency analysis for SUN IB-C01. It does not change
browser semantics or claim that the canonical task compiled to DEX.

The retained [IB run 38107554419](https://github.com/isomorphisms/ib/actions/runs/38107554419)
used source `a624cef0c1f8b867154e49c7f172f7069b4a3708`, Idriç
`ff4d852862a3942592f8ade9afde8d409d9803be`, and checked DEX backend
`2dd7bc593cee22c94cb3927635e9737e398daf8c`. Its
[artifact 11691015247](https://github.com/isomorphisms/ib/actions/runs/38107554419/artifacts/11691015247)
contains `longview-idric/dex/longview-android.anf` and the original failure log.
The downloaded archive SHA-256 is
`b8584ad3b504632fa079bde1b0e41f53e4a56e2666d8534b9671f23dba94b060`;
the ANF member SHA-256 is
`e19b3db6251e2f326c5252443ba6ce07ce5d517ce6c6f9636170f582efff25a3`.

The actual first rejection is:

```text
Error: dex rejected checked program: No checked helper body for DEX call Prelude.Types.fastUnpack
```

The compiler returned zero while printing that error. The maintained verifier
rejected the diagnostics; its result receipt now records
`stage_status=BLOCKED_COMPILER_GAP` and the exact error before exiting 1.
A zero process status must never establish target compilation.

## Root and reachable forms

The root is the actual five-Text-argument `longview_host_generation` export.
This inventory follows its named calls, known captured continuations, and
constructor/primitive uses in the retained ANF. Status after the first error
is **source analysis**, not an observed later compiler failure. The newer
layout repair does not modify `Lower.idr` or these language boundaries.

| Reachable definition or form | Required behavior | Existing target boundary |
| --- | --- | --- |
| `LongViewAndroid.longview_host_generation` | Project invalid input to -1; otherwise project host generation | Text/Int32 export ABI exists; the body fails specialization |
| `LongViewAndroid.resume_task` and `{resume_task:4}` through `{resume_task:0}` | Validate all five identities, bind each successful result, construct the real task and dispatch host change | Acyclic checked calls and statically known captured continuations have specialization support; this is not runtime closure support |
| `Prelude.Types.>>=` | Propagate Nothing or invoke the known continuation on Just | Known constructor cases can be specialized; arbitrary escaping runtime sums have no qualified ABI |
| `IB.LongViewTask.durable_identity` and its case block | Require nonempty text and refuse tab, LF and CR | First actual rejection occurs at its `fastUnpack` call |
| `Prelude.Types.fastUnpack` | Convert dynamic text to its actual `List Char` | Retained foreign definition declares Scheme/RefC/JavaScript only, no checked DEX body or convention; adding a scalar foreign declaration cannot supply this list result |
| `_builtin.NIL`, `_builtin.CONS`, `_builtin.NOTHING`, `_builtin.JUST` | Represent and inspect the dynamic character list and validation result | Known source constructors can be propagated internally; dynamic character-list allocation, fields and tags remain a separate missing runtime facility |
| `IB.LongViewTask.contains_record_separator` | Scan every character and reject any record separator | Explicit self-recursion is present; `specialize_with` refuses recursive helpers. Character constant cases are also outside the qualified Int32/Text slice |
| `IB.LongViewTask.identity_text` and `Prelude.Types.fastPack` | Recover the validated identity without changing characters | The ANF erases `IdentityBeginning` to a CONS value. `fastPack` accepts that list and has no DEX foreign convention/body in the retained ANF |
| `IB.LongViewTask.start_protected_task`, `rebuild_task`, `ProtectedLongView` | Construct/rebuild the existing 17-field task, retaining identities and uncertainty | Statically known constructor projection is supported. This does not establish a general runtime task-record representation or an escaping record ABI |
| `IB.LongViewLifecycle.open_in_host` and the Text instance of `Prelude.EqOrd.==` | Distinguish same-host recreation from a different host | Checked Text equality is implemented; it still depends on the missing identity packing path |
| `IB.LongViewTask.activity_was_recreated` and `host_process_restarted` | Preserve host generation for the same host, advance it for a new host, advance activity generation | Retained ANF contains arbitrary-precision `+Integer` operations and the export's `cast-Integer-Int32`; current lowering qualifies Int32 arithmetic, not these operations. No narrowing substitution is accepted |

The ANF dump also contains compiler/main scaffolding and library definitions
such as Integer comparison and subtraction. Their mere presence is not proof
that this export calls them. The closure above does not count uncalled dump
entries as target obligations.

## Further product obligations

`neutral_address`, renderer callbacks and serialization are not called by this
diagnostic export. Passing it would not cover their semantics. C01's eventual
acceptance still needs real neutral-address redaction, stale/wrong-generation
renderer refusal, session and heap uncertainty, and the existing host mutants.
Neither those obligations nor the full application can inherit the separate
String.indexOf probe's ART success.

The artifact independently records:

- actual host lifecycle success and rejection of wrong-host, accept-stale and
  identity-separator mutants;
- exact source-compiled text-presence DEX executed on Android 14 x86_64 ART;
- canonical task DEX rejected at `fastUnpack`, with no full-task ART execution.

The next language facility belongs in the reusable checked backend: a real
dynamic text/character-list boundary and lowering of the checked recursive scan,
with nonempty/tab/LF/CR and character-preservation controls. After that, run the
unchanged canonical export again to expose the next actual error. Do not report
the analyzed later gaps as executed failures or replace the task with a constant
scalar, another browser model, Java, smali, or host execution.

Owner: [android-NDK issue #22](https://github.com/isomorphisms/android-NDK/issues/22).
Consumer: [isomorphisms/ib PR #134, Move Long View lifecycle dispatch into Idriç and exercise its direct-DEX path](https://github.com/isomorphisms/ib/pull/134).
Envelope: [Flexible Pipes SUN IB-C01 #76](https://github.com/isomorphisms/flexible-pipes/issues/76).
No registered Flexible Pipes dispatch, independent ai-ci promotion, merge, APK,
Shizuku, disk recovery or physical-device acceptance follows from this analysis.
