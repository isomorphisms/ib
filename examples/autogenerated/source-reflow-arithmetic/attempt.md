# Exact saturating differences for source positions

The advance-based reader at a0130375d15a46d72f8e4262385315a12a005b03 improves
the independently observed 8407-byte projection on API34 to 4.406992614 seconds
and 31507 bytes to 55.196061023 seconds, but the full 84007-byte target still
exceeds the complete unchanged 180-second deadline. This remains FAIL.
Source-bound android-NDK run 41313837280, API29 job 124869623760 and API34 job
124869624015, maps repeated samples to the portable unary
IB.Reflow.Arithmetic.nonnegative_difference during prefix-width subtraction.

Taking 7000 from 7001 should produce one without walking through 7000
predecessors. The maintained Number minus operation has exactly the same
saturating meaning as the existing helper, including zero when the amount
taken is greater. The pinned Idriç frontend
740bbf2b1d9e811fd00c719c3473eede17ceea3b, Compiler/Opts/Constructor.idr,
magic__natMinus and natHack, explicitly lowers that operation to integer
subtraction plus prim__integerToNat saturation. This is implementation-source
evidence, not a runtime qualification.

Type sketch: Number → Number → Number, unchanged. The public semantic helper
now delegates to that maintained operation. No signed subtraction on Number,
new foreign primitive, width truncation or backend-dependent numeric result
is introduced. Both greedy and squared layouts retain the same source ranges,
costs, breaks and exact selection meaning.

The slow structural definition is retained only as a bounded test oracle:
all pairs from zero through 64 compare equal. Literal 31-digit values check
one apart, underflow, equality and subtraction of zero without a machine-width
assumption. The actual exported source-reflow regression command still runs
all Unicode/source/reference/advance cases and its compiled mutations.

The accompanying compiler pin 0f69ef2019164710b66f5c323fdd3e815d6578ef reuses the
already qualified managed export bridge for optimized checked command World
closures. The separate framework Activity and pure-export entry contracts are
preserved. Prior producer failures 37edf48 and 6a14667 explicitly refused the
new command shape, with no stale DEX accepted.

Status: source change; new head emission/ART/large-body/installed acceptance
is NOT_RUN at authoring. No timeout, body target or negative control is
relaxed. TAB_P10 physical access remains BLOCKED.
