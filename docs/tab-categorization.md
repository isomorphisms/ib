# Adaptive tab categorization

IB should organize material by why it matters to the user, not primarily by the media type or site that happens to contain it.

A URL may be an article, book, video, product page, forum post, image, or PDF. Those remain useful fetching, rendering, and classifier features, but they are usually not the user's central category. A wrench product page may belong to `car-repair`; glue and a sanding block may belong to `home-repair`; a book listing may belong to both its subject and `book-shopping`.

## Personal vocabulary, not a universal ontology

The vocabulary is allowed to describe actual use plainly. Categories such as `sex`, `gambling`, `morbid-curiosity`, and `hate-read` should not be hidden behind euphemism merely because they are awkward. They also must not be collapsed into one another: morbid curiosity about death or disturbing history is a different purpose from reading an argument one expects to dislike.

This is a personal taxonomy learned from one person's corpus. Fixture-derived categories are evidence about this browser's user, not a claim to a universal, gender-neutral, or general-purpose taxonomy.

## Classification is multilabel

Membership is represented as non-exclusive per-category scores or proposals rather than one leaf in a tree. Conceptually, each category gets its own inclusion surface over tab, resource, event, and task features. A simple baseline may use one affine score `s_c(x) = w_c dot phi(x) + b_c` and separately recorded inclusion threshold `tau_c` per category. It learns from positive, explicit-negative, and unlabeled evidence without treating every object outside the category as negative. Richer or ensembled classifiers can replace it without changing the contract. This does not assert that categories or scores are statistically independent.

The central semantic question is roughly: why is this here? Purpose categories such as repair, research, ordinary curiosity, morbid curiosity, applying, scheduling, transacting, reading, and acquiring are useful signals and memberships. They are not gating branches through which every object must descend.

An object may pass several inclusion thresholds at once. For a soft-margin SVM, slack variables measure labeled examples' margin violations; support vectors are the examples that determine the fitted separator. Normalized geometric distance, held-out errors, and disagreement among resampled planes are useful but different diagnostics. None is by itself a probability or full epistemic uncertainty.

Missing membership is normally unlabeled, not an automatic negative example. Explicit removal supplies negative evidence for that category only.

When one region contains a coherent recurring subcluster, IB may fit another affine separator to that region or to residual errors from an earlier separator. Several planes may be retained as an oblique tree, bagged vote, boosted ensemble, or independent overlapping category scorers. Members need not leave the broader category, and the broader category may remain useful. This is not necessarily CART and not a compulsory single hierarchy.

Classifier output is evidence for zero, one, or many memberships. High-recall proposal and retrieval policy should prefer a useful extra candidate to making material disappear because one supposedly exclusive classification won. A proposal does not silently become accepted durable membership.

## Do not conflate identity levels

Three distinct things may share the same URL string:

1. **Resource identity** — a browser-owned record carrying requested and resolved URLs plus any asserted canonical URL. Immutable fetched representations and their content hashes have separate identities; one resource may change over time, and unrelated resources may contain equal bytes.
2. **Tab identity** — a durable navigation thread. Two deliberately open tabs may refer to the same resource without being the same thread or tab-level intention.
3. **Visit or opening event** — an append-only occurrence with its own time, referrer, task, and order.

Resource bytes may be deduplicated. Tabs must not be collapsed merely because their current URLs match. Visits must remain distinct history events.

Category views may point at resources or tabs according to what the view means. Revisit and duplicate-tab events normally inform membership, ranking, promotion, and `_active` selection rather than being discarded. The target kind must be explicit so a category link never silently changes an event into a resource or a resource into a tab.

## Categories are overlapping filesystem views

Canonical objects exist outside category directories. Categories are derived views over those objects, preferably expressed with ordinary filesystem links where practical.

Conceptually:

```text
state/
  resources/
    01R.../
  tabs/
    01T.../
  events/
    visits.log

categories/
  algebraic-topology/
    tab-01T... -> ../../state/tabs/01T...

  algebraic-topology-1950s/
    tab-01T... -> ../../state/tabs/01T...

  Serre/
    tab-01T... -> ../../state/tabs/01T...
```

The same tab can therefore appear in `algebraic-topology`, `algebraic-topology-1950s`, `Serre`, and any other useful view without duplicating the underlying record.

These views need not form a strict directory hierarchy. Do not require:

```text
math/algebraic-topology/1950s/
```

when separate overlapping categories are more useful:

```text
algebraic-topology/
algebraic-topology-1950s/
algebraic-topology-1970s/
```

Topic, era, person, work, game, move, purpose, and other axes may overlap freely.

## Granularity follows actual interest

Category resolution should become finer where the user's browsing becomes dense.

A recurring cluster of roughly 5–10 relevant objects is a useful signal that a narrower category may deserve a name. This is a heuristic for proposing a view, not a fixed threshold or ontology rule.

For example, a general `critical-role` category may eventually be less useful than categories for a particular campaign, world, character, or storyline. Someone deeply interested in Go may want separate views for a player, a particular game, a joseki, and historical interpretations of that joseki. Someone deeply interested in mathematics need not prefix every useful category with `math/`.

