# Architecture

## Principle and scope

`ib` treats browsing state, user decisions, and task intent as durable data. Acquisition, extraction, rendering, and frontend presentation are replaceable services.

A tab is not a renderer process. It is a persistent navigation thread that may currently have a renderer attached. A task may span several tabs, resources, searches, and actions.

IB's present product target is a personal browser/workbench, not universal web compatibility. The substrate still supports multiple first-class frontends and renderer adapters so a broader browser can be built over it without owning or changing the stored model. See `docs/personal-workbench.md`.

## Ownership

The browser core owns:

- resource, tab, event, and task identity;
- navigation and search history;
- sleeping and waking policy;
- task and investigation frontiers;
- saved representations and snapshot references;
- user organization, assertions, corrections, and active choices;
- indexes, category semantics, and acceptance policy;
- renderer selection and display-repair policy;
- synchronization and export boundaries.

Acquisition and extraction adapters obtain HTML, direct media, structured responses, or other useful source material. They may complete a task without a renderer.

A renderer owns only the temporary machinery required to display and interact with a live page while attached.

A frontend projects browser and task state and issues commands. It does not become the source of truth.

## Main layers

```text
page frontend       task workbench       inspector/commands
       \                  |                    /
        +--------- browser and task core -----+
                  /           |          \
       persistent store   acquisition   renderer adapters
                           / extraction    |     |     |
                           HTTP, parsers  Servo WebView text/etc.
```

The page frontend, text-first task workbench, inspector, information extractor, and text renderer are distinct roles. In particular, a text-oriented renderer is not the ChatGPT-like workbench frontend.

The persistent store remains intelligible and useful without a rendering engine or language model installed.

## Persistent objects

### Resource

A browser-owned record carrying requested and resolved URLs plus any asserted canonical URL. Immutable representations and content hashes have separate identities. Compatible response bytes may be reused without collapsing resource, tab, or event identity.

### Tab

A durable navigation thread. It survives process exits and normally survives renderer changes. It references a current history event and has browser-owned metadata such as stable id, creation time, sleeping state, priority, and optional renderer preference.

Two tabs may deliberately refer to the same resource. Equal URLs do not collapse tab identity.

### Event

An append-only occurrence such as a visit, search, duplicate-tab action, referrer traversal, explicit correction, or handoff. Events have stable identities and retain time, order, source, and task context when known.

### Task or investigation

A durable user goal spanning roots, tabs, resources, candidates, background work, and actions. It preserves enough intention and frontier state to resume after process death or a later reboot. A task is not a renderer session and is not identical to its cached bodies or summaries.

### Snapshot or representation

A stored version of fetched or derived material. Large content belongs in content-addressed or otherwise immutable storage, not in a small tab manifest. A representation can outlive the tab or task that first produced it.

### View state

Browser-owned state useful for reconstructing a view, for example scroll position and selected history event. Renderer-specific opaque state may exist as optional cache but cannot be the only representation of browser state.

### Proposal and decision

A model result is an untrusted append-only proposal with provenance. Deterministic validation and explicit user action may accept, reject, aggregate, or supersede it. Models never write canonical history or user assertions directly. See `docs/inference-and-learning.md`.

## Sleeping and bounded residency

Sleeping is the ordinary state of an old tab, not an exceptional recovery path.

A sleeping tab has no renderer session and consumes approximately the cost of persistent metadata plus bounded indexes and caches. Waking attaches a renderer only when a task needs one and reconstructs the best available view from stored records.

A 10,000-resource or history corpus does not imply 10,000 logical tabs or live documents. The current developer fixture deliberately separates 10,000 known URLs, 32 logical tabs, and a 3–10-tab resident working set. Steady-state renderer RAM should follow the resident set, not corpus size.

## Renderer swapping

The core selects a renderer through a narrow adapter contract. Swapping renderers must not change resource, tab, event, task, organization, or stored-representation identity.

Renderer choice may be global, per site, per tab, or selected from required capabilities. No first architecture assumption requires every renderer to support JavaScript, WebAssembly, DRM, extensions, or identical DOM restoration.

Exact live DOM or JavaScript-heap continuity across unrelated engines is not required. Continuity belongs to browser-owned records.

## Acquisition and information extraction

Cheap acquisition and extraction complement renderer adapters; they are not forced through the renderer interface.

An existing extracted view, direct image, HTML body, structured response, or small source-specific adapter may satisfy a task before any live page machinery starts. A full renderer remains available when interaction or visual context genuinely requires it. See `docs/resource-constrained-rendering.md` and `docs/prefetch-and-reading.md`.

## Display repair

IB need not reproduce a site's interface defects literally. It may add browser-owned controls or presentation when doing so makes ordinary actions easier.

Display repair is browser policy above a page renderer. It must not silently rewrite fetched responses, stored snapshots, or canonical history. Trusted controls should normally be painted outside page-controlled DOM, CSS, and JavaScript so the site cannot hide or impersonate them. See `docs/display-repair.md`.

## Durable decisions and derived views

Canonical records and explicit user decisions are the source of truth. Indexes and materialized views are disposable acceleration or presentation structures.

Examples of rebuildable derived state include:

- recency, domain, revisit, and awake/sleeping indexes;
- full-text and vector-search indexes;
- renderer capability requirements;
- category directories and reverse-membership indexes;
- extracted information views, rankings, and summaries.

Category definitions, accepted memberships, correction events, and active-category choices are durable even when their filesystem projections are rebuilt. See `docs/tab-categorization.md` and `docs/storage-model.md`.

## Reversibility and inference

Automated organization and model-assisted work follow one direction:

```text
immutable source observations -> untrusted proposals -> validated decisions --+
canonical browser events -----------------------------------------------------+-> views
authoritative user assertions and corrections -------------------------------+
```

Reducers emit decisions; they never create an assertion in the user's name.

Destructive operations are explicit. Model absence or failure cannot make basic browsing or recovery unavailable. Git may be useful for small textual metadata and schema evolution but is not required for large bodies, caches, or renderer state.

## Current non-goals and open choices

IB is implemented in Idriç, with Grease for operating-system and process orchestration and narrow native platform adapters. That language boundary is already chosen; it is not an open architecture question.

The current work does not promise:

- universal web, MIME, renderer, or malformed-input compatibility;
- one mandatory frontend;
- faithful reproduction of interfaces irrelevant to the user's task;
- preserving a JavaScript heap across renderer changes;
- automatic understanding of every private application protocol;
- unbounded crawling or one live renderer per candidate;
- direct canonical mutation by a language model.

Still open are the final frontend toolkit, first conventional renderer, task and proposal serialization, sync provider, archive format, and source-specific acquisition budgets.
