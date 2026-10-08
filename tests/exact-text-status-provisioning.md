# Exact-text regression provisioning receipt

**BLOCKED_DURABLE_GREASE_PROVISIONING**, 2026-10-08. This is the remaining
automatic process-injection regression prerequisite for SUN IB-Q01 and
[PR #121, Reject exact-text compiler and oracle producer failures](https://github.com/isomorphisms/ib/pull/121).
The producer-status repair and focused local regression are implemented.
This receipt does not represent the injection harness as an automatic CI gate.

## Actual acceptance

PR head `c4a7b9bdf2ceac42b4f0aaa7d903255e307d22f3` has successful hosted
Exact text semantic oracle run
[37808933768](https://github.com/isomorphisms/ib/actions/runs/37808933768),
universal merge gate run
[37808933889](https://github.com/isomorphisms/ib/actions/runs/37808933889)
and browser foundation run
[37808933776](https://github.com/isomorphisms/ib/actions/runs/37808933776).
The exact-text job `113420442664` ran the real pinned compiler and all twelve
semantic comparisons. It did not run the process-injection harness.

The local harness was rerun from this exact head using actual maintained
workflow bodies and consumer Grease. `evidence/exact-text-status/current.tsv`
records success/incorrect-output/producer-exit-7/compiler-exit-7 with exact
observed statuses 0/1/7/7. Compiler failure retains a plausible executable and
log. Substituting main's workflow at
`10fa7bbe93ff9ffa1ba7b9f62e0dd8f6c10a86bc` makes the harness exit 1:
both exit-7 cases incorrectly pass, as recorded in `baseline.tsv`.
This is mutation-sensitive status acceptance, not exact-text semantics.

Host: Ubuntu 24.04.3 / x86_64 / Linux 6.18.44. Consumer Grease repository
`ba869518c7d850de6c47d8c6234654575e264e6c`, Oils gitlink
`5651cf97a1b5042f24f14112a7ade9a1518eb0bc`, executable SHA-256
`7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`,
wrapper SHA-256
`37088097c36a2f82cf45aaea9392e4a4cf16155fecb9da87d08a9c7fb4d92e4a`.
`ASAN_OPTIONS=detect_leaks=0` accommodates the ptraced host. This already
available sibling runtime adds no compile/link stage to IB.

## Maintained provisioning paths inspected

| Owner / exact source | Actual path | Remaining prerequisite |
| --- | --- | --- |
| Grease `ba869518c7d850de6c47d8c6234654575e264e6c` | `.github/workflows/grease-receipt.yml`, blob `4149ce14eabb1fba0cdc5a161a2994f31a098d19` | Inherited native C++ container build has no declared ICK/NDK stage contract. Uploads `_bin/cxx-asan/ysh` with diagnostics for **7 days**. No durable consumer asset. |
| Cat Food `609a9628d5a52860f956bf62e0914a0cd03292ae` | `build-tools.sh`, blob `9847c3623fd2d1862f23812bea5d80dc83a3874a` | Real source interpreter recipe builds vendored Python 2.7.13 using configure/make, then Grease. Its bootstrap compile stage has no ICK/NDK contract. |
| Same Cat Food source | `.github/workflows/grease-stage-one-fixtures.yml`, blob `b99f99c46c04db397b169b8d189a795561ef566a` | Exercises the source recipe and canonical Grease identity, but clones mutable Grease main. It does not bind a reproducible qualified producer for IB. |
| [Grease releases](https://github.com/dilapidated-shed/grease/releases), live API inspected 2026-10-08 | 21 returned release records and asset ABI receipts | Grease assets target NetBSD amd64 or Android/Termux ARMv7/AArch64; Android Ish assets are a distinct program. No matching Linux-x86_64 Grease asset was present. |

The Grease workflow verifies the source gitlink before its native tests. That
provenance does not qualify its generic compiler under the ICK/NDK contract.
The PR-triggered commit-run query returned no run for exact `ba8695…`;
that query does not establish push-run status. The sibling execution is
local runtime evidence only.

A raw root `grease` symlink in another scratch checkout refused with
`oils: Invalid applet 'grease'`. The tested consumer uses an exec-only wrapper
invoking the producer's actual `ysh` executable. Basename aliasing is not
runtime acceptance. No stock Oils/YSH, Bash, Python or Ish replaced the
Grease process fixture.

## First dependency and completion contract

The next owner must provide a durable **Linux x86_64 maintained Grease
runtime**, pinned repository/gitlink, artifact SHA-256, entrypoint and runtime
dependencies. Its compile/link stages must have the required ICK/NDK producer
contract. It must pass the language/identity smoke and this four-case fixture
on a clean intended runner. ICK C++ acceptance is not established by existing
C-only Android application probes. No compiler substitution manufactured a
package for this receipt.

Once that input exists, IB's existing exact-text workflow can acquire/verify
it and run `tests/exact-text-status.grease` automatically. The successful
control must pass; missing/corrupt/incompatible assets and plausible-output
producer failures must fail. Preserve the real pinned compiler and all twelve
comparisons. A clean runner and baseline workflow mutation must demonstrate
the gate without an expiring sibling artifact.

The inherited IB Chez compiler bootstrap remains separate unqualified build
debt. This receipt proves no new compiler build path, Android APK, emulator
or physical-device acceptance. No release, merge or device mutation occurred.
