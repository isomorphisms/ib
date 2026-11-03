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

This slice is not browser-assisted acquisition. Rendered DOM provenance and
admission require a separate adapter; no browser content is relabeled as an ICU
body. Compilation, semantic controls, Android and live Google acceptance are
PENDING until actually executed. The first concrete language/runtime failure
must be recorded here without selecting another backend.

First producer41571281718 at source3e2938ee failed before DEX emission:
`IB.WebSearch.Results:229` reported `Expected 'else'`. The continuation inside
the `do` block needed deeper layout indentation. The existing source was
preserved in the failed commit; the current tree repairs that layout and
qualifies the pure semantics with independent compiled controls before the
larger DEX suite. This is a source correction, not a backend gap or acceptance.
