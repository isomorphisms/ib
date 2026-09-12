# Title-only diagnostic — not the corpus experiment result

The only account-wide artifact available on 2026-09-12 contains 219 titles and
weak assistant-proposed filing destinations, but no conversation bodies.  This
run tests ingestion, duplicate grouping, train-only fitting, independent
separators, abstention, model persistence, and the query surface.  It does not
test semantic conversation classification.

The deterministic random split contains 151 train, 27 development, and 41 test
rows in 212 duplicate groups.  Thresholds target 90% precision on available
development labels, with documented fallbacks for sparse categories.  On the 41
test rows, sparse title hyperplanes automated 63.4% and disagreed with the weak
proposal on 65.4% of those choices.  Dense LSA hyperplanes automated 51.2% and
disagreed on 71.4%; the positive-centroid variant automated 61.0% and disagreed
on 76.0%.  Examples include classifying “August 12 2026 Eclipse” as home
repairs, “Mein Kampf Anti-Semitism Analysis” as music, and “ESCO 608 Exam
Details” as literature.  Small category development samples did not make the
title-only geometry safe.

Exploratory exact rules covered 31.7% with no disagreement against the weak
proposal.  Adding sparse hyperplanes raised coverage to 65.9% but produced a
51.9% disagreement rate, erasing the rules' precision advantage.  Those figures
are deliberately not headline results: the rules were written after inspecting
this ledger, and the reference labels are not user-verified.  The exact
machine-readable figures and limitations are in
`title-only-diagnostic.json`.

The chronological test is worse: sparse, dense-hyperplane, and dense-centroid
misfile rates among automatic choices are 85.7%, 84.6%, and 93.8%.  Exact rules
retain zero disagreement but cover only 12.5%; adding sparse geometry raises
coverage to 46.9% while misfiling 80.0% of automatic choices.  Even as a weak
diagnostic, this is strong evidence against treating a random title split as an
adequate proxy for future filing.

This diagnostic supports one narrow architectural decision: filing policy
should permit abstention, and category scores must be ranked by margin relative
to each category's own threshold.  It does not answer whether user-only text
helps, whether semantic hyperplanes beat rules, or whether overlapping concepts
are useful.  Those conclusions remain blocked on the one-time export.
