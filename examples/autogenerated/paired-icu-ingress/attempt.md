# Paired ICU capture into the existing private Arena publisher

The requested result is an installable checked Idriç native reader for real
pages. This change addresses one necessary native ingress boundary; it does
not establish that product result.

## Values, types and invariants

The browser owns request identity and positive navigation generation. An
explicitly authorized bridge receives only a private write descriptor and the
browser-approved URL. ICU produces an observed response metadata record and
exact response bytes. The platform adapter receives their bounded `IBICU01`
frame, validates it, and uses the existing canonical appendfat API to commit
only the body bytes. Reopening must reproduce the exact body and SHA-256.

Conceptual signatures:

- receive capture: authorized request × finite byte stream → response or error
- validate response: observed metadata × exact bytes → verified capture or error
- commit response: current generation × verified capture → immutable Arena receipt
- reopen source: request × current generation → checked read-only source descriptor

The source generation passed to the publisher comes from the armed session,
not from worker-controlled metadata. A nonzero worker exit, cancelled/stale
generation, incomplete frame, malformed/duplicated/missing metadata, byte-count
mismatch, false UTF-8 claim, or wrong digest must not append or replace content.
The legacy body-only publisher still writes v1 with navigation admission NO.
No body-only path can call the v2 publisher successfully.

The existing narrow C/Binder/storage adapter remains the native implementation
boundary. No parallel browser state, parser, renderer, network client, store or
application-specific DEX is introduced. Idriç continues to own browser semantics;
the checked source-to-DEX consumer is an independent unfinished dependency.

## Implementation and effects

The existing paired ICU command is shared between the two-file adapter and a
new frame-only Grease worker. It has no app-private Arena pathname. The native
publisher validates the eight existing ICU metadata fields and full frame
length, independently checks UTF-8 and hashes, commits through the existing
appendfat library, reopens bytes, then syncs an immutable Arena v2 receipt.

Observed redirects are retained but cannot admit navigation because this ICU
capture supplies only a redirect count, not the required redirect chain.
An unobserved charset is likewise not silently treated as UTF-8.

## Verification boundary

Grease runtime: compiled Oils source 5651cf97a1b5042f24f14112a7ade9a1518eb0bc,
binary SHA-256 7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c.
The existing stdout/producer-failure/authorization/quoting controls execute with
the actual Grease entrypoint in its inherited-shell compatibility mode.

The canonical native test uses the pinned actual appendfat library and checks
the joined private-pipe → framed capture → Arena v2 → verified reader PFD path.
Its metadata/body source is synthetic. Hosted ICK compilation/execution and
ARM32/ARM64 adapter cross-compilation are required and initially PENDING.
Real Android Binder, Shizuku authorization, ICU HTTPS, checked Idriç rendering,
APK packaging/publication, and physical C67 acceptance remain NOT_RUN here.

## Remaining language and product work

The reusable DEX backend must execute canonical reader list/record/closure
operations; the known consumer failure is the changing list accumulator in
IB.Information.tokenize_chars. A host compiler or ingress test is not evidence
that this program runs under ART. Complete that backend, wire app-owned Binder
capability lifecycle and durable navigation to the existing checked UI, preserve
the installed material3 signer/package, and exercise real fetched pages before
publishing a reader APK.
