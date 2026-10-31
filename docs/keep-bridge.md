# Google Keep bridge — IB issue #165

This is a bounded, optional export handoff, stacked on S08 human decisions
(PR #125) and the Pensieve projection (PR #135). It is not Keep sync.

## Ownership and explicit permission

The new S08 export_to_keep action serializes as export-keep in the existing
human-decision journal. Neither a model proposal nor Pensieve's ordinary
keep_item authorizes external publishing. Pensieve prepares the new action
only for a currently readable, source/revision-bound row. A later Dismiss or
Done revokes any unexecuted export permission.

IB.KeepBridge validates the committed S08 journal and current retained source
identity, then obtains permitted source text, the exact original source URL,
and a provenance label. The source owner must authorize releasing those bytes
and check for private URL/query secrets. The editor's text is the original
body followed by a Source: URL line. Local metadata retains the source,
target, revision, provenance label and distinct export-decision identity.

A local request identity of keep.<decision-id> is stable across retries and
distinct from a possible remote note ID. Bodies and authentication secrets
are never placed in the delivery snapshot.

## Delivery and restart state

The Idriç reducer alone controls the strict, bounded delivery record. A native
C operating-system helper performs atomic app-private snapshot replacement
using write, fsync, rename and directory fsync, or bounded read. It cannot
authorize or reinterpret a decision. The app must persist in-flight BEFORE
launching Android; after process loss that state is recovered as uncertain,
blocking automatic retry. A successfully launched editor is awaiting-user,
not confirmed; a user must inspect Keep and explicitly confirm note creation.
A missing handler is unavailable. Other launch errors or ambiguous exit
statuses are uncertain. Confirmed, declined, awaiting-user and uncertain
states never automatically resend, avoiding accidental duplicate notes.
Unavailability can be retried only by a deliberate user action.

This is a single-item snapshot codec and narrow persistence primitive;
production S08 executor/wiring and multi-item index remain unfinished. The
opaque snapshot never stores note text, a password or bearer token.

## Android boundary

Native Activity callers can use ib_keep_start_share with a JNI environment,
Activity object and checked UTF-8 share payload. It sends ACTION_SEND with
type text/plain, EXTRA_TEXT and target package com.google.android.keep. The
implementation is C/JNI with no first-party Java or Kotlin. It handles
ActivityNotFoundException and returns STARTED_UNCONFIRMED, NO_HANDLER or
FAILED_OR_UNKNOWN. It requires neither Shizuku nor a package-list query.

For an explicit Shizuku/rish path, use:

    sh lib/android_keep_handoff.grease rish /private/accepted-payload

Or from an Android shell able to start activities directly:

    sh lib/android_keep_handoff.grease direct /private/accepted-payload

Only an authorized IB export executor should supply the payload file. The
shell handoff accepts 1–32768 bytes, preserves Unicode, quotes and trailing
newlines, refuses symlink payloads and malformed arguments, targets the Keep
package and does not echo the body. Its status means started-unconfirmed,
unavailable or outcome-unknown, never saved. Because rish -c exposes the note
payload transiently in process arguments, do not use rish for secret bodies;
prefer the in-app native adapter.

Package visibility may affect preflight queries on Android 11+, but does not
prevent an app from attempting a share Intent. Handler resolution, prefilling
and final note creation require separate physical confirmation.

## Independent authenticated ChatGPT MCP read contract

This is a contract only, not a deployed MCP service. A self-controlled HTTPS
MCP endpoint must provide independently authenticated, per-account read
access, with credentials stored outside IB and revocable without changing
IB's durable journal.

Initial read-only tools:

* search_notes({query: string, limit?: integer, cursor?: string}) returns
  {items: [{id, title, excerpt, updated_at}], next_cursor}. Query length
  must be capped at 1024 Unicode characters; limit is at most 50.
* get_note({id: string}) returns
  {id, title, body, source_url?, labels?, updated_at}, or an explicit error.

Remote IDs are opaque and never silently equated with local keep IDs.
Errors include not_configured, unauthorized, forbidden, not_supported,
not_found, temporarily_unavailable and conflict. Authentication, account
selection, privacy filtering and token rotation belong to the remote
adapter. Do not expose mutation, deletion, sync, labels or checklists to
ChatGPT on the strength of Android sharing. An authorized backend may
advertise separate capabilities if its host and account allow them.

The official Keep REST API is enterprise-focused, not a general consumer
account adapter. gkeepapi, keep-it-markdown and google_keep_mcp are reference
implementations, not in-core runtime dependencies or permission grants.

## Evidence and remaining acceptance

Host synthetic checks: sh tests/test_keep_bridge.sh.

These execute real C UTF-8/UTF-16 validation; atomic snapshot replacement,
reopening and symlink refusal; fake-Android am and rish argument fidelity;
no-handler, bad-payload and ambiguous-failure refusal. The separate Idriç
KeepBridgeCases module specifies approval, refusal, stable IDs, replay,
rejected proposal, duplicate prevention and restart recovery. Its actual
pinned compiler/evaluator execution has not yet been established.

NOT_RUN: compilation into the maintained IB DEX/Activity runtime; complete
retained-source producer / S08 executor integration; installed package and
saved-note confirmation on the SVITOO TAB_P10; physical process-loss recovery;
authorized remote consumer-account read/search; packaged update identity.
The D07 native presentation is still draft, so this prepares a decision and
an independently invocable transport, not a shipping Share-to-Keep button.
Do not merge or promote an APK on host fixtures alone.
