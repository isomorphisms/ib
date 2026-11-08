# Scholarly source recovery — staged IB implementation

## User story
The goal is that a restricted Springer academic page triggers low-priority,
nonblocking, lawful public-copy discovery and source-grounded model summaries.
A publisher book landing page (for example Yoshida, *Hypergeometric Functions,
My Love*, 1997, DOI 10.1007/978-3-322-90166-8) is a bibliography,
not evidence that the full book has been read.

## What this branch actually implements
- Checked Idric publisher access classification with refusal to treat an HTML
  200 or an advertised book title as an admitted complete work.
- Append-only per-task investigation root and per-event files in existing
  browser state, separate from disposable prefetch cache.
- Four bounded public query descriptions: Wayback CDX, Internet Archive
  catalog, OpenAlex and archive.today. Their outputs are not yet fetched or
  verified; archive.today's lookup contract remains unqualified.
- Exact bibliographic identity, source digest/readback, MIME, completion and
  trusted coverage requirements for candidate availability. Abstract-only,
  metadata-only and complete-text claims are distinct.
- A small charcoal native reader notice for restricted publisher pages.
  Existing history, selected document, source identity and scroll are unchanged.
- Model-neutral, source/hash/prompt/revision-scoped summary proposals,
  with immutable proposal files, never canonical source mutation.
- CI tests for the pure semantics, refusal cases and metadata persistence.

## Known incomplete gates: not accepted or released
1. Wire restricted/404 discovery to navigation without waiting for its worker.
   Provide a distinct durable background job/generation; do not retarget the
   existing foreground Arena/PFD owner or create a second canonical store.
2. Acquire provider queries and candidate copies through maintained ICU and
   authorized Shizuku/Binder ingress, not an independent app-side curl path.
3. Parse source-specific provider replies with bounded provenance, validate
   work identity, resolve redirects, and independently reopen exact original
   bytes. PDF-to-text/full-work coverage needs its own acceptance.
4. Present a selectable verified alternative-source list and actual read action
   in the native workbench, with preserved scroll/selection and no fake link.
5. Invoke an optional model only with selected content/spans, validate returned
   references and label all summaries full-text, abstract-only or metadata-only.
6. Test real API29/34 ART, independent ai-ci source/candidate refusals, and
   physical C67/TAB_P10. No physical acceptance, release or merge is claimed.

See issues #11, #83, #102 and #104 and native reader PRs #167–#170.
