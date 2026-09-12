# Exact interior-text search

IB now has a deliberately plain Idriç semantic reference for exact substring search in `src/IB/ExactText.idric`.

This is the operation that PR #9 described only as a future "grep-like pass". It is unrelated to PR #17's exact vector scan: that scan walks a 10,000 × 384 Float32 vector index and computes dot products, while this one walks ordinary text looking for an exact literal.

## Semantic reference

The portable reference is intentionally boring:

1. test whether the wanted characters are a prefix at the current position;
2. if not, move forward by one character;
3. repeat until a match or the end.

`contains_string` short-circuits on the first match. `match_positions` records every zero-based character position, including overlaps, and `match_count` is its length. The empty pattern matches every character boundary, including the end.

That source remains the oracle even if production lowering later uses a completely different algorithm.

## First deterministic fixtures

`ExactTextSmoke.idric` fixes the cases we need before optimizing anything:

- absent match;
- beginning, middle, and end matches;
- overlapping matches (`aba` in `ababa`);
- one-character pattern;
- repeated-prefix/adversarial input (`aaaaab` in `aaaaaaaaab`);
- empty-pattern behavior.

The first ARM benchmark should stay ASCII so byte offsets and character offsets coincide. Unicode text can be added after the backend API says explicitly whether it returns byte offsets, character offsets, only a Boolean, or a count.

## Compiler/backend experiment

The architectural question is not whether C is slow. A competent C/library implementation is a required baseline.

The question is whether preserving the semantic operation long enough lets a target backend choose a better realization than flattening the request immediately into generic character iteration and nested branches.

For ARM/Thumb, candidates include:

- ordinary scalar comparison;
- short-literal word comparisons;
- packed Shift-And/Shift-Or state when the literal fits the usable machine word;
- candidate-byte filtering plus verification;
- table/state-machine dispatch where that shape actually wins;
- NEON/SIMD only on targets where it is available and profitable.

The corresponding compiler-design note is `isomorphisms/Idric#21`. ARM/Thumb experiments are tracked in `isomorphisms/idric-arm-thumb#9`, `#10`, and the IB-workload bridge `#11`. Browser-side tracking is `isomorphisms/ib#33`.

## Current backend boundary

The current ARM/Thumb backend is still a runtime-free straight-line numerical leaf backend. PR #8 deliberately adds branch/dispatch fixtures but does not yet add branches, loops, byte-buffer input, Bool/integer returns, strings, or general runtime support.

So this branch makes the browser semantic workload executable first. The ARM implementation should be stacked separately and should not fake success by replacing the browser oracle with a backend-shaped toy.

## Evidence rule

For every backend realization, keep together:

- the exact source fixture and literal/input bytes;
- semantic result, match positions/count where applicable;
- emitted assembly and object/disassembly;
- code size;
- bytes scanned;
- setup/table cost separately from steady-state scan;
- wall time/cycles on representative hardware;
- target identity and SIMD capability;
- a competent ordinary implementation as the comparison point;
- non-wins and regressions as evidence.

This gives IB a real browser workload and gives the compiler work a before/after benchmark without making unusual instructions the goal by themselves.
