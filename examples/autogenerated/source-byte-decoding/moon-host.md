# Independent host qualification

Prerequisite: decoder implementation commit
`f6c1c195f71070014e7aa5b254beea20051f91e5`, tree
`364684f10d4cfa95f5996a6c6cdb68b21d686955`.

The settled semantics are an octet-to-Unicode decoding operation and a
source-bound disposable derivative. Original navigation observations are
preserved as data; derivative creation grants no reader admission, including
when the original observation says YES. Grease owns process orchestration and
same-filesystem no-clobber publication. The actual Idriç binary owns decoding.

The independent check rebuilds the existing Idriç command and semantic oracle
in a fresh source root, runs the provided controls and compiled mutants, then
checks navigation-YES preservation, opaque descriptor retention, malformed
descriptor refusal and a competing destination created after the initial
precheck. The competing winner must survive and the staged loser must be
removed. No decoder semantics or native admission gate is changed.

The primary WHATWG index was fetched independently from
https://encoding.spec.whatwg.org/index-windows-1252.txt. It has identifier
`e56d49d9176e9a412283cf29ac9bd613f5620462f2a080a84eceaf974cfa18b7`,
SHA-256 `2c5fa192d566f92b8b9e6e116255b1f94016b4ece5b9dacb3b98be70b8c60795`
and 5,667 bytes. The initial retained fixture had one additional trailing
newline; `diff -u` showed only that final blank line. The fixture is now exactly
byte-identical to the independently fetched primary file. Its index identifier
and every numeric mapping are unchanged. The fresh command must also pass
against the fetched file.

The first supplemental harness expected the adapter's post-rename conflict
message. This host's `mv --no-clobber` refuses with exit 1 and “not replacing”
before that message; the winner survived and the losing stage was removed.
That failed harness is retained under `initial/MoonVerify.grease`. The updated
check accepts either real conflict diagnostic while still requiring nonzero
status, exact winner retention, no published loser, and removal of the stage.
This is a corrected test assumption, not an implementation change.

The complete fresh run at `moon-decoder-final-evidence` exited zero using the
verified Idriç `94dfd99bd3e376507fedc8611053b7173b2519f0` bundle and native
Grease executable SHA-256
`7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`.
The archive digest and every `payload.sha256` entry were independently checked.
`ASAN_OPTIONS=detect_leaks=0` was explicit for this host's orchestration;
this is not leak-analysis acceptance. Both the primary and corrected retained
index passed all 256 scalar checks; the existing alias, binary NUL/CRLF/C1,
BOM, process/source-integrity, exact real-capture and two compiled-mutant
controls passed. The supplemental checks passed as described above.

The Idriç decoder and command source remain byte-identical to the prerequisite;
their digests and the independently rebuilt programs are in `moon-host.tsv`.
Chez program bytes depend on their build paths, so the independent program
digests do not purport to equal the earlier staging-root digests. Vocabulary
and full `git diff --check` passed with no fixture whitespace exception.

These are Linux x86-64 host checks; DEX/ART, joined Prepaint/Arena admission,
Android delivery and physical-device evidence remain NOT_RUN. No new blocking
decoder capability was exposed. The next integration obligation remains a
decoded-representation consumer that preserves original byte identity and
independently establishes navigation/reader admission; this qualification does
not supply the missing redirect chain or implement that consumer.
