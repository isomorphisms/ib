# Prefetch and the reading corpus

This document describes IBrowser/workbench state. It is separate from the `isomorphisms/ddg` DuckDuckGo Android browser fork.

## Two different stores

IB prefetching has two storage classes that must not be conflated.

### Disposable prefetch cache

The prefetch cache contains response bytes fetched before foreground navigation needs them. It is keyed by exact URL and may be deleted at any time.

Default location:

```text
${XDG_CACHE_HOME:-~/.cache}/ib/prefetch/
```

`IB_PREFETCH_DIR` can override it.

Clearing this cache must not delete browser history, canonical IB state, or the reading corpus.

### `~/reading`

`~/reading` is durable, ordinary filesystem content intended to be inspectable by people and queryable by a language model or other semantic tools.

Default location:

```text
~/reading/
```

`IB_READING_DIR` can override it for tests or another installation layout.

A prefetched response becomes part of `~/reading` only when browser/site policy says it is reading material. A generic speculative fetch is not automatically promoted.

## Prefetch policy

The Idriç core owns the resource-set policy. A caller supplies resources in priority order; `IB.Prefetch` removes duplicate URLs and enforces a resource budget. It does not invent links to crawl.

Grease owns HTTP requests, the disposable response cache, and filesystem materialization.

The intended renderer path is:

```text
renderer requests URL
        |
        +-- exact prefetched response exists -> use it
        |
        `-- no prefetched response -> ordinary network path
```

This lets the workbench fetch likely next resources without making a renderer or WebView own durable state.

## arXiv first policy

arXiv is deliberately more aggressive than a generic website. An arXiv HTML paper is a bounded document resource graph, so IB may prefetch the complete HTML document plus every figure resource explicitly referenced by the paper HTML. This is not treated as open-ended lookahead spidering.

For an arXiv identifier, the reading layout begins as:

```text
~/reading/arxiv/<identifier>/
  document.html
  source.tsv
  images.tsv
  unresolved_images.tsv
  images/
  image_tags/
```

The HTML and external figures also remain available in the disposable URL cache. The files under `~/reading` are the semantic corpus: the language model can query the paper text, captions, image manifest, and the figure files without depending on a live renderer cache.

If usable arXiv HTML is unavailable, the existing scientific-media policy may fall back to PDF.

## Workbench role

The developer workbench should eventually expose prefetch state separately from hot renderer state: queued, fetched/disposable, promoted-to-reading, and evicted. A large known URL universe must still not imply a large live renderer working set.
