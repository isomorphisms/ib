# Adaptive tab classification

IB should classify tabs by the user's reason for keeping them, not primarily by page format, domain, MIME type, or generic web taxonomy.

The category system is personal and adaptive. It should learn from corrections rather than force every tab into a fixed ontology.

## Classification is closer to a decision tree than a flat label set

A useful mental model is a CART-like recursive partition of the user's tab universe:

1. Start with broad motivational categories.
2. Split a category when it becomes too heterogeneous or too large to scan usefully.
3. Keep a category coarse when its members remain meaningfully interchangeable, even if it contains many tabs.
4. Permit parent/child categories rather than requiring a single fixed level of granularity.
5. Permit `unknown` instead of inventing a bad classification.

Category size is therefore a soft regularizer, not a hard maximum. A category around 15--20 tabs may suggest a useful split if clear substructure exists. A coherent category may contain 50 tabs without needing a split. Conversely, a small category may deserve subdivision when the distinction changes what the user would do next.

For example, `Critical Role` can begin as one category. If it grows beyond roughly 15--20 active tabs and contains obvious sub-intents, it could split into children such as episodes, characters, campaign reference, or other naturally emerging groups. The children should be created because they improve retrieval, not merely because a counter crossed a threshold.

## User intent outranks page type

The same site or nominal content type can belong to different categories depending on why the tab exists.

Examples learned from classification corrections:

- Amazon and Temu product pages are `shopping`.
- AbeBooks can be `reading` when the purchase is subordinate to acquiring a book to read.
- Banking, benefits, food stamps, and similar resources belong with important money/income administration, not shopping.
- Job applications may split into `white-collar` and `blue-collar` when that distinction is useful to the user.
- Open Syllabus belongs to `Black Ball` because that project is the reason for keeping it, despite the site's academic subject matter.
- Tillers International belongs to `sex` when the actual reason for keeping the tab is researching a crush, despite the site's nominal subject matter.
- History podcasts belong to `history`; podcast is only the medium.
- Sex should be a first-class category when it is a recurring user intent.
- `Critical Role` deserves its own category once it has enough activity to cease being merely part of generic pop culture.
- Idiocracy can remain `pop culture`.
- Music being listened to should be distinct from writing or research about music.
- Documentaries should be distinct from generic movies/video when that difference predicts later retrieval.
- Cars, electronics, and software contribution work should not be collapsed into one generic DIY/technical bucket.
- Haaretz is news/reading rather than money merely because the encountered page is a subscription offer.
- Kids-related material should be grouped by the family task it supports, even when the individual pages are PDFs, calendars, sign-up pages, government pages, or payment pages.
- A Benn Peifert post can be `long-form reading / self-education`; being hosted on X does not make it social-media ephemera or pop culture.
- Jmail, Knockrup, death-related sites, execution details, assassination material, Nazi correspondence, antisemitic imagery, and similar material can belong to `morbid curiosity` when the reason for retaining them is deliberate inspection of disturbing, repellent, dangerous, or taboo material.
- A documentary such as *The Decent One* can likewise have `morbid curiosity` as its primary retrieval category when the subject matter and reason for retaining it are inspection of Nazism; `documentary` or `history` can remain secondary descriptors.
- `Problems I Like` and a Jacobian-counterexample visualization can form a bounded `intellectual fun / short reading` class: interesting enough to explore for pleasure, but meaningfully distinct from long-form reading that competes for substantial scheduled time.

These examples imply that URL/domain/content features are evidence for classification, not the definition of the category.

## Identified content does not imply identified intent

IB may be able to determine exactly what a site is while still not knowing why the user opened it. That distinction should be preserved rather than papered over.

For example, if a site is known to sell or curate books but the user no longer remembers why the tab was opened, the correct motivational category may still be `unknown`. Content classification can be retained as evidence, but it should not be promoted to a user-intent label without support.

## Primary retrieval category and secondary motives

A tab may have more than one true motive. Classification should therefore distinguish the category most useful for later retrieval from secondary descriptive motives rather than pretending that every tab has exactly one semantic meaning.

For example, `morbid curiosity` can coexist with `self-education`, `history`, `documentary`, or another subject label. Someone may be examining disturbing material to understand it, to confront something frightening or repellent, or simply because dark material can attract attention. The primary category should answer where the user would later look for the tab; secondary labels can preserve the other motive.

`Morbid curiosity` is intentionally different from `hate-read`. A hate-read is more naturally something like an opinion piece consumed despite expecting to dislike or disagree with it. Morbid curiosity instead covers disturbing subject matter itself: death, violence, atrocity, taboo, evil, grotesque material, or unsettling historical evidence.

## Non-moralizing organization

IB's classifier should not treat a category name as a judgment about the user. Categories such as `sex` and `morbid curiosity` are ordinary descriptions of browsing motives and should be handled as neutrally as `cars`, `history`, or `kids`.

The system should not hide, euphemize, or downgrade these categories merely because they concern sensitive or disturbing material. The purpose is retrieval and organization, not moral inference.

## Desired objective

A useful classification should balance at least four things:

- motivational coherence: tabs in a category answer roughly the same future need;
- retrieval value: opening the category should substantially narrow the search for the wanted tab;
- manageable size: categories should not become needlessly difficult to scan;
- low fragmentation: do not create tiny distinctions that add hierarchy without helping retrieval.

This suggests a split criterion analogous to tree impurity reduction, with a penalty for both oversized heterogeneous nodes and unnecessary fragmentation. Cardinality can influence the split decision without determining it.

## Learn from corrections

User reclassification is valuable training data. IB should retain enough provenance to distinguish:

- machine-suggested category;
- user-confirmed category;
- user-corrected category;
- confidence or competing candidate categories.

The derived classifier/index may be rebuilt. The underlying tab/history record should remain canonical and reversible.

Repeated corrections should alter future classification boundaries. In particular, IB should learn personal distinctions that a generic taxonomy would never infer correctly from the URL alone.

## Hierarchy should emerge from use

Do not require the user to design the category tree in advance.

A category can begin broad and later acquire children as the corpus grows. A child can disappear or merge again if it ceases to improve retrieval. The relevant question is not "what is the universally correct taxonomy?" but "what partition of the current tab universe best predicts how this user will look for these tabs later?"
