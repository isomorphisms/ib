# Native IB reader candidates

The current candidate source is
[966a613a79b2a5800c209298800678bd984d4966](https://github.com/isomorphisms/ib/tree/966a613a79b2a5800c209298800678bd984d4966).
[Download the exact signed APK](https://raw.githubusercontent.com/isomorphisms/ib/bb90699ed284c37127e1747f3dff250ba98b8304/candidates/ib-reader-966a613a.apk)
(10,233,767 bytes). SHA-256:
`1abdd64d0ec69933e02c61106e23ca5d9539a327104da4a612f841abd0cafd9a`.
[Publisher run41319796031](https://github.com/isomorphisms/ib/actions/runs/41319796031)
verified and copied the producer's unchanged signed bytes.

[Producer run 41315891303](https://github.com/isomorphisms/ib/actions/runs/41315891303)
passes all required stages on API29/API34: actual checked source emission,
independent DEX decode/determinism and compiled mutations; direct signed
multi-ABI packaging; installed Example Domain HTTPS through Binder/Shizuku/
Grease/ICU; committed Arena readback; actual link acquisition; Back/Forward;
exact Unicode/soft-hyphen copy; Greedy/Squared reflow; offline/update/restart;
and checksummed semantic-journal corruption recovery without refetching.

The reusable recursive compiler and exact 84007-byte source consumer also
pass both APIs in
[android-NDK run 41315882527](https://github.com/isomorphisms/android-NDK/actions/runs/41315882527).
That consumer result does not establish UI responsiveness for every body.

Package org.isomorphisms.ib.prepaint, versionCode 3 / versionName 0.3.0,
minimum Android API29, target API36. The build contains x86_64 / armeabi-v7a /
arm64-v8a. Existing public test signer:
de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9.
Publication copies the exact existing signed bytes and checks both DEX files
and all three native libraries. It does not rebuild, repackage or re-sign.

**Acceptance remains insufficient for release.** Independent
[run41319549929](https://github.com/isomorphisms/ai-ci/actions/runs/41319549929)
passes native Google partial-static presentation and genuine ChordReader
handoff on API29/API34, including committed original-byte readback, TLS
certificate refusal and semantic-journal recovery. The API29 legacy-decoding
profile passes. The API34 decoding profile fails actual Binder-death state;
[run41324114821](https://github.com/isomorphisms/ai-ci/actions/runs/41324114821)
reproduces that older-APK race on both APIs before stronger result-click checks.
IB fb9b34a9 repairs dead-RPC classification and preserves the concurrent
Unicode-break repair; its fresh qualification is pending. This APK predates
both changes. Native search-result click/retention remains unqualified. Physical TAB_P10 is BLOCKED:
there is no authenticated tablet endpoint or fresh installed signer/version,
ABI/SDK/storage/Shizuku inventory. Packaged ABIs are build facts, and emulator
update acceptance does not establish physical update compatibility. No merge,
permanent release, uninstall, data deletion, signing rotation or SD migration
is performed.

The [older A734 APK](https://raw.githubusercontent.com/isomorphisms/ib/2ffb9b2bf357a36763e874b71d9571abba860433/candidates/ib-reader-a7349efc.apk)
is retained as historical evidence. It has the known Google original-octet
traversal stack overflow and does not qualify the current source.
