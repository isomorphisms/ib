# Full-corpus runbook

This run starts only after a one-time ChatGPT export ZIP is available outside
the Git worktree.  Do not substitute per-conversation UI traversal.

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

## 4. Compare representations

Run the random and chronological partitions separately.  Include all principal
views in the same frozen configuration:

```text
python3 experiments/conversation-classification/conversation_lab.py compare \
  --corpus experiments/conversation-classification/corpus \
  --events experiments/conversation-classification/corpus/assertions \
  --axis concept \
  --partitions experiments/conversation-classification/corpus/derived/partitions-v1 \
  --view full --view user --view assistant --view title_user --view structural \
  --rules experiments/conversation-classification/corpus/definitions/rules-v1.json \
  --output experiments/conversation-classification/corpus/derived/random-v1
```

Repeat with `--split-field chronological_split`.  Filing-destination evaluation
is a separate run with `--axis filing_destination --filing-evaluation`; only
that single-destination axis uses closed-world negatives.

Do not interpret a modern semantic embedding result unless an actual embedding
provider and model hash are recorded.  The included dense baseline is LSA.

## 5. Inspect and correct

Use `query` for unclassified rows, membership, overlaps, boundary cases, and
explanations; use `evidence` for weak-only provenance.  Diagnose representative
false positives and negatives against the raw source, including role
contamination, topic drift, incidental repository vocabulary, multi-task
threads, and incoherent definitions.

`correct` appends an authoritative event.  `project` overlays it on immutable
classifier output.  Retrain only the affected category when the correction is
to become training evidence; retain the prior model generation for comparison.

## 6. Measure incremental work

Train and classify are separate:

```text
python3 experiments/conversation-classification/conversation_lab.py train-model <training arguments>
python3 experiments/conversation-classification/conversation_lab.py classify-model <classification arguments>
```

For one new or changed item, pass `--only <corpus-id>` and
`--previous-proposals`.  The command reads the cheap index for all fingerprints,
parses only selected bodies, and reports classified versus reused counts and
elapsed time.  Use `destinations` to emit the final deterministic mapping with
confidence, margin, evidence, alternatives, and abstentions.

## 7. Adaptive stop

Increase reviewed examples only while category support, held-out filing risk,
coverage/review curves, or dominant failure clusters materially change.  Stop
when repeated additions leave the architecture choice and practical risk
estimate stable.  Never expand merely to reach a round corpus count.
