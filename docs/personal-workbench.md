# Personal browser and task workbench

## Scope

IB's immediate target is a personal browser: a workbench over one person's durable browsing corpus, tasks, and habits. It is not presently a general-purpose compatibility browser and should not spend its effort reproducing every web input or every site's preferred interface.

The architecture must nevertheless support multiple first-class frontends over the same browser-owned state. A conventional page-oriented browser, a small phone surface, and a ChatGPT-like text-and-action workbench may coexist. The text-first workbench is **a frontend**, not **the frontend**, and no frontend becomes the owner of tabs, history, tasks, or saved material.

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

## One text-first frontend

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
- task completion rather than page reproduction as the success measure;
- durable task intent separated from disposable renderer and response caches;
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
