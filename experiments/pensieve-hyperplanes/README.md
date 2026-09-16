# Pensieve body-text hyperplane experiment

This experiment addresses the main weakness of the earlier category-hyperplane probe: URL and title strings are not enough evidence for semantic separation.

The primary input here is distilled Pensieve body text. The experiment remains derived and disposable until held-out body-text evidence justifies promotion.

## Boundary

- Pensieve source observations are authoritative input and are never rewritten by the model pass.
- `bin/pensieve_hyperplane_inputs.grease` can run as an `after-distill.d` hook or as a full rebuild. It writes only under `pensieve/indexes/hyperplanes/`.
- `build_pensieve_inputs.py` chooses one body representation per item in this order: HTML, PDF text, abstract text. It does not concatenate duplicate renderings of the same paper.
- Long bodies are represented by deterministic windows spread across the document rather than by a title, URL, or only the first few hundred tokens.
- Embeddings and fitted planes are derived state. The retained model state is explicit: embedding provenance, plane normal, offset, threshold, support rows, provisional-unlabeled rows, slack diagnostics, and input hashes.
- Missing membership remains unlabeled. Unlabeled rows may be sampled provisionally as comparison rows for a plane; that does not assert that they are negative.
- Held-out rows are excluded from plane fitting, provisional-unlabeled sampling, and threshold selection.
- Filing destinations remain a separate policy layer. Hyperplane margins do not rank destinations across concepts.

## Deterministic integration check

`tests/test_pensieve_hyperplane_hook.grease` installs the body-input builder at the real 0.2 `after-distill.d` boundary, distills the existing fake arXiv fixture, verifies that the derived representation uses Pensieve body text, and verifies that deleting the derived hyperplane index leaves the Pensieve source text byte-identical.

## Real-body diagnostic

The workflow also stages a disposable six-paper arXiv corpus in a temporary Pensieve-shaped directory. This staging helper is **not** the IB acquisition implementation; production acquisition remains behind the 0.2 ICU boundary. The temporary paper bodies are not uploaded as workflow artifacts.

The diagnostic concept is `piecewise-linear-network-geometry`:

- fit positives: `1901.09021`, `1606.05336`;
- held-out positive: `2305.00241`;
- unlabeled pool: `1107.0595`, `2203.11355`, `1706.03762`.

These labels are an experiment fixture based on paper subject matter. They are not durable user organization and must not be imported as personal category assertions.

The body sketches are embedded with the same pinned `mixedbread-ai/mxbai-embed-xsmall-v1` INT8 ONNX representation used by the earlier real-embedding probe. `body_probe.py` then fits independent linear soft-margin SVM planes with positive/unlabeled resampling and reports held-out recall, proposal threshold, support examples, slack, ranking, and pairwise cosine between normalized plane normals.

No minimum held-out score is asserted in CI. A poor result is evidence, not a test failure. CI only enforces data isolation, body-only input, explicit portable plane state, and reproducibility boundaries.

## What this does not establish

Six papers and one held-out positive cannot establish general semantic quality. A useful result here answers only whether the body-text path behaves materially better than the earlier URL/title diagnostic and whether it is stable enough to justify a larger labeled corpus. Promotion into a persistent Pensieve hyperplane index requires broader held-out evidence across overlapping concepts.
