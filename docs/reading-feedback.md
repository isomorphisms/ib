# View feedback, reading evidence, and revisiting

This is a future IB contract around the 0.2 Cauldron/Pensieve architecture. It does not make a renderer part of the current 0.2 acceptance boundary.

## Design goal

A browser should help answer a question that ordinary history does not answer:

> Did the person actually engage with this material enough to plausibly have read it, and should it be brought back later?

`opened once` is not `read`.

A page can be opened accidentally, abandoned immediately, skimmed too quickly to read, read carefully, revisited several times, or deliberately kept in rotation after it has already been read. These are different facts and must not collapse into one visited/read boolean.

Passive viewport data can provide evidence of reading. It cannot prove comprehension. Comprehension must remain unknown unless there is stronger evidence such as an explicit human assertion or a task-specific interaction that actually bears on comprehension.

## Ownership

The temporary View observes presentation and interaction. It sends reading observations back to persistent IB state.

```text
Cauldron
   | source bytes + provenance
   v
Pensieve
   | distilled item
   ^
   | reading observations
View
```

Reading state belongs with the Pensieve item, because it concerns a person's interaction with distilled material. The observation must retain enough source/representation identity to trace back to the Cauldron bytes that were actually shown.

The Cauldron remains intake and source evidence. It does not own the judgment that something was read.

A View is disposable. Destroying a View must not destroy reading observations already accepted into persistent state.

## Do not store a single `read = true`

Keep the evidence that supports later policy decisions. At minimum a reading session needs stable identities for:

- Pensieve item;
- exact source or rendered-representation revision/hash;
- session;
- timestamps from a monotonic/fake-clock-testable clock;
- which text regions or blocks were actually visible;
- how long visible regions remained visible while the View was active;
- scroll/navigation movement;
- foreground/background or otherwise-active state;
- explicit human actions such as `mark read`, `keep in rotation`, or `show again` when those exist.

The exact on-disk grammar is still open. The semantic rule is not: raw observations are durable evidence; derived reading classifications are rebuildable and policy-versioned.

## What the View should report

The View should report observations, not conclusions.

Useful observations include:

- a text region entered the viewport;
- a text region left the viewport;
- the visible range changed because of scrolling or navigation;
- the View became active or inactive;
- the person returned to an earlier region;
- the person reached the end of the item;
- the session ended;
- an explicit reread/reminder preference was selected.

The persistent layer can combine these later.

Do not count background time as reading time. Do not treat a rapid sweep from top to bottom as equivalent to dwelling on the text. Do not require continuous scrolling: a person may stop moving precisely because they are reading.

Text size and layout matter. Reading coverage should be expressed against semantic text regions/blocks or stable text offsets, not merely screen pixels, so the same material can be compared across different window sizes and renderers.

## Reading evidence, not fake certainty

A derived policy may calculate evidence such as:

- fraction of the item's text that was actually exposed;
- dwell time per exposed amount of text;
- whether exposure was plausibly sequential rather than one high-speed jump;
- repeated exposure to difficult or earlier passages;
- number of distinct reading sessions;
- explicit human confirmation.

Those inputs can support states such as `unseen`, `partly exposed`, `likely skimmed`, or `likely read`, but the names and thresholds are policy, not source facts.

There must be no passive `comprehended = true` inference from viewport or scrolling telemetry. If IB later wants a comprehension signal, it must preserve what produced that signal rather than laundering it into certainty.

## Re-reading is first-class

Completing one plausible reading must not remove an item from future consideration.

The persistent model needs a revisit policy separate from reading evidence. Examples include:

- show again tomorrow;
- keep in daily rotation;
- show again after a chosen interval;
- spaced revisiting;
- no reminder;
- manually pinned recurring reading.

A poem is the obvious counterexample to the conventional browser model. Reading it once can be a reason to return, not a reason to suppress it forever.

Repeated readings should append sessions. They must not overwrite the first reading or collapse all visits into one timestamp.

## Poetry Foundation examples

Two concrete source examples for acceptance/design discussions are:

- Robert Frost, **Mending Wall** — `https://www.poetryfoundation.org/poems/44266/mending-wall`
- Jalal al-Din Rumi, **“Where did the handsome beloved go?”** — `https://www.poetryfoundation.org/poetrymagazine/poems/144612/where-did-the-handsome-beloved-go`

These are examples of source identity and revisit behavior. Deterministic tests should not depend on the live Poetry Foundation site and need not copy copyrighted poem text. Local synthetic fixtures can carry the source URL/title plus representative block lengths and scrolling traces.

For either example, a completed reading session may coexist with `keep in rotation = daily`. The next day's resurfacing is therefore correct behavior, not evidence that the previous reading record failed.

## Minimum deterministic acceptance cases

The fixture harness should prove at least:

1. **Open is not read.** Open an item and close it without meaningful exposure. A visit/open event exists, but the reading policy cannot classify the item as plausibly read.
2. **Fast scroll is not careful reading.** Sweep through a long item faster than the configured policy regards as plausible. Coverage may be high while reading evidence remains weak.
3. **Stationary reading counts.** Keep successive text regions visible for plausible intervals with little or no scrolling. Reading evidence increases even though movement is small.
4. **Background time does not count.** Leave a region visible, background the View, advance the fake clock, and return. Inactive time contributes nothing to active dwell.
5. **Partial reading survives.** Read the first half, destroy the View, recreate it, and preserve the earlier observations without pretending the second half was read.
6. **Rereading is preserved.** Complete an item twice in two sessions. Both sessions remain visible to policy and inspection.
7. **Revisit policy is independent.** Mark a completely read poetry item for daily rotation. The next fake-clock day schedules it again without deleting or weakening the completed-reading evidence.
8. **Representation identity matters.** If source/rendered text changes materially, retain old reading evidence against the old revision/hash instead of silently claiming the new text was read.
9. **Shared bytes do not erase interaction history.** Two Views may display the same underlying representation while retaining distinct reading sessions.
10. **Derived policy is rebuildable.** Delete a derived reading score/classification, replay the durable observations under the same policy version, and obtain the same result.

## Privacy and locality

This feedback loop should work entirely locally. Reading telemetry is unusually personal and should not need to leave the device merely to make the Pensieve useful.

The normal IB rule applies: preserve source facts and human actions; keep derived judgments replaceable.
