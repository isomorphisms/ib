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

## Import receipt publication

The one-off integration repair keeps `import_bundle : Bundle → ImportReceipt`
and the existing per-source intake unchanged. Its OS effect must publish the
complete receipt directory by a rename within the destination filesystem.
Frozen input may use temporary scratch on another filesystem; publication
staging must be a private child of the verified `cauldron/imports` parent.
The visible destination must contain the complete manifest, completion marker
and every unavailable reference's original provenance together. A conflicting
destination refuses without replacement, and a failed stage is cleaned up.

The earlier `$work/receipt` could reside in `/tmp` while the destination lives
on another filesystem. GNU `mv` would then copy the directory, exposing its
completion marker before the entire receipt had arrived. Grease owns this
filesystem operation; no new Idriç semantics or alternate source store is
needed. This repairs same-filesystem publication only; it does not establish
power-loss durability, fsync ordering or physical-device behavior.

### Independent publication acceptance

The Moon prerequisite was the exact Earth commit
`be372cf661e1915d016ee2f06dce134f8d54aff2`. The new
`tests/portable-evidence-publication.grease` test and its native Grease `mv`
fixture do not replace the implementation. They inspect the actual source and
destination filesystem devices and complete staged inventory immediately before
delegating to `/usr/bin/mv`. The input snapshot stays on `/tmp` overlayfs while
the selected synthetic home lives on `/dev/shm` tmpfs.

The test executed successfully on Ubuntu 24.04 x86-64 with the verified Grease
native runtime SHA-256
`7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`
(runtime archive
`295329ec3bb419d90364fcb5937eafe4306b03f68b7aaa63089dadde8082ffb5`).
The `GREASE` environment named its absolute installed entrypoint, which also
executed the test. `ASAN_OPTIONS=detect_leaks=0` was required because this host
prevents LeakSanitizer from reading its process-task information. This is
native command/filesystem acceptance, not leak analysis.

The final invocation exited zero with all four behavioral controls passing:
distinct-filesystem input with same-filesystem complete receipt publication;
injected pre-rename interruption with retained-source survival, no visible
receipt, private-stage cleanup and successful retry; a competing complete
winner with no clobber and successful idempotent verification; and rejection of
a deliberate mutant restoring default temporary receipt staging. The mutant
was rejected specifically for differing observed filesystem devices before a
visible receipt appeared. Its previously committed source remained exact.

Preliminary harness attempts exposed two test defects before acceptance: the conflict check
initially required only the adapter's post-`mv` diagnostic, while observed GNU
coreutils 9.4 refuses the no-clobber move itself with `not replacing`; and the
disposable mutant initially lacked intake's canonical Pensieve library. The
test now recognizes those two specific conflict-refusal paths and copies the
unchanged intake dependency into its disposable mutant. Neither failure was
counted as an implementation PASS. Android, physical-device execution, fsync
ordering and power-loss durability remain NOT_RUN.
