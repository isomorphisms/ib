# Native Material 3 reader — 14 October 2026

The source-snapshot reader passes its Android14/SDK34/x86_64 acceptance gate.
Checked Idriç constructs the actual Activity, LinearLayout, ScrollView,
selectable TextViews, source Button and layout parameters. App-owned resources
supply Material 3 dark colors and TitleLarge/BodyLarge/LabelLarge roles, with
explicit 28/24/20sp line heights. The body remains IB's own pinned README.

## Accepted source and artifact

[Run 38530691032](https://github.com/isomorphisms/android-NDK/actions/runs/38530691032)
passes at backend head `8040820ebd06e7441aab244ee7e62e727919c764`, tree
`ff25925ceb70e72a72368461160aac6ba5c14f06`. The separate NativeActivity packager
and native audio workflows also pass at that exact head. The platform owner is
[isomorphisms/android-NDK PR #26, “Construct native framework objects and generate checked Activity lifecycle”](https://github.com/isomorphisms/android-NDK/pull/26).

The workflow consumes immutable IB application source
`e53df40034f7e0413f07b5f34b0510f17ae52fd2` and Idriç
`ff4d852862a3942592f8ade9afde8d409d9803be`. Later IB commits add receipts and
historical-observer annotations; application source, resources, manifest,
build procedure and active screen verifier remain identical to that input.
The retained artifact's source-head files match these identities.

| Artifact/input | Exact identity |
| --- | --- |
| Retained artifact | `11877668095`; `dex-framework-effects-8040820ebd06e7441aab244ee7e62e727919c764` |
| Downloaded ZIP SHA256 | `dfc5bcee9140a236a447204e9a4ff3fed55d5268c94aa654119f7f7745c0479c` |
| Qualified, installed and delivered APK | 20,990 bytes; `6794bf03fc06d023e354db954d8d58f853f3f8058d4e9fea36332d6275fcf448` |
| Positive/recovery screen DEX | 5,160 bytes; `b2ce30b0af52bc52beffd167fcbac331d447eeb7ac4a13db0e068662be2d0c17` |
| Package/version | `org.isomorphisms.ib.material3`, versionCode1/versionName0.1.0 |
| SDK bounds/tools | min28, target34; platform34, Build Tools35.0.0 |
| Existing public test certificate | `de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9` |
| Public key fixture | `isomorphismes/wegert`, commit `89dcfb840cec1a66ee04c7f7404954cbd6c09839`, blob `7dc3fd0f38608bfcfe8e6942c8b2d183557b6396` |
| Reader text/source document | IB `9027d5ea0ce8718b0337b4cd3d16c0549dc2b2ed`; README UTF8 SHA256 `23a8e104147e94cb1bf27f5b63d4ddb6007dd1b73479e0dfe83796f3965e5867` |

The downloaded archive hash was checked against the retained artifact digest.
The delivered APK is byte-identical to the APK pulled from the installed
Android package; its existing public test certificate was verified again.
The positive and recovery DEX hashes match. This delivers the actual hosted
Android candidate, rather than substituting a separately packaged APK.

## Passing Android behavior

Registered Android Instrumentation checks the actual production ReaderText at
the pinned IB input in a normal target Application/UID/provider/font context.
The target APK contains only source-generated candidate DEX. Separate smali
supplies independent observation, never candidate behavior.

| Executed source case | Instrumentation code | Required observed result |
| --- | ---: | --- |
| Original | 0 | Exact Unicode/whitespace text, selectable text, empty replacement and narrow/wide line reflow |
| Text production mutant | 73 | `FAIL exact native reader text` |
| Reflow production mutant | 73 | `FAIL native reader width reflow` |
| Selection production mutant | 73 | `FAIL native reader selection` |
| Clean recovery | 0 | Full original marker |

All five installed fixture APKs were pulled and byte-compared with their signed
inputs. The retained runtime files confirm each code and distinct marker.

The actual IB APK freshly installs and launches, displays its pinned text,
scrolls after an actual swipe and reflows at 320×640 and 720×640 with density160.
The retained screenshots were visually inspected: actual native views show the
dark surface, pale foreground, primary rounded source button and explicit
Material type roles/line heights.

The source-button coordinates come from its observed native hierarchy.
An actual touch opens Android's ChooserActivity. A real chooser choice starts
the independent `org.isomorphisms.idric.construct.oracle` receiver, which
requires ACTION_VIEW and the exact trusted README address. Its received
hierarchy belongs to that receiver package and displays
`PASS exact native source address`.

Two disposable observer packages make the chooser observable in this SDK
fixture. Each first shows failure on a MAIN/no-data launch; the factory Chrome
package is disabled only in the disposable emulator. Its automatically restored
browser preference otherwise routes web intents directly. Default-only handler
resolution contains both observed receiver components before the IB touch.
Neither source data, provider permissions nor native results are replaced.

## Source/package gates and earlier failures

Fresh production source emits the Activity twice; independent baksmali verifies
its parent constructor, original-Bundle superclass onCreate, checked handler,
native setters and Intent construction. All twelve backend modules source-check.
Twenty-three constructor/lifecycle controls, inherited callback/foreign/encoder/
layout suites, wrong-name/nonvoid source refusals and two actual backend
production mutations pass. High allocation staging and parent Bundle transport
are checked at their production boundaries.

The candidate packager verifies resources, DEX checksum/extracted bytes,
alignment, certificate, package and launcher. Wrong certificate, missing signer
and wrong package reject without exposing a final APK. No signer is generated.

The earlier local APK `9d589b84564fe25bc9999ed000d96d94c00aff8df79e7adc174a263e199f63d7`
has the same screen DEX, but is a separate ZIP/signature artifact. Its source/
package receipt and Flexible Pipes package-slice closure remain scoped to those
original bytes. The qualified APK above supersedes it for delivery.

Software-emulator system ANRs and standalone app_process observer failures
remain failures. Standalone Context/Application reflection did not establish
the registered application identity required by selection's Settings provider.
Hosted chooser setup attempts also refused on observed browser-resolution,
help-status and restored-role conditions before the final fixture passed.
Exact failed heads, runs and artifact hashes remain in the platform
[construction receipt](https://github.com/isomorphisms/android-NDK/blob/8040820ebd06e7441aab244ee7e62e727919c764/examples/autogenerated/framework-construction/receipt-2026-10-14.md)
and `examples/autogenerated/material-reader-text/standalone-observer-history.md`.
Historical standalone scripts are not the active acceptance path.

## Remaining scope

This is a test-signed source-snapshot view. Browser, source, retained-item and
store ownership remains unchanged. Live Pensieve/source/store wiring, central
package registration, independent AICI/build-policy promotion, versioned
upgrade/data compatibility and physical MIRO A1/TAB_P10 acceptance remain
NOT_RUN. Inherited Chez host-bootstrap/shared build-policy debt is explicit.
Both PRs remain draft and unmerged; no release or physical install occurred.
