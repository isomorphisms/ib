# Immutable-result publication acceptance

The task is replacement E2 implementation from the publication contract on
Longview source `ea322cd1c2dcc9021efddd089b2593bd4b40dd6b`, not recovery of an
unavailable historical commit.

## Type sketch before implementation

The existing `SpaceLedger` owns observed capacity and unreflected commitments.
A publication request carries result identity, byte limit, backend identity,
media identity, observation identity and reservation receipt. These are decoded
`Text` atoms at this acceptance boundary, not mount paths or process IDs.
Current backend/media/observation identities must match those admitted.

`publication_for : SpaceLedger → PublicationRequest → PublicationLocation →
Maybe AdmittedPublication` uses the existing `admission_for` decision and
`reserve_peak` transition; it emits the unchanged request together with the
reserved ledger only when admission and identity checks pass. Denied
observations have no publication action. The adapter consumes an already
admitted request and enforces bounded immutable file publication; it cannot
turn an unavailable observation into admission.

The admission owner must serialize/thread the returned ledger, as with the
existing reservation API. An E4 caller with an already-held reservation must
carry its settled admitted action rather than call this function to charge the
same reservation twice. The acceptance program imports the real `IB.StorageAdmission` and
`IB.WorkerLifecycle` implementations. Its IO writes a TSV action plan, which
the existing Grease ordinary-file acceptance driver executes against actual
files. The driver counts adapter invocations and compares complete bytes.
The Android filesystem tests call the real `DurableResultStore`; injection is
limited to filesystem scheduling and explicit I/O failures.

## Required facilities and evidence boundaries

Compile with the existing pinned Chez Idriç command in browser-foundation CI.
No RefC, generated-C, Python, or Node implementation is introduced. Grease
runtime is unavailable in this local container; inherited hosted POSIX
compatibility execution must be labeled as such, never as a Grease receipt.
Android tests use the existing hosted Java/Gradle adapter path. No package
installation, signer change, physical-device operation or Shizuku claim is
part of this task. Whole-device power-loss durability remains outside the
process-interruption acceptance contract.

## Result and language work

Pending exact-head hosted compilation/execution when this record is created.
The first local execution boundary is unavailable Idriç/Grease runtimes;
hosted compilation will determine whether the typed publication boundary is
supported by the pinned compiler. No language fallback is authorized or
implemented. Acceptance requires the real compiled policy's emitted actions
to result in exactly one allowed adapter invocation and zero denied ones.
