# First execution wave

Both assignments were displayed in chat before the in-session workers started.
The exact dispatch identifiers are in `execution.tsv`. They are not Flexible
Pipes dispatch IDs; no deployed FP delivery adapter or automatic SUN queue run
is claimed. The three family audits were reconnaissance, not implementation.

## Q01

[isomorphisms/ib PR #121, “Reject exact-text compiler and oracle producer failures”](https://github.com/isomorphisms/ib/pull/121)
is draft at `c4a7b9bdf2ceac42b4f0aaa7d903255e307d22f3`.
The workflow now obtains producer output directly before displaying it, so the
producer exit status remains visible. All twelve semantic comparisons remain.

The actual maintained workflow run bodies were extracted and exercised under
their existing runner contract, with Grease owning the regression orchestration.
Only temporary locations were substituted. The independently fetched published
head produced:

| Control | Compiler status | Oracle status |
| --- | --- | --- |
| Success | 0 | 0 |
| Wrong output | 0 | 1 |
| Expected lines followed by exit 7 | 0 | 7 |
| Plausible compiler artifact/log followed by exit 7 | 7 | Not run |

Restoring the original workflow falsely accepted both exit-7 cases and caused
the regression to fail. This demonstrates the masking defect and narrow repair.

The injected regression is **not yet automated in CI**. IB has no qualified,
durable Grease provisioning path. The observed sibling artifact expires October
13, and its Cat Food provisioner also installs unrelated model runtimes/builds
ICK. That is not a sound narrow CI dependency. Regression/fixture changes trigger
the workflow, but triggering does not mean the new harness runs. Existing Chez
and build-policy debt is also unresolved. This job is not merge-ready.

At the recorded snapshot, universal gate run 37808933889 passed; exact-text run
37808933768 and foundation run 37808933776 were in progress. These mutable checks
must be refreshed before further claims. Physical Android acceptance is NOT_RUN.

## B01

Published implementation `cc97557718cd46ddee7fef41fbad3eb048b1aa00` and receipt
head `5205209f189cb3813d18ec251a273d59f369e50e` are on
`fix/shizuku-authority-status`, a child of the retained Longview head. The retained
branch and its closed integration request remain unchanged.

Failed UID queries now preserve their exact status. Malformed output produces
status 2 and no authority. Successful UID 0/2000/ordinary controls and provider
exits 37/124/13, missing/nonexecutable providers, and malformed/multiline output
were checked through Shizuku and the Binder-facing caller.

Both hosted suites passed again after fetching the published implementation.
Published tree hashes matched the tested local trees. Restoring the original
status collapse failed with `ib_shizuku_authority returned status 1; expected 37`.

The real pinned Grease runtime needed its inherited shell-compatibility option
for existing suite syntax. The external fake-rish fixture remains a mock. This
is hosted adapter evidence, not default-mode language qualification, actual
Binder/Shizuku authority, Android process survival or physical acceptance.

## Remaining queue

Twenty-seven assignments are not dispatched. Their full text, precise starting
conditions and dependencies are in `index.md` and the individual text files.
No tracker was closed, no change was merged and no device or account was mutated.
