# Adaptive tab categorization

IB should organize tabs by why they matter to the user, not primarily by the media type or site that happens to contain them.

A URL may be an article, book, video, product page, forum post, or PDF. Those are useful features for fetching and rendering, but they are usually not the user's central category. A wrench product page may belong to `car-repair`; a book may belong to `algebraic-topology`; a page about a disturbing historical event may belong to `morbid-curiosity`.

## Classification is not an ontology

Membership should be treated as a collection of independent scored decisions rather than one exclusive tree. Conceptually, each category gets its own decision surface over tab features. A simple linear classifier may literally be one separating hyperplane per category; richer classifiers can replace it without changing the contract.

The central semantic question is still roughly: why is this tab here? Purpose categories such as `car-repair`, `home-repair`, research, ordinary curiosity, morbid curiosity, applying, scheduling, or transacting are useful signals and memberships, but they are not gating branches through which every tab must descend.

A tab can land on the positive side of several category decisions at once. Being classified into one category must not exclude simultaneous membership in other useful categories. Scores near a boundary are evidence of uncertainty, not a demand to pick one winner.

CART-like behavior is useful at a different level: when one category becomes dense, IB may propose splitting off a narrower recurring cluster and give that new category its own decision surface. This is adaptive refinement, not a single hierarchy.

The classifier output is evidence for zero, one, or many memberships.

Prefer a useful extra membership to making material disappear from retrieval because one supposedly exclusive classification won.

## Categories are overlapping views

Canonical tab/history objects should exist once. Category directories are views over those objects, preferably expressed with ordinary filesystem links where practical.

Conceptually:

```text
state/
  tabs/
    01J.../
    01K.../

categories/
  algebraic-topology/
    01J... -> ../../state/tabs/01J...
    01K... -> ../../state/tabs/01K...

  algebraic-topology-1950s/
    01J... -> ../../state/tabs/01J...

  Serre/
    01J... -> ../../state/tabs/01J...
```

The same tab can therefore appear in `algebraic-topology`, `algebraic-topology-1950s`, `Serre`, and any other useful view without duplicating the underlying record.

These views need not form a strict directory hierarchy. Do not require structures such as:

```text
math/algebraic-topology/1950s/
```

when independent overlapping categories such as these are more useful:

```text
algebraic-topology/
algebraic-topology-1950s/
algebraic-topology-1970s/
```

Topic, era, person, work, game, move, purpose, and other axes may overlap freely.

## Granularity should follow actual interest

IB should allow category resolution to become finer where the user's browsing becomes dense.

A recurring cluster of roughly 5–10 tabs is a useful signal that a narrower category may deserve a name. This is a heuristic, not a threshold built into the ontology.

For example, a general `critical-role` category may eventually be less useful than categories for a particular campaign, world, character, or storyline. Likewise, someone deeply interested in Go may want separate views for a player, a particular game, a joseki, and historical interpretations of that joseki. Someone deeply interested in mathematics need not be forced to prefix every useful category with `math/`.

Once a narrower name is unambiguous and useful to the user, redundant umbrella prefixes should be optional rather than mandatory.

The system may propose new category names when a cluster becomes dense, but creation and naming should remain reversible and inspectable.

## Active categories are a working set

Categories can remain durable without all of them being active in the current workbench.

Use an underscore-prefixed control directory such as `_active` as a small set of pointers to category views currently in use:

```text
categories/
  _active/
    algebraic-topology-1950s -> ../algebraic-topology-1950s
    algebraic-topology-1970s -> ../algebraic-topology-1970s
    campaign-4               -> ../campaign-4
```

Removing a link from `_active` does not delete or freeze the category. It merely removes it from the current working set. This gives IB an inexpensive form of archival attention: old interests remain searchable and recoverable without crowding the present surface.

## Derived, rebuildable organization

Category directories and active-view links are derived organization over canonical browsing records. They should be cheap to rebuild, inspect, edit, or discard.

LLM output may propose memberships, names, merges, or splits, but should not silently rewrite canonical browsing history. Proposals should retain enough provenance to explain why a link exists when that matters.

The implementation should favor Grease and ordinary operating-system/file primitives for constructing, inspecting, and rearranging these views. The browser core owns the durable identities and classification policy; Grease is the natural workbench for filesystem-facing orchestration.

## Consequence for search and UI

The category surface is not a tree browser with one path per tab. It is a set of overlapping neighborhoods over the same durable corpus.

Search should therefore be able to answer both:

- which tabs belong to this category?
- which categories point at this tab?

The second direction matters for understanding and editing overlapping organization, even if the on-disk representation uses ordinary forward symlinks and derives reverse membership by scanning or indexing them.

## Observation from a 491-URL browsing batch

A hand classification of 491 real browsing rows is a useful scale check for this model. A primary-purpose partition was possible without collapsing the corpus into one giant `math` or `books` bucket: 115 rows were book shopping/acquisition, 115 were book library/download/preview, 54 were mathematics reading/reference/course material, 29 were visual or interactive mathematics, and 16 were mathematics history/classic-source material. The rest formed a long tail of repair, tools, building, software, music, literature, politics, family/admin, games, food, and other purposes.

That partition is useful as a diagnostic view, not as canonical truth. A Brecht listing can simultaneously belong to `book-shopping`, `Brecht`, and `theatre`; a mathematical book can belong to both an acquisition view and its mathematical subject. Dense recurring clusters such as Brecht, Philip Ball, Galois theory, paper folding, Euclid/Kepler, Thurston/Teichmuller theory, Japanese woodworking, Go, and EPA 608 already justify narrower overlapping views.

Two details matter for ingestion:

- preserve duplicate visits as distinct history rows even when their URL strings are identical. Repetition is evidence of salience rather than redundant noise; revisit count, recency, and spacing between revisits are useful derived signals for ranking, category promotion, and deciding what belongs in `_active`;
- do not label an opaque URL `unknown` merely because its subject metadata has not been resolved yet. An Anna's Archive `/md5/...` or download URL is already known to be book retrieval; subject/title enrichment can happen later.

Private account, authentication, messaging, password-reset, and token-bearing URLs should be excluded from public fixtures rather than treated as ordinary categorization examples. In this batch only one bare archive snapshot lacked enough context even for a useful primary-purpose guess.