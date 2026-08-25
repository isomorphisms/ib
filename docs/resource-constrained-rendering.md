# Resource-constrained information rendering

IB should treat expensive full-page rendering as one possible way to obtain useful information, not as the prerequisite for seeing a page.

This is not primarily a desktop-versus-mobile distinction and it is not ordinary reader mode. A phone may only need a small amount of useful state from a very large web application: headings, links, forms, tables, status values, prices, dates, controls, or a small set of images. The browser should be able to expose that information without first paying the full CPU, RAM, network, and latency cost of booting the site's preferred application renderer.

## Goal

Make the cheapest useful representation visible first, then spend more resources only when the cheaper representation is insufficient.

The user should be able to request a resource-constrained profile explicitly, and IB should eventually be able to select it automatically for slow or resource-heavy sites.

## Escalation ladder

A navigation can climb through increasingly expensive representations:

```text
cached extracted representation
        ↓
HTML-only fetch
        ↓
HTML + useful CSS/images
        ↓
limited JavaScript / selected network requests
        ↓
full renderer
```

IB should stop climbing as soon as the current representation contains what the user needs. A full renderer remains available as an explicit escape hatch.

## Pre-paint behavior

As soon as IB has useful material, it should paint it. It should not wait for the site's application shell to finish booting.

Useful early material includes:

- title and canonical URL
- headings and ordinary text
- links and navigation targets
- forms and form actions
- tables and definition-list-like data
- buttons and labeled controls
- important images or thumbnails
- obvious status, price, date, count, and identifier fields
- structured metadata already present in HTML

The first view may be incomplete. It should be replaced or augmented as better information arrives rather than holding the screen blank until a heavyweight renderer reports completion.

### arXiv integration demonstration

`bin/prepaint_arxiv_progressively.grease` joins the scientific-media fetch boundary to the Idriç information renderer without introducing a renderer process. It uses the HTML-first arXiv source selection from the scientific-media layer, then gives the information core only progressive 4, 16, and 32 KiB prefixes of the selected HTML document.

For arXiv paper `2203.11355`, the live regression requires the title, paper heading, and abstract heading to become available across those bounded inputs while script text remains absent. The script reports the complete document size, the exact bytes admitted at each stage, and zero heavyweight-renderer invocations. A compact deterministic arXiv-shaped fixture exercises the same boundary without depending on the network.

## Information renderer

The lightweight path should produce a renderer-neutral extracted representation rather than pretending to be a complete DOM snapshot.

A first representation can contain fields such as:

```text
title
requested_url
resolved_url
canonical_url
fetch_time
headings
text_blocks
links
tables
forms
controls
images
structured_metadata
observed_data_responses
```

The exact schema can evolve. The important boundary is that this representation describes useful page information and can be painted without owning the browser's durable tab/history state.

## Network-response extraction

Heavy web applications often fetch useful data before spending substantial additional work turning it into interface chrome. IB should be able to observe those responses and, when they are intelligible and safe to expose, render useful data directly.

For example, if a web console eventually requests JSON containing a list of projects, credentials, jobs, or status records, IB should be able to turn that response into a plain table instead of requiring the site's JavaScript framework to construct the final interface first.

This should remain an information-extraction path, not a general promise to reverse-engineer every private application protocol. Authentication, permissions, and origin policy still apply.

## Resource policy

The resource-constrained profile should be able to suppress or defer work that is not needed for the current information view, including:

- animations
- decorative web fonts
- analytics and telemetry
- ads and tracking resources
- autoplay media
- large decorative images
- video
- expensive canvas/WebGL work
- background application code unrelated to the visible information

JavaScript should be escalated rather than assumed. If HTML already contains the needed information, JavaScript need not run merely because the site normally expects it.

When JavaScript is necessary, IB should prefer the smallest amount of execution or network activity needed to recover the missing information before escalating to a full renderer.

## Relationship to renderer capabilities

This fits the existing renderer-adapter model. Resource-constrained rendering should be expressed through capabilities and policy rather than hard-coding renderer brands.

Likely capabilities include or refine the existing ideas around:

```text
html
css
images
javascript
reader-text
structured-extraction
network-response-observation
information-view
full-dom-snapshot
```

A text-oriented or information renderer may satisfy a navigation without supporting the capabilities required for a full application view. The core can hot-swap to a heavier renderer without changing tab identity or history identity.

## Cache boundary

The extracted representation is valuable as a fast pre-paint cache, but cached extraction must remain disposable.

Clearing cache may remove:

- extracted information views
- response bodies retained only for pre-paint
- thumbnails and reduced images
- renderer-specific recovery material
- derived extraction indexes

It must not remove durable tabs, history, labels, collections, or other canonical browser state.

A later visit should be able to show a cached information view immediately while IB refreshes it or decides whether a heavier renderer is necessary.

## Phone-first behavior

On a resource-constrained phone, the default question should be: what is the smallest representation that lets the user continue?

A large developer console, store, dashboard, or administration site may be perfectly usable as a small collection of plain tables, links, status fields, and forms. IB should not force the phone to reproduce the site's desktop application architecture when that architecture contributes little to the user's immediate task.

This policy is deliberately different from accepting a site's own "mobile" rendering. The site may still ship a very large mobile JavaScript application. IB owns the decision about how much work to perform.

## Initial tests

A first implementation should be testable without depending on a particular heavyweight site.

Fixture cases should cover:

1. Initial HTML contains a useful table: IB paints the table without JavaScript.
2. Initial HTML contains headings, links, and a form: all are usable before full rendering.
3. Useful JSON arrives after a small scripted request: IB can expose it as a plain information view.
4. Decorative fonts, analytics, video, and large images are present: the constrained profile does not require them before useful paint.
5. The lightweight representation is insufficient: the same tab escalates to a fuller renderer without losing browser-owned history or view identity.
6. A cached extracted representation exists: it can pre-paint immediately and is independently clearable.
7. Cache is cleared: durable tab/history state remains intact and the information view can be regenerated.

## Non-goals for the first slice

- perfect visual reproduction of arbitrary sites
- preserving a JavaScript heap across renderer swaps
- automatically understanding every application protocol
- replacing a full renderer for workflows that genuinely require one
- treating every page as an article

The first useful slice is much smaller: fetch cheaply, extract obvious useful information, paint it immediately, cache it disposably, and escalate only when necessary.
