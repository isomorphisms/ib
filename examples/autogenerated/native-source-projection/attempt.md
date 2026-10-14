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
