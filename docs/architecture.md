# Architecture

## Principle

`ib` treats browsing state as durable data and rendering as a replaceable service.

A tab is not a renderer process. A tab is a persistent record that may currently have a renderer attached to it.

The browser core owns:

- tab identity and ordering
- navigation history
- sleeping and waking policy
- saved page snapshots
- user organization and labels
- indexes and search metadata
- renderer selection
- display-repair policy and browser-owned augmentations
- synchronization/export boundaries

A renderer owns only the live machinery required to display and interact with a page while it is attached.

## Main layers

```text
UI / commands / LLM hooks
          |
          v
+-----------------------+
|      browser core     |
| tabs, history, state  |
+-----------------------+
     |             |
     v             v
 persistent      renderer
   store          adapter
                    |
          +---------+---------+
          |         |         |
       Servo     Chromium   text/etc.
```

The persistent store must remain intelligible without any rendering engine installed.

## Persistent objects

### Tab

A durable browsing intention. It survives process exits and normally survives renderer changes.

A tab references a current history entry and has browser-owned metadata such as:

- stable id
- creation time
- last visit time
- current history entry
- sleeping/awake state
- priority
- labels or collections
- optional preferred renderer

### History entry

A navigation event. It records at minimum the requested URL, resolved URL when known, title when known, timestamps, and links to any saved snapshot or recoverable view state.

### Snapshot

A stored representation of fetched material. Large content belongs in content-addressed storage or an archive format, not in the small tab manifest.

A snapshot can outlive the tab that first produced it.

### View state

Browser-owned state useful for reconstructing a view, for example scroll position and selected history entry. Renderer-specific opaque state may be stored as an optional cache, but it cannot be the only representation of browser state.

## Sleeping

Sleeping is the ordinary state of an old tab, not an exceptional recovery path.

A sleeping tab has no renderer session and should consume approximately the cost of its persistent metadata plus indexes. Waking attaches a renderer and reconstructs the best available view from the stored record.

This means a session with thousands or tens of thousands of tabs is principally an indexing problem, not a requirement for thousands of live web views.

## Renderer swapping

The core selects a renderer through a narrow adapter contract. Swapping renderers must not change tab identity, history identity, labels, ordering, or stored snapshots.

Renderer choice can eventually be:

- global
- per site
- per tab
- chosen automatically from required capabilities

The first architecture should not assume that every renderer supports JavaScript, WebAssembly, DRM, extensions, or identical DOM state restoration.

## Display repair

`ib` is not required to reproduce a site's interface defects literally. It may add browser-owned controls or presentation when doing so makes ordinary page actions easier.

Display repair is browser policy above the renderer. It must not silently rewrite fetched responses, stored snapshots, or canonical history. Trusted repair controls should normally be painted outside page-controlled DOM/CSS/JavaScript so the site cannot hide or impersonate them.

The first concrete repair is a `Copy` button for semantic text regions. Candidate detection is deliberately separate from the button primitive, so cheap structural heuristics can work without requiring an expensive semantic or language-model pass. See `docs/display-repair.md`.

## Derived indexes

Indexes are disposable acceleration structures. Canonical browsing records are the source of truth.

Examples of rebuildable indexes:

- recency
- domain
- awake/sleeping
- priority
- labels
- full-text title/URL search
- renderer capability requirements

This keeps the stored model portable and makes aggressive reorganization possible without rewriting the underlying history.

Tab categories are one such derived organization surface: they may overlap freely, become more specific where browsing is dense, and expose a small active working set without imposing a single hierarchy. See `docs/tab-categorization.md`.

## Reversibility

Automated organization and LLM-assisted edits should operate on durable records through auditable changes. Destructive operations should be explicit. Derived organization should be cheap to rebuild or revert.

Git may be useful for small textual metadata and schema evolution, but it should not be required to version large page bodies, caches, or renderer state.

## Initial non-goals

The first design does not choose a final GUI toolkit, rendering engine, programming language, sync provider, or archive format. Those choices should follow the persistent-state boundary rather than determine it.
