# Google result semantics over retained source

`HtmlPrepaintResult → Either Text (Maybe GoogleResults)` consumes an already
admitted ICU response and its maintained recovered document log. Non-search
documents retain their ordinary projection. A Google search must yield actual
titled external destinations; retry, consent, challenge and navigation links
cannot become results. No HTTP client or HTML tokenizer is introduced here.

Each `GoogleResult` retains its source key, generation, digest, recovered anchor
occurrence, original href, decoded destination, title and optional snippet.
The occurrence belongs to that exact retained log; it is not an original-byte
offset. Original Arena bytes and their verified ranges remain immutable. The
native result projection is explicitly a derived representation, and exact
selection copies its decoded title/URL text rather than invented network
coordinates. Reflow and Back/Forward reconstruct it from the original source.

The request query is decoded once from IB's retained, canonical request URL.
Google redirect wrappers are independently decoded once as query values. The
existing strict UTF-8 percent decoder is reused. No provider endpoint, result,
snippet or missing original-byte range is invented. Snippets remain absent
unless a source association is implemented and verified.

Effects remain in the existing Android adapter and acquisition worker. The
checked result model and projection are pure. Native UI search uses the same
query validator and existing background navigation path as ChordReader.

The initial slice did not provide browser-assisted acquisition. The current
integration adds a separately identified Google WebView adapter and immutable
DOM receipt. No browser serialization is relabeled as an ICU body. The checked
model is `BrowserDomEvidence → Text → Either Text HtmlPrepaintResult`; an opaque
redirect becomes a result only after its original anchor and retained ICU
response qualify through `verify_google_destination`. Ordinary destinations
still use the existing ICU/Binder/Shizuku path and canonical Arena.

First producer41571281718 at source3e2938ee failed before DEX emission:
`IB.WebSearch.Results:229` reported `Expected 'else'`. The continuation inside
the `do` block needed deeper layout indentation. The existing source was
preserved in the failed commit; the current tree repairs that layout and
qualifies the pure semantics with independent compiled controls before the
larger DEX suite. This is a source correction, not a backend gap or acceptance.

Local replay of the verified producer compiler now passes ten maintained
result assertions and the independent observer's eleven assertions. All three
independently compiled mutants execute and fail their designated assertion
(heading semantics, first-party control refusal and source generation).
These receipts are host execution, not ART or live Google acceptance. The
second hosted attempt stopped because `rg` was used before its installation;
the workflow now installs it before running that observer.

The fresh ICU matrix41573122647 completed at ICU4f365/ai-ci aef9dd. Every one
of six controlled legitimate requests returned HTTP200 and an identity-encoded
ISO-8859-1 HTML entity, but the independent classifier found zero qualifying
public results. The HTTP/1.1 entity is92433 bytes, chunked, with one301 redirect
to www.google.com. All six semantic outcomes are JAVASCRIPT_REQUIRED. A
successful matrix job means characterization completed, not search success.

The independent resolver41580675104 at ai-ci410974fd followed three exact
opaque Google hrefs from retained API29 browser DOM. ICU observed302 redirects
to real public HTTPS HTML destinations. This supports browser-assisted
integration, while ICU-only Google result acceptance remains incomplete.

Local host replay now passes the original ten result assertions and sixteen
browser-source/destination controls. The first browser-wire assertion assumed
anchor occurrence0; the maintained recovery log numbers events, so the check
now retains the actual recovered occurrence. The first attempt is preserved in
scratch receipt ib-local-dom-model-6; ib-local-dom-model-7 passes. This repairs
the observer's assumption without changing producer extraction or admission.

The checked native Activity and typed WebView callback declarations compile
through the pinned DEX backend and independently disassemble in
ib-local-dom-activity-3. This is no ART, native C, network or view acceptance.
The new native platform adapter, canonical DOM owner and full integrated
Google transaction remain PENDING until their hosted qualification executes.
The C FD boundary preserves the existing bounded read-only FIFO; an initial
sealed-memfd proposal was rejected during source review because the maintained
reader deliberately admits only that FIFO contract. Its admission was not
weakened. Native C continues to require ICK and the maintained NDK linker.

`verify-activity.grease` reproduces local checked Activity/typed callback DEX
and independent disassembly without needing an SDK or pretending to run ART.
No Java/Kotlin, candidate smali, second network implementation or parallel
history store is introduced. Snippets remain absent until source-backed
association is implemented; titles and verified destinations are retained.

An explicit native-layout counterexample in `ib-local-url-layout-before`
compiled and failed `complete result URL wraps within the native viewport`.
The normal prose break provider supplies no cut inside an entire destination
URL. `IB.Reflow.Address` now supplies bounded ASCII source cuts, preserving
percent triplets and every original address character, to the same advance
solver through `reflow_at_breaks_advances`. Prose's default break rules remain
unchanged. Both Greedy and Squared pass exact-copy and viewport assertions in
`ib-local-url-layout-after-1`. The fixture is not a live result.

Source review also found the existing ReaderProjection helper still expected
HTTP charset/UTF-8 fields on the new common identity. It now reads those fields
from its already parsed original HtmlEvidence, preserving the prior observer
wire without manufacturing HTTP observations for a DOM source.
