# Representation and held-out evaluation plan

This experiment is intended to decide which representation and classification
method actually generalizes to previously unseen ChatGPT conversations.  It must
not turn a successful fit on sparse supervision into evidence about an encoder,
or turn a failure of one separator into evidence against a representation.

## Evidence imported from IB PR #24

IB PR #24 provides a useful negative control at exact head
`f02f0f94bcfab94c243843f6dff98b5da9233c2c`.  Its first real
`mixedbread-ai/mxbai-embed-xsmall-v1` run embedded only the grammar
`url: <url>` and fitted each reading plane from three authoritative positives
plus six provisional-unlabeled rows.  Across 16 unique planes:

- held-out positive recall was 0/2;
- every one of the nine fit rows was a support vector in every plane;
- pairwise cosine between normalized plane normals ranged from 0.578891 to
  0.772758, with median 0.670730;
- changing only the provisional-unlabeled sample materially rotated the plane.

That is evidence that the URL-only three-positive problem is underdetermined.
It is not evidence that mxbai is generally a bad encoder.  The conversation
experiment therefore treats **representation**, **classification head**, and
**threshold policy** as separate experimental factors.

## Order of operations

1. Import and screen the body-bearing corpus.
2. Define conceptual categories and freeze versioned definitions.
3. Group exact and near duplicates.
4. Freeze both deterministic random and chronological train/development/test
   partitions.
5. Record the hashes of the corpus, definitions, assertions, and partitions.
6. Fit every contender from the same training rows.
7. Tune any thresholds on development rows only.
8. Evaluate every contender on the same untouched test rows.

Do not change category definitions, representation grammar, pooling, classifier
hyperparameters, rules, or thresholds after inspecting test outcomes.  A changed
choice is a new generation and requires a fresh untouched holdout.

## Representation matrix

The principal comparison is about conversation content, not URLs or current
folders.  Run the same partitions through these representation families.

### Controls

- `structural`: lengths, turn counts, role counts, and other non-semantic
  structural features only.  This is a nuisance-information control, not a
  serious semantic contender.
- `title`: title text only.  This preserves the existing weak baseline and makes
  the gain from conversation bodies measurable.
- `assistant`: assistant text only.  This is an assistant-contamination control;
  it is not the preferred representation even if it scores well.

### Sparse lexical baselines

- `user`: user messages only.
- `title_user`: title plus user messages.
- `full`: role-preserving full conversation text.
- the existing fixed weighted view with title/user/assistant contributions.

Use the existing word/character TF-IDF plus structural features, fitted on train
only.

### Offline dense baseline

For each semantic text view above, not just the first requested view, fit LSA
from the corresponding train-only sparse representation.  LSA remains an
offline dimensional-reduction baseline and must not be called a pretrained
semantic embedding.

### Pretrained semantic representation

Reuse the exact pinned mxbai model family only as a separately identified
representation generation.  Unlike PR #24, feed conversation **content**.
Record the model revision, model-file hash, tokenizer hash, runtime version,
pooling, input grammar, chunking, and truncation statistics.

Long conversations must not be reduced silently to an arbitrary first token
window.  Compare at least these deterministic, category-independent packings:

- `mxbai_user_message_mean`: embed user-message chunks separately, L2-normalize
  chunk vectors, average them, then normalize the conversation vector;
- `mxbai_title_user_mean`: include the title as its own chunk and pool it with
  user-message chunks under a fixed recorded weight;
- `mxbai_full_role_mean`: pool title, user, and assistant chunks with fixed
  recorded role weights.

Chunking must use the model/tokenizer limit and record how many chunks each
conversation required.  No category-specific chunk selection or test-informed
pooling is allowed.

## Classification-head matrix

A representation should not be judged from one decision rule.  For every
numeric representation that can support both operations, compare the same two
heads:

1. **positive centroid** — normalized positive prototype with development-only
   threshold calibration;
2. **bagged positive/unlabeled linear hyperplanes** — the existing independent
   category separator with recorded provisional-unlabeled samples.

This is the minimum crossed comparison needed to distinguish:

