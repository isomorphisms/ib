# Deliberately stupid pre-renderers

This directory is an executable design probe for the browser-core/renderer boundary. It is not a choice of implementation language and it is not intended to grow into the real renderer.

`prerender.py` accepts already-fetched HTML and emits two disposable forms of first paint:

- `preview.txt`: title, source URL/label, and visible-ish text, capped at 8 KiB by default.
- `screen-01.pbm` through `screen-03.pbm`: at most three 180×320 one-bit rough visual previews. Each default PBM is about 7.2 KiB.

The bitmap preview is deliberately not a screenshot of a fully rendered web page. It ignores CSS, JavaScript, images, fonts, layout, forms, media, and subresources. It rasterizes extracted text with a tiny built-in 5×7 font. Its purpose is to test whether ib can show something recognizable immediately while the real renderer is absent, sleeping, loading, crashed, or being replaced.

## Try it

```sh
python3 prototype/prerender.py page.html \
  --source https://example.org/ \
  --out /tmp/ib-preview
```

The input may also be piped on stdin by using `-` as the input path.

## Tests

```sh
python3 prototype/test_prerender.py
```

The tests check that scripts/styles are excluded, text output remains bounded, visual previews stay below 8 KiB per default screen, and visual output is capped at three screens.

## Architectural rule being tested

A pre-renderer consumes browser-owned fetched/snapshot material and produces a disposable preview. Neither `preview.txt` nor the PBM files are canonical browsing state. Deleting all of them must not delete a tab, a history entry, a snapshot, or user organization.
