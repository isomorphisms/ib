# IB-R1 — 29 complete assignments

Each linked text file is self-contained, including exact source aliases and the shared execution contract. [Full literal queue](full-queue.txt) contains the same assignment bodies together. The family recovery files preserve source findings and candidate dispositions; the individual jobs are authoritative after parent reconciliation.

`READY_CI` means the hosted slice may start after its stated checks; it never means that a compiler/runtime is present or that CI/device acceptance passed. Two jobs executed; 27 remain undispatched. The queue does not create 29 new GitHub issues.

| Assignment | Starting state | Dependencies | Execution |
| --- | --- | --- | --- |
| [SUN IB-Q01 — Enforce exact-text producer failures](IB-Q01.txt) | READY_CI | none | IMPLEMENTED_HOST; AUTOMATION_BLOCKED |
| [SUN IB-D01 — Qualify the selected Android view boundary](IB-D01.txt) | READY_CI | none | NOT_DISPATCHED |
| [SUN IB-D02 — Feed the Material view from live retained projections](IB-D02.txt) | BLOCKED_EXTERNAL | D01,S03,canonical owner | NOT_DISPATCHED |
| [SUN IB-D03 — Recover useful pinned/recent and collapsible navigation](IB-D03.txt) | READY_CI | adapter:D01,D02;store:S05 | NOT_DISPATCHED |
| [SUN IB-D04 — Display prepared scientific figures without cropping](IB-D04.txt) | BLOCKED_EXTERNAL | D01,D02 | NOT_DISPATCHED |
| [SUN IB-D05 — Wire trusted exact Copy through browser policy](IB-D05.txt) | READY_CI | adapter:D01,D02 | NOT_DISPATCHED |
| [SUN IB-D06 — Connect source-linked view observations to the core ledger](IB-D06.txt) | READY_CI | S06,S07,D01,D02 | NOT_DISPATCHED |
| [SUN IB-D07 — Show retained user intent on the first useful Pensieve screen](IB-D07.txt) | BLOCKED_EXTERNAL | S08,D01,D02 | NOT_DISPATCHED |
| [SUN IB-D08 — Make discovered images directly actionable and measure the task](IB-D08.txt) | READY_CI | adapter:D01,D02,D04 | NOT_DISPATCHED |
| [SUN IB-B01 — Preserve authority failure status through Binder](IB-B01.txt) | READY_CI | none | IMPLEMENTED_HOST; DEVICE_NOT_RUN |
| [SUN IB-B02 — Bind IB capability provenance to the actual rish bundle](IB-B02.txt) | READY_CI | serialize:S01 | NOT_DISPATCHED |
| [SUN IB-B03 — Make worker lifetime acceptance restartable across callers](IB-B03.txt) | READY_CI | B01 | NOT_DISPATCHED |
| [SUN IB-B04 — Bound live PFD sessions and prove unauthorized access refusal](IB-B04.txt) | READY_CI | qualified Android stage | NOT_DISPATCHED |
| [SUN IB-B05 — Execute the distinct MIRO live-channel and durable-reader gates](IB-B05.txt) | BLOCKED_EXTERNAL | B04,physical A1 | NOT_DISPATCHED |
| [SUN IB-B06 — Bound provider observation time and output](IB-B06.txt) | READY_CI | B01 | NOT_DISPATCHED |
| [SUN IB-B07 — Resume the preserved latency correction with current evidence](IB-B07.txt) | READY_CI | qualified Android stage;serialize:S03,D02 | NOT_DISPATCHED |
| [SUN IB-B08 — Finish the existing credential-safe Drive adapter only at its real gates](IB-B08.txt) | BLOCKED_EXTERNAL | private signer,enrollment,C67 | NOT_DISPATCHED |
| [SUN IB-B09 — Close the gap between owned-process validation and signaling](IB-B09.txt) | READY_CI | edit:B01,B06;coordinate:B03 | NOT_DISPATCHED |
| [SUN IB-S01 — Reconcile storage observations before admission](IB-S01.txt) | READY_CI | serialize:B02 | NOT_DISPATCHED |
| [SUN IB-S02 — Enforce decoder growth limits at the real boundary](IB-S02.txt) | BLOCKED_EXTERNAL | S03 decoder | NOT_DISPATCHED |
| [SUN IB-S03 — Persist source-derived Longview results in Cauldron/Pensieve](IB-S03.txt) | READY_CI | stable Longview integration;serialize:B09 | NOT_DISPATCHED |
| [SUN IB-S04 — Confine and enumerate durable task records safely](IB-S04.txt) | READY_CI | qualified Android stage for execution | NOT_DISPATCHED |
| [SUN IB-S05 — Preserve ordinary tabs and stable navigation events](IB-S05.txt) | BLOCKED_EXTERNAL | S04 | NOT_DISPATCHED |
| [SUN IB-S06 — Persist source-linked fragments and ordered strands](IB-S06.txt) | READY_CI | canonical owner contract | NOT_DISPATCHED |
| [SUN IB-S07 — Journal private source-linked view observations](IB-S07.txt) | BLOCKED_EXTERNAL | S06 | NOT_DISPATCHED |
| [SUN IB-S08 — Persist human decisions and reversible organization](IB-S08.txt) | READY_CI | none | NOT_DISPATCHED |
| [SUN IB-S09 — Resume a bounded investigation frontier](IB-S09.txt) | BLOCKED_EXTERNAL | S05,S03 | NOT_DISPATCHED |
| [SUN IB-S10 — Export and reimport selected retained evidence](IB-S10.txt) | READY_CI | pinned Pensieve | NOT_DISPATCHED |
| [SUN IB-S11 — Require retention evidence before cache clearance](IB-S11.txt) | READY_CI | coordinate:S08 | NOT_DISPATCHED |

Parent reconciliation: S03 owns the whole source-to-durable-result path; D02 only consumes it. S08 includes the deterministic retained-intent reducer needed by D07, alongside durable decisions. S01 and B02 share capability observations but have different consumer responsibilities. Serialize their edits. B09 records the observed validation-to-signal race without claiming exploitation. No append-arena implementation job is emitted before that adapter is inspected.
