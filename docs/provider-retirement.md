# Durable provider successor

isomorphisms/ib PR #87, “Prove durable result reads through an Android provider”
(https://github.com/isomorphisms/ib/pull/87), exact head
c22b6c72d5b71ecee95680304ab73f0320a95426, is semantically superseded by
isomorphisms/ib PR #114, “Consolidate Longview through the Binder boundary with retained acceptance work”
(https://github.com/isomorphisms/ib/pull/114), implementation head
041734f9e6a520350599d900581b5ae72d1a36c6. The old head is not an ancestor:
this retirement follows file and obligation comparison.

DurableResultProvider and ResultReaderActivity have identical blobs across
these heads. DurableResultFixtureActivity only adds failure detail. The current
store generalizes immutable bounded named results, preserves legacy hello-v1
files, and adds concurrent-writer, overwrite-mutant, partial-write and
bounded-reader tests. Packaging retains the separate reader package/UID;
the provider remains nonexported, explicitly granted and read-only, with an
independent descriptor per open. The live PFD experiment remains separate.

Both branches repair the emulator runner's separate-shell PID boundary and
force a fresh late reader launch: the old branch uses clear-top; the current
branch force-stops the reader before relaunch. Current implementation run
37372692839/job/111973985861 passes replacement installation, distinct UIDs,
two complete descriptors, host termination, a late complete read through a
changed provider generation/process identity, and live PFD peer-death cases.
The overall run is FAILURE with cancelled heavy/renderer jobs, not all-gates
PASS. E2/E3 source receipts remain separately recorded historical evidence.

## Unresolved physical obligations — one live successor

Keep PR #114 draft. On MIRO A1, record independent complete descriptor reads,
provider PID/UID/generation and the separate reader UID, then late reads after
host death. Separately observe URI-grant behavior after force-stop and reboot;
no emulator result proves either on the phone. Preserve the stored bytes and
never call the provider the durable store. Live Shizuku authority remains
unverified. No physical result is claimed by retiring PR #87.
