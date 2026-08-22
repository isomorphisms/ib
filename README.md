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
- generic JSON containing `url`, `titleUrl`, or `href` fields.

The adapter registry is open: `register_history_adapter(name, adapter)` adds another importer without changing the indexing layer.

```python
from ib import IndexBuilder, ingest_history, write_plaintext_indices

entries = ingest_history("urls.txt")
indices = IndexBuilder().build(entries)
write_plaintext_indices(indices, "state/index")
```

The prototype intentionally does **not** deduplicate the visit stream. `import_order` is preserved, and derived indices cover timestamp chronology, exact URL, host, source, day, YouTube search query, and searchable terms. Plain-text JSONL/TSV output is derived data and can be thrown away and rebuilt from `visits.jsonl`.

Run the tests with:

```sh
python -m unittest discover -s tests -v
```
