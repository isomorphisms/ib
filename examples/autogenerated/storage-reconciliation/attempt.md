# Storage reconciliation attempt

Purpose: consume existing versioned IB app/shell capability rows in Idriç and
feed the existing `IB.StorageAdmission` publication policy without promoting
shell visibility or `canWrite` to an app write proof.

Values: five-column capability rows; observed installation/source/time/media
generation/authority; wide nonnegative byte quantities; selected app root and
known capacity-pool identity. Inputs are already trusted observer reports,
not page/model claims. Rows retain their original provenance verbatim.

Types/signatures: `capability_rows : Text → Maybe (List CapabilityRow)`;
`ledger_from_report : StorageExpectation → List CapabilityRow → SpaceLedger`;
`publication_from_reports` joins an internal and selected-root report using
explicit pool relationships, then calls existing `publication_for`.

Invariants: absent, malformed, duplicate or stale identity cannot admit; equal
models do not join installations; app and shell authority remain separate;
reported/simulated writer states never become measured; alias capacity is never
added; integer byte quantities do not narrow at 4 GiB; internal floor remains
checked when selecting a large SD volume. Effects belong to Grease; the core
performs no file or device action.

Current producer limitation: app report has no installation/time/media-generation
binding and reports only framework-root-plus-canWrite. Shell pool equivalence is
unknown. The consumer therefore refuses those exact reports. Enriched successful
fixture rows describe required evidence, not implemented physical probes.

Required implementation path: maintained Idriç compiler source
`ff4d852862a3942592f8ade9afde8d409d9803be`; semantic checking and compiler
normalization, with no consumer code generation. A qualified ICK/NDK execution
path remains separate; no Chez/RefC/Java fallback is selected.

Attempt/result: filled after actual compiler invocation. Fallback: none.
