# IB Developer Workbench

IB should have a developer workbench for exercising browser state, rendering, caching, and memory behavior against a large deterministic universe of fake URLs.

## Implementation boundary

The workbench state model is Idriç. Deterministic URL identities, working-set policy, and browser-state regressions belong in `.idric` source.

Grease owns fixture orchestration that actually touches the operating system or network: starting a local fixture server, fetching live scientific pages, arranging temporary directories, invoking renderers/parsers, and collecting process-level measurements. That distinction avoids turning Idriç into a shell while keeping browser policy out of scripts.

## Fixture universe

Start with a few hundred generated URLs and scale to at least 10,000.

The fixture set should eventually be able to use a sanitized backlog derived from real browsing history so that URL shapes and navigation patterns resemble actual use without depending on the live web.

Each fake URL should have deterministic content and known behavior. Useful fixture classes include:

- tiny text pages
- long documents
- image-heavy pages
- redirects
- repeated/shared resources
- query-string variants
- broken resources
- pages with unusually large resource graphs

A fixture URL such as `ib://fixture/000042` should reproduce the same response graph every run.

## Hot working set

The important architectural distinction is between the number of known URLs and the number of live browser documents.

IB should normally keep roughly 3–10 working tabs hot. A hot tab may keep parsed document state, layout state, rendered data, interaction state, and other genuinely active-page data in RAM.

The remaining fixture universe should be cold or warm state rather than thousands of live documents.

Possible states:

- **hot** — active in RAM
- **warm** — serialized page state and/or render cache exists
- **cold** — metadata only
- **never visited** — fixture exists but has no browsing state

Opening a cold URL promotes it into the working set. Memory pressure or working-set limits should demote another page without losing durable browser state.

## Workbench display

The workbench should make the distinction visible, for example:

```text
IB WORKBENCH

Fixture universe          10,000 URLs
Known/cacheable             8,412
Never visited               1,588

HOT TABS                     RAM
1  fixture/000042            18 MB
2  fixture/000731            11 MB
3  fixture/004211            27 MB
4  fixture/008002             9 MB
                             -----
Working set                  65 MB

WARM / EVICTED
27 pages with serialized page state
413 pages with render cache
7,912 metadata-only URLs
```

## Abuse controls

The first version should make it easy to force pathological transitions:

- open random URL
- open many tabs
- close tab
- evict tab
- restore evicted tab
- clear render cache
- simulate memory pressure
- simulate process death
- reload many URLs
- thrash repeatedly between URLs
- impose a synthetic RAM ceiling

## First invariant to test

Growing the fixture universe from 200 to 2,000 to 10,000 known URLs should not make steady-state RAM scale with the universe when the hot-tab count remains fixed.

For example, with five hot tabs, the RAM cost of 10,000 known URLs should be dominated by compact metadata and bounded caches, not 10,000 parsed or rendered documents.

If RAM grows approximately with the number of known URLs, IB has accidentally coupled catalog size to active-page state and the architecture should be corrected.

## Initial implementation order

1. Deterministic fake-URL fixture generator.
2. Explicit hot/warm/cold tab-state model.
3. Workbench counters for fixture count, state counts, and memory estimates.
4. Controls for promotion, eviction, cache clearing, and tab thrashing.
5. Automated regression that holds hot tabs fixed while scaling the fixture universe.
6. Live/recorded scientific-media fixtures through Grease.
7. Later, import a sanitized browsing-history backlog as fixture seeds.
