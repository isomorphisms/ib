# Checked HTML fragment positions in the maintained reader

Purpose: close the existing native fragment-link gap without introducing a
second parser, navigation store or source-coordinate system. The same
Information collector now optionally observes id and legacy a/name markers
after processing each opening token. Ordinary extraction retains its previous
items; marker collection shares the capture/skip/whitespace/entity semantics.
Identifier matches precede named-anchor matches, duplicate identifiers choose
the first occurrence, and hidden/nontextual targets refuse explicitly.

Types: an InformationAnchor carries a decoded item occurrence, code-point
offset and supported-position observation. HtmlPrepaint maps the item prefix
through its existing image/caption folding to a retained block occurrence.
HtmlAnchors maps that checked source position through the actual SourceLine
plan to a UTF-16 display point. None of these derivatives identifies original
HTML bytes or grants source admission.

Fragments use strict once-only percent/UTF-8 decoding. Plus stays literal;
overlong sequences, surrogate scalars, values above U+10FFFF, incomplete
sequences and NUL refuse. Empty fragments and the conventional absent #top
target open the top. An actual id named top still wins. Unknown or unsupported
targets report an error while preserving the previous readable state.

The real checked NativeReader completion performs fragment lookup after
committed-source admission, extraction, reflow and selection validation. Its
native adapter binds a pre-draw listener to that exact immutable projection,
then asks the SDK to expose the checked point after layout. A listener for a
replaced projection cannot move the new one. Both class and Activity DEX use
the maintained direct compiler/writer; C implements SDK mechanics only.
Back/Forward and offline reopen derive the point again from the retained
requested URL and source; no additional database or journal format is added.
Same-document fragments currently follow ordinary acquisition rather than a
no-fetch optimization; that behavior must not be described as offline anchor
navigation until a retained occurrence exists.

The maintained HtmlAnchorSmoke includes literal source/item/display positions,
Unicode entities, named/id priority, duplicate ids, inline whitespace, hidden
and absent targets, strict UTF-8 bounds and percent-once controls. Its actual
command DEX, repeated deterministic bytes and separately compiled offset and
percent-erasure mutants are required on both API29/API34. The real Activity
also exposes the same admitted fragment consumer for an independent ART
observer. Native viewport/actual fragment click acceptance is a separate
required follow-up; pure coordinate success does not establish it.

Status at authoring: compilation, ART and native viewport checks NOT_RUN.
Physical TAB_P10 BLOCKED. No release, merge or physical evidence is claimed.
