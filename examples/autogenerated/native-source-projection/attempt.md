# Retained source projection to the native frontend

The replacement view must consume IB's existing decoded-source model rather
than making Android own HTML recovery or inventing another representation.
Input is decoded Text; output in this bounded attempt is the recovered title
as Text. The existing InformationView and RecoveredDocumentLog own the parsing
and source semantics; the view only projects their result.

NativeSourceProjection.idric imports those exact existing modules and targets
the checked DEX backend. This attempt is required before claiming that the
native reader can replace the complete Java prepaint/source pipeline.

Actual source check passes with Idriç 2734ac5a86610eddfab73b1644d72b4ac85a65ee.
The actual checked DEX driver rejects the existing recovery/title path with:
`DEX tail-loop parameter needs a supported first-order runtime representation`.
No source.dex is emitted. The driver prints that backend error with exit 0;
artifact/diagnostic inspection correctly classifies it as failure.

The source imports are materialized from this IB line's actual src/IB into a
fresh temporary source root, since IDRIS2_PATH supplies compiled modules, not
another source-discovery directory. The initial unresolved import is preserved
as setup failure and is not mistaken for the subsequent lowering rejection.

Continuation: the checked backend at
78522f32576b7c14f138a875c0c572f8243dc2eb materializes literal Char/Text loop
values and bounded Boolean flags, and encodes DEX Unicode string data.
With the same Idriç revision, this unchanged consumer passes source checking,
then reaches a more precise refusal:
`DEX unpack cannot reinterpret a source object as an integer: source constructor _builtin.NIL in checked helper IB.Information.tokenize_chars`.
No source DEX is emitted. The initial Boolean flag is no longer the blocker;
the existing changing character/token accumulators need a checked collection
representation. Successful reduced loop-value tests remain separate evidence.

BLOCKED: provide a checked runtime representation for the existing source
collections/records and demonstrate this exact source-title consumer through
DEX/ART before integrating full document recovery. Java and RefC are not
fallbacks. A small text-view control is not full source-recovery acceptance.

## One-off replay against the current canonical backend

The pure operation remains `source_title : Text → Text`. Its input is decoded
source text, its output is the title selected by the existing IB recovery and
information modules, and its invariant is that Android gains no second parser
or source owner. The source file above is retained unchanged.

`replay.grease` is host orchestration rather than a new implementation of that
operation. Its inputs are explicit absolute paths to the IB repository, checked
DEX driver, matching backend modules, frontend prefix/packages, Chez executable,
frontend support directory, a fresh evidence directory, and `pure` or `managed`
entry selection. It materializes this
branch's existing modules, invokes the checked source compiler, preserves its
actual exit status and diagnostics, and requires the complete DEX artifacts
before reporting source emission. It never reports ART or phone acceptance.

The executed replay uses android-NDK
`973c24bc0b4eab6f3c88699102f7cb0e7fd8951f` and frontend
`740bbf2b1d9e811fd00c719c3473eede17ceea3b`. The canonical backend's new managed
collection path is selected for one IO entrypoint that reaches `getArgs`; the
pure export still uses ordinary checked export lowering. Collection support in
the managed path therefore does not establish support for this consumer. The
reader dependency pin must remain unchanged unless the intended consumer
actually qualifies.

A separate diagnostic, `NativeSourceProjectionCommand.idric`, keeps the same
`source_title` definition and existing IB imports, but selects one command-style
IO export. `System.getArgs` includes a synthetic program name at index zero;
the diagnostic requires exactly one following HTML source argument, calls the
unchanged projection once, and prints its result. Wrong arity prints an explicit
refusal and exits 1 through the existing checked `System.exitFailure` service.
It never parses the program name, joins arguments, or projects partial inputs.
This is deliberately a different entrypoint, required to exercise the
backend's managed path; its emission or execution cannot qualify the original
pure export or the Android Activity bridge. It introduces no HTML parser,
durable record, source model or native transport. Its effects are only argument
input and line output. The original file's `%export` is not imported into that
diagnostic; the same projection definition is an internal helper beneath the
sole IO export.

`verify-command.grease` is a host reference check, with an explicitly supplied
compiled command and a fresh evidence directory. It invokes that program with
zero, one and two source arguments and with one Unicode source, compares raw
exit statuses and independent literal outputs, and rejects any title output
from a wrong-arity call. This check qualifies source argument selection on the
host; it cannot qualify the emitted DEX runtime.

