# View-observation journal

IB-S07 consumes IB-S06's exact source-binding and checked fragment graph. The
first slice records facts supplied by an explicit adapter: observation id,
adapter and monotonic-clock session, source revision, fragment and code-point
subrange, interval, kind, foreground and occlusion. It does not infer reading,
understanding or attention and never observes another application.

`accept_observation` validates range/time/source/identity and makes callback
replay idempotent. An equal id with unequal facts refuses rather than replacing
an observation. `replay_journal` reconstructs accepted prior records, reports
invalid complete rows and preserves an explicit uncommitted final tail.
`visible_duration_v1` is a versioned derived query: only visible foreground,
unoccluded intervals count. Event facts and human decisions remain distinct.

Idriç owns semantics and replay. Grease owns bounded file reads and append
publication. The adapter and fake clock cross the same source-linked contract;
physical visibility is not proven by the fixture. Qualified consumer executable
generation is still blocked at S06's ICK/NDK gate; typechecking and compiler
normalization cannot erase that blocker. No alternative runtime is introduced.
