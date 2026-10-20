# Fragment-store acceptance, 2026-10-08

IB-S06 extends the sole canonical successor, isomorphisms/ib PR #48,
“Rewrite canonical IB slice in current Idriç style”
(https://github.com/isomorphisms/ib/pull/48), at
631052fbeccd183c4fd409428180b3eab824c481. It does not modify that owner branch.
Its `IB.Strand.Strand` remains the information projection. Fragment traversal
continues to use `IB.Occurrence.MaterializedStrand`; graph persistence adds
source-revision binding and checked links around those existing types.

The new source is `src/IB/FragmentStore.idric`. Its TSV schema retains a source
identity, immutable representation reference, decoded-byte digest, decoder
revision and explicit Unicode code-point coordinate unit. Source bodies remain
in the original immutable store. Graph serialization is not prepaint, and a
renderer never owns source identity. Incoming/outgoing links and context
windows rebuild from graph records rather than durable index state.

`lib/fragment_store.grease` consumes the existing Longview
`lib/durable_object_store.grease`. The canonical successor does not contain
that adapter yet; it is a dependency from the retained Longview line, not a
second implementation. This run used its unchanged blob, SHA-256
d64f1806a1d5691f424b6cb69c86f060d6fab1857ed88e777bb9d05413b7b902,
in the sibling checkout at 8826790933fb3cd76d3f32e777dd4dd9ec7bcd03.
The wrapper snapshots bounded source/graph inputs, verifies decoded SHA-256
and UTF-8, invokes the semantic normalizer, preserves failure status and
publishes only successful output through the existing immutable mechanism.

## Evidence

The real Idriç checker is a clean checkout at
ff4d852862a3942592f8ade9afde8d409d9803be. Its executable reports
`Idris 2, version 0.8.0-ff4d85286`; the bootstrap implementation artifact
`idris2_app/idris2.so` hashes to
10002074cfae31a15e6f136cd191b6abe9f8d58efcec438d21145e72802b2819.
No stock Idris, RefC, generated C or alternative backend was selected.

Actual `--check` passes for the new core, generated acceptance program and
normalizer command. `--client acceptance_results FragmentStoreAcceptance.idric`
evaluates pure Idriç in this compiler's normalizer without consumer code
generation. All 13 cases are True: code-point/UTF-8 mapping (including a
non-BMP scalar), overlapping literal matches, traversal context, TSV round-trip,
alternate view cuts, wrong units, out-of-range and dangling records, truncated
commit, changed decoded revision, and index-free link rebuild.

A temporary known-bad change bypassing the source-binding comparison produces
False for `byte-units-refused` and `changed-revision-refused`. Restoring the
comparison restores all cases to True. The mutation is not in the delivered
source. This is compiler-normalizer semantic evidence, not an executable
backend or physical-device result.

The current Grease runtime is source 5651cf97a1b5042f24f14112a7ade9a1518eb0bc;
its implementation binary hashes to
7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c.
`tests/test_fragment_store_io.grease` passed against it: real bounded file
round-trip, plausible producer output followed by exit 7, stale source bytes,
unpublished partial tail and independent late reopen. The producer in this
test is an explicitly controlled OS fixture, not an Idriç substitute.

The inherited immutable adapter requires the runtime's compatibility options
`+O no_parse_sh_arith +O strict_errexit +O no_osh_builtins`; this debt is
explicit. The new driver uses Grease `forkwait`, `try` and block traps. The
sanitized host runtime also requires `ASAN_OPTIONS=detect_leaks=0` because
LeakSanitizer cannot inspect process tasks in this container. This disables
leak checking only; it does not change the selected language implementation.

Public Idriç vocabulary and whitespace checks pass. A pre-existing canonical
compatibility failure was repaired only in this child: current `Text` is a
reserved type spelling, so the old `Text` block constructor becomes `TextBlock`
at its declaration and two consumers. The canonical smoke also explicitly
imports `Data.Text` for its existing `unlines` call.

## Remaining acceptance

S06 is PARTIAL. The semantic core and ordinary-file boundary are implemented
and exercised separately. The integrated Idriç normalizer executable cannot
yet be generated through an observed qualified ICK/NDK consumer stage. The
discovered Chez bootstrap is not declared ICK/NDK consumer qualification;
no production executable was built to bypass this gate. Durable filesystem
fault injection is inherited from the unchanged Longview adapter; this new
consumer test adds preservation after normalizer failure and partial staging.
Physical Android, scrolling, installation and real private corpus are NOT_RUN.
