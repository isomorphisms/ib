# Pensieve projection: checking and mutation receipt

The actual maintained checker/evaluator now runs. These results qualify the
pure source projection; they do not establish a generated consumer, durable
store, Material screen, APK or physical-device behavior.

## Exact inputs

Core source: `ff4d852862a3942592f8ade9afde8d409d9803be`, clean tracked tree.
The rebuilt compiler reports `Idris 2, version 0.8.0-ff4d85286`.

| Input | SHA-256 |
| --- | --- |
| Rebuilt compiler `idris2.so` | `e235a25c6b9c2218c57f8b256cad7d6036b568603db21c8a1d709af97fc25818` |
| Threaded Chez portable runtime | `d5bc72667ad1718edba05a25cf8f31930c6d5dd279368c1c02c479f9fe8575c4` |
| NDK-built compiler C support | `fc6939f9232ffa519f65dd35e55980dc0612649c14c5a48c2d5775b9f0ecd3c3` |
| Production `IB.PensieveHome` | `aebb11803591529605f58adbdbcea4c3c266e96292923173c5646361cc71d7db` |
| Existing S08 `IB.HumanDecisions` | `c7dffb3335ea81c048d82b8bc0b7831a0ea1f03e9f1ed43656011d73103e1bd5` |
| Cases after dated-comment correction | `d08c6f6db0dd313427f0c49965daf6e79c2838f93fb30ada2d523c5d8f544675` |

The C support stage uses NDK r29, version `29.0.14206865`, host
`x86_64-unknown-linux-gnu` clang, llvm-ar and llvm-ranlib. ICK source inspected
at `143e29580c2644ea0f345fe609fa5673d036865d` supplies no qualified installed
host compiler for this runtime stage: `gap:host-runtime-stage-qualification`.
The inherited Scheme bootstrap is a checker dependency, not a selected IB
consumer backend or whole build-policy qualification. RefC/GMP support was
neither built nor installed.

The initial unavailable-command result (127) remains historical. Restoration
found three actual bootstrap issues: the portable runtime first lacked usable
foreign-call support; Linux's `libc.so` was a linker script; and the final
compiler subprocess failed after emitting all 290 checked compiler modules.
A verified threaded runtime with foreign calls was copied into this checkout,
a private libc alias selected the real ELF library, and the compiler's own
generated `compileChez` program compiled successfully when invoked directly.
The existing generated launcher was bound to that rebuilt `idris2.so` rather
than the inherited `idris2-boot.so`. Version and source checks distinguish the
two. No failed full-bootstrap invocation is relabeled as a clean full build.

## Observed results

| Control | Source/checker status | Actual evaluator result |
| --- | --- | --- |
| Production module | 0, no error diagnostics | Typecheck passes |
| All 26 production cases | 0, no error diagnostics | Every named result `True` |
| Row-binding production mutant | Entire case module checks, status 0 | Existing row-reuse predicate evaluates `False` |
| Failed-save production mutant | Entire case module checks, status 0 | Feedback is `"Decision saved"`; exact failed-save assertion evaluates `False` |
| Clean isolated recovery | Entire case module checks, status 0 | All 26 named results `True` again |

Mutation evaluation is not a compiler-error test. The compiler evaluator
returns process status 0 even when the assertion is `False`; the semantic
result violates the same original acceptance requirement. The original two
patches were applied only in isolated source trees, never to delivered code.
Both the production and recovery matrices contain exactly 26 named results.

The normalizer was invoked through the actual compiler's `--client home_cases`
command after checking the production module and loading that exact TTC cache.
The isolated trees contain byte copies of the two actual production modules
and the case source; each has its own build directory. The recovery matrix is
preserved in `recovery-cases.txt` beside this receipt. The two targeted mutation
results are preserved in `mutation-results.txt`.

One direct interactive attempt using `≟` failed because this inherited REPL
command interface does not apply the `.idric` source spelling pass. That is
recorded as a failed diagnostic, not a mutant rejection. The compatibility
REPL equality spelling then evaluated the same failed-save predicate to
`False`; maintained `.idric` source retains `≟` throughout.

## Remaining product boundary

The retained-activity/permitted display producer, S08 durable executor and
reported write failures, native Material rendering/actions, Edit owner and
physical first-open acceptance remain separate work. Journal encode/decode
and replay here prove pure framing/reconstruction, not disk durability.
