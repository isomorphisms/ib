# Deterministic protected-admin fixture

This is the simulated remote application for `IB-ADMIN-LONGVIEW-E2E-01`.
It exercises the actual native IB protected renderer. It is not an alternate
browser, a provider authorization adapter, or Google/YouTube acceptance.

## Types and effects

`AdminFixture.idric` defines the fixture's remote state: verification,
accepted usable-control actions, one-time-result deliveries, and ambiguous
remote operations. Requests are a closed set of fixture actions. Their
result is either refusal, a confirmed ordinary action, one synthetic result,
or an intentionally incomplete response. The pure transition function does
not claim to execute network IO or to model IB's task state.

Required operations are `step : Action -> State -> (State, Outcome)` and
`checks : Bool`. A fresh server starts unverified with all counts zero.
Read-only navigation never increments a remote-operation count. One-time
delivery happens at most once; every attempted mutation is counted so a
replayed request cannot disappear behind idempotence. A simulated ambiguous
operation increments its effect counter before truncating the response.

The desired Idriç semantic contract is separate from socket operations.
`server.c` is the narrow test-only POSIX socket/file/process adapter, built
through the explicitly supplied qualified ICK executable. It uses no Python,
Node, Java application source, or product network code. JavaScript in the
HTML and `ready.js` is the requested external web-application input; it is
not an IB implementation fallback.

## Endpoints and observations

`/` serves the admin page; `/ready.js` becomes available after a bounded
delay. Four irrelevant 256 KiB asset responses run independently, so they
cannot serialize the useful control behind their delays. The fixture
requires an actual action on `confirm-control`, confirmed by the independent
server's `usable_actions` counter. A ready-state string cannot satisfy it.

`/verify` provides a mock verification step; its explicit button sends a
POST to `/verification-complete`, setting a fixture-only HttpOnly cookie.
This is synthetic remote state, not evidence of real authentication.
`/action` returns a fresh synthetic one-time value exactly once.
`/ambiguous` commits one synthetic effect and closes an intentionally short
response; the application must report uncertainty and must not resubmit it
on renderer or host recovery. `/fake-interactive` has a claimed interactive
marker and an unusable control. `/partial` closes a short main response.

`/counts` returns only fixed names and decimal counters. No response body,
cookie, URL query, one-time value, or user data is logged by the server.
The independent observer must not save a screenshot while the one-time
value is present. After checking it without emitting its bytes, erase it
from the DOM before any screenshot or log collection.

The HTTP diagnostic controls are separate from those remote actions.
`/mainframe-503` returns an actual 503 HTML response with the fixed
`mainframe-http-error` marker. `/subresource-404` returns 200 HTML with the
`subresource-control` marker and requests `/missing-asset-404.css`, which returns
404. `/http-error-counts` exposes only `mainframe_503`, `subresource_page`, and
`subresource_404` counts of fully written responses. These routes leave the
original `/counts` schema and values unchanged, including `parent_served_bytes`; that original
byte counter excludes all new HTTP diagnostic responses. No verification,
usable action, one-time delivery, or ambiguous-operation count is changed.

## Invocation and evidence boundary

From any directory, supply absolute paths:

`grease verify-server.grease IB QUALIFIED_ICK NEW_EVIDENCE`

This compiles and exercises the actual HTTP fixture, including the denied
pre-verification action, single result delivery, ambiguous committed effect,
read-only reload, and empty state on a separate fresh process. A passing
host server test is fixture evidence only. The separate independent Android
observer must exercise the installed IB controls and WebView and bind its
receipt to source, APK, signer, version, SDK, ABI, and WebView identity.

Host qualification is retained in `host-qualification.tsv` and
`host-source-digests.txt`. On Linux x86_64, qualified Idriç frontend
`740bbf2b1d9e811fd00c719c3473eede17ceea3b` checked this module and executed
`--exec main`, which printed `True`. The qualified ICK executable compiled the
actual socket server. The Grease coordinator then exercised real HTTP requests,
one-time-delivery counts, false cookie-prefix refusal, a truncated ambiguous
response, read-only reload and a separate fresh process. All passed. The server
emits no one-time value in its stdout/stderr or counter receipts.

`http-error-qualification.tsv` and `http-error-source-digests.txt` preserve the
later HTTP diagnostic fixture checks. The original server fails the new 503
assertion at the intended missing-route boundary. The corrected server returns
503/200/404 with the exact markers and stylesheet reference, produces separate
1/1/1 counts, preserves the original counters byte for byte, and resets its new
counters on a fresh process. Those are actual host HTTP checks; they do not
establish that Android delivered either WebView callback.

The independent observer is maintained in `isomorphisms/ai-ci/android-admin`.
It drives actual pointer events, injects the actual synthetic result into the
native numeric timing callback, checks stale callbacks, terminates the renderer,
recreates the Activity and observes host/update recovery. It records ordinary
identities and generations, not the result. Installed Android assertions,
the public-page ladder and physical C67 acceptance remain NOT_RUN until exact
target receipts exist. No real credentials or live-provider writes are used.

## Remaining language boundary

The fixture model requires only current Idriç algebraic data, records,
natural-number counting, pairs, and equality. The missing runtime facility
for an Idriç-only HTTP adapter would be qualified nonblocking socket and
bounded response IO; no such facility is assumed here. The declared native
test transport does not qualify an Idriç network implementation.
