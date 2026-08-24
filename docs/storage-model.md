# Persistent storage model

The canonical format should be boring, inspectable, append-friendly, and independent of a specific renderer.

## Directory sketch

```text
state/
  tabs/
    <tab-id>/
      tab.txt
      history.log
      view.txt
  snapshots/
    <content-hash>/...
  indexes/
    recency
    domain
    awake
    sleeping
```

This layout now has a first executable slice in `IB.FileStore`: store and tab directories, `tab.txt`, and `history.log` are created and accessed by Idriç. The contents remain an evolving schema.

## `tab.txt`

One small manifest per tab is preferable to one enormous session file. A human should be able to inspect it with ordinary text tools.

Example:

```text
id 01J...
created 2026-08-22T14:00:00-04:00
last_visited 2026-08-22T14:20:00-04:00
state sleeping
current_history 17
priority normal
renderer auto
label books
label to-read
```

Fields that can be derived should generally stay out of the canonical manifest.

## `history.log`

Navigation history is naturally append-oriented.

Example:

```text
1 2026-08-22T14:01:00-04:00 request https://example.org/
1 2026-08-22T14:01:01-04:00 resolved https://www.example.org/
1 2026-08-22T14:01:02-04:00 title Example Domain
2 2026-08-22T14:05:00-04:00 request https://example.org/page
```

The exact grammar can change, but the important property is that browser history exists independently of the renderer's private session database.

The current file-store slice appends complete caller-supplied records and preserves duplicates and ordering. It does not yet parse or validate the record grammar. It also does not yet provide `fsync`, atomic manifest replacement, or journal recovery; those guarantees require a small native filesystem boundary and fault-injection tests.

## Snapshots

Large content should be addressed separately from tab metadata.

A history entry can refer to zero or more stored representations:

- response body
- headers
- extracted readable text
- WARC record
- screenshot
- renderer-specific recovery cache

Content-addressing permits deduplication when many tabs or visits refer to identical bytes.

## Credentials and secrets

Passwords, session cookies, authentication tokens, private form fields, and other secrets must not leak into ordinary plaintext manifests or Git history.

The browser core should define a separate secret/session-storage boundary. Persistent browser records may reference protected state without embedding it.

## Indexes

An index should be reconstructible from canonical records whenever practical.

A simple textual index can begin as lines such as:

```text
<timestamp> <tab-id>
```

or

```text
<domain> <tab-id>
```

For tens of thousands of tabs, the first implementation can remain intentionally simple. If scans become expensive, an SQLite or custom index can be introduced without changing the canonical tab model.

The first vector implementation follows the same rule without SQLite: inspectable `format.txt` and ID files point to row-major `float32` vector bytes under `indexes/vectors/`. It uses exact scanning at the 10,000-URL scale and exposes a backend-neutral streaming command contract, so an approximate implementation can replace it without changing canonical records. See `vector-index.md`.

## Sync

Sync should operate on the browser-owned records and snapshots, not on a renderer profile directory. This allows multiple browser front ends or machines to share the same durable browsing corpus while maintaining separate live renderer processes and caches.
