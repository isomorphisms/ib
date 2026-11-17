# Issue #100 — synthetic administrative transaction acceptance slice

Status: **PARTIAL / NOT AN END-TO-END IB ADMIN ACCEPTANCE.** Proposed against
`isomorphisms/ib` `work/tabp10-reader-complete` at
`9f468f9b6dfba8f3895f4ab5cd7a316dc08c562e`. This branch-only implementation is proposed for review; nothing has been
merged, installed, signed or released.

## Prior implementation and selected integration line

- `main` was `0f3d8dcdc20239987ce30da3d27788a90d2c090a` on inspection.
  It owns the already maintained Arena v2/PFD source and navigation semantics.
- Closed, unmerged PR #113 (`longview-heavy-page-acceptance` at
  `1cf3fb6990879ac09a35ca05e3ec461bdea2201f`) measured useful-first
  versus eight irrelevant SVG bodies, a durable immutable result and host
  restart. Its hardcoded result marker was not source-derived. It was written
  for the historical Java/WebView Longview experiment, not the current native
  Android reader.
- Closed, unmerged PR #114 (`longview-renderer-loss-acceptance`, head
  `3524599f53e4028957d9d9580079a36760dd9b3f`) retained E2/E3 action
  publication, source-derived late read, and canonical Cauldron handoff work.
  These are useful authority/recovery precedents but not current native UI
  acceptance. That branch was 234 ahead/30 behind inspected `main`.
- PR #167's native line owns `native/android/ib_reader_activity.c`,
  `ib_browser_dom_source.[ch]`, the checked `NativeReader.idric`, and the
  original ICU → Arena → PFD → source-backed native viewport. The existing
  browser-assisted WebView adapter is explicitly **Google search only**; it
  refuses arbitrary admin routes and does not authorize generic POSTs.
- PR #168's delivered F43 APK predates qualifying live Google results, let
  alone authenticated Google Cloud / YouTube Studio API-key operations.

This candidate adds a **bounded non-secret action journal** to that selected
native line, rather than copying the old Java/WebView application or creating
another canonical HTML/source store. The one-line `android-native/Makefile`
change links it into the ICK-built native reader candidate. No production
Activity/Idriç callback currently calls it; that is an explicit blocker.

## Action invariant

For each browser-minted task identity the journal accepts only:

`NEW → DRAFT → PREPARED → MAY_HAVE_APPLIED → OBSERVED_APPLIED`.

A labelled draft can be amended before prepare. The browser-owned control
must ask the user for explicit authorization; `prepare` itself does not grant
permission. The host **must commit `MAY_HAVE_APPLIED` durably before sending**
a state-changing HTTP request. A lost response, timeout, app switch, renderer
loss, host death or boot leaves it uncertain; **no automatic replay**. An
independently authenticated, operation-ID-bound read-after-write observation
may then mark ordinary completion without repeating the mutation. If this
proof is unavailable, show uncertain state and request a deliberate user
reconciliation, not a retry.

The store uses immutable, owner-only 0600 event files in a 0700 subtree,
`openat`/`O_NOFOLLOW`, exclusive writes and file+directory `fsync`. It rejects
invalid event ordering, damaged records, stale writers and symlinked
subdirectories. Its total count is bounded to 64 events. It stores only a
restricted, site-approved ordinary label, opaque task and operation IDs, phase
and revision. It never accepts a response body, key, password, cookie, OAuth
code, query string, URL, or untrusted DOM as a completion credential. The
caller must enforce the ordinary-field allowlist and independently verify a
remote status observation; the journal itself cannot authenticate a site.

The existing canonical Arena must continue to own ordinary source byte
receipts, immutable identity/digests, and recovered display. The action
journal is **not** a replacement for `ib_arena_source_owner`, an API-key vault
or an authority to replay network requests.

## Test components and specific assertions

- `tests/admin/test_journal.c`: compiled native action-policy behavior across
  close/open, ambiguous send, wrong operation ID, stale writer, damaged
  immutable event and symlink. Host `clang` is test-only; Android production
  compilation still uses pinned ICK/NDK.
- `tests/admin/http_fixture_acceptance.py`: actual loopback HTTP service
  with cookie-gated synthetic login, small useful table and Create control,
  eight delayed 256-KiB SVG bodies, explicit verification endpoint, one-time
  volatile synthetic key, success, and a POST whose server mutation succeeds
  while the reply connection is intentionally severed. The host invokes
  the **same compiled C journal** in separate processes before and after
  mutation. It requires exactly one POST for each operation, status
  read-after-write after a fresh login, and no key/cookie/password in event
  files or receipts. This verifies real HTTP bytes, not browser rendering.
