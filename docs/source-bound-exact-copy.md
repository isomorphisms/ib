# Source-bound exact Copy — SUN IB-D05

Owner: [isomorphisms/ib issue #102](https://github.com/isomorphisms/ib/issues/102)
and [recovery issue #120](https://github.com/isomorphisms/ib/issues/120).

This extends the existing primitive from
[isomorphisms/ib PR #13, “Add browser-owned display repair and Copy primitive”](https://github.com/isomorphisms/ib/pull/13).
It consumes the checked graph/source boundary from
[isomorphisms/ib PR #123, “Persist source-bound fragments and checked reading traversals”](https://github.com/isomorphisms/ib/pull/123),
accessor head `a3bbcb5741d34318d85ab32ad9627abddb6f4a62`.

`IB.CopyPolicy` accepts neutral candidates with stable target and fragment
identity, the existing source/revision binding, and a half-open code-point
subrange inside that canonical fragment. It resolves the exact retained
authoritative characters into the existing `CopyButton`. It never copies a
cleaned paragraph, rendered row or renderer-supplied replacement payload.
Zero-length subranges are valid. Empty target identities, missing/stale
provenance, wrong units, absent fragments and invalid ranges refuse.

Cheap explicit/code/preformatted role selection is separate from activation.
Unclassified roles and inconsistent kind/role combinations do not become
automatic copy controls. Discovery performs no clipboard action and needs no
model or page-style heuristic.

The browser owner supplies the actual current source graph and canonical site
identity. Candidates do not choose the site's policy context. Global and
per-site disable policy applies before preparation and again at activation.
Activation also checks the current trusted control target, original site,
source revision and resolved payload, so a recycled row, changed source or
newly disabled policy cannot silently copy some other text. Missing clipboard
capability returns a refusal. A successful result is a validated handoff, not
a claim that the platform wrote the clipboard.

The source tuple is an identity reference. S06's byte-owning boundary still
must verify source bytes and decoder/digest provenance before admitting the
graph. The pure core does not compute a digest or authenticate arbitrary caller
claims. The acceptance corpus uses synthetic source/digest labels explicitly.
It additionally refuses a changed payload hidden behind an unchanged claimed
tuple, but that does not replace byte verification.

## Executed evidence

The real clean Idriç compiler source is
`ff4d852862a3942592f8ade9afde8d409d9803be`. Its launcher SHA-256 is
`55bd2f65fde3fd6093048e75369002d14f12db8f12258778d197376e5f37c0bf` and implementation
SHA-256 is `10002074cfae31a15e6f136cd191b6abe9f8d58efcec438d21145e72802b2819`.

Actual `--check` passed for `src/IB/CopyPolicy.idric` and the preserved example
`ExactCopyAcceptance.idric`. Actual `--client copy_case_results` normalized
25 named cases to `True`. The case matrix beside the example records the exact
good, whitespace-mutant and restored results. The mutant checks successfully
but makes three exactness cases `False`; reversing it restores all 25 to
`True`. Compiler normalization returning a Boolean result is distinct from a
consumer program or backend. The normalizer's process status is 0 in all three
successfully evaluated variants; semantic rejection is the compared matrix.

Checked source SHA-256:

| Input | SHA-256 |
|---|---|
| `src/IB/CopyPolicy.idric` | `43fd0607802b6722b17b6154b1fa81455e4f138153ad5c28077f1b5e3246026d` |
| `ExactCopyAcceptance.idric` | `72159b1d36498c1d10642c22189b1b5ff7130bf65ef8874aa5b890145c278cfd` |
| S06 `src/IB/FragmentStore.idric` | `c58261c0788c16a5f180d48076bd6623d1c961f4fb79cea344261971e3b46bde` |

The source vocabulary check also passes. No consumer code generator, Java,
Gradle, Chez program build or replacement implementation was invoked.

## Remaining integration gates

D01/D02 have not supplied a qualified native view/clipboard adapter. There is
no executed clipboard write/readback, no proof that page CSS/JavaScript cannot
intercept or restyle actual controls, and no packaged, emulator or MIRO result.
This job therefore completes the independent pure Copy policy slice and leaves
the full UI/clipboard job **PARTIAL**. Renderer recreation is exercised only
as decode/rebuild of the accepted retained graph, without a physical renderer.

The selected consumer ICK/NDK path must execute the same cases before claiming
runtime acceptance. An actual platform run must separately compare retained
source hashes before/after and inspect clipboard bytes/code points. A raw
legacy `copy_button` alone is not permission for an adapter write: use the
validated browser-owned handoff and report the adapter's actual result.

These local source/normalization receipts do not qualify inherited repository
workflows that still build consumers through another toolchain. No merge,
real-source ingestion, device mutation or clipboard collection is included.

Review publication is a child source branch against the preserved S06 branch.
A draft PR is deferred because `.github/workflows/idric-core.yml` automatically
invokes the inherited Chez consumer build for these paths. That stage is not
the authorized source-check/normalization path and has no current ICK/NDK
qualification. No workflow was weakened or rewritten to disguise that gap.
