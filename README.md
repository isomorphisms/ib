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
- canonical `visits.jsonl` written by the indexer.

The adapter registry is open: `register_history_adapter(name, adapter)` adds another importer without changing the indexing layer.

```python
from ib import IndexBuilder, ingest_history, write_plaintext_indices

entries = ingest_history("urls.txt")
indices = IndexBuilder().build(entries)
write_plaintext_indices(indices, "state/index")
```

The prototype intentionally does **not** deduplicate the visit stream. `import_order` is preserved. Derived indices cover newest-first chronology, exact URL, host, source, UTC day, extracted Google/YouTube search query, and searchable terms. Equal timestamps and missing timestamps keep their original import order.

Plain-text JSONL/TSV output is derived data. `visits.jsonl` is the canonical normalized stream and can be fed back to `ingest_history()` to rebuild byte-identical derived index files.

## Storage inspector

`StorageInspector` is the read-only model for an eventual native IB debug screen. It inspects the browser-owned store rather than pretending persistent records are an ordinary tab list.

It separates canonical records, rebuildable indexes, saved snapshots, cache, protected secrets, and unknown files; parses planned `tabs/<id>/tab.txt` manifests; and can show a bounded tail of canonical imported visits. Protected secret files are never opened through the inspector.

The current text harness is useful while the Android UI does not exist yet:

```sh
python -m ib.inspect state
python -m ib.inspect state --limit 50
python -m ib.inspect state --read index/chronology.tsv
```

The native screen should consume the same `StorageInspector` data and keep destructive maintenance actions separate from inspection. See `docs/storage-inspector.md`.

Run the tests with:

```sh
python -m unittest discover -s tests -v
```
