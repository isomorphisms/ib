# Inference and learning boundary

IB should interface cleanly with one or more local language or embedding models without allowing a probabilistic model to become the owner of browser state.

## Observations and proposals, not mutations

A model result is an append-only observation or proposal. It may suggest:

- category membership;
- a category name, split, or merge;
- task or purpose;
- relevance or ranking;
- a summary or extracted fact;
- a relationship between resources;
- a description of what a stretch of browsing history records.

It must not silently rewrite history, delete a prior label, replace a canonical resource, or directly make its current guess authoritative.

A proposal should retain enough information to reproduce or challenge it:

```text
proposal_id
target_kind
target_id
predicate
proposed_value
raw_score_and_meaning
calibrated_confidence_if_available
model_id
model_revision
policy_or_prompt_revision
created_at
source_references
supporting_features_or_evidence
```

This is a conceptual record, not a frozen serialization.

The proposal itself is immutable. Acceptance, rejection, supersession, and aggregation are separate append-only decision events that cite `proposal_id`; current status is a derived view over those decisions.

Accepted memberships and other derived views may be materialized for fast use, but those materializations must be rebuildable from canonical events, human decisions, and retained proposals.

## Deterministic reduction

Deterministic code may validate, compare, reduce, accept, or reject proposals. Validation should check at least target identity, source availability, schema, score range, and whether the proposed action would violate a browser invariant.

When several models or resampled classifiers are useful, preserve their individual outputs before reducing them. Bagging, voting, or rank aggregation should not erase disagreement. An aggregate records its inputs, quorum, reducer, and reducer version. A missing model is not a negative vote, and disagreement is itself evidence that a category boundary or ranking is uncertain.

The adapter should identify the model and task explicitly so local models can be replaced, compared, or run as an ensemble without changing canonical records or callers. Browsing must remain usable when every model is absent, slow, or crashes. Local models receive only explicitly selected corpus material; secret and session storage are excluded by default.

## Classification baseline

Category membership is multilabel. A useful first baseline is one scored binary classifier per category rather than a single exclusive multiclass classifier.

A linear baseline can use one separator per category over persistent Float32 embeddings and cheap structured features. Train from explicit or trusted positive and negative evidence; where absence is merely unlabeled, use a positive-unlabeled treatment rather than declaring every other object negative. Useful features include:

- URL, host, title, MIME type, and source;
- extracted text or image description;
- referring tab, search, or task;
- time, recency, and revisit spacing;
- prior accepted memberships and explicit corrections;
- neighborhood or vector similarity.

The literal classifier remains replaceable. A margin is a score, not an ontology. Slack, support examples, and disagreement among plausible separators should remain inspectable where they help explain uncertainty.

Zero, one, or many categories may pass their per-category decisions. Retrieval should generally prefer an extra plausible membership to hiding material because another category won.

## Corrections are training events

A drag, drop, rename, membership addition, or membership removal is an explicit human correction. Record the correction as an event and update derived models conservatively; do not overwrite the model proposal that prompted it.

Dropping an object into category `B` is an authoritative positive assertion for `B` and changes that view immediately. It is an add, not a move: existing membership in `A` remains because categories overlap. Explicitly removing `B` is negative evidence only for `B`. Removing `B` from `_active` is attention control and produces no classification or training event. Never train on a model's own unaccepted labels.

The desired interaction is that two or three nearby corrections noticeably improve subsequent proposals. That is an evaluation target, not a promise that every category will converge after three examples. An update creates a new replayable model version, is bounded or regularized toward its predecessor, and is validated before activation. Rollback leaves the correction events and explicit memberships intact. Tests should measure whether corrections improve ranking and membership on held-out nearby items without erasing older useful categories.

Human correction has higher authority than a later model guess. A model may propose revisiting it, but may not silently reverse it.

## Browsing history as evidence

A history-analysis view may ask: *what is this a record of about the user?* It can surface recurring projects, purposes, people, subjects, abandoned and resumed threads, or changes in attention.

Those are evidence-backed interpretations, not facts about identity or belief. Every interpretation should retain links to the searches, visits, tabs, or accepted categories that support it. A repeated URL or duplicate tab is evidence of salience or revisitation, not redundant noise and not by itself proof of endorsement.

Private source material and credentials remain subject to the storage and export boundaries even when inference runs locally. A public fixture should not acquire private URLs merely because a model could classify them.

## Implementation status

The generic proposal, validation, aggregation, and correction records described here are not implemented on `main`. The current low-priority harvested-image naming pass predates this boundary: it validates a rename plan but then mutates filenames and a manifest directly, without retaining model identity, score, or prompt/source provenance. Treat it as a narrow earlier adapter to replace, not as the pattern for category, summary, or history inference.

## Commitment level

Settled boundaries:

- local-model adapters are replaceable;
- model output is append-only evidence, never direct canonical mutation;
- proposals retain provenance, model identity, scores, and source references;
- deterministic code owns validation and acceptance;
- human corrections are durable events;
- multilabel classification does not require one winning category.

Current baselines and evaluation ideas:

- Float32 embeddings and exact vector search at the current 10,000-URL scale;
- one scored linear decision per category using explicit or trusted labels;
- ensembles when disagreement is useful;
- measurable improvement after a few nearby corrections.

Still open:

- the embedding model and feature weights;
- the online update rule;
- acceptance thresholds and which proposals require explicit review;
- the exact proposal serialization;
- whether a given category uses one classifier, an ensemble, or a non-linear replacement.
