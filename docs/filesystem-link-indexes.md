# Filesystem link indexes

The fragment graph should be usable through ordinary files and directories as well as compact binary indexes. The filesystem representation is not merely a debug dump: directory names, filenames, and symlinks provide another set of indexes that shell programs can traverse directly.

A basic bidirectional projection is:

```text
~/from/231/...
~/to/231/...
```

`from/231` answers "what does fragment 231 point to?" and `to/231` answers "what points to fragment 231?" without scanning the complete link table.

## Prefer symlinks for the filesystem projection

An empty file whose name is another fragment id can represent an edge, but a symlink carries useful extra structure: the directory entry names the relationship while the target identifies the canonical fragment object.

For example:

```text
~/fragment/231
~/fragment/804

~/from/231/804 -> ../../fragment/804
~/to/804/231   -> ../../fragment/231
```

The forward and reverse directories deliberately duplicate one logical edge. They are indexes optimized for opposite queries.

Relative symlinks are preferable when practical because moving the containing corpus does not require rewriting every absolute path.

The durable graph must still have stable fragment identities. A symlink is one materialized projection of that identity, not a process pointer.

## Directories and filenames are another indexing layer

Do not reserve the directory hierarchy merely for `from/<id>` and `to/<id>`. Directory structure can classify the same edges several ways.

For example:

```text
~/from/231/
  next/
    232 -> ../../../fragment/232
  citation/
    804 -> ../../../fragment/804
  annotation/
    991 -> ../../../fragment/991

~/to/804/
  citation/
    231 -> ../../../fragment/231
```

That permits ordinary filesystem queries for both endpoint and link type.

Additional rebuildable projections might include:

```text
~/by-link-kind/citation/from/231/804
~/by-link-kind/citation/to/804/231

~/by-source/book-17/paragraph/0042/231
~/by-document-order/book-17/0000042/231

~/by-strand/27/0000500/231
~/by-fragment/231/strand/27/0000500

~/by-tag/conditioning/231
~/by-task/42/231
```

The exact hierarchy is intentionally not fixed. One purpose of the semantic operating-system experiment is to permit the same object to appear in many useful directory indexes without pretending that there is one privileged tree.

Filenames can carry sortable fields where that is useful. A zero-padded ordinal such as `0000500` makes ordinary directory order represent strand order. A filename may also encode a compact edge id, rank, source coordinate, or other value when doing so makes a common shell query simpler.

Avoid stuffing all metadata into filenames merely because it is possible. Use names for fields that are useful as lookup or ordering keys; keep richer metadata in the link record or a sidecar record.

## Logical graph versus projections

The system can therefore have several simultaneous representations of the same relationship:

```text
canonical / append-friendly link record
        |
        +-> from/ filesystem index
        +-> to/ filesystem index
        +-> link-kind filesystem indexes
        +-> strand directories
        +-> compact source adjacency index
        +-> compact destination adjacency index
        `-> mmap/range-read indexes used by the reader
```

This duplication is deliberate. Indexes should be cheap to rebuild and disposable independently of the durable relationship record.

The filesystem projections serve shell traversal, inspection, composition, and semantic organization. The compact binary indexes serve hot paths where resolving thousands of pathname components or performing one filesystem operation per edge would be unnecessarily expensive.

## Strand construction

Filesystem indexes can help construct a strand without requiring one random disk lookup for every graph hop. A shell program can enumerate a `from/<fragment>/<kind>/` directory or another precomputed projection in batches, choose edges, and emit a strand file or strand directory.

Once selected, the hot reading form should still be materialized contiguously when appropriate:

```text
strand 27 -> [231, 804, 991, ...]
```

The filesystem graph is therefore a rich query surface; a strand is a compiled traversal optimized for sequential consumption.

## Android first

The first implementation should use whatever ordinary filesystem and symlink support is available in the IB application's accessible storage on the current Android phone. Do not require a new filesystem.

Measure both costs:

- how many inode/directory entries the projections consume;
- how much latency comes from pathname and symlink resolution versus compact mmap indexes.

If a particular projection becomes too expensive as literal directories, retain its semantics and materialize it as a compact index instead. The important design commitment is multiple query-oriented indexes, not that every index must literally be a directory.

## Semantic operating-system direction

Multiply indexing the same fragments and links is a core semantic-system idea. Filesystems already provide several useful primitives—names, directories, links, ordering by names, permissions, and ordinary stream tools. IB should exploit those first.

If the same patterns recur everywhere, they provide concrete evidence for later OS/filesystem primitives: typed links, reverse-link queries, indexed directory projections, stable fragment identities, or fast materialization of a graph walk into a sequential strand.

See also `fragment-link-table.md` and `storage-model.md`.
