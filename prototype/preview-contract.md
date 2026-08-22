# Preview cache contract

The pre-renderers in this directory are disposable producers. Their outputs are cache entries, never canonical browser state.

For one history entry the browser core may retain zero or more preview representations:

```text
text/plain-preview
image/x-portable-bitmap-preview; screen=1
image/x-portable-bitmap-preview; screen=2
image/x-portable-bitmap-preview; screen=3
```

Each preview records or is keyed by:

- history-entry id
- source snapshot/content hash when available
- pre-renderer id and version
- viewport class or exact preview dimensions for visual output
- creation time

A preview is stale when its source snapshot/content hash changes. It may still be displayed as a stale placeholder if the UI marks that fact and immediately allows navigation away.

Deleting every preview must leave tabs, history, snapshots, labels, collections, and URLs intact.

The UI must not require all previews to be loaded to enumerate tabs. A preview is fetched only when a tab/card/view actually needs it.

A sleeping tab therefore has no live renderer requirement. Its first paint can be, in increasing cost order:

1. URL/title metadata already present in an index or tab record.
2. `preview.txt`, if cached.
3. one PBM preview screen, if cached.
4. additional PBM screens on demand.
5. a real renderer session only when interaction or fidelity requires it.
