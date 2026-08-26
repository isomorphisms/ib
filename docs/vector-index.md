# Readable filesystem vector spaces

IB's first vector backend is a flat exact scan whose source of truth is ordinary fixed-width text. It is deliberately a file tool, not a database. A row-major little-endian Float32 sidecar is a rebuildable query cache, not the only copy of the vectors.

Several embedding models may materialize independent ways of organizing the same collection. Categories and vector spaces are siblings below `organizing-the-information`; no model is promoted to the one true representation.

## Boundary

Canonical URLs, visits, extracted text, and document identities remain ordinary inspectable browser records. Embeddings and their index are derived state and can be deleted and rebuilt.

Idriç owns the index specification: collection, embedding model, dimensions, metric, and selected backend. `IB.VectorIndex` lowers that specification to a versioned process contract. Grease or thin platform glue may run the selected program. The initial program is `ib-vector-index`, implemented in C99 so the same source builds for Linux and the Android NDK.

A replacement backend must implement the same standard-input/standard-output contract:

```text
BACKEND build INDEX_DIRECTORY DIMENSIONS cosine|dot < rows.tsv
BACKEND query INDEX_DIRECTORY RESULT_COUNT < vector.txt
BACKEND query-text INDEX_DIRECTORY RESULT_COUNT < vector.txt
BACKEND column INDEX_DIRECTORY ONE_BASED_COORDINATE
BACKEND compile-cache INDEX_DIRECTORY
BACKEND check INDEX_DIRECTORY
BACKEND inspect INDEX_DIRECTORY
```

Build input has one row per line:

```text
document-id<TAB>0.1 -0.2 0.3 ...
```

Query input is one space-separated vector. Query output is score-descending text:

```text
document-id<TAB>0.8125
```

`query` memory-maps the Float32 cache. `query-text` performs the same exact scan by parsing the readable file and works with the cache removed. `column` uses the fixed-width layout to print one coordinate across every ID. `compile-cache` atomically recreates the Float32 file from text. A later exact or approximate program can occupy the same process boundary without changing canonical browser state or callers that stream rows and queries.

## Files

One model-specific view lives below the derived view root:

```text
views/organizing-the-information/vector-spaces/<embedding-model>/
  embedding-model.txt
  <collection>/
    format.txt
    ids-<generation>.txt
    vectors-<generation>.txt
    vectors-<generation>.f32
```

`embedding-model.txt` is the exact pinned model manifest copied once beside its collections; the materializer refuses to bind the same slug to different model facts. Each collection's `format.txt` is the atomic pointer to one immutable vector generation. It records contract version 2, backend, metric, dimensions, row count, text encoding, text slot width, and current filenames. IDs and authoritative vectors remain text. The `.f32` file is row-major little-endian IEEE 754 Float32 derived from that text.

Every scalar occupies exactly 16 ASCII bytes: a 15-byte signed scientific number with eight digits after the decimal point, followed by a space or the row-ending newline. For example:

```text
+1.00000000e+00 +0.00000000e+00 -2.50000000e-01
```

The format uses nine significant decimal digits, enough to round-trip a finite Float32. For zero-based row `r`, zero-based coordinate `c`, and `D` dimensions, the scalar begins at:

```text
(r * D + c) * 16 bytes
```

There is therefore no auxiliary transpose index to understand or rebuild. A direct seek finds any scalar; `column` applies that arithmetic across rows.

New builds write new generation files and replace `format.txt` last. A failed build therefore cannot make a partial generation current. Old generation files may be removed during serialized index maintenance after active queries finish.

`check` validates every text slot and proves the cache contains the identical Float32 values. Ordinary cached queries only check structure and file sizes before scanning, so they do not pay for a redundant text parse. Deleting the cache does not lose the vector view: `query-text` continues to work and `compile-cache` restores it.

## Metric and precision

`cosine` normalizes stored and query vectors once and then uses a dot product. Zero, NaN, and infinite vectors are rejected. `dot` stores the supplied values without normalization.

Similarity accumulation and the cache use 32-bit float. The text encoding preserves each stored Float32 exactly while remaining inspectable and interoperable. The rebuildable boundary lets a later backend use another cache representation without migrating the readable vectors.

## Initial model views

The initial pair is intentionally heterogeneous:

| View | Mechanism | Pinned inference file | Coordinates | Purpose |
| --- | --- | ---: | ---: | --- |
| `potion-base-2m` | static token lookup, mean, normalize | 7,563,349-byte ONNX | 64 | very cheap always-on semantic view |
| `mxbai-embed-xsmall-v1-int8` | INT8 transformer, mean pool, normalize | 24,448,010-byte ONNX | 384 | retrieval-oriented challenger |

Their manifests under `models/embedding/` pin the Hugging Face repository, immutable commit, preprocessing, every required file's size, and every required file's SHA-256. `bin/ib_embedding_model.grease` fetches and verifies those artifacts. `bin/ib_vector_view.grease` binds a manifest to its readable vector directory and invokes the backend with the manifest's dimensions. The disposable ONNX adapter under `experiments/embedding-models/` has run both models through this index contract; it is evidence for model selection, not a new Python browser runtime dependency.

At 10,000 rows, the 64-coordinate view occupies 10.24 MB as readable text plus a 2.56 MB cache. The 384-coordinate view occupies 61.44 MB as text plus a 15.36 MB cache. One 384-coordinate exact query performs 3.84 million coordinate products. These sizes remain small enough that a graph database would add opaque persistent state and approximate behavior before this corpus needs either.

## Growth path

The text scan is the correctness reference for the Float32 cache and for any approximate replacement. Add an ANN cache only after measurements on the phone show exact-query latency or corpus size is actually a problem. Such a cache must remain disposable and reproducible from the readable vectors.
