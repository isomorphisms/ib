# Retained sources for Cauldron and Pensieve

The existing arXiv path retains acquired material in Cauldron and derives text
in Pensieve. `bin/ib-source.grease` extends that boundary to explicitly supplied
source bytes. It does not add a storage provider, fetch credentials, import
secrets, or turn a Drive object into a browser resource/tab/event.

## Admission

`retain FORMAT BYTES PROVENANCE.tsv` accepts `chatgpt-export`, `raw-urls`, or
`opaque`. Provenance is a strict two-column TSV with exactly these fields:

| Field | Meaning |
| --- | --- |
| source | Caller-owned source name; not a byte hash |
| corpus | Caller-owned corpus name |
| provider | `drive` or `local` |
| object | Opaque Drive object ID or explicit local origin |
| version | Drive provider version/revision, or explicit local generation |
| member | ZIP member name, or `-` for an entire object |
| acquired_at | Acquisition time supplied by the acquisition receipt |
| privacy | `ordinary` or `private`; neither means credentials |
| model | `allow` or `deny`; explicit per source, independent of privacy |

This convenience entrypoint converts the descriptor into the existing
`retain-source.grease` manifest and calls that canonical intake. It adds
source_kind/source_identity/source_generation, measured byte_count and SHA-256,
and defaults exposure to local-only. `ib retain-source` also accepts the complete
manifest directly, retaining additional provider/member receipt fields verbatim.
Cauldron retains `raw` and `provenance.tsv` under `cauldron/sources/<identity>`.
The canonical source identity binds the exact manifest and payload SHA-256.
Different origins of
equal bytes remain different records. Different generations of one source also
remain distinct. Admission copies only the supplied member, never its entire
remote ZIP. `opaque` supports retained material without parsing it.

All files use private local permissions even for ordinary material. Those
permissions are storage hygiene, not a claim that every corpus has the same
secrecy policy. The privacy/model decision is mandatory; the tool has no model
or upload hooks and does not expose credentials to the corpus layer.

Publication uses the existing per-source lock and staged intake, then rename.
Existing records are byte/hash-checked, never replaced.
Interrupted staging remains unaccepted; retry reconstructs it. This is an
ordinary-file host slice, not the Android immutable-store/fsync qualification.
Its locking assumes one user-controlled store, not hostile concurrent writers.

## Derived records

`distill ID` verifies retained bytes before rebuilding
`pensieve/sources/<privacy>/ID`. The parser/command digests and jq version are
retained with each conversation view. Exact-text reindex includes ordinary
sources and excludes private sources. Private views require explicit selection.
The model allow/deny field is retained policy, not an implemented transmission
hook or authorization. No decoder deletes Cauldron bytes.

For ChatGPT, retained JSON is authoritative. The adapter emits source-linked
conversation and mapping-node records, retaining complete node/message objects,
parent/children, original IDs, timestamps, current-node selection and unknown
fields. Array position and JSON Pointer are locators, not invented message IDs.
Mapping iteration order is explicitly unspecified: parent/children preserve the
conversation graph; text output is an inspection projection, not a linear chat.
Only textual content parts enter the disposable text view. Non-text parts,
attachments, citations and other references remain in complete node records and
the original bytes; unresolved references are not fetched or declared retained.
Schema errors refuse distillation without damaging source or previous views.
The canonical intake currently accepts selected shards up to 16 MiB. The JSON
adapter uses jq within that bound. Larger conversation exports need qualified
streaming/sharding support; this fixture does not establish that acceptance.

The raw-URL adapter delegates to the existing `project-raw-urls.grease` and
IB's already implemented `raw_urls` history semantics:
ordered nonempty, non-comment lines. It retains repeated visits and source line
locators in coordinates.tsv. It does not manufacture timestamps, titles, tab IDs or event IDs.
Chrome/Firefox/Google Activity decoding remains separate existing work; this
slice does not silently invoke its inherited Python importer.

## Drive handoff

Use cloud-storage-api's bounded ZIP inventory/extraction transport. Preserve
the exact metadata response, archive inventory and member verification receipt
alongside the extraction. Supply the *observed* Drive object ID and version,
member name and acquisition time; do not infer identity from a filename.
The present live extraction harnesses are tests, not an installed product
extractor. Version-fenced member extraction and durable attachment acquisition
remain transport work. A descriptor is caller-supplied provenance, not proof
that this command authenticated with Drive.

No actual OpenAI export has been observed in the authorized account. Synthetic
test JSON establishes decoder behavior only. Retention, discovery, authenticated
extraction, private corpus inspection, model quality and physical C67 execution
are independent evidence stages.

Raw SDF archival remains `[0,T)` → bounded SSH producer → Drive upload, owned
by cloud-storage-api. mbox/SPEC-LIST parsing/refiling consumes a verified raw
generation later; it cannot gate raw archival or authorize source mutation.

Cat Food owns runtime/package/deployment facts (issue 117); Kitchen owns user
procedures; Flexible Pipes owns repeatable execution; AICI owns receipt policy.
Neither retained source records nor this command duplicate those roles.
