# Native IB reader — f43af998 acceptance ledger

**Campaign incomplete. Physical TAB_P10 acceptance is BLOCKED; public Google results FAIL. This candidate is insufficient for release.**

Candidate source: `f43af99856a8ad4161114863e681c118e38c2218`. The original signed `ib-reader-f43af998.apk`, exact hashes/signer/archive receipt `ib-reader-f43af998.receipt.tsv`, and published ledger are retained together in `candidates/` only after the publication program verifies the required exact-source evidence.

Runtime scope is actual x86_64 Android ART, API 29 and API 34. Packaged ARM ABIs are independently checked build outputs; ARM execution and all physical tablet observations remain unrun. Emulator Binder/Shizuku transactions are never labeled physical transactions.

| Requirement | Status | Evidence / practical limit |
| --- | --- | --- |
| Maintained checked Idriç Activity, parser, NativeSourceProjection and source decoding compile | PASS | [Producer 41370466399](https://github.com/isomorphisms/ib/actions/runs/41370466399); actual consumers, not replacement fixtures |
| Original NIL/fastUnpack paths; reachable lists, records, constructors, closures, helpers, Unicode and checked foreign boundaries | PASS | Same producer and actual ART; reusable compiler153 |
| Command, pure-export and framework Activity entry contracts remain distinct | PASS | [android-NDK153 / PR41](https://github.com/isomorphisms/android-NDK/pull/41); all six suite families |
| Independent DEX decoding, deterministic bytes, explicit refusal, no stale DEX and compiled semantic negatives | PASS | Source/host checks, both required ART workers and independent full replay |
| App-owned private Binder registration, caller/provider identity, official Shizuku authority and revocation | PASS | Actual emulator transactions; forged callers/flags and stale-authority compiled controls rejected |
| Real Grease-supervised ICU, bounded navigation/source/attempt/generation and private streaming/PFD handoff | PASS | Installed original APK and independently verified source; no app direct-network fallback |
| Worker/Binder death, timeout, stale results and last-good preservation | PASS | Host/native faults, actual SDK Binder death, dead-RPC revocation without notification and compiled unsafe revocation control |
| Real process loss during a held ICU response | PASS | Original PID must die before response release; complete-response unchanged-state claim rejected; independent source/history/readback unchanged |
| Legitimate process recreation and returned Activity ownership | PASS | New process permitted; real two-Activity return, late-destroy refusal, resume-noop/detach-any controls, new navigation and offline restoration |
| Activity transition during an active HTTP request | NOT_RUN | No separate claim beyond the qualified process-loss and ordinary Activity-return paths |
| Requested/final URL, observed redirects, status/type/charset/transfer, bytes/digest, attempt and source/generation | PASS | Authenticated transport observations and independent original-range receipts; unavailable observations are not invented |
| HTTP/HTTPS and TLS trust | PASS | Actual ICU HTTPS and controlled HTTP; real untrusted certificate rejected |
| Status errors, binary/unsupported charset/invalid UTF-8, truncation, encoding, malformed chunking and body bounds | PASS | Installed refusal profiles preserve the prior source and history |
| windows-1252 and ISO-8859-1 through actual source admission | PASS | Fresh independent decoder lanes require actual shell UID2000 after snapshot, accepting the observed quiet status0 CLI case; original bytes, decoded derivatives and source identities remain distinct |
| Native title, paragraphs, headings and actual URL entry/loading | PASS | Installed native view and SDK input; actual fetched content |
| Absolute/relative/root/query/fragment and supported base links | PASS | Actual touches, newly acquired documents, original bytes and independent address/history checks |
| Once-encoded UTF-8 query, spaces/plus/literal percent | PASS | Exact literal request target at the real ICU fixture server; genuine ChordReader query also checked |
| Native fragment touch, source-linked insertion line, Back/Forward and padded visibility | PASS | Independent SDK geometry plus collapsed insertion point on the actual target line; zero-point and stale-cursor/animation compiled APKs rejected |
| First link touch after an idle reflow reaches the native document | PASS | Actual SDK frame commit and child DOWN/UP within200ms; single-scroll compiled control rejected; new ICU document independently read back |
| Browser-owned history and retained document reopening without refetch | PASS | Source/index/receipt counts and bytes unchanged during offline Back/Forward |
| Scrolling, source-linked selectable Unicode text and exact clipboard | PASS | Actual SDK selection/copy, stale-clipboard control, surrogate/soft-hyphen and source-coordinate negatives |
| At least two visible reflows and measured width handling | PASS | Greedy/Squared selector and actual native layout; incompatible complete/streaming modes refuse explicitly |
| Incremental streaming renderer | NOT_RUN | Current Activity admits CompleteSource; no streaming renderer or full shaping/browser compatibility claim |
| Canonical Arena v2 ordinary-file append/publication/readback | PASS | Userspace contract, complete response validation, synchronization and atomic admitted-generation publication; no SQLite/kernel-appendfat claim |
| Short writes, storage full, interrupted/torn commits, permissions, corruption and independent readers | PASS | Host/native fault controls and actual emulator source/journal corruption; hardware storage faults NOT_RUN |
| Source identity independent of storage path; retained sources survive failed acquisition | PASS | Canonical identity/generation/digest checks; no removable-storage migration |
| Source/history/last-good reconstruction after loss, update and reboot | PASS | Actual emulator replacement without uninstall, process kill and guest reboot; checksummed journal fallback |
| Explicit capability/document errors and partial static HTML | PASS | Last readable source remains available; JavaScript/forms and unsupported documents are explicit limits |
| Genuine managed ChordReader SEARCH delivery and input/caller validation | PASS | Frozen e6c84d645ad7086a11ae2735258188a49e444d13/version1002; spoofed/no/wrong/warm caller controls rejected |
| Google acquisition and original-byte handoff through the same pipeline | PASS | Actual Google response, identity, ranges and digest; this is not public-results acceptance |
| Actual public Google results | FAIL | First-party continuation supplies JavaScript-enabling instructions; no fabricated results, browser or DDG fallback |
| Clicked public Google result and retained result history | NOT_RUN | No qualifying public result in those responses |
| Direct checked DEX + ICK/NDK ELF + aapt2/zipalign/apksigner APK | PASS | Original signed candidate retained without rebuilding or re-signing |
| Actual application DEX/native rebuild and signed APK comparison | PASS | Checked Activity/platform/ICU DEX repeated; all three application ELF rebuilt into fresh paths and compared; signed APK bytes identical. Native ICU/Grease dependencies are unchanged hashed inputs, not rebuilt twice |
| Independent packaging determinism and caller input preservation | PASS | Original9861 timestamp counterexample rejected;153 fixed producer must produce equal signed bytes and preserve caller content/modes/mtimes; exact ai-ci result archive required |
| Second build of native ICU/Grease/compiler/bootstrap dependencies | NOT_RUN | Reused as declared source-bound hashed inputs; no claim of a second full toolchain or dependency build |
| Existing package/version/certificate and emulator data-preserving replacement | PASS | org.isomorphisms.ib.prepaint, versionCode3/versionName0.3.0; established test certificate; exact certificate in receipt |
| Actual installed tablet signer/version and replacement compatibility | BLOCKED | No authenticated installation observation; no uninstall, downgrade or signing rotation attempted |
| Fresh Cat Food tablet identity, ABI/SDK, storage/capacity and Shizuku | BLOCKED | No authenticated ADB/SSH/Cat Food endpoint; synthetic inventories are not physical evidence |
| Tablet visible launch, example.com ICU/body/Arena match and native link (checks1–8) | BLOCKED | Hardware NOT_RUN |
| Tablet Back/Forward, scrolling, selection/copy and reflow (checks9–10) | BLOCKED | Hardware NOT_RUN |
| Tablet process/restart/reboot persistence and refusal/recovery (checks11–13) | BLOCKED | Hardware NOT_RUN |
| Tablet genuine ChordReader results/click (check14) | BLOCKED | Hardware NOT_RUN; public Google result flow also fails in emulator observations |
| Independent source-bound inputs/outputs, hashes, statuses and negative controls | PASS | ai-ci observer in candidate receipt; full plus isolated profiles consume the same original and nine compiled control APKs |
| Other ai-ci legacy delivery/Grease bootstrap CI | FAIL | HTTP410 on expired old Grease artifact12192502970; these separate workflows were not rewritten or counted as reader acceptance |
| Legacy live-arXiv progressive-prepaint CI | PASS | DCE run41367335245 passes unchanged title/heading/abstract prefix assertions against134877 public bytes; F43 browser-foundation41370466428 also passes. Earlier live-input failures remain historical; no causal code repair is claimed |
| Automatic merge/permanent release/SD migration/data deletion/reset/formatting | NOT_RUN | Not performed; restrictions preserved |
| Product completion / later release decision | BLOCKED | Required physical acceptance and working public Google result flow are missing |

Compiler:153e7fc8053ba4072954a5e77d4efa3609d829e4; frontend:740bbf2b1d9e811fd00c719c3473eede17ceea3b; ICU:5b0ab280c3f2613ab933ed230b5cc50f50b9613f; Idric-Net:b21801b72acb8ab835c297bbe298af8599550134; canonical append source:05a0cb7246c5a9f46abfbe92925812e7b607f454. Grease/ICK/NDK/SDK observations, exact DEX/ELF/APK hashes and every frozen archive are retained in the candidate receipt and referenced source-bound runs.

The active physical target is SVITOO TAB_P10. Damaged MIRO A1/C67 evidence remains historical. An unavailable SD card never means deleted source. No physical ABI, API, storage layout, authorization or signer is inferred from a model name or an emulator.

A previous DCE build check changed the worker command name and failed DEX equality. [Independent decode41370605552](https://github.com/isomorphisms/ai-ci/actions/runs/41370605552) found only the getArgs()[0] constant difference (archive13109405471, SHA2568719e724f56caa69c2c05572d2319d5cc02535c447a6d3543fa704db7b128aae). The F43 repetition keeps that input fixed in fresh directories. No candidate DEX normalization or compiler semantics change was used.

Build provenance: application C uses qualified ICK with the NDK assembler/linker. The maintained ICU Android transport/OpenSSL uses its declared NDK C build interface. Grease worker native bytes retain their qualified source/artifact identities. These are distinct recorded build owners.

Transport provenance: [ICU35](https://github.com/dilapidated-shed/icu/pull/35) retains bounded chunked framing, Content-Range observations and redirect HTTP/1.1 handling from the overlapping36 work while owning the maintained Android/JNI path. [Idriç-Net14](https://github.com/dilapidated-shed/Idric-Net/pull/14) owns the pinned finite HTTP failure outcomes. The historical isomorphisms/Idric-Net URL redirects to this same repository (ID1352865034); it is not a different network implementation.

Review: [IB167](https://github.com/isomorphisms/ib/pull/167), [compiler41](https://github.com/isomorphisms/android-NDK/pull/41), [independent acceptance331](https://github.com/isomorphisms/ai-ci/pull/331), [APK delivery168](https://github.com/isomorphisms/ib/pull/168). Trackers [151](https://github.com/isomorphisms/ib/issues/151) and [164](https://github.com/isomorphisms/ib/issues/164) remain open.

Independent observer notes: ecea325a32d7d922540a51d6e460349d4d9fabef adds a read-only SDK check that the collapsed insertion point lies on the visible target line. Earlier84c API34 full replay exposed an escaped anchor-reset compiled control; it is retained as FAIL and is not reused as acceptance. The observer now rejects either of the two exact target visibility/caret assertion failures, with the instrumentation failure status required. The application bytes are unchanged. Both current full replays execute the stronger check.

A frozen read-only diagnostic41375984193 established status0 with empty root/unroot output in the earlier decoder failure. Current decoder acceptance allows that observed CLI behavior while still requiring bounded reconnection and actual shell UID2000 before readback. Both current decoder lanes pass original-byte, generation, digest and charset controls.

Two initial ecea API34 isolated attempts stopped producing output after successful native reader assertions. They were canceled only after a fixed-run identity/time check; partial archives13113145705 and13112334800 retain those incomplete attempts. They grant no fragment or SEARCH completion. The same-source targeted retry records used for publication are identified by the final frozen archive table; completed full suites and all other independent results are preserved.

Final independent core run41376593878 contains all eight successful full/fragment/lifecycle/link API29/API34 jobs, including targeted API34 fragment retry125069029229. Search/decoder run41376593837 passes both decoders and both genuine SEARCH handoffs, while both complete public-results lanes FAIL after real first-party continuation. Its API34 targeted retry125069099920 supplies the fresh search receipt. Exact completed archives are frozen in archives.tsv and the candidate receipt; canceled captures are not substituted.
