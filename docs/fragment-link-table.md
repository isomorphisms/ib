# Fragment links, indexed strands, and reading observations

IB should be able to present a document as a bounded stream of fragments while Pensive records what the view actually exposed. The phone is one view adapter, not the definition of reading. The first implementation should work with ordinary files on Android; filesystem or OS changes can come later if the file representation demonstrates a useful primitive.

The main storage rule is deliberate redundancy:

> Index the same durable relationships several ways when doing so turns an important query into a RAM lookup, contiguous range read, or sequential walk.

Indexes are not canonical truth. They are rebuildable projections of durable fragment, link, strand, and observation records. Duplication is acceptable when it removes repeated random I/O from a hot path.

## Fragments are delivered by the view

Do not require an in-band sentinel character. A paragraph sign or other marker may be shown in a workbench to make chunk boundaries visible, but it is instrumentation, not document content.

A view consumes a moving window of fragments. A phone may prepare them in stages:

```text
current       painted
next          prepainted
next + 1      laid out
next + 2      fetched/decoded
farther       not resident
```

The window size is adapter- and memory-dependent. A laptop may keep much more resident; another view may not scroll at all.

A fragment may line up with a paragraph, viewport-sized chunk, source block, or another cut chosen by the adapter/chunker. Reading is device-dependent, so IB should not pretend that a phone screen and a laptop pane expose identical units.

Stable content coordinates still matter. A view-specific fragment should be able to identify the underlying source range it covers so Pensive can relate observations made by different views.

## View observations for Pensive

The view reports observations, not conclusions such as `read` or `understood`.

Useful observations include:

```text
fragment entered view
fragment left view
fragment passed
fragment returned
fragment activated or selected
visible duration
visible fraction, when available
view adapter / device class
```

Scroll position, scroll velocity, pointer position, and viewport geometry are optional adapter-specific evidence. A non-scrolling view must still be able to report useful exposure and navigation events.

Repeated returns are particularly useful evidence. Pensive may later infer that a repeatedly revisited range deserves a stronger index entry, prefetch priority, landmark, or workbench shortcut. That inference remains separate from the raw observation log.

## Fragment records

Persistent relationships must not depend on process virtual addresses. Raw C pointers die across restart or remapping.

A compact fragment record may contain hot fixed fields such as:

```text
fragment_id
source_id
source_start
source_end
previous_fragment
next_fragment
edge_start
edge_count
```

`previous_fragment` and `next_fragment` may be duplicated here even when the same relationship also appears in the general link table. They are hot enough to justify duplication.

Inside a particular mmap file, a derived index may use relative offsets for cheap pointer-like access. Durable records should retain stable fragment identities or otherwise survive remapping and rebuilding.

## Link table

The logical graph is a link table. One possible logical row is:

```text
source_fragment
link_kind
destination_fragment
rank
flags
metadata_reference
```

Examples of `link_kind` include document order, parent/child structure, citation, annotation, source, semantic association, task membership, and Pensive-created relationships.

The logical table does not imply that every query scans or probes a generic database table. Physical representations should be specialized for the queries IB actually performs.

## Multiply index the graph

At minimum maintain source-oriented and destination-oriented indexes.

A source-oriented adjacency index can look like compressed sparse row storage:

```text
fragment_directory
  fragment 184 -> edges[8301..8307]
  fragment 185 -> edges[8308..8310]

edges
  8301 -> next       185
  8302 -> parent     173
  8303 -> citation   9271
```

The small `fragment -> (start,count)` directory can remain resident in RAM. All outgoing links for one fragment are contiguous, so `links from 184` becomes one small lookup followed by one range access.

Maintain the reverse projection as well so `what points to 184?` does not scan the graph.

Likely useful indexes include:

```text
source_fragment -> contiguous outgoing edges
destination_fragment -> contiguous incoming edges
(source_fragment, link_kind) -> matching outgoing edge range
(destination_fragment, link_kind) -> matching incoming edge range
link_kind -> edge range or posting list
source coordinate -> fragment(s)
fragment -> strand memberships
strand -> ordered fragment ids
```

More indexes are acceptable when measurements show a recurring query. The semantic-system direction is to make important relationships cheap from several directions rather than treating one normalized representation as sacred.

The durable relationship set can remain simple and append-friendly while indexes are generated, compacted, replaced, and rebuilt independently.

## Strands

A strand is one chosen ordered journey through fragments. It is distinct from the full graph.

```text
graph  = all available relationships
strand = an ordered sequence selected for reading or work
```

A known document-order strand should not be reconstructed by repeatedly looking up `next` and seeking again.

Store a materialized strand as a contiguous sequence:

```text
strand 27
  184
  185
  186
  191
  205
  206
```

Then a view can request a window such as positions `500..563`, receive many fragment ids in one range read, and queue their text/layout/prepaint work without one graph lookup per hop.

An arbitrary Pensive graph walk may also be materialized into a temporary or durable strand when the user chooses to read it. Materialization is an optimization and a record of a chosen traversal; it does not erase the graph that produced it.

## Ordinary-file first layout

A first Android implementation can use ordinary files and mmap/range reads. A possible conceptual layout is:

```text
state/fragments/
  fragments.idx
  text.dat
  links.dat
  observations.log
  indexes/
    links-by-source.idx
    links-by-destination.idx
    links-by-source-kind.idx
    source-ranges.idx
    fragment-strands.idx
  strands/
    <strand-id>.idx
```

This grammar is not frozen. The important properties are:

- small directories/index roots can remain resident;
- neighboring records needed together are stored contiguously;
- distant rendered/layout state can be evicted;
- indexes can be rebuilt from durable records;
- a reader can fetch a useful window with a few range operations rather than many random seeks;
- the same storage can be queried without running a heavyweight database server.

Large text bodies are not necessarily the main RAM cost. Parsed structures, layout objects, decoded images, glyph runs, and rendered surfaces can dominate. Keep those bounded separately from durable fragment metadata.

## Shell query surface

The graph and strand representation should have small composable commands. Exact names remain open, but the operations should include the equivalent of:

```text
ib-fragment text 184
ib-link from 184
ib-link to 184
ib-link from 184 --kind citation
ib-strand window 27 500 64
ib-strand containing 184
```

Commands should support batch/range input and ordinary stdin/stdout composition so a program can traverse hundreds of relationships without launching one query per edge.

For example, a strand window command should be able to emit a block of fragment ids that another command consumes as one batch, rather than forcing:

```text
next(184)
next(185)
next(186)
...
```

The binary/index formats may be optimized, but inspection and debugging should remain possible through these tools.

## Relation to prefetch and prepaint

The strand gives the view a cheap ordered lookahead source. Preparation remains staged:

```text
fetch -> decode/parse -> layout -> prepaint -> display
```

The view can walk the strand ahead of the current position and decide independently how far to advance each stage. Android may keep only a narrow painted/prepainted window while retaining a larger cheap fragment-id window.

Pensive observations flow the other direction: the view records which fragments/ranges were actually exposed and for how long. Prefetch policy may use those observations later, but exposure telemetry is not itself a claim that the material was read.

## Filesystem and OS direction

This representation may eventually suggest filesystem or operating-system primitives for typed links, fragment addressing, graph indexes, or strand traversal. Do not require those changes for the first implementation.

First make the ordinary-file version fast on the current phone. If the implementation repeatedly recreates the same indexing, adjacency, range-fetch, or typed-link mechanisms, those repeated mechanisms become concrete candidates for lower-level support.

See also `storage-model.md`, `prefetch-and-reading.md`, `resource-constrained-rendering.md`, and `prepaint-display-contract.md`.
