# Native reader text continuation — October 13, 2026

Status: **PARTIAL / BLOCKED_NATIVE_VIEW**. This implements the decoded-text
adapter in `src/IB/Android/ReaderText.idric`. It has checked-source and fresh
DEX evidence, not an accepted Material 3 screen or installable update.

## Change and ownership

`present_reader_text` accepts an existing disposable TextView and already
decoded Text. It disables single-line mode and horizontal scrolling, enables
selection, converts the complete platform string to CharSequence, and calls
the real TextView.setText. It adds no source parser, browser model, store,
navigation identity, or source-coordinate conversion. Application code must
supply the view and source-backed projection on Android's UI thread.

The qualification exports import the production module. Their independent
smali observer constructs a framework TextView, begins with unsuitable
single-line/nonselectable state, checks exact Unicode/whitespace and empty
replacement, and measures the same text at widths of 240 and 720 pixels.
Those widths are observer inputs rather than product dimensions. Candidate
text behavior is produced by Idriç, not by that observer.

## Executed evidence

Host: Ubuntu 24.04.3, Linux 6.18.44, x86_64. Backend source:
`isomorphisms/android-NDK@b7a637fdbf3e3725c8113bf665333215d569ceb2`.
Existing maintained compiler executable reports `0.8.0-2734ac5a8`, source
`isomorphisms/Idric@2734ac5a86610eddfab73b1644d72b4ac85a65ee`.
Tracked compiler source was clean; its untracked build/launcher paths are not
source qualification. The existing checked DEX driver was built from the
pinned backend against that compiler API. The inherited Chez compiler/driver
bootstrap is host-tool debt, not an ICK/NDK-qualified application producer.
No new native compile/link stage, Java application compiler, Gradle, or RefC
path ran.

| Boundary | Actual result |
| --- | --- |
| Idriç checking and checked ANF | PASS; both the production module and importing qualification exports compile |
| Original DEX emission | PASS; 1,028 bytes |
| Independent DEX decoding | PASS; baksmali 3.0.10 decodes real TextView setters/getter, String.subSequence and CharSequence.toString |
| Text/wrapping/selection production mutants | All compile and emit distinct DEX; runtime rejection remains NOT_RUN |
| Clean recovery | PASS; byte-identical to original DEX |
| Independent Android observer | Assembled successfully with smali 3.0.10; ART execution NOT_RUN |
| Public Idriç vocabulary | PASS |
| Material theme/resources and visible screen | NOT_RUN |
| Activity/view creation, lifecycle, retained-source/store input, actions | INCOMPLETE |
| APK, signing/update, physical A1/C67/tablet | NOT_RUN |

Digests:

| Input/output | SHA-256 |
| --- | --- |
| Production ReaderText.idric | `49b7607b8278434d89d1e0f0f49d86e072d278a6c9ac8929b0d93ba30543c4de` |
| Importing qualification source | `c882f13c64e3887205c18eea6364b40e1ffae819f6a0a0c1abf28ebc4484fc51` |
| Original/recovery DEX | `89e77bf9dc70947c74dc5801cc9158e5476dec70b5785bc60ca783b31f70e9d5` |
| Empty-text mutant DEX | `9473a1d74c2d30e6f0ed95cb521a9fc54ad1768dbbc08946728844e18ec45084` |
| Horizontal-scrolling mutant DEX | `90f5f558982254a2f888f4b9fe8d25fb194a0b1dea8bc4c76393f4f42020f5a0` |
| Nonselectable mutant DEX | `b21fde84ee13a4e0fc1c5dd6ef7aa540130bc6b0976992c5f2c9e85d2b6f4478` |
| Observer source | `83c98545765029fe1387e5cc01b6d44593722df75fda38c2b5abd67908d32951` |
| Assembled observer | `b43e9ecbc411be3bf465bf22cdde6cf9dc592350076e83880f5bb2156b5b2e98` |
| Checked DEX driver's compiled host program | `71f826e27bbfc9487025ff65e0c22ab40c50d55291f3b7494d518bf05056fde7` |
| baksmali external decoder | `37ae4a41a8886e15c20b8362fa4250f96bbdb55e1a608199ad8b5dff068b588f` |
| smali observer assembler | `32fa0e88a6c397b3922201adf5f3e534fbaed5a663c71d0c558c3ddce0af844a` |

Grease implementation source remains `5651cf97a1b5042f24f14112a7ade9a1518eb0bc`,
ELF SHA-256 `7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`.
Its verified consumer entrypoint ran `verify.grease`. The existing
`ASAN_OPTIONS=detect_leaks=0` accommodation avoids LeakSanitizer's inability
to inspect this ptraced host; it does not turn any compiler/observer failure
into success. `verify-art.grease` is prepared but was not executed.

## Execution origin and failed attempts

Flexible Pipes main `7dc68ca3d4ee49511e73b39f618a6f32da206bf7` opened the
data/assignment envelope before the implementation. Its existing runner is
explicit legacy Python debt; no new Python program was authored. Envelope
digest: `44f1812f93a25b6e5a9fb3df1d5b9d2d0303d4119b295d9225cff89b5cd1549d`.
The first source pipeline executed its emission stage successfully but failed
the existing vocabulary script because that script expected the IB working
directory. Its failed receipt remains retained. The next pipeline supplied
the observed absolute IB directory and passed all three stages. Production
module extraction was then checked again against fresh isolated source copies.
This is an authoring/source-check continuation, not independently promoted FP
execution or artifact-publication authority.

An initial driver-build invocation lacked the compiler's `network` package
and failed with `Module Network.Socket.Raw not found`. The next invocation
selected the existing package explicitly (`-p network`) and built the driver.
That failure is not an Idriç reader-source defect.

The successful driver build used the existing compiler's `--cg chez`,
`-p network`, pinned backend `dex/idric/src/Backend/DEX/Main.idr`, and isolated
`backend/build` / `backend/exec` outputs. Source checking ran
`grease examples/autogenerated/material-reader-text/verify.grease` with six
absolute arguments: this IB checkout, the observed compiler checkout, that
driver executable, its `backend/build/ttc`, the digest-verified baksmali jar,
and a fresh evidence directory. Grease parsing of both verifiers passed;
the ART verifier requires four absolute arguments and an observed
Android 14/x86_64 `emulator-5554`, refuses stale inputs/evidence and checks
both received candidate and observer bytes before execution.

## First remaining product boundary

The checked backend's source foreign-call surface admits static, virtual and
interface calls. It explicitly refuses constructor names. Its generated
callback has a specific Object-initializing constructor; that does not supply
source-level TextView/Material factory calls or an Activity subclass/lifecycle.
Application-owned screen construction and Material resources still need the
existing compiler/platform owner or a separately qualified narrow native
adapter. This continuation changes neither architecture nor toolkit selection.

The source text operation no longer needs a hypothetical generic String-to-
CharSequence cast: the real Android interface is reached through the supported
String.subSequence return. The next acceptance runs `verify-art.grease` on the
declared Android 14/x86_64 emulator and requires original/recovery success plus
each production mutant's specific failure. Only then may text/selection/reflow
behavior be claimed. A full D01 screen additionally needs real Material
resource application, construction/lifecycle, source-backed action input and
the separate package/device receipts. No Director link was opened or reflowed
on a physical phone during this continuation.
