# ib

An experimental browser built around persistent browsing state rather than a renderer-owned tab model.

The browser core should own navigation history, sleeping/waking, snapshots, organization, and indexes. Rendering engines are adapters that can be replaced without changing the stored browsing model.

## History ingestion prototype

`ib.ingest_history()` is the test hook for importing existing browsing traces before the renderer exists. It currently accepts:

- raw URL text or URL lists;
- Google Takeout / My Activity JSON;
- YouTube activity/search JSON;
- Chrome/Chromium `History` SQLite databases;
- Firefox `places.sqlite` databases;
- generic JSON containing `url`, `titleUrl`, or `href` fields;
- canonical `visits.jsonl` written by the history store.

The adapter registry is open: `register_history_adapter(name, adapter)` adds another importer without changing the indexing layer.

```python
from ib import IndexBuilder, ingest_history, write_history_store

entries = ingest_history("urls.txt")
indices = IndexBuilder().build(entries)
write_history_store(indices, "state")
```

The prototype intentionally does **not** deduplicate the visit stream. `import_order` is preserved. Derived indices cover newest-first chronology, exact URL, host, source, UTC day, extracted Google/YouTube search query, and searchable terms. Equal timestamps and missing timestamps keep their original import order.

`state/visits.jsonl` is canonical normalized history. Rebuildable files live below `state/indexes/`, so deleting or rebuilding indexes cannot delete canonical history. `write_plaintext_indices()` remains as a flat self-contained export/test bundle for the history-ingestion prototype; it is not the browser-owned storage layout.

## Storage inspector

`StorageInspector` is the read-only reference model for an eventual native IB debug screen. It inspects the browser-owned store rather than pretending persistent records are an ordinary tab list.

It separates canonical records, rebuildable indexes, saved snapshots, cache, protected secrets, and unknown files; parses planned `tabs/<id>/tab.txt` manifests without waking renderers; and can show a byte-bounded tail of canonical imported visits. Classification follows schema-shaped paths rather than filenames alone. Symlinks are listed but never followed by readers.

Generic raw reads are deliberately narrower than classification: only declared transparent text records can be opened. Cache, snapshots, unknown files, secret paths, symlinks, special files, and unrecognized future index formats remain metadata-only until a specific safe viewer exists.

The current text harness is useful while the Android UI does not exist yet:

```sh
python -m ib.inspect state
python -m ib.inspect state --limit 50
python -m ib.inspect state --read indexes/chronology.tsv
```

The Python harness defines and tests behavior; the native screen should implement the same storage contract directly and does not need to embed Python. Destructive maintenance actions remain separate from inspection. See `docs/storage-inspector.md`.

Run the tests with:

```sh
python -m unittest discover -s tests -v
```
