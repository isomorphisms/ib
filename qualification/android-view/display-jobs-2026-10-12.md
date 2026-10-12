# IB D01–D10 execution boundaries — 2026-10-12

Request: execute IB D01–D10. Exact assignments were recovered from
[issue #120](https://github.com/isomorphisms/ib/issues/120#issuecomment-6064539546)
and its [D09/D10 supplement](https://github.com/isomorphisms/ib/issues/120#issuecomment-6064726379).
Their IDs refer to the reconciled assignments, not the original search probes.
The source boundary remains Idriç semantics, Grease orchestration and narrow
platform adapters; the selected presentation is Material 3.

## Current native gate

The October 8 statement that the checked backend supports only Int32/Text
exports is superseded by actual outbound framework/IO lowering. Reused owner:
[isomorphisms/android-NDK PR #20, “Lower checked framework calls and IO directly
to DEX”](https://github.com/isomorphisms/android-NDK/pull/20), current inspected
head `5c94120944aa50dc8dc8056ba34006aa8d38d13e`. Its bounded qualified consumer
pin is `7ea62a3e3cd19f2ab04a27f72ace1ae9e8b1fe66`; framework reference calls,
ordered effects and scoped Quick Settings observations passed on x86_64 ART
in [run 37889734090](https://github.com/isomorphisms/android-NDK/actions/runs/37889734090).
This outbound surface does not supply a generated View/Activity callback.

[isomorphisms/ib PR #134, “Move Long View lifecycle dispatch into Idriç and
exercise its direct-DEX path”](https://github.com/isomorphisms/ib/pull/134)
is the existing application/compiler consumer, at
`a624cef0c1f8b867154e49c7f172f7069b4a3708` during inspection. Its
[exact-head run 38107554419](https://github.com/isomorphisms/ib/actions/runs/38107554419)
contains distinct results:

- Actual Idriç lifecycle host execution and wrong-host/stale-callback mutants
  pass their controls.
- The genuine checked text-presence DEX is emitted and independently parsed,
  then executes on SDK 34 x86_64 ART; the external runner's received candidate
  bytes are compared with the emitted bytes.
- Full task DEX generation refuses `Prelude.Types.fastUnpack` with
  `No checked helper body for DEX call Prelude.Types.fastUnpack` and exit 1.
- No Material view construction, input callback, native text/action round trip,
  APK, clipboard, restored reading surface or physical phone run is established.

The current shared D-family product blocker is the missing qualified checked
Idriç-to-Material view/action adapter. Full-task DEX has an additional concrete
compiler blocker owned by C01/android-NDK; fixing it does not itself qualify
the view. No parallel compiler or Java/Gradle implementation was introduced.
This executor additionally lacks an installed Idriç compiler for the independent
pure D-family work. The dated October 8 compiler paths no longer exist; neither
`idric` nor `idris2` resolves. That is local availability, not proof that the
language cannot implement these semantics.

## Job-by-job result

| Job | Reused source/owner and actual result | First remaining boundary |
| --- | --- | --- |
| D01 — selected Android view | Existing qualification branch at `be731c7916e9489a03cf6cd5ea9328763faaee96`; repaired stale successful inspection after a failed rerun. Four actual Grease controls pass and original code fails the targeted control. | Native Material text/action round trip remains BLOCKED_NATIVE_VIEW; startup/RAM/APK costs UNKNOWN. |
| D02 — live retained projection | Longview `3524599f53e4028957d9d9580079a36760dd9b3f` has the existing Cauldron/source-result handoff and host projection; S03 binding owner `6da9b7482d186c643e6a30d5252cfb7352627602` remains separate. No Activity file was edited. | D01 adapter, actual worker acquisition and production projection consumer. Host projection is not native reopening acceptance. |
| D03 — pins/recents/collapse | Existing `material3-pinned-recents@b72b2ac9f56d3c7a66137dc0c265d1ae9756083d` stores fixture article selection/pins in Compose local state; it is not durable tab/event identity. Existing History and LongViewTask remain authoritative. | S05 ordinary durable navigation and D01/D02; independent Idriç work cannot execute here without the compiler. Do not turn article IDs into tabs. |
| D04 — scientific figures | Existing `0.2/viewport-figures@778266431c57060080d35324180c71e577c995b6`, `docs/viewport.md`, owns prepared-local-image + optional-caption presentation. | D01/D02 prepared-image adapter; decoded memory, color, recreation and physical visual behavior NOT_RUN. |
| D05 — trusted Copy | Existing `feature/source-bound-exact-copy@7ded5b7ecec37579f72513449c82af8566fe1706` owns CopyPolicy over S06 fragments, exact source spans, disable policy and stale/payload refusal. Historical 25-case normalization and whitespace mutant are recorded there. | Qualified consumer executable and D01/D02 clipboard/trusted-chrome adapter; clipboard write/readback and hostile-page separation NOT_RUN. No duplicate policy was created. |
| D06 — source-linked observations | Existing S07 `35541d40c16533fa4385c879c3c83c45821b4e3e` owns code-point ranges, revision/clock identity, replay refusal and visibility facts. | D01/D02 native geometry, foreground/occlusion and platform-offset adapter; S06/S07 qualified runtime. Journal fixtures are not actual viewport observation. |
| D07 — first useful Pensieve | Existing S08 `cb8cdc14b6065b91240b97b580055c0d18375fae`, `IB.HumanDecisions.useful_candidates`, owns accepted decisions and save/defer/return/exposure ordering. | S08 executable/store + live retained activity wiring, then D01/D02 presentation; no empty notes database or invented recommendations. |
| D08 — actionable images | Existing prepared-image contract and browser-owned action intent remain the owners. Copy, Save, Share and Open source must use actual retained-object/grant identity. | D01/D02/D04 and executable action consumer. Offline handoff, chooser cancellation, byte availability and task latency NOT_RUN; no real recipient transmission. |
| D09 — temporary preview | Existing viewport prepared-image reference is reused; PDF engine remains owned by `house-of-government/pdf`, `notes/androidx-pdf-engine-gate.md`. | D01/D02/S05 parent identity/scroll/selection restoration; PDF additionally needs actual read-only engine output. No parallel tab or PDF model. |
| D10 — optional Keep | Existing S08 accepted-item and S10 export/privacy boundaries are reused. Verified primary documentation supplies the channel limits below. | Qualified payload consumer and D01/D02 share adapter, plus observed Keep capability. No account or note write occurred. |

All ten jobs remain incomplete at their end-to-end acceptance boundaries. The
new executed repair is D01's inspection result lifetime. Previously recorded
source/normalization results are not presented as new execution or runtime
acceptance. No new source-only frontend model was created when the selected
consumer could not execute it.

## D10 channel limits from primary documentation

[Google Keep API overview](https://developers.google.com/workspace/keep/api/guides)
documents an enterprise-administrator API and domain-wide delegation. It is
not evidence that this user's personal Keep account has an admitted direct
API create/update/save-confirmation route. No such account capability was
observed or enrolled here.

[Android's sharing documentation](https://developer.android.com/develop/ui/compose/sharing/send)
documents `ACTION_SEND` with a declared content type and user-mediated chooser.
Text uses `EXTRA_TEXT`; binary content uses a readable URI and bounded grant.
The platform documentation does not establish that the installed Keep app
accepts a particular payload, preserves a requested title or confirms a saved
note. Inference for D10: a verified installed Keep receiver could qualify an
explicit handoff; it cannot qualify update or save without a supported receipt.
Selection/chooser dispatch remains UNCONFIRMED. The Compose examples describe
platform Intent behavior and do not select Compose or Kotlin as IB's language.

## Resume without duplication

Continue D01 through its existing qualification branch and the existing C01
compiler owner. Once a real text/action round trip is qualified, D02 consumes
the S03 source-backed projection; D03 and D09 wait for S05; D04 consumes prepared
images. D05/D06/D07 continue their actual policy/observation/decision owners,
then connect the native adapter. D08 and D10 consume the qualified image/export
commands. Hosted source checks, outbound DEX, ART probes, APKs and physical
acceptance must keep separate receipts.