### Observed result, 2026-10-24

The canonical driver was freshly built from the exact backend revision above
using the clean frontend source and installed API at the stated revision. The
first host build stopped before consumer execution because the relocated prefix
had no Chez data directory. Supplying the existing frontend support directory
through `IDRIS2_DATA` repaired that setup; no compiler semantics changed. The
pinned Grease binary's sandbox-incompatible leak-scanner teardown required
`ASAN_OPTIONS=detect_leaks=0` for orchestration. This was not an Idriç/DEX test
or a change to its acceptance checks.

The final replay script was executed in both modes:

- **Pure:** the original file is byte-identical to its retained version. Source
  checking reaches the same unsupported `_builtin.NIL` constructor in
  `IB.Information.tokenize_chars`. The compiler returns **1**, and no source
  DEX exists. The script returns **1** and records **BLOCKED**.
- **Corrected managed diagnostic:** checked source emits **75,092 bytes** of DEX
  with SHA-256
  `9f9f845e141fe7a76b96fd91eb4d67c1e5cd7eadc18ac402b80033588e6ddf6c`.
  A repeated compilation through the final script produces identical bytes.
  Independently pinned baksmali 3.0.10 decodes those bytes successfully into
  166 methods, including the checked nonzero exit service. The checked plan
  retains the existing `recover_document_log`,
  `information_from_log`, `tokenize_chars`, and diagnostic `source_title` paths.
  These checks prove emission and parser acceptance, **not semantic execution**.

The initial command diagnostic iterated through all of `getArgs`, including the
synthetic program name. Its earlier 73,716-byte DEX emission did not satisfy the
intended one-source interface. That source and artifact identities remain
historical in the receipt, explicitly superseded by the corrected diagnostic.
The original pure-export failure remains unchanged.

The corrected source was also compiled through the same frontend's Chez host
backend as a reference execution, separate from DEX. `verify-command.grease`
observed exact statuses and outputs: zero source arguments and two source
arguments each print only the explicit refusal and exit 1; one HTML argument
prints `One document` and exits 0; one Unicode HTML argument prints `café 🦉`
and exits 0. The same checks reject the actually compiled initial all-arguments
diagnostic at the zero-argument assertion: it emits a blank title and exits 0.
This is a process-level negative control for the arity defect, not ART evidence.

The compact input/result identities and raw pure-export diagnostic are retained
in `replay-2026-10-24.tsv` and `evidence/pure-export.log`. The managed command has
not run under ART or on either physical phone; no APK was produced or installed.
The original native-reader compiler/backend pin is unchanged.

The next language boundary is now specific: provide a checked collection/record
bridge for the callable `Text → Text` export used by the Activity path, then run
this exact pure consumer under ART with title, Unicode, malformed-source, and
wrong-parser controls. Managed command emission alone cannot satisfy that test
or replace the native reader's missing acquisition/storage integration.

### Independent replay

The independent Moon worktree first replayed the unchanged pure consumer at
`c2621afe72bbe7b546c9cb32a65b7465b50ec3e0`. The actual compiler and replay each
returned 1, no DEX existed, and the raw diagnostic SHA-256 matched the retained
`6bc224e669de8544871684bda66ed1e81824dc6fbc4c4550c42cf5d2e6d0ed54`.
The original consumer and its IB information/recovery modules matched their
recorded hashes; the app dependency pin remained unchanged.

After advancing the separate worktree to the arity correction
`d3096af355a4bf455c1dcc550379b2d9d06b52dd`, Moon ran the real managed replay
with a fresh source/evidence directory and the same verified backend/frontend.
It independently reproduced the 75,092-byte DEX and SHA-256 above. The verified
baksmali JAR decoded it successfully into 166 methods. The actual native
compiler-driver `.so`, rather than only its launcher, matched
`c54218dbfb1cab6c3845cda379a387da5e66abe4cc54d044abd6019b333eccf7`.

Moon also executed the host arity checker against the separately supplied
corrected host program and observed all four controls pass. Running that same
checker against the compiled prior all-arguments program failed at the expected
zero-argument status check: the prior program exited 0 and emitted an empty
title. These independent runs verify the stated emission/decoding and bounded
host behavior; they do not execute DEX or establish ART, APK or phone behavior.
