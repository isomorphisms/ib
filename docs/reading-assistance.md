# Pensieve reading assistance

This is a future contract around the 0.2 Cauldron/Pensieve architecture. It is separate from [`reading-feedback.md`](reading-feedback.md): reading feedback records what the View observed, while this document describes how persistent material can help a person read, return, ask questions, and enter an overwhelming corpus.

## Do not optimize for finishing long reads

IB should not treat `finished a long document` as the primary reading objective.

Long continuous reading is one legitimate mode, especially for fiction, but it is not the only useful mode and it is not evidence by itself that the material was understood. Text can simply wash over someone. A person may instead read intermittently, stop to ask a question, reread a passage, inspect one section of a paper, compare several papers, or leave an item partially read for weeks.

The Pensieve should therefore support reading as an ongoing relationship with material rather than a pipeline from `unread` to `finished`.

## Fiction: memory support without replacing the book

For a long novel, the useful question may be small and immediate:

- Who is this character again?
- Where did this person first appear?
- How is this person related to another character?
- What place or event is being referred to here?
- Have I seen this name before, or is it new?

The Pensieve should be able to answer those questions quickly from the text already accumulated for the book. The answer should point back to source passages or locations rather than inventing an independent encyclopedia of the novel.

Reading progress matters here for another reason: spoilers. By default a reading-companion query should be answerable from material at or before the person's known reading frontier. Later text must not be used merely because the Pensieve has already ingested the complete book. A user can explicitly ask for unrestricted/full-book information when desired.

A synthetic acceptance fixture can model a large novel with recurring people, places, aliases, and relationships. The test should prove that a question about a returning character is answered from earlier source-backed occurrences and does not leak a fact introduced after the current reading frontier.

## Scientific papers: reduce the cost of getting started

The first scientific-paper problem is often not `how do I finish this paper?` It is `I have too many papers I intend to read and cannot even establish a useful starting point.`

The same problem occurs with a pile of GitHub repositories, documentation sets, issues, or source trees.

The Pensieve should be able to create a cheap first working view before the person has opened every item individually. For an arXiv corpus this can include, for example:

- a one- or two-sentence source-backed orientation;
- the paper's stated problem or question;
- principal objects, methods, and named results;
- section-level orientation;
- relationships to other already-known papers;
- unresolved terms or prerequisites worth looking up;
- figures or equations likely to matter to the current task;
- a short explanation of why this paper may or may not deserve foreground attention now.

These are aids for choosing where to spend attention. They are not claims that the paper has been read, understood, or correctly summarized.

The same pattern can apply to a repository: purpose, important directories, entry points, current work, dependencies, tests, and likely files relevant to the present question can be surfaced before a person manually explores the whole tree.

## Derived reading units: working terms `strand` and `fragment`

The final vocabulary is deliberately not frozen here.

A language-model or other analysis hook may produce small derived reading units. `strand` and `fragment` are working terms for this idea. A unit might summarize one section, connect several related source regions, answer a question, identify a relationship, or provide a compact route into a larger item or corpus.

Whatever name survives, a generated unit must remain visibly derived. It should retain at least:

- the Pensieve item or items it was derived from;
- exact source/representation hashes or revisions;
- source locations sufficient to inspect the basis for the unit;
- generator/model identity when applicable;
- prompt, policy, or transformation revision when applicable;
- creation time;
- whether generation was complete or interrupted;
- staleness when any supporting source changes.

A generated unit must not overwrite source material, human annotations, or reading observations. It can be deleted and rebuilt.

A unit may become useful enough to pin, edit, accept into an organization, or keep in a reading rotation, but those later human actions are separate durable facts from the original machine proposal.

## Language-model hooks are part of the architecture

The existing `after-distill.d` process boundary is intentionally suitable for language models as well as ordinary indexes.

Conceptually:

```text
Cauldron source
      |
      v
   distill
      |
      v
  Pensieve item
      |
      +--> exact-text / vector / other indexes
      |
      +--> language-model or other analysis hooks
                |
                v
         derived guides / fragments / strands
```

A model hook may read both the distilled Pensieve item and its Cauldron provenance, generate summaries or relationships, and append derived artifacts. It does not become the owner of the Pensieve.

The model boundary should remain replaceable. Re-running the same source through a different model, prompt, or policy may produce another derived result without destroying the previous one.

## Corpus-level assistance

The most useful generated object may span many items rather than summarize one item at a time.

For a large reading queue, IB should be able to answer questions such as:

- What are these papers mostly about?
- Which three appear closest to the problem I am working on?
- Which papers seem to depend on concepts absent from the rest of the corpus?
- Which papers repeat the same basic result or background?
- What can I read first to make the rest easier?
- Which repositories appear to implement the same idea differently?

A corpus-level guide must preserve per-source provenance. It is task-specific derived state, not canonical truth and not a replacement for the underlying items.

## Connection to View feedback

The two directions form a loop without collapsing their semantics:

```text
                 derived guides
Pensieve ------------------------------> View
   ^                                      |
   |                                      |
   `---------- reading observations <-----'
```

The Pensieve may use prior reading observations to improve assistance: do not repeatedly explain a term the person has explicitly marked as familiar, resume near a partially read section, or prefer an earlier-source explanation when answering a fiction question.

But a model-generated summary being displayed does not mean it was read. A source being summarized does not mean the source was read. A correct answer to `who is this character?` does not prove comprehension of the novel.

## Minimum deterministic acceptance cases

1. **Novel recall without spoilers.** A returning character appears before and after the fake reading frontier. A question about the character is answered from earlier occurrences only unless full-book scope is explicitly requested.
2. **Partial reading remains useful.** A person can ask source-backed questions about a half-read item without changing it to `finished` or `understood`.
3. **Paper overload gets a starting view.** Given a local fixture corpus of many papers, derived orientations can be built before each paper has been opened in a View.
4. **Repository overload gets the same treatment.** A repository fixture can produce a compact source-backed orientation without pretending every file was manually inspected by the person.
5. **Generated units remain derived.** Deleting all generated guides leaves Cauldron bytes, Pensieve source text, human annotations, and reading observations intact.
6. **Model replacement is safe.** Two model/policy versions may generate different guides from the same immutable source representation; both retain provenance and neither rewrites the source.
7. **Changed source becomes stale.** Updating a supporting representation marks old generated units stale rather than silently treating them as current.
8. **Corpus synthesis keeps provenance.** A multi-item guide can trace each substantive claim or relationship back to its supporting Pensieve items/source locations.
9. **Reading telemetry is not inferred from generation.** Generating or displaying a summary creates no synthetic `read` event for either the summary or its source.
10. **Question-driven reading is first-class.** A user may repeatedly ask focused questions and revisit passages without any requirement to complete the document linearly.
