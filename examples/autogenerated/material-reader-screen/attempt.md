# Native Material reader screen

Purpose: use the actual checked construction/lifecycle backend to display a
prepared text projection in an application-owned Material 3 reading surface.
The Activity and all application behavior are generated from Idriç. Android
resources specify presentation; no Java/Kotlin/Gradle application is produced.

## Type sketch before implementation

`present_reader_screen : ReaderActivity → IO ()` receives the actual platform
Activity through its generated onCreate bridge. Android receives the original
Bundle through the superclass call; this disposable screen does not introduce
another durable task or store. The constructor and lifecycle remain the
android-NDK backend's ownership; the application owns presentation resources.

Framework handles are opaque, constrained by the checked foreign descriptors.
The Activity's Window supplies its real themed Context. Constructors create
LinearLayout, ScrollView, TextView, Button and layout parameters. Existing
ReaderText supplies exact decoded text, wrapping and selection. getRootView
on an unattached widget gives a real View reference for containment; this is
not an unchecked source cast or an integer disguised as a framework object.

String/resource inputs are a prepared, bounded snapshot for this qualification
screen. They are not a new source store or a substitute for the existing
retained projection. The first snapshot is IB's own source document with its
exact revision and byte digest. Live retained-source/store wiring is separate.
External Open source is an explicit user action via Android's chooser, not an
IB tab/history operation. Its target remains in trusted application chrome.

## Acceptance

Generate source-checked DEX with real Activity inheritance, superclass onCreate
and the source-owned handler. Decode it independently. Link application-owned
resources/manifest with aapt2, add only the checked candidate DEX, and preserve
the existing public test certificate rather than generating a signer. Actual
Android execution must display the source text, permit selection and vertical
scrolling, and reflow at two widths. A screen screenshot and resource inspection
must match the actual Material color/typography roles. Mutants, update identity,
physical targets and live source wiring remain explicit separate gates.

Fallback: none. The target is direct DEX with native framework Views.
