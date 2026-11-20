# Optional cuts adjoining combining marks

The source break generator could emit an optional cut before a combining mark
or word joiner. The independent boundary validator then refused the complete
paragraph, including a viewport wide enough to display it without wrapping.

## Types and invariants

The maintained operation remains `source_breaks : Text → List SourceBreak`.
`safe_boundary : Char → List Char → BreakKind → Bool` remains the single
boundary policy shared with validation. `Text` is decoded source; offsets count
Unicode code points. Neither painted spelling nor Android UTF-16 units defines
the source coordinates.

Suppress an unsafe *optional* candidate at generation. Do not suppress a
mandatory line ending, weaken validation of externally supplied cuts, remove
source characters, or invent a split inside a combining sequence. Both the
general callback and additive-advance solvers consume the same corrected cuts.
This retains the existing bounded Unicode tailoring; it does not claim complete
Unicode line-break or grapheme segmentation.

## Qualification

Extend the maintained `SourceReflowSmoke` consumer with SPACE, SOFT HYPHEN and
ZERO WIDTH SPACE adjoining combining marks, and a discretionary cut adjoining
WORD JOINER. Require exact one-line painted text at a wide viewport, faithful
original-source recovery, absent unsafe generated cuts, and refusal of the
same cuts when explicitly supplied. The existing compiled boundary mutation
must remain rejected. Host source execution, DEX generation/decoding and ART
execution are recorded separately; physical tablet acceptance remains blocked.

No fallback implementation or new runtime primitive is needed.

## Observed local result — 2026-11-01

Using frontend `740bbf2b1d9e811fd00c719c3473eede17ceea3b` and backend
`0f69ef2019164710b66f5c323fdd3e815d6578ef` on the disposable x86_64 host:

| Stage | Result |
|---|---|
| New cases against the original generator | FAIL as intended: `source-reflow-unicode-offset-cases=False`, exit 1. |
| Complete maintained regression after the repair, checked host execution | PASS: `source-reflow-unicode-offset-cases=True`, exit 0. |
| Restore unsafe generation with `if True` at the optional-boundary guard | REJECTED: the same regression reports False and exits 1. |
| Checked DEX emission, repeated fresh output | PASS, byte-identical SHA-256 `8f4a12d6ea023b8528c83c21221166a64ecfd5fe036898b671affb4ae2673c7c`. |
| Independent baksmali 3.0.10 decode | PASS; oracle JAR SHA-256 `37ae4a41a8886e15c20b8362fa4250f96bbdb55e1a608199ad8b5dff068b588f`. |
| Public Idriç vocabulary / Grease parse / diff whitespace | PASS. |
| Changed-source ART, installed reader, Binder/ICU/Arena, and tablet | NOT_RUN locally. The maintained API 29/34 workflow includes this consumer and its new targeted generation mutant. |

The checked host execution is a pure reflow regression, not an alternate reader
runtime or Android acceptance. The existing foreign-boundary, source-admission,
mandatory-break and source-coordinate checks remain required.
