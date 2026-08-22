# Storage inspector

IB needs a way to inspect persistent state even though the normal browser UI may never present a conventional list of tabs. This is a debugger for the durable store, not a second tab manager.

## Boundary

The inspector belongs above the persistent store and below renderer-specific UI. It must remain usable when no web renderer is attached or when a renderer is broken.

Do not implement the inspector as an HTML page that requires Chromium, Servo, or another renderer to be healthy. A native Android debug screen can consume the same `StorageInspector` model currently exercised by the text harness.

Inspection is read-only. Cache deletion, index rebuilding, record repair, and destructive edits are separate maintenance operations with explicit controls.

## First native screen

The first useful phone screen has four sections.

### Overview

Show compact counters rather than opening thousands of records:

- total bytes and file count;
- canonical-record bytes;
- snapshot bytes;
- rebuildable-index bytes;
- cache bytes;
- protected-secret bytes;
- persistent tab-record count;
- canonical visit count.

Unknown bytes should remain visible instead of being silently folded into another category.

### Durable records

Expose browser-owned records with stable identity. A tab row is a debugging representation of `tab.txt`, not the user's everyday tab switcher.

Useful fields include:

- stable tab id;
- sleeping/awake state;
- current history entry;
- last visit time;
- preferred renderer;
- labels;
- manifest path.

Selecting a record should expose its raw safe manifest and related history log. It should not implicitly wake a renderer.

### Physical files

Show the actual tree under the chosen storage root with byte sizes and classification:

- canonical;
- snapshot;
- derived;
- cache;
- secret;
- unknown.

A safe text file can be opened as text. Binary files can initially show only metadata. Protected secret files can be listed and sized but must not be opened through this inspector.

### Canonical visits

Show a bounded tail of `visits.jsonl` for debugging import order and ingestion. This is explicitly the canonical imported stream, not a claim that it is the browser's ideal user-facing history view.

## Storage classification

The prototype currently recognizes these transparent browser-owned records:

- `visits.jsonl` as canonical history;
- `tabs/<id>/tab.txt`, `history.log`, and `view.txt` as canonical tab/view state;
- `index/` or `indexes/` output such as `chronology.tsv`, `hosts.tsv`, and `terms.tsv` as derived;
- `snapshots/` as saved content;
- cache/scratch directories as disposable cache;
- obvious secret/credential/cookie/token paths as protected.

Classification should eventually come from the storage schema rather than file-name heuristics. The heuristics are deliberately conservative scaffolding for the current prototype.

## Debugger invariants

1. Opening the inspector must not wake sleeping tabs.
2. Reading records must not mutate last-visited times or ordering.
3. The inspector must work without a renderer session.
4. Derived indexes must be visually distinct from canonical data.
5. Unknown storage must stay visible.
6. Secret contents must not leak into the ordinary debug surface.
7. A store with 10,000 sleeping tabs must open through summary/index information rather than eagerly constructing 10,000 rendered pages.
8. Raw record identity and paths must remain visible enough to diagnose corruption, duplicate records, ordering errors, and stale indexes.

## Current harness

Until the native Android surface exists:

```sh
python -m ib.inspect state
```

This exercises the same read-only model that the native screen should use. The CLI is a test harness, not the intended final interaction design.
