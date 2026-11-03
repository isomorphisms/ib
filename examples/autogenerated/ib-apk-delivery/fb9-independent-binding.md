# Exact repaired APK with independent archive binding

The publisher consumes immutable FB9 producer archive13092196333
(921674882 bytes, SHA2560b20640e265b5f76114f0d9f4db09bd15f67c8b2951059d14ef47b8dce7f3f53),
requires both application-source receipts, compiler6e1 and frontend740,
verifies package/version/public test certificate, and byte-compares both DEX
and all three ELF payloads with the original build receipt.

It also verifies the exact four independent EDB archives from41328116370:
both decoder positives and the caller/query/original-byte handoff subprofiles
in the two failed stronger search jobs. Their source receipts, observer head,
API, dead-RPC mutation controls, real TLS and semantic-journal controls, and
candidate APK digest must agree. Search failure is retained as FAIL: Google's
redirect-only partial body has no qualified public result. Downloading a
passing subreceipt from a failed broader search job never makes that job PASS.

Only unchanged signed APK bytes and their scoped receipt are written under a
new FB9 filename on the existing delivery branch. Older artifacts remain.
No APK rebuild, packaging, signing, install, release or merge occurs here.
Physical TAB_P10 and update/signing compatibility remain BLOCKED/NOT_RUN.
Status at authoring: repaired publication NOT_RUN.
