# Typed Prepaint to source-bound reflow

## Purpose and source boundaries

Join existing typed Prepaint revisions to the existing Unicode source-bound
reflow implementation. This is derived layout; the revision and each original
information block remain unchanged. A block position identifies an occurrence
only within the retained immutable revision, not across different revisions.

The base is IB HTML-to-Prepaint commit
`33191f743e2f3bc51ba950f499187db595d43ec2`. Reflow modules and their existing
smokes are reused unchanged from commit
`4a7380c5f25409443c5e8c1ae17bd358372012fc`. No line-breaking algorithm is added.

## Values, operations, and invariants

- Input: an existing `PrepaintRevision`, `ReflowRequirements`, a nonempty
  measurement identity, and its `Text → Number` width callback.
- Output: either a refusal or a derived layout retaining that exact revision,
  requirement, measurement identity, original blocks and block positions.
- Text headings, text paragraphs and link labels receive per-block
  `SourceLine` plans. Table rows, forms and images remain explicitly unreflowed;
  their semantic payloads are not flattened into invented text.
- Offsets count decoded Unicode code points in each block's text. They are not
  original HTML byte offsets, UTF-16 offsets, glyph positions or pixels.
- The HTML-result entrypoint retains the complete `HtmlPrepaintResult`,
  including `HtmlEvidence`. Version 1 wire bytes alone do not authenticate the
  Arena source generation or digest.
- Width zero, absent measurement identity and unsupported source-bound
  streaming/typographic modes must refuse. Existing no-split and source
  reconstruction rules remain authoritative.
- All operations are pure. No I/O, networking, storage, native shaping, device
  installation or renderer claim is introduced.

## Intended signatures

`reflow_prepaint : ReflowRequirements → Text → (Text → Number) →
PrepaintRevision → Either Text PrepaintLayout`

`reflow_html_prepaint : ReflowRequirements → Text → (Text → Number) →
HtmlPrepaintResult → Either Text HtmlPrepaintLayout`

## Compiler and verification

The available Linux x86-64 compiler bundle identifies Idriç
`94dfd99bd3e376507fedc8611053b7173b2519f0`, Chez 10.4.1, producer run
`38090627721`, attempt 1. Use its actual `bin/idric` entrypoint and preserve
the first compiler failure rather than substituting another runtime.

Verification must execute the real typed HTML converter and adapter, compare
known Unicode/width line plans, reconstruct exact decoded text through the
source spans, retain wire/source/revision/block identity, and exercise refusals.
Source-only or host execution does not prove native rendering or physical C67.

## Result

Implemented `IB.Prepaint.Reflow`. The adapter never reparses V1 wire output.
It retains the input revision and original blocks, records the measurement
identity, and delegates break selection to the unchanged source-bound solver.

Actual host verification used a scratch copy of `src/` at
`/workspace/scratch/22d72635f498/u3-source-check`, with the canonical
`PrepaintReflowChecks.idric` copied into that source root. The compiler entrypoint
was `/workspace/scratch/58fb4da72caa/tools/idric/idric-linux-x86_64-94dfd99bd3e376507fedc8611053b7173b2519f0/bin/idric`;
its arguments were `PrepaintReflowChecks.idric -o prepaint-reflow-checks`.
Then the generated `build/exec/prepaint-reflow-checks`
was executed. It printed `prepaint-source-reflow-cases=True` and exited 0.
The existing `SourceReflowSmoke.idric` was separately compiled and executed;
it printed `source-reflow-unicode-offset-cases=True` and exited 0.
`sh tests/idric-public-vocabulary.sh` passed.

The first check compile exposed a missing `Data.Maybe` import in the new
check program; adding the actual library import fixed it. No language or
runtime fallback was used. These are Chez host semantic results, not DEX,
Android layout, pixel/shaping or physical-device results.
The check supplies an admission-evidence fixture to the real typed converter;
it does not itself reopen or authenticate an Arena range.

## Remaining language or integration work

This slice does not make the whole-document HTML ingestion path bounded, remove
its existing 4 MiB admission ceiling, implement source-bound streaming or
typographic shaping, serialize source evidence into V1, or connect Android views.
The first integration boundary is the native view consumer supplying its real
measurement callback plus versioned measurement identity and consuming these
per-block plans. Acceptance must independently compare its copied/source text
and actual measured line geometry on the identified target.

## Independent Moon checks

The independent check program consumes the existing immutable revision and
derived layout. Its input domain includes ordered mixed semantic blocks,
Unicode decoded text, source/revision identities, line-codepoint intervals,
measurement identities and explicit reflow requirements. Its output is a
Boolean acceptance result with a failing process status for any mismatch.
It reconstructs decoded source directly with `take`/`drop` over Unicode code
points, independently of `source_for_line`. It compares serialized original
blocks and explicit source identity separately because V1 does not serialize
the source reference. It must reject a dropped/reordered block or changed
source/revision/metric, and preserve table/form/image payloads without
claiming those payloads received a text layout. Its only effects are checked
host compilation, output and exit status; it neither authenticates Arena
bytes nor performs native font measurement.

Moon verified exact Earth commit
`d3d6c710a9b29ea501eb12fca97888804c515cd2` in a separate worktree.
The reused reflow paths match their declared `4a7380c5...` source exactly.
The compiler bundle's complete `payload.sha256` verification exited 0 and
`receipts/identity.txt` identifies `94dfd99b...`, Linux x86-64, Chez 10.4.1,
producer run 38090627721 attempt 1.

In a fresh staged source directory, the exact `bin/idric` entrypoint compiled
`PrepaintReflowChecks.idric`, `PrepaintReflowMoonChecks.idric`, and
`SourceReflowSmoke.idric`. Each generated executable was launched through the
same bundle's absolute `bin/idric-env` entrypoint. The outputs were respectively
`prepaint-source-reflow-cases=True`, `prepaint-moon-independent-cases=True`, and
`source-reflow-unicode-offset-cases=True`, all with exit 0. The independent
program covers mixed heading/text/link/table/form/image blocks, exact Unicode
slice coverage, Han, CRLF, SHY, ZWSP, NBSP, combining marks, an overwide emoji
ZWJ sequence, source/revision changes and width/manner/measurement records.

A separate scratch production mutant changed the final adapter construction
to use `reverse prepared`. The same independent check compiled successfully,
printed `prepaint-moon-independent-cases=False` and exited 1. The maintained
source was unchanged. The program also rejects dropped or reordered plans
and a stale source or revision directly. Public vocabulary and whitespace
checks passed. The first new fixture compile rejected the reserved word
`total` as a parameter name; renaming that fixture parameter fixed it.

These are independently reproduced host semantic results. They do not qualify
bounded ingestion, native font measurement, Android/ART, Arena authentication,
or physical C67/A1 execution.
