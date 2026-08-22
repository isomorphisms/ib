# Storage inspector

IB needs a way to inspect persistent state even though the normal browser UI may never present a conventional list of tabs. This is a debugger for the durable store, not a second tab manager.

## Boundary

The inspector belongs above the persistent store and below renderer-specific UI. It must remain usable when no web renderer is attached or when a renderer is broken.

Do not implement the inspector as an HTML page that requires Chromium, Servo, or another renderer to be healthy. The Python `StorageInspector` is an executable reference contract and temporary text harness. A native Android implementation should preserve the same semantics directly; Python is not a runtime dependency of the phone UI.

Inspection is read-only. Cache deletion, index rebuilding, record repair, and destructive edits are separate maintenance operations with explicit controls. In particular, a future “clear cache” action must target cache only and must not share a generic delete path with canonical state.

## Read boundary

Listing and reading are intentionally different capabilities.

The physical-file view may list metadata for canonical state, snapshots, derived indexes, cache, protected secrets, unknown files, symlinks, and special files. Generic raw reads are allowed only for schema-declared transparent text records:

- `visits.jsonl`;
- `tabs/<id>/tab.txt`;
- `tabs/<id>/history.log`;
- `tabs/<id>/view.txt`;
- the known textual files currently emitted below `indexes/`.

Snapshots, cache, unknown files, protected secrets, symlinks, special files, and unrecognized future index formats are metadata-only by default. A later specialized viewer can deliberately broaden that boundary for a specific format.

Reads must be rooted beneath the selected storage directory and must not follow symlinks in any path component. On Android/Linux this should be implemented with a rooted, no-follow file-open primitive or an equivalent kernel-enforced mechanism, not merely by normalizing a string path and then opening it. A live store may change between listing and opening; the UI should report or tolerate disappearing records without mutating them.

## Storage layout and classification

Browser-owned storage should keep canonical history outside disposable indexes:

```text
state/
  visits.jsonl
  tabs/
    <tab-id>/
      tab.txt
      history.log
      view.txt
  indexes/
    chronology.tsv
    urls.tsv
    hosts.tsv
    sources.tsv
    days.tsv
    queries.tsv
    terms.tsv
    summary.json
  snapshots/
    ...
  cache/
    ...
```

`write_history_store()` writes this split layout. `write_plaintext_indices()` remains a flat self-contained export/test bundle and should not be treated as the browser's persistent-storage schema.

Classification is schema-shaped, not basename-shaped:

- root `visits.jsonl` is canonical history;
- exact `tabs/<id>/tab.txt`, `history.log`, and `view.txt` paths are canonical tab/view state;
- `indexes/` is rebuildable derived state;
- `snapshots/` is saved content;
- cache/scratch directories are disposable cache;
- secret-like credential, cookie, token, password, authentication, session, and protected-store paths are protected;
- everything else is unknown.

A file named `visits.jsonl` inside cache or indexes does not become canonical just because of its basename. Secret-name recognition is conservative defense in depth until the dedicated secret/session-storage schema is frozen.

## First native screen

The first useful phone screen has four sections.

### Overview

Show compact counters rather than parsing thousands of records:

- total bytes and file count;
- bytes and file counts by classification;
- persistent tab-manifest count;
- indexed visit count when a trustworthy derived `indexes/summary.json` is available.

Do not scan the entire canonical visit log merely to paint the overview. An exact canonical visit count is an explicit diagnostic operation because it is linear in history size. Unknown bytes stay visible rather than being silently folded into another category.

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

Load these records in bounded pages. Selecting a record may expose its declared safe manifest and related canonical history log; it must not implicitly wake a renderer. The reference parser caps each manifest read so one corrupt or gigantic record cannot dominate the screen.

### Physical files

Show the actual tree under the chosen storage root with byte size, classification, physical type, and whether generic raw reading is allowed. Symlinks should remain visible as symlinks rather than disappearing or being traversed.

### Canonical visits

Show a small tail of root `visits.jsonl` for debugging import order and ingestion. The reference implementation reads backward from the end, returns stable byte offsets rather than pretending it knows global line numbers, and caps both returned records and bytes read. It may return fewer records if the byte budget is exhausted. This is explicitly the canonical imported stream, not a claim that it is the browser's ideal user-facing history view.

## Debugger invariants

1. Opening the inspector must not wake sleeping tabs.
2. Reading records must not mutate last-visited times, ordering, cache state, or renderer state.
3. The inspector must work without a renderer session.
4. Canonical state and rebuildable derived indexes must remain visibly and physically distinct.
5. Unknown storage must stay visible but is not generically raw-readable.
6. Protected secret contents must not leak through the ordinary debug surface.
7. Symlinks must never be followed by automatic parsers or generic raw reads.
8. Overview must not parse every tab manifest or scan the full canonical visit log.
9. Visit-tail work must be bounded by both record count and byte budget.
10. Raw record identity, physical type, and paths must remain visible enough to diagnose corruption, duplicates, ordering errors, stale indexes, and unexpected files.
11. Maintenance operations must be separate from inspection and must never treat canonical state as disposable merely because it is near an index or cache directory.
12. The temporary Python harness is a behavioral reference, not a required layer in the native Android architecture.

## Current harness

Until the native Android surface exists:

```sh
python -m ib.inspect state
```

The CLI exercises the same read-only contract that the native screen should implement. It is a test harness, not the intended final interaction design.
