# S03 source-result binding attempt

Purpose: a completion marker must not be promoted to recoverable source content.
`IB.SourceResult` consumes the existing `WorkerTask`, `ProcessAttachment`,
`AdmittedPublication` and `NeutralAddress`; its new records are adapter readback
receipts referencing the existing retained source, not another corpus or browser
model. `SourceResult` and `SourceProjection` constructors remain private.

The reducer requires matching event/requested/resolved/acquisition revision,
canonical intake-shaped source identity and SHA-256, nonempty complete bytes
within the existing intake/admission ceiling, matching result/location, zero
producer exit and exact published/retained hash/count. Existing E3 commits only
the matching attempt. Raw-URL projection receipts retain PR46's `raw-urls-v1`
decoder identity and must bind the same source identity/hash. Failed replacements
retain the prior checked projection with its prior source identity.

Receipts are facts supplied by trusted byte owners, not cryptographic proofs or
authenticated transport. The reducer does not compute hashes, acquire URLs,
write files, enforce decoder growth, or assert that an adapter supplied truthful
measurements. Acquisition timestamps remain supplied provenance.

Actual current compiler `ff4d852862a3942592f8ade9afde8d409d9803be` checks the
module and acceptance source. Its normalizer evaluates 30 cases to True.
Removing the published/retained SHA comparison makes `source-hash-change-refused`
and `completion-marker-refused` False. Restoring it restores all 30 to True.
No consumer executable was generated. This is compiler semantic execution, not
ICK/NDK or packaged Android acceptance.

The unchanged PR46 intake/decoder ran under actual Grease on the two committed
synthetic fixtures. `tests/test_source_result_readback.grease` measures the exact
source identities and projected hashes/counts used by semantic acceptance,
rebuilds in an independent caller without acquisition and checks tampered-source
refusal preserves the prior view. It is ordinary-file host evidence; it does not
exercise the Shizuku worker or an Idriç-produced action crossing into Grease.

First integration blocker: the existing worker emits `deterministic-ok`, and
`ib_publish_owned_admitted_result` requires the published object to be its
completion record. That is not the retained payload this binding requires.
Backend recovery additionally reports actual baseline/child worker exit44 at
the first process-identity read in this executor. No PID fallback was introduced.
The qualified consumer/compiler/Android stage is absent. S03 is PARTIAL and S02
remains BLOCKED_EXTERNAL. No stock-shell/Idris/RefC/Java build substitution,
network acquisition, private import, device change, or prepaint expansion.
