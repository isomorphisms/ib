# IB C67: private Binder/PFD reader handoff (bounded receiver)

**Status:** native Binder provider/receiver, verified Arena source-owner callback and app-host assembly, with hosted fixture tests. No phone APK.
This branch is stacked on IB PR #152, which stores checked ICU body bytes in
the existing userspace appendfat Arena. **No URL-capable APK, provider service,
Idriç screen connection, or physical C67 Binder execution is claimed.**

## Required connection

    Idriç browser-owned navigation epoch + source identity
       → Shizuku-authorized ICU worker (no Android-reader HTTP)
       → exact captured bytes → checked appendfat Arena source
       → source-owner checks admitted v2 receipt + matching navigation epoch
       → independently reopens and hashes committed bytes
       → private app-scoped Binder provider: OPEN_COMMITTED_SOURCE(request,epoch)
       → one read-only, per-open ParcelFileDescriptor (not Arena file FD)
       → native reader verifies exact stream length + SHA-256 + epoch
       → source-linked extraction / Idriç Prepaint → existing native views

The eventual app-scoped source owner must first consume the **real** committed source using
bin/ib_android_arena_read.grease (which does not emit bytes until its
digest check passes). Materialize the verified bounded source into a
fresh **read-only anonymous pipe** whose descriptor, rather than a path, is transferred. A backing Arena file descriptor, writable descriptor or socket is refused.
Never hand out the appendfat Arena's descriptor or rely on a shared
/data/local/tmp response. Never give the reader a network client.

Private Binder implementation contract, transaction code **1**:

- Request parcel: validated UTF-8 request ID and signed 64-bit navigation
  generation. A real provider authorizes Binder calling UID against an
  explicit same-signer/allowed-reader identity before opening a source.
  Missing/denied authorization returns a nonzero status and no descriptor.
- Reply parcel: int32 status **0**, source byte_count (int32), 64-character
  lowercase source SHA-256, then a ParcelFileDescriptor with **only**
  the already checked committed source's bytes. Provider closes its write
  side after the declared byte count. A matching, admitted v2 receipt is necessary but does not
  replace the browser's navigation authorization.
- Browser-held admission ticket: request ID, generation, byte count
  (1–4194304) and lowercase SHA-256 copied from the previously checked
  immutable source receipt, **never from untrusted page content**. The
  ticket must be authenticated at the broker/reader boundary. The client
  also rejects any length or SHA difference between its browser-held ticket
  and the provider's receipt-derived reply metadata before accepting the FD.
  For a
  matching ticket, the receiver verifies the entire stream, checks for EOF,
  and compares SHA-256 before replacing the last displayed source.
- A newer browser navigation updates the expected generation. Late results,
  corrupt/replayed metadata, malformed IDs, extra/short input, over-limit
  bodies, reader/provider death and stalled streams are refused. The
  existing last-good snapshot remains owned by the reader.

native/android/ib_arena_pfd_provider.{h,c} implement a narrowly UID-gated
**serving** Binder object (the host supplies its verified-source callback).
The object must be handed privately to the checked reader by the app host:
no global Binder service registration is used here. The callback is now implemented in native/android/ib_arena_source_owner.{h,c}:
it opens the app's existing 0700 Arena root, reads an immutable source receipt,
checks canonical .used and locked range, independently rehashes the complete
body, and emits an anonymous read-only pipe from a bounded writer thread.
It never returns the Arena's file descriptor. Host assembly wiring in
native/android/ib_reader_host_assembly.{h,c} binds that callback to the
private Binder provider object. **The app lifecycle has not yet instantiated
this assembly, and no Binder reference has been delivered to the Idriç DEX.**

native/android/ib_reader_pfd.{h,c} implement the **receiving** C11
boundary. On Android API 29+ they use NDK Binder parcel transactions and
AParcel_readParcelFileDescriptor. The consumer does not discover a global
service, request Shizuku permission, decide URLs, parse HTML, or publish
history. It requires an already authorized provider Binder handle supplied
by the host adapter. This is compatible with Android 14/SDK 34 on A1/C67;
older Android API 28 is not yet supported by this native Binder adapter.

