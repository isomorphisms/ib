# Shizuku authority status — hosted execution receipt

Date: 2026-10-08. Assignment: SUN IB-B01, existing owner
https://github.com/isomorphisms/ib/issues/93.

Retained Longview baseline: `843c7bcf5336c685c4a96a4b4bc28b96fbc7a5dc`.
Initial local implementation commit: `02c0ea6dd316dd2d896dcf472386663248e750fc`.
Published and independently fetched implementation commit:
`cc97557718cd46ddee7fef41fbad3eb048b1aa00`.
Both commits have the exact tree `1c4b0e8dbf38e520cdb6c50316dc68d42616d114`.
Isolated branch: `fix/shizuku-authority-status`.

## Executed boundary

Hosted adapter execution on container `0deb85a33ecd`, Ubuntu 24.04.3 LTS,
Linux 6.18.44, x86_64. No build, Android packaging, installation, or physical
Shizuku/rish operation was performed. Physical-device acceptance is NOT_RUN.
Worker identity, immutable publication, and browser policy ownership are
unchanged. The retained branch remains unchanged.

The actual Grease entrypoint was
`/workspace/scratch/4cbbf3939bb5/il0/qualification/grease`, qualified from
Grease repository revision `ba869518c7d850de6c47d8c6234654575e264e6c`.
Its reported implementation revision and repository gitlink both identify
`5651cf97a1b5042f24f14112a7ade9a1518eb0bc` (reported version 0.37.0).

Entrypoint SHA-256:
`37088097c36a2f82cf45aaea9392e4a4cf16155fecb9da87d08a9c7fb4d92e4a`.
Implementation executable SHA-256:
`7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`.

Invocation: `grease +O ysh:all TEST_PATH`, with
`ASAN_OPTIONS=detect_leaks=0`. The implementation option selects inherited shell
compatibility semantics. This option is necessary for these existing suites:
the unmodified Shizuku suite fails on its legacy `trap` syntax in default mode;
the unmodified Binder suite fails on its legacy word splitting. Default-mode
Grease acceptance is therefore not claimed. The pre-existing external fake-rish
fixture uses `/bin/sh`; the adapter and test orchestration ran under the pinned
Grease executable, not a substitute interpreter. CI's inherited `sh` invocation
was not relabeled as Grease execution and was not changed by this focused fix.

## Results

| Case | Result |
| --- | --- |
| Shizuku host suite on implementation commit | PASS, exit 0: `android shizuku host adapter tests: ok` |
| Binder host suite on implementation commit | PASS, exit 0: `android Binder host boundary tests: ok` |
| UID 0, UID 2000, ordinary UID 10123 | PASS: root, shell, and uid:10123 respectively |
| Provider exits 37 and 124 with plausible root/shell text | PASS: exact nonzero status and no authority output |
| Provider permission denial, exit 13 | PASS: exact status, stderr preserved, no authority output |
| Missing explicit provider and nonexecutable provider | PASS: status 1, diagnostic, no authority output |
| Empty, textual, whitespace-padded, suffixed, negative, and multiple-line UID output | PASS: status 2, diagnostic, no authority output |
| Real Binder delegation | PASS: same successful controls and refusal matrix as Shizuku |
| Isolated Binder boundary | PASS: root/shell controls and exits 37, 124, 13, 1, 2 forwarded |
| Restored status-masking mutant | REJECTED: test exit 1, `ib_shizuku_authority returned status 1; expected 37` |
| Restored implementation after mutant | Both suites PASS again; worktree clean |
| Independently fetched published implementation | Exact tree matches; both suites rerun from detached published checkout, PASS |
| Whitespace/error checks | `git diff --check` PASS |

The mutant changed only the authority query's `return $?` back to the original
`return 1`. The regression rejected that exact defect; the change was then
reversed. It was not committed or published.

Initial terminal publication lacked HTTPS write credentials. GitHub object API
publication preserved every source blob, file mode, tree, and retained baseline
parent; GitHub supplied different commit metadata. The published implementation
was then fetched through ordinary Git, compared against the tested local tree,
and both suites executed again from that exact remote commit. This receipt is
added in a subsequent documentation-only commit. No PR was created, reopened,
merged, or closed.

Tested file SHA-256 values:

| File | SHA-256 |
| --- | --- |
| lib/android_shizuku_host.grease | df9c8515c0c834a9f1e7280e4b435ddc7983198c382399177e47bca6ee8b47ad |
| lib/android_binder_host.grease | 031276630ac8183ce52bb110cb5faf968711d866688d8d2971d3a18b7f120238 |
| tests/test_android_shizuku_host.grease | ce35635ac8b7e0d1adcc18427d3ff4ae431157b24a3e0e71631f97ccd06df9b4 |
| tests/test_android_binder_host.grease | b306a2495d1d0915207393a934ee46636b3221a3cbac35854a049517ab1dc405 |

This receipt proves hosted adapter status handling with mock providers. It does
not prove real Binder access, real Shizuku authority, device permission state,
or process survival on either physical phone target.
