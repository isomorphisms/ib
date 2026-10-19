# Retained sources at the Cauldron/Pensieve boundary

This extends the independent shell-first Pensieve line, not Longview's Android
store or the canonical browser event model. Drive transports bytes; IB retains
source evidence and owns interpretation. A selected shard (at most 16 MiB) goes
through `ib retain-source SOURCE_BYTES SOURCE_TSV`, then an explicitly supported
decoder. Set GREASE to a verified runtime; no phone-side compilation is needed.

The TSV requires unique `source_kind`, `source_identity`, `source_generation`,
`acquired_at`, `privacy`, `exposure`, `sha256`, and `byte_count` fields. All other
provenance fields are retained verbatim. Privacy is ordinary or private;
exposure is local-only or selected-export. Neither value authorizes an upload.
Ordinary sources can enter the ordinary search index; private projections are
excluded and can be inspected only by explicitly selecting their source path.
Both are stored with private local file permissions. Sensitivity is independent
of temperature, placement, and rebuildability.

Cauldron keeps exact bytes and the manifest under an identity derived from both.
Equal bytes from different identities/generations remain distinct observations.
Same-input replay verifies rather than overwrites. Staged publication and a
per-source lock prevent cooperative writers from replacing retained evidence.
An interrupted lock requires inspection; it is not silently cleared. This shell
slice does not yet synchronize publication with fsync, so process-reopen and
hash evidence are not power-loss durability or Android store acceptance.

`ib project-raw-urls SOURCE_ID` is the first executable decoder. It follows the
existing IB.History raw-line trim/comment rule, keeps duplicate occurrences,
records original line numbers and explicit unknown timestamps, and feeds the
existing exact-text index. Its files are disposable: deleting Pensieve and
reprojecting does not reacquire or change retained evidence. It does not claim
to have executed the Idriç semantic model. No live browser/Takeout source was
inspected to validate this adapter.

## Drive-selected ChatGPT/OpenAI JSON

Use the existing bounded ZIP inventory/selective extractor in
`dilapidated-shed/cloud-storage-api`. Retain the extracted `conversations.json`
unchanged, plus Drive file ID, version or head revision when available, source
size/checksum/modified time, selected member path, compressed/uncompressed sizes,
CRC, extraction receipt, acquisition time and extractor revision. Those belong
in the intake manifest or retained bounded receipt bytes. A missing version
must be recorded as unavailable, never guessed from a filename. Detect source
metadata drift before publishing an extraction. A provider checksum identifies
the archive; the local SHA-256 identifies the selected member.

The [conversation adapter](retained-source-ingestion.md) now implements this
contract for bounded retained source shards using `ib-source distill SOURCE_ID`:

- preserve conversation and message IDs when supplied; otherwise use stable
  source coordinates and mark the external identity absent;
- preserve the mapping/tree, parent/children edges, current-node selection,
  message/conversation timestamps and original source order separately;
- retain multimodal parts, attachment identities and references as structured
  references, including unavailable external bytes; do not silently convert
  everything into a transcript or treat the selected branch as the whole tree;
- retain unknown fields and exact JSON; use source coordinates/JSON pointers
  for every derived record and bind the decoder revision and raw hash;
- distinguish complete, partial, unsupported and malformed inputs; rebuild
  views/indexes from retained bytes without changing original identities;
- put human corrections/decisions outside disposable decoder outputs.

JSON remains appropriate for the original external format. TSV intake is not a
replacement conversation schema. An adapter fixture would prove only decoder
behavior. The last authenticated discovery found no actual standard export;
live ChatGPT ingestion remains SOURCE_ABSENT until independently observed.

## Other sources and raw mail

Selected history shards and ordinary local material use the same intake, with
their own source policy and decoders. No special Drive protocol is needed.
SDF raw mailbox archival keeps its fixed [0,T) generation in Drive, with exact
source and destination hashes/receipt, without passing a whole mailbox through
this bounded local intake. Later selected mbox records link back to generation
and byte ranges. SPEC-LIST refiling is a separate mutating operation with its
own lock and recovery qualification; it is never an archival prerequisite.

Ownership: Cat Food #117 owns delivery; Kitchen owns user procedures; Flexible
Pipes owns execution/replay; AICI owns evidence validation. No credential or
private corpus belongs in their public regression artifacts.
