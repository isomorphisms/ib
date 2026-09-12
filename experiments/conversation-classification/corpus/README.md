# Experimental corpus

Only public-screened outputs belong below this directory.  The unredacted
ChatGPT export stays outside the repository.

`raw/` preserves screened source messages.  `metadata/` preserves identity and
message structure.  Assertions and corrections are append-only.  Everything
under `derived/` and `views/` must be reproducible from raw/metadata,
definitions, assertions, and pinned run configuration.

The corpus is intentionally empty until a bulk source passes the safety gate.
The title-only cleanup ledger is kept under `sources/`, outside this corpus, so
it cannot be mistaken for full conversation text.

