# Native IB reader candidates

The current candidate source is
[966a613a79b2a5800c209298800678bd984d4966](https://github.com/isomorphisms/ib/tree/966a613a79b2a5800c209298800678bd984d4966).
Its signed APK is being independently verified for direct publication. This
file will receive the immutable APK URL and hash after publication succeeds.

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

**Acceptance remains insufficient for release.** Independent current-APK
legacy decoding/TLS/ChordReader replay is pending. Native Google results and
clicked-result navigation are unqualified. Physical TAB_P10 is BLOCKED:
there is no authenticated tablet endpoint or fresh installed signer/version,
ABI/SDK/storage/Shizuku inventory. Packaged ABIs are build facts, and emulator
update acceptance does not establish physical update compatibility. No merge,
permanent release, uninstall, data deletion, signing rotation or SD migration
is performed.

The [older A734 APK](https://raw.githubusercontent.com/isomorphisms/ib/2ffb9b2bf357a36763e874b71d9571abba860433/candidates/ib-reader-a7349efc.apk)
is retained as historical evidence. It has the known Google original-octet
traversal stack overflow and does not qualify the current source.
