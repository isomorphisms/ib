# IB hostile-ingestion consumer

IB consumes the exact canonical corpus revision in `ai-ci.lock`; it does not
copy the fixtures or call the external oracle.

The Idriç `IB.Information` path reads the canonical valid and malformed HTML
fixtures directly. Network is `SKIP/not_applicable_local_fixture`, after which
identity decompression, UTF-8 decoding, HTML subset recovery, document
construction, and extraction remain independently visible.

The first receipt establishes a complete local-fixture path. The malformed
fixture deliberately records the current first boundary at
`downstream_extraction`: the document title is constructed, but the unclosed
link is not extracted. That expected FAIL is retained rather than converted to
a green extraction claim. Network ownership remains in ICU, not IB.
