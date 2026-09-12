# Personal browser and task workbench

## Scope

IB's immediate target is a personal browser: a workbench over one person's durable browsing corpus, tasks, and habits. It is not presently a general-purpose compatibility browser and should not spend its effort reproducing every web input or every site's preferred interface.

IB's first two first-class user frontends are:

1. a visual browser frontend that immediately pre-paints the cheapest useful source-backed representation, then progressively augments it or attaches a live renderer when needed; and
2. a ChatGPT-like, text-only-by-default task frontend that answers questions and offers direct actions over the same browsing corpus.

These are two projections of the same browser-owned tabs, tasks, history, organization, and source material. Neither frontend owns that state, and the architecture may support additional frontends later.

Renderer adapters and capability negotiation remain first-class architecture. That makes a broader compatibility browser possible without making broad compatibility IB's current product requirement. An unsupported or irrelevant input may be marked unknown or discarded after a safe failure. MIME type, source, and renderer requirements remain useful signals even when IB does not support the input itself.

## The unit of success is the task

A page view is often only an intermediate artifact. IB should optimize the thing the user is trying to accomplish:

- learn enough from a documentation set to answer and act;
- find an image and send it to another person;
- find a purchasable copy at the delivered price and continue to checkout;
- recover a fact, file, control, or status from a large application;
- resume an investigation after the renderer processes and machine have gone away.

A tab remains a durable navigation thread, but a task may span many tabs, resources, searches, and actions. The workbench therefore needs a browser-owned task record rather than inferring the whole task from whichever page is currently visible.

A task record may retain:

- the user's request or query;
- root tabs, searches, and resources;
- discovered source relationships;
- selected facts and candidate results;
- actions offered and actions actually taken;
- background work still pending;
- source and inference provenance;
- enough state to resume after process death or a later reboot.

This is a conceptual record, not a frozen schema.

## Visual pre-paint frontend

The visual frontend should paint useful extracted material as soon as it exists. A page may begin as title, headings, text, operative links, forms, tables, and already-fetched images, then gain richer styling or a live renderer only when the task needs them. Pre-paint is a user-facing frontend behavior, not merely a cache artifact or developer diagnostic.

The current focus region is also task evidence. Safe operative links at or near its top are strong short-lived candidates for bounded background acquisition; changing focus reprioritizes work without manufacturing tabs.

## Text-only task frontend

The ChatGPT-like frontend should keep a searchable text history of requests, evidence, results, and actions. Its default output is the smallest useful combination for the current task, not a faithful miniature of a corporate webpage.

When a direct structured request, ordinary HTTP fetch, small parser, cached extraction, or provider-neutral API can answer the task, the workbench should prefer that path. A heavyweight page renderer is an escalation path when the task genuinely requires page interaction or visual context.

For example, a purchase result may be only:

```text
used copy — $8.40 delivered — arrives Friday — Go
```

The source, timestamp, offer qualifications, and alternate results remain inspectable without occupying the primary surface.

The foreground should remain small and serialized: one active result or action, at most one immediate secondary context, and a short candidate list. Background investigations may fan out across sources, prefetch material, and build summaries without turning every candidate into a live renderer tab.

## Acceptance story: programming documentation

Opening five or fifty documentation tabs should not leave fifty isolated pages that each require another round of manual link opening.

Given a documentation task, IB should:

1. preserve the root tabs and the links discovered from their operative content;
2. recognize when several roots point at the same resource;
3. retain every source edge even when the resource bytes can be fetched once;
4. continue bounded background acquisition and extraction;
5. make the accumulated material searchable and available to later questions;
6. preserve the work frontier across shutdown, so returning a week later resumes the investigation rather than forgetting it.

The depth and priority policy remain open. The invariant is that the durable task remembers *what remains to be learned*. Disposable cached bytes are not the only record of that intention.

A deterministic fixture should keep the root document usable while a bounded set of early safe links from its operative content becomes warm. Selecting a warmed target should add no predictable foreground network wait and should not require a renderer session for every warmed document.

## Acceptance story: GitLab or a comparable software page

Given a project, README, documentation, issue, or source page with roughly seventeen plausible operative links, IB should not require the user to open seventeen background tabs and manually wade back and forth through them. It records those links as task candidates, warms a bounded safe subset, extracts and summarizes useful material in the background, and lets the user ask, “just tell me what I need to know,” against the accumulated source-backed corpus.

A discovered candidate is not a tab. A navigation thread or renderer is created only when interaction actually requires one. The same behavior applies to a batch of arXiv links: the question motivating the browsing may be asked before every paper has been opened and manually read.

## Direct assistant context

IB should expose a provider-neutral, user-authorized task-context interface to the person's configured assistant. The user should not have to copy URLs, open every candidate tab, or manually restate what they are browsing. A context bundle may include the current resource and navigation thread, visible and selected source spans, the current task question, roots and frontier, extracted views, retained summaries, and source provenance.

For an actively watched YouTube video or comparable media resource, the bundle should include available title, channel, chapters, captions or transcript, and current playback position without requiring an automatic full-media download. For a batch of arXiv links, it should include the task-linked papers, their extracted text, and current summaries.

The automatic low-cost summarizer and the user's preferred interactive assistant are separate configurable roles, although one model may fill both. A remote provider receives only the explicitly scoped context bundle; credentials, session storage, and unrelated private browsing state remain excluded. Context assembly and export are visible, inspectable actions governed by user policy, not a hidden exfiltration path.

## Acceptance story: find and send an image

For a query such as finding a young Larry Wall in a loud 1970s shirt to show a friend, success is not merely displaying image-search results. The requested terminal action is sending a suitable image.

The result surface should therefore expose usable image candidates with direct `Share`, `Copy`, or `Save` actions and source provenance. Loading the search result page, then a preview, then the source page, then another menu is avoidable work unless one of those stages contributes information the user needs.

## Performance contract

Measure task progress, not a site's load event.

- Aim to expose an actionable result in under two seconds.
- Aim to finish a routine, already-understood action path in under five seconds.
- Count user actions, renderer starts, foreground network waits, and forced context switches.
- Treat an added click, added blocking fetch, or added renderer escalation as a regression unless it buys necessary information or control.
- Exercise these stories on weak and old hardware, including an old Kindle-class device and the target Android Go phone. A fast developer machine is not the acceptance environment.

These are benchmark targets, not claims that an arbitrary remote service can always meet them. Cached pre-paint, background acquisition, and direct actions are how IB controls its portion of the delay.

## Commitment level

Settled boundaries:

- personal-browser priorities first;
- multiple frontends over shared browser-owned state;
- visual pre-paint and text-only task interaction as the first two frontends;
- task completion rather than page reproduction as the success measure;
- durable task intent separated from disposable renderer and response caches;
- a scoped provider-neutral bridge from the active browsing task to the configured assistant;
- safe failure for unsupported inputs rather than general compatibility work.

Current heuristics and benchmarks:

- a very small serialized foreground;
- actionable output under two seconds and routine completion under five seconds;
- direct structured acquisition before a heavyweight renderer when it supplies the needed result.

Still open:

- the exact task-record grammar;
- the link-frontier and budget policy for each kind of investigation;
- the final frontend toolkit and interaction vocabulary;
- which conventional renderers are attached first.
