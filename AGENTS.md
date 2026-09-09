# Agent instructions

## Cross-repository anti-patterns

These rules apply in addition to stricter repository-specific rules below.

- Claim only the boundary actually exercised. Source presence, fixtures, generation, compilation, packaging, installation, launch, smoke checks, semantic execution, backend execution, and physical-device execution are different evidence levels. If a stronger boundary was not exercised, report it as unverified.
- The named mechanism is part of acceptance. Do not substitute a fallback, oracle, mock, alternate backend, alternate executable, lookalike renderer, or conventional nearby toolchain and keep the original label.
- Do not weaken acceptance to obtain green. Repair the implementation. Change the contract only when the requirement itself is shown to be wrong or obsolete, and keep that semantic decision explicit. Targeted negative tests must fail for the intended reason when the distinction matters.
- Keep semantics independent of convenient representations. Mathematical, domain, and language objects are not defined by tuples, matrices, compiler nodes, ABI records, transport bytes, storage shapes, or UI payloads unless the semantics explicitly say so.
- Current explicit human corrections and current architecture outrank stale source, generated code, upstream conventions, older branches, bootstrap precedent, and familiar practice. Do not restore a rejected abstraction under its old name or a near-synonym.
- Acceptance belongs to an exact head and its material pins. An ancestor's, sibling branch's, or previous pin's green result is historical evidence only.
- Mocks, fixtures, harnesses, and today's platform adapter must cross replaceable interfaces; they do not get to define the permanent architecture merely because they are currently convenient.
- Preserve the repository's chosen implementation path and layout before introducing familiar infrastructure. Where `_` is an established machinery boundary, keep build/package/generated/test/compiler material there and preserve canonical source and intended soft links.

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

Do not assign permanent semantic ownership merely from the implementation language. The current shell-first Pensieve/Cauldron line treats shell as a serious implementation and orchestration layer, including filesystem-visible state. Use typed Idriç components where the ontology or invariants justify them, shell/Grease where orchestration and visible state fit, and native Android as a narrow platform adapter.

Do not move browser semantics into renderer state, Android views, or generic transport plumbing merely because that layer is easier to modify. When language ownership is still evolving, follow the current branch architecture instead of freezing a temporary allocation here.

Python or another convenient comparison implementation is not IB runtime acceptance.

## Acceptance must exercise the intended layer

A smoke fixture, successful renderer launch, or string canary does not prove semantic identity, ordering, revision, serialization, or refusal behavior. Keep semantic acceptance at the semantic layer and platform acceptance at the platform layer; do not use one as a substitute for the other.