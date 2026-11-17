# SUN IB-ADMIN-LONGVIEW-E2E-01

## Intended outcome

The [assignment recorded on isomorphisms/ib issue #100](https://github.com/isomorphisms/ib/issues/100#issuecomment-6949634914)
requires a usable control on slow administrative pages and honest continuation
after Android lifecycle events. Loading a page, seeing an interactive DOM, and
retaining a cookie profile do not prove that a remote task can continue.

This change connects the retained checked protected-task model to the current
native IB application. Its deterministic acceptance surface is a delayed local
administrative page. Google Cloud Console and the YouTube Studio / YouTube API
journey remain separate physical acceptance obligations.

## Fresh integration base and retained work

The isolated branch begins at `9f468f9b6dfba8f3895f4ab5cd7a316dc08c562e`, the
inspected head of [isomorphisms/ib PR #167, "Integrate and qualify the checked native IB reader; retain tablet and Google blockers"](https://github.com/isomorphisms/ib/pull/167).
The inspected main head was `0f3d8dcdc20239987ce30da3d27788a90d2c090a`.
The existing native WebView was a bounded Google search-acquisition attachment;
it did not execute the retained LongView lifecycle model.

The following work supplies requirements or reusable source, with its original
evidence preserved:

| Work | Reconciliation in this change |
| --- | --- |
| [isomorphisms/ib PR #133, "Fix C67 Long View startup on Android-managed directory symlinks"](https://github.com/isomorphisms/ib/pull/133), head `d05807236c2a8891195d057405a0bd4050313a2b` | Preserve the trusted Android root / strict IB descendant distinction. The native base already obtained `getFilesDir().getCanonicalPath()`. The new store exercises that boundary directly. The rejected Java launcher is not restored. |
| [isomorphisms/ib PR #91, "Account for page-load latency without UI-thread receipt fsync"](https://github.com/isomorphisms/ib/pull/91), head `f3533211bc0eca556b3ad58a3ca64ecaaea1d618` | Recover the timing categories. Native observations use bounded numeric fields, immutable callback identity, and no raw JavaScript result logging. Actual task durability barriers retain separate elapsed-time measurements. |
| [isomorphisms/ib PR #134, "Move Long View lifecycle dispatch into Idriç and exercise its direct-DEX path"](https://github.com/isomorphisms/ib/pull/134), head `41d4cae9c15e42d3b0b4ed4ed957ba86a7ac396a` | Reuse generation-checked lifecycle dispatch in the actual Activity. Its earlier diagnostic DEX is historical evidence, not acceptance of this connection. |
| [isomorphisms/ib PR #147, "Remove Java application paths and add a checked direct-DEX reader"](https://github.com/isomorphisms/ib/pull/147), head `f2b3ea861ef960bc79fe59406131abbff0c628d2` | Recover the stricter neutral-address validator lost from the native branch. |
| [isomorphisms/ib PR #114, "Consolidate Longview through the Binder boundary with retained acceptance work"](https://github.com/isomorphisms/ib/pull/114), head `3524599f53e4028957d9d9580079a36760dd9b3f` | Retain the browser-owned task/tab/history model and native ownership. Do not introduce a second application or an alternate browser data model. |

The native base's [run 41631727130](https://github.com/isomorphisms/ib/actions/runs/41631727130)
passed source-to-DEX and its API34 reader job. Its API29 job failed after the
device became unavailable at the observer's `adb unroot` boundary; subsequent
checks could not use that device. Those results describe the base only. The
older latency branch's low-memory run failed at installation before page
acceptance. Neither result establishes this change's Android behavior.

## Implementation and safety boundaries

`IB.LongViewNative` is a bounded, strict codec around `IB.LongViewTask` and
`IB.LongViewLifecycle`. It rejects malformed, missing, repeated, unknown and
inconsistent record material. The native boundary transfers UTF-8 exactly and
executes the checked exports. Task transitions remain in Idriç.

The current application exposes **Open protected page** and **Resume protected
page**. One active protected renderer uses the real Android WebView. The source
reader and existing Google result-acquisition path remain available for their
existing purposes. No Java/Kotlin application or Gradle path is added.

An entered URL is transient. Ordinary state records contain only its permitted
origin and opaque task/tab/navigation identities. Task, tab and history records
become durable before an atomic current-selection reference is published.
IB-owned directories and files reject symbolic links, unsafe ownership/modes,
unbounded reads and incomplete state. An incomplete selection stops recovery;
it does not silently create a substitute task.

App switching can retain the live renderer. Activity, renderer and whole-host
reconstruction retain logical identity and advance the corresponding
generations. A reconstructed page is fetched at its origin. Lost paths,
queries, ordinary form input and JavaScript state may require repeating the
remote step. A page commit never sets an authenticated-session claim. No
state-changing request is replayed by recovery.

Metrics separate renderer construction, durable state checkpoint cost, main
frame callbacks, navigation timing and independently observed useful actions.
The native logger accepts only bounded numeric observation fields. It does not
write diagnostic receipt files on the UI thread. Task checkpoints are actual
durability barriers and are measured as such. No performance improvement is
claimed without an installed measurement.

The local fixture includes a delayed script, later resource traffic, an actual
touchable control confirmed by the server, mock verification, a synthetic
one-time result and a server operation whose committed effect has an incomplete
response. The independent observer belongs to `isomorphisms/ai-ci`; product
code does not recognize a fixture-ready marker as task success.

The consumer pins observer commit `40ff48763a45f0da97cc83ba29c2759ba8e80b29`
from [isomorphisms/ai-ci PR #355, "Independently exercise IB admin controls and protected-task recovery"](https://github.com/isomorphisms/ai-ci/pull/355).
Its host qualification rejects 31 record counterexamples and six diagnostic
counterexamples. The separately signed observer package was built and decoded;
that result does not establish installed ART execution. The consumer workflow
executes these host refusal controls and three independent installed lanes.

## Declared build inputs

| Stage | Exact input |
| --- | --- |
| Checked frontend | `isomorphisms/Idric` `740bbf2b1d9e811fd00c719c3473eede17ceea3b` |
| Direct DEX backend | `isomorphisms/android-NDK` `153e7fc8053ba4072954a5e77d4efa3609d829e4` |
| First-party C frontend | ICK `fbe86e23d55cfec2000c08e61deea2a407fd7175` |
| Android assembly and link | NDK `27.2.12479018` |
| Host orchestration | Grease `f19c94c6df18cddbdc1e81463e5bd689533e3c13` |
| Package | `org.isomorphisms.ib.prepaint` |
| Existing certificate SHA-256 | `de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9` |

The local execution host is Debian 13, x86-64. The exact frontend/backend,
Grease runtime and qualified ICK stages were recovered from the native base's
source-bound replay artifacts. Their original embedded runner path was mapped
to the recovered directory by a symlink; compiler payloads were not rewritten.
An actual literal-assignment-arrow program was compiled and executed through
the recovered ICK before using it for new host boundaries.

The NDK archive was checked against its official size, SHA-1
`090e8083a715fdb1a3e402d0763c388abb03fb4e` and ZIP integrity. The first `wget`
transport stopped after one range; `curl` ranges completed the same pinned
archive. One temporary Grease command initially used unsupported quoted-glob
syntax after downloading; an explicit ordered loop assembled the unchanged
parts before the full hash check. These are provisioning failures, not product
test results. SDK build-tools 35 and platform 34 archives were independently
checked against the official repository manifest before use.

## Acceptance ledger

Detailed source-bound receipts live beside the core, native and fixture
attempts. A receipt naming a working-tree base and source hashes is evidence
for those bytes; it is not relabelled as a later commit's hosted acceptance.

| Obligation | State at initial integration |
| --- | --- |
| Strict checked codec and generation/session refusal cases | PASS on host; 48 named assertions and three compiled counterexamples |
| Six actual managed codec exports, independently disassembled | PASS at the declared backend; no ART execution implied |
| Real local HTTP fixture, one-time delivery, ambiguous effect and read-only no-replay | PASS on host |
| Trusted-root alias, strict descendant and atomic store behavior | PASS on host; 40 assertions execute the production store |
| Complete native app DEX and three ABI native libraries | PASS at the declared compiler stages; independent DEX/ELF inspection |
| Complete signed APK packaging and byte reproduction | In progress; first primitive-field generation attempt failed and was repaired using the supported boxed reference field |
| Installed API29 / API34 admin and retained reader suites | NOT_RUN for this change at initial integration |
| Separate 1 CPU / 1 GiB / 480×800 lane | NOT_RUN for this change at initial integration |
| Physical C67 startup and authenticated Google Cloud control | BLOCKED_EXTERNAL; no physical connection or signed-in interaction available |
| Separate YouTube Studio / YouTube API workflow | BLOCKED_EXTERNAL; exact live screen and user-operated authorization required |

The physical gates require the user's logged-in phone and an explicitly
available installation channel. They must distinguish Google Cloud project
credentials/OAuth administration from YouTube Studio and the YouTube API setup
journey. An embedded sign-in rejection must be recorded and handled through a
supported external authorization path, without user-agent spoofing or bypass.
No live credentials are created, rotated, revoked or copied by this change.
No release, merge, physical installation, data clearing or issue closure is
authorized by these CI results.
