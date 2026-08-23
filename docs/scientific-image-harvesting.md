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

IB pins `isomorphisms/harvest-images-from-pdf` under `vendor/harvest-images-from-pdf`, currently at the figure/caption association line. That repository owns PDF parsing and figure/caption semantics; IB does not fork those data types into a second PDF parser.

The current Grease bridge uses the operating-system `pdfimages` utility for the raw byte-extraction step because the PDF repository does not yet expose an end-to-end command-line extractor. Those PDF images enter IB as ordered temporary names and therefore naturally go through the second naming pass. When the repository grows its extraction entry point, the Grease bridge should invoke it instead without changing browser policy or stored manifests.

This split is deliberate: PDF semantics stay in Idriç; process execution and file movement stay in Grease.

## Scientific fixtures

The live test uses previously collected scientific arXiv identifiers:

- `2203.11355` — *Origami in N dimensions: How feed-forward networks manufacture linear separability*
- `1901.09021` — *Complexity of Linear Regions in Deep Networks*
- `1606.05336` — *On the Expressive Power of Deep Neural Networks*
- `2305.00241` — *When Deep Learning Meets Polyhedral Theory: A Survey*

The test requires at least one of these to resolve through arXiv HTML and verifies that every HTML image is downloaded in order. PDF fallback is also exercised.
