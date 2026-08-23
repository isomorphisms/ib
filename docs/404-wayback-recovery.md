# 404 recovery through the Internet Archive

## Principle

A live-server `404 Not Found` is not the end of navigation. It means IB should immediately try harder.

The first recovery source is the Internet Archive Wayback Machine. The original URL remains the history identity; an archived capture is a recovered representation of that failed navigation, not a silent rewrite of history.

This policy belongs to the Idriç browser core. A renderer may display the result, but it does not decide whether recovery happens.

## First behavior

For an ordinary navigation:

1. Fetch the requested URL normally.
2. If the response is not a 404, continue the ordinary path.
3. As soon as a 404 is known, record the live response and start a Wayback availability lookup for the requested URL.
4. Query:

   `https://archive.org/wayback/available?url=<percent-encoded-original-url>`

5. If `archived_snapshots.closest.available` is true and the capture is usable, fetch that archived URL through the ordinary fetch/storage boundary.
6. Store the archived response as another representation attached to the same history entry.
7. Pre-render the archived body immediately when it is fetchable.
8. Display a visible source marker such as `Archived 2019-04-03` so IB never pretends an archived copy is the live page.
9. If the availability lookup has no usable capture, show the original 404 without blocking further browser use.

The Wayback Availability API is deliberately a cheap first query: it returns one closest available capture. It is explicitly suitable for a 404/error-handler path.

## Try harder, but in stages

The availability lookup is the latency-sensitive first attempt. A second recovery stage may use the Wayback CDX index when the simple availability result is empty, unusable, or points at a capture whose replay fails.

CDX can answer a more specific question: are there successful captures of this exact URL, and which timestamps are available?

Initial CDX policy:

- exact URL before wildcard/prefix searches
- prefer HTTP 200 captures
- prefer the original MIME type when known
- prefer a capture with a body over redirects or archive error pages
- normally prefer the newest successful capture
- keep older candidates available instead of discarding their timestamps

Do not make the first 404 paint wait on an exhaustive archive search. Recovery can improve after the initial result is visible.

## Local snapshots are also recovery material

IB already treats snapshots as durable browser-owned data. If IB has a previous local body for the URL, that body is immediately useful on a 404.

A sensible race is:

```text
live 404
   |
   +--> local stored representation -> immediate recoverable view
   |
   +--> Wayback availability lookup -> archived recoverable view
                                      |
                                      +--> optional CDX second search
```

The local snapshot and Wayback capture are separate provenance records. Neither overwrites the live 404.

## History and provenance

A recovered history entry should retain at least:

- requested URL
- live resolved URL, if any
- live status = 404
- time the live request was made
- local snapshot hash, if used
- archive lookup time
- Wayback capture timestamp
- Wayback replay URL
- archived original URL
- archived status/MIME type when known
- stored content hash for the recovered body

This makes later inspection and LLM use straightforward: the browser can answer both "what happened when I visited this?" and "what older content did IB recover?"

## No recursion traps

Archive recovery must terminate.

- Do not Wayback-recover a failing `web.archive.org` replay by recursively asking Wayback about itself.
- Do not create a new user-visible history entry merely because recovery followed an archive replay URL.
- Put a small per-navigation budget on archive lookup attempts.
- Deduplicate identical capture bodies by content hash.

## Cache policy

Wayback availability results are disposable acceleration data, not canonical history.

It is reasonable to cache a negative or positive availability result briefly so repeated paints do not hammer the service. A later navigation may retry because a live 404 can be transient and the archive index can change.

The fetched archived body, once stored, is a durable snapshot and can outlive that lookup cache.

## UI behavior

The desired failure experience is not a dead error page. It is approximately:

```text
This page returned 404.
Showing an archived copy from 2017-11-06.
[original 404] [archived copy] [other captures]
```

If a local stored copy exists it can be offered alongside the archive capture.

The archived copy should be eligible for the same cheap text/preview path as any other stored HTML body.

## Initial deterministic fixtures

The first implementation should be testable without depending on live archive.org behavior.

1. live 200: no Wayback request
2. live 404 + availability says capture exists: archive fetch is scheduled
3. live 404 + no capture: original 404 remains final
4. live 404 + local snapshot + archive capture: local can paint immediately; archive remains a separate source
5. live 404 + availability capture replay fails: preserve original 404 and permit CDX second-stage lookup
6. archive replay itself returns 404: no recursive Wayback lookup
7. repeated visit within lookup-cache window: no duplicate availability request
8. two archive timestamps with identical content hashes: one stored body, two provenance records

## Implementation boundary

The policy/state machine belongs in `.idric` source. HTTP is supplied through IB's network boundary (eventually through the small Idriç/ICU path); the Internet Archive does not become a special renderer.

The smallest implementation slice is therefore:

- an Idriç recovery decision type
- a Wayback availability request/response decoder
- provenance fields for recovered representations
- deterministic fixtures for the transitions above
- then the real network adapter
