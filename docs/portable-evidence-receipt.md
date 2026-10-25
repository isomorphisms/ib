# SUN IB-S10 hosted execution receipt

Date: 2026-10-08. Owner: https://github.com/isomorphisms/ib/issues/104.
Review output: [isomorphisms/ib PR #124, “Add bounded selected-source portable
evidence export and import”](https://github.com/isomorphisms/ib/pull/124), draft,
against the retained Pensieve branch. No merge or closure was performed.

Retained baseline: `b4d93c47327f112433b90c0a77a00b3ddcba0af4`.
Final published implementation: `5204dc477674280ebd512b5494fb68d921c29733`.
Implementation tree: `c8f7033273ab7b1ab7fa10ab2e7500797616cc37`.
Tested local equivalent: `94a6f8778e4334a3946e8d35303b54a7545d7d3b`, exact same
tree. Initial publication was `c93866516c38046096720523aaadea1472ac6476`;
the final follow-up preserves original provenance in selected assistant context.

HTTPS terminal publication had no write credential. GitHub object API
publication preserved the tested blobs, executable modes, tree and retained
parent. The published final implementation was fetched through Git into a
separate detached checkout and all three maintained suites were executed there.
Every producer returned zero. This receipt is a subsequent documentation-only
addition, not a claim that a new target binary was built.

## Exact execution boundary

Container `0deb85a33ecd`, Ubuntu 24.04.3 LTS, Linux 6.18.44, x86_64.
Only synthetic fixtures and local directories were used. No real private corpus,
upload, model transmission, live Shizuku, Android packaging or device operation
was performed.

Grease entrypoint:
`/workspace/scratch/4cbbf3939bb5/il0/qualification/grease`.
Reported implementation: Oils 0.37.0, source
`5651cf97a1b5042f24f14112a7ade9a1518eb0bc`. Its unchanged packaging launcher
comes from Grease `ba869518c7d850de6c47d8c6234654575e264e6c`, restored artifact
`11419719229` from run `37478624499`. This is not a whole Grease-suite claim.

Entrypoint SHA-256:
`37088097c36a2f82cf45aaea9392e4a4cf16155fecb9da87d08a9c7fb4d92e4a`.
Actual implementation SHA-256:
`7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`.

The new adapter and all three suites run in native Grease mode. Only the
unchanged canonical `retain-source.grease` is invoked with its named inherited
shell compatibility mode `+O ysh:all`; no stock shell executes the new adapter.
`ASAN_OPTIONS=detect_leaks=0` is required by the ptraced host executor. It avoids
the observed LeakSanitizer/ptrace startup error and is not a language fallback.

Idriç compiler:
`/workspace/scratch/fd7b61cd742b/Idric/_/build/exec/idris2`.
Source checkout is clean at `ff4d852862a3942592f8ade9afde8d409d9803be`;
reported version is `0.8.0-ff4d85286`. Launcher SHA-256:
`55bd2f65fde3fd6093048e75369002d14f12db8f12258778d197376e5f37c0bf`.
Underlying `idris2_app/idris2.so` SHA-256:
`10002074cfae31a15e6f136cd191b6abe9f8d58efcec438d21145e72802b2819`.
The actual compiler checked and evaluated the Idriç policy through
`--client acceptance_results PortableEvidenceAcceptance.idric`. The compiler's
existing Chez bootstrap is identified; no Chez-generated IB consumer was added.

## Executed results

| Acceptance | Result |
| --- | --- |
| `tests/portable-evidence.grease` from published implementation | PASS, exit 0 |
| Equal bytes from distinct origins | Two distinct fixed source-ID goldens; exact raw/provenance retained |
| Repeated import | Same bundle receipt and original source identities |
| Private source with ordinary selection | Refused before export publication |
| Explicit private selection with assistant deny | Local export succeeds; context includes no source data |
| Classified secret with explicit private selection | Refused; selection cannot admit a secret source |
| Selected assistant-allowed context | Exact source bytes/provenance and whole-source byte coordinates; source command text remains data |
| Omitted/missing raw bytes | Unavailable references with original provenance; no fabricated empty canonical source |
| Tampered later source on fresh target | Entire input refused before any canonical source publication |
| Tamper/schema/profile/traversal/symlink/oversize cases | Refused; existing canonical bytes unchanged |
| Existing intake publication lock on source two | Source one stays usable; no complete bundle receipt; removal of fixture lock and retry succeeds |
| `tests/portable-evidence-policy.grease` | PASS, exit 0; ten actual Idriç evaluation values True |
| `tests/portable-evidence-mutations.grease` | PASS, exit 0 |
| Removed ancestor-symlink guard mutant | Clean control refuses; mutant writes into isolated outside fixture; refusal acceptance rejects mutant with exact exit 17 |
| Idriç default-admit mutant | Unselected-private/secret/profile checks evaluate False; mutant rejected |
| Public Idriç vocabulary on the new examples | PASS |
| Source whitespace/error check | PASS |

Fixed synthetic source IDs:

* Origin A: `6417fb2d83faf7c85c74114bf1a6f08ddeab64b393e9dab0926f07126996a711`.
* Origin B: `6ddf7ca08227cfb84dc46684e22b0186a06764d7301fb1012615022b43256211`.

The malicious-path and policy mutants ran in disposable copies. Maintained
sources and retained user state were never patched by the mutation harness.

## Tested files

| File | SHA-256 |
| --- | --- |
| bin/ib-evidence.grease | `38215e7b120cdef9127be0a04b9516699c0791778f6b8c2bdfa7b1000894291b` |
| tests/portable-evidence.grease | `47b23e0f8f4a8771b33710ad19b98559468219272badf2d00699f7e5bd9bab38` |
| tests/portable-evidence-mutations.grease | `b7a126f53cfe30a32f389de77e56dcc4f0bfb05fff35d8791331fc34dcc6cb46` |
| tests/portable-evidence-policy.grease | `d31017ae496deb96de1ba18eb4a3367880426311c89b90cf51aa1728cf6d26d2` |
| PortableEvidence.idric | `1f00f7c9e30a863cc2ebdcdcafc403710836332ae75b0d6657eb3cf6978ad57d` |
| PortableEvidenceAcceptance.idric | `9e700118b9fd5aa3ea8634caacf50d6bdac5cdf57dc8baddb55320957d835d81` |

## Remaining gates

Idriç policy/compiler evaluation and Grease wire validation remain separate;
qualified Idriç command-line policy integration is not complete. ICK/NDK
application/runtime delivery, Android filesystem/fsync and physical MIRO A1/C67
acceptance are NOT_RUN. Source hashes detect corruption, not signed origin
authentication. Static symlink checks do not prove kernel-enforced containment
against concurrent topology replacement. Upstream classification must identify
credentials embedded inside selected source content; this is not a secret-content
scanner. New task/event/decision formats and synchronization remain outside this
slice. Issue #104 is not claimed complete.
