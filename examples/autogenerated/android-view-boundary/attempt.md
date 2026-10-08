# Android view boundary control

Purpose: isolate the existing typed Text export prerequisite before attempting
an Android text/action adapter. This control preserves the generated Idriç
attempt required by SUN IB-D01 and issue #55. It owns no browser model.

The eventual boundary must accept a browser-owned projection containing text
and stable action identity, apply it to a temporary Android view, and return a
typed action to the existing core command owner. Construction, callback,
lifecycle and action identity must be represented explicitly; an integer or
rendered string cannot silently become an Android object or canonical event.

`ViewTextBoundary.idric` deliberately tests only `Text → Text`, the source
signature already admitted by the checked DEX backend. Its `dex:display_text`
annotation selects the intended exported method. It does not implement the
eventual projection, a view, a callback, or a round trip.

On 2026-10-08, the installed maintained Idriç compiler at source
`ff4d852862a3942592f8ade9afde8d409d9803be` accepted `--check` for this source.
The same executable with `--cg dex` refused before producing a DEX file:
`No such code generator`. Full output is in
`../../../qualification/android-view/evidence/text-dex-attempt.log`.
This identifies an installed compiler integration gap. It is not a failed
execution of android-NDK's separate checked DEX driver.

Source inspection of `isomorphisms/android-NDK` at
`7c61ee43e75f7c2dab9288edb0e10055898b36e6` found that driver and checked
Int32/Text export lowering, including Text constants, object moves/returns and
the explicit String equality call. Its source ABI accepts only those two
primitive export types. It does not currently expose generic framework object
construction, view methods, callback classes or Activity lifecycle lowering.
Its retained hand-encoded NativeActivity experiment is a different boundary.

No Java, Kotlin, Gradle, d8, RefC, handwritten application DEX or host-language
UI replaced the blocked attempt. No additional compile/link stage ran.
Typechecking uses the inherited installed compiler; its Chez bootstrap is
unqualified build debt, not an ICK/NDK producer receipt.

Next language/toolchain work: provide a qualified executable driver from the
owned checked backend, then define and qualify the framework-object and
callback boundary (or an explicit narrow JNI adapter) before attempting the
real text/action round trip. Preserve identity, refusal and lifecycle evidence.
