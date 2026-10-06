# Shell-first Cauldron and Pensieve

The independent 0.2 arXiv path starts with persistent information. Cauldron
retains original HTML/PDF/figures, source URLs and acquisition time; distillation
produces locally searchable text, title/link records and figure manifests in
Pensieve. Indexes remain rebuildable. A tab or renderer is outside this slice.

`bin/ib` provides `fetch`, `distill`, `add`, `search`, `reindex`, and `paths`.
`add` acquires, distills, then rebuilds the exact-text file-list index. Search
uses retained local text without reacquisition. Persistent data defaults to
`${XDG_DATA_HOME:-$HOME/.local/share}/ib`; `IB_HOME` selects a disposable or
alternate root and `IB_ICU` names the ICU acquisition executable.
Modern and one-component legacy arXiv IDs are validated before acquisition or
replacement. Resolved item parents must stay within the intended storage root,
including when a valid legacy ID crosses a symlink.

Executable `hooks/after-distill.d/` hooks receive Pensieve and Cauldron item
paths. The body-hyperplane experiment is a derived consumer: partitions remain
independent of labels, non-fit rows cannot train planes or enter provisional
unlabeled samples, and external text declares its representation contract.
The frozen proposal policy is unchanged. Its inherited Python tools remain
experimental migration debt, not IB runtime or Idriç acceptance.

The reading contracts remain in `reading-feedback.md`, `reading-assistance.md`,
and `reader-support-system.md`. This line does not retire the canonical Strand
model or Longview/Binder architecture. README.md retains current main verbatim;
the 0.2 overview lives here to keep independent design lines explicit.
