# IB hostile-ingestion consumer

IB consumes the exact canonical corpus revision in `ai-ci.lock`; it does not
copy the fixtures or call the external oracle.

The Idriç candidate path now inserts `IB.RecoveredInformation` between raw HTML
tokenization and `InformationView`. Its first declared recovery contract is the
pinned corpus's `document_log_subset_v0`: source fixture identity plus ordered
tag/text events, with only the implied-end recovery needed by the useful-document
subset. It is not a browser DOM and does not claim full WHATWG recovery.

The malformed fixture exercises that contract directly. Before document
construction can pass, the recovered log must retain its source identity and
show implied closure for the malformed paragraph, table cell/row, and the
unterminated link at EOF. `InformationView` construction then has to retain the
recovered `A`/`B` table row, and downstream extraction has to recover the
`syllabus` link at `/syllabus`.

Network remains `SKIP/not_applicable_local_fixture` here. The fixture bytes are
read locally from the pinned `ai-ci` corpus, the external oracle is never called
or used as fallback, and network ownership remains in ICU rather than IB.
