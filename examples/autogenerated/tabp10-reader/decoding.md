# Committed original bytes and decoded derivatives

The installed reader previously could not consume the maintained Windows-1252
decoder: its source owner refused every non-UTF-8 receipt before decoding.
The new decoder-input entrypoint reuses that owner's single receipt parser,
canonical range reader, immutable generation checks, double digest and bounded
anonymous pipe. It is explicitly a raw snapshot contract. The existing admitted
UTF-8 APIs retain their stricter contract. No v1/unobserved body receipt enters
either path, and an unproven redirect cannot acquire decoder authority.

The Android adapter preserves non-UTF-8 octets through the SDK ISO-8859-1 String
mapping. That String is a one-byte/one-scalar carrier. Checked Idriç refuses
scalars above 255, applies the existing WHATWG decoder, checks the original
byte count against the independently reopened count, and checks source identity,
digest, MIME, status, URLs and completeness before extraction. UTF-8 uses the
SDK's reporting decoder rather than silent replacement.

`HtmlPrepaintResult.evidence` remains the original response evidence. Its
optional decoding record names the derivative; the Strand uses DerivedSource
and a source reference containing the original key/generation/digest plus the
decoder identity. Original charset and invalid-UTF-8 observations are never
rewritten into fabricated UTF-8 admission. Selection coordinates belong to
the decoded semantic blocks, separate from HTML bytes and painted wrapping.

`fixtures/windows-1252.html` is 35 original octets, SHA-256
`f75de31144f8a4bcd321f0d54f8298fc7d429c8a9213c45c86e1abfc29f01a54`.
The ReaderProjection export observes the real committed-document preparation
and selection owners. The independent ai-ci observer supplies literal expected
metadata, derivative identity and selected text. Compiled mutations bypass
source admission, change the Euro mapping, bypass the original count, or allow
scalar truncation; each must fail its designated assertion under ART.

An initial direct compile attempt refused because NativeReader was outside the
declared source directory; no DEX was emitted. The maintained full consumer
build correctly stages the existing source tree and compiled the new record,
gate and decoder operations without source-semantic workarounds. No alternate
language or decoder implementation was needed. Host canonical publication and
raw readback tests pass, including duplicate charset, changed committed byte
and stale generation refusals. Fresh exact-source ART and Android pipeline
qualification remains required; physical TAB_P10 access is unavailable.
