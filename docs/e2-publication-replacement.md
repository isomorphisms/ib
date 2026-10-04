# E2 immutable-result publication replacement

This work derives a replacement implementation from the documented contract
on consolidated Longview source `ea322cd1c2dcc9021efddd089b2593bd4b40dd6b`.
It does not recover either inaccessible historical publication or E3 WIP
commit, and it does not substitute the old work-bounds smoke baseline.

## Ownership and integration

`IB.StorageAdmission.publication_for` applies existing per-volume admission
and binds the unchanged result identity, byte limit, backend/media identity,
observation identity and reservation receipt. It returns the reserved ledger
using the existing `reserve_peak` transition; the admission owner threads that
ledger across subsequent requests. An E4 caller that already owns a reservation
must carry its existing admitted action and must not charge it again here.
`lib/result_publication.grease`
only transports that already-admitted action to the existing ordinary-file
adapter. It rejects missing/refused/malformed actions and changed identities
without invoking publication. It has no capacity policy or fallback volume.

The action file is an internal trusted Idriç-to-Grease handoff, not an
untrusted page interface or a cryptographic authority token. E4 must issue it
from actual retained-result admission and carry the already-settled values
across the worker boundary. E3 must bind completion to the launch/attempt
identity before it uses the action. This change does not advertise those
unimplemented integrations as complete.

The ordinary-file publisher stages at most the initially observed source size,
refuses growth/change in size, synchronizes, makes the staged inode read-only,
and uses a same-filesystem hard link for atomic create-if-absent publication.
Equal bytes are idempotent; unequal concurrent writers cannot replace the
winner. Media initialization also uses create-if-absent rather than overwriting
another initializer's marker. Unsupported hard links fail closed with an I/O
failure; no rename-overwrite fallback is allowed. This is an ordinary-file
backend requirement, not a claim that FAT/exFAT supplies hard links.

The Android app-private `DurableResultStore` uses an atomic directory rename
and a bounded private snapshot of caller-owned bytes. It stages the complete,
synced read-only payload inside a unique directory, then atomically renames that
directory to `<result>.committed`. A committed directory is always nonempty;
Unix rename cannot replace it. No regular-file overwrite rename or nonatomic
fallback is permitted. Unsupported atomic moves fail closed. This avoids the
hard-link operation that an Android app sandbox can deny. The Unix directory
restriction is documented in [rename(2)](https://man7.org/linux/man-pages/man2/rename.2.html).
Legacy `<result>.txt` results from the baseline remain readable and are verified
without migration or rewriting. Staging cleanup runs even if the I/O write
fails. It refuses oversized reads before returning bytes. Provider grants,
Binder/PFD boundaries, retained information, renderer loss, package identity
and signing remain on the consolidated Longview line.

`ib_store_read_bounded` requires an explicit limit. The existing three-argument
reader now has the same 4096-byte ceiling as the Android fixture; consumers of
larger admitted ordinary-file results must pass their actual explicit bound.

## Deterministic acceptance

The real ordinary-file adapter is driven on disposable files. Two writers are
held after staging and before publication; the first commits and the second
must return object-conflict (70) for unequal bytes or success for equal bytes.
Staged data remains unreadable, including after terminating a launched
disposable writer before publication. Source growth is injected after observation
inside the copy primitive, and a partial staging write simulates No space left
on device (ENOSPC); neither may create a published object. Refused bounded reads
emit no prefix, committed bytes stay unchanged, replacement media fails lookup,
and independent late reads compare equal.

The actual compiled `ResultPublicationAcceptance` imports the production
`IB.StorageAdmission` and `IB.WorkerLifecycle` modules. Its action plan is
executed through the real ordinary-file adapter. Invocation counting requires
one admitted publication and zero invocations for unknown, stale, denied,
read-only, absent, backend-mismatched, media-mismatched or stale-identity cases.
It also asserts the existing committed-worker state survives later authority
loss. A deliberately partial plan followed by exit 37 must make the parent
return exactly 37, with no publication attempt. The foundation driver's former
compiler/runtime `tee` pipelines now capture output before displaying it and
return the exact failed-child status; the same compiled exit-37 probe tests
that logging boundary.

Android unit tests call the actual `DurableResultStore` with real files and
atomic nonempty-directory publication. The filesystem seam controls only
schedules and explicit faults.
Both writers stage before either is released; the first must finish before the
second publishes, deterministically exposing overwrite races. A known-bad
real atomic-rename primitive must fail the same success-count assertion.
Two staged unequal writers must yield exactly one success; two equal writers
must both succeed; interrupted staging is invisible; simulated staging ENOSPC
cleans up and never invokes publication; bounded reads refuse; a mutable caller
array cannot rewrite its committed snapshot; independent reconstructed readers
return the same bytes. A legacy committed-file regression prevents a storage
layout change from losing baseline results. Existing emulator acceptance must still prove retained
results after renderer/host loss and cross-UID provider reads.

## Evidence limits

Local container execution through the inherited POSIX compatibility path is
ordinary-file adapter evidence, not a current Grease runtime receipt. The local
container has no usable Idriç compiler, Grease binary or javac. Existing hosted
foundation and Android workflows provide the real compiled-policy and native
adapter execution paths; no new package installation is introduced.

No whole-device power-loss guarantee is asserted by shell `sync` or Java file
descriptor sync. No physical MIRO/Shizuku operation, authority acceptance,
automatic replay or signing migration is performed. The pre-existing worker
identity, retry policy, runtime envelopes, Binder boundary and live-PFD tests
are retained. The fixed privileged worker's eventual publication integration
belongs to Earth E3, not to a simulated Shizuku receipt here.
