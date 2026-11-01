# Source-linked cursor and animation ownership after fragment restoration

Purpose: keep an explicit checked source anchor visible through real native link touch and retained Back/Forward, while preserving normal Android text selection and exact source-backed copying.

Type boundary: Idriç supplies the verified source-to-UTF-16 display point for the current immutable projection. The maintained JNI adapter bounds the point, requires a real direct ScrollView parent, measures the Layout and padding, sets a collapsed SDK selection at that same point and settles the parent's old animation at its measured final scroll. User range selection remains native and is not changed by ordinary scrolling.

## Failed attempt and actual evidence

IB57e0352175ee2dc6254ea5b722540e94e0d6d50a fixed the earlier padded first-line clipping. In actual API29 job124977111484, producer41347346657, native touch reached the target at screen y=493 and retained Back restored the beginning at y=213. Retained Forward then failed the unchanged SDK target-visibility assertion. The adapter had applied parent_scroll=7262 at point4974/line290; the next SDK observations showed the page drifting to the old beginning at y=194 then193. The in-process screenshot and target/global-rectangle coordinates remain in that failed job's artifact. This is a real failed navigation, not a passing scroll request.

Android10 TextView.onPreDraw brings its current selection end into view; changing selection registers another pre-draw. ScrollView.scrollTo does not abort its OverScroller. A pending smooth scroll to the old selection can therefore overwrite an otherwise correct immediate source-anchor scroll. These public framework sources were inspected at aosp-mirror/platform_frameworks_base, android-10.0.0_r47, core/java/android/widget/TextView.java and ScrollView.java.

## Repair

The new source-derived insertion point is set with the public Selection.setSelection(Spannable,int) API. The adapter then issues a zero-distance ScrollView.smoothScrollTo at the already measured/clamped position. That replaces an older animation without moving the requested position. Later TextView pre-draw now names the same checked source point. No focus stealing, disabled selection, repeated arbitrary scroll loop, hidden SDK reflection or Java/Kotlin application is introduced.

The same ICK-built native source now produces an additional anchor-reset APK: only settle_fragment_cursor's body is replaced with a no-op. It keeps the real checked Activity/DEX, source admission, original payloads, package/version and public test signer. Its independent retained Forward visibility check must reject the diagnosed old behavior. The existing compiled zero-point control remains required. The ninth APK is included in the producer's digest manifest and same-run input verifier.

All other source, Binder/ICU/Arena ownership, decoder, history and reflow contracts remain in their maintained implementations. Producer observer8561 is unchanged; the independent observer will consume this exact new archive and exercise the new ninth control after publication. Previous57 source/host/package and unrelated online/offline tests remain historical observations. This repair is not called qualified until the new host build and both real Android APIs execute it. Physical TAB_P10 is still BLOCKED/NOT_RUN.
