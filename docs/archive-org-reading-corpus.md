# archive.org reading corpus and paper pre-rendering

## Goal

archive.org links should be treated as unusually valuable reading material in IB.

When a user considers, opens, queues, or revisits an archive.org paper, IB should try to acquire a durable, provenance-preserving local representation suitable for:

- very fast first paint
- offline rereading
- figure-first browsing
- full-text search
- later personal-LLM retrieval
- export into a personal training corpus

The important distinction is between the **full stored corpus object** and a **cheap preview**. A preview may be clipped aggressively. The body stored for a paper should not be clipped merely because first paint is cheap.

This is browser-owned behavior and should ultimately be implemented in Idriç. Helper binaries may be used at the PDF/native boundary, but IB should not grow a Python archive crawler as an implementation layer.

## Recognizing Internet Archive material

At minimum recognize:

- `https://archive.org/details/<identifier>`
- `https://archive.org/download/<identifier>/...`
- direct item-file hosts such as `ia*.us.archive.org/.../items/<identifier>/...`

Normalize these to an Archive item identifier when possible, while retaining the exact URL the user visited.

For item pages, fetch item metadata early from:

`https://archive.org/metadata/<identifier>`

The metadata response includes item metadata and a file list, so IB can choose a useful representation without scraping the archive.org chrome.

## Representation preference

The archive item may contain several versions of the same work. Prefer the representation that gives the cheapest faithful reading result.

Initial order:

1. **Meaningful HTML body**, when the item actually supplies HTML for the work.
2. **Archive-derived structured/plain text**, when a scan already has OCR derivatives such as DjVu XML / DjVu text.
3. **Text-bearing PDF**, keeping the PDF and extracting its text layer.
4. **Scanned PDF/page images**, using existing Archive page-image/OCR derivatives when available.
5. OCR only when no usable text already exists.

Do not confuse the Archive details-page HTML with HTML of the paper itself. The details page is metadata/navigation chrome; it is not the preferred corpus body.

If several PDFs or text files exist, use metadata (`source`, `format`, filename, size) plus conservative naming rules to select candidates, and keep the choice in provenance so it can be corrected later.

Respect Archive access flags. Public downloadable files are eligible for local storage. Restricted/private/borrow-only material should not be bypassed by the ingestion path.

## HTML-first pre-render

If a clean HTML representation exists, use it aggressively:

- retain the original HTML body
- derive normalized `body.txt`
- extract title/authors/headings/figure captions
- collect referenced local/public image resources
- build the tiny first-paint representation immediately

For a paper, a useful first paint is not only the first paragraph. Figures should appear as early as possible because they are often the fastest way to decide whether the paper is worth reading.

## PDF path

The PDF path should be deliberately cheap before it becomes clever.

### Full text

