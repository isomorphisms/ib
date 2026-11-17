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

### Late callback attribution correction

Review of the first published head, `6b74dae7419e5d373ff227e268155ee6318516ea`,
found that raw commit/finish callbacks discarded their URL and could acquire a
later document's mutable epoch. The existing stale timing-object control did
not exercise those raw lifecycle methods. This was a source-level finding;
an installed race was not claimed as reproduced.

The corrected adapter attributes raw callbacks only during the first accepted
main-frame start of a fresh renderer, with the callback URL matching both the
bounded transient started URL and the WebView's current URL. A later start or
URL ambiguity produces `document_epoch=0`, unknown attribution and no elapsed
time claim. These callbacks do not checkpoint a page observation or initiate
an attributed timing sample. Normal page interaction continues. Invalid numeric
payloads still reach the bounded parser when task/generation identity is current
and produce only a fixed refusal message.

This deliberately leaves later-document and ambiguous-redirect lifecycle
timings UNKNOWN within the same renderer. Android's callback API supplies a
URL, not an immutable originating-document token; a URL-history set would not
remove the redirect ambiguity. The
[Android WebViewClient contract](https://developer.android.com/reference/android/webkit/WebViewClient)
also separates a visible commit from complete loading or a useful control.
The independent observer exercises both a prior URL and the repeated current
URL through the actual client methods. Its bounded observation interval must
contain four UNKNOWN callbacks and no checkpoint or timing sample.

The consumer pins observer commit `81b9fe7b95446216c9626ef252255e26782008ab`
from [isomorphisms/ai-ci PR #355, "Independently exercise IB admin controls and protected-task recovery"](https://github.com/isomorphisms/ai-ci/pull/355).
Its host qualification rejects 31 record counterexamples and 23 diagnostic
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

The recovered OpenSSL source archive omitted 61 tracked paths through the
pinned commit's export-ignore rules. The first complete build stopped at the
unchanged clean-source preflight. Those missing paths were restored from the
exact local Git objects at `f4dc4d58b48d346a8270183f89acf826d459b0ca`; no source
pin or preflight was weakened. A fresh build from immutable IB head `6b74dae`
then completed the whole maintained recipe, including repeated DEX, three-ABI
ELF, signed APK byte comparison, payload counterexamples and signer/package
refusals. Its APK was 10,425,976 bytes, SHA-256
`d40958d8406bb41b5f31ffa4866282cf9120c12323c125472b2c9b8cdd6a3dab`.
This package evidence precedes the callback correction and does not qualify
that later source or installed Android behavior.

A further fresh, complete build at
`b02e67811acd43067eec2bc2f9d9a2042fd978bb` includes the callback correction.
The maintained recipe exited zero with repeated managed DEX, all three
application ELF libraries, signed APK bytes and the maintained payload/signer
refusal controls passing. The APK is 10,430,072 bytes, SHA-256
`614c6e7da28a09571f71c03c9400e802a726877b6f9b8dfa9de805171fcbc7e8`.
It keeps versionCode 3 and the existing certificate. The exact invocation and
result are retained in `package-b02e678-command.tsv` and `package-b02e678.tsv`.
These are package/build results; installed Android and physical acceptance
remain distinct.

The hosted `b02e678` pipeline also completed native packaging and its repeated
DEX/ELF/signed-APK checks. Its 10,430,072-byte candidate has SHA-256
`e7da1f237160ecc8965e881ce8e1d932eb4c7b2c66a5fd22ad155b43760e8ae0`.
Package, version and certificate match the local build. Application and platform
DEX, all three `libib_reader.so` libraries and the packaged Grease libraries are
byte-identical between those two builds. The three ICU transport dependencies
differ, so whole-APK equality across hosts is not claimed. Each build's own
repeat check reuses its declared, hashed dependency inputs. Installed evidence
must bind the hosted candidate actually installed. This comparison is retained
in `hosted-package-b02e678.tsv`.

### Existing scientific-media CI failure

The initial head's
[scientific-media job 134194762486](https://github.com/isomorphisms/ib/actions/runs/44127381054/job/134194762486)
failed before its PDF bridge was used. XML serialization replaced the row
separator U+001F with `&#xFFFD;`, so the image downloader read all four fields as
one malformed URL. The unchanged image-harvest regression reproduced this
failure locally with Ubuntu XMLStarlet 1.6.1-5build1, libxml2 2.15.2 and
libxslt 1.1.45.

The repair keeps the existing row contract. HTML text fields use text output;
the actual image tags remain XML and are joined to those ordered fields
outside the XML serializer. The two PDF row serializers also use text output.
Simply applying text output to the copied image node would lose its markup,
so the existing image-tag assertions remain necessary and unchanged.

All three existing image-harvest, reading-prefetch and navigation-prefetch
regressions then passed on the final local source. The PDF bridge used its
unchanged vendor revision and the declared local Idriç frontend `740bbf2b1`.
That is additional local evidence; the hosted scientific-media job's
`7050c1163` compiler remains its authoritative gate. No test, compiler pin or
assertion was weakened.

At `b02e678`, all three deterministic scientific regressions passed in the
hosted job with its pinned `7050c1163` frontend. The live arXiv check then
failed at the copied-tag / manifest-row count for `1901.09021`. One rerun of
the failed job reached the same assertion. Both attempts deleted their
temporary input through the existing cleanup, so the exact failed HTML and
rows were unavailable. A fresh public capture produced consistent counts;
it is not a reproduction of those hosted failures.

The followup fixes a separately reproduced locale defect. XML text is UTF-8,
but the existing `iconv` call inferred its source encoding from the process
locale. Under `LC_ALL=C`, a valid Unicode caption produced partial ASCII
output followed by the fallback input, yielding two filename lines. Declaring
`-f UTF-8` makes the new regression pass and changes the captured real caption
from two lines to one. The default `C.UTF-8` output remains byte-identical.
All three deterministic tests also pass locally with the same `740bb`-built
PDF bridge. This does not establish the cause of the hosted count failure.

The existing `sh` live-test boundary now retains a bounded failure capture
for its fixed public article identifiers: exact HTML, manifests, copied tags,
regenerated extraction rows, a NUL-delimited image filename inventory, source
and input hashes, versions, locale and original exit status. Downloaded image
binaries are excluded. The workflow uploads this capture only after that live
step fails. A 32 MiB limit is checked with an extra-byte probe; truncated
captures are explicitly incomplete. Executed Grease-driven controls preserve
the original exits, including cleanup failure, and distinguish an exact-limit
archive from one tar block over the limit even when the producer exits zero.
The source-bound receipt and repeatable helper are retained alongside this
attempt. Hosted execution of this followup remains a separate gate.

The next hosted head, `003524f`, passed the core job but failed the new Unicode
regression before the local fixture server started. The exact Ubuntu
`2.43-2ubuntu2.4` libc, converter, gconv modules and locale reproduced the
failure: `iconv` emitted converted text but exited one under `C.UTF-8`. The
fallback was inside the same command substitution, so it appended the original
caption to the emitted text. Moving fallback assignment outside the command
substitution discards failed output before retaining the original caption.
Both exact expected-slug assertions remain in place. The local `2.41` result
is retained separately; it had not exercised this converter behavior.
All three deterministic scientific tests pass with the exact `2.43` converter
components. The same local shell and `740bb` PDF bridge are explicitly recorded
in `scientific-iconv-243.tsv`; this is not a complete Ubuntu runner replay.
The old library still fails the unchanged expected strings with the new bounded
diagnostic branch. The next hosted run remains the authoritative CI result.

At `e436289`, the hosted core job, all three deterministic scientific tests
and the four-paper live HTML-first harvest passed with pinned frontend
`7050c1163`. The later live progressive-prepaint step failed without retaining
its input or naming an assertion. A fresh acquisition of the fixed public paper
has 134,967 bytes, SHA-256
`9b5baedbd35099ade52a9f9d3e3940c7b567edf70f235c333afd2e640c60c106`.
Its paper heading begins beyond the deliberately required 16,384-byte prefix.
An actual local `740bb` renderer replay fails exactly that heading assertion;
the 32 KiB stage includes the paper heading and opening abstract within the
existing 48-item painting limit, and all three stages remain width-bounded.
The fresh replay is distinct from the missing hosted inputs. The documented
byte-stage requirement is preserved; its compatibility with current source
layout remains a separate acceptance issue.
The followup test acquires that fixed public paper once, feeds the exact bytes
through the existing wrapper hook, names each unchanged assertion and retains
bounded HTML, renderer output, stderr and stage files on failure. A complete
capture means retained evidence, never a passing renderer result. The executed
capture and original-exit controls are recorded in
`scientific-prepaint-capture.tsv` with a repeatable Grease helper. Hosted
execution must supply its own input and assertion before that failure is
attributed to the same source-layout boundary.

The observer repository's 46 checks passed at
`81b9fe7b95446216c9626ef252255e26782008ab`. This includes the repaired
[native handoff check](https://github.com/isomorphisms/ai-ci/actions/runs/44130578780/job/134206206402)
and [delivery check](https://github.com/isomorphisms/ai-ci/actions/runs/44130578672/job/134206206444).
These host and repository results do not establish installed admin acceptance.

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
| Complete signed APK packaging and byte reproduction | PASS at `b02e678`, including the callback correction, complete maintained recipe and signed APK byte equality; installed evidence remains separate |
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
