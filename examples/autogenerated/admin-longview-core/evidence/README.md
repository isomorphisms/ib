# Retained local core evidence

These are real outputs from the pinned compiler execution on 2026-11-17.
They bind the working-tree source bytes recorded by each `source.sha256` to
IB base `9f468f9b6dfba8f3895f4ab5cd7a316dc08c562e`. The base identifier is not a
claim that the uncommitted additions were already in that commit, and no final
commit, complete APK, installed Activity, ART execution or physical phone is
qualified by these receipts.

- `host/`: the second host run, with 48 assertions including hidden-task
  Activity reconstruction; actual candidate and three separately compiled
  mutation outputs, statuses and executable hashes.
- `prior-host-47/`: retained original 47-assertion output and its source/pin
  receipts, before the additional test. The production codec did not change
  between these runs.
- `dex/`: compilation of the production codec probe through the checked DEX
  backend, with the six descriptors independently extracted by baksmali.
  The DEX hash is retained; the binary and disassembly tree are not included.
- `compiler-artifacts.sha256`: actual frontend/backend launchers and Scheme
  payloads, plus the independent baksmali JAR used by these executions.

Frontend source: `740bbf2b1d9e811fd00c719c3473eede17ceea3b`.
Backend source: `153e7fc8053ba4072954a5e77d4efa3609d829e4`.
Absolute paths in copied logs and manifests identify the actual execution
directories; they have not been rewritten to suggest a different environment.

`verify.grease` and `verify-dex.grease` in the parent directory recreate the
respective checks in a fresh evidence directory using the supplied installed
toolchain. They do not download or choose replacements. The negative controls
must compile successfully and exit 1 at their named assertion; a compiler
failure is not accepted as a successful negative control.
