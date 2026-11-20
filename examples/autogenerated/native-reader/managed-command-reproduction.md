# Preserve managed-command input identity during reproduction

IB dce78140c79d43674fdc2300b0a0852334b31450/run41367335129 failed the new
worker DEX comparator (exit1, byte9) before native repetition or ART. Its frozen
failed source archive13108753418 is469333550 bytes, SHA256
0184dffe97bafb6508095682edcaccf62ac3010627da9f1863e3ef5d5edf01c1.
The first signed APK packaged successfully; the incomplete producer is not
qualified and is not promoted.

The repeat check changed the output command name from icu-worker to
icu-worker-repeat. The qualified compiler's Codegen.managed_plan passes
requested_output_name into RuntimeArgs.with_runtime_arguments; runtime_argument
emits that name as the zeroth getArgs element. That is an observable command
input, unlike the pure-export Activity's output filename. Removing the command
name from generated DEX or forcing Activity and command contracts together would
change supported semantics.

This repair compiles the same ICU source and the same icu-worker command name
into fresh build/output directories. The original byte comparator remains
mandatory; no DEX normalization, binary patch, compiler refusal weakening or
cached DEX replacement is introduced. The existing Activity/platform repetition,
application ICK/NDK rebuild and final signed APK comparator remain mandatory.
Independent baksmali inspection of both failed outputs is being retained under
ai-ci, separate from any later positive build.

Type sketch: source + command name + compiler/pins + build flags are semantic
inputs. A fresh owned output directory is the reproduction destination. Equal
command inputs must produce equal DEX and a byte-identical application APK.
Native ICU/Grease dependencies remain explicit unchanged hashed inputs.
Results at this changed source are pending; physical TAB_P10 stays NOT_RUN.
