# Conversation classification corpus experiment

This directory is a disposable empirical branch experiment.  It asks whether
IB can classify previously unseen ChatGPT conversations into overlapping
conceptual memberships and then make a separate, defensible filing proposal.
It is not IB runtime code, not a canonical corpus, and not a ChatGPT mover.

The experiment starts from `main` at
`d199a992a70e54338e1f0dd73ffd82d6ca6bad96`.  Delete the experimental branch to
delete every repository artifact introduced for this work.  Do not merge the
branch merely because a classifier looks promising.

## Boundaries inherited from IB

The experiment follows the current architecture rather than creating another
browser model:

- source conversations and stable identities are canonical observations;
- source location is evidence, not conceptual truth;
- conceptual membership is independently scored and may be zero, one, or many;
- one filing destination is a later policy projection;
- classifier outputs are immutable proposals with model and input provenance;
- human corrections are append-only authoritative events;
- materialized indexes, vectors, scores, and destination views are rebuildable;
- secret/session material is outside the public corpus and model-input boundary.

`conversation_lab.py` and the `ib_conversations` package are conventional Python
reference machinery for this disposable comparison.  They do not change IB's
Idriç/Grease runtime boundary.  The existing Idriç work supplies the semantic
shape—dimensioned vectors, explicit dot products, residuals, and explicit
tolerances—while the older IB category-hyperplane probe supplies the existing
affine-separator precedent.  This experiment must not be reported as Idriç
execution unless an actual Idriç artifact is run.

## Source acquisition status

The preferred input is one ChatGPT data export containing `conversations.json`.
It provides stable conversation/message identifiers and complete message trees
without opening conversations individually.  The authenticated account export
path was inspected on 2026-09-12.  It requires reauthentication and sends an
asynchronous download link to the registered email; no export archive is
available in this environment yet.

The largest currently available account-wide artifact is a 219-row cleanup
ledger.  It contains conversation titles, dates, existing locations, and
assistant-proposed filing destinations.  It explicitly reports that only 12
conversation bodies were exposed.  Therefore it is retained only as a
title-only acquisition diagnostic and weak filing evidence.  It cannot answer
the main user-text, assistant-contamination, overlap, or semantic-separation
questions.

No experiment is built around repeatedly opening individual conversation pages.

## Public corpus gate

Never copy an export archive, `conversations.json`, browser profile, cookie
store, or unfiltered staging file into this repository.  Import from a path
outside the worktree:

```text
python3 experiments/conversation-classification/conversation_lab.py import-export \
  --source /outside/git/chatgpt-export.zip \
  --corpus experiments/conversation-classification/corpus \
  --acquired-at 2026-09-12T00:00:00Z
```

The importer writes only screened public-corpus material.  It excludes internal
roles and conversations with detected child-identifying context or private
third-party correspondence.  It redacts credentials, secret-bearing URLs,
financial/government identifiers, exact street addresses, email addresses, and
telephone numbers.  Each removal produces a mechanical record containing a
reason, location, original byte length, and one-way hash—not the removed text.

Automatic scanning cannot prove that prose contains no undiscovered private
fact.  `verify-public` is a required deterministic gate for known detector
families, inventory consistency, and source hashes.  The scanner revision is
part of reuse identity, so a detector change rescreens every source record.

## Corpus layout

```text
corpus/
  raw/                 screened source-faithful JSON by stable corpus id
  metadata/            source identity, chronology, message graph, and hashes
  index.jsonl          inspectable corpus inventory
  safety/
    exclusions.jsonl   whole-conversation exclusions
    message-exclusions.jsonl omitted role/message receipts
    redactions.jsonl   span-level redaction receipts
    report.json        counts and blocking status
  assertions/          append-only human/accepted label events
  definitions/         persistent category and filing-destination definitions
  derived/             feature/model generations and immutable proposals
  views/               rebuildable query projections and Markdown inspection
  reports/             evaluation and failure analysis
```

The public-safe files under `raw/` are “raw” relative to classification: they
retain original message text, roles, boundaries, order, and branching rather
than summaries.  They are structured JSON so classifiers never reparse a
rendered Markdown view.  They are not the unredacted account export.

## Evidence axes

Every label or proposal names its axis and provenance:

1. `existing_location` — where ChatGPT currently stores the conversation;
2. `concept` — overlapping semantic membership;
3. `filing_destination` — one policy projection for later filing.

Absence from a category is unlabeled.  Only an explicit removal or negative
correction is negative evidence for that category.  The authority order is
recorded rather than inferred from a filename: existing location, classifier
proposal, inferred membership, accepted decision, explicit user assertion, and
manual correction remain distinguishable.

## Reference comparison

The executable comparison keeps representation and separator distinct.  It can
run high-confidence regex rules; word/character TF-IDF plus structural features;
independent bagged positive/unlabeled linear separators; LSA dense vectors with
either separators or positive centroids; user/assistant/title weighting; and a
rules-plus-geometry hybrid.  The comparison includes `full`, `user`,
`assistant`, `title_user`, and `structural` input views.  Model features are fit
on the training split only.

For each category and each bag, the stored score is:

```text
score_c(x) = dot(normal_c, representation(x)) - offset_c
```

The normal is L2-normalized.  Acceptance is independent per category and uses a
calibrated threshold plus a vote threshold across positive/unlabeled bags.
Unlabeled examples sampled as provisional negatives are recorded as such; they
do not become durable negative labels.

Dense LSA is the practical offline dense baseline currently available.  It is
not a modern pretrained semantic embedding and must not be described as one.
If an approved local or API embedding source becomes available, it belongs
behind the same derived-representation boundary.

## Shell surface

Run `conversation_lab.py --help` for full arguments.  The intended sequence is:

```text
import-export  -> partition -> train-model -> classify-model -> project
```

`train-model` writes no proposals.  `classify-model --only <corpus-id>` can
reuse a previous proposal file after validating the unchanged model generation
and cheap index fingerprints; it does not parse other conversation bodies.
`correct` appends a user correction.  `project` overlays authoritative evidence
without rewriting either the source or immutable classifier proposal.

Queries cover unclassified rows, category membership, boundary proximity,
category overlap, per-conversation explanations, weak evidence, and changes
between proposal generations.  This is the deterministic projection layer only;
there is no ChatGPT mover.

The reference environment used Python 3.12, NumPy 2.3.5, SciPy 1.17.0, and
scikit-learn 1.8.0.  Stored joblib encoders are trusted local derived artifacts,
not a portable or safe interchange format.

## Reproducibility and stopping rule

Dataset partitions are group-based so exact and near duplicates cannot cross
train/development/test boundaries.  Both deterministic group-hash and
chronological splits are retained.  Model runs cite source hashes, input-view
grammar, feature configuration, category definitions, assertions, thresholds,
and software versions.

The 219-title diagnostic is documented under `reports/`.  Its labels are weak,
its rules are exploratory, and it is explicitly invalid for the primary
full-text conclusions.

Corpus expansion stops when held-out estimates and the dominant failure modes
are stable enough to decide the architecture.  A round count is not evidence.