Once a narrower name is unambiguous and useful to the user, redundant umbrella prefixes are optional. Model-inferred names, splits, and merges remain reversible proposals until accepted; a category the user explicitly creates is already an authoritative assertion.

## Active categories are a working set

Categories can remain durable without all of them being active in the current workbench.

One representation may use an underscore-prefixed control directory such as `_active` as a small set of pointers to category views currently in use:

```text
categories/
  _active/
    algebraic-topology-1950s -> ../algebraic-topology-1950s
    algebraic-topology-1970s -> ../algebraic-topology-1970s
    campaign-4               -> ../campaign-4
```

Removing a link from `_active` neither deletes nor freezes the category. It removes it from the present attention surface while leaving older interests searchable and recoverable. `_active` is not the renderer's hot-tab working set: activating a category must not wake every tab or fetch every resource in it.

## Derived, rebuildable organization

Category directories and active-view links are materialized projections. They should be cheap to rebuild, inspect, or replace. Category definitions, user-authored or accepted memberships and corrections, active-category choices, and proposal decisions are durable browser-owned metadata. A direct filesystem edit must be imported as an assertion or attention event before a later rebuild. Rebuilding a projection must not mean rerunning a model and losing human work.

Language models and classifiers may propose memberships, names, merges, or splits, but do not directly rewrite canonical browsing state or accepted human corrections. Proposed and accepted organization must remain distinguishable. See `docs/inference-and-learning.md` for provenance, ensembles, and correction events.

The browser core owns durable identities, events, acceptance policy, and category semantics. Grease and ordinary operating-system primitives are appropriate for filesystem-facing construction and rearrangement of the materialized views.

Search should answer both:

- which resources or tabs belong to this category?
- which categories point at this resource or tab?

The second direction matters for understanding and editing overlap even if the on-disk projection uses forward symlinks and derives reverse membership by scanning or indexing them.

## Evidence from the 491-row exercise

One provisional conversational classification pass over 491 real browsing rows deliberately forced a primary-purpose answer so that distinctions and candidate clusters would become visible. It was an analytical view, not a checked-in artifact, ground truth, or a requirement that IB store one primary category.

The early `reading` versus `non-reading` split was likewise a useful elicitation pass, not a mandatory first classifier or a gate that every future category must descend through.

The pass separated, among other things, book acquisition from book reading or retrieval, home repair from car repair, and visual or interactive mathematics from mathematics reading and history. It also suggested candidates for narrower views such as Brecht, Philip Ball, Galois theory, paper folding, Euclid/Kepler, Thurston/Teichmuller theory, Japanese woodworking, Go, and EPA 608.

A Brecht listing can still belong simultaneously to `book-shopping`, `Brecht`, and `theatre`; a mathematical book can belong to an acquisition view and several mathematical subjects. The exercise supports overlapping refinement precisely because the forced partition loses useful memberships.

If a reproducible labeled fixture is later derived from that pass, it should retain explicit corrections rather than plausible guesses from names alone: the fixture URL `crux.jp` (earlier transcribed as `cruz.jp`) was kids'-toy shopping, and Daniel Litt's “Problems I Like” was a broken mathematics site.

Two ingestion lessons are settled:

- Preserve repeated URLs and duplicate tabs as distinct events or intentions. Repetition is noisy evidence of salience rather than redundant noise or proof of intent; count, recency, spacing, task context, and simultaneous duplication are distinct derived signals. Preserve deliberate duplication, background opening, revisit, reload, redirect, restore, and importer-duplication causes when the source exposes them; otherwise preserve the event and mark its cause unknown. Never infer cause from URL equality alone. Repeated visits do not manufacture the breadth needed for a 5–10-object cluster.
- Classification may be partial by axis. An Anna's Archive `/md5/...` or download route may already support `book-retrieval` while title and mathematical or literary subject remain unresolved. `unknown` is not an all-or-nothing label.

Private account, authentication, messaging, password-reset, and token-bearing URLs belong outside public fixtures rather than inside ordinary categorization examples.

## Implementation status

The current `IB.History` and `IB.Index` slices preserve repeated URL rows and their input order. That normalized order is not yet a stable event identity across imports, merges, restarts, or sync. Category definitions, proposal records, accepted-membership storage, filesystem projections, reverse-membership inspection, and `_active` are not implemented on `main` yet. The current storage inspector also does not follow category symlinks. This note defines the boundary for that future work; its example directories are not a claim about the present schema.

## Commitment level

Settled boundaries:

- non-exclusive overlapping memberships;
- canonical identities separated from derived category views;
- resources, tabs, and visits are not one identity level;
- duplicate and revisit evidence is preserved;
- denser personal interests may acquire narrower views;
- model organization is proposed and reversible.

Current heuristics and baselines:

- roughly 5–10 relevant objects as a category-promotion signal;
- one affine inclusion scorer and separate threshold per category using positives, explicit negatives, and unlabeled material correctly;
- over-inclusion when the alternative is failed retrieval;
- `_active` as a filesystem-shaped working-set control.

Still open:

- the exact feature representation and classifier;
- category-specific thresholds;
- when category creation or activation requires explicit confirmation;
- whether a given materialized view points at resources, tabs, or both;
- the final filesystem grammar.
