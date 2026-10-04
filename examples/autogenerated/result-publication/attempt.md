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

Source inspection of the exact pinned compiler
`bd9fbe1e68ce9b3dd3981fcd3e1abf9e50bd350e`,
`Parser/Lexer/Source.idr`, found no lowering for the canonical Boolean equality
glyph. `EqualitySpelling.idric` preserves that attempt. The first production
use at `f0ae644237693409ce7ef89a7af38f3acb5bee53` failed hosted compilation
with `Undefined name ≟` in job 111526940515 of run 37233165624. The archived
standalone file itself is not claimed compiled. Newer compiler source supports
the glyph but upgrading this Longview
pin is outside scope. The production function uses the pinned compiler's
supported `compare : Text → Text → Ordering` and accepts only `EQ`; this keeps
the language/semantic owner and has no host-language fallback. Smallest language
work: adopt the existing glyph lowering on a separately verified compiler pin,
with this exact equality attempt as acceptance.

The production policy compiled and executed at
`253794cba59c40df4e758e445f49bb48fd0c7cd9` under that unchanged compiler pin.
Foundation run 37234493165, job 111530795102, passed the actual emitted-action
acceptance with exactly one allowed adapter invocation and zero denied ones,
reservation accounting, committed-state preservation and both exact exit-37
probes. The real ordinary-file adapter also passed its hosted POSIX
compatibility race/growth/interruption/bounds/fault tests. Local restoration of
overwrite rename is rejected with conflict expected 70 versus actual success 0.

At the same source, Android run 37234493282, build job 111530795224, passed
production DurableResultStore unit tests, including rejection of the real
overwrite primitive, lint, package boundary and stable signer checks. The
separate emulator stages are identified by that workflow rather than inferred
from unit tests. Those stages failed: the app could not commit its first result
and the reader therefore had no grant. Diagnostic source
`b3de3e3f80b4b684d557eb27e672b5c447464070`, run 37235216146,
heavy-page job 111533011151, exposed `AccessDeniedException` at the hard-link
operation on Android API 29. The hard-link Android implementation is not an
accepted E2 source.

Replacement candidate `b731444de8bed8455f36a56998a10cbb38cabe97` uses atomic
nonempty-directory publication in the Android adapter, while the ordinary-file
adapter retains hard links. The same deterministic race acceptance now rejects
regular-file overwrite inside the directory. A new legacy-file regression
preserves baseline retained results without migration. Foundation run
37235523461 passed; Android build job 111533720609 of run 37235523508 passed
unit, lint, boundary and stable signer checks. All three emulator jobs also
passed: renderer-loss 111533935352, whole-host-loss 111533935323 and
replacement/cross-UID/Binder/live-PFD 111533935373. This exact source is the
accepted E2 replacement. The source receipt and two downstream handoffs pin it
without historical prerequisites.

No host-language policy fallback was implemented. Actual
Grease runtime execution remains unverified because this local/hosted consumer
path uses the inherited POSIX compatibility interpreter.
