# Cache and storage

## Clearing cache must not delete user information

**Negative user story**

I need space for photos or another application, press Clear cache, and discover that IB deleted history, tabs, bookmarks, annotations, explicit saved pages, or learned choices that cannot be reconstructed.

**Desired behavior**

Every byte classified as cache is safely rebuildable. Clearing all cache may make IB slower for a while, but it must not destroy user information.

Durable state includes at least:

- ordered history
- logical tabs and their durable state
- bookmarks/collections
- annotations
- explicit saved pages
- full durable URLs
- explicit typed-query/chosen-destination observations used for learning

Reclaimable acceleration includes at least:

- prefix/subword/trigram indexes that can be rebuilt
- memoized query result lists
- thumbnails and pre-paints that can be regenerated
- renderer/network resource caches
- temporary parsed representations

**Inspector evidence**

The storage inspector shows durable and reclaimable totals separately and can drill down by category.

---

## Search remains correct immediately after Clear cache

**Negative user story**

I clear cache and then cannot find pages that are still present in my history until a background rebuild finishes.

**Desired behavior**

Correctness survives cache deletion. IB may use a slower raw scan or smaller durable structure immediately after clearing. Fast indexes rebuild opportunistically from durable data.

**Inspector evidence**

Show whether a query used the hot index, a smaller fallback index, or a raw scan, and whether a rebuild is underway.

---

## IB must notice when the device is short on space

**Negative user story**

IB continues growing speculative indexes and caches while the device is nearly full, leaving too little room for photos, downloads, updates, or other applications.

**Desired behavior**

IB observes actual available device storage and changes cache policy as free space shrinks.

A reasonable qualitative policy is:

- plenty of space: grow useful acceleration structures normally
- space becoming constrained: stop speculative growth and prefer reuse
- low space: evict cold rebuildable data
- user explicitly clears cache: reclaim rebuildable data immediately

The exact thresholds are policy parameters and should be visible rather than hidden constants.

**Inspector evidence**

Show available device bytes, current IB durable bytes, current IB reclaimable bytes, configured thresholds, and the reason for the most recent cache-growth or eviction decision.

---

## Cache growth should buy measurable speed

**Negative user story**

IB consumes hundreds of megabytes for an index or memoization layer that has no meaningful effect on latency.

**Desired behavior**

Reclaimable storage exists to buy time back for the user. IB should be able to attribute bytes to acceleration mechanisms and measure whether they are being used.

Cold or low-value acceleration data should lose space to hot data when the cache budget is constrained.

**Inspector evidence**

For each major cache/index category, expose approximate bytes, hit/use count, last use, rebuild cost if known, and latency path it accelerates.

---

## A cheap phone and a roomy phone should not get the same fixed cache budget

**Negative user story**

IB assumes one hard-coded cache size. On a small phone it becomes a storage hog; on a roomy phone it refuses to use inexpensive spare storage that could make repeated operations faster.

**Desired behavior**

The cache budget is adaptive to available storage and may also respect a user-set ceiling. Spare storage may be borrowed aggressively when plentiful because it is reclaimable.

**Inspector evidence**

Show the current automatic budget, any user ceiling, and why the automatic budget changed.

---

## Clearing cache should be immediately useful

**Negative user story**

I press Clear cache because I urgently need space, but IB merely marks data for later cleanup or immediately begins recreating everything I just removed.

**Desired behavior**

Clear cache promptly deletes reclaimable data and reports the reclaimed amount. Rebuilding should be demand-driven or throttled so the requested free space remains available.

**Inspector evidence**

Show bytes before, bytes after, bytes reclaimed, and whether cache regrowth is temporarily suppressed after an explicit clear.

---

## Memoized query results should be compact

**Negative user story**

A memoized omnibox query stores duplicate copies of long URLs, titles, and page text for every prefix, so a small speed feature grows disproportionately.

**Desired behavior**

Memoized query entries primarily store the query key, compact result identifiers/ranking information, and bookkeeping. Full durable URLs and titles remain in their authoritative records.

As an engineering target rather than a contract, a hot-query entry should normally be on the order of hundreds of bytes rather than kilobytes unless it has a specific reason to be larger.

**Inspector evidence**

Show per-entry and aggregate memoization bytes, result count, and whether payload data is duplicated or referenced.

---

## Fast URL search should not index every byte equally

**Negative user story**

The substring index spends most of its disk budget indexing tracking parameters, long generated slugs, hashes, UUIDs, and fragments that users almost never search.

**Desired behavior**

The fast representation favors titles, hosts, human-readable path tokens, short object identifiers, and explicit user-entered text. Long high-entropy URL material can remain in durable raw URLs and be handled by slower exact/raw fallback unless prior user behavior makes it worth promoting.

**Inspector evidence**

Expose the normalized/indexed representation and its byte cost next to the original durable URL.

---

## Cache deletion and app-data deletion are different operations

**Negative user story**

The only way to reclaim IB's speed-up storage is Android's Clear storage/data action, which also destroys the browser's durable state.

**Desired behavior**

IB provides an in-app Clear cache operation that reclaims rebuildable storage without requiring the user to clear application data.

**Inspector evidence**

The storage inspector identifies which categories the in-app Clear cache action will remove before the action is taken.

---

## Cache-size experiments should include realistic scales

These are sizing test points, not fixed promises. Implementations should measure their actual bytes at these scales and record the result in the inspector/workbench.

Suggested fixtures:

- 50,000 durable history records
- 500,000 durable history records
- 1,000,000 durable history records
- 10,000 memoized queries
- 100,000 memoized queries

For each scale, record at least:

- durable source bytes
- normalized searchable-text bytes
- prefix/token index bytes
- substring/trigram index bytes, if present
- memoization bytes
- cold-start query latency
- warm query latency
- query latency immediately after Clear cache
- time/bytes required to rebuild hot acceleration structures

The purpose is to make the storage-for-milliseconds trade visible rather than guessed.
