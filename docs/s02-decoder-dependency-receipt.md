# S02 precise decoder dependency receipt

Selected existing decoder: PR46 `project-raw-urls.grease` at
`b4d93c47327f112433b90c0a77a00b3ddcba0af4`. Its actual supported contract is
`source_kind=raw_urls` → ordered trimmed nonempty/noncomment URL occurrences,
repeats preserved, source-line coordinates, timestamp unknown, decoder
`raw-urls-v1`. It verifies canonical manifest-bound source identity, bytes/count
and SHA before decoding. The two supplied fixtures actually pass this owner
under current Grease, with measured 143→84 and116→59 text bytes, plus64/52
coordinate bytes. That is the decoder selection S03 can concretely supply.

First missing semantic/runtime bridge: Longview's `IB.WorkBounds` exposes
`first_work_bound_failure` and `admit_work_delta`; neither is consumed by PR46's
real decoder. `project-raw-urls.grease` lines40–45 redirect both awk transforms
without temporary/output/diagnostic/elapsed-work enforcement. Lines47–51 remove
the previous derivative before publishing its replacement. Input admission's
16MiB ceiling therefore does not establish post-decoder growth enforcement or
last-complete replacement recovery.

The current actual Idriç checker/normalizer is available, but no qualified
ICK/NDK consumer executable/action bridge for incremental `WorkBounds` admission
has been supplied. Duplicating its numeric policy in Grease, substituting Chez,
or passing a fixed success receipt would not establish that connection. This
child makes no decoder rewrite or WorkBounds enforcement claim.

S02 remains BLOCKED_EXTERNAL on that qualified executable/action boundary and
the S03 actual source producer/publication route. Source-result binding only
adds independently tested semantic refusal/recovery; it does not promote S02 to
READY_CI. Next execution must connect the exact reducer to this existing decoder,
enforce measured growth before publication, preserve source/last-complete view
on quota/parser/interruption failure and make a deliberate bypass fail actual
boundary acceptance. Physical memory/whole-device pressure remain NOT_RUN.
