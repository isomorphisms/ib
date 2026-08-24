# Suggestions and search

## Interior search must not collapse into first-letter completion

**Negative user story**

I search for a remembered word or phrase that occurs inside a title, hostname, URL path, or earlier typed query. IB treats the input mostly as a prefix and gives me unrelated items that merely start with the same first letter.

A concrete failure is the DuckDuckGo-style behavior where searching for something like `merge request` does not find the remembered merge-request page, while an unrelated name beginning with an early typed letter is promoted instead.

**Desired behavior**

After enough input exists to make interior matching useful, IB searches inside indexed title words, hostnames, meaningful URL path components, and prior explicit queries. Prefix matching remains a signal, not the definition of search.

**Inspector evidence**

For every candidate, show which field matched, whether the match was prefix/interior/exact, the matched span, and the ranking contribution.

---

## A short exact identifier can be the best clue

**Negative user story**

I remember a merge request, issue, pull request, paper, or other object by a short number such as `274`. IB discards numeric URL components as meaningless slug material, so the page is impossible to recover quickly.

**Desired behavior**

Short path identifiers are searchable. An exact match on a short identifier should receive substantial weight, especially when combined with a matching host, project, title, or prior explicit query.

Long high-entropy UUIDs, hashes, tracking parameters, session values, and similar noise do not need the same fast indexing treatment unless the user has explicitly typed or searched for them before.

**Inspector evidence**

Show whether a URL component was classified as a meaningful token, short identifier, high-entropy token, query parameter, fragment, or ignored noise.

---

## Meaningful parts of a URL matter more than the whole raw string

**Negative user story**

A long URL with tracking parameters and generated slugs consumes index space and dominates matching even though the parts I actually remember are the host, project name, route name, and object number.

**Desired behavior**

Keep the complete URL in durable history, but build the fast suggestion/search representation from useful fields such as:

- title
- hostname
- human-readable path words
- short numeric identifiers
- explicit user-entered query text
- explicit user-entered URLs

Generated or high-entropy material may remain available to a slower exact/raw search without occupying the hottest index.

**Inspector evidence**

Show the durable raw URL separately from the normalized/indexed representation.

---

## Explicitly typed destinations outrank incidental encounters

**Negative user story**

I have explicitly typed or selected the same destination many times, but IB ranks a page I merely encountered once through a link or redirect above it.

**Desired behavior**

Distinguish explicit user intent from incidental browsing. Repeated explicit typing/selection is a strong ranking signal. Mere appearance in history is weaker.

**Inspector evidence**

Show counts and recency separately for typed, explicitly selected, linked, redirected, restored, and background-prefetched visits where those distinctions are known.

---

## Learn prefix-to-destination choices without polluting history

**Negative user story**

Every intermediate keystroke such as `g`, `gi`, `git`, and `gith` is stored as permanent history, producing junk and making later search worse.

**Desired behavior**

The partial typing stream is ephemeral. When the user commits to a destination, IB may durably record the useful observation that a particular entered prefix/query led to that chosen target.

For example, repeated choices may teach IB that `gi` usually means a particular GitHub repository without preserving every transient keystroke as a visit or search.

**Inspector evidence**

Show the current ephemeral input separately from durable learned choice observations.

---

## Search must remain correct after the fast index is missing

**Negative user story**

The substring/prefix index is deleted or has not been rebuilt yet, and IB silently stops finding existing history entries.

**Desired behavior**

Search correctness does not depend on rebuildable acceleration data. IB may fall back to a slower scan or smaller index while the fast structure is absent or rebuilding.

**Inspector evidence**

Show which search path answered the query: memoized result, hot index, secondary index, or raw fallback.

---

## Memoized searches may accelerate repeated typing

**Negative user story**

IB repeats the same expensive work for `mer`, `merge`, `merge r`, and other common/repeated query prefixes even when the underlying searchable state has not changed.

**Desired behavior**

IB may memoize query-to-result-ID lists and incrementally narrow previous results where valid. Memoized results contain compact durable-record IDs rather than duplicate full URLs or page text.

The memoization layer is optional acceleration, not authoritative state.

**Inspector evidence**

Show cache hit/miss, source generation, invalidation reason, candidate count before/after narrowing, and bytes occupied by the memoized entry.

---

## Ranking should be explainable

**Negative user story**

A seemingly irrelevant suggestion appears above the page I expected, and there is no way to discover why.

**Desired behavior**

The suggestion inspector exposes candidate generation and ranking separately. It should be possible to see signals such as prefix match, interior match, exact identifier, typed-before count, selection count, recency, bookmark status, already-open status, and penalties.

**Inspector evidence**

For each candidate, expose a score breakdown rather than only a final score.

---

## An already-open logical tab should be reusable

**Negative user story**

I search for a page already open in a sleeping or active logical tab, select it, and IB creates an unnecessary duplicate because the suggestion system only knows history URLs.

**Desired behavior**

Open logical tabs participate in candidate generation. Selecting one may focus/wake that tab instead of creating another visit when appropriate.

**Inspector evidence**

Show candidate source as open-tab, sleeping-tab, history, bookmark, learned choice, or other source.
