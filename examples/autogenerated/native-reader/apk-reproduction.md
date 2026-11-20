# Repeat the actual application APK with normalized direct packaging

The original independent packaging test at ai-ci48065212/run41365426201 observed
different signed bytes from identical D02 inputs under android-NDK9861.
android-NDK153e7fc8053ba4072954a5e77d4efa3609d829e4 repairs only owned ZIP staging:
fixed UTC timestamps, excluded host extra fields and preserved caller modes.
The maintained source/compiler, Activity contracts and existing public test
certificate are retained.

This source pins that exact producer and observer
cef5ae8eb891c97dba81cd7bfc6f36edebb44c53, which includes the original-producer
nondeterminism counterexample and input metadata preservation checks. Its native
fragment observer also requires a new ICU source identity and two newly
committed responses after the frame-timed early-link gesture.

Type sketch: repeated checked Activity/platform/ICU DEX + repeated ICK/NDK
application ELF for three ABIs + the same qualified ICU/Grease native dependency
bytes + fixed manifest/resources/key -> byte-identical signed APK. The existing
maintained producers perform both builds; no alternate packager, generated Java,
Kotlin, candidate smali or duplicate interpreter is introduced.

The build now recompiles the actual application native source into fresh
directories and compares every application ELF before comparing the final signed
APK. The Activity and platform DEX were already repeated; the ICU managed worker
DEX is now also repeated. Native ICU/Grease dependencies are explicitly reused
unchanged hashed inputs, so this is not a second build of those dependencies or
of the compiler/bootstrap toolchain. Those limits are stated in reproduction.tsv.

All positive results at this new integration head are pending. Qualification of
backend9861 or IB0d2 is historical until fresh suites and actual ART execution
succeed. No physical hardware, installed signer, ABI or update compatibility is
inferred from these source changes.
