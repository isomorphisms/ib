# Idriç implementation boundary

IB is an Idriç application with a Grease operating-system boundary.

The browser-owned layer is written in `.idric` and owns durable tab/history identity, ordering, sleeping policy, storage classification, index policy, inspection semantics, renderer selection, and source-preference policy. A renderer, Grease program, or Android boundary may expose facilities, but none becomes the source of truth for those decisions.

The initial source modules deliberately keep the executable boundary small:

- `IB.History` owns normalized history values, import ordering, and stable newest-first chronology.
- `IB.Index` builds transparent rebuildable list indices without collapsing duplicate visits.
- `IB.Storage` classifies schema-shaped paths and defines which records may be generically inspected.
- `IB.Inspect` summarizes physical rows without following or interpreting renderer state.
- `IB.ScientificMedia` owns HTML-before-PDF source preference and the evidence order for image naming.

## Grease boundary

Grease owns operations whose meaning is substantially "ask the operating system or another program to do this":

- HTTP GETs and retries
- temporary files and directories
- renames and moves
- invoking Idriç, PDF utilities, XML/HTML utilities, and language-model commands
- CI command sequences
- later cache-pruning and outbox-flush command sequences

This audit leaves `IB.History`, `IB.Index`, `IB.Storage`, `IB.Inspect`, and the workbench state model in Idriç. Although some of those modules contain path strings or fixture values, their work is classification/policy rather than shell orchestration.

The previous large `run:` blocks in GitHub Actions are now delegated to `bin/ci_browser_foundation.grease`. YAML still selects actions and cache behavior because GitHub requires that format; the actual command sequences live in Grease.

## Native boundary

A future Android inspector should ask native glue only for bounded metadata/read operations that need kernel enforcement. In particular, rooted no-follow opens belong at the Linux/Android boundary. Classification, visibility, paging policy, and presentation data remain Idriç decisions.

## Build

CI builds the current `isomorphisms/Idric` compiler and compiles `src/Smoke.idric`. The smoke executable exercises history ordering, duplicate preservation, indices, storage classification, protected reads, inspector readability, and scientific-media policy. Grease tests exercise the OS-facing media pipeline separately.
