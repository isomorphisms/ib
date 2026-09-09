# Reader-support system: intake, prefetch, and derived support structures

This note deliberately records goals before IB has decided which operations belong to the Pensieve itself, which belong to deterministic indexes, and which belong to replaceable language-model hooks.

The architecture should preserve that uncertainty rather than prematurely assign every useful behavior to one component.

## Reader support is larger than a document viewer

The intended system should gradually become useful around a person's actual reading life, not only around URLs opened in a browser.

A person may have:

- physical books on shelves;
- ebooks or PDFs;
- novels currently being read;
- scientific papers queued for later;
- GitHub repositories and documentation sets under investigation;
- poems or other short works deliberately kept in rotation;
- prior notes, questions, and remembered connections.

IB should be able to use these as a reader-support environment while keeping source material, human facts, and machine-derived guesses distinct.

## Bookshelf photographs as intake

A photograph of a bookshelf is a legitimate source object.

The original photograph belongs with source/provenance material. It should be retained unchanged if the user chooses to keep it.

OCR, vision, or language-model processing may then propose:

- candidate titles;
- authors;
- editions when visible;
- likely duplicates or volumes in a series;
- links to already-known Pensieve material;
- possible topics or relationships among the books.

Those recognitions are derived artifacts. A blurry spine or model guess must not silently become the canonical fact that the person owns or has read a particular book.

A useful model may also form provisional higher-level observations such as `this collection contains a great deal of numerical analysis and geometry`. Those may help later ranking or conversation, but they remain model-derived interpretations with provenance and should be replaceable or discardable.

The aim is to let the system get to know the reader through material the reader has deliberately supplied, without confusing inference with fact.

## Do not freeze Pensieve versus model ownership yet

Some support structures can be built deterministically. Others probably benefit from a language model. Some may eventually have both implementations.

Examples:

- exact occurrence index: deterministic;
- chapter/section boundaries: often deterministic;
- names and candidate entities: deterministic, model-assisted, or hybrid;
- aliases and character identity: often model-assisted;
- relationship descriptions: likely derived/model-assisted;
- source-backed summaries: derived;
- vector or hyperplane indexes: derived and rebuildable;
- reading observations: durable human interaction evidence, not model output;
- explicit user annotations or corrections: durable human facts.

The architectural invariant is more important than the initial implementation choice: every support structure must say what source it came from, whether it is observed or inferred, and whether it can be rebuilt.

## Prefetch means preparing useful support, not merely fetching bytes

For a long work, prefetch can extend beyond acquiring the next network resource.

If a person is reading *War and Peace*, background preparation may build reader-support structures before the person explicitly asks for them. Useful work may include:

- chapter and section map;
- exact character-name occurrence index;
- candidate aliases, titles, patronymics, and alternate forms;
- first and previous appearances relative to the reading frontier;
- places and recurring events;
- source-backed candidate relationships among characters;
- local passages likely to answer `who is this person again?`;
- compact support fragments generated for recurring entities.

This is the same architectural idea as prefetching figures for an arXiv paper or walking a bounded GitHub dependency/documentation graph: spend bounded background work now so the likely next question is cheap later.

The work must remain bounded by storage, CPU, model, and power budgets. Prefetch should improve responsiveness without requiring a permanently live model or renderer.

## Spoiler-aware book support

A complete book may be locally available while the reader is only halfway through it.

Support structures can be built over the whole source when useful, but reader-facing answers should default to the known reading frontier. A query such as `who is this guy?` should prefer evidence from earlier appearances and avoid revealing a later relationship or event merely because the index already knows it.

This suggests separating:

- what the background system is allowed to index;
- what an answer is allowed to reveal at the current reading frontier.

A model hook that has seen the whole book must obey the same frontier rule as a deterministic index.

## Fragments and support structures are plural

There probably will not be one universal `Fragment` type.

Different generated/support objects may have different purposes, for example:

- character card;
- previous-appearance fragment;
- relationship fragment;
- chapter orientation;
- paper summary;
- prerequisite note;
- repository map;
- question-answer fragment;
- corpus-level strand connecting several sources;
- reminder or rereading cue.

The final vocabulary and type hierarchy remain open. The important requirement is that these objects remain source-linked, inspectable, and distinguishable from original material and human annotations.

## Display is deliberately unresolved

The semantic capability should not depend on a final UI decision.

`Who is this guy again?` might eventually appear as:

- a transient inline card;
- a side panel;
- a small overlay;
- a text-only task response;
- a search result;
- a generated strand inserted near the current reading position;
- some interface not yet designed.

The first contract should therefore be renderer-neutral: given the current item, reading frontier, and a question or selected entity, return source-backed support plus provenance. Presentation can evolve separately.

## Language-model hooks

The existing hook architecture should allow a model to participate in bookshelf intake, book indexing, summaries, relationship extraction, question answering, and corpus synthesis.

The model is not required to own any of these concepts permanently. A later deterministic or specialized implementation may replace a model-generated structure without changing the source corpus or human reading history.

Useful model output should retain:

- source item(s);
- source revision/hash;
- source locations when available;
- model/generator identity;
- prompt or policy revision when applicable;
- creation time;
- staleness;
- confidence or uncertainty when the operation is genuinely uncertain.

Human correction should outrank a later regenerated guess unless the user explicitly revises that correction.

## Minimum deterministic acceptance cases

1. **Bookshelf source survives.** A synthetic shelf image remains unchanged while recognition artifacts can be deleted and rebuilt.
2. **Recognition is not ownership fact.** A deliberately ambiguous spine may yield two candidate titles; neither becomes a confirmed owned/read book without human confirmation or stronger evidence.
3. **Model interpretation stays derived.** A generated observation about the reader's interests can be removed without modifying the source collection or human annotations.
4. **Book prefetch prepares the next question.** A long synthetic novel is indexed in bounded background work before a `who is this character?` query occurs.
5. **Previous appearances are cheap.** After prefetch, a returning-character query can retrieve earlier source-backed occurrences without scanning the entire book synchronously.
6. **No spoiler leakage.** Whole-book indexes may exist, but reader-facing answers at chapter N reveal no fact supported only after chapter N unless unrestricted scope is explicitly selected.
7. **Different support structures coexist.** Character, chapter, relationship, and question-answer artifacts can refer to the same source without being collapsed into one generic truth record.
8. **Model replacement is safe.** Rebuilding character relationships with another model changes only derived artifacts, not source text, reading observations, or confirmed human corrections.
9. **UI independence.** The same support response can be consumed by two different Views without changing its source/provenance semantics.
10. **Bounded background work.** Increasing book size or corpus size does not imply an unbounded number of live renderers or permanently resident model sessions.
