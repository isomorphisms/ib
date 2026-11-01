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
