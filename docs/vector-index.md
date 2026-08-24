# Filesystem vector index

IB's first vector backend is a flat exact scan over 32-bit floats. It is deliberately a file tool, not a database.

At the initial 10,000-URL workbench size, 384-dimensional vectors occupy 15.4 MB and one exact query performs 3.84 million multiply-adds. A graph index would add persistent graph state, tuning, and approximate results before this corpus needs them.

## Boundary

Canonical URLs, visits, extracted text, and document identities remain ordinary inspectable browser records. Embeddings and their index are derived state and can be deleted and rebuilt.

Idriç owns the index specification: collection, embedding model, dimensions, metric, and selected backend. `IB.VectorIndex` lowers that specification to a versioned process contract. Grease or thin platform glue may run the selected program. The initial program is `ib-vector-index`, implemented in C99 so the same source builds for Linux and the Android NDK.

A replacement backend must implement the same standard-input/standard-output contract:

```text
BACKEND build INDEX_DIRECTORY DIMENSIONS cosine|dot < rows.tsv
BACKEND query INDEX_DIRECTORY RESULT_COUNT < vector.txt
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

This interface does not expose the flat backend's private files. A later USearch or HNSW program can occupy the same boundary without changing canonical browser state or the callers that stream rows and queries.

## Files

An index lives below the existing derived namespace:

```text
state/indexes/vectors/<collection>/<embedding-model>/
  format.txt
  ids-<generation>.txt
  vectors-<generation>.f32
```

`format.txt` is the atomic pointer to one immutable generation. It records the contract version, backend, scalar, byte order, metric, dimensions, row count, and current data filenames. IDs remain text. Vectors are row-major little-endian IEEE 754 `float32` values.

New builds write new generation files and replace `format.txt` last. A failed build therefore cannot make a partial generation current. Old generation files may be removed during serialized index maintenance after active queries finish.

The generic inspector may read `format.txt` and generated ID text. It classifies the vector bytes as derived but does not treat them as generic readable text.

## Metric and precision

`cosine` normalizes stored and query vectors once and then uses a dot product. Zero, NaN, and infinite vectors are rejected. `dot` stores the supplied values without normalization.

Storage and accumulation both use 32-bit float. This is the natural precision of common embedding outputs, halves the vector bytes relative to doubles, and is sufficient for similarity ranking here. The rebuildable boundary lets a later backend use another representation without migrating canonical data.

## Growth path

The flat scan is also the correctness reference for any approximate replacement. Add an ANN backend only after measurements on the phone show that exact query latency or corpus size is actually a problem. USearch is the leading replacement candidate because it has a C API, Android support, `f32`, and disk-backed index viewing, but it is not a dependency of this first backend.
