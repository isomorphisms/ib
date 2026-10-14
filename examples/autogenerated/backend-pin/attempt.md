# Committed Android compiler input

Purpose: continue the Java removal without making Longview qualification
depend on an obsolete compiler hash copied into four scripts.

Domains: an absolute IB repository path, exact 40-hex IB revision, committed
Git tree entry, checked-out compiler revision, and compile/ART receipts.
The committed entry must have gitlink mode 160000 and name vendor/android-NDK.
`verified_backend_pin(repository, expected_head)` produces the declared compiler
revision only after checking the IB head, staged/worktree dependency inputs,
compiler checkout and compiler source cleanliness. It refuses mismatches.

Grease owns these Git/process operations. No parser, browser data model or
alternate language runtime is introduced. The three Longview stages and the
final acceptance gate use the same declaration; receipt rows must match it.

Acceptance: actual miniature Git repositories exercise the correct dependency,
different committed backend revision, changed source head, changed staged pin,
dirty backend source and clean recovery. These are qualification-boundary
controls; they do not establish Idriç compilation, ART or physical behavior.
The gate also refuses a regular file masquerading as the dependency gitlink.
Existing compilation and ART assertions remain.

The actual controls, Java-source boundary and vocabulary check passed through
the inherited Flexible Pipes controller. Its Python implementation remains
migration debt. An initial Grease strict-errexit rejection of command
substitution inside a conditional was corrected by binding command results
before comparison; no alternate interpreter ran the new Grease programs.
The vocabulary runner needs its explicit source-root input because FP's cwd
is the control repository. Compiler/ART requalification of the changed IB
head remains separate from these source and Git-boundary results.
