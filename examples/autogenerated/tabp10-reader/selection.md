# Native source selection and link projection

The reader needs one display with selectable reflow, exact copying and links.
Android reports UTF-16 selection boundaries. Prepaint reflow ranges identify
decoded Unicode code points in immutable blocks. Original HTTP bytes remain
separately owned by Arena. Re-parsing the displayed text as source would copy
inserted newlines and discretionary hyphen glyphs incorrectly.

`SelectionProjection` is an opaque derived value containing decoded block text,
painted text, checked display/source rows and native styling regions. Between
blocks the decoded derivative has two newline separators. Within a block copy
returns the exact selected decoded substring, preserving hidden soft hyphens,
zero-width spaces, hard line endings and Unicode. Inserted wrapping glyphs
never enter copied text; their boundaries retain original spaces or hidden
controls at the break. A selected discretionary hyphen maps to its source soft hyphen. A
boundary inside a UTF-16 surrogate pair refuses. This is exact decoded text,
not a claim that copied text contains original HTML markup or original charset
bytes.

The ephemeral IBP1 coordinates are not a document store or admission receipt.
Their reader verifies continuous original ranges, bounds, canonical numbers,
complete coverage and each row's spelling through the maintained source reflow
owner. Rows are contiguous and displayed hard line endings retain their
original CR/LF, NEL, line-separator or paragraph-separator ranges. Links are
already resolved by HtmlPrepaint, then checked again as navigation requests.
Native Android owns SDK storage, selection handles, styling and clipboard
operations; checked Idriç owns source/paint coordinates and link decisions.

The first CLI build lacked a direct SourceBreaks import and refused with exit 1
and no DEX. That attempt is retained in the build log; adding the owner import
does not change semantics. The initial display projection did not give original
hard line endings a selectable displayed range. Mandatory-ending rows now give
one displayed newline the exact original ending, including CRLF. Separate
endpoint affinities preserve hidden characters inside an enclosing source
selection while excluding adjacent hidden controls from a selected word.

The native adapter passed strict host ICK syntax checking with unmodified
OpenJDK 17 JNI headers. This is a host C check, not an NDK ELF or Android test.
The typed native TextView declarations emitted and independently decoded through
the maintained writer. An actual Activity export alias exposed a reusable
backend defect; android-NDK PR 41 repairs closure saturation without rewriting
the aliases. Fresh frontend and DEX compilation passed for the complete Activity,
projection/search/numeric/Prepaint/history consumers, selection and their ten
compiled source mutants. All candidates independently decoded with Baksmali
3.0.10; repeated Prepaint/history/selection DEX matched byte for byte. Independent
ART selection controls and installed touch/clipboard/link checks remain required.
No native selection, link click, ART or physical-device pass is claimed here.
