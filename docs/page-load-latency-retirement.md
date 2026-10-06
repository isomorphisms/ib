# Independent latency correction

isomorphisms/ib PR #91, “Account for page-load latency without UI-thread receipt fsync”
(https://github.com/isomorphisms/ib/pull/91), remains this family's sole live
successor. Its implementation head 4d9342771ad555288afcc5178e8f88a311c8c444
is not contained in Longview. Comparing LongViewActivity against Longview
041734f9e6a520350599d900581b5ae72d1a36c6 shows that Longview still syncs
ordinary receipts on the UI thread and lacks the detailed navigation trace.
Do not retire this correction merely because Longview is newer.

At the old head, Android build/tests/lint/signer and replacement installation
passed. Real-page job 108541785145 failed at APK installation with
`Can't find service: package`, after a 791600 ms emulator boot. The page never
loaded, and artifact upload found no evidence files. That is an emulator
failure, not a measured browser regression or a successful page test.

The harness additionally lost package/activity/url/passed variables between
script lines, split its multiline poll, used unsafe remote shell quoting, and
contained an unterminated quote in the error assertion. The repair keeps one
fail-fast command in the action's required POSIX compatibility boundary, reads
receipts through an explicitly quoted remote command, and checks launch status
and literal tab-delimited error events. The 1 GiB RAM, Android 14 target,
45-attempt limit and commit/finish/navigation-timing assertions are unchanged.
No new generic shell runtime or physical acceptance is claimed.

Fresh exact-head low-memory emulator acceptance remains required. If package
installation still fails, retain that failure; do not enlarge RAM or remove
the test to obtain green. MIRO A1 physical timing evidence remains absent and
must separately cover recovery/construction, durable checkpoints, DNS/TLS,
first byte, response, visible paint and DOM/load completion.
