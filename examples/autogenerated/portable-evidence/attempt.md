# Selected portable evidence

SUN IB-S10 extends the existing Cauldron intake on Pensieve
`b4d93c47327f112433b90c0a77a00b3ddcba0af4`. It does not create source IDs or
decode conversations again.

The domain values are existing source IDs, immutable provenance, raw-byte
representations, unavailable references, explicit private-source selections and
per-source assistant policy. Export maps selected references to a bounded
directory; import verifies every row before calling existing intake. Repetition
preserves source IDs and bytes. Source-coordinate units are whole-source bytes;
no decoded-character span is inferred from byte offsets.

The intended semantic operations are `admit_selection : Selection →
SourcePolicy → Maybe SelectedSource`, `assistant_reference : SelectedSource →
Maybe SourceReference`, and `reconcile_import : SourceReference → StoredSource
→ ImportOutcome`. Explicit private selection cannot grant assistant permission;
secret/profile classes are outside this source adapter's admitted representations.

Grease owns file traversal, hashing, bounded copies, staging and existing intake
invocation. Idriç owns the policy meaning. An Idriç attempt is retained here;
the initially located compiler source had no built executable. A separate clean
current compiler checkout was subsequently found and verified.
No RefC, stock Idris, Java or generated-C substitute is used.

The Grease directory adapter runs in native Grease syntax. The real Idriç
compiler at source `ff4d852862a3942592f8ade9afde8d409d9803be` checks and evaluates
the ten policy fixtures through `--client acceptance_results
PortableEvidenceAcceptance.idric`; every value is True. This is compiler
evaluation, not a built target executable. Wire decoding/meaning checks in the
adapter remain an explicit boundary pending qualified Idriç command-line
delivery; they are not described as an executed Idriç application. No
Chez-generated consumer was introduced solely to claim integration.

The first remaining capability is a qualified current Idriç command-line program
that checks selected-source policy with exact argument/status semantics. The
acceptance must deny unselected private sources, secrets/profile classes and
assistant-denied sources, while admitting the matching ordinary and explicitly
selected private controls.
