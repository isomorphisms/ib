# Source reflow with measured code-point advances

Purpose: make retained real documents readable without repeated whole-source
indexing and unbounded full-paragraph candidate measurements.

Type sketch: the retained source remains Text; SourceBreak and SourceLine keep
decoded code-point coordinates. The additional provider is Char → Number:
nonnegative additive advances, not an arbitrary Text → Number callback.
MeasuredCut caches only derived prefix widths and a suffix of that same source.
The canonical source, admission evidence, selection coordinates and Arena
identity do not change. No additional browser or storage model is introduced.

Observed failure: ai-ci f07e4f0901ebca358d497cce75ce495a98977924 run 41311555347,
API 29 job 124862837322 and API 34 job 124862836989, executes the real unchanged
A734 reader consumer through android-NDK 150ae18dcac6e97e6bb5150e868929245f133594.
The independent 12000-deep non-tail/list/mutual controls and sibling calls pass,
and four source mutants are rejected. The 231-byte original passes; the
8407-byte case exceeds the unchanged 180-second limit. Samples map to repeated
SourceBreaks indexing/length validation and Source source slicing, filtering
and exhaustive measurement. No larger case completed in that replay.

Implementation: validate ordered cuts and every source boundary together;
retain the old general callback API and its exhaustive semantics; give the
reader an explicit additive provider and stop only when the nondecreasing base
advance excludes every later cut. A soft-hyphen glyph is excluded from that
pruning bound, since it can disappear at a later cut. Both the Greedy and
Squared objectives retain costs, tie ordering and exact source offsets.
Only selected/source-local spans are materialized.

The Android provider measures individual code points through the immutable
Paint snapshot. Its identity now says so. This approximates whole-string
shaping; it does not promise kerning, bidi shaping or full browser typography.
The existing whole-painted-line viewport check still refuses overflows.
There is no fallback to a different measure, renderer, network client or
unchecked source.

Regression: compare both implementations over nine texts, widths 1–8, six
objectives and three advance providers, including zero advances, unequal
spaces/hyphens, combining text, emoji, CRLF, soft hyphens and trailing spaces.
Compile and independently decode the actual command and three source mutants:
missing mandatory boundary, premature candidate pruning and corrupted source
coordinates. Require exact True/False results and normal semantic exit status.
The independent ai-ci original-byte consumer still runs 231/8407/31507/84007
bytes under the same 180-second deadline with exact source text and an
admission mutant.

Status at authoring: source implementation only. New checked compilation,
ART, installed-reader and physical acceptance remain NOT_RUN. Historical
passes do not qualify these changes. The TAB_P10 connection remains BLOCKED.
