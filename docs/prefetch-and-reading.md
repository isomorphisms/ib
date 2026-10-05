# Investigations, prefetch, and the reading corpus

This document describes IB workbench state. It is separate from the `isomorphisms/ddg` DuckDuckGo Android browser fork.

## Three different storage classes

IB must not conflate a durable intention to investigate, disposable fetched bytes, and a durable human-readable corpus.

### Durable investigation frontier

A task may remember roots, discovered links, source edges, priority, and unfinished work across browser shutdown and reboot.

Conceptually:

```text
state/investigations/<task-id>/
  task
  roots
  resource-edges
  frontier
  decisions
```

The exact grammar remains open. The invariant is that clearing cache does not make IB forget which documentation set or source graph it intended to learn from.

### Disposable prefetch cache

The prefetch cache contains response bytes fetched before foreground work needs them. It is keyed by resource identity or an exact request key and may be deleted at any time.

Default location:

```text
${XDG_CACHE_HOME:-~/.cache}/ib/prefetch/
```

`IB_PREFETCH_DIR` can override it.

Clearing this cache must not delete browser history, tabs, investigation frontiers, accepted organization, or the reading corpus. If a durable task is resumed later, missing disposable bytes can be fetched again.

### `~/reading`

`~/reading` is durable ordinary filesystem content intended to be inspectable by people and queryable by language models or other semantic tools.

Default location:

```text
~/reading/
```

`IB_READING_DIR` can override it for tests or another installation layout.

A fetched response becomes part of `~/reading` only when browser or task policy promotes it. A generic speculative fetch is not automatically durable reading material.

## Discovery is not a request

Knowing that a document contains an image or links to another document does not imply that its bytes have already been requested.

A resource may move through states such as:

```text
discovered -> queued -> fetching -> ready
                             `-> failed
```

`discovered` may remain unrequested indefinitely. Background policy, a foreground action, layout, painting, or a question may force the request. The discovered dependency and its eventual in-flight request are separate records even if an implementation represents lazy work with a memoizing promise.

This distinction lets an information renderer or extractor walk a dependency graph without starting every transfer. It also lets IB preserve uncertainty about future need without pretending that an in-flight network operation already exists.

## Resource reuse without history loss

Several tabs or documents may point at the same resource. IB may fetch and store reusable bytes once within a compatible authentication, cookie, request-header, and `Vary` partition while retaining:

- every referring edge;
- every tab or task that made the resource relevant;
- every visit or opening event;
- edge order and operative-content position when known;
- whether a human selected the resource or policy only proposed it.

Requested URLs, fragments, redirects, and aliases remain evidence even when they resolve to a shared representation. If one parent claims a resource transiently and another pins it for a durable reading bundle, retention resolves to the stronger requirement rather than whichever edge happened to arrive first.

Multiplicity can affect priority and salience. It must not be erased merely because the response body is content-addressed. Authenticated, token-bearing, logout, form-action, and other unsafe requests are not speculative prefetch candidates and never leak credentials into ordinary task records.

## Bounded automatic expansion

The current `IB.Prefetch` slice accepts an already-selected resource list and does not discover links. That is an implementation boundary, not a permanent architecture rule.

For a documentation or research task, an acquisition adapter may discover and prioritize links automatically. Selection may use the operative part of a document, the current question, repeated same-site navigation, shared targets from several roots, source-specific structure, and prior corrections.

The operative part is not simply the first links in the HTML or body. It is the relevant document content after global navigation, branding, and unrelated chrome have been excluded.

Expansion remains bounded by explicit depth, bytes, storage, CPU, network, and task budgets. It warms response or extraction state on disk; it does not create a live renderer session for every target and it is not a promise to crawl arbitrary sites.

## Programming-documentation behavior

If fifty saved documentation tabs belong to one documentation task and each contains further relevant links, they should share one durable investigation rather than fifty isolated renderer sessions. Unrelated goals remain distinct tasks even when their resources or page summaries can safely be reused.

When several roots link to the same page, keep one reusable resource plus all source edges. Continue bounded background fetching and extraction while the foreground remains usable. A later question should search and combine the already accumulated material with per-source provenance instead of requiring the user to open each link again.

After a week and a reboot, the browser may have lost every cached response and renderer process. It should still know the roots, discovered frontier, completed work, and missing work. Resume or refetch according to current policy rather than starting the investigation from nothing.

Summaries and rankings are versioned derived artifacts keyed to immutable source-representation hashes. They retain page and section references, generator or model identity, prompt or policy revision, creation time, completeness, and staleness. A new result appends or supersedes; it does not overwrite the source or the prior result. A page summary may be reused by several tasks, while a cross-page synthesis remains task-specific.

For a resumable investigation, the accepted or last-complete task summary—or its complete raw proposal—plus source references is durable enough to show immediately after a week and a reboot. Presentation indexes and intermediate or rejected attempts remain disposable. The task, user selections and corrections, and source graph are durable browser state.

## Renderer path

The acquisition path is independent of any particular renderer:

```text
task or renderer requests resource
        |
        +-- exact reusable response exists -> use it
        |
        `-- no response -> ordinary acquisition path
```

