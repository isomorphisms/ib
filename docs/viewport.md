# 0.2 viewport content boundary

The viewport is a transient presentation over material supplied from the Pensieve. It is not a PDF renderer and it does not read the Cauldron directly.

The first content boundary is deliberately small:

```text
PreparedImageRef

Figure
  image   : PreparedImageRef
  caption : optional text

ViewportBlock
  ViewportText text
  ViewportFigure figure
```

`PreparedImageRef` is an opaque reference to an already-local image chosen by the Pensieve-facing producer. The viewport does not need to know whether storage policy colocates the image bytes with the Pensieve entry or resolves the reference through retained provenance.

A figure caption is optional. Source provenance stays in the Pensieve and does not become viewport state merely because the figure is visible.

Document-list metadata such as a title or short description is separate from the block stream. It can be added to the left-side document/navigation model without changing text and figure rendering.

## Rendering

The Android Material 3 mapping can stay mechanical:

```text
ViewportText
  -> Material Text

ViewportFigure
  -> Compose Image
     preserve aspect ratio
     ContentScale.Fit
     never crop the scientific figure

  -> optional caption underneath
     Material typography bodySmall
```

A bare image plus caption is the default. A `Surface` or `OutlinedCard` may later be used when a figure needs an explicit interactive container, but card styling is not part of the content contract.

Tap-to-enlarge, pan, and pinch zoom are later viewport behaviors. They do not require another content type.

## Ownership boundary

The producer/Pensieve side owns:

- choosing text and figures to expose;
- resolving prepared image references;
- captions and provenance;
- indexing and search relationships.

The viewport owns:

- distinguishing text from figures;
- painting the supplied content;
- preserving figure aspect ratio;
- local presentation interactions.

The viewport does not:

- fetch images;
- parse PDFs;
- extract figures or captions;
- understand arXiv identifiers;
- index figures;
- treat the current article mock as a permanent input shape.

This keeps the present arXiv fixture replaceable by substantially different Pensieve material without changing the viewport's basic text-or-figure interface.
