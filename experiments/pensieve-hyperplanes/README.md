# Pensieve body-text hyperplane experiment

This experiment is intentionally derived from Pensieve body text rather than URL or title strings.

It is not yet a production classifier. It exists to answer a narrower question: can explicit positive/unlabeled affine separators recover held-out conceptual memberships from distilled document text strongly enough to justify a persistent hyperplane index?

The intended execution boundary is the shell-first Pensieve `after-distill.d` hook. Source observations stay in Pensieve; all representation rows, fitted planes, scores, and reports are rebuildable derived state.

## Evidence rules

- Human assertions and corrections are supervision.
- Missing membership is unlabeled, not negative.
- Categories may overlap.
- Held-out documents do not participate in representation fitting, plane fitting, or proposal-threshold selection.
- Filing destinations are a separate policy layer and are not ranked by cross-category hyperplane margins.
- Portable plane state is explicit: representation definition, normal, offset, threshold, vote policy, and provenance hashes. Python/joblib state is not authoritative.

## Acceptance target

The first useful run must use actual body text, include document-disjoint held-out positives, and report held-out recall/rank, abstentions, slack, and bag-to-bag normal rotation. A URL/title-only result is diagnostic only and does not satisfy this target.

See issue #61 for the complete acceptance boundary.