An extractor may satisfy a task without attaching a page renderer. A renderer may consume the same prefetched response later. Neither the acquisition cache nor a renderer owns the task or tab.

## arXiv first policy

arXiv is deliberately more aggressive than a generic site. An arXiv HTML paper is a bounded document resource graph, so IB may prefetch the complete HTML document plus every figure resource explicitly referenced by the paper HTML. This is not treated as open-ended lookahead spidering.

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

The HTML and external figures may also remain in the disposable response cache. The files under `~/reading` are the semantic corpus: a person or local model can query the paper text, captions, image manifest, and figure files without depending on a live renderer cache.

If usable arXiv HTML is unavailable, the existing scientific-media policy may fall back to PDF.

## Test surfaces

The fixture harness should expose these independently:

- durable task roots and frontier;
- discovered but unrequested resources;
- queued and in-flight requests;
- fetched disposable responses;
- extracted or summarized derived artifacts;
- material promoted to `~/reading`;
- hot renderer sessions.

A regression should clear response and extraction caches, reconstruct the same task frontier, and resume acquisition without losing roots, edges, or human decisions. Scaling the known resource graph must not scale the live renderer working set.

Minimum deterministic cases should also prove:

- header, navigation, form, logout, and token links do not outrank safe links in operative documentation content;
- two roots targeting one child yield one compatible fetch and two retained edges;
- a stronger later retention claim is not lost;
- killing the process after queueing or during a fetch never exposes a torn body/metadata pair, and pending work resumes;
- a fake-clock return one week later shows the last complete view immediately and revalidates stale material in the background;
- relative URLs, `<base>`, fragments, redirects, and incompatible auth/`Vary` partitions preserve the correct identity distinctions;
- a changed source marks the old summary stale and appends a new source-backed version;
- duplicate tab intentions and visit events survive while compatible response bytes remain shared.

## Commitment level

Settled boundaries:

- durable investigation intent is not cache;
- discovery is not an in-flight request;
- reusable bytes may be deduplicated while edges, tabs, and visits remain distinct;
- background acquisition does not imply a live renderer;
- a resumable task retains its accepted or last-complete summary and provenance without making it canonical truth;
- clearing cache preserves tasks, history, accepted organization, and `~/reading`.

Current policies and first slices:

- `IB.Prefetch` bounds and deduplicates a caller-supplied resource set;
- Grease owns current HTTP and filesystem materialization;
- arXiv HTML plus explicit figures is an aggressive bounded source adapter.

Still open:

- the investigation serialization;
- operative-content extraction and frontier scoring;
- source-specific depth and budget policies;
- retention beyond the last-complete resumable-task result;
- scheduling across competing background tasks.
