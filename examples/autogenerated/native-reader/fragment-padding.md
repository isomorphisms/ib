# Measured native fragment scrolling through a padded ScrollView

Purpose: make an admitted source-linked UTF-16 point visible after native layout, including a return to the beginning. Keep the Idriç HTML anchor recovery, source coordinates, retained navigation and private acquisition path intact.

Type boundary: the checked Idriç consumer supplies a bounded UTF-16 display point. The maintained JNI adapter accepts that point only for its current immutable projection, measures Android Layout line bounds, requires the actual direct parent to be a ScrollView, and checks integer ranges and the padded viewport. It calls SDK scrollTo and checks the observed result. No Java/Kotlin application or handwritten candidate DEX is introduced.

## Reproducible failed attempt

IB 1a0a0602de168387f556c428c470e9b3eb6f5fe1 used View.requestRectangleOnScreen on pre-draw, after the earlier focused-only TextView.bringPointIntoView attempt. Source compilation, source-coordinate anchors and packaging passed. Actual API 29/34 primary instrumentation still rejected the document beginning. Independent AICI observer 8561acf3f3e88baa6af59cf06f149e71b4038a14, run 41343801337, API 34 job 124959245730, captured the screenshot inside the failure and measured the beginning at screen y=193 while the clipped document rectangle started at y=199. Root screen origin was (0,0); this was real clipping, not a coordinate-plane mismatch. The first link was visibly clipped. That replay's API 29 fragment job failed to boot its emulator and supplies no fragment result.

The Android 10 ScrollView implementation computes requestChildRectangleOnScreen/computeScrollDeltaToGetChildRectOnScreen without subtracting its padding from its vertical window. Its result can put the first line behind clipToPadding. Source: aosp-mirror/platform_frameworks_base, android-10.0.0_r47, core/java/android/widget/ScrollView.java.

## Repair and acceptance

The adapter now measures the direct ScrollView's actual padding and the TextView's child top and Layout line. The requested scroll is child_top + line_top + text_padding_top - parent_padding_top, clamped by SDK scrollTo at the real content extent. The adapter normalizes TextView's own selection scroll and refuses a wrong parent, invalid geometry, overflow, or a target line outside the padded viewport after scrolling. No display size or toolbar height is assumed.

The independent SDK screen-visibility predicate is unchanged. The compiled zero-point native mutant remains required and must fail on the target rather than pass with an arbitrary scroll request. The required API 29/34 primary workers now pin that exact 8561 observer, including its verified already-stopped-Shizuku handling. Activity ownership and all six native link-shape/offline-history observations were positive on the unchanged 1a0 APK in that replay, with ownership/metadata/byte controls; that is not qualification of this changed candidate.

The new candidate's build and both Android workers must run before this repair can be called qualified. Physical TAB_P10 acceptance remains BLOCKED: no authenticated device transport or fresh installed signer, SDK, ABI, storage or Shizuku observations are available. No tablet, search-results or release completion is claimed.

The first padded-scroll source publication70267233165581089ae0295d65ad7409666b2ba8 advanced both workflow checkouts but retained the old210e6a observer expectation in verify-observation-input.grease. Review found that mismatch before interpreting its Android results. This follow-up aligns both archive and live-checkout comparisons with the same explicit8561 observer; no identity check is relaxed. It also refreshes the ledger with the exact independent1a0 ownership/link positives and the fragment failure rather than retaining obsolete NOT_RUN entries.
