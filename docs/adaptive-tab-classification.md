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

These examples imply that URL/domain/content features are evidence for classification, not the definition of the category.

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
