# Persistent storage model

The canonical format should be boring, inspectable, append-friendly, and independent of a specific renderer or model.

## Identity and state classes

Do not use one URL string as the identity of everything around it.

- **Resource** — a browser-owned record carrying requested and resolved URLs and any asserted canonical URL.
- **Tab** — a durable navigation thread or browsing intention.
- **Event** — an append-only visit, search, duplicate-tab action, selection, correction, or other occurrence with a stable id.
- **Task/investigation** — a goal spanning roots, tabs, resources, candidates, actions, and unfinished work.
- **Explicit assertion** — a user-authored membership, removal, active-set choice, or other authoritative decision.
- **Model proposal** — an untrusted, versioned observation with source provenance.
- **Materialized view** — a rebuildable category directory, index, embedding generation, extracted view, ranking, or summary.

Representations and content hashes have identities separate from resources. One resource may change over time, and unrelated resources may contain equal bytes. Compatible representations may be shared while retaining distinct resource, tab, event, referrer, task, and user-action records.

## Directory sketch

```text
state/
  resources/
    <resource-id>/
      resource.txt
      representations/
  tabs/
    <tab-id>/
      tab.txt
      history.log
      view.txt
  events/
    browser.log
  tasks/
    <task-id>/
      task.txt
      roots
      resource-edges
      frontier
  assertions/
    organization.log
  proposals/
    <proposal-id>.txt
  decisions/
    inference.log
  snapshots/
    <content-hash>/...
  indexes/
    ...
```

This is not a frozen on-disk grammar. `IB.FileStore` implements the first slice: store and tab directories, `tab.txt`, and `history.log` are created and accessed by Idriç. The category, task, proposal, and assertion paths remain design boundaries rather than claims about the current runtime.

Rebuildable filesystem projections currently live under a separate `views/` root. Source-derived text and images live below `retrieved-from-the-web/`; cross-object categories and model-specific vector spaces live below `organizing-the-information/`; `_active` and `hot/` remain short control surfaces at the root. The Grease slice does not yet provide the durable Idriç assertion records required to reconstruct accepted human organization after deleting every view.

## Stable event identity

Every visit, search, opening, duplicate-tab action, correction, and explicit decision needs an identity stable across import, merge, restart, and sync. List position or a normalized input order is useful presentation metadata but is not a durable source reference.

An event records its kind and available context rather than flattening every repeated URL into an undifferentiated revisit. Reload, redirect, restored session, deliberate duplicate, background open, and imported duplicate may all remain distinct.

## Tab manifest and history

One small manifest per tab is preferable to one enormous renderer session file. A person should be able to inspect it with ordinary text tools.

Example:

```text
id 01T...
created 2026-08-22T14:00:00-04:00
last_visited 2026-08-22T14:20:00-04:00
state sleeping
current_event 01E...
priority normal
renderer auto
```

Fields that can be derived should generally stay out of the canonical manifest. History is append-oriented and references stable events; it exists independently of a renderer's private session database.

## Tasks and link graphs

A task persists the user's intention and unfinished frontier even when every associated renderer and response cache has disappeared. Roots and edges refer to stable tab, event, and resource identities. Several parent edges may point to one reusable resource without losing their individual context, order, or retention claims.

Fetched bodies, extracted views, rankings, and summaries are not the task itself. See `docs/personal-workbench.md` and `docs/prefetch-and-reading.md`.

## Snapshots and representations

Large content belongs separately from small metadata. A resource or event can refer to zero or more stored representations:

- response body and headers;
- extracted structured information;
- readable text;
- WARC record;
- screenshot or thumbnail;
- renderer-specific recovery cache.

Immutable or content-addressed representations permit safe reuse. Reuse must respect authentication, cookies, request headers, `Vary`, and other security or representation boundaries. Requested URLs, redirects, fragments, and source edges remain evidence even when bytes are shared.

## Assertions, proposals, and decisions

User-authored organization and corrections are durable browser-owned metadata. They are not disposable merely because a filesystem category view can be rebuilt.

Model outputs are stored separately as append-only proposals or observations. Acceptance, rejection, aggregation, supersession, and rollback are appended decisions. A materialized result cites the proposal or proposals and the validator/reducer version that produced it. A reducer may emit a decision; it never manufactures an assertion in the user's name.

The normative direction is:

```text
immutable source observations -> untrusted proposals -> validated decisions --+
canonical browser events -----------------------------------------------------+-> views
authoritative user assertions and corrections -------------------------------+
```

A malformed or malicious model output cannot change canonical history, source files, explicit assertions, or secret storage. See `docs/inference-and-learning.md`.

The current file-store slice appends complete caller-supplied records and preserves duplicates and ordering. It does not yet parse or validate the record grammar. It also does not yet provide `fsync`, atomic manifest replacement, or journal recovery; those guarantees require a small native filesystem boundary and fault-injection tests.

## Derived artifacts

An index or derived artifact should be reconstructible from canonical records and durable decisions whenever practical.

Examples include:

- recency, domain, and revisit signals;
- category filesystem projections and reverse membership;
- vector-index generations;
- extracted information views;
- page summaries and task-specific syntheses;
- renderer recovery material.

Versioned summaries retain immutable source hashes, source spans, generator identity, time, completeness, and staleness. A new summary appends or supersedes; it does not overwrite its source or silently replace the previous version.

For tens of thousands of known resources, the first index may remain intentionally simple. SQLite or a custom backend can be introduced without changing canonical identity.

## Credentials and secrets

Passwords, session cookies, authentication tokens, private form fields, and other secrets must not leak into ordinary plaintext manifests, model prompts, public fixtures, or Git history.

The core defines a separate secret/session boundary. Persistent browser records may reference protected state without embedding it. Local inference receives only explicitly selected corpus material; it does not gain secret storage merely by running on the same machine.

## Cache deletion

Clearing cache may remove response bytes retained only for prefetch, extracted information-view presentations, thumbnails, embeddings, rankings, transient summaries, and renderer recovery blobs.

It must not remove tabs, stable history events, task roots and frontiers, explicit assertions and corrections, proposal decisions, accepted category definitions, material deliberately promoted to the reading corpus, or the accepted or last-complete summary record required to resume a retained task. Missing presentation artifacts may be rebuilt without rerunning a model in a way that loses human decisions.

The first vector implementation follows the same rule without SQLite: model-specific directories under `views/organizing-the-information/vector-spaces/` retain IDs and fixed-width row-major vectors as plain text. A little-endian Float32 sidecar accelerates exact scans but can be deleted and recreated from the text. The backend-neutral streaming command contract permits another disposable cache implementation without changing canonical records or readable vector views. See `vector-index.md`.

## Sync

Sync operates on browser-owned identities, events, tasks, assertions, proposals, decisions, and selected snapshots rather than a renderer profile directory. Multiple frontends or machines may share the durable corpus while maintaining separate live renderers and caches.
