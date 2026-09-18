# Pensieve body-text hyperplane experiment

This experiment addresses the main weakness of the earlier category-hyperplane probe: URL and title strings are not enough evidence for semantic separation.

The primary input here is distilled Pensieve body text. The experiment remains derived and disposable until held-out body-text evidence justifies promotion.

## Boundary

- Pensieve source observations are authoritative input and are never rewritten by the model pass.
- `bin/pensieve_hyperplane_inputs.grease` can run as an `after-distill.d` hook or as a full rebuild. It writes only under `pensieve/indexes/hyperplanes/`.
- `build_pensieve_inputs.py` chooses one body representation per item in this order: HTML, PDF text, abstract text. It does not concatenate duplicate renderings of the same paper.
- Long bodies are represented by deterministic windows spread across the document rather than by a title, URL, or only the first few hundred tokens.
- Embeddings and fitted planes are derived state. The retained model state is explicit: embedding provenance, plane normal, offset, thresholds, support rows, provisional-unlabeled rows, slack diagnostics, proposal-policy hash, and input hashes.
- Missing membership remains unlabeled. Unlabeled rows may be sampled provisionally as comparison rows for a plane; that does not assert that they are negative.
- Semantic labels and fit partitions are separate inputs. An optional complete `id → partition` map can mark every row as `fit`, `development`, or `held_out`, including rows with no semantic label.
- Development and held-out rows are excluded from plane fitting and provisional-unlabeled sampling even when they are unlabeled. A semantic label whose role disagrees with its partition is rejected.
- Filing destinations remain a separate policy layer. Hyperplane margins do not rank destinations across concepts.

## Deterministic integration check

`tests/test_pensieve_hyperplane_hook.grease` installs the body-input builder at the real 0.2 `after-distill.d` boundary, distills the existing fake arXiv fixture, verifies that the derived representation uses Pensieve body text, and verifies that deleting the derived hyperplane index leaves the Pensieve source text byte-identical.

## Real-body diagnostic

The workflow stages a disposable arXiv corpus in a temporary Pensieve-shaped directory. This staging helper is **not** the IB acquisition implementation; production acquisition remains behind the 0.2 ICU boundary. Temporary paper bodies are not uploaded as workflow artifacts.

The diagnostic concept is `piecewise-linear-network-geometry`.

Fit positives:
- `1901.09021`
- `1606.05336`

The first exact-head body run used `2305.00241` as a held-out positive. It ranked immediately below the two fit positives and lay above the zero surface in every retained plane, but the old fit-positive-floor threshold rejected it. After inspection it became development evidence and can no longer be counted as an unbiased test.

## Frozen proposal policy v2

`proposal-policy-v2.json` was committed before any fresh held-out papers were added. The policy is:

- aggregate score = mean signed geometric distance across retained planes;
- fit-positive floor = the threshold required to retain the configured fraction of fit positives;
- proposal score floor = `0.70 * fit-positive floor`;
- proposal also requires a zero-surface vote fraction of `1.0`.

The 0.70 ratio is deliberately coarser than the observed development ratio (`0.718...`). It is frozen for the next test and must not be changed after seeing the fresh held-out rows without retiring those rows from test status.

## Fresh test after the freeze

Fresh positives:
- `2206.08615` — regions of piecewise-linear neural networks;
- `2006.00978` — linear regions of convolutional neural networks.

Fresh hard negatives:
- `2104.13478` — geometric deep learning broadly, rather than piecewise-linear region geometry;
- `1806.07366` — neural ordinary differential equations.

The original unlabeled comparison pool remains:
- `1107.0595`
- `2203.11355`
- `1706.03762`

These labels are experiment fixtures based on paper subject matter. They are not durable user organization and must not be imported as personal category assertions.

CI deliberately does not require a good fresh-test score. A poor result is evidence, not a test failure. CI enforces isolation, body-only input, explicit portable plane state, the frozen proposal-policy boundary, and reproducibility.

## What this does not establish

This small corpus cannot establish general semantic quality. Promotion into a persistent Pensieve hyperplane index requires broader held-out evidence across multiple overlapping concepts.
