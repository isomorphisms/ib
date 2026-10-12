# First useful Pensieve projection — IB-D07

Owner: [issue #118](https://github.com/isomorphisms/ib/issues/118).
This is an **unverified source implementation**, not a running screen.
`IB.PensieveHome` consumes the existing `IB.HumanDecisions` reducer from
[isomorphisms/ib PR #125, “Add source-linked human decisions and retained-item
reducer”](https://github.com/isomorphisms/ib/pull/125), parent
`cb8cdc14b6065b91240b97b580055c0d18375fae`.

The producer supplies already retained source references, committed decisions,
retained activity references and source-bound display descriptors. Candidate
selection, save/defer/return/exposure ranking and unchanged-evidence dismissal
remain S08's responsibility. The view never creates its own corpus, history,
retention classifier or decision journal.

The projection adds bounded rows, exact supplied title/excerpt/source labels
and a plain explanation of the core's activity reason. Each row carries the
original target/source/revision and chosen activity identity. Two equal titles
remain different items. Empty input yields an honest source-intake explanation.
No recommendation/news/promotional input is added by this projection.

Missing, denied, oversized and conflicting display descriptors yield a generic
unavailable row with no old title/excerpt/source label. Admitted source identity
is distinct from permission to display its bytes. The source owner must supply
only permitted presentation and recheck access at execution. This function
neither hashes source bytes nor authenticates caller assertions.

Keep, Dismiss, Done and Later prepare an existing S08 Decision after rechecking
the currently projected row and rejecting recycled/stale identities. They do
not modify the committed journal. Open source returns the exact existing
SourceReference; its actual navigation adapter remains required. Pending,
failed and unknown persistence reports have different feedback from a recorded
decision. A prepared command or animation is not a durable receipt.

The output budget caps rows at 64, title/source labels at 256 decoded Unicode
characters and excerpts at 1024. The ordinary default is 32 rows. These are
presentation bounds, not input-processing/RAM or encoded-byte claims. No source
content is normalized or retyped by the view. Edit remains blocked on its
semantic command owner, and optional Keep integration stays in D10.

## Verification attempted

The preserved generated example has 26 acceptance cases over the production
functions, including browser/mail/note activity, priority, exact Unicode and
whitespace, source binding, denial/ambiguity, output bounds, four commands,
stale revision/activity, recycled rows, journal replay, no optimistic
persistence and failed/unknown-save feedback. All are **NOT_RUN**.

The actual compiler invocation returned 127 (`idris2: command not found`).
This executor has no installed Idriç compiler; the inspected published releases
have Android ARMv7 artifacts, and the current Linux compiler-build artifact
contains acceptance evidence rather than a compiler distribution. No stock
Idris, Chez consumer, Java, Gradle, Python, RefC or unqualified compiler build
was used as a replacement. Whitespace and existing public-vocabulary source
checks pass; neither is a typechecking or behavior receipt.

The adjacent row-binding and failed-save mutation patches are preserved test
inputs. They must apply to the delivered source, typecheck, and make their
intended assertions false before mutation rejection can be claimed. No such
execution has occurred yet.

Next command sequence on a qualified checker: check the production module,
then check the generated example with that exact source's compiler cache;
evaluate `home_cases` and compare every named result; repeat in isolated trees
with the two supplied mutants and restored production. Preserve checker status
and distinguish normalization from consumer/backend execution.

## Remaining D07 boundary

Source/type/behavior qualification comes first. The actual retained-activity
and permitted display producer, S08 durable executor and error reports,
Material renderer/actions, Edit owner, native process-loss reconstruction and
physical phone usability are still incomplete. No screen or APK was produced.
The source-review branch intentionally opens no duplicate PR or automatic
inherited consumer-build path while its checker/runtime boundary is absent.
