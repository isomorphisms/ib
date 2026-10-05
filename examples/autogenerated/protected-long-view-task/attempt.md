# Program attempt: `protected-long-view-task`

Status: `NOT_RUN`

## Purpose

Exercise the browser-owned task transition needed when an authenticated IB
workflow outlives its Android host process.  This came from the MIRO A1 long-view
workflow work tracked by issues #71 and #81.

## Intended behavior

The input is a protected task with stable task, tab, and navigation identities.
The output is the same logical task after a different host process discovers it.
The output must identify host-process reconstruction, increment the host and
activity generations, clear renderer attachment, and refuse to claim either
authenticated-session or JavaScript-heap continuity.

This example does not own cookies, page fields, passwords, authorization codes,
or tokens.  It does not promise exact remote-site or JavaScript restoration.

## Type-system sketch

### Values and domains

- durable task, tab, navigation, host-process, and renderer identities;
- a neutral HTTP(S) origin target with path, query, and fragment already removed;
- live, Activity-recreated, renderer-replaced, host-restarted, and remote-blocked
  continuity kinds;
- independently reported session, form, heap, and reconstruction results;
- optional OAuth-adapter metadata containing only ordinary fields and references
  to protected state.

### Types and signatures

```text
start_protected_task : identities → neutral navigation → host → LongViewTask
renderer_was_lost : LongViewTask → LongViewTask
attach_initial_renderer : RendererIdentity → LongViewTask → LongViewTask
attach_renderer : ReplacementRendererIdentity → LongViewTask → LongViewTask
host_process_restarted : HostIdentity → LongViewTask → LongViewTask
serialize_long_view_task : LongViewTask → Text
```

### Actions and effects

The state transitions and serialization are pure.  The example's only action is
printing one record.  Android file I/O and WebView attachment remain platform
adapter effects outside this program.

### Laws, invariants, and errors

- renderer and host replacement preserve task, tab, and navigation identities;
- host replacement increments host and Activity generations;
- renderer replacement increments only the renderer generation;
- reconstruction cannot imply live JavaScript-heap continuity;
- session continuity is `not-proven` until independently confirmed;
- an ordinary tab has renderer-lifetime policy and does not acquire this durable
  record;
- durable record fields cannot contain tab or line separators;
- ordinary records contain references to protected OAuth state, never codes,
  tokens, passwords, nonces, state values, or PKCE verifiers.

Construction fails when an identity or neutral address is empty or contains a
record separator.  Neutral addresses containing query or fragment delimiters
are refused.

### Runtime and backend boundary

The intended semantic backend is the repository's pinned Idriç/Chez test path.
The Android app is a narrow Java platform adapter until IB has a direct Idriç
Android application path; Java does not become the semantic source of truth.

## Idriç attempt

Source: `protected-long-view-task.idric`

Revision and compiler: pending branch revision; IB's pinned Idriç compiler
`bd9fbe1e68ce9b3dd3981fcd3e1abf9e50bd350e`.

Command: pending execution through the repository's existing Idriç build path.

What the program tried to do: construct one protected task, simulate host-process
restart, and emit the durable ordinary-state record.

## Result or failure

Not run yet.

## Fallback

None.  The Java Android adapter is required platform integration, not a fallback
for this semantic program.

## Idriç language work exposed

First blocking capability: not yet known.

Smallest plausible language change: none claimed before the exact compiler run.

Acceptance test for that change: not applicable until a compiler failure is
observed.

## Evidence boundary

Source and type sketch exist.  Compilation and behavior are not yet verified.
