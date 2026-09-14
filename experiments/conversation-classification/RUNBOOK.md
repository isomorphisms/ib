# Full-corpus runbook

This run starts only after a one-time ChatGPT export ZIP is available outside
the Git worktree.  Do not substitute per-conversation UI traversal.

`EVALUATION.md` is normative for the representation/classifier comparison.  In
particular, the same frozen held-out rows must be used for every contender, and
representation, classification head, and threshold policy must remain separate
experimental factors.

## 1. Import and publication gate

```text
python3 experiments/conversation-classification/conversation_lab.py import-export \
  --source /outside/git/chatgpt-export.zip \
  --corpus experiments/conversation-classification/corpus \
  --acquired-at <UTC timestamp>

python3 experiments/conversation-classification/conversation_lab.py verify-public \
  --corpus experiments/conversation-classification/corpus
```

Inspect `corpus/reports/acquisition-distribution.json` before sampling or
judging results.  Commit no raw file if the public verifier fails.  The ZIP and
unfiltered `conversations.json` never enter Git.

## 2. Define and annotate

Induce candidate concepts from source locations, repeated explicit user filing
decisions, frequent corpus themes, and inspected heterogeneous examples.  Write
versioned definitions under `corpus/definitions/`; do not promote the title-only
destinations automatically.  Record positive memberships and explicit
negatives as assertion events.  Missing membership stays unlabeled.

Start with broad coverage across source locations, chronology, conversation
length, repository/non-repository material, and related categories.  Add labels
where development boundaries, overlaps, and failure clusters remain unstable.
Keep the test labels untouched while rules and thresholds are being developed.

Define filing destinations separately from concepts.  After concepts are stable
enough to evaluate, write a filing policy conforming to
`corpus/definitions/filing-policy.schema.json`.  A destination may draw support
from several concepts and weak inherited location evidence.  Do not make a
concept name a destination implicitly.

## 3. Partition before fitting

```text
python3 experiments/conversation-classification/conversation_lab.py partition \
  --corpus experiments/conversation-classification/corpus \
  --events experiments/conversation-classification/corpus/assertions \
  --axis concept \
  --output experiments/conversation-classification/corpus/derived/partitions-v1
```

The partitioner groups normalized branch titles, exact user-text duplicates,
and near duplicates before assigning deterministic random and chronological
splits.  Inspect category support; the reference classifier refuses categories
with fewer than five positive training examples, but five is an execution floor,
not evidence of a reliable estimate.

Freeze the partition, category-definition, assertion, and corpus hashes before
fitting contenders.  A material change after test inspection creates a new
experiment generation and requires a fresh untouched holdout.

## 4. Compare representations before choosing a classifier

Run the random and chronological partitions separately.  The comparison must be
paired: every contender sees the same training, development, and test rows.
Follow the full matrix in `EVALUATION.md`.

The existing command remains useful for the sparse baselines and current LSA
reference:

```text
python3 experiments/conversation-classification/conversation_lab.py compare \
  --corpus experiments/conversation-classification/corpus \
  --events experiments/conversation-classification/corpus/assertions \
  --axis concept \
  --partitions experiments/conversation-classification/corpus/derived/partitions-v1 \
  --view structural --view title --view user --view assistant --view title_user --view full \
  --rules experiments/conversation-classification/corpus/definitions/rules-v1.json \
  --output experiments/conversation-classification/corpus/derived/random-v1
```

Repeat with `--split-field chronological_split`.

Do **not** treat the current command's output as a decisive representation/head
comparison until the matrix is fully crossed.  At this branch state, sparse
hyperplanes run for every view, while LSA and centroid are attached only to the
first requested view.  Before architecture selection, run both the positive
centroid and bagged positive/unlabeled hyperplane heads on every serious numeric
representation, including content-bearing pretrained semantic embeddings when
their exact model/tokenizer/runtime provenance is recorded.

A direct `filing_destination` classifier may still be run with
`--axis filing_destination --filing-evaluation` as a diagnostic baseline, but
it is not the primary filing architecture and must be reported separately from
the policy projection.

Do not interpret a modern semantic embedding result unless an actual embedding
provider and model hash are recorded.  The included dense baseline is LSA.  For
mxbai or another pretrained encoder, classify conversation content rather than
URLs, record deterministic message/chunk pooling and truncation statistics, and
preserve assistant-only as a contamination control rather than a preferred
input.

## 5. Evaluate held-out behavior, then inspect failures

Thresholded F1 is not sufficient under positive/unlabeled supervision.  For
each category and contender, preserve the ordinary explicit-label metrics and
also report held-out positive absolute/percentile ranks, recall within fixed
review-budget percentiles, and unlabeled proposal rate separately from any
false-positive rate.

For PU hyperplanes, also record bag-to-bag normal cosine, held-out score/rank
spread, vote fractions, and the fraction of fit rows on or inside the SVM
margin.  If almost every fit row is margin-active while provisional-unlabeled
resampling rotates the separator and held-out ranks move substantially, report
that category as underdetermined rather than choosing the cleanest training
fit.

Use `query` for unclassified rows, membership, overlaps, boundary cases, and
explanations; use `evidence` for weak-only provenance.  Diagnose representative
false positives and negatives against the raw source, including role
contamination, topic drift, incidental repository vocabulary, multi-task
threads, and incoherent definitions.

`correct` appends an authoritative event.  `project` overlays it on immutable
classifier output.  Retrain only the affected category when the correction is
to become training evidence; retain the prior model generation for comparison.
Do not reuse the inspected test set as development evidence.

## 6. Measure incremental work

Train and classify are separate:

```text
python3 experiments/conversation-classification/conversation_lab.py train-model <training arguments>
python3 experiments/conversation-classification/conversation_lab.py classify-model <classification arguments>
```

For one new or changed item, pass `--only <corpus-id>` and
`--previous-proposals`.  The command reads the cheap index for all fingerprints,
parses only selected bodies, and reports classified versus reused counts and
elapsed time.

Filing is a third operation and does not retrain the classifier:

```text
python3 experiments/conversation-classification/conversation_lab.py destinations \
  --proposals <resolved-concept-proposals.jsonl> \
  --policy experiments/conversation-classification/corpus/definitions/filing-policy-v1.json \
  --events experiments/conversation-classification/corpus/assertions \
  --output experiments/conversation-classification/corpus/derived/destinations-v1
```

The destination projection emits one of `confident_destination`,
`several_plausible_destinations`, or `no_sufficiently_supported_destination`.
It records the policy hash, support components, alternatives, and any explicit
filing correction that overrode or blocked policy output.  Per-category
classifier margins remain explanation evidence and are not summed across
categories.

## 7. Adaptive stop

Increase reviewed examples only while category support, paired held-out method
ordering, held-out filing risk, coverage/review curves, or dominant failure
clusters materially change.  Stop when repeated additions leave the
architecture choice and practical risk estimate stable in both random and
chronological regimes.

A category with very few held-out positives is a case study, not an architecture
verdict.  If the paired evidence is too sparse to distinguish contenders, record
`insufficient held-out evidence` rather than selecting the largest point
estimate.  Never expand merely to reach a round corpus count.
