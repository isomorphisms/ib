# Experiment status at the bulk-acquisition boundary

This is an evidence-limited status report, not the requested full-corpus result.
The experiment must resume when a one-time ChatGPT export is supplied outside
Git.  Repeated per-conversation UI retrieval is intentionally not used.

The branch remains disposable and unmerged.  It is based on IB `main` at
`d199a992a70e54338e1f0dd73ffd82d6ca6bad96`.  The exact branch head must be
reported externally after this status commit because a file cannot contain the
SHA of the commit that contains itself.

1. **Conversations acquired.** Zero body-bearing conversations.  There are 219
   title/date rows in a legacy cleanup artifact, explicitly segregated from the
   primary corpus and not counted as acquired conversation bodies.  The source
   ledger states that only 12 of the 219 individually opened conversations ever
   exposed readable bodies, so reconstructing a body corpus from that work would
   require the expensive traversal this experiment is intended to eliminate.
2. **Bulk-source search.** The preferred source remains one ChatGPT export ZIP
   containing `conversations.json`.  On 2026-09-14 the saved-file Library was
   searched for `conversations.json`, ChatGPT export ZIP/HTML artifacts, and
   equivalent archive names; none was present.  Connected repositories were
   also searched for an already staged `conversations.json` or conversation
   archive; none was found.  No additional per-conversation account traversal
   was performed.
3. **Source-category distribution.** The full distribution is unavailable.
   All 219 legacy rows were observed in `Uncategorized`; the 28-value weak
   proposed-destination distribution is recorded in
   `sources/title_only/report.json`.  A real import emits source-location,
   length, chronology, repository-reference, and redaction distributions before
   modeling.
4. **Conceptual categories.** Zero authoritative concept definitions can be
   responsibly induced without bodies.  The 28 legacy destination values are
   filing proposals, not conceptual memberships.  The branch contains a
   versioned category schema and keeps `existing_location`, `concept`, and
   `filing_destination` as separate evidence axes.
5. **Redaction/exclusion.** No raw conversation bodies were committed, so body
   redactions and exclusions remain zero.  The bulk importer records every span
   redaction, whole-conversation exclusion, and omitted internal-role message
   mechanically.  Scanner revision `public-corpus-safety-v3` redacts credentials,
   secret URLs, financial/government identifiers, exact street addresses,
   email addresses, and phone numbers; detected child-identifying context and
   actual received/forwarded correspondence shapes can exclude a whole thread.
   An earlier overbroad rule that excluded a thread merely because it asked to
   draft/reply/send a message was removed; changing the scanner revision forces
   future imports to rescreen rather than reuse old screening results.
6. **Approaches implemented.** High-confidence regex rules; word/character
   TF-IDF plus structural features; independent bagged positive/unlabeled linear
   hyperplanes; dense LSA hyperplanes; dense LSA positive centroids;
   user/assistant/title weighting; structural-only input; and
   rules-plus-geometry.  LSA is explicitly an offline dimensional-reduction
   baseline, not a pretrained semantic embedding.
7. **Idriç boundary.** Current IB design documentation already supports durable
   observations, rebuildable derived state, overlapping category decisions,
   positive/unlabeled semantics, and explicit corrections.  No executable Idriç
   conversation classifier was found on current `main`; Python/scikit-learn is
   therefore only an empirical oracle on this branch.  Portable candidate state
   is the ordered representation definition plus explicit normal, offset,
   threshold, vote policy, and hashes—not joblib or Python runtime state.
8. **Train/evaluation method.** Exact and near duplicates are grouped before a
   deterministic 70/15/15 random split and a chronological 70/15/15 split.
   Encoders and plane weights fit on train only; acceptance thresholds use
   development evidence and target 90% precision when both positive and
   negative development labels exist.  Missing membership remains unlabeled.
   Test data is excluded from model-generation hash inputs.
9. **Quantitative results.** No valid full-text quantitative result exists.  In
   the weak title-only random test, micro F1 was 0.267 sparse, 0.175 dense LSA,
   0.164 centroid, 0.481 exploratory rules, and 0.535 hybrid.  These figures use
   assistant-proposed destinations as reference and the rules were developed
   after inspecting the title ledger, so they are diagnostics rather than
   unbiased estimates.  Exact machine-readable figures remain in
   `reports/title-only-diagnostic.json`.
10. **Filing-oriented title diagnostic.** The old direct-destination baseline
    produced random automatic-coverage / disagreement-with-weak-reference of
    46.3%/57.9% sparse, 39.0%/68.8% dense, 46.3%/78.9% centroid,
    31.7%/0% exploratory rules, and 63.4%/26.9% hybrid.  Chronological
    disagreement for the three geometric variants was 80.0%, 80.0%, and 88.9%.
    These are not results for the corrected concept-to-filing architecture.