- a representation effect: both heads improve on the same held-out rows;
- a head effect: one head improves while the representation is held fixed;
- an interaction: a representation helps one head but not the other.

High-confidence regex rules remain a separate expert baseline.  A
rules-plus-geometry hybrid may be reported after the pure contenders, but it
must not hide which component supplied the gain.

The current `compare` implementation does **not** yet satisfy this matrix: sparse
hyperplanes run on every view, while LSA and centroid are currently attached
only to the first requested view.  Results from that asymmetric matrix are
exploratory until the comparison is fully crossed.

## Held-out measurements

Thresholded F1 alone is not enough under positive/unlabeled supervision.  For
each category and each contender, report the following on the identical test
rows.

### Metrics that use only explicit labels

- positive recall;
- precision, false-positive rate, and F1 only when explicit negative labels make
  those quantities defined;
- micro and macro summaries over categories, with the contributing label counts
  shown.

Unlabeled rows remain unlabeled.  Proposal rate on them is reported separately;
it is not a false-positive rate.

### Positive-ranking metrics

For each held-out positive, rank its category score among all test candidates
for that category and report:

- absolute rank and percentile rank;
- recall within the top 1%, 5%, 10%, and 25% review budgets;
- median held-out-positive percentile rank by category and overall.

These measures answer whether the representation puts unseen positives near the
front even when the threshold is poor or explicit negatives are sparse.  They
also make a PR-#24-style `0/N` threshold recall result diagnosable rather than a
single opaque failure.

### Paired comparison

All method comparisons are paired because they score the same held-out
conversations.  Report per-conversation/per-category deltas and paired bootstrap
intervals for the principal held-out metrics.  Keep the deterministic random and
chronological test regimes separate; do not pool them into one headline number.

A method does not "win" from one lucky category.  Show the number of categories
improved, unchanged, and degraded, and preserve representative failure cases.

## Positive/unlabeled stability diagnostics

PR #24 showed that a fitted plane can be more sensitive to provisional-unlabeled
sampling than the headline training fit suggests.  Every PU hyperplane result
must therefore report, per category:

- pairwise cosine distribution between normalized bag normals;
- per-example score and percentile-rank spread across bags for held-out
  positives;
- vote-fraction distribution;
- fraction of fitted rows on or inside the SVM margin (`y * f(x) <= 1`, within a
  fixed numerical tolerance);
- train-positive slack/margin diagnostics separately from provisional-unlabeled
  tension.

If nearly every fit row lies on the margin, bag normals rotate substantially,
and held-out ranks are unstable, call the separator **underdetermined**.  Do not
promote support-vector membership itself as useful selectivity in that regime.

## Evidence floors

The implementation may execute a category with the existing five-positive
training floor, but that is only a plumbing floor.  Categories with tiny
held-out positive counts are case studies, not architecture verdicts.  Global
claims should rely on multiple categories and paired uncertainty estimates, and
must show the actual number of train/development/test positives and explicit
negatives for every included category.

If the label set is still too sparse to distinguish contenders, the correct
result is "insufficient held-out evidence" rather than selecting the model with
the largest point estimate.

## Interpretation rules

Evidence for a **better representation** requires improvement on held-out rows
with the classification head held fixed, preferably for both centroid and PU
hyperplane heads and in both random and chronological regimes.

Evidence for a **better classification method** requires improvement with the
representation held fixed.  A head that fits training positives but gives
unstable held-out rankings under unlabeled resampling has not established a
better method.

Evidence for **assistant contamination** exists when assistant-only or
full-conversation representations outperform user-only representations mainly
because the assistant repeats project/category names.  Inspect those gains as a
failure mode before accepting them.

Evidence for **semantic value from mxbai** exists only if a content-bearing
mxbai representation improves held-out ranking or explicit-label metrics against
matched sparse/LSA baselines.  PR #24's URL-only negative result neither proves
nor disproves that.

The experiment stops when the paired held-out ordering of the serious
contenders and the dominant failure modes remain stable under additional
reviewed labels.  It does not stop because one training fit becomes clean.
