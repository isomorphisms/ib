# Decode retained bytes without changing source admission

`IB.SourceDecoding` owns the bounded HTML byte-decoding step. Its first decoder
implements the WHATWG windows-1252 index and the 17 HTML labels which select it,
including ISO-8859-1, Latin1 and ASCII. It does not reinterpret a label based on
whether the particular bytes happen to form valid UTF-8.

ASCII whitespace is trimmed from labels and ASCII case is ignored. NBSP is not
ASCII label whitespace. Every source octet has one Unicode scalar in this
index, including the five C1 slots sometimes treated as undefined by other
libraries. NUL, CR/LF and those C1 scalars remain present; HTML preprocessing,
recovery and extraction have different owners.

A recognized UTF-8/UTF-16 BOM selects a different HTML decoder. This bounded
implementation explicitly refuses those BOMs rather than decoding their bytes
under a conflicting windows-1252 label. UTF-8/UTF-16 decoder support and the
full HTML encoding-prescan algorithm remain future work.

## Binary command and disposable output

`SourceDecodingCommand.idric` reads actual bytes using the existing binary
Buffer/file interfaces. It checks file size before allocation, requires the
actual read count to match, probes EOF, and writes actual UTF-8 bytes through
a binary buffer. Input is bounded to 4 MiB; it still holds the whole bounded
source and decoded value. This is not streaming ingestion.

The native Grease adapter `bin/ib_decode_html_bytes.grease` takes an explicit
compiled decoder, a source descriptor TSV, raw source path and a new disposable
destination directory. Inputs and executable paths must be absolute. A source
descriptor is bounded key/value data with distinct keys and these required
fields:

| Field | Meaning |
| --- | --- |
| `source_key` | Existing caller-supplied source reference |
| `source_generation` | Existing caller-supplied version reference |
| `sha256` | Expected original-byte SHA-256 |
| `byte_count` | Expected original-byte count, at most 4 MiB |
| `charset` | Original declared HTML encoding label |
| `navigation_admitted` | Original YES/NO observation, never changed here |

Additional well-formed fields are opaque provenance and are retained exactly.
These are source claims supplied by the caller, not authentication or proof of
an Arena reopen, current generation or permission to navigate.

The adapter snapshots the bounded descriptor and raw input, verifies original
count/digest, requires producer success and consistent decoder counts/identity,
and checks that neither frozen input changed during decoding. It stages beneath
the verified destination parent and publishes by a no-clobber same-filesystem
directory rename. A failed producer or conflict cannot replace an earlier
derivative. Static path checks do not promise containment against concurrent
filesystem-topology replacement or power-loss durability.

The output directory contains:

- `source.tsv`: the exact supplied descriptor, including the original charset
  and navigation observation;
- `decoded.utf8`: disposable derived bytes;
- `decode.tsv`: source and descriptor hashes, decoder identity, selected
  encoding, output hash/count, decoded codepoint count and explicit
  `reader_admission=NOT_GRANTED`;
- `complete`: the derived metadata digest.

The raw body is not duplicated into another persistent store. Byte offsets in
the original source and decoded Unicode codepoint offsets are different units.
The original `HtmlEvidence`, native source owner and Arena admission paths are
unchanged; this receipt is not one of their accepted admission schemas.

## Specification and acceptance

The independent normative index fixture retains WHATWG identifier
`e56d49d9176e9a412283cf29ac9bd613f5620462f2a080a84eceaf974cfa18b7`
(2024-09-18) from:

- https://encoding.spec.whatwg.org/#names-and-labels
- https://encoding.spec.whatwg.org/#single-byte-decoder
- https://encoding.spec.whatwg.org/index-windows-1252.txt
- https://html.spec.whatwg.org/multipage/parsing.html#determining-the-character-encoding

`SourceDecodingChecks.idric` checks all 256 octets against that fixture, label
rules, special scalars and BOM refusals. `tests/source-decoding.grease` executes
the real binary command and publication adapter, checks bad source claims and
producer failures, and consumes the exact existing Google capture supplied by
absolute path. `tests/source-decoding-mutations.grease` compiles and rejects
actual naïve-Latin1 and undefined-byte-replacement decoders against the same
normative oracle. These are host decoder/process results, not Android evidence.

The retained Google capture from search source
`dcbd86c0f7a69c4e0b381c0ab53f7c41f9966900` has 93,166 ASCII bytes, SHA-256
`f71e991ab87271e77255fe2d2454cb4cb401fe47cdf953ea550c6e0fcd8afeb4`.
Its derivative is therefore byte-identical, while the declared charset remains
ISO-8859-1 and redirect count remains one. The test descriptor explicitly uses
a fixture identity, not an invented live Arena generation. The missing redirect
chain and native reader/DEX connection still prevent joined navigation
acceptance. Decoding does not make those conditions true.
