# IB-C67 HTML → typed Prepaint Version 1

Status: **DRAFT / NOT QUALIFIED** until an actual Idriç execution, independent
consumer and source admission have succeeded. This is the first complete-revision
conversion only. It is not a C67 URL-browser acceptance receipt.

## Owners and immutable starting points

- Semantic base: PR #148, `work/linux-shared-contract`, originally
  `2425d21d984de21e5b65ae0d2e09c586120239ee`.
  This includes the types and existing serializer from #48/#123.
- Upstream Arena producer: PR #152, original head
  `cc5f64b84c316b88d4c5c67fe4ea7ee5fa1b9bbe`.
  `bin/ib_android_arena_read.grease` supplies a digest-checked range but its
  receipt has `resolved_url=UNOBSERVED`, `mime_type=UNOBSERVED`,
  `http_status=UNOBSERVED`, `navigation_admitted=NO`, and no pinned source
  generation or charset. **No live HTML can pass source admission yet.**
- Separate reader-handoff owner: PR #153, original head
  `9cf28c2f6c38cb12bb906a844e061ebfab34f943`.
- C67 native Material reader: PR #129. It currently paints a pinned snapshot.
- ICU final URL/redirect evidence: `dilapidated-shed/icu` PR #35.
- The browser-foundation CI pins Idriç
  `7050c116326c9c8e8d0a56c2db7baf2119cbae19`.
  The underlying runner's compiled executable digest must be recorded from the
  *actual* artifact; the source pin is not an executable receipt.

## Byte and semantic boundaries

1. A source owner must commit the original bytes, source generation, requested
   and actual ICU-resolved URLs, status, content type, charset, byte count and
   source digest. Unobserved or stale metadata is **not admitted**.
2. Grease reopens and checks the exact readback bytes and SHA-256 and validates
   the UTF-8 encoding. The source key/generation must match the browser-owned
   navigation expectation. It must not mint a second persistent source store.
3. The Idriç converter consumes an admitted immutable HTML string through
   `IB.RecoveredInformation` and `IB.Information`, maps ordered items into
   `IB.Strand`, then calls `prepaint_from_strand` and the existing
   `IB.Prepaint.Version1.serialize_version1`.
4. A newly emitted complete Version 1 stream is checked by a *different* C11
   consumer and replaces the last valid disposable projection only after
   success. A rejected source or converter failure never changes the prior
   projection.
5. Display width is not an input to semantic extraction; links remain navigation
   candidates, never reader-fetch permissions. The Android reader/Binder/PFD
   adapter must independently bind the publication to the retained source and
   currently remains **NOT_RUN**.

The Version 1 wire schema does **not** contain a source digest/generation. The
`HtmlPrepaintResult` retains that metadata and the Grease receipt identifies
the projection separately. The reader must keep that binding at handoff; a
standalone `.prepaint` file must not be promoted to canonical source identity.

`bin/ib_emit_html_prepaint.grease` consumes one thirteen-field source admission
receipt and an **already verified** body path, then requires an exact expected
source key and `IB_EXPECTED_SOURCE_GENERATION`. The receipt is authority only
when supplied by the trusted source-store owner, not by an app/remote page.
The `fixture` kind is prohibited unless `IB_ALLOW_FIXTURE=1`; tests always
label fixtures. This adapter does **not** grant Arena metadata qualification:
PR #152 must be reconciled before an Arena-owner receipt can be supplied.

## Supported now / explicit limitations

- Six distinct headings, nested inline text, paragraphs, lists as ordered text
  blocks, table rows, basic character references, relative/absolute/query/
  fragment links and a valid HTML base URL.
- Missing closing heading/paragraph/list/cell/row tags are passed through the
  existing recovered document log.
- Forms are visible descriptions with **no submission**. Script, style,
  canvas, video, SVG and other dynamic elements are excluded or noted.
- Images have resolved acquisition-request URLs, alt text and available
  title/figure captions. Without separate image admission only a text
  placeholder is serialized. **No remote image URI is emitted for the reader
  to fetch directly.**
- Not a full HTML5/CSS/JavaScript/DOM parser. Unsupported charsets, unsafe
  schemes and an unobserved final URL cause refusal or explicit limitations.
- In-place width reflow and selection identity need graphical reader acceptance;
  typed source-linked blocks alone are not that acceptance.

## Acceptance matrix

| Check | Authority | Current status |
|---|---|---|
| Idriç typecheck + actual executable | IB CI | NOT_RUN until workflow completes |
| Offline example.com-shaped fixture | Idriç + independent C11 oracle and frozen golden | NOT_RUN |
| Offline arXiv-shaped fixture | Idriç + independent C11 oracle and frozen golden | NOT_RUN |
| Bad digest, generation, source ID, UTF-8 and omitted/stale metadata | Grease refusal, last-valid byte comparison | NOT_RUN |
| Mutated heading, link, dropped text and malformed escaping | Independent oracle/golden | NOT_RUN |
| Real ICU HTTPS → committed Arena → admitted metadata | ICU PR #35 + IB PR #152 | BLOCKED |
| Binder/PFD delivers new projection to reader | IB PR #153 | NOT_RUN |
| Physical C67 paints example.com and navigates links | User-observed device + receipts | NOT_RUN |
| ARM32 A1/C67, ARM64 C67 converter execution | Cat Food/CI evidence | NOT_RUN |

Keep the PR draft and task open until independently admitted retained HTML
actually produces a valid complete prepaint. Do not label an offline fixture live.