First ask whether the PDF already has a text layer. If it does, use a lightweight PDF text extractor (for an initial probe, Poppler's `pdftotext` is sufficient) and store the complete normalized text separately from the small preview.

Do not OCR a PDF whose text can already be extracted correctly.

The eventual IB-specific PDF reader can replace helper programs later without changing the corpus/storage contract.

### Figures first

Start figure extraction before waiting for every later paper-processing step.

Initial strategy:

1. enumerate embedded image objects (`pdfimages -list` is a useful reference behavior)
2. extract embedded raster images losslessly when possible
3. scan extracted text for `Figure`, `Fig.`, and caption-like lines and retain page associations
4. associate obvious images with nearby captions
5. when a figure is vector/composite and therefore not an embedded image object, render the candidate page or candidate region to a bitmap (`pdftocairo`, `mutool draw`, or an eventual IB PDF raster path)
6. preserve page number and extraction method for each candidate

Writing our own tiny image-object scanner may eventually be worthwhile, especially if the only goal is to walk PDF objects and pull image streams without rendering. It should be a replacement for a proven narrow helper, not a prerequisite for the first working paper pipeline.

For scanned books/papers, Archive page-image derivatives may already be more useful than trying to discover images inside the PDF wrapper.

## Suggested durable object

A paper snapshot can extend the existing content-addressed snapshot model:

```text
state/
  snapshots/
    <content-hash>/
      source.txt
      archive-item.txt
      metadata.json
      provenance.json
      body.html          # if available
      body.txt           # complete normalized text
      pages.json         # optional page/offset map
      figures.json
      figures/
        figure-0001.*
        figure-0002.*
      preview/
        preview.txt
        ...
```

`body.txt` is corpus material. `preview/preview.txt` is disposable first-paint material.

The snapshot may retain either the original public source file or a content hash/reference to it depending on storage policy. Large original PDFs should not be duplicated merely because several tabs point to the same paper.

## Provenance

For later LLM use, every normalized body/figure needs enough provenance to reconstruct where it came from.

Keep at least:

- exact visited URL
- normalized Archive item identifier
- item title/creator/date when present
- selected source filename
- Archive file `format` and `source`
- fetch time
- source content hash
- normalized body content hash
- extraction method/version
- page boundaries or offsets when known
- figure source page/object and caption when known

The personal model should be able to distinguish "I saw this in paper X" from "this sentence appeared in some unattributed training blob."

## The `considered` hook

A useful LLM hook happens earlier than "finished reading."

If a paper enters a real browsing intention — for example the user opens it, queues it to read, or otherwise marks it as considered — IB can record an event such as:

```text
2026-08-23T10:48:00-04:00 considered archive.org <item-id> <history-entry-id>
```

That event makes the item eligible for corpus acquisition under the user's local storage/bandwidth policy.

This matters because "I thought this paper was worth looking at" is itself useful personal context even if the paper is not read immediately.

## LLM boundary

Do not couple browsing directly to irreversible model training.

Instead expose two cheap products:

1. **Immediate retrieval/index product**
   - title/creator/URL/item id as soon as metadata arrives
   - complete normalized body when acquired
   - page/section offsets
   - figures + captions
   - considered/opened/visited timestamps

2. **Training export product**
   - a reproducible manifest of corpus objects marked eligible for personal training
   - content hashes and provenance
   - normalized text plus optional figure-caption pairs
   - deduplication across repeated visits

This lets a personal LLM "know about" a considered paper immediately through retrieval while still allowing later LoRA/fine-tuning experiments to consume the same corpus deliberately.

A domain-level policy can make archive.org a high-priority acquisition source because it is disproportionately likely to contain long-form reading material.

## First-paint priority for papers

For archive papers, the cheap paint should aim for:

1. title + authors/date
2. first useful figure(s), with caption when available
3. abstract or opening body text
4. headings/table of contents when cheaply available
5. continuation text

This is intentionally different from a generic website preview where textual reading order may dominate.

The full body is acquired/stored for search and LLM use even when only a tiny prefix is painted initially.

## Storage pressure

The browser should distinguish durable reading corpus from disposable renderer cache.

Good candidates for durable storage:

- normalized full text
- provenance/metadata
- figure/caption pairs
- source hashes
- a source PDF when policy/storage budget permits

Good candidates for eviction/rebuild:

- rasterized page previews
- thumbnail sizes
- transient extraction scratch files
- renderer-specific caches

This keeps `Clear cache` compatible with preserving the user's reading corpus.

## Deterministic fixture set

The first implementation should use fixed item/file metadata fixtures rather than relying on the live Archive during tests.

Cover at least:

1. item with real HTML body + images
2. item with Archive-derived full text / DjVu text
3. text-bearing PDF
4. scanned PDF with no text layer
5. PDF with embedded raster figure
6. PDF whose important figure is vector/composite and needs page rasterization
7. item with multiple PDF candidates
8. item with metadata only / no publicly downloadable useful body
9. repeated visits to the same body: one corpus object, multiple history/provenance events
10. `considered` item acquired before it is opened again

## Small implementation slices

Keep the work separable:

1. Archive URL -> item-id recognition in Idriç
2. metadata response model + file classification
3. pure representation preference function
4. corpus snapshot/provenance schema
5. `considered` event and LLM-index/export hook
6. HTML full-body extraction
7. PDF text helper boundary
8. raster figure extraction
9. vector/composite figure fallback
10. replace helpers with smaller native/Idriç-owned readers only where it buys simplicity or speed

The target is not a general digital-library application. It is: **when IB encounters a paper the user cares about, make the paper cheap to paint, cheap to reread, and already organized as personal knowledge.**
