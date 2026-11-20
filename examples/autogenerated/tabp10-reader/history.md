# Checked retained navigation in the maintained reader

The reader needs Back and Forward over independently verified retained source
occurrences. `IB.History` owns imported visit/search metadata and chronology;
it has no selected retained-source generation or Back/Forward cursor. The
current reader's bounded navigation sequence belongs to `IB.Reader.Navigation`.
It does not replace that global history, mint another document identity, or
create a second document database.

The type sketch is an opaque `RetainedSource` carrying a safe source identity
and canonical positive generation, and an opaque `BrowserHistory` carrying an
ordered sequence of those occurrences and its selected position. Only checked
constructors/parser and bounded transitions construct these values. References
do not grant source admission. Canonical Arena receipts and fresh independent
length/digest/identity readback remain required on every retained open.

`IBH1` is metadata for up to 128 occurrences. Arrival after Back drops forward
references; it never removes Arena source bytes. Native `IBLAST2` atomically
publishes the selected source reference and opaque checked history, with a
SHA-256 over the complete prefix. Its ordinary-file owner loops over short
writes, synchronizes the file, renames, then synchronizes the directory. A
failed final directory sync reports uncertainty, even if the complete head can
already be read. The last valid previous head remains available. Legacy
`IBLAST1` is read without inventing missing history; verified reopening can
migrate it to a single occurrence. Idriç owns journal meaning; C owns the
narrow filesystem/JNI byte boundary.

The actual maintained Activity now includes Back/Forward, generation-preserving
retained open, checked selected-occurrence agreement, and acknowledgement only
after document preparation. Reflow reopens the same retained occurrence. An
explicit Back/Forward failure cannot silently select the previous-head source.
History controls disable when saving the displayed state fails.

After scratch removal the published sources were restored from exact-commit
Actions checkouts. The uncommitted earlier history implementation could not be
recovered. This reconstruction was frontend-checked and the complete Activity
was emitted through android-NDK `b0f135d7d3536093874fdcae62cf57520f6f1af3`.
Its Text-ordering repair has fresh independent API 29/34 ART qualification in
android-NDK run 41263805784. That qualification does not accept this new reader.

`HistoryProjection` exports only primitive observations of the maintained
transitions. ai-ci owns independent literal expectations for Back, Forward,
branch replacement, parsing, source identity and generation bounds. The
producer compiles mutations that retain forward references, accept an invalid
cursor, and admit a source generation outside the platform range.

`test_reader_index.c` exercises the actual native metadata owner with linked
short-write, no-space, interrupted-write, file-sync, rename and directory-sync
faults; corrupted digest, aliases, permissions, torn metadata and legacy
migration. Its compiled digest-check bypass failed at the intended previous
head assertion. Fresh host execution passed with qualified ICK source
`fbe86e23d55cfec2000c08e61deea2a407fd7175`. The complete maintained projection,
search receiver, Activity, numeric casts, Prepaint, and history consumers emitted
DEX and independently decoded with Baksmali 3.0.10. Repeated Prepaint/history
outputs were identical; all seven source mutations emitted distinct decoded
candidates. This does not yet demonstrate their rejection under ART.

New API 29/34 semantic execution, Android UI and process-loss tests are pending.
No prior uncommitted test result qualifies this reconstruction. TAB_P10
acceptance remains blocked by missing device access.
