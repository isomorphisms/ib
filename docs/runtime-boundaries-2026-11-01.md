# Actual installed reader failures and next repair

## Current end-to-end limit

At 0003f7187e1a0f9f1b70693e2780bf0c27388932, run 41275673178,
both installed emulator lanes complete the actual Grease/ICU acquisition and
canonical Arena publication: worker exit 0, publication 0, accepted 1. The
first fetched native document still fails its independent 60-second display
expectation on API 29 and 34. Later offline, update and reboot checks are
NOT_RUN. Physical TAB_P10 acceptance remains BLOCKED by missing access.

Compact artifact 13071424309 is 1,752,662 bytes, SHA-256
`d75927a18e0f8c607a3f413cdd2f2b00eb9d26f3adbd9d9032a8f524a9e573af`.
It does not yet distinguish a source-owner refusal from a stalled checked
renderer. The new traces expose only stage/status/count/generation values;
the independent instrumentation captures at most 24 main-thread stack frames
at an assertion failure, without source text or requested URLs. A failed JNI
renderer callback now reports a recoverable error instead of silently clearing
its exception. These changes require fresh installed execution.

The source-linked TextView and its checked selection/link exports are present
in 394ac8156eee7ab20fc398550fdddf2f7e88bd0d. Host checked emission, independent
decode, deterministic output, literal selection expectations and compiled
source mutations passed. Its new ART and installed results are pending; this
is not a claim of working on-screen selection or link navigation.

Campaign status is **INCOMPLETE**. The source-bound installed reader test at
`8eb2a1eff42cef1371b8ff064cd2c23d1da89ca9`, IB run 41266041447, failed on
both Android APIs. Neither the native worker's successful construction nor
its authenticated Binder callback is a successful HTTPS read.

| Stage | API 29 x86_64 | API 34 x86_64 |
|---|---|---|
| Maintained checked Activity/ICU DEX and three-ABI APK build | PASS | Same built APK |
| Actual provider attachment, permission denial/grant, identity and Binder death with compiled controls | PASS | PASS |
| User-service registration, source-bound worker initialization, authenticated connection | PASS | PASS |
| Actual app-UID private worker RPC | PASS | FAIL: framework flags 18 refused |
| Grease-supervised acquisition | FAIL: broken pipe signal, exit 141 | NOT_RUN after refused RPC |
| ICU HTTPS response and durable publication | NOT_RUN as a successful pipeline | NOT_RUN |
| Visible fetched document, retained reopen/update/reboot | FAIL at first document; later checks NOT_RUN | Same |
| Physical SVITOO TAB_P10 | BLOCKED: no authenticated endpoint or fresh inventory | BLOCKED |

Compact diagnostic artifact 13067522140 is 1,748,959 bytes, SHA-256
`76de397e6e1206b673a3bd87453b2b02036754e392ec1069f735e313262a8616`.
It records actual worker UID 2000, actual app caller UIDs, and the callback and
request flags. It redacts Shizuku service attachment nonces.

Android's `BinderProxy` adds `FLAG_COLLECT_NOTED_APP_OPS` (2) to synchronous
transactions when listening for noted operations. The narrow direction check
now permits that documented bit along with libbinder's `TF_ACCEPT_FDS` (16).
Oneway direction, unexpected bits, caller identity, exact capability epoch,
descriptor, bounds, complete parcel consumption and descriptor-transfer checks
remain separate. `test_binder_flags.c` has literal accepted/refused cases and
a compiled old-mask mutation. Primary contract:
https://github.com/aosp-mirror/platform_frameworks_base/blob/5d412489475694286e3b3c43e74f42ef21ebc4de/core/java/android/os/BinderProxy.java

The API 29 broken pipe is not yet localized to source stdout versus inherited
stderr. The native supervisor now drains a dedicated stderr pipe into at most
64 KiB of private diagnostics while enforcing its existing deadline. It keeps
draining after that cap, so logging cannot break the source stream. Successful
requests remove only their own temporary diagnostic. Failed diagnostics stay
private; Android logcat receives numeric stages and counts only. The independent
observer may collect them only for its public fixture in a verified fresh
emulator. No physical device or arbitrary user URL is included in that surface.
The ingress reader also reports numeric EOF/cancellation/generation observations
without source contents. These changes are diagnostics and a framework contract
repair; fresh execution is required before claiming they fix acquisition.

The checkpoint stage separately failed because a quoted path followed by a
shell glob is not valid in this Grease profile. It is repaired using native
`find` with exact archive selection. Replay now retains Grease's generated
ASDL/native modules as well as its frozen source. The qualified host ICK archive
is split into independently retrievable pieces below the connector's 512 MiB
limit, with whole-archive and piece digests. These are recovery changes, not
reader acceptance.

android-NDK `b0f135d7d3536093874fdcae62cf57520f6f1af3` separately passed current
API 29/34 Unicode scalar Text ordering, managed command and pure export checks,
including rejection of the compiled UTF-16-order mutant, in run 41263805784.
This does not qualify future changed reader code. Reconstructed browser history
remains separate pending work; exact copy, links, non-UTF-8 admission, SEARCH and
the remaining recovery/fault acceptance are still required. No merge, permanent
release, signer change or device installation is performed by this checkpoint.

## Next actual execution at 175fdbf

