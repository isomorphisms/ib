# Linux first implementation wave

Tracking: [STAR assessment / SUN graph](https://github.com/isomorphisms/ib/issues/137),
[shared contract](https://github.com/isomorphisms/ib/issues/138),
[executable foundation](https://github.com/isomorphisms/ib/issues/139), and
[continuous parity](https://github.com/isomorphisms/ib/issues/146).
These jobs remain in progress; this slice does not complete their acceptance.

## Source integration

This branch is stacked on the existing source/fragment successor
a3bbcb5741d34318d85ab32ad9627abddb6f4a62 rather than creating another information
model. It includes the selected source-backed Strand/Prepaint/FragmentStore work.
LongViewLifecycle is reused byte-for-byte from Android lifecycle source
41d4cae9c15e42d3b0b4ed4ed957ba86a7ac396a; Git blob
72e891af7b76ca7e33517d7916e227db2c7f70cf. Both predecessors remain unmerged.

LongViewTask now chooses durable browser tasks for ordinary tabs too. The
legacy Android store's ordinary-tab refusal remains an adapter mismatch, not
accepted behavior. This change does not add disk storage or ordinary navigation
replay, and it does not weaken protected-address redaction.

FragmentStore retains Unicode code-point source coordinates and adds an explicit
conversion to Android UTF-16 units. Existing UTF-8 byte conversion remains
distinct. fragment_source_text resolves a checked fragment identity against its
admitted source/representation reference, decoded revision, decoder revision and
exact decoded text. It slices the source directly and preserves whitespace; it
does not normalize or reparse displayed text. Digest authentication and immutable
source admission remain the byte-owning adapter's responsibility.

The Linux C boundary is stateless. IB.Linux.Platform isolates bounded C values
and refuses embedded NUL before passing a borrowed UTF-8 string. It explicitly
rejects Android x86-64 as a Linux desktop target.

## Acceptance and execution

Flexible Pipes owns the first-wave envelope and pipeline registration. Kitchen
owns the versioned Grease recipes; Ike executes their source-file protocol and
receipts the generated Ikefile hash. Cat Food supplies pinned compiler and
native Grease artifacts. AI-CI owns independent golden cases and the existing
follower verifier; producer assertions are not imported as expected results.

The initial 38-case Linux checker/evaluator run passed, rejected disposable-tab,
UTF-16-width and source-binding mutations, and restored the clean control. A
producer that printed True then exited 7 was refused before evaluation. The
public Idriç vocabulary guard passed. Native generation/FFI qualification and
exact published-source replays are recorded separately in FP receipts. The
generated Chez program actually executed all 46 shared/native cases, rejected
the C byte-count mutant and missing library, restored the clean library, and
launched from a relocated Unicode/apostrophe path. This is build-container
runtime evidence; it is not graphical desktop acceptance.

| Evidence lane | Meaning | First-wave obligation |
| --- | --- | --- |
| Semantic | Actual shared Idriç modules plus independent goldens | Linux check/evaluator result is available |
| Linux generated program | Maintained Chez program, declared native runtime, ICK C ABI | 46 cases and native failure/recovery passed; no direct-ELF claim |
| Android ARM32 / ARM64 | Same shared semantic changes on maintained targets | Required unresolved followers; do not accept the shared change yet |
| Native graphical | Real ib-desktop in qualified Wayland/X11 session | Not run by this slice |
| Real network | Actual HTTP(S), redirects and clicked-link content | Not run by this slice |
| Durable recovery | Exit/relaunch and deliberate worker/renderer termination | Not run by this slice |

## Continuing gate

AI-CI's ib-shared-core/parity.grease wraps its existing follower verifier. It
requires exact-source Linux, MIRO A1 and TAB_P10 records, their declared acceptance
kinds, and matching accepted receipts. Empty, missing, stale, conditional or
blocked records cannot make shared parity green. FP keeps these exact-trigger
jobs and receipts. Future shared-core changes must create new exact-trigger
records or explicit supersessions; ancestor success is historical evidence.

This bounded gate is distinct from the eventual graphical/network/restart gates
and is not yet a required default-branch CI check. SUN IB-X64-08 must wire the
qualified producer/artifact closure into hosted Ubuntu CI, pin all dependencies,
cover every material input without incomplete path filters, and keep the lanes
separate. Until both Android followers pass, the integration PR remains draft.

## Outstanding work

First-wave qualification still needs fresh-host/compiler qualification and
delivered-bundle reproducibility. Two fresh consumer output builds produced
identical generated Scheme, launcher and C library but different compiled Chez
program hashes; byte reproducibility is unresolved. The compiler/support
archive has legacy producer toolchain debt; the new consumer C stage uses ICK
and records its borrowed host runtime hashes. A direct x86 backend without
general Text, allocation and FFI remains insufficient for the browser.

Maintained compiler 94dfd99bd also misquotes non-ASCII Chez build paths and
returns zero after that compilation failure. The checking recipe refuses those
build roots explicitly and rejects Error: output. Unicode source text/FFI and
runtime relocation are separately successful. The upstream repair is tracked
in [Idriç issue 138](https://github.com/isomorphisms/Idric/issues/138); a proposed
repair is not counted as executed work.

SUN IB-X64-02/03/04/05 must deliver the first integrated milestone: native
window/input, existing source-backed prepaint/reflow, real ICU acquisition and
navigation, durable storage/lifecycle and fault recovery. Pensieve and the
workbench follow that useful browsing slice; full-page rendering remains optional.
