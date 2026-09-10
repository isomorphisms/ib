# View-exposure implementation receipt — 2026-09-10

Target branch before this change:

```text
material3-arxiv-viewer
5ddeebcffb37dd3a2d2cc0bf0ca46e68527e1076
```

Relevant architecture check:

- the Material reader consumed the frozen Pensieve asset snapshot but had no
  fragment-level reverse path;
- the existing shell Pensieve state is local filesystem state with append-friendly
  conventions;
- PR #52 is open and mergeable at `43d4a63c60647c7a2a4df85d465c083cccc6f516`,
  but its fragment-link/strand change is documentation only and is not this
  branch's storage implementation;
- this change therefore adds view observations beside Pensieve state rather than
  creating another fragment graph or copying #52's filesystem projections.

Host verification:

```text
$ sh tests/test_view_exposure.grease
REAL_FRAGMENT_0=frag-489072684d050cd4733bd165-0-32
REAL_FRAGMENT_1=frag-489072684d050cd4733bd165-33-92
REAL_FRAGMENT_2=frag-489072684d050cd4733bd165-93-138
REAL_EXPOSURE_US=8000000
REAL_VIEW_UNITS=1
REAL_REMAINDER_US=1000000
view exposure tests: PASS (21 assertions)
fragment    frag-489072684d050cd4733bd165-33-92
weighted_exposure_us    8000000
weighted_exposure_ms    8000.000
view_units    1
remainder_us    1000000
remainder_ms    1000.000
real Pensieve view-state -> exposure query: PASS
```

The real exercise uses the checked-in `2203.11355` Pensieve PDF-text fixture, cuts
it into three stable source-range fragments, simulates repeated short downward
scrolls through the ordered fragment strand, persists the resulting observation
rows, reopens the store, and queries the middle fragment through `bin/ib exposure`.

The deterministic suite also covers 6/7/14/18-second stationary thresholds,
partial visibility, two visible fragments at once, entering/leaving, fast-scroll
suppression, cumulative return visits, and restart persistence.

Durability boundary: 500 ms observation sampling, durable append batches every four
samples (about two seconds), plus reader-disposal and Activity-stop flushes. A hard
failure can lose the small unflushed tail; flushed history survives restart.
