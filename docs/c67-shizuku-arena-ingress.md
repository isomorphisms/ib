# C67 Shizuku → app-private Arena ingress

**Status:** implemented native stream/commit boundaries, host tests and ABI build;
**not a usable phone URL flow**. Stacked on IB #153 → #152 → #112.

## Reconciliation with admitted v2 source owner

The paired ICU metadata/body capture from the v2 source branch and the
body-only stdout ingress remain distinct capabilities. Reconciliation retains
the metadata-aware three-argument `ib_shizuku_icu_get`; the stdout helper does
not overwrite it. The canonical ingress publisher still emits v1 with
`navigation_admitted=NO`. Tests independently reopen its retained bytes with
the canonical store and require the newer v2 HTML source owner to refuse
those receipts. They do not weaken the source owner or claim that this
body-only publisher can feed an admitted page.

The host object is compiled with `-fPIE` for its PIE test executable. Stream
tests count actual invocations separately from lines in the remote command.
The inherited shell CI boundary remains compatibility debt; local stream
execution uses Grease implementation `5651cf97a1b5042f24f14112a7ade9a1518eb0bc`
with inherited-shell compatibility enabled. Leak checking is disabled only
for that sanitized runtime in the disposable container, whose process
namespace prevents LeakSanitizer inspection.

The next product dependency is observed HTTP metadata over the authorized
private ingress, followed by Android app-handle lifecycle and the checked
Idriç reader. None is established by compiling the Binder adapter.

## One owner per operation

The *browser/Idriç host* owns the requested URL, request identity and
navigation generation. An approved Shizuku broker (UID 2000 if the actual
Binder caller is shell; otherwise an explicitly granted app UID) has only
a one-shot **write descriptor**. It never opens app-private storage or
receives a pathname for the Arena.

    app-owned browser event (URL, request ID, navigation generation)
        ↓ explicit approved Binder caller UID
    private ShizukuArenaIngress.v1 Binder object
        ↙ transaction 2: BEGIN(request_id, generation)
       app returns approved URL + one WRITE ParcelFileDescriptor
        ↓
    approved broker process / pinned Grease runner
        ↓ verified Shizuku authority (rish), checked ICU DEX + native SO
        ↓ exactly binary ICU stdout → inherited PFD; no body-as-string
    app-owned reader thread drains a bounded anonymous pipe concurrently
        ↓ broker closes its WRITE descriptor
       transaction 3: FINISH(request_id, generation, actual ICU exit)
        ↓ app confirms EOF, <= 4 MiB, successful zero exit, current generation
        ↓ app-owned canonical sd-card-append-fat append + commit pointer
        ↓ independent committed-range readback and exact byte comparison
        ↓ atomic private ib-icu-arena-v1 receipt publication
    existing IB source-owner verifies receipt, bytes and SHA-256
        ↓ later source-linked Idriç prepaint and native reader (NOT WIRED)

### Native owners

- \`ib_shizuku_arena_ingress.{h,c}\`: single-use stream lifecycle, exact
  Binder caller UID/ID/generation admission, bounded concurrent draining,
  35-second deadline, 4 MiB body cap, cancellation and fail-closed FINISH.
- \`ib_shizuku_ingress_binder.{h,c}\`: separate private Binder interface.
  BEGIN transmits the *app-authorized* URL and a PFD, never a storage name.
  FINISH accepts only process exit 0 before calling the publisher.
- \`ib_shizuku_ingress_runner.{h,c}\`: broker side, obtains the write PFD,
  launches a separately qualified Grease executable/worker using
  \`posix_spawn\` with that FD mapped onto stdout, watches exit/death,
  closes PFD, reports actual exit status. The launcher and script must be
  pinned by Cat Food; the local ownership checks alone do not attest them.
- \`lib/android_shizuku_icu_provider.grease\`: existing checked ICU
  command reused for both ordinary capture and new stdout streaming.
  \`bin/ib_android_icu_stdout.grease\` is its narrow binary output entry.
- \`ib_shizuku_arena_publish.{h,c}\`: app-owned persistence callback using
  the **actual** \`appendfat_arena\` C API, not an alternate store. The
  source root/receipts/locks must already exist with 0700 ownership.
  Each request ID gets a unique lock. Existing appendfat owns the .used
  commit-pointer and lock protocol. After append it independently
  reopens/verifies every byte, then fsyncs a temporary receipt and
  atomically links it at \`receipts/REQUEST_ID.tsv\`. Failed writes do not
  publish a valid receipt; committed but unreferenced tails remain
  inaccessible to reader navigation.

The v1 receipt retains \`UNOBSERVED\` for HTTP status, resolved URL and MIME
and \`navigation_admitted=NO\` until ICU #35 metadata is integrated. It is
**only** proof of retained source bytes, not evidence that a URL was rendered.

## Security properties

- Shell UID 2000 and a Termux/app bridge UID differ. Access is bound to
  the *actual* Binder caller UID that the app approved for one navigation;
  there is no wildcard permission and no authority derived from page data.
- The source directory is 0700 and belongs to the receiving Android app
  process. No world-readable \`/data/local/tmp\` response and no reliance
  on cross-UID ordinary-file access. Path names from the producer never
  identify source files in the app.
- On nonzero ICU exit, source too large or empty, stale generation, denied
  Shizuku, broker death, timeout, duplicate ID, full Arena, write/readback
  failure, malformed receipt, or path substitution: do not make a new
  current source available. The prior committed receipt remains valid.
- Broker's actual ICU command still requires Cat Food-pinned SHA-256 of
  both Android ICU DEX and native transport. It does not silently use
  WebView, \`HttpURLConnection\`, curl or a reader-owned network client.
- Binder service is never registered globally; the native Android host
  must distribute its object privately and reject unknown/unapproved
  clients. Merely compiling a Binder class is not physical authorization.

## Verification limits

The fast source tests use two Linux processes and SCM_RIGHTS transfer to
exercise the actual reader thread and one-shot source admission. They
test refused UID, request identity, epoch, unsafe URL, worker exit 29,
empty/oversized/late output, duplicate calls and retained recovery.

The pinned-storage tests use the real \`sd-card-append-fat\` C implementation
compiled by the checked ICK compiler; append, pointer publication, complete
source receipt, duplicate prevention, corruption and independent PFD reopen
are exercised. The Android CI compiles the native Binder/runner library for
ARM32 and ARM64 and the canonical publisher *adapter object*. Linking
the canonical appendfat code for Android still needs the qualified target
compiler/runtime; the object alone is not a complete Android app.

**Not yet done:** delivering private Binder handles between real MIRO C67
UIDs; app-side Android lifecycle creating/arming ingress; Cat Food packaging
and pinned launcher deployment; real Shizuku+rish execution of ICU HTTPS
and its response metadata; full native Android publisher linkage;
Idriç prepaint projection and live reader navigation; physical device
receipts. Do not advertise a URL-capable APK from this PR.

Tracking: IB #151, #142, #141, #97; PR #112, #129, #147, #152, #153,
and ICU PR #35.
