# Inference and learning boundary

IB should interface cleanly with user-configured local or remote language, embedding, reranking, and summarization models without allowing a probabilistic model or provider to become the owner of browser state.

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

The adapter should identify the model and task explicitly so models can be replaced, compared, or run as an ensemble without changing canonical records or callers. Browsing must remain usable when every model is absent, slow, or crashes. Local models receive only task-relevant corpus material. Remote models receive only an explicitly scoped, inspectable context export; secret and session storage and unrelated private browsing state are excluded by default.

## Classification baseline

Category membership is multilabel. A useful first baseline is one independent inclusion scorer per category rather than a single exclusive multiclass classifier. It is binary relevance only in the sense of asking one category question at a time; material outside category `c` is not automatically a negative example for `c`.

For category `c`, an explicit linear baseline is an affine score and a separately recorded policy threshold:

```text
s_c(x) = w_c dot phi(x) + b_c
propose c when s_c(x) >= tau_c
```

`b_c` is normally learned, so the decision surface is affine and is not forced through the origin. `tau_c` need not be zero: it should reflect the cost of hiding relevant material. There is no argmax across categories. An authoritative human membership remains included and an authoritative category-scoped exclusion remains excluded regardless of a later model score; passing the threshold is still a proposal, not silent acceptance.

Train from explicit or trusted positive and negative evidence; where absence is merely unlabeled, use a positive-unlabeled treatment rather than declaring every other object negative. Useful features include:

- URL, host, title, MIME type, and source;
- extracted text or image description;
- referring tab, search, or task;
- time, recency, and revisit spacing;
- prior accepted memberships and explicit corrections;
- neighborhood or vector similarity.

In a soft-margin SVM, each labeled example has a scalar slack variable measuring violation of the desired margin. The collection of those scalars may be called a slack vector. Support vectors are instead the training examples with nonzero dual weight that determine the separator; some lie on the margin and some violate it. The signed geometric distance to the fitted zero surface is `s_c(x) / norm(w_c)`; distance to the model-policy threshold surface is `(s_c(x) - tau_c) / norm(w_c)`. Raw score, normalized distance, slack, support-vector status, held-out error, and ensemble disagreement are separate diagnostics; none is automatically a calibrated probability or an inclusion band.

One-class SVM is an origin-related construction that separates examples from the feature-space origin with an offset. It is a possible positive-only probe, not the ordinary soft-margin binary SVM and not the default once explicit negative corrections exist.

IB may fit another affine separator inside a coherent region or against residual errors from an earlier separator. That yields a collection of binary decisions—possibly an oblique tree, a boosted ensemble, or overlapping category scorers—not a compulsory single hierarchy. A parent and a narrower category may both remain true.

Bagging may fit planes over row, feature, or provisional-unlabeled resamples and retain every plane before voting or averaging; this is a useful positive-unlabeled baseline. Boosting may fit later learners against earlier errors. A sum of unthresholded linear scores collapses algebraically to one linear score, while thresholded-plane voting or tree structure can represent a more elaborate boundary. Vote fraction still is not automatically a probability.

Zero, one, or many categories may pass their per-category decisions. Retrieval should generally prefer an extra plausible membership to hiding material because another category won.

## Human organization is supervision

Machine learning should learn from intentional human organization, not merely from corrections made after a bad proposal.

- creating or naming a category supplies category semantics;
- adding membership supplies an authoritative category-scoped positive;
- removing membership supplies an authoritative negative for that category only;
- accepting a split or merge supplies a structural constraint;
- grouping resources into a task or reading bundle supplies relationship and ranking evidence;
- explicitly meaningful pinning or ordering may supply attention evidence.

Each signal retains its original event, target kind, scope, and authority instead of being flattened into a universal label. Incidental filesystem order, passive visibility, `_active` removal, and unaccepted model output are not negative classification evidence.

## Corrections and assertions are training events

A drag, drop, rename, membership addition, membership removal, or accepted structural change is a typed human assertion. Record the event and update derived models conservatively; do not overwrite the model proposal that prompted it.

Dropping an object into category `B` is an authoritative positive assertion for `B` and changes that view immediately. It is an add, not a move: existing membership in `A` remains because categories overlap. Explicitly removing `B` is negative evidence only for `B`. Removing `B` from `_active` is attention control and produces no classification or training event. Never train on a model's own unaccepted labels.

The desired interaction is that two or three nearby corrections noticeably improve subsequent proposals. That is an evaluation target, not a promise that every category will converge after three examples. An update creates a new replayable model version, is bounded or regularized toward its predecessor, and is validated before activation. Rollback leaves the correction events and explicit memberships intact. Tests should measure whether corrections improve ranking and membership on held-out nearby items without erasing older useful categories.

Human correction has higher authority than a later model guess. A model may propose revisiting it, but may not silently reverse it.

## Browsing history as evidence

A history-analysis view may ask: *what is this a record of about the user?* It can surface recurring projects, purposes, people, subjects, abandoned and resumed threads, or changes in attention.

Those are evidence-backed interpretations, not facts about identity or belief. Every interpretation should retain links to the searches, visits, tabs, or accepted categories that support it. A repeated URL or duplicate tab is evidence of salience or revisitation, not redundant noise and not by itself proof of endorsement.

Private source material and credentials remain subject to the storage and export boundaries whether inference is local or remote. A public fixture should not acquire private URLs merely because a model could classify them.

## Implementation status

The generic proposal, validation, aggregation, and correction records described here are not implemented on `main`. The current low-priority harvested-image naming pass predates this boundary: it validates a rename plan but then mutates filenames and a manifest directly, without retaining model identity, score, or prompt/source provenance. Treat it as a narrow earlier adapter to replace, not as the pattern for category, summary, or history inference.

## Commitment level

Settled boundaries:

- local and remote model adapters are replaceable and provider-neutral;
- model output is append-only evidence, never direct canonical mutation;
- proposals retain provenance, model identity, scores, and source references;
- deterministic code owns validation and acceptance;
- human corrections are durable events;
- multilabel classification does not require one winning category.

Current baselines and evaluation ideas:

- Float32 embeddings and exact vector search at the current 10,000-URL scale;
- one explicit affine inclusion score and a separately recorded threshold per category using positive, explicit-negative, and unlabeled evidence correctly;
- ensembles when disagreement is useful;
- measurable improvement after a few nearby corrections.

Still open:

- the embedding model and feature weights;
- the online update rule;
- acceptance thresholds and which proposals require explicit review;
- the exact proposal serialization;
- whether a given category uses one classifier, an ensemble, or a non-linear replacement.
