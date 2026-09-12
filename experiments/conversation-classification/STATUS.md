# Experiment status at the bulk-acquisition boundary

This is an evidence-limited status report, not the requested full-corpus result.
The experiment must resume when a one-time ChatGPT export is supplied outside
Git.  Repeated per-conversation UI retrieval is intentionally not used.

1. **Branch/head.** Branch
   `experiment/conversation-classification-corpus`, based on IB `main` at
   `d199a992a70e54338e1f0dd73ffd82d6ca6bad96`.  The last published machinery
   checkpoint before this report is
   `b6085eae7dd6094f3923f4426e3deaa92d37e20e`; the final handoff supplies the
   newer exact report commit.
2. **Conversations acquired.** Zero conversation bodies.  There are 219
   title/date rows in a legacy cleanup artifact, explicitly segregated from the
   corpus.  They are not counted as conversations acquired.
3. **Source-category distribution.** The full distribution is unavailable.
   All 219 legacy rows were observed in `Uncategorized`; the 28-value weak
   proposed-destination distribution is recorded in
   `sources/title_only/report.json`.  A real import automatically emits source
   location, length, chronology, repository-reference, and redaction
   distributions before modeling.
4. **Conceptual categories.** Zero authoritative concept definitions can be
   responsibly induced without bodies.  The 28 legacy destination values are
   filing proposals, not conceptual membership.  A versioned definition schema
   and three-axis evidence representation are ready.
5. **Redaction/exclusion.** No raw conversations were committed, so corpus
   redactions and exclusions are zero.  The 219-row title artifact passed the
   safety scan without removal.  The importer mechanically records every span
   redaction, whole-conversation exclusion, and omitted internal-role message;
   a detector revision forces rescreening.
6. **Approaches implemented/tested operationally.** Exact regex rules;
   word/character TF-IDF plus structural features; independent bagged
   positive/unlabeled linear hyperplanes; dense LSA hyperplanes; dense LSA
   positive centroids; user/assistant/title weighting; structural-only input;
   and rules-plus-geometry.  LSA is not claimed to be a pretrained semantic
   embedding.
7. **Train/evaluation method.** Exact/near duplicates are grouped before a
   deterministic 70/15/15 random split and a chronological 70/15/15 split.
   Encoders and plane weights fit on train only; acceptance thresholds use
   development evidence and target 90% precision when both positive and
   negative development labels exist.  Test data is excluded from the model
   generation hash inputs.
8. **Quantitative results.** No valid full-text quantitative result exists.  In
   the weak title-only random test, micro F1 was 0.267 sparse, 0.175 dense LSA,
   0.164 centroid, 0.481 exploratory rules, and 0.535 hybrid.  These figures are
   contaminated for the rules and use assistant proposals as reference.  Exact
   figures and limitations are in `reports/title-only-diagnostic.json`.
9. **Filing-oriented results.** On the same weak random diagnostic, automatic
   coverage/misfile-among-automatic were 46.3%/57.9% sparse,
   39.0%/68.8% dense, 46.3%/78.9% centroid, 31.7%/0% exploratory rules, and
   63.4%/26.9% hybrid.  Chronological misfile rates for the three geometric
    methods were 80.0%, 80.0%, and 88.9%.  This is evidence to abstain, not an
   estimate of performance on conversation text.
10. **Representative failures.** Title geometry proposed home repairs for
    “August 12 2026 Eclipse,” music for “Mein Kampf Anti-Semitism Analysis,”
    literature for “ESCO 608 Exam Details,” and shopping for “Target weight and
    fat loss.”  Sparse category support and incidental title similarities
    overwhelmed intent.  The full-text failure phenomena remain untested.
11. **Did user-only text help?** Unknown; no user messages were available.  The
    comparison and held-out tests for full/user/assistant/title+user/weighted
    views are ready, but synthetic execution is not empirical evidence.
12. **Did hyperplanes materially beat rules?** No in the usable title
    diagnostic.  The random hybrid gained coverage but introduced a 26.9%
    disagreement rate; chronologically, exact rules had 12.5% coverage and zero
    disagreement while the hybrid had 34.4% coverage and 63.6% disagreement.
    The primary question remains unknown.
13. **Dense versus sparse.** Neither is safe on titles.  Sparse was slightly
    less bad on the random filing diagnostic; chronological sparse and dense
    hyperplane micro F1 were both 0.095.  No full-text conclusion is possible.
14. **Overlapping categories.** Independent acceptance, overlap queries, and
    multi-category metrics pass deterministic fixtures.  Whether the overlaps
    are conceptually useful on the real corpus is unknown.
15. **Incremental cost.** On 219 title rows, full sparse reclassification took
    0.166 seconds in-process.  Reclassifying one selected row parsed/classified
    one, reused 218 proposals, took 0.096 seconds, and produced a byte-identical
    proposal file.  Model loading dominates this tiny diagnostic; full-text
    cost is unmeasured.  A 48-document fixture independently verifies 47/48
    reuse after one changed input.
16. **Architecture suggested.** Preserve stable source identity; structured raw
    conversation observations; separate existing-location, concept, and filing
    axes; append-only evidence/corrections; versioned category definitions;
    immutable model generations; portable explicit planes; calibrated
    abstaining filing projections; and separate train/reclassify operations.
    Segment-level representations may be needed for drifting multi-task threads,
    but the evidence does not yet justify implementing them.
17. **Experimental code worth considering later.** The public-screened bulk
    importer and receipts, duplicate-grouped partitions, evidence reducer,
    content-hashed model/proposal boundary, separate incremental classifier,
    abstaining destination projection, and inspection commands are the parts
    most plausibly worth porting after the corpus result.
18. **Discard with this branch.** The public title ledger, exploratory title
    rules, title diagnostic partitions/reports, sklearn/joblib artifacts, and
    Python reference backend should be discarded unless the resumed experiment
    supplies evidence for a narrower preservation decision.  Nothing should be
    merged merely because the machinery runs.

## Exact remaining acquisition limitation

The signed-in ChatGPT Data Controls export initiates reauthentication and then
delivers the archive asynchronously to the account email.  No export ZIP,
`conversations.json`, `chat.html`, database, or other body-bearing bulk source
is accessible in this environment.  The available Library and prior-context
sources were searched; the title ledger is the largest result.  The next action
is to provide the export ZIP outside the Git worktree and rerun `RUNBOOK.md`.
