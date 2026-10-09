# Bounded provider capture — IB-B06

The intended program runs an existing trusted OS operation while bounding its
elapsed time and both byte streams. It publishes stdout only after a successful
bounded producer, preserves a bounded failure's exact status/stderr, and makes
no claim that stopping a local rish client killed an Android process.

The type sketch is `CaptureBudget(seconds, stdoutBytes, stderrBytes)` plus an
explicit executable/argv, producing either completed bytes or a failure with
its status and bounded diagnostics. This record is an orchestration boundary,
not a new browser resource, result, worker identity, storage admission, or
supervisor. Existing Idriç semantic owners remain unchanged. Following IB's
README, Grease owns this OS-visible subprocess/FIFO operation; no Python/Node
runtime, Idriç semantic fallback, or native helper was introduced.

The implementation is `lib/bounded_process_capture.grease`. Initial attempts
exposed two real Grease integration mistakes: default mode reads imported
environment through `ENV`, and FIFO redirections outside `timeout` can hang
before the command is started. The final code reads `ENV.TMPDIR` and starts a
timed Grease child before either producer FIFO open. A reader-startup failure
test specifically rejects the old hang. A 1,024-byte stream mutant bypassing
the `head` cap is rejected by the 33-byte capture-count oracle for a 32-byte
budget. Final results and exact runtime identities are in
`docs/shizuku-bounded-capture-receipt.md`.

No new language work or compiler qualification is inferred. The current CI
workflow invokes these inherited adapters through `sh`; switching that shared
workflow to an explicitly qualified Grease stage is a separate unresolved
execution gate, not a reason to pretend shell compatibility is Grease evidence.
