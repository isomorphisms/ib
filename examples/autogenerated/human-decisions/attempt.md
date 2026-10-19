# Program attempt: human decisions

Status: PARTIAL — pure core/typechecker/evaluator exercised; disk journal pending

The adjacent `HumanDecisionCases.idric` preserves the failed equality-proof
attempt. Imported private helper implementations are opaque to definitional
equality; that attempt was refused by the real compiler. It is not a passing
test or a language fallback. The maintained executable assertions live in
`src/HumanDecisionCases.idric`; the real compiler evaluator exercises them
without exporting every implementation helper as a public proof interface.

## Purpose

SUN IB-S08 implements the core boundary used by issue #103 (overlapping
organization) and issue #118 (useful retained items). The adapter supplies
already retained source/activity references; this module does not fetch,
decode, infer labels, or write source bytes.

## Type-system sketch

Values: stable target identity/kind, immutable retained source revision,
proposal/model/policy identity, human decision identity, category, and activity
identity. Identity is not URL equality or list position.

`validate_proposal : List SourceReference → Proposal → Maybe Proposal`
checks exact available source evidence and a restricted proposal value.
`append_decision : List SourceReference → Decision → List Decision → Maybe (List Decision)`
adds an immutable human action, rejecting identity collision, stale evidence,
and invalid category/action combinations. Replay of identical records is
idempotent; a conflicting record is refused.
`memberships_for : Target → List Decision → List Text` keeps categories
independent. Attention actions do not create labels.
`useful_candidates : List SourceReference → List Decision → List RetainedActivity → List Candidate`
is a transparent projection: save > defer > return > exposure. Explicit
dismiss/done decisions suppress the same evidence revision. Rejection does not
invent a negative label; a new human keep may reverse a dismissal.

Effects: the core is pure. A bounded Grease journal adapter owns reading,
atomic append/replay and durability. Native views only project candidates and
submit explicit actions. No UI, Keep write, model execution or private corpus
ingestion is included.

Laws: category B add/remove cannot change category A; attention cannot train;
later proposals cannot override human decisions; stale revisions cannot mutate
current targets; rollback is another identified action, not deletion.

## Idriç attempt

Source: `src/IB/HumanDecisions.idric`, with executable examples kept beside
this file. Base: IB main `10fa7bbe93ff9ffa1ba7b9f62e0dd8f6c10a86bc`.
Canonical style inspected from isomorphisms/Idric `Idriç` and clean local
`ff4d852862a3942592f8ade9afde8d409d9803be`. Actual `--check` on
`src/HumanDecisionCases.idric` succeeds. Actual compiler REPL evaluates
`all_cases` (the 20 core assertions and all codec/append/refusal/recovery assertions) to
`True`; this is compiler-evaluator evidence, not a generated executable.
Changing `dismiss_item ⇒ True` to `dismiss_item ⇒ False` makes
`all_expected`, `same_evidence_is_suppressed` and
`unchanged_dismissal_cannot_reappear` evaluate `False`. Restoring the source
restores `all_expected = True`.

## Runtime/backend boundary

The repository's inherited Chez compiler pin does not have a declared qualified
ICK/NDK consumer stage. The clean current compiler supplies real typechecker
and evaluator evidence only. A generated semantic validator executable and its
bounded Grease disk journal adapter remain blocked on that qualified stage.
No stock Idris, generated C, Python or Java substitute.

## Fallback

None.

## First language/build work

Qualify the maintained compiler's check and execution path for current Text,
Number and ≟ syntax under the shared build-toolchain contract, then execute
the decision-sequence and journal refusal/recovery fixtures at the published
IB head. Journal integration must remain blocked until the core validator is
actually executable.

## Evidence boundary

Typechecking and compiler evaluation are established. Disk persistence,
filesystem crash recovery, generated-executable execution, automatic CI,
Android durability and physical-device usability remain NOT_RUN. The encoded
commit marker proves framing/record count only; it does not authenticate bytes
or establish fsync behavior.
