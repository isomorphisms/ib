# E4: bounded source-backed Longview result

This is a **backend acceptance slice**, not a new browser, Android Activity,
Cauldron store, or complete reading UI. It intentionally changes no Java,
Gradle, Manifest, package, signer, or live Android application behavior.
Idric continues to own admission through IB.StorageAdmission, worker authority
through IB.WorkerLifecycle, and task identity through IB.LongViewTask.

## Existing handoff

1. A trusted, explicitly ordinary (noncredential) acquisition supplies a
   selected regular source file, a stable source identifier and the **expected**
   SHA-256 from its retained-source provenance. The Grease composer does not
   fetch pages or infer a privacy classification from the URL or file contents.
2. E3 supplies a completed, attempt-bound worker receipt that **already
   binds** the selected source identity, SHA-256, byte count, kind and ordinary
   classification as five distinct source_binding fields. The adapter refuses
   missing, conflicting or duplicated bindings as well as symlink sources,
   source hash mismatches, private classification, unsupported formats,
   empty/oversized sources, and a preexisting output candidate. This binds the
   bytes to an existing completion, but does not prove a real Shizuku worker
   generated the receipt or that a Cauldron generation authorized its privacy.
3. lib/longview_source_result.grease appends source kind, source identity,
   classification, measured SHA-256, measured byte count and bounded base64
   payload to that exact E3 completion. It verifies the composed content by
   decoding **before** exposing a candidate. The small base64 envelope is a
   result transport format; it does not replace canonical Cauldron bytes.
4. The same full completion is passed through
   ib_publish_owned_admitted_result with the preexisting Idric
   publication_for decision (source-action emits a **4096-byte admitted
   fixture action**). The existing E3 attempt match and E2 immutable
   publication remain mandatory. This adapter does not admit or reserve space.
5. Any later process may independently open the immutable result using
   ib_store_read_bounded and decode it. The reader verifies record shape,
   source identity, classification, byte count and SHA-256 *before emitting
   any bytes*. It also checks all five original worker source bindings against
   the recovered content. A renderer is not required for the read.

The first source selection is deliberately limited to 1–2048 bytes of
text/html or text/plain, an ordinary classification, and a 4096-byte result
envelope. Content requiring a larger bound belongs in Cauldron's larger
source-intake and projection path rather than silently growing this worker
fixture. The test uses the exact 1154-byte HTML fixture already committed
under tests/fixtures/arxiv_site/html/2203.11355. It checks the real heading
and abstract bytes, not a constant receipt label.

## Acceptance and negative controls

tests/test_longview_source_result.grease exercises the **compiled**
ResultPublicationAcceptance program's 4096-byte action, immutable publication,
two fresh late readers after the original temporary source/receipt files are
removed, exact source byte comparison, altered digest/encoding/count/privacy
refusal without partial stdout, incorrect expected hash, linked input,
oversized input and stale-worker refusal. Existing E2 and E3 tests remain
independent and unchanged.

Hosted tests cannot prove that acquired source bytes were actually obtained
through the live trusted worker, that a real Cauldron source descriptor supplied
the expected SHA-256 and privacy decision, or that the Android app private
provider displays this content. This does not claim login/authenticated-page
continuity, live renderer/host death on physical Android, CPU/RAM measurements,
native Grease on Android, Shizuku authority, an installed APK or a UI
presentation. Current source fixtures may contain HTML/JS text; the decoder
never executes that HTML.

An end-to-end E4 follow-up must bind a verified Cauldron source generation and
Idric-approved admission to a bounded actual acquisition, then expose the
recovered projection in a browser-owned view; preserve source provenance
without putting credentials, query-bearing URLs or arbitrary private page
contents in ordinary task records. It must also reconcile the **Idric-only
Android application path** from PR #134; the older Java fixture is not accepted
as the chosen browser implementation. Physical MIRO evidence remains distinct.

**Evidence at initial authoring:** source and tests committed; execution and
source-to-device acceptance are NOT_RUN until a fresh exact-head CI receipt
exists. Do not cite ancestor green runs as acceptance of this new slice.
