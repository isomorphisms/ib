# IB browser-foundation fixture harness

This developer harness exercises browser state, acquisition, rendering, caching, and memory behavior against a large deterministic universe of fixture URLs.

It is not the user-facing text-and-action task workbench described in `docs/personal-workbench.md`. Keeping the names and roles separate matters: the fixture harness tests the substrate; a frontend uses the substrate.

## Implementation boundary

The fixture state model is Idriç. Deterministic URL identities, working-set policy, and browser-state regressions belong in `.idric` source.

Grease owns fixture orchestration that touches the operating system or network: starting a local server, fetching live scientific pages, arranging temporary directories, invoking renderers or parsers, and collecting process-level measurements. That avoids turning Idriç into a shell while keeping browser policy out of scripts.

## Fixture universe

Start with a few hundred generated URLs and scale to at least 10,000.

The checked-in sanitized backlog derived from real browsing history makes URL shapes, duplicate visits, and navigation patterns resemble actual use without depending on the live web. It does not make the private 491-row classification exercise a checked-in ground truth.

Each synthetic URL has deterministic content and known behavior. Useful classes within IB's intended personal corpus include:

- tiny text pages;
- long documents;
- image-heavy pages;
- redirects and repeated shared resources;
- query-string variants;
- broken resources;
- documentation pages with operative and non-operative links;
- pages with unusually large but bounded resource graphs.

A fixture such as `ib://fixture/000042` reproduces the same response graph every run. This is targeted failure and workload coverage, not a promise to support arbitrary malformed input or every MIME type.

## Known corpus, logical tabs, and hot renderers

Do not conflate the number of known resources, logical tabs, and live renderer documents.

The current executable scale is:

```text
known fixture URLs     10,000
logical tabs               32
resident renderer tabs   3–10
```

A hot tab may keep parsed document state, layout state, rendered data, interaction state, and other genuinely active-page data in RAM. The remaining corpus stays warm, cold, or never visited.

Possible states:

- **hot** — active renderer state in RAM;
- **warm** — serialized page state, fetched bytes, or render cache exists;
- **cold** — durable metadata or task graph only;
- **never visited** — fixture exists but has no browsing state.

The filesystem `views/hot` set is a request to favor presentation targets for immediate use. It is not the same measurement as resident renderer tabs: a text or synthesized Markdown presentation may be hot without any page renderer, and memory pressure may temporarily make requested-hot differ from actually resident. See `docs/filesystem-views.md`.

Opening a cold URL may promote a logical tab into the renderer working set. Memory pressure or working-set limits demote another renderer without losing browser-owned tab, event, task, or organization state.

Prefetched documents remain cold or warm; prefetching seven documentation pages must not create seven live renderer sessions.

## Harness display

The harness should make the identity and residency distinctions visible, for example:

```text
IB FOUNDATION HARNESS

Known resources             10,000
Logical tabs                    32
Hot renderers                     5
Warm responses                   27
Metadata-only resources       8,380
Never visited                1,588
```

Task frontiers, discovered resources, response cache, extracted views, promoted reading material, and renderer residency should have separate counters.

## Abuse controls

The first versions should make pathological transitions easy to force:

- open or duplicate a tab;
- revisit or reload a resource;
- evict and restore a renderer;
- clear response, extraction, or render cache independently;
- simulate memory pressure and process death;
- kill during queued and partial acquisition;
- resume a week-old task under a fake clock;
- thrash repeatedly between resources;
- impose synthetic RAM, byte, and deadline ceilings;
- feed malformed model proposals without changing canonical state.

## Core invariants

Growing the known universe from 200 to 2,000 to 10,000 resources must not make steady-state renderer RAM scale with the universe while hot-renderer count remains fixed.

Other required distinctions include:

- repeated URL rows, duplicate tabs, and visits remain distinct events or intentions;
- compatible response bytes may be shared without collapsing their source edges;
- clearing cache preserves history, tasks, assertions, and accepted organization;
- process death never exposes a partial response as complete;
- pending durable work resumes without waking every related tab;
- a malformed or unavailable model cannot mutate canonical history or block browsing;
- adding a category membership does not remove another membership;
- removing a category from `_active` creates no negative training event.
- a focus-priority hint may reorder safe prefetch work but creates no speculative tab or renderer;
- the configured assistant receives only the explicitly scoped, inspectable task-context bundle.

If RAM grows approximately with known-resource count, or rebuilding a derived view loses a human correction, the architecture has coupled state classes that must remain separate.

## Initial implementation order

1. Keep the existing 10,000-resource, 32-logical-tab, 3–10-resident regression.
2. Give browser events stable identities and distinguish duplicate, reload, redirect, restore, and revisit sources.
3. Add durable task roots, link edges, and acquisition frontier fixtures.
4. Add explicit hot, warm, cold, and never-visited counters.
5. Test independent cache clearing, process death, atomic completion, and restart.
6. Add operative-document-link and shared-child documentation fixtures.
7. Add proposal, validation, correction, and reversible materialization fixtures.
8. Continue live or recorded scientific-media fixtures through Grease.
9. Add a GitLab-shaped seventeen-link fixture: changing visual focus reprioritizes safe links, creates zero speculative tabs or renderers, and supports a cited text answer.
10. Add a multi-paper arXiv fixture: early per-paper summaries and one cross-paper answer require no renderer per paper.
11. Add a mock video fixture: captions and playback position enter an authorized assistant context bundle without fetching video bytes or exposing secrets.
