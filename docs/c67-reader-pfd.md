# IB C67: private Binder/PFD reader handoff (bounded receiver)

**Status:** reader-side native receiver and independent host simulation only.
This branch is stacked on IB PR #152, which stores checked ICU body bytes in
the existing userspace appendfat Arena. **No URL-capable APK, provider service,
Idriç screen connection, or physical C67 Binder execution is claimed.**

## Required connection

    Idriç browser-owned navigation epoch + source identity
       → Shizuku-authorized ICU worker (no Android-reader HTTP)
       → exact captured bytes → checked appendfat Arena source
       → source-owner verifies immutable receipt + reopened bytes
       → private app-scoped Binder provider: OPEN_COMMITTED_SOURCE(request,epoch)
       → one read-only, per-open ParcelFileDescriptor (not Arena file FD)
       → native reader verifies exact stream length + SHA-256 + epoch
       → source-linked extraction / Idriç Prepaint → existing native views

The supplier must first consume the **real** committed source using
bin/ib_android_arena_read.grease (which does not emit bytes until its
digest check passes). Materialize the verified bounded source into a
fresh pipe/memfd whose descriptor, rather than a path, is transferred.
Never hand out the appendfat Arena's descriptor or rely on a shared
/data/local/tmp response. Never give the reader a network client.

Private Binder implementation contract, transaction code **1**:

- Request parcel: validated UTF-8 request ID and signed 64-bit navigation
  generation. A real provider authorizes Binder calling UID against an
  explicit same-signer/allowed-reader identity before opening a source.
  Missing/denied authorization returns a nonzero status and no descriptor.
- Reply parcel: int32 status **0** then a ParcelFileDescriptor with **only**
  the already checked committed source's bytes. Provider closes its write
  side after the declared byte count. A successful receipt alone does not
  admit browser navigation.
- Browser-held admission ticket: request ID, generation, byte count
  (1–4194304) and lowercase SHA-256 copied from the previously checked
  immutable source receipt, **never from untrusted page content**. The
  ticket must be authenticated at the broker/reader boundary. For a
  matching ticket, the receiver verifies the entire stream, checks for EOF,
  and compares SHA-256 before replacing the last displayed source.
- A newer browser navigation updates the expected generation. Late results,
  corrupt/replayed metadata, malformed IDs, extra/short input, over-limit
  bodies, reader/provider death and stalled streams are refused. The
  existing last-good snapshot remains owned by the reader.

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

The independent Android CI lane cross-compiles the actual NDK Binder
calls for ARM32 and ARM64 and retains digests. Cross-compilation does
**not** prove the provider exists, the Idriç DEX can call this library,
or that an Android Binder transaction has happened.

## Required next integration

1. Implement the existing Binder host provider's private authenticated
   descriptor operation, creating a fresh bounded FD only after the
   canonical Arena receipt checks. Handle shell UID 2000 and app-private
   storage correctly; do not assume they share a directory or UID.
2. Bind the native C receiver to the **existing checked Idriç Material
   reader** (#129/#147), with qualified DEX/native FFI and lifecycle.
   Do not introduce Java/Kotlin application logic or a replacement UI.
3. Admit a page only when ICU's resolved URL, status, encoding and
   redirect evidence are available; retain the source and link events
   through Idriç. At present PR #152 says navigation_admitted=NO.
4. Device-check denied/missing Shizuku, private reader UID, real ICU
   HTTPS, committed Arena and readback digest, actual Binder PFD
   transaction, page presentation, copying text, replacement/restart
   recovery and late/out-of-order completions on the physical C67.
   Mark each absent test NOT_RUN.

Existing source owners: IB #151, #142, #141, #97; IB PR #112
(Binder-first host), #129/#147 (checked Android reader), #152
(ICU/Arena). ICU PR #35 owns real resolved-URL/redirect metadata.
