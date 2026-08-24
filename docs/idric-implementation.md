# Idriç implementation boundary

IB is an Idriç application.

The browser-owned layer is written in `.idric` and owns durable tab/history identity, ordering, sleeping policy, storage classification, index policy, inspection semantics, and renderer selection. A renderer or Android boundary may expose platform facilities, but it must not become the source of truth for those decisions.

The initial source modules deliberately keep the executable boundary small:

- `IB.History` owns normalized history values, import ordering, and stable newest-first chronology.
- `IB.Index` builds transparent rebuildable list indices without collapsing duplicate visits.
- `IB.Storage` classifies schema-shaped paths and defines which records may be generically inspected.
- `IB.FileStore` performs the first real browser-owned file I/O: it creates the store/tab directories, reads and writes tab manifests, appends history records, rejects paths outside the canonical schema, and reports filesystem failures.
- `IB.Inspect` summarizes physical rows without following or interpreting renderer state.

The first slice does not embed Python, Ithon, WebView UI, or a renderer. Chrome/Firefox SQLite import and Android filesystem walking are platform adapters to add around this core, not reasons to move the core out of Idriç.

`IB.FileStore` deliberately does not yet claim crash-safe replacement. Its manifest write uses the ordinary Idriç file API, and its history write uses append mode. Atomic replacement, flushing, and restart recovery need a narrow native boundary plus fault-injection tests; they are the next durability layer, not properties of this first file-I/O slice.

## Native boundary

A future Android inspector should ask native glue only for bounded metadata/read operations that need kernel enforcement. In particular, rooted no-follow opens belong at the Linux/Android boundary. Classification, visibility, paging policy, and presentation data remain Idriç decisions.

## Build

The CI job builds the current `isomorphisms/Idric` compiler and then compiles `src/Smoke.idric` and `src/FileStoreSmoke.idric`. The ordinary smoke executable exercises history ordering, duplicate preservation, indices, storage classification, protected reads, and inspector readability. The file-store smoke runs as two separate processes: the first creates a tab and appends two history records, and the second reloads the same files to prove they survive a process restart.
