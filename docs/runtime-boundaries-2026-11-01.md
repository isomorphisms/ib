# Actual installed reader failures and next repair

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
