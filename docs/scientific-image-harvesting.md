# Scientific image harvesting

## Source preference

For arXiv material, IB tries the HTML representation first and only falls back to PDF when HTML is unavailable or unusable.

For an identifier such as `2203.11355`, the ordered candidates are:

1. `https://arxiv.org/html/2203.11355`
2. `https://arxiv.org/pdf/2203.11355`

The preference is browser policy and therefore lives in `IB.ScientificMedia`. Grease performs the actual requests.

## HTML first pass

`bin/harvest_arxiv_images.grease` fetches the preferred representation. For HTML it walks `<img>` elements in document order and downloads each image in that same order.

Every download starts with an ordinal temporary identity. Naming then uses this evidence order:

1. nearest enclosing `<figcaption>` text;
2. useful `alt` text;
3. `temporary_image_0001`, `temporary_image_0002`, ... when neither gives a conceptual name.

Caption-derived names drop the mechanical `Figure 3.` / `Fig. 3:` prefix, retain a substantial amount of descriptive text, and normalize it to lower snake case. Duplicate conceptual names receive stable numeric suffixes rather than overwriting earlier images.

`images.tsv` records order, source URL, local file name, naming basis, caption, and alt text. `unresolved_images.tsv` records only images that still have temporary names.

## Second naming pass

If `IB_IMAGE_NAMER_COMMAND` is set, unresolved images are passed to that command after extraction. The command receives the complete source document and the unresolved manifest and writes:

```text
ORDER<TAB>CONCEPTUAL NAME
```

IB runs this pass through `nice -n 20` by default (`IB_IMAGE_NAMER_NICENESS` can override it). That deliberately treats semantic renaming as background-quality work rather than something allowed to interfere with foreground browsing.

The language model sees the whole HTML/PDF plus the list of extracted images, so it can use surrounding text and cross-figure context. It is not asked to name images that already have a useful caption or alt-derived name.

## PDF fallback and harvest-images-from-pdf

IB pins `isomorphisms/harvest-images-from-pdf` under `vendor/harvest-images-from-pdf`. That repository owns PDF parsing and figure/caption semantics; IB does not fork those data types or association rules into a second PDF parser.

The vendored repository does not yet expose an end-to-end extraction command. Until it does, the Grease adapter uses Poppler's `pdftohtml -xml` output only for mechanical image extraction and page-layout data. It converts Poppler's top-left rectangles to the bottom-left coordinate convention expected by `PDF.Figure`, then invokes a small compiled Idriç bridge that imports the vendored `PDF.Figure.associate_figure_caption` rule. Grease does not decide whether a text fragment is a caption.

When that Idriç rule associates a caption with an extracted image, the caption is written into the same caption field in `images.tsv` used by the HTML path and participates in the existing caption-first naming policy. Images without an identified caption remain temporary and can still go through the low-priority language-model naming pass.

This split is deliberate: PDF figure/caption semantics stay in Idriç; Poppler invocation, coordinate normalization, temporary files, and file movement stay in Grease. The vendored harvester remains narrow and unchanged.

## Scientific fixtures

The live test uses previously collected scientific arXiv identifiers:

- `2203.11355` — *Origami in N dimensions: How feed-forward networks manufacture linear separability*
- `1901.09021` — *Complexity of Linear Regions in Deep Networks*
- `1606.05336` — *On the Expressive Power of Deep Neural Networks*
- `2305.00241` — *When Deep Learning Meets Polyhedral Theory: A Survey*

The deterministic PDF fixture contains one embedded image and the nearby text `Figure 1. Tiny color blocks`; the test requires that the vendored Idriç association rule put that caption into `images.tsv` and that the existing naming path produce `tiny_color_blocks.png`.

The live test requires at least one collected paper to resolve through arXiv HTML and verifies that every HTML image is downloaded in order. PDF fallback is covered deterministically rather than depending on arXiv withholding HTML for a particular paper.
