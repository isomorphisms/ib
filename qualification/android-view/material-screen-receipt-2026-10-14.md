# Native Material 3 reader — 14 October 2026

Checked Idriç now constructs a real Activity, LinearLayout, ScrollView, selectable
TextViews, source Button and layout parameters. Application-owned XML supplies
Material 3 dark colors and TitleLarge/BodyLarge/LabelLarge roles. Framework
TextView needs explicit scaled line heights; the current source applies
28/24/20sp rather than relying on text appearance alone. Source handoff prepares
ACTION_VIEW inside Android's chooser from the trusted application-owned address.

The reusable platform extension is
[isomorphisms/android-NDK PR #26, “Construct native framework objects and generate checked Activity lifecycle”](https://github.com/isomorphisms/android-NDK/pull/26),
at `37714fa1a28b51ba485744e6a17769cc806550cb`, tree
`5cd02e6016a1ee6cd0b3c9ae9d87ffb134cd5eee`. It owns construction, register
placement and the bounded original-Bundle lifecycle bridge. Browser, source,
retained-item and store owners remain unchanged. The body is a bounded snapshot
of IB README at `9027d5ea0ce8718b0337b4cd3d16c0549dc2b2ed`, UTF8 SHA256
`23a8e104147e94cb1bf27f5b63d4ddb6007dd1b73479e0dfe83796f3965e5867`.
Its exact address is in `android-material-reader/content-source.tsv`; this is
not live Pensieve acquisition, a parallel browser/store or a Markdown parser.

## Passing source/package checks

Fresh actual production source emits the Activity twice. Independent baksmali
decoding verifies its superclass/constructor, onCreate superclass call and
checked handler, native setters and Intent construction. Recovery DEX is
identical. All twelve backend modules source-check; 23 constructor/lifecycle
controls, inherited callback/foreign/encoder/layout suites, two production
mutations and wrong-name/nonvoid source refusals pass. Three ReaderText
production mutants emit distinct DEX; runtime rejection remains a separate gate.

| Artifact/input | Exact identity |
| --- | --- |
| Current screen DEX | 5,160 bytes; `b2ce30b0af52bc52beffd167fcbac331d447eeb7ac4a13db0e068662be2d0c17` |
| Final current-producer test APK | 20,990 bytes; `9d589b84564fe25bc9999ed000d96d94c00aff8df79e7adc174a263e199f63d7` |
| Package/version | `org.isomorphisms.ib.material3`, versionCode1/versionName0.1.0 |
| SDK bounds/tools | min28, target34, platform34, Build Tools35.0.0 |
| Existing public test certificate | `de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9` |
| Public key fixture | `isomorphismes/wegert`, commit `89dcfb840cec1a66ee04c7f7404954cbd6c09839`, blob `7dc3fd0f38608bfcfe8e6942c8b2d183557b6396` |
| Observed Idriç compiler | `2734ac5a86610eddfab73b1644d72b4ac85a65ee` |
| Compiled driver object | `b06499beb4b378b3103fbf146a3b3d83cc805cf6ff75489c8b1f622488949104` |
| baksmali 3.0.10 | `37ae4a41a8886e15c20b8362fa4250f96bbdb55e1a608199ad8b5dff068b588f` |
| smali 3.0.10 | `32fa0e88a6c397b3922201adf5f3e534fbaed5a663c71d0c558c3ddce0af844a` |

The candidate packager verifies linkage, DEX checksum/extracted bytes, alignment,
certificate, package and launcher. Wrong certificate, missing signer and wrong
package exit1 at their intended gates and expose no final APK. No signer was
generated. Existing Chez host-bootstrap/shared build-policy debt remains explicit.

## Android history, not an overall PASS

The disposable Ubuntu24.04/x86_64 host runs Android14/SDK34/default/x86_64 with
software emulation/SwiftShader and no KVM. Emulator and ADB share one Grease
session. An earlier APK (`eb0496c32e84222a32d71e213b848a5301b6459d3009b09f895e3381761fb6cd`)
installed, launched, displayed the source text and scrolled after an actual swipe.
Its DEX (`96ba9d572835d8f8a6fc46602a19f80c8b51b669fc5efba55b981f76c67cfdc3`)
predates the explicit line-height correction. Current-body APK attempts installed
and reported Activity launch, but UI observations were interrupted by system/
system-UI ANR dialogs and one package-manager restart. The full current-width
and source-chooser gate is **NOT_PASS**. This is not a performance/device claim.

Standalone observers exited137 because DeviceConfig used the system Application
identity under shell UID2000. A later attempt also exposed uninitialized lazy
fonts outside normal application binding. Creating a shell view Context alone
was insufficient because that API preserves its container's app-ops attribution.
Further standalone setup still had no registered Application process for the
Settings provider. The active independent observer now runs through Android
Instrumentation in a real target process. Android supplies normal Application,
UID, provider registration and fonts. Candidate DEX remains source-generated in
a separate target APK; smali supplies only the observer. All failed standalone
runs remain failures. Native text, selection, width reflow and all three production
mutant rejections must pass independently. The screen APK uses its own normal
Activity/Application context.

The platform PR extends the existing CI gate with constructor original/recovery
observations and received-byte comparisons. Hosted acceptance must bind to its
exact head and actual retained artifact. Current CI is running; its earlier
green dependencies do not supply current-head success.

Live retained-source/store wiring, central package registration, independent
AICI promotion, versioned upgrade/data compatibility and physical MIRO A1/TAB_P10
acceptance remain NOT_RUN. The result is a test-signed source-snapshot candidate,
draft and unmerged. No release, production signing, physical install or
device-side compilation occurred.
