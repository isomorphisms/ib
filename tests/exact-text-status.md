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

These tests prove the status handling of the maintained workflow steps on a
Linux host. They do not prove Idriç compilation, exact-text semantics, Android,
or physical-device behavior. They add no compile/link stage. The existing Chez
compiler/bootstrap path has no declared ICK/NDK build-toolchain contract;
qualification or migration of that path is separate unfinished work.

The regression currently requires an available qualified Grease runtime. The
workflow still runs the real pinned compiler/oracle; it does not provision
Grease or run this process-injection harness automatically. Fixture changes are
included in the workflow path filter so they still trigger semantic acceptance.
