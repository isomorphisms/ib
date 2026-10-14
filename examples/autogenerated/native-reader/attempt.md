# Native reader application boundary

Purpose: move the phone-facing text/action path from Java to checked Idriç
and directly emitted DEX. This is an application consumer of the Activity
adapter in android-NDK, not another browser/task/store implementation.

Types: ActivityReference, IntentReference, ViewReference, TextViewReference
and CharacterSequenceReference are distinct external platform domains. Raw
nullable Intent text is normalized through Android TextUtils before it enters
the visible text projection. Supplied source text is copied verbatim to a
native TextView; Android performs line wrapping. Reading a user-pasted text
uses the native CharacterSequence -> Text conversion. No HTML parser,
source store, durable task or navigation journal is introduced in this view.

Actions: Android create -> checked create_reader; Android XML onClick ->
checked receive_reader_action. Source panel disclosure is ephemeral viewport
state. Durable browsing state remains IB.LongViewTask/Pensieve/Cauldron.
The receiver and clicked view must survive the adapter unchanged.

Package identity: org.isomorphisms.ib.prepaint, label IB Prepaint, existing
public test signer. A build must preserve signer and use versionCode >= 2;
physical replacement installation remains separate acceptance. This source
does not replace or erase installed org.isomorphisms.ib.webview state.

Target: checked Idriç -> ANF -> direct DEX -> Android native framework views.
Application XML supplies the view construction; no Java/Kotlin/Gradle/d8,
AndroidX, Compose or JNI application path is selected.

Scope boundary: this first reader consumes already-extracted text and explicit
user input. Full retained-source projection, images, asynchronous acquisition,
Longview/Binder worker and durable navigation integration must still use their
existing owners. A text reader is not acceptance of those unfinished paths.

Status: source/backend/APK verification pending. Physical C67 NOT_RUN.
