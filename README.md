# ib 0.2

> browsers don't have bookmarks.

The 0.2 line starts again from the persistent information underneath a browser rather than from a renderer, Android application, or large typed browser core.

The first implementation is deliberately shell-first and arXiv-only.

## Cauldron and Pensieve

A fetched thing first enters the **Cauldron**.  The Cauldron is intake: original HTML, PDF bytes, fetched figures, source URLs, and acquisition time.  It may be messy, but it should retain enough source material to reinterpret later.

The **Pensieve** is the next layer.  Material distilled from the Cauldron becomes locally searchable text and small metadata/relationship files there.  Indexes are derived from the Pensieve and must be rebuildable.

```text
arXiv
  |
  v
ICU + shell
  |
  v
Cauldron
  |  original HTML / PDF / figures / provenance
  v
distill
  |
  v
Pensieve
  |  searchable text / title / links / figure manifest
  v
indexes and later model hooks
```

A tab or renderer is a temporary view onto this persistent state.  Neither is part of the 0.2 acceptance boundary.

Two future reading contracts are recorded separately:

- [`docs/reading-feedback.md`](docs/reading-feedback.md): opening is not reading, passive interaction is evidence rather than proof of comprehension, and completed items may deliberately remain in a rereading/reminder rotation.
- [`docs/reading-assistance.md`](docs/reading-assistance.md): the Pensieve should support interrupted and question-driven reading, fiction recall without spoilers, corpus triage for papers/repositories, and replaceable source-backed model-generated guides/fragments/strands rather than optimizing for finishing long reads.

## Commands

```sh
bin/ib fetch https://arxiv.org/abs/2203.11355
bin/ib distill 2203.11355
bin/ib add https://arxiv.org/abs/1901.09021 https://arxiv.org/abs/2305.00241
bin/ib search 'Bergman kernel'
bin/ib reindex
bin/ib paths
```

`add` is just `fetch` followed by `distill`, then an exact-text index rebuild.

By default persistent data lives under `${XDG_DATA_HOME:-$HOME/.local/share}/ib`.  Set `IB_HOME` to put the whole experiment somewhere else.  Set `IB_ICU` to the ICU executable when it is not on `PATH` as `icu`.

The first arXiv corpus is in `tests/fixtures/arxiv-0.2.urls`.

## Host tools

The acquisition path requires ICU.  HTML extraction prefers `xmlstarlet` and has a deliberately crude shell fallback.  PDF-to-text extraction uses `pdftotext` when available.  Missing `pdftotext` does not prevent the HTML-backed Pensieve entry from being created.

This is not a claim of general Web compatibility.  The only site-specific adapter in 0.2 is arXiv.

## Derived indexes and hooks

`bin/ib reindex` currently builds the simplest possible exact-text index: a sorted file list over Pensieve text representations.  `bin/ib search` searches those local files; it does not return to the Web.

After an item is distilled, executable files under `$IB_HOME/hooks/after-distill.d/` are called with the Pensieve item path and its Cauldron source path.  This is intentionally a small process boundary for later vector spaces, hyperplanes, other indexing methods, model-context adapters, and language-model-generated reading guides.  Derived systems may append replaceable, provenance-linked artifacts; they do not own or silently rewrite the Pensieve, source evidence, human annotations, or reading observations.

## Earlier work

The older Idriç browser core, Android prepaint, scientific-media, and workbench material remain on this branch as reference while 0.2 is established.  The active 0.2 path does not depend on them.  They can be converged or removed after this smaller shell architecture proves itself.
