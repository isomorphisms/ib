# Durable task discovery repair — SUN IB-S04

Owner: [isomorphisms/ib issue #99](https://github.com/isomorphisms/ib/issues/99).
Source baseline: `main@10fa7bbe93ff9ffa1ba7b9f62e0dd8f6c10a86bc`.

The protected-task adapter admitted literal `.` and `..` as path components,
selected only the newest task file, followed linked entries and read its bytes
without a bound. A malformed newest file could therefore conceal every older
valid task.

The source repair rejects both dot identities before any store hierarchy is
created. It checks existing directory ancestors and entries without following
symbolic links, refuses non-regular/linked record and history destinations,
opens record descriptors with `NOFOLLOW_LINKS`, and retains atomic same-directory
replacement. Both task and tab identities and output bounds are checked before
creation. Task/tab publication remains two separate operations; this does not
introduce or claim a multi-file transaction.

`discover()` supplies every valid protected task, ordered by its task identity,
and a separate refusal for each invalid entry. Encoded task records are limited
to 64 KiB and an enumeration to 1,024 entries. Exceeding the entry limit refuses
the whole enumeration rather than labeling a partial set complete. Task reads
also enforce the limit while reading, reject malformed UTF-8 and require the
final line terminator. The history append boundary has a separate 4 MiB cap.
These limits are adapter budgets, not general corpus limits or a new identity
scheme.

The current single-task Android harness consumes `discover().latest`, records
the valid/refused counts and per-record refusal codes, and can still reopen an
older valid task when the newest is corrupt. Equal timestamps use task identity
order for a deterministic selection. Refusal codes do not echo serialized
fields or filesystem exception text into receipts. No neutral-navigation,
authorization, form, session or protected-task schema was changed.

## Evidence and first blocker

Implementation and focused acceptance source are present. `git diff --check`
is the available whitespace/source check; it is not Java execution.

Compiled host tests, mutation execution, Android API qualification, APK build,
emulator behavior and physical-device behavior are **NOT_RUN**. The first build
blocker is concrete: this repository has no `ci/build-toolchain.tsv` declaration
or qualified ICK/NDK stage for the existing Java adapter/tests. Its inherited
`android-webview/app/build.gradle.kts` selects Java 17, and
`.github/workflows/android-webview.yml` directly invokes Gradle. Those are
unqualified Java/Gradle debt under the current shared build policy, not an ICK
or NDK path. No Java/Gradle compiler was invoked for this repair.

Review publication is a source branch. Opening a draft PR is deferred because
the existing Android pull-request workflow would execute that unqualified
stage. Publication, branch presence and unrelated green checks cannot satisfy
the missing compile/test qualification.

The filesystem checks reject existing static symbolic-link parents and entries.
They **do not prove descriptor-relative confinement against concurrent directory
replacement by another actor with access to the app-private hierarchy**. Java's
path-based directory creation and atomic rename still have that race boundary.
Kernel-confined relative directory operations, concurrent-writer policy and
Android filesystem durability must be qualified separately. Hard-link attacks
and power-loss directory durability are also not established by these tests.
This repair must not be advertised as complete adversarial confinement.

## Focused acceptance source

The existing `DurableTaskRecordTest` retains all previous continuity, secret
exclusion and protected-only tests. Added cases cover:

- dot task/tab identities and read/append refusal before state creation;
- all older valid records and an explicit corrupt-newest refusal;
- deterministic identity order and equal-timestamp selection;
- linked state parent, task directory, record and history refusal, with unchanged
  outside targets;
- oversized, incomplete-final-line and wrong-directory-identity records;
- an interrupted temporary replacement leaving committed bytes intact;
- directory-entry overflow refusing a supposedly complete subset.

Once a qualified stage exists, execute these tests from the exact source head
and execute both deliberately bad patches beside this note. The dot mutation
must fail `dot_identities_refuse_before_any_state_creation`; the discovery
mutation must fail `corrupt_newest_keeps_all_older_valid_records_and_failure`.
These are expected rejection cases, **not executed mutation evidence**.

## SUN IB-S05 dependency

Ordinary tabs still deliberately fail the protected-task adapter's `save`
boundary. S05 remains **BLOCKED_EXTERNAL** on this reader's compiled acceptance
and the declared Idriç/Grease execution path. The existing `IB.History`
`entry_order` is presentation order, not stable event identity. No new Java
browser model, ordinary-tab persistence, migration of real records or stable
event implementation is claimed here. S05 may proceed only from an accepted
reader and must preserve `IB.LongViewTask` identity, generations and secret
exclusions when adapting copied protected fixtures.
