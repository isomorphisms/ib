# Filesystem views and presentation working sets

IB uses ordinary directories and symbolic links as inspectable control and organization surfaces. The names around the larger work area remain provisional; this contract does not depend on calling it a forge, workbench, or anything else.

## Keep three axes separate

Category membership, present attention, and renderer residency answer different questions.

- A category says why a resource or tab may matter. Categories overlap.
- `_active` says which category neighborhoods belong on the present work surface. Activating one category does not wake all of its members.
- `hot/` says which already-available presentation targets should be favored for immediate use. It is a requested working set, not proof that the bytes are currently resident under every memory-pressure condition.

Actual residency remains a measured runtime state. A later inspector can report requested-hot versus resident without making a symbolic link pretend to be RAM.

Removing a link from `hot/` is a demotion of attention, not deletion or freezing. The presentation can remain in the reading corpus, task state, categories, and search indexes and become hot again without reconstructing its identity. There is no separate `warm/` directory in this first slice because “warm” may mean several different facts—saved readable material, disposable fetched bytes, serialized renderer state, or actual cache residency—and those must not be collapsed into one link.

## Sketch

```text
browser-owned state/                  human-readable presentation root/
  tabs/01T-CAMPAIGN/                    aws-composite/
  resources/01R-RULES/                    view.md
                                           sources.tsv

views/
  retrieved-from-the-web/
    text/
    images/

  organizing-the-information/
    categories/
      critical-role/
        tab-01T-CAMPAIGN -> .../state/tabs/01T-CAMPAIGN
      campaign-4/
        tab-01T-CAMPAIGN -> .../state/tabs/01T-CAMPAIGN
    vector-spaces/
      potion-base-2m/
        embedding-model.txt
        pages/
          format.txt
          vectors-<generation>.txt

  _active/
    campaign-4 -> .../views/organizing-the-information/categories/campaign-4

  hot/
    aws-iam -> .../presentation-root/aws-composite
```

`retrieved-from-the-web` names source-derived text and image views without pretending that a fetched corporate page is the browser's sovereign output. `organizing-the-information` groups ways of looking across those and other browser objects: overlapping categories and one or more model-specific vector spaces. The repeated category link does not duplicate the tab. The hot entry does not need to point at that tab or at any corporation's preferred page. It may point at a source-backed text file, Markdown file, HTML file, or a presentation bundle containing a view, local images, and provenance. A synthesis made from twenty documentation pages is one legitimate hot presentation while its twenty source identities and edges remain intact.

Markdown is only a cheap current presentation format. It is not canonical browser state and does not constrain later renderers.

## The page is input, not sovereign output

Fetched HTML, PDFs, structured responses, screenshots, and saved pages are source material. IB may extract, combine, rank, or summarize them into the representation that answers the user's task fastest. The original representations and source references remain available; a derived presentation does not rewrite them and does not masquerade as an original page.

This supports the documentation case directly: acquire the relevant parts of a documentation set once, pass selected immutable representations through a cheap local producer, and pre-paint the resulting text, Markdown, HTML, or local-image bundle without opening twenty corporate page interfaces.

A hot local presentation should be paintable without a network round trip. The intended interaction is console-fast immediate content followed by richer replacement only when it adds something useful; this filesystem slice supplies the pointer handoff but does not claim the frontend or RAM loader yet.

## First executable slice

`lib/filesystem_views.grease` now owns the filesystem-facing primitives:

- initialize `retrieved-from-the-web/{text,images}`, `organizing-the-information/{categories,vector-spaces}`, `_active`, and `hot` under `views/`;
- add one tab or resource to any number of category directories;
- activate or deactivate a category independently of membership;
- list reverse category membership for a typed object link;
- publish or replace a hot presentation link;
- remove only symbolic links, never their targets;
- prune broken view links;
- reject traversal, missing targets, target-root escapes, reserved names, and collisions with ordinary files.

`bin/ib_views.grease` exposes the same operations as small shell commands. Materialized links use resolved targets and can be rebuilt when storage moves. A deterministic mock producer proves that a twenty-source composite bundle can become hot without depending on a language model or a renderer in the test.

This slice creates views only. It does not itself load hot targets into RAM, execute an embedding model in the phone-resident browsing loop, store canonical category assertions, import hand-edited links as assertion events, or rebuild all projections from durable Idriç records. The initial models are now pinned and exercised end to end by `experiments/embedding-models/`; moving the selected inference adapter behind the Android/runtime boundary remains separate work. The Grease layer must not fabricate those claims.
