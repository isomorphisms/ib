# Source-linked fragment store

The intended result is a recoverable source graph and ordered reading traversal,
independent of any renderer or model. `IB.Strand.Strand` remains the canonical
information-block projection owned by PR #48. `IB.Occurrence.MaterializedStrand`
is an ordered list of fragment identities, not a replacement information model.

Values: an immutable source representation, decoded authoritative text, its
verified decoded-byte digest, decoder revision, half-open Unicode code-point
ranges, stable fragment identities, a chosen traversal and explicit links.
The graph stores references and ranges; it never serializes rendered text as
canonical input. An acquisition/storage adapter verifies decoded bytes against
the supplied digest before constructing the semantic source binding.

`revisioned_source` checks scalar identities. `validate_graph` checks range,
identity, traversal and link meaning. `serialize_graph`/`decode_graph` round-trip
ordinary UTF-8 TSV records with an explicit terminal commit marker. Decode is
bound to the exact current source/representation/digest/decoder tuple.
`utf8_byte_offset` maps a code-point boundary to a UTF-8 byte boundary explicitly.
Incoming/outgoing links and occurrence windows derive from canonical records.

Grease owns private temporary files, bounded capture, rename and filesystem
recovery; Idriç owns validation. No Android, Java, RefC or model boundary is
required. A compiler typecheck is separate from an executable backend result.
The maintained compiler checkout discovered here is ff4d852862a3942592f8ade9afde8d409d9803be,
with a Chez bootstrap checker. Consumer executable generation still requires
the shared declared ICK/NDK stage qualification; no alternative backend is
implicitly admitted by its presence.

The runtime results and first specific blocker are recorded in
`docs/fragment-store-acceptance.md` when executed. No fallback language is used.
