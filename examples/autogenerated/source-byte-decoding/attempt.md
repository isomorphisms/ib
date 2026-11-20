# Program attempt: source-byte-decoding

Status: PASS for bounded host decoding and process checks; integration partial.

## Purpose and intended behavior

Decode a retained HTML source into a disposable UTF-8 representation without
changing its original bytes, declared charset, generation or navigation
admission. The current Google observation declares ISO-8859-1; HTML uses the
WHATWG windows-1252 decoder for that label. Decoding alone cannot admit its
redirected navigation or join the Android reader.

## Type-system sketch

The input domain is octets, not already-decoded Text. `SourceOctet` names the
existing Bits8 raw boundary. `HtmlDecodedText` contains the exact supplied
label, selected encoding, original octet count and decoded Unicode text.
`decode_html_bytes : Text → List SourceOctet → Either Text HtmlDecodedText`
is pure. The 17 windows-1252 labels use ASCII whitespace/case rules. All 256
octets have defined mappings; CR/LF, NUL and C1 values remain unchanged where
the index specifies them. HTML preprocessing belongs to the existing recovery
owner. Recognized UTF-8/UTF-16 BOMs explicitly refuse in this bounded decoder
instead of being misread as windows-1252.

The command uses the compiler's binary Buffer interfaces, checks a bounded file
size, the actual read count and EOF, then writes UTF-8 bytes through a binary
buffer. Input is bounded to 4 MiB; this is not a streaming-ingestion repair.
The command reports codepoint count separately from byte count.

Grease owns source/derivative hashing, immutable input snapshots, producer exit
checking and publication of a private staged directory by same-filesystem
rename. It retains a caller-supplied source descriptor byte-for-byte, including
source key, generation, original digest/count, declared charset and original
navigation admission. Opaque extra descriptor fields are preserved as data.
The derived receipt always says `reader_admission=NOT_GRANTED`; it cannot be
used as an original UTF-8 `HtmlEvidence` or as an Arena admission receipt.

## Specification and runtime boundary

Primary specification: WHATWG Encoding names/labels and single-byte decoder;
windows-1252 index identifier
`e56d49d9176e9a412283cf29ac9bd613f5620462f2a080a84eceaf974cfa18b7`
(2024-09-18), retained independently as a test fixture. In particular, 0x81,
0x8D, 0x8F, 0x90 and 0x9D map to their C1 scalars; they are not replaced or
rejected. HTML's BOM precedence is preserved by explicit refusal until BOM
decoder paths exist.

Use the verified Linux x86-64 Idriç host bundle at compiler commit
`94dfd99bd3e376507fedc8611053b7173b2519f0`, archive SHA-256
`0be57deab9410e8d4c09c7198bf7e8ee0ec3e24121f76640147efa4a53d120da`.
No RefC, iconv, Python or Java decoder is a runtime substitute.

## Planned acceptance

Compile and execute actual Idriç code against all 256 bytes and the independent
normative index; alias/ASCII-whitespace and BOM refusals; NUL/CRLF/Euro/quotes/
café controls; real captured Google body identity; bad source count/hash and
nonzero producer publication refusal. Known-bad Latin1 and replacement mutants
must fail the same oracle. Preserve original admission and metadata.

## Result, fallback and remaining boundary

Fallback: none. DEX/ART,
Android, canonical Arena admission, joined Prepaint and physical devices remain
NOT_RUN. The existing native redirect-provenance and charset gates are unchanged.

## Executed result

The production module and binary-file command compiled with the exact verified
94df host bundle. A fresh source staging directory received the current `src/`
tree plus `SourceDecodingChecks.idric`, then executed these compiler arguments:

```text
SourceDecodingCommand.idric -o source-decoder
SourceDecodingChecks.idric -o source-decoding-checks
```

`bin/idric` and the execution wrapper `bin/idric-env` were the absolute
entrypoints inside that verified bundle, not ambient commands. Native Grease
was the verified installed runtime with binary SHA-256
`7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`.
Its inherited LeakSanitizer could not inspect this host's process-task data;
`ASAN_OPTIONS=detect_leaks=0` was explicit for orchestration execution. Leak
analysis is not an accepted result.

Actual execution of the compiled checks and `tests/source-decoding.grease`
passed with exit zero. The normative all-octet check matched all 256 scalars.
The binary command produced exactly 401 UTF-8 bytes from the 256-octet fixture;
independent byte checks retained the entire 128-byte ASCII prefix (including
NUL and CR/LF) and verified Euro/C1, NBSP and café UTF-8 octets. Alias, ASCII
trimming, BOM, unknown-label, size-bound, empty-input and input-as-output
controls passed.

Native process tests refused wrong source count/hash, a nonzero producer with
partial output, and a producer which changed the frozen descriptor. No accepted
derivative appeared; the earlier valid derivative remained exact. The real
93,166-byte Google observation round-tripped byte-for-byte; its original
ISO-8859-1 and redirect-count-one metadata were unchanged and the derived
receipt still grants no reader admission. The test uses an explicit fixture
identity rather than inventing a live Arena generation.

`tests/source-decoding-mutations.grease` compiled two actual changed Idriç
decoders in distinct temporary source roots. The naïve Latin1 decoder and the
five-C1-slot replacement decoder both failed the same normative all-256 scalar
assertion with nonzero exits. The real decoder passed that assertion.

The final fresh command and check-program native `.so` hashes, source hashes,
normative fixture and bounded outcomes are in `host-evidence.tsv`. Vocabulary
and whitespace checks passed. Independent qualification normalized the fixture
to the exact fetched primary bytes by removing an additional trailing newline,
then reran the semantic, process and compiled-mutant checks. Its digest is
`2c5fa192d566f92b8b9e6e116255b1f94016b4ece5b9dacb3b98be70b8c60795`;
the index identifier and every numeric mapping are unchanged. See
`moon-host.md` for the independent rebuild and publication controls.
Existing `HtmlEvidence`, canonical-store and native reader admission source
files are unchanged.

The continuation recheck compared the maintained module, command and semantic
check source byte-for-byte with the qualified staging tree, verified the
recorded native program digests, then re-executed the full semantic/process
controls and both freshly compiled mutant checks. They again passed with exit
zero on this Ubuntu 24.04 x86-64 host. This repeated execution does not upgrade
the Android, Arena or joined-navigation evidence boundary.

## Earlier test-helper failures and language work

The initial style-adjusted helper used Unicode `−`, which this 94df bundle
parsed but could not resolve. Its source is retained under `initial/`. A
subsequent reduced lookup attempt exposed that this pinned `Data.List` does not
export the assumed `elemIndex`; that rejected assumption is also retained.
The maintained helper uses a simple descriptive digit-position lookup, still
entirely in Idriç. No external decoder or language substitution was introduced.

Those compiler failures were not counted as acceptance. Final verification
rebuilt from a fresh staging root and required successful compilation before
running the new executable; earlier generated executables were not carried
forward as final-head proof.

The first remaining integration boundary is a verified decoded representation
consumer which retains original byte identity and independently establishes
reader admission. This diagnostic does not remove the missing redirect chain,
implement bounded streaming, change the native original-UTF8 gate or execute
the decoder under DEX/ART. The language bundle's missing canonical subtraction
binding remains a separately identified compatibility gap; no language change
was necessary to deliver this decoder slice.
