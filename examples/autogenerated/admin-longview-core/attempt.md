# Program attempt: `admin-longview-core`

Status: `PARTIAL` — host semantics and checked DEX compilation pass;
installed Android and live administrative workflows are not established here.

## Purpose

Connect one real protected administrative task to the maintained checked
Idriç Android application. This is the semantic portion of
`SUN IB-ADMIN-LONGVIEW-E2E-01`, based on IB
`9f468f9b6dfba8f3895f4ab5cd7a316dc08c562e`.

## Intended behavior

The existing `IB.LongViewTask` remains the task owner and
`ib-long-view-task-v1` remains its ordinary durable representation.
`IB.LongViewNative` validates that representation, dispatches the existing
transitions and exposes bounded text/integer projections to the native
platform adapter. It does not add a browser, store, authentication provider,
renderer, or parallel task record.

## Type-system sketch

### Values and domains

- Existing durable task, tab, navigation, host and renderer identities.
- Validated HTTP(S) neutral origins with no path, query, fragment or user-info.
- Existing lifecycle, session, form, heap and reconstruction outcomes.
- Bounded, canonical ordinary-state records and positive generations.
- Renderer observations bound to every current identity and generation.

### Types and signatures

`start_record(task, tab, navigation, host, requested_address) → Text`
creates an existing protected task and serializes only its neutral origin.
`open_record(record, host) → Text` restores the checked task in the supplied
host. `attach_record(record, renderer) → Text` attaches a new renderer to that
task. `observe_record(record, kind, identities, generations) → Text` admits
only a current renderer-loss or page-commit observation.
`field(record, field_number) → Text` and
`counter(record, counter_number) → Int32` project the validated record.

Empty text and negative counter results represent refusal at the platform
boundary; they never authorize replacement of an existing valid record.
Internally, parsing and transition dispatch retain the existing semantic types.

### Actions and effects

The codec, transitions and projections are pure. The test program prints named
assertion results. Filesystem synchronization, Android callbacks and actual
renderer operations belong to the separate platform adapter.

### Laws, invariants, and errors

- Restore and renderer replacement preserve task/tab/navigation identities.
- New hosts advance host generation; same-host reopen advances only Activity
  generation. Replacement attachment advances renderer generation.
- Callback admission checks task/tab/navigation/host/renderer identity and all
  three generations, refusing stale or unrelated observations.
- A page commit does not prove an actionable control or authentication.
- This first slice supports no authorization-adapter payload, persisted form
  value, authenticated-session claim, or persisted JavaScript-heap claim.
- Origin-only restart never replays a state-changing form or one-time value.
- Missing, repeated, unknown, corrupt, noncanonical or oversized fields are
  refused. A refused record remains refused rather than being replaced by a
  fresh task under its old identity.

### Runtime and backend boundary

The native boundary is the existing checked Idriç/DEX framework export path.
Text and Int32 are transport representations, not a replacement task model.
Host/Chez checks provide semantic diagnostics; only pinned direct DEX and ART
execution can establish Android behavior. No Java/Gradle or generated-C
fallback is selected.

## Idriç attempt

Source: `AdminLongViewCoreChecks.idric`, exercising
`src/IB/LongViewNative.idric` and the retained task/lifecycle owners.

The neutral-address validator is recovered from IB
`f2b3ea861ef960bc79fe59406131abbff0c628d2`; lifecycle dispatch is recovered from
`41d4cae9c15e42d3b0b4ed4ed957ba86a7ac396a`.

Exact frontend: `740bbf2b1d9e811fd00c719c3473eede17ceea3b`.
Exact checked DEX backend: `153e7fc8053ba4072954a5e77d4efa3609d829e4`.
Both were recovered from the native reader's pinned compiler artifacts.
The differently pinned local compiler was not used.

The reproducible host entry point is `verify.grease` with absolute repository,
frontend and fresh evidence directory arguments. `verify-dex.grease` takes
absolute repository, frontend, backend, pinned baksmali JAR and fresh evidence
directory arguments. Neither script downloads or substitutes a compiler.

The initial executed host evidence directory was
`/workspace/scratch/c37bb9f8dd86/admin-core-verified-01`; the added hidden-task
regression was checked in `admin-core-verified-02`. The reproducible direct DEX
run used `/workspace/scratch/c37bb9f8dd86/admin-core-dex-verified-01`.
Compact unedited receipts, output and source/compiler hashes are retained in
`evidence/`. These executions tested the recorded working-tree bytes over the
base above. They do not claim that a final commit or complete APK was tested.

## Result or failure

The first authoring compilation refused `record` as a reserved identifier.
Renaming that local parameter to `encoded` repaired the source syntax without
changing the language or selecting another backend.

The pinned frontend then compiled the actual codec and executed all 47 named
host assertions. A subsequent native integration review found that a hidden
task's Activity recreation also needs to dispatch `open_record`. A distinct
reducer regression now checks loss, same-host reopen and reattachment, including
refusal of the old Activity generation with the current renderer. All 48
assertions passed in the second run. That reducer case does not itself test
whether the Android callback invokes it.

Three separately compiled production mutations were executed in both runs and
rejected with exit 1 at the intended assertion:

- removed host-identity matching: `FAIL wrong host callback refuses`;
- removed renderer-generation matching: `FAIL wrong renderer generation refuses`;
- authenticated-session promotion on page commit:
  `FAIL page commit remains unconfirmed without a usable action`.

The production-codec probe compiled through the pinned direct DEX backend.
The 164,112-byte DEX had SHA-256
`5f4dbaa0d97a844b66b2d99118a9727564b7640687bb7dda639cfc0a9d09cd57`.
Independent baksmali parsing, using JAR SHA-256
`37ae4a41a8886e15c20b8362fa4250f96bbdb55e1a608199ad8b5dff068b588f`,
confirmed all six Text/Int32 export signatures. This does not prove ART
execution or the complete Activity's dispatch.

The repository's public Idriç vocabulary check also passed.

## Fallback

None.

## Idriç language work exposed

First blocking capability: none observed in the host compiler or checked DEX
generation of this consumer. Android runtime acceptance remains separate.

Smallest plausible language change: none proposed before an actual failure.

Acceptance test for that change: the unchanged codec and callback-refusal
consumer must compile and execute through the pinned backend.

## Evidence boundary

No live Google/YouTube, authenticated-session, Android installation, physical
C67 or performance claim follows from these semantic checks.
