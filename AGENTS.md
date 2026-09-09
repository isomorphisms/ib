# Agent instructions

## Inspect the current model before adding a parallel one

Before introducing a new browser object, interface, renderer contract, or name, read the current architecture and the nearby implementation that owns the concept. Do not invent a second model from a mock, an old branch, or conventional browser architecture when the repository already has an explicit boundary.

Current human corrections and current architecture documents outrank stale implementation precedent. If a term has been renamed because it carried the wrong ontology, do not restore the old concept merely because old files still contain it.

## Do not collapse the information pipeline

Keep source acquisition, source representation/decoding, recovery/parsing, semantic extraction, selection/projection, prepaint/view preparation, and viewport rendering as distinct responsibilities. They may be small, but do not merge them into one catch-all document/state module for convenience.

Do not render information and then reparse the rendered form as the source of truth. Durable source and semantic records feed derived views; derived views do not redefine the source.

A frontend or renderer projects browser-owned state. It does not own resource, tab, event, task, accepted organization, or semantic identity.

## Mocks must remain replaceable

A current arXiv id, article fixture, title list, phone harness, or other example is not the future interface. Put mocks behind the same semantic boundary that real Pensieve/source material will use. Do not bake today's fixture shape into viewport, storage, or browser-core types.

When the viewport needs to distinguish display kinds such as text and images, expose only the information the viewport actually needs; do not leak acquisition/parser/backend records upward merely because they are available.

## Preserve language and platform boundaries

IB application state, policy, and invariants belong in Idriç. Grease owns operating-system/process orchestration. Native Android code is a narrow platform adapter. Do not move browser policy into shell scripts, Android views, renderer state, or generic transport plumbing because that layer is easier to modify.

Python or another convenient comparison implementation is not IB runtime acceptance.

## Acceptance must exercise the intended layer

A smoke fixture, successful renderer launch, or string canary does not prove semantic identity, ordering, revision, serialization, or refusal behavior. Keep semantic acceptance at the semantic layer and platform acceptance at the platform layer; do not use one as a substitute for the other.