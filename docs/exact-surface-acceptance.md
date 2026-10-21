# Exact mathematical-paper surface acceptance (IB #160)

The first supported equation is the local HTML/MathML source

`x^4 - x*y^3 - z^4 + z = 0`

in real coordinates `x, y, z` in the explicitly declared affine chart
`W=1`. As a projective equation one can homogenize it to
`X^4 - X*Y^3 - Z^4 + Z*W^3 = 0`, but **this check does not visualize the
whole projective quartic**. It selects a bounded real affine surface, cut by
SURFER's unit-sphere clipping and a fixed orthographic camera.

## Actual integration

1. The existing `bin/harvest_arxiv_images.grease` fetches HTML first, preserving
   `document.html`, `source.tsv`, and the old `images.tsv` asset manifest.
   The new extractor does not replace or rewrite any of these.
2. `bin/accept_scientific_equations.grease DIR real W=1` reads harvested
   HTML and records MathML nodes with their raw alttext or TeX annotation,
   arXiv ID, effective source URL, HTML SHA-256, source-manifest SHA-256,
   and MathML ordinal. It rejects conflicting MathML sources. Text appearing
   in an HTML `<img alt=...>` is not accepted mathematical source.
3. A host-built C bridge compiles against the **real pinned** SURFER PR #33
   `native/android/formula.c` and ray tracer. It checks the four collected
   monomials exactly:

   ```text
   +1:4,0,0    -1:1,3,0    -1:0,0,4    +1:0,0,1
   ```

   It executes a 32×32 orthographic host ray-trace (height 2.4; yaw 0.25,
   pitch 0.25; zoom 1; clipped to the unit sphere), reports foreground
   pixel count and FNV-64 of ARGB pixels, and **does not label them a
   qualified still/movie**.
4. `IB.ExactSurface` Idriç types determine whether the verified terms,
   real domain, and explicit W=1 affine chart select `surfer-real-implicit`.
   Unknown chart, complex domain, or changed signed monomials select
   `unsupported`.
5. The `equations.tsv` record is append-only in intent for this one
   harvest: rerunning identical inputs compares bytes; changed interpretation
   or evidence cannot overwrite it. Original paper and original downloaded
   images remain available if any enhancement fails.

No C parser is being relabeled as the future typed Idriç equation parser;
the C implementation is the current bounded SURFER parser, not a general
LaTeX/MathML-to-polynomial interpreter.

## Promotion gates

Every entry currently says `NOT_VERIFIED` for media and
`BLOCKED_NO_EXACT_RESTORE` for the interactive application link.

- A successful C parse and host ray trace does **not** attest to a
  reproducible finished still or rotation, its rendering engine revision,
  complete settings, GPU equivalence, or mathematical certification.
- SURFER #33 has an unfinished Android formula-editor smoke and no accepted
  intent/deep-link path restoring this exact equation, parameters and camera.
  A launch into a preset surface is *not* a working equation link.
- A generated graphic is not accepted from arbitrary image metadata or a
  plausible AI image. The exact numerical engine must provide independently
  checked source-/settings-bound media evidence.
- PDF image extraction is preserved, but unverified PDF mathematical
  extraction yields `PDF_MATH_NOT_SUPPORTED` and no enhancement.
- An accepted input domain/chart is explicit caller context; this initial
  slice does not claim to infer it from an arbitrary paper.

The fake arXiv fixture ID `2401.00001` is **local test data**, not an
assertion about the real arXiv paper with that number.

## Reproduction and receipts

The IB `scientific-media` GitHub job loads a fixed SURFER SHA and runs
`sh bin/ci_browser_foundation.grease exercise-exact-math`. That command
builds the actual native parser/ray engine and Idriç policy and tests
the existing harvester with fake HTTP input, changed signs and exponents,
unsupported sin, conflicting MathML annotations, wrong domain, missing chart,
and unrelated images.

The independent AICI oracle in `isomorphisms/ai-ci`
`scientific-equation/verify.sh` separately checks the extracted source
against the receipt, fixed sparse tuple, and independent values at (2,3,4):
`F=-290` and `∇F=(5,-54,-255)`. It also kills forged-media,
forged-link, changed-signature, and document-hash mutants.

The registered execution envelope lives at
`isomorphisms/flexible-pipes#239` and checks pinned IB/SURFER/AICI
revisions through ordered command stages with retained receipts.
The three PRs remain independent and should not be merged on an owner-only
host test. A pipeline receipt is **not** protected controller admission,
finished-media certification, Android installation, or C67 acceptance.
