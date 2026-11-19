# Program attempt: exact decoded text in the native Material reader

Continue the existing D01 owner. The immediate result is a checked application
adapter for a supplied Android TextView, not a second browser or a finished APK.
The implementation lives in `src/IB/Android/ReaderText.idric`; the generated
qualification exports import that adapter instead of copying its body.

## Type sketch before implementation

`reader_text : ReaderTextView → Text → IO ()` transfers already decoded text
from the existing browser projection into a disposable view. The view's
resource/theme, construction, and retained-source identity remain supplied by
their owners. `read_reader_text : ReaderTextView → IO Text` reads the platform
value for qualification; it must not become durable browser input.

An opaque `PlatformCharacters` represents Android's CharSequence interface.
The existing backend has no general source-level subtype cast. String.length
and String.subSequence(0, length) produce an actual CharSequence containing
every UTF-16 code unit. This is a platform representation conversion, not
source slicing or a claim that UTF-16 offsets equal IB code-point coordinates.
It preserves supplementary characters, whitespace, and the empty string.

Effects are ordered: disable single-line mode, disable horizontal scrolling,
enable selection, then set exact text. Android lays out lines against the
parent-supplied width. IB does not insert newlines, normalize spaces, remove
source characters, or calculate a parallel text layout here.

## Required acceptance

Check and lower this source with the existing android-NDK checked backend.
Independently decode its DEX. A separate Android observer constructs TextView,
supplies Unicode and whitespace, checks getText().toString(), measures narrow
and wide layouts, and requires more lines at the narrow width. The actual
source mutant converting an empty subsequence must fail exact-text acceptance.
Restore source and require byte-identical DEX.

The observer may construct the framework object but cannot own or replace the
candidate text adapter. Its successful measurement would establish text/reflow
behavior only. It would not prove visible Material resource styling, Activity
construction, live Director acquisition, source/store wiring, click admission,
accessibility, APK/update identity, or physical MIRO behavior.

Fallback: none. General object factories and Activity construction are still
owned by android-NDK. A failed construction attempt must remain a compiler
requirement rather than selecting Java, Gradle, or handwritten candidate DEX.

## Result

The result below records the 13 October source-only stage. The 14 October
continuation adds source-generated constructors, a real Activity and registered
instrumentation; see the [current screen receipt](../../../qualification/android-view/material-screen-receipt-2026-10-14.md).
Standalone runtime attempts are preserved in `standalone-observer-history.md`;
they are not the active acceptance path.

Production module and importing fixture now check and emit independently
decoded DEX. The original/recovery bytes match. Three actual production
mutations emit different DEX. The external Android observer assembles; its
execution and mutation rejection remain NOT_RUN. Invocation details, tool/source
identities, failed attempts, digests and remaining construction/lifecycle
capabilities are in `qualification/android-view/reader-text-receipt-2026-10-13.md`.
