# Exact-text producer status regression

The exact-text workflow must require both a successful producer and all twelve
semantic comparisons. A successful `tee` cannot establish producer success.
Both maintained steps now redirect output directly to their log and display it
only after the producer succeeds. The existing GitHub Actions implicit Linux
runner (`bash -e`) stops on a failed compiler or oracle before later checks.
This repairs the existing runner boundary; the new test orchestration and
process fixtures are Grease.

Run `grease /absolute/path/to/ib/tests/exact-text-status.grease` with a qualified
Grease executable on `PATH`. The test locates IB from its own source path,
extracts the actual two workflow bodies, and executes them using that same
Actions shell boundary. It changes only absolute compiler/log paths, so each
case has isolated files. The optional argument selects another workflow file
for regression mutation testing.

The process fixture is an explicit test double, not the Idriç compiler or an
implementation of exact-text semantics. It supplies controlled status/output
at the process boundary. Its successful compiler action writes a plausible
executable and log; the failing compiler does those same writes then exits 7.
The oracle controls are correct output/status 0, incorrect output/status 0,
and all twelve correct lines/status 7. The test requires exact refusal statuses
and checks the plausible artifacts, so a launch error cannot satisfy a negative
case. Restoring the old workflow must fail this regression on both exit-7 cases.

## Evidence boundary

These process fixtures prove status handling on the Linux host.

The workflow builds the scoped native Grease runtime through ai-ci's pinned
source action and runs the four process controls first. It retains the Grease
producer receipt. The real Idriç compiler and twelve semantic comparisons
run in the separate `oracle` job after the controls pass. Fixture changes still
trigger both jobs. See [the provisioning record](exact-text-status-provisioning.md).
