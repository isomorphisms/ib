# Reader view observations and fragment exposure

IB's reader is now a two-way boundary:

```text
Pensieve/model
    |
    v
reader/view
    |
    v
view observations
    |
    v
Pensieve/model
```

The reverse path records what material the reader actually presented. It does not
claim that the user looked at, read, understood, or attended to that material.

See `../android-material3/README.md` for the current Android adapter and
`storage-model.md` for the broader persistent-state model.

## Three separate concepts

Keep these distinct:

1. **View observation** — a sampled interval describing visible fragments,
   viewport position, and viewport movement.
2. **Weighted exposure** — elapsed display time accumulated for a fragment after
   visibility and movement weighting.
3. **View unit** — the current derived rule: one complete unit for every seven
   seconds of weighted exposure.

Seven seconds is not encoded in fragment identity or in the observation format.
The observation log retains weighted exposure below the threshold, so changing the
threshold later does not destroy history.

For weighted exposure `E`:

```text
view_units = floor(E / 7 seconds)
remainder  = E mod 7 seconds
```

Thus 18 seconds is two complete view units with four seconds carried forward.

## Fragment identity

The current Material reader cuts its displayed Pensieve text at non-empty text-line
boundaries. That is an adapter choice, not a permanent definition of an IB
fragment. Each fragment carries the source character range it displays.

The stable reader identifier is derived from:

- the arXiv source identity;
- a SHA-256 digest of the displayed Pensieve representation;
- the fragment's source start and end offsets.

It does not contain the current `pensieve/arxiv/...` directory pathname. Changing
reader history never changes the fragment identifier or canonical document bytes.
A later fragmenter can replace the line cut while preserving the same observation
and exposure interfaces.

## Observation record

The Android adapter samples the laid-out reader at 500 ms intervals. Each visible
fragment contributes one row for the interval. Rows are tab-separated with these
fields:

```text
end_epoch_ms
elapsed_ms
fragment_id
visible_pixels
fragment_pixels
visibility_ppm
stability_ppm
weighted_exposure_us
first_visible_item
first_visible_offset_px
viewport_height_px
movement_px
```

This is deliberately more information than the final integer view-unit count.
Elapsed time, approximate visibility, viewport position, movement, and the applied
weights remain inspectable. A later reducer can use a different seven-second
threshold or a different movement interpretation without pretending the old view
unit was raw evidence.

The current file is:

```text
<pensieve>/view-history/observations.tsv
```

That path is implementation-level local state, not part of fragment identity and
not a promise that the long-term filesystem projection keeps this exact spelling.

## Visibility

Visibility is weighted by the fraction of the fragment's useful displayed extent
inside the viewport. A fragment clipped to an edge therefore contributes less
than the same fragment held fully in view.

If two fragments share the viewport, both can accumulate weighted exposure during
the same wall-clock interval. Exposure is not a mutually exclusive active-fragment
counter.

The observation for an interval uses the average of the fragment's visibility at
the beginning and end of that interval. This gives an entering or leaving fragment
a tapered contribution instead of a binary visit.

## Scrolling and stability

Small scrolling does not reset anything. If the same fragment stays visible while
its top position moves, the next observation continues the same cumulative
exposure with the new visible fraction.

Movement is estimated from a fragment visible in both adjacent samples. When no
fragment survives between two non-empty samples, the movement estimate is at
least one viewport height. The first simple stability rule is:

```text
movement < 0.15 viewport    -> 1.00
0.15 .. < 0.40 viewport     -> 0.70
0.40 .. < 0.75 viewport     -> 0.25
>= 0.75 viewport            -> 0.00
```

The exact cutoffs are deliberately simple and changeable. `movement_px` is stored
alongside `stability_ppm`, so later code is not forced to treat this first weighting
rule as ground truth.

## Persistence and durability

Reader history is local user state. The Android app writes it beneath its private
files directory; the shell model uses the equivalent Pensieve root selected by
`IB_HOME`.

The UI samples twice per second but does not perform a durable write for every
sample. Observations accumulate in memory and are appended as one batch every four
samples, approximately every two seconds. Each batch is flushed and `fsync`ed.
Changing the selected article, disposing the reader effect, and ordinary Activity
stop also flush pending observations.

A hard process or device failure may therefore lose a very small unflushed tail,
normally at most about two seconds. Already flushed reading history survives
reader/process restart. The append-only log avoids rewriting fragment contents,
link indexes, or a large exposure table while scrolling.

No networking, telemetry service, analytics SDK, advertising tracker, or third
party is involved.

## Query surface

Given a stable fragment id:

```text
ib exposure FRAGMENT-ID
```

returns:

```text
fragment                 <id>
weighted_exposure_us     <microseconds>
weighted_exposure_ms     <milliseconds>
view_units               <complete seven-second units>
remainder_us              <sub-unit microseconds>
remainder_ms              <sub-unit milliseconds>
```

The command folds the append-only observation log. That is intentionally a small
first query surface; a rebuildable compact index can replace the scan if history
volume makes it hot.

## Relationship to graph and strands

Exposure belongs to the stable fragment, not to the path used to reach it. The
same fragment can accumulate exposure whether it came from document order, a
materialized strand, search, history, or a semantic link.

The current Material branch only has an ordered article-fragment adapter. PR #52's
multiply indexed fragment graph and materialized-strand design is separate work;
this implementation does not create a competing graph store and does not freeze
its proposed directory projections. A later strand reader can emit the same
`ViewportSnapshot` observations without changing the exposure ledger.