11. **Filing architecture correction.** The executable branch previously
    documented concept and filing as separate axes but the `destinations`
    command still ranked accepted classifier categories directly.  That was a
    semantic defect.  Filing now consumes accepted conceptual memberships plus
    weak existing-location evidence through a separately versioned policy
    conforming to `filing-policy.schema.json`.  Policy support scores are sums
    of explicit policy weights, not cross-category hyperplane margins.
    Per-category margins remain explanation evidence only.  Filing can return a
    confident destination, several plausible destinations, or no sufficiently
    supported destination.
12. **Correction persistence.** Manual or explicit user filing assertions are
    read from the append-only evidence log.  The latest strongest positive
    filing assertion overrides the policy; an explicit negative filing
    assertion blocks that destination.  Neither operation rewrites source text,
    conceptual membership proposals, or trained planes.  Changing only filing
    policy invalidates filing projections, not feature rows or concept models.
13. **Representative title failures.** Geometry proposed home repairs for
    “August 12 2026 Eclipse,” music for “Mein Kampf Anti-Semitism Analysis,”
    literature for “ESCO 608 Exam Details,” and shopping for “Target weight and
    fat loss.”  Sparse category support and incidental title similarities
    overwhelmed intent.  Full-text phenomena such as assistant contamination,
    topic drift, unrelated tasks in one thread, repository-name swamping, and
    changing terminology remain untested.
14. **Did user-only text help?** Unknown; no substantial user-message corpus is
    available.  Full/user/assistant/title+user/weighted views are implemented so
    the resumed run can answer this on the same held-out partitions rather than
    by assertion.
15. **Did hyperplanes materially beat rules?** Not in the usable title
    diagnostic.  The random hybrid increased coverage but introduced 26.9%
    disagreement; chronologically, exact rules had 12.5% coverage with zero
    disagreement while the hybrid had 34.4% coverage with 63.6% disagreement.
    The primary full-text question remains unknown.
16. **Dense versus sparse.** Neither is safe on titles.  Sparse was somewhat less
    bad on the random direct-destination diagnostic; chronological sparse and
    dense-hyperplane micro F1 were both 0.095.  There is no evidence yet for
    choosing sparse, LSA-dense, or a modern semantic embedding on conversation
    bodies.
17. **Overlapping categories.** Independent acceptance, overlap queries,
    multi-category metrics, and correction overlays are implemented.  A new
    policy regression fixture also enforces that a concept such as
    `compiler_backend` can project to a differently named destination such as an
    Idriç project; classifier margin magnitude does not alter policy weight.
    Whether real overlaps are useful remains unknown.
18. **Incremental cost.** On 219 title rows, the prior full sparse
    reclassification took 0.166 seconds in-process.  Reclassifying one selected
    row parsed/classified one, reused 218 proposals, took 0.096 seconds, and
    produced a byte-identical proposal file.  A 48-document fixture verifies
    47/48 proposal reuse after one changed input.  Full-text cost remains
    unmeasured.  Filing-policy changes require no model retraining.
19. **Architecture suggested for IB.** Preserve stable source identity;
    structured source-faithful screened conversation observations; separate
    location/concept/filing axes; append-only evidence and corrections;
    versioned concept definitions; immutable model generations; portable
    explicit planes; a separately versioned filing policy; abstention; separate
    train/reclassify/project operations; and cheap content-hash invalidation.
    Segment-level representations may be needed for drifting multi-task threads,
    but the evidence does not yet justify implementing them.
20. **Experimental code most plausibly worth preserving after a real run.** The
    screened bulk importer and receipts, duplicate-grouped partitions, evidence
    reducer, content-hashed model/proposal boundary, incremental classifier,
    versioned filing-policy projection, correction semantics, and inspection
    commands.  Preservation is conditional on the body-corpus experiment.
21. **Discard with this branch unless later evidence says otherwise.** The title
    ledger copy, exploratory title rules, title diagnostic partitions/reports,
    sklearn/joblib artifacts, Python reference backend, and any policy fixture
    categories.  Nothing should be merged merely because the machinery exists.

## Validation boundary at this head

The title-only metrics and incremental timings above were generated by the
previous executable diagnostic and remain reproducible artifacts on the branch.
The source changes on 2026-09-14 add unit-test fixtures for filing-policy
separation and the narrowed correspondence exclusion.  No exact-head automated
workflow run was produced by repository-content commits in the available
GitHub connection, so those new tests are present but must not be represented as
executed CI evidence.  The full-corpus empirical claims remain intentionally
unmade.

## Exact remaining acquisition limitation

The signed-in ChatGPT Data Controls export initiates reauthentication and then
delivers the archive asynchronously to the account email.  No export ZIP,
`conversations.json`, `chat.html`, database, or other body-bearing bulk source
is accessible in this environment.  Saved files, prior audit material, and
connected repositories have been searched; the 219-title ledger remains the
largest bulk artifact.  The next executable step is to supply the export ZIP
outside the Git worktree and follow `RUNBOOK.md`.  Until then, expanding the
219-title sample would add title rows rather than the missing semantic evidence
and would not satisfy the experiment's stop condition.