The retried run 41268776178 completed source/ELF/package production, host
transport flag checks and the compiled old-mask rejection. Both real installed
lanes then authenticated the worker and admitted its private request, including
API 34's flags 18. Private diagnostics now expose the actual pre-acquisition
failure: the worker calls `url.startswith`, which does not exist on Grease's
builtin `Str`. The interpreter exited 3; no ICU process or response frame was
produced. The previous broken-pipe signal had concealed this stderr diagnostic.
The stream logs show no source bytes and matching navigation generations.

Compact diagnostic artifact 13069250436 has SHA-256
`04279438c77ee0c6ee1e2bb97030373584ec02360f2de702cd2a35c0f86e7d60`.
The maintained worker is corrected to `startsWith`, as defined by the pinned
native Grease runtime. No interpreter, shell, acquisition client or transport
policy is substituted. A new host guard test executes the actual script:
HTTP/HTTPS reach deliberate missing-file refusal (1), while invalid schemes
and empty addresses refuse before acquisition (2). A missing method (3) cannot
pass these expectations. These guards passed on the restored exact host Grease
runtime. They neither launch Android nor qualify ICU or the private pipeline;
the full installed test remains required on both APIs at the changed head.

The replay checkpoint completed with separately retrievable host ICK pieces
and Grease's generated modules. New reconstruction tests for the uncommitted
browser history are independent work and do not qualify the reader here.

## Actual acquisition at 81b5c47

Run 41271152056 again passed source/native packaging and the real installed
permission/identity/death controls. Both APIs now execute the maintained worker
through authorized Shizuku, launch the checked ICU process, and finish Grease
with exit zero. The app-side ingress observes EOF after 873 bytes with matching
navigation generations. Private worker stderr is empty. The actual Arena
publication returns -1; no committed reader source or fetched native document
is accepted. Response fields/body digests have not yet been independently
recovered, so no HTTP status or successful source admission is inferred.

Compact artifact 13069893303 is 1,769,991 bytes with SHA-256
`57edd7743b3325e5bf90dc927cbb3f9a2bc9f41e90e6b63bbe8e422bd13e9d46`.
The canonical publisher now emits numeric refusal stages on Android: 1–15
identify envelope/metadata/response validation; 20–32 identify private storage,
request locking, canonical append/commit, independent reopen/readback and receipt
publication. No source text, URL, authorization token or response value enters
those logs. All refusal conditions remain unchanged. Fresh host canonical
append, redirect-chain, independent readback and retained-generation mutation
checks passed with this diagnostic change. Android execution remains required
to localize the observed failure.

Checked browser history is now published at f7daf125d44e9d53280dec4305125fc14c49318c,
with fresh host fault/DEX checks and independent ai-ci 9ab044b92417f7ffd3eea54edb65367526e1503a
observers. Its new Android/UI acceptance remains pending. The campaign remains
INCOMPLETE; physical TAB_P10 acceptance remains BLOCKED.

## Receipt publication localized at d893697

The actual Android log from run 41273278580 reaches publisher stage 32 with
errno 13 on both APIs: immutable receipt publication fails after the candidate's
frame validation, canonical append/commit and reopened byte comparison. This
localization is a candidate-internal trace, not independent source acceptance.
Compact artifact 13070204978 has SHA-256
`3b43cfec83d6fcee7962b0084eda7e2d7ebf1d5e40cae6c052833dfae57fdf00`.
No committed readable native document is accepted by that failed run.

The publisher replaces hard-link publication with `renameat2(RENAME_NOREPLACE)`.
The staged receipt is still synced before publication and its directory after
publication. An existing destination refuses atomically; unsupported syscalls
or denied access refuse without an overwriting fallback. The selected source
identity and previous receipts stay independent of temporary paths. More precise
receipt-only numeric failure stages retain diagnostics without private data.
Host canonical tests passed for immutable-target collisions and preserved prior
readback. The compiled flag-zero mutation failed its exact no-replacement
assertion. This is not yet Android qualification of the repair.

Separately, f7daf125's maintained history semantics and all three compiled
branch/cursor/generation mutants passed independent API 29/34 ART observations
in run 41272370194. The independent assertion helper produced the required
diagnostic plus status 137, while positives exited normally. The installed
reader still failed at its first fetched document, so native history events and
reopen/update/reboot remained NOT_RUN.

The next integrated candidate consumes android-NDK 114a5481cd1cdc15ce02a7b134b798a3124f7ec7
for checked export-alias saturation and ai-ci f3764fe8afbdc02956382cc618caab297a3b9f47
for explicit reader-v3 source-selection observations. It adds a maintained-writer
native TextView with checked Idriç copy/link coordinates and SDK gesture,
clipboard and styling adapters. Host type/DEX/decode/determinism checks passed;
independent ART, actual touch/clipboard/link events and the complete acquired
document remain pending. This candidate does not imply physical acceptance or
finish non-UTF-8 admission, fragment targets, SEARCH, background layout, all
storage faults or remaining lifecycle work.

The earlier alias-runtime attempt at 09289bf failed in its positive independent
observer, before a valid result, with exit 137 and no assertion marker. That
failure is retained. The observer now preserves its SDK counter's register type
across failure branches and captures AndroidRuntime errors before exit. The
compiler semantics are unchanged between the two alias qualification heads;
fresh qualification is still required, including actual reader link effects.
