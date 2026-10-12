# Program attempt: first useful Pensieve projection

Status: SOURCE_ATTEMPT; compiler checking and execution NOT_RUN

## Purpose

IB-D07 / issue #118 projects the existing S08 retained-item reducer into a
small source-linked list. An item explains why it appears and offers explicit
actions without requiring manual re-entry of its source content. This is the
independent presentation/command slice, not a native Android screen or store.

## Type-system sketch

`SourceReference`, `RetainedActivity`, `Candidate`, `Decision`, `human_action`,
`same_evidence`, `useful_candidates` and `append_decision` remain owned by
`IB.HumanDecisions`, parent `cb8cdc14b6065b91240b97b580055c0d18375fae`.

`SourcePresentation` is ephemeral, source-owner-supplied title/excerpt or an
unavailable descriptor. Its evidence is the existing source/target/revision
reference, not a title, URL, array position or another canonical identity.
It does not own source bytes, extraction, navigation, retention or policy.

`HomeBudget` bounds output rows and decoded-character presentation lengths.
It does not claim to bound the existing reducer's input processing or RAM.
`HomeRow` carries the actual core candidate, its explanation and a checked
display. `HomeView` is disposable and reconstructed from committed decisions.

`project_home : HomeBudget → List SourceReference → List Decision →
List RetainedActivity → List SourcePresentation → Either home_refusal HomeView`

`prepare_item_decision : HomeBudget → List SourceReference → List Decision →
List RetainedActivity → List SourcePresentation → Candidate → Text →
human_action → Either home_refusal Decision`

`prepare_source_open` returns the existing exact `SourceReference` after
checking the current displayed candidate and readable presentation. The real
source/navigation owner must recheck permissions when executing it.

## Invariants and effects

The view consumes S08's priority and suppression unchanged. No recommendation
feed, inferred human decision or duplicate notes store is introduced. Exact
source/target/revision plus activity identity bind row actions; recycled/stale
rows cannot mutate another item. Missing/denied, ambiguous or oversized
presentation produces a generic unavailable row and carries no old title or
excerpt. Distinct source identities remain distinct despite equal titles.

All functions are pure. Preparing a Decision is not a disk commit; reproject
from the actual committed journal only after the existing store reports its
result. Pending, failed and unknown feedback cannot say the decision was saved.
The source owner supplies already admitted references and already permitted
presentation bytes. This layer authenticates neither arbitrary callers nor
file hashes and does not perform acquisition, decoding or persistence.

Edit is not implemented: S08 has no accepted source-edit command yet. No text
box may pretend to persist an edit through a missing semantic owner. Keep's
optional external action remains D10, not an automatic side effect here.

## Idriç attempt and execution boundary

Canonical style was read at compiler source
`ff4d852862a3942592f8ade9afde8d409d9803be`. Source:
`src/IB/PensieveHome.idric`; cases: `PensieveHomeCases.idric` beside this file.

This fresh disposable host has no `idric`, `idris2`, Grease or Chez executable.
The previously recorded compiler path was wiped. Available Idriç releases
contain Android ARMv7 packages, not the required Linux x86_64 checker. The
latest inspected Linux build artifact contains XBRL acceptance evidence, not
a compiler distribution. No unqualified bootstrap or different language is
used to turn this attempt into a passing receipt.

Actual checking attempt/result is recorded below after invocation. The
expected acceptance cases remain NOT_RUN until the actual compiler evaluates
them. Writing expected True values is not evidence that they evaluate True.

Actual command, with the verified IB checkout as its working directory:

`idris2 --no-color --source-dir src --check src/IB/PensieveHome.idric`

Result: exit 127, `idris2: command not found`. No source/typechecking was
performed. `git diff --check` passes. The existing public-vocabulary linter
passes separately from the verified repository root; it is a text check,
not parsing or execution of Idriç. The first linter invocation from `/tmp`
correctly refused its missing relative `src` input; it is not a passing run.

## Fallback and language work

Fallback: none. First local boundary is access to the qualified Idriç checker
and evaluator. Then source-check the module/cases and evaluate `home_cases`.
Run targeted mutants removing row identity/revision comparison, revealing a
denied presentation, and reporting save after persistence failure. The planned
golden cases must reject those actual production changes before acceptance.
Both supplied mutation patches pass `git apply --check` against this source.
Their first preparation attempt had insufficient patch context; corrected
patches apply cleanly. This is patch preparation, not semantic rejection.

Native Material view construction/callbacks, the real retained-source producer,
S08 durable execution, view loss/reopen, APK/signing and physical phone
usability remain separate incomplete D07 boundaries.