This is **not** a second canonical store. The C receiver holds one
replaceable in-memory candidate. The browser-owned committed source and
navigation journal continue to reside in existing IB/Cauldron/Arena.
Raw bytes remain binary until source verification and the existing Idriç
extraction/Prepaint decision. It is not correct to render raw HTML as if it
were already the semantic prepaint projection.

## Source and validation

test_android_reader_pfd.c uses two real Linux processes and transfers an
anonymous pipe descriptor via SCM_RIGHTS. This checks the exact native
consumer's binary fidelity, known SHA-256 vectors, good-source retention,
and fail-closed refusal on corruption, short/extra data, bad IDs,
over-limit tickets and stale navigation generations. **SCM_RIGHTS is a
host stand-in for descriptor ownership, not a claim of tested Android
Binder delivery or tested cross-UID Android authority.**

The second host test constructs a real pinned sd-card-append-fat Arena,
appends an admitted UTF-8 HTML fixture using its canonical append command,
publishes a complete v2 receipt and a legacy v1 refusal fixture, then drives the actual C source owner and reader
verifiers end-to-end. It mutates receipts, source bytes, commit pointer,
generation, root permissions and path identity, requiring refusal and
last-good preservation. This is still Linux-hosted source admission.

The independent Android CI lane cross-compiles actual NDK Binder calls,
source-owner assembly and pipe writer for ARM32 and ARM64 and retains digests. Cross-compilation does
**not** prove the provider exists, the Idriç DEX can call this library,
or that an Android Binder transaction has happened.

## Required next integration

1. Instantiate the newly connected native host assembly in the existing
   Android application lifecycle, resolve the real reader UID from trusted
   package identity and privately supply its Binder handle to the checked
   Idriç DEX. The assembly cannot use root paths owned by Termux or shell:
   add the separate authorized Shizuku-worker-to-app-private-Arena ingress
   through scoped IPC. Shell UID 2000 cannot read the app's private Arena.
2. Bind the native C receiver to the **existing checked Idriç Material
   reader** (#129/#147), with qualified DEX/native FFI and lifecycle.
   Do not introduce Java/Kotlin application logic or a replacement UI.
3. The native source owner now requires v2 admission, observed HTTP status,
   resolved URL, MIME, charset and a source-generation/epoch match.
   IB #152 host tests passed, but real C67 ICU/TLS remains NOT_RUN.
   Navigation ordering and link events remain owned by Idriç.
4. Device-check denied/missing Shizuku, private reader UID, real ICU
   HTTPS, committed Arena and readback digest, actual Binder PFD
   transaction, page presentation, copying text, replacement/restart
   recovery and late/out-of-order completions on the physical C67.
   Mark each absent test NOT_RUN.

Existing source owners: IB #151, #142, #141, #97; IB PR #112
(Binder-first host), #129/#147 (checked Android reader), #152
(ICU/Arena). ICU PR #35 owns real resolved-URL/redirect metadata.

## ICU #35 / IB #152 v2 admission invariant

Native PFD publication requires exactly 23 receipt fields, schema
`ib-icu-arena-v2`, `source_key=arena:REQUEST_ID`, source_generation
matching the requested epoch, observed HTTP 200, `text/html`, explicit
`utf-8` charset, identity content encoding, ICU UTF-8 validity,
`acquisition_complete=YES`, `reopen_check=MATCH` and
`navigation_admitted=YES`. Reopened canonical Arena body length and SHA-256
must match. v1 UNOBSERVED receipts are always refused by this native source
owner, including when their body digests are correct.

These changes are host candidates until exact-head CI and independent AI-CI
acceptance. The host fixture is not live ICU on C67; physical Shizuku,
cross-UID worker ingress, Binder/Idriç DEX registration and UI remain NOT_RUN.
