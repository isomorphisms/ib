# Exact native APK candidate delivery

This publishes the exact signed APK from independently exercised IB source
a7349efc8041b4c41ff158afca7f13079c8df1c2, original run41294413498. It does not
rebuild, repackage, resign, install, merge or create a release. The candidate
passes both API29/API34 main reader/navigation profiles and actual checked
query controls; real ChordReader/Google results and all physical TAB_P10
acceptance remain unqualified.

The existing frozen CI archive is verified by exact length/SHA-256 and both
source-commit receipts. Android build-tools35.0.0 independently reverify the
finished package/version/minimum/target API and unchanged public test signer.
DEX and all three native ELF payloads must match the build inputs byte for byte;
the APK SHA-256 must match the original source-bound receipt. A plain TSV
records artifact origin, package identity, packaged ABIs and scoped acceptance.
Multi-ABI packaging is never a physical ABI observation.

Grease owns this artifact/OS work; no browser semantics or new small-program
fallback is introduced. The typed boundary is VerifiedExistingAPK + immutable
source receipts to ReviewableCandidateBytes + scoped receipt. The declared
Cat Food runtime supplies Grease. JSON is used only for the external GitHub
Git-data API, with fixed paths and validated hashes; credentials never become
public data or shell syntax.

The fixed delivery/ib-reader-a7349efc branch is the only publication target.
It is checked against the expected exact CI head, then updated by a non-forced
fast-forward to a direct child. A concurrent change refuses publication. The
active implementation branch, default branch, releases, signing identity and
application data cannot be changed by this path. The new commit retains the
APK and receipt under candidates/ so the exact APK can have a durable direct
download URL for review. Existing application compatibility on TAB_P10 remains
BLOCKED until fresh Cat Food/installed-signer/ABI/SDK/storage observations exist.
No installation instruction bypasses those checks.

The first publication attempt41299060008 refused before any Git write because
the observer expected the older sdkVersion label. Independent captured
badging from that exact signed APK reports minSdkVersion:'29' and
targetSdkVersion:'36'. The guard now checks the actual label while keeping
both exact required values; signer, package and byte checks are unchanged.
No candidate or packaging change was needed for this observer repair.