- `tests/admin/hosted_browser_acceptance.py`: additional **real Chromium**
  interactive browser observer, executing the identical server and compiled C
  journal. It includes actual DOM visibility, click, temporary second-tab
  interruption, closed Chromium process and fresh login/reconciliation.
  It is a hosted diagnostic, **not** an installed IB or Android Activity test.
  The available local Chromium has an administrator `URLBlocklist: ["*"]`
  and refused the local URL (`ERR_BLOCKED_BY_ADMINISTRATOR`), so the browser
  observer is **NOT_RUN TO COMPLETION** in the current environment. Do not
  disable policy or report browser acceptance. The proposed workflow installs
  a pinned Playwright browser for an independent CI attempt after approval.

## Local evidence — 2026-11-17

From this unpushed candidate, using the local C compiler and synthetic loopback
fixture:

| Obligation | Observed result |
| --- | --- |
| Native C safety/recovery tests | PASS |
| Actual loopback authenticated HTTP | PASS (synthetic only) |
| Both state-changing operations sent | 1 POST each |
| Ambiguous completed POST automatically replayed | 0 |
| Operation result state reopened in new process | PASS, ordinary status only |
| Secret present in immutable journal | NO |
| Delayed irrelevant SVG bodies | 8 × 262,144 = 2,097,152 bytes |
| All HTTP response body bytes served by fixture | 2,104,441 bytes (one run) |
| Bytes served when useful HTTP document was retrieved | 4,653 bytes (one run) |
| First useful HTTP document retrieval | 0 ms rounded on loopback; **not UI first paint** |
| Actual journal disk-block growth | 32,768 bytes on host filesystem |
| Fixture HTTP runner peak RSS | Measured in `evidence/http.tsv` (Linux `ru_maxrss`); **not Android/browser RAM** |
| Browser/process peak memory, decoded image memory | NOT_MEASURED |
| Real Chromium interactive observer | BLOCKED by managed URL policy |
| Native IB Activity/retained Arena viewport | NOT_RUN |
| API29/API34 installed ART | NOT_RUN |
| Physical MIRO A1/C67 | NOT_RUN |
| Google Cloud or YouTube authenticated workflow | NOT_RUN |

`evidence/http.tsv` is a raw, secret-free host receipt for this run. Numbers
are scenario-specific, not latency/memory forecasts for the phone. The server
body counter is successful response-body bytes written, **not** TLS or TCP
wire octets. The journal disk number is allocated host blocks, not Android
app-data delta. An actual browser-owned first useful paint or peak total RAM
has not been measured.

## Before any acceptance or release

1. Wire an **explicit test-only** `127.0.0.1` administrative fixture route
   through the maintained checked Idriç Activity and existing browser adapter,
   without weakening the real Google search route policy. Connect the C action
   journal to browser-owned task identity, user approval, the renderer's real
   click/POST event, Activity lifecycle and the protected-session authority.
   Never let page JS mint browser authority.
2. Carry the ordinary table/result through the existing Arena admission,
   source-provenance and PFD readback paths; validate later native viewport
   reopen without reacquisition. Secret material needs an explicit live-only
   reveal/copy boundary entirely outside ordinary receipts and source logs.
3. Add independently observed on-device first useful UI paint, response-body
   bytes, transferred/wire bytes where supported, foreground absence, peak
   host+renderer memory, actual disk growth and capacity policy (#95/#96).
   Exercise Activity recreation, renderer death, host death, Shizuku loss and
   restart, and reboot separately. No real-phone disk-fill test.
4. Qualify the new source/Makefile under exact-head pinned ICK and Android
   API29/API34, then an authorized MIRO A1 physical installation. Existing
   signer/package/version and no-uninstall upgrade rules remain mandatory.
5. Only then select a real Google Cloud or YouTube Studio API-key operation.
   Provider-approved sign-in/verification, per-origin renderer profile,
   authenticated remote status, one-time key reveal/copy, account permissions,
   and explicit human confirmation are currently unimplemented/untested. A
   Google Search DOM derivative cannot establish these behaviors.

**No deployed change, APK or actual Google/YouTube qualification is claimed.**
