# Program attempt: artifact inspection status

Status: PASS for inspector process/publication behavior; native view BLOCKED

## Purpose and type sketch

IB-D01 must refuse qualification after any failed artifact inspection. The
existing inspector publishes `inspection.tsv` only after all inputs pass, but
leaves a previous run's TSV present when a later input fails. A caller checking
file presence can mistake stale evidence for the current attempt.

Inputs are an absolute inspector source path, an absolute verified Grease
executable, and a fresh absolute disposable directory. The fixture supplies a
single synthetic POM with pinned bytes; it tests the real inspector's execution
and result publication, not Android dependency compatibility or native views.

`inspect : Manifest × Cache → ExitStatus × Maybe CurrentInspection`

The positive run must publish the inspected input digest. A corrupt cached
input and an unavailable download must return a nonzero status and leave no
current inspection. Re-running valid inputs must recover. Filesystem setup,
process execution, hashing and byte comparison are operating-system actions;
the browser's semantic policy is not reimplemented here.

## Language and runtime boundary

Idriç is not available in this executor: neither `idric` nor `idris2` resolves,
and the recorded October 8 compiler path no longer exists. This fixture is
shell/process orchestration, so canonical source is Grease under the user's
Grease rule. No Python, JavaScript, Bash implementation, new source build or
alternative Idriç backend is used.

Run with the exact maintained Grease implementation and pass its absolute
executable as the second argument. The fixture copies the inspector body
unchanged beside a synthetic one-entry manifest so dependency downloads and
compiler qualification stay separate.

## Expected known-bad case

Removing only the start-of-attempt invalidation must fail specifically with
`FAIL: corrupt input retained stale success`. Comparing the process status
alone would miss this evidence-lifetime defect.

## Result and language work

Execution results, runtime digest and commands are recorded in the qualification
receipt after the actual controls run. Restoring a qualified Idriç executable
is necessary for D-family semantic work; this Grease result supplies no Idriç,
Android, UI, APK or physical-device acceptance.

The actual old inspector at `be731c7916e9489a03cf6cd5ea9328763faaee96`
failed this regression with exit 1 and the exact intended stale-success
diagnostic. The corrected inspector passed all four controls. Producer exits
were 0, 1, 0 and 37 respectively. See
`../../../qualification/android-view/inspection-status-receipt.md`.
