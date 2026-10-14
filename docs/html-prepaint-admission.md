# IB-C67 HTML → typed Prepaint Version 1

Status: **host fixture converter QUALIFIED / retained Arena HTML BLOCKED / physical C67 NOT_RUN**. Executed [IB CI run 38653082213](https://github.com/isomorphisms/ib/actions/runs/38653082213) passed both `idric-core` and `scientific-media` on `d228f78b00e56a34a93f9f58ca908977ef1c1249`; the pinned separate `isomorphisms/ai-ci@0fb20839a2da68be0aafc10511e13c634bfa02b9` independently consumed the actual generated fixture bytes and ran its wrong-output self-tests. [Retained artifact 11935167962](https://github.com/isomorphisms/ib/actions/runs/38653082213/artifacts/11935167962) SHA-256: `acfe20c464eb4955a75fd5f27a31f038dcaa806c1936557f01dc3e1a5b20f9c6`. These are offline fixtures, **not live ICU network data or a C67 device receipt**.

## Owners and immutable starting points

- Semantic base: PR #148, `work/linux-shared-contract`, originally
  `2425d21d984de21e5b65ae0d2e09c586120239ee`.
  This includes the types and existing serializer from #48/#123.
- Upstream Arena producer and verified source reader: PR #152, whose
  Version 2 candidate was pinned at
  `e2a2a17a4378b7c3845ec43f42d1d3360b12f051`.
  It now consumes ICU #35's *paired same-request* response body and actual
  metadata, validates source key/generation, HTTP status, MIME/charset,
  response completeness, final URL and SHA-256, and independently reopens the
  committed canonical Arena range. Its legacy v1 body-only receipt remains
  permanently navigation-denied. Its v2 host test passed at
  [run 38652958137](https://github.com/isomorphisms/ib/actions/runs/38652958137),
  **with mocked Shizuku/ICU transport**, not actual C67 HTTPS. The separate
  converter now has a pinned cross-branch mock-integration test for the same
  v2 producer/reader and the actual Idriç command; that test is not yet a
  live device acceptance claim.
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
label fixtures. For a v2 Arena source, this adapter passes through the
**unchanged pinned** #152 producer/verified reader. Mocked metadata proves only
contract compatibility, never live acquisition or Android reader delivery.

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
| Idriç typecheck + actual executable | IB CI #38653082213 | **PASS**, Linux/host fixtures |
| Offline example.com-shaped fixture | Actual Idriç output + independent C11/AI-CI oracle and frozen golden | **PASS** |
| Offline arXiv-shaped fixture | Actual Idriç output + independent C11/AI-CI oracle and frozen golden | **PASS** |
| Bad digest, generation, source ID, UTF-8 and incomplete admission | Grease refusal, last-valid byte comparison | **PASS**, fixture-only |
| Mutated heading, link, dropped text and malformed escaping | C11 oracle + independent AI-CI + golden | **PASS**, fixture-only |
| Mock ICU metadata → actual canonical Arena v2 → Idriç V1 | Pinned PR #152 + canonical appendfat + AICI | COMMITTED / exact-head CI queued |
| Real ICU HTTPS → committed Arena → admitted metadata | ICU PR #35 + IB PR #152 | NOT_RUN on physical C67 |
| Binder/PFD delivers new projection to reader | IB PR #153 | NOT_RUN |
| Physical C67 paints example.com and navigates links | User-observed device + receipts | NOT_RUN |
| ARM32 A1/C67, ARM64 C67 converter execution | Cat Food/CI evidence | NOT_RUN |

Keep the PR draft and task open until independently admitted retained HTML
actually produces a valid complete prepaint. Do not label an offline fixture live.
