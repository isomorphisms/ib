# IB-B06 host receipt — 2026-10-08

Implemented after B01 on source base
`5205209f189cb3813d18ec251a273d59f369e50e`. The new generic capture bounds
provider execution and stdout/stderr; existing Binder callers and fixed
Shizuku command lowering retain their interfaces. No result/attempt state
files, worker identities, Android code, or authorization logic changed.

Exact tested file SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `lib/android_shizuku_host.grease` | `61de0fd41c50997cd46c11cca7c73349c17b7deea08580498b81c6069c40e8b3` |
| `lib/bounded_process_capture.grease` | `cbb2ade5e4c08b5cf86b4d3da26aeabe567fb9401def2fb864eba9a511cd974a` |
| `tests/test_android_shizuku_capture.grease` | `24672c04fef5ddef10ca9cdbd2e9c3d47c13bafb59473cdced2648c531f1fd1d` |
| `tests/fixtures/bounded-rish.grease` | `eaf810ec653786098e191c214683cd652bb65b3f497782cdd0877ba167757e9d` |

The tested current Grease checkout is
`dilapidated-shed/grease@f19c94c6df18cddbdc1e81463e5bd689533e3c13`, with
Oils-derived source `6d29702a10ea9eb72a43950554dbcd4174d07a89`. The exact
executable was
`/workspace/scratch/68b28b5cfcf8/qualification/runtime/.build/grease/bin/grease`,
SHA-256 `eb0688fe3ddf9ecafc32de0018911a32a654264c1c86cca0b12281e710965ac8`.
This Cat Food source launcher invokes the existing Grease source using its
CPython 2.7.13 runtime; it is **not a native Grease build receipt**. Its source
entrypoint SHA-256 is
`59251609fb336e2d40150871c006f06fe9b4595016277683362b91109e3140c6`, and
the interpreter SHA-256 is
`f87a0117f28a30c72c8825cd9ec1b54d21875bfb4f987e471383d2d51417a737`.
`--version` reports Oils 0.37.0, x86_64 Linux. `timeout` and `head` are GNU
coreutils 9.4. No new compiled helper, ICK/NDK stage or device build was used.

PASS, exit 0: set `IB_GREASE_COMMAND` to that executable, then run the same
executable with `tests/test_android_shizuku_capture.grease` in default mode.
An independent reviewer repeated the suite with unchanged production hashes.
That review used test SHA-256
`7e0bee6d61cc6b8636cb552644cfe465d21e0c46b5155bdce2cb3a06b935eeaa` and
fixture SHA-256
`703b5c1b79eb10468f2fedb738fa1b7597b3d93c5aaecaf5a5dce8d37f4e20ec`;
the only subsequent test change removed each file's final blank line, after
which the implementation agent repeated the suite successfully.
Cases include successful/empty/exact-limit output; exact exit 37 with denial
stderr; provider-owned 124; independent and simultaneous stream overflow;
hangs before and after output; TERM resistance; a provider waiting for its
child; later recovery; absent runtime; invalid budgets; and absent `timeout`.
The missing-primitive case retains only `dirname` for an older launcher and
requires the capture helper's own diagnostic, so an interpreter startup failure
cannot accidentally pass.

The oversized stalled producer writes 1,024 bytes to each stream. Real capture
reports exactly 33 bytes for each 32-byte budget. Replacing the reader with an
uncapped `cat` reports 1,024 and is rejected by the same count oracle. A reader
that exits 91 before opening its FIFO returns bounded capture failure 74 under
an additional eight-second outer test deadline; no provider fixture starts and
no success bytes or temporary directory remain.

PASS, exit 0: use the same executable and `IB_GREASE_COMMAND`, with arguments
`+O ysh:all tests/test_android_shizuku_host.grease`. This explicitly disables
the inherited YSH option bundle for the pre-existing shell-compatible host
suite; the new capture subprocess still runs in default Grease mode. Identity,
process/package, signaling dispatch, Binder authority classification, denial,
and B01 status preservation controls pass. The existing fixture's `/bin/sh`
shebang is test-double debt, not the capture implementation or Grease runtime.

Cleanup evidence consists of the three direct wait completions and removed
FIFO directory, followed by a working valid operation. This environment
translates process IDs; absence of a printed PID from `/proc` or `kill -0` is
not used as proof of termination. The waiting-child case does not prove
containment of daemonizing providers. Remote worker lifetime stays unknown.

NOT_RUN: physical MIRO A1/Shizuku cancellation, native qualified Grease on
Android, Android primitive compatibility, latency and C67 behavior. Current
`.github/workflows/idric-core.yml` still invokes this host suite through `sh`;
the new helper deliberately requires qualified Grease, so this workflow needs
an explicitly qualified runtime stage before CI/merge readiness is claimed.
This receipt is host source-runtime acceptance, not whole-product or device
acceptance.
