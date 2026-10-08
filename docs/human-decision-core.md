# Human decision core — IB-S08

`IB.HumanDecisions` is the sole new decision owner for the bounded issue #103 /
#118 slice. Proposals and human decisions are separate values. The core checks
available source/target/revision references; category IDs are restricted stable
identities, not display names. Adding/removing one category preserves other
memberships. Attention is a separate projection. Explicit dismissal/done binds
the evidence revision; unchanged evidence cannot reappear until a human
keep/later action reverses it.

`useful_candidates` consumes retained activity references from the existing
browser/source owner, ordered save > defer > return > exposure. It emits exact
source/target/revision identity, the activity identity and an explicit reason.
The supplied availability list must come from the retained-source owner, not a
view's assertion that bytes exist. No fixture adapter has been promoted to
that production authority.

The canonical codecs are strict ordinary TSV. Unknown/extra fields, unknown
actions, command payloads, invalid IDs, duplicate decision IDs, dangling
supersession and incomplete commit framing refuse. `append_serialized_decision`
replays an existing framed journal and validates a new explicit human action
against current source references before producing the replacement bytes.
Identical replay is idempotent; changed records with the same identity refuse.
A rollback appends another action rather than deleting previous records.

The bounded v1 journal allows at most 4,096 decisions and 1 MiB of decoded text.
Those limits bound this first record adapter, not the total retained corpus.
Rotation/compaction is not implemented. A terminal marker binds record count
and final decision identity, not authenticity or platform durability. Historical
replay does not imply that referenced bytes remain available.

Actual clean Idriç `ff4d852862a3942592f8ade9afde8d409d9803be` typechecks the
core and assertions and evaluates positive, negative and recovery controls.
Its wrapper SHA-256 is
`55bd2f65fde3fd6093048e75369002d14f12db8f12258778d197376e5f37c0bf`;
compiler image SHA-256 is
`10002074cfae31a15e6f136cd191b6abe9f8d58efcec438d21145e72802b2819`.
The compiler REPL evaluates core assertions to True, the deliberately broken
dismissal to False, and the repaired dismissal to True. Codecs/refusal/replay
checks also evaluate True on the actual core.

First blocker: IB has no qualified ICK/NDK stage producing this semantic
validator executable. A bounded Grease durable journal adapter therefore
remains incomplete; no shell implementation of semantic policy was substituted.
Production retained-activity wiring and D07 presentation must consume this
owner after that exact executable/store contract is qualified. Disk crashes,
CI automation, Android and physical device results remain NOT_RUN.
