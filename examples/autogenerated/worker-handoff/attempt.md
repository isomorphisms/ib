# B03 phase-separated handoff

Type sketch: a committed handoff carries the exact eleven-field existing
WorkerLifecycle attachment, the executing source head and a transport digest.
Start returns only after publishing this bounded receipt without overwrite.
Observe and resume are distinct caller processes; resume means read the exact
attempt's committed result, never relaunch or decide retry policy. Signal uses
the existing provider and its B09 refusal. A digest detects accidental change;
it is not authentication against someone who can rewrite the selected file.

The new phase transport is native Grease. Existing Binder/provider programs
execute in their explicitly named Grease compatibility mode. No new worker,
supervisor, application-owned background service or canonical policy is added.

Acceptance distinguishes transport mocks, the actual fixed worker program,
compiler policy and physical service/reboot outcomes. This executor reports
`/bin/sh` PID 2 while `/proc/self/stat` reports the outer Linux PID, so the
unchanged baseline actual-worker test exits 44 at its first process-identity
read. It cannot supply actual kernel identity/reuse or caller-loss acceptance.
The bounded mock can still verify handoff parsing, exact status, refusal before
provider contact and later independent read. Physical rows remain NOT_RUN.

Known limits: receipt publication after launch has an interruption window;
same-UID concurrent directory replacement is not descriptor-confined; sync/link
has not been power-loss tested; a qualified identity-bound Android signal
primitive is absent. None is represented as a completed lifecycle outcome.
