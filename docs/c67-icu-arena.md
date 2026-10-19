# C67 ICU #35 → Shizuku → Arena integration candidate

Status: **host-contract implementation; physical ICU/Shizuku/reader acceptance NOT_RUN**.
This is not a C67 URL-capable release or a claim that ICU #35 is merged.

## Acquisition and provenance

The pinned candidate API is [dilapidated-shed/icu PR #35](https://github.com/dilapidated-shed/icu/pull/35),
head commit 0da041fbe906657f4f705da45a842d96832e5ee1 at integration.
Do **not** substitute older ICU DEX/native objects: old versions write only
body stdout, not the paired ICU_CAPTURE_BODY_PATH and
ICU_CAPTURE_METADATA_PATH files required here.

    browser-owned request (future scoped Binder/PFD handoff)
      → IB Binder-shaped ICU capability (current Shizuku rish lowering)
      → verify deployed ICU DEX + native TLS .so against explicit SHA-256 pins
      → one Idric.Generated get via Android app_process
      → ICU #35 creates private paired body + metadata under the shell UID
      → single bounded, length-framed rish stdout transfer
      → independently validate metadata shape and captured byte count
      → canonical preinitialized appendfat userspace Arena commit
      → native C reopen + compare exact range with staged captured body
      → immutable v2 Arena receipt; admit only qualified HTML
      → Idriç HTML extraction/prepaint in IB PR #154 [NOT WIRED]
      → native Android reader [NOT WIRED]

This stays in the existing userspace sd-card-append-fat Arena, not a new
browser store, app-side HTTP fetch, WebView transport, or Java service.

### Setup and development invocation

The owner supplies a non-symlink private root (mode 0700), creates the
canonical responses.arena and selects checked executable paths:

    appendfat_arena create PRIVATE_DIR/responses.arena 16777216

Set IB_APPEND_ARENA and IB_ARENA_SLICE to the pinned compiled executables.
Set IB_C67_ICU_DEX, IB_C67_ICU_SO, IB_C67_ICU_DEX_SHA256,
IB_C67_ICU_SO_SHA256 to **actual qualified** Android app_process DEX and
ARM native ICU #35 artifacts in /data/local/tmp/ib-icu/. This PR does not
produce these artifacts or invent their SHA-256 values. Shizuku must be
started, and the caller must be authorized as shell/root.

    sh bin/ib_android_icu_arena.grease REQUEST_ID https://example.com/ PRIVATE_DIR SOURCE_GENERATION

SOURCE_GENERATION is a nonzero canonical decimal integer (max 18 digits)
allocated by the browser/source owner, **not inferred from HTTP**. The
caller remains responsible for ordering generations and binding user intent;
this development command is not an app IPC contract.

The Shizuku UID cannot write directly into the application's private Arena.
Its one trusted shell invocation makes a random mode-0700 staging directory
inside the pinned /data/local/tmp/ib-icu/, passes both ICU capture paths
through environment variables, then transports a single byte-counted packet:

    IBICU01 METADATA_BYTES BODY_BYTES\n
    [exact eight-field ICU metadata]
    [exact binary response body]

The packet header contains two eight-digit decimal byte lengths and a
newline. Metadata is limited to 8192 bytes; the body to 4 MiB, stricter
than ICU's 8 MiB native limit. The untrusted packet and staging files are
discarded on failures. The reader never sees a partial packet.

## Receipt versions and navigation policy

ib-icu-arena-v1 is still safely readable as verified **body-only** data
with UNOBSERVED status, MIME and final URL. It **cannot** be upgraded into
navigation by guessing those fields.

ib-icu-arena-v2 records exactly 23 named fields including:

- Request identity, source_key=arena:REQUEST_ID, source_generation.
- Requested URL and **ICU-observed** final resolved_url plus redirect_count.
- HTTP status, MIME type, explicit charset, content_encoding, ICU-reported
  utf8, declared_length and received_length.
- Canonical arena_offset, arena_end, byte_count and sha256.
- reopen_check=MATCH, acquisition_complete=YES, navigation_admitted.

The producer checks the exact eight-field upstream ICU metadata schema,
received byte count, declared Content-Length (if present), redirect limit,
URL syntax, nonzero 2xx status, and independently reopened Arena range.

navigation_admitted=YES currently requires **all** of: HTTP 200,
text/html, explicitly declared charset=utf-8, identity content
encoding, valid ICU UTF-8 observation, complete acquisition, intact
reopened bytes, source key/generation and correct SHA-256. Unsupported
text encodings, unknown MIME, missing charset, compressed undecoded bodies
and non-HTML responses remain NO; they are not converted to prepaint.
The reader independently rechecks the v2 admission predicate before
emitting even a byte. The external IB #154 adapter should still check
the receipt and call the verified reader, not bypass it.

The receipt is published only after append, native reopen and body compare.
Failure after append but before receipt leaves only an unreferenced tail;
no browser history is updated. Each request ID is single-use with a retained
per-request lock even on failure.

## Evidence and unfinished qualification

The workflow uses the **real pinned canonical appendfat_arena** and native
range reader. It mocks exactly the Shizuku command's framed stdout,
including redirect metadata, verifies v1/v2 reading, original binary bytes,
and deliberate missing/duplicate/forged metadata, corrupt digest and
commit-pointer refusals. ICU #35 has its own **separate** real native
redirect integration tests; passing IB host mocks is not passing that test.

Still **NOT_RUN** here: pinned ICU #35 Android DEX/ARM32 build and installed
artifact digests, actual TLS HTTPS and Shizuku invocation on C67, cross-UID
scoped Binder/PFD into the app, end-to-end #154 Idriç conversion, Material
reader URL entry and clicked links, C67 durable history/restart.
Coordinate IB #151, #142, #153, #154, #129 and ICU #35 before merge/release.
