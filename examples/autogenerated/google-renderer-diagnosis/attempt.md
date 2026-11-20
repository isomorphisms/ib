# Diagnose the actual Google renderer exception

The unchanged checked IB source at a7349efc8041b4c41ff158afca7f13079c8df1c2
qualified native Example/link/history/reflow/source copy, retained recovery,
and actual Windows-1252 and ISO-8859-1 decoding on API 29 and API 34.
Independent ChordReader 86dc5406fdceb593fa5062d4e67312e5a15b75fc
dispatch qualified the framework caller and exact once-encoded Google request.

The captured actual Google response is HTTP 200, text/html, ISO-8859-1,
one independently observed 301 redirect to www.google.com, identity content
encoding and chunked transport. ICU, private worker/PFD, publication and
native committed readback succeeded; the checked renderer callback raised
an exception on both APIs. This is a product rendering failure, not successful
search and not an observed Google consent/bot refusal. The source remains
committed but the readable navigation index stays on Reader Two.

API 29 captured 92584 bytes, SHA-256
4ec8bd0af3b94a50e6fa0fa38f20b626ee45118fb507234817cd894f855ec25a.
API 34 captured 92857 bytes, SHA-256
46339c5bf37cc7e2b36cdb95341518cb1ef2817e33ca4c575bb62ef8f6f5554b.
These values come from actual original receipts; a strict UTF-8-only
source oracle refuses the legacy charset and must not be relabeled a
passing independent legacy-source verification.

The prior JNI adapter cleared the pending exception without its type or
stack. The retained capture therefore cannot identify its cause.
This attempt adds a bounded native JNI diagnostic: Throwable -> exception
class plus at most 12 (class, method) frames -> existing recoverable UI error.
It does not read exception messages, URLs, source text or capability values.
No parser, decoder, checked DEX source or admission predicate is replaced.
The suspected cause remains unproven until the rebuilt APK reproduces it.

The original SDK carrier already selects UTF-8 only for a declared UTF-8
charset and validated original bytes. The inspected frozen source does not
contain the previously suspected charset-selection defect. A legacy-declared,
UTF-8-valid multi-octet fixture remains useful additional qualification.

This is an Android JNI adapter operation and is maintained in the existing
NDK C boundary, not a new standalone helper or alternative reader.
Host/compiler and installed API 29/API 34 qualification of this changed
native head: NOT_RUN at authoring. Actual Google reproduction: NOT_RUN.
Physical SVITOO TAB_P10: BLOCKED, no authenticated device endpoint or fresh
Cat Food identity/signer/ABI/SDK/storage/Shizuku inventory. No install on
physical devices, package change, new signing key, merge or release.
