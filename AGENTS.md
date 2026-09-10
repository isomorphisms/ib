# Agent instructions

Apply the shared evidence and acceptance guardrails in
`isomorphisms/ai-ci/AGENTS.md`. The rules below are IB-specific.

Before changing browser objects, ownership, interfaces, or implementation
boundaries, read [`README.md`](README.md) and [`docs/architecture.md`](docs/architecture.md).

## Do not invent a parallel browser model

Inspect the current owner of a concept before introducing another resource,
tab, event, task, representation, proposal, renderer contract, or competing
name. Current architecture and explicit human corrections outrank mocks, old
branches, and conventional browser structure.

Do not let a fixture, frontend, renderer, Android view, or transport record
become the source of truth merely because it is convenient to edit.

## Keep the information pipeline separated

Keep acquisition, source representation/decoding, recovery/parsing, semantic
extraction, selection/projection, view preparation, and viewport rendering as
distinct responsibilities. They may be small; do not collapse them into one
catch-all state or document module.

Do not render durable information and then reparse the rendered form as the
source of truth. Durable source and semantic records feed derived views, not the
reverse.

## Preserve replaceable boundaries

Mocks, current article ids, title lists, phone harnesses, and other examples
must cross the same semantic interfaces intended for real sources. Do not bake
today's fixture shape into browser-core, storage, or viewport types.

Follow the current language boundary in the README: Idriç owns browser semantics
and invariants, Grease owns orchestration and OS-visible work, and native Android
is a narrow platform adapter. Python/Ithon may be disposable comparison tools;
they are not IB runtime acceptance.

## Test the layer being claimed

A renderer launch, string canary, or phone smoke test does not prove semantic
identity, ordering, revision, serialization, recovery, or refusal behavior.
Keep semantic acceptance at the semantic layer and platform acceptance at the
platform layer.
