# Android Material view qualification

**BLOCKED_NATIVE_VIEW**, 2026-10-08. SUN IB-D01 / [issue #55](https://github.com/isomorphisms/ib/issues/55)
has executable, pinned dependency inspection and a preserved Idriç control.
It has no accepted native text/action round trip. D02–D04 and D06–D08 must not
infer a qualified UI compiler or runtime from this receipt.

The first available-executable blocker is that installed Idriç
`ff4d852862a3942592f8ade9afde8d409d9803be` has no `dex` generator. The
separately maintained android-NDK DEX driver admits checked Int32/Text exports,
but does not supply the generic framework object/callback surface needed by
an Android View. Neither acquiring an AAR nor selecting NDK repairs that
source-to-framework gap.

## Choice and ownership

Keep the user's selected **Material 3** product target. Qualify a narrow
Android projection/action adapter against the existing Idriç core; do not
adopt an application framework from package presence. Preserve the distinction
between a Views/MDC implementation and Compose Material 3. Platform
`android:style/Theme.Material` is not evidence of Material 3.

Material Components Views 1.14.0 is a concrete Material 3 styling/component
candidate without Compose. Its upstream [Android guidance](https://m3.material.io/develop/android/mdc-android)
declares this the final stable Views release and moves it into maintenance in
2026. That limits the candidate's maintenance horizon. Compose remains an
explicit comparator; no requirement here selects its compiler plugin,
recomposition runtime or transitive stack as IB's implementation.

The preserved `material3-arxiv-viewer` branch at
`78d17f8a4683a292db849647c8716a3ab2c1ceeb` pins Compose BOM 2026.08.00,
activity-compose 1.13.0, AGP 9.1.2 and Kotlin 2.4.20; its build declares
compileSdk 37 / minSdk 26 / targetSdk 36. Its Kotlin Activity loads a frozen
Pensieve/arXiv asset. It is a snapshot comparison input, not the accepted
source acquisition, core model, generic view adapter or build path.

`isomorphisms/fastchat` at `638033df5072cbe256486236a39af751a86bf4ca`
provides a useful boundary pattern in `src/render_policy.h` and
`src/conversation.h`: `render_stored_window` takes a bounded stored window,
while event admission retains conversation/event identity separately. Reuse
that separation concept; IB's current model and command owners remain
authoritative. Its C records, alternate renderer experiment and compiler debt
are not adopted as IB semantics.

## Actual package inspection

`artifacts.tsv` fixes 31 input URLs and SHA-256 values: 15 bytecode archives
and 16 POM/BOM files. `inspect-artifacts.grease` verifies cached and acquired
bytes, checks ZIP structure, counts classes/resources/native members and
preserves each AAR's manifest/metadata. It produces `inspection.tsv` only
after all inputs pass. Downloads have time and 32 MiB limits. The committed
`evidence/artifacts.tsv` is the observed result, not an estimated APK.

| Artifact | Archive bytes | Classes | Min API in manifest | Min compile SDK metadata | Requirement supplied |
| --- | ---: | ---: | ---: | ---: | --- |
| Material Views 1.14.0 | 2,741,429 | 1,108 | 23 | 1 (sentinel) | Material themes/components for Views |
| Markwon core 4.6.2 | 133,475 | 131 | 16 | absent | Markdown into Android TextView spans |
| CommonMark 0.13.0 | 141,562 | 122 | n/a | n/a | Markwon parser dependency |
| Annotation 1.1.0 | 28,478 | 59 | n/a | n/a | Markwon annotation dependency |
| Window 1.5.1 | 485,445 | 262 | 23 | 34 | Window/fold layout observations |
| AppCompat 1.7.1 | 1,148,656 | 400 | 21 | 34 | AndroidX view compatibility comparator |
| Core 1.17.0 | 1,409,473 | 936 | 21 | 36 | AndroidX utility comparator |
| Activity Compose 1.13.0 | 144,321 | 35 | 23 | 36 | Preserved Compose Activity boundary |
| Material3 Android 1.4.0 | 5,167,765 | 1,367 | 21 | 35 | Preserved BOM's Material 3 implementation |
| Foundation Android 1.12.0 | 4,441,825 | 1,982 | 23 | 37 | Preserved BOM's Compose foundation |
| UI Android 1.12.0 | 4,047,839 | 1,300 | 23 | 37 | Preserved BOM's Compose UI |
| Runtime Android 1.12.0 | 1,997,054 | 764 | 23 | 34 | Preserved BOM's Compose runtime |
| Core 1.16.0 | 1,377,184 | 918 | 21 | 35 | Material Views POM's baseline core |
| AppCompat 1.7.0 | 1,148,962 | 400 | 21 | 34 | Material Views POM's baseline AppCompat |
| termux-am-library v2.0.0 | 16,687 | 8 | 21 | 1 (sentinel) | Activity-manager control, no view renderer |

All 15 inspected archives have **zero `.so` members**. This supports no claim
about a resolved transitive closure, final ABI readiness, DEX method count,
APK size, memory or launch time. Class counts are archive entries. Metadata
compile floors are packaged compatibility declarations; the Gradle-plugin
minimum is not a grant to use Gradle. Detailed classes.jar bytes and resource
counts remain in the TSV. Compose BOM 2026.08.00 is a 41,126-byte version
mapping and contains no UI bytecode itself.

Material/AndroidX/Markwon/termux-am are Apache-2.0; CommonMark 0.13.0 is
BSD-2-Clause, verified in its packaged `META-INF/LICENSE.txt`. License names
for the others are in pinned POMs and upstream license records. termux-am's
source tag v2.0.0 is `d979ee308d52c59c6265ed9d9e254c97702ee4fb` and declares
no runtime library dependency. It belongs at a replaceable activity-manager
provider boundary if needed, not at the view boundary.

### Dependencies measured and unresolved

Markwon's POM declares CommonMark 0.13.0 and Annotation 1.1.0. These three
archives total 303,515 bytes / 312 class entries before any packaging or
shrinking. This is the measured declared root/runtime set, not an installed
cost. Plugins such as images, tables, HTML or syntax highlighting are absent.
Markwon transforms presentation; it must not parse rendered Markdown back
into browser-owned canonical content.

Material Views' POM declares activity 1.8.0, annotation 1.2.0,
annotation-experimental 1.0.0, appcompat 1.7.0, cardview 1.0.0,
coordinatorlayout 1.1.0, constraintlayout 2.2.1, core 1.16.0,
customview 1.2.0, drawerlayout 1.1.1, dynamicanimation 1.1.0,
fragment 1.2.5, lifecycle-runtime 2.0.0, recyclerview 1.2.1,
resourceinspection-annotation 1.0.1, transition 1.5.0,
vectordrawable 1.1.0, viewpager2 1.0.0, graphics-shapes 1.0.1 and
error_prone_annotations 2.15.0. Its Kotlin BOM 1.8.22 is dependency
management. Core 1.16.0 and AppCompat 1.7.0 were inspected; the remaining
resolved payload is **UNKNOWN**. Core 1.17.0/AppCompat 1.7.1 are separate
inventory comparators and must not be summed as its resolved closure.

Window 1.5.1 declares window-core 1.5.1, collection 1.4.2, core 1.8.0,
jspecify 1.0.0, Kotlin stdlib 2.0.21, coroutines-android 1.8.1 and
annotation 1.8.1. It does not supply text rendering or durable navigation.
The Compose POMs carry their own larger dependency sets. Full variant-aware
resolution, merged resources/manifests, desugaring and packaging have not run;
these POMs advertise richer Gradle module metadata, which remains unresolved.

## Source-to-platform capability

The actual [android-NDK checked backend](https://github.com/isomorphisms/android-NDK/tree/7c61ee43e75f7c2dab9288edb0e10055898b36e6/dex/idric/src/Backend/DEX)
must be read beyond its older Int32-only README. `IR.idr`, `Codegen.idr` and
`Lower.idr` now support explicit Int32/Text source export ABIs, Text constants,
object moves/returns and the specific String equality call. They do not
admit arbitrary object signatures, constructors, View virtual methods,
callback classes, lifecycle or resources. `dex/README.md` explicitly separates
the checked compiler from retained hand-encoded NativeActivity/JNI fixtures.
No compiled checked-driver executable was available in the inspected host
inventory; its inherited Chez build would introduce an undeclared stage.

The preserved `examples/autogenerated/android-view-boundary` Text control
typechecked using installed Idriç. An actual `--cg dex` attempt failed with
the generator list recorded in `evidence/text-dex-attempt.log`. This diagnoses
installed integration, not failure of the separate source driver. No DEX,
application adapter or APK was produced.

ICK's concrete native Android gap was inspected in FastChat's retained
`qualification/android-application-boundary`: the owned repaired compiler
`2a27ad6ab4e4601c9a0e4a5fa7915712db707af0` rejects `_Nonnull`/`_Nullable`
and availability attributes in unmodified NDK r29 Android headers.
Its unchanged compiler descendant is
`326366fffbbcaa8b23fa0abe4f58d1c2b3c07420`. That AArch64/Linux syntax
diagnostic is not a MIRO A1 ARM32/Bionic artifact or ICK C++ qualification.
NDK can be selected for a separately declared narrow JNI compile/link stage
once its input and ICK gap are bound. It cannot compile the missing Idriç
framework boundary into existence. No stock compiler replaced either gap.

## Execution and recovery receipts

| Check | Observed result | Evidence scope |
| --- | --- | --- |
| 31 pinned artifacts, cached hashes and ZIP/class inspection | PASS | Linux package inputs |
| Same inspector from an arbitrary working directory | PASS, identical TSV | Source-relative manifest discovery |
| Cached input with wrong SHA-256 | FAIL, exit 1; no completed TSV | Incompatible bytes refuse inspection |
| Relative artifact directory | FAIL, exit 2 | Explicit path contract |
| Idriç Text control typecheck | PASS, exit 0 | Source only |
| Installed compiler `--cg dex` | FAIL, exit 1 | Undeclared generator refuses emission |
| Native typed text/action round trip | BLOCKED / NOT_RUN | Source-to-framework capability |
| DEX/AAR merge, APK/signing/update | NOT_RUN | Build/package boundary |
| Emulator / physical MIRO A1 | NOT_RUN | Platform/device boundary |
| Startup, RAM, APK bytes | UNKNOWN | Requires the actual artifact/device |

Inspection ran on Ubuntu 24.04.3 / x86_64, Linux 6.18.44. Consumer Grease
source is `ba869518c7d850de6c47d8c6234654575e264e6c`, Oils gitlink
`5651cf97a1b5042f24f14112a7ade9a1518eb0bc`; executable SHA-256
`7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`.
The consumer wrapper SHA-256 is
`37088097c36a2f82cf45aaea9392e4a4cf16155fecb9da87d08a9c7fb4d92e4a`.
`ASAN_OPTIONS=detect_leaks=0` is a ptraced-host accommodation. This available
sibling runtime is not a durable Linux CI provisioning receipt.

On another qualified host, invoke `grease` with this inspector's absolute
source path and an absolute fresh artifact directory. Use the committed
hashes; a missing, corrupt or oversized download must preserve its failure.
The retained TSV is reproducible input inspection. Clean-host native build
recovery remains blocked with the native round trip.

Next required qualification is an owned, build-policy-qualified checked
compiler/framework or JNI adapter followed by one real typed Text/action
round trip and refusal cases. Record its full resolved payload before
claiming APK cost; keep startup/residency and physical MIRO acceptance separate.
