# ib

An experimental personal browser and task-workbench substrate built around durable browsing state rather than renderer-owned tabs.

IB's immediate target is one person's real browsing corpus and workflows, not general-purpose web compatibility. It optimizes the task behind navigation: learning a documentation set, recovering a fact, finding and sharing an image, comparing delivered prices, or resuming an investigation after the live browser processes are gone.

The substrate supports multiple frontends over the same browser-owned state. A conventional page surface, a small phone frontend, a text-and-action workbench, and developer inspectors may coexist. Renderers, acquisition adapters, extractors, and models remain replaceable; none owns tabs, history, tasks, or accepted organization.

The browser core owns resource, tab, event, and task identity; sleeping and waking; snapshots; organization; indexes; inference acceptance; and renderer selection. Only roughly 3–10 renderer working sets should normally be resident even when the known corpus reaches 10,000 resources.

## Design notes

- `docs/architecture.md` — ownership and replaceable-service boundaries
- `docs/personal-workbench.md` — personal scope, task frontend, user stories, and latency targets
- `docs/prefetch-and-reading.md` — durable investigation frontiers, disposable fetches, and `~/reading`
- `docs/tab-categorization.md` — overlapping personal categories and adaptive refinement
- `docs/inference-and-learning.md` — local-model proposals, validation, ensembles, and correction events
- `docs/storage-model.md` — identity levels and canonical, proposed, and derived state
- `docs/developer-workbench.md` — fixture and memory-pressure harness

## Implementation languages

IB is implemented in **Idriç**. Browser-owned state, policy, and invariants belong in `.idric` source under `src/`.

**Grease** is the shell and operating-system language for orchestration. HTTP fetching, temporary directories, file movement, invoking compilers or parsers, cache maintenance, and low-priority model batch passes belong in `.grease` programs rather than being reimplemented as Idriç application logic.

Python and Ithon are not IB implementation layers. A disposable comparison may exist outside the runtime, but the browser core, storage and index policy, inspector model, and phone-facing application logic must not depend on them.

Android or other native code is a narrow platform boundary for facilities Idriç and Grease cannot yet reach directly: NativeActivity, EGL, renderers, kernel-enforced filesystem operations, clipboard and share handoff, and similar FFI edges. Those adapters do not own browser state.

The current Idriç core covers ordered history values, rebuildable indexes, storage classification and read policy, and the renderer-independent inspector model. Scientific-media work adds HTML-first arXiv harvesting, ordered image downloads, caption and alternate-text naming, PDF fallback, and a low-priority second naming pass. The task, category, and generic inference records documented above remain design boundaries rather than claims of completed implementation.
