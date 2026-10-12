# IB B01–B09 resumption — 2026-10-12

The jobs are the emitted B01–B09 assignments in
[the recovered queue](https://github.com/isomorphisms/ib/blob/38d455f9a36797b7f301d9604eb7aaab064e928a/docs/jobs/ib-r1/index.md),
not the initial search probes in issue 120. Work continues on existing owners;
no merge, release, device operation or OAuth enrollment is part of this receipt.

## Implemented changes

B03's retained `work/ib-bounded-worker-recovery` input is
`1f466af1b030b0fb2bf3a04c60c9a45207e27d26`, containing B01 at
`5205209f189cb3813d18ec251a273d59f369e50e` and preserved Longview/E2/E3.
Its duplicate bounded capture now delegates to the existing B06 helper from
`6b3350acccb22ba27a3546a7ea56fc672313cf2a`. The helper, fixture and suite are
reused unchanged, and the duplicate `android_shizuku_capture.grease` is removed.
B03's operation-specific budgets, phases and exact attachment are retained;
B09's refusal before signal/provider contact and attempt mutation is retained.
No runtime, browser policy, supervisor or native compiled helper is added.

B02 at `3233277c914715892a37d2bf50cd572f4d09e94c` had a separate regression
setup omission: `test_rish_bundle_provenance.grease` loaded host and generic
capability modules but omitted the newly required Binder boundary. The test
returned exit 1 at the matching active-pair control because launcher resolution
was unavailable. Its setup now loads the real worker/capability providers and
Binder module. No mocked Binder function or direct provider bypass is added.
The same setup correction is propagated to its existing dependent storage line
at `cc5771576d37b5651ecfe39da090901a1f77428d`.

## Executed evidence

Execution host: disposable x86_64 Linux, Ubuntu 24.04.3; no ADB executable or
verified physical-device channel. Grease implementation is
`5651cf97a1b5042f24f14112a7ade9a1518eb0bc`, version 0.37.0.
The existing consumer launcher SHA-256 is
`29c673cf786d167e606af96a167c51cf6ee56d4527cfabdf21d727a269a649ee`;
underlying executable SHA-256 is
`7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`.
`ASAN_OPTIONS=detect_leaks=0` is the existing ptraced-host requirement.

The capture and handoff suites execute in default native Grease. Inherited
host/Binder/capability/worker/provenance suites use explicitly named
`+O ysh:all` compatibility mode through that same Grease executable.
Provider fixtures are test doubles, not live Shizuku authority.

| Executed boundary | Result |
| --- | --- |
| Shared B06 capture | PASS: exact/empty output, exact denial 37 and producer-owned 124; independent and combined overflows 65; stalls 124; TERM resistance 137; later recovery. |
| Reader cap and reader startup | PASS: 33-byte retention for a 32-byte budget; uncapped-reader mutant rejected; failed readers return 74 before fixture producer starts. |
| B03 phase transport | PASS: independent callers, running versus committed read, denial/recovery, changed boot, tampered/truncated/oversized receipts, duplicate start, no-contact signal refusal. |
| Existing host and E3 worker adapter suites | PASS in explicit Grease compatibility mode; exact authority failures and B09 refusal retained. |
| B02 provenance | Original setup FAIL exit 1; corrected setup PASS all ten positive/refusal/recovery groups on parent and dependent storage sources. |
| B02 Binder boundary and existing capability reports | PASS on inspected parent source before the setup correction. |
| Actual fixed worker program | BLOCKED exit 44 at first process-identity read, both original retained input and updated source. Host PID translation does not provide the worker's required `/proc/$pid` identity. |

SHA-256 of shared capture:
`cbb2ade5e4c08b5cf86b4d3da26aeabe567fb9401def2fb864eba9a511cd974a`.
SHA-256 of corrected B02 regression:
`e046971b7d52eee20806b73a3415e8d2ede3a2fcc972d19e1a105e979f775447`.

The inherited B03 foundation workflow still invokes positive adapter suites
through `sh`; it does not qualify this native capture or handoff. B02's green
foundation workflow does not execute its dedicated provenance suite. This
receipt supplies local execution, not continuously protected regression CI or
merge readiness. No new unqualified bootstrap/build stage was introduced.

## Remaining boundaries by assignment

| Job | Concrete remaining boundary |
| --- | --- |
| B01 | Hosted authority status repair passes; physical Shizuku authority remains unrun. |
| B02 | Corrected complete local provenance suite passes; automatic qualified Grease execution of that dedicated suite and MIRO reboot/reacquisition remain unrun. |
| B03 | Actual fixed-worker identity cannot execute on this host; physical caller/service/reboot phases and launch-before-handoff interruption recovery remain incomplete. |
| B04 | Preserved PFD adapter still has uncapped session threads and blocking read/EOF waits. Its Android path at Longview `843c7bcf5336c685c4a96a4b4bc28b96fbc7a5dc` uses Gradle/Java and has no declared qualified ICK/NDK consumer stage. The emitted job conditions runtime edits on build qualification; no substitute build or transport was selected. |
| B05 | No verified MIRO A1 connection or qualified B04 artifact; cross-UID live-channel and durable-reader cases are NOT_RUN. |
| B06 | Shared native capture and adversarial tests pass locally; published B06 exact-head native job also passes at [run 37951400274](https://github.com/isomorphisms/ib/actions/runs/37951400274). No physical remote cancellation follows from local timeout. |
| B07 | Preserved latency source `f3533211bc0eca556b3ad58a3ca64ecaaea1d618` remains unaccepted: its latest [run 37468854379](https://github.com/isomorphisms/ib/actions/runs/37468854379) failed APK installation before navigation with a broken package-service pipe. The emulator explicitly increased configured 1 GiB RAM to 2560 MB, so this also supplies no 1 GiB execution evidence. The build qualification boundary and actual MIRO timings remain open. |
| B08 | Preserved Drive adapter `0b0a7bed15cb668a00ba11147618e3d353d58f1b` has hosted checks; approved private signer, installed C67 identity, OAuth registration and physical consent/authorized request remain external. No account state or credential was changed. |
| B09 | Fail-closed refusal returns 78 without signaling. No qualified identity-bound Android primitive is exposed through the current adapter; actual owned-child signaling and kernel exit/reuse barrier remain BLOCKED. |

The latency failure is emulator/package-service evidence, not a page speed
measurement. Public/test signing is not live OAuth enrollment. Successful
capture or handoff mocks are not physical-device acceptance or proof of a
worker lifetime. All nine jobs remain separate obligations; this continuation
does not claim all nine complete.
