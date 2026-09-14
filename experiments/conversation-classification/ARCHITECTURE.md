# Experimental architecture findings

## What already fits IB

The current IB design already distinguishes canonical observations from
append-only model proposals and deterministic reductions.  Its categorization
notes explicitly permit overlapping memberships, independent one-vs-rest
separators, positive/unlabeled training, explicit negatives only, and
authoritative user corrections.  The experiment uses those semantics directly:

| Need | Existing idea reused | Experimental object |
|---|---|---|
| stable object identity | observation identity | `conv-<hash>` derived from export conversation id |
| faithful source | canonical observation | screened raw JSON plus separate metadata |
| overlapping membership | many-to-many categories | independent category proposal entries |
| hyperplane provenance | append-only model result | model generation, plane arrays, threshold, input hash |
| user correction | higher-authority event | append-only assertion and deterministic overlay |
| querying | rebuildable views | JSONL proposals plus shell commands |

## Idriç boundary

Existing Idriç work provides dimension-indexed real vectors, dot products,
residuals, nonzero-normal invariants, and explicit tolerances.  The older IB
category probe already represents each category with independent affine planes.
This branch expresses every fitted separator as
`score(x) = dot(normal, representation(x)) - offset`, with a separate calibrated
acceptance threshold and bag vote.  Normals are L2-normalized so margins have a
consistent geometric meaning within one representation and fitted category.

No Idriç compiler is available in the environment, and current Idriç text and
embedding ingestion is not complete enough to run this corpus.  Python,
scikit-learn, SciPy, and NumPy are therefore an explicit empirical reference
backend.  Pickled encoders and Python model code are not proposed as canonical
IB runtime artifacts.  The portable candidate boundary is: ordered feature
definition, dimension, normal vector, offset, calibrated threshold, voting
policy, and hashes.

## Classification versus filing

Conceptual membership and ChatGPT filing are not the same classifier target.
The experiment now keeps the executable boundary explicit:

```text
screened conversation
  -> overlapping concept proposals
  -> authoritative concept corrections
  -> versioned filing policy
  -> one destination / several plausible / unsupported
```

A filing policy maps accepted concepts to named ChatGPT destinations with
inspectable positive weights.  Weak inherited location can contribute a small
policy weight but is not ground truth.  Per-category classifier margins are
retained in the explanation record but are deliberately not added across
categories: independently calibrated hyperplane margins are not automatically
commensurable destination scores.  Filing-policy support is a deterministic
policy score, not a probability.

Explicit user filing assertions and manual corrections are a separate durable
evidence axis.  A current high-authority positive filing assertion overrides the
policy.  A current high-authority negative assertion blocks that destination.
Neither operation rewrites the conversation source or the concept proposal.

A direct `filing_destination` classifier remains useful as a diagnostic
baseline, but it is not the primary filing architecture and must not be reported
as the concept-to-destination projection.

## Small missing abstractions exposed

The branch adds only experimental forms of abstractions IB will probably need:

- a screened conversation observation with stable source and message identity;
- three explicit evidence axes rather than one overloaded category field;
- versioned category definitions separate from trained weights;
- immutable model generations and source-hashed proposals;
- an authoritative correction overlay that never rewrites source or proposals;
- distinct train and classify operations, including one-object reclassification;
- a separately versioned filing policy that projects accepted concepts to ChatGPT locations and can abstain.

The full corpus may invalidate parts of this shape.  In particular, it remains
unknown whether one vector per conversation is adequate for drifting or
multi-task threads.  Segment-level representations with a conversation-level
reduction may be necessary, but should not be added before failures demonstrate
it.

## Invalidation rules

| Change | Reuse conversation parse | Reuse feature row | Reuse category plane | Reuse concept proposal | Reuse filing projection |
|---|---:|---:|---:|---:|---:|
| unchanged rerun | yes | yes | yes | yes | yes |
| one new conversation | n/a for new row | existing rows yes | yes until retraining | existing rows yes | existing rows yes |
| source text changes | no for that row | no for that row | yes until retraining | no for that row | no for that row |
| concept correction | yes | yes | no for affected category when retrained | classifier proposal remains; resolved view changes | affected row no |
| category definition changes | yes | usually | no for affected category | no for affected category generation | affected mappings no |
| input-view grammar changes | yes | no | no | no | no |
| filing policy changes | yes | yes | yes | yes | no |
| filing correction | yes | yes | yes | yes | affected row no |

Retraining and reclassification are deliberately separate.  Some model updates
mathematically require fitting against the training corpus; classifying one new
conversation against an unchanged model does not.  Filing-policy changes require
no classifier retraining at all.
