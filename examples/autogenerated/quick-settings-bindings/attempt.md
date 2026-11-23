# Quick Settings binding access

## Purpose

Make android-NDK's Quick Settings declarations importable by IB/Pensieve.
The user requested bindings with no chosen purpose. The dependency owns the
Android declarations; IB does not introduce a second binding facade, a tile
action, a service subclass, or an application manifest entry.

## Type sketch

The source imports `Android.QuickSettings` from the pinned
`vendor/android-NDK/quick-settings/idric/src` tree. Its package is
`idric_android_quick_settings`, declared in
`idric-android-quick-settings.ipkg`.

The domains are platform-owned service, tile, icon, component, context,
callback, and pending-intent references, together with tile state and the
platform's request result values. IB must use the dependency's declared
types directly. A browser object, source identifier, integer, or text label
does not become an Android object.

The upstream reference domain includes Java null. Its distinct reference types
do not prove non-nullness, Android API availability, or lifecycle validity. The
consumer preserves that boundary and does not invent a `Maybe` conversion.

The central operations are the dependency's declarations for obtaining and
updating a tile, requesting listening, and requesting user approval to add a
tile. Framework calls remain effects. The consumer fixture checks access to
their declared signatures without executing any effect.

API availability remains the binding's platform boundary: a source import
does not establish that a particular Android release supports an operation.
An installed tile, listening service instance, callback lifecycle, or active
application context must come from an actual future platform adapter. This
change does not manufacture any of those references or choose IB behavior.

## Required boundary

The intended platform execution route is the upstream direct DEX foreign
interface. The fixture has no `main` and is checked as Idriç source only.
Source checking must preserve opaque framework reference types and primitive
widths, and must use the pinned dependency rather than a local copy.

Checked DEX lowering and Android execution are separate acceptance targets.
The upstream declaration package does not by itself supply service-class
generation, callback lowering, packaging, or lifecycle wiring. No Java, JNI,
Gradle, handwritten DEX, or host implementation substitutes for those gaps.

## Validation

On 2026-10-09 UTC, the dependency was freshly cloned from its canonical
repository and checked out at
[`65769fd68a30a26e0dacd2d0f9ba182850857e86`](https://github.com/isomorphisms/android-NDK/commit/65769fd68a30a26e0dacd2d0f9ba182850857e86).
Its tree is `fab868b58afa8105fe14dd73ae22f59a8d96e3a9`; the IB gitlink points
at that exact commit. The checked-out dependency stayed clean during checks.

The actual Idriç compiler source was
`ff4d852862a3942592f8ade9afde8d409d9803be`, reporting
`0.8.0-ff4d85286`. Its executable payload SHA-256 was
`10002074cfae31a15e6f136cd191b6abe9f8d58efcec438d21145e72802b2819`.
Execution used an existing compiler; it did not bootstrap a compiler or
perform an application compile/link stage.

The pinned package passed `--typecheck` into a fresh, separate checked-module
directory. The consumer then passed `--check`, using those newly checked
upstream modules and the explicit prelude/base directories from the same
compiler tree. The installed-prefix search was disabled. Exact commands,
input identities, and output are retained in `source-check.txt`.

The consumer file SHA-256 is
`9436423bfc4d5368317f72b125a3530da010f99475decd3e21baefd3a0b5272c`.
It has private signature checks for `tile_for_service`, `set_tile_state`,
`update_tile`, `request_add_tile`, and `read_add_result`. None is invoked.

| Boundary | Result |
| --- | --- |
| Pinned upstream declaration package source check | PASS |
| IB/Pensieve direct import and five declared signatures | PASS |
| Existing public Idriç vocabulary checks for `src` and this example | PASS |
| DEX emission / framework invocation / APK packaging | NOT_RUN |
| Emulator / physical Android device | NOT_RUN |

This is source access, not a claim that IB can already invoke Android. The
compiler and package checks ran on Ubuntu 24.04.3, Linux 6.18.44, x86_64.

## Language work exposed

The next platform boundary, if a tile purpose is later chosen, is checked DEX
lowering for these framework references and foreign calls together with real
service and callback lifecycle ownership. Its acceptance must execute the
generated binding against Android; this source-only fixture cannot establish
that result.
