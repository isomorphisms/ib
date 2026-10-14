# Linux native foundation attempt

Owner: SUN IB-X64-01, IB issue 139. This follows the opened FP first-wave
envelope; it is not the graphical desktop milestone.

## Type sketch before implementation

Keep the C ABI stateless: version, pointer width, and the byte length of a
borrowed UTF-8 C string. Return bounded C integers and refuse overflow. Compile
only for Linux x86-64 and reject Android explicitly, including its x86-64 ABI.
There is no tab, history, task, session, source store or navigation model in C.

The Idriç boundary owns conversion into semantic values:
- native_abi_version : IO Number
- native_pointer_bits : IO Number
- utf8_text_bytes : Text → IO (Maybe Number)

An embedded NUL cannot cross a C-string boundary intact; refuse it before FFI.
These effects are ABI qualification probes, not a source admission or decoding
implementation. SourceCoordinate remains a Unicode code-point boundary.

AI-CI owns independent golden byte lengths and reuses its 38-case source and
lifecycle oracle in the generated executable. A mutated C byte-length result
must fail the intended native case; the clean executable must then pass again.
An Android compile attempt must fail for the platform guard.

## Selected implementation and evidence

Idriç source 94dfd99bd3e376507fedc8611053b7173b2519f0, explicit Chez backend
10.4.1 from Cat Food's verified bundle. No RefC or handwritten generated-C
fallback. The new C library must compile/link with the pinned, qualified ICK
c61e448251744a2f40ad743ebef1a027bdcd2f9d compiler.

The initial compiler/support bundle has legacy producer build-toolchain debt;
using it does not retire that producer's source/bootstrap qualification. The
new consumer C stage records ICK and the native dependency hashes separately.

## Result

Linux generated-program / native FFI PASS on the declared build container. The
actual 38-case shared oracle plus eight independent ABI/UTF-8 cases executed;
compiled C mutation and missing-library controls were rejected, and clean
restoration passed. ICK's pinned producer qualified at -O0/-O2 and rejected its
division mutant. The native library is ELF 64-bit x86-64 with resolved libc.

FP runs retain exact exits and input/executable hashes. The existing Idriç/Chez
route succeeded without RefC. Native Grease source-file quoting was repaired
after an actual apostrophe-path failure. A separate compiler failure with 東京
in the build root exposed decimal escapes in compileChez and a zero compiler
exit after Error:. See https://github.com/isomorphisms/Idric/issues/138. The
recipe explicitly restricts build roots to ASCII and rejects compiler error
output; the compiled program separately passed Unicode-path relocation/launch.

The generated Scheme source, launcher and C library matched across two fresh
output builds; the compiled Chez .so hashes differed. This is not yet byte
reproducibility acceptance. Source/runtime/producer qualification remains bounded
and compiler/support archive debt is not retired. Window, input, HTTP(S), disk
restoration, worker termination and Android runtime remain NOT_RUN.
