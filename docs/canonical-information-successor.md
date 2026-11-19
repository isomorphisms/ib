# Canonical information successor

isomorphisms/ib PR #48, “Rewrite canonical IB slice in current Idriç style”
(https://github.com/isomorphisms/ib/pull/48), owns this independent line.
Its historical head f4f7803d2f097f86836fe42399138c4c21ea380f contains the
entire exact head a2fed1e0d7da9dbc348b12ba82b1a6aba9eaf79f of
isomorphisms/ib PR #47, “Define canonical Idriç information and prepaint model”
(https://github.com/isomorphisms/ib/pull/47).

The successor preserves source-backed Strand, six heading levels, the closed
Heading/Text/Link/TableRow/Form/Image block sum, checked nonempty references
and table rows, zero-capable increasing revisions, partial/complete prepaint,
and version-1 serialization. IB.Information remains the separate extractor;
neither Longview nor Pensieve supersedes this model.

The old exact-head foundation run 34351678892 failed because the smoke fixture
named a local `partial`, a reserved declaration keyword. The local is renamed
`partial_prepaint`. The integration retains main's current compiler bootstrap,
exact-head checkout, hostile-input, occurrence, and paint-budget assertions,
and restores all canonical smoke assertions with direct child exit handling.
Fresh current-head compilation and execution are required before merge.
No previous failure or skipped downstream job is counted as acceptance.
