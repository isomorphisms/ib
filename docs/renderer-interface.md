# Renderer adapter

A renderer is a replaceable implementation detail behind a browser-owned interface.

The contract should be narrow enough that Servo, Chromium/WebView, a text-oriented engine, or a future renderer can coexist without becoming the source of truth for browser state.

## Core responsibilities

The browser core supplies:

- navigation request
- browser-owned tab id and history id
- viewport dimensions
- policy and permissions
- cookies/session access through a protected interface
- any recoverable neutral view state
- access to stored response/snapshot material when appropriate

The renderer returns events rather than directly mutating persistent browser records.

Typical events:

```text
navigation_started
navigation_redirected
navigation_committed
page_title_changed
history_state_changed
view_state_changed
capability_required
renderer_failed
```

The core decides what becomes durable.

## Conceptual interface

```text
Renderer
  capabilities() -> CapabilitySet
  attach(PageRequest, BrowserServices) -> RenderSession

RenderSession
  navigate(PageRequest)
  resize(Viewport)
  input(InputEvent)
  capture_neutral_state() -> ViewState
  snapshot(SnapshotRequest) -> SnapshotResult
  suspend() -> SuspensionResult
  close()
```

This is deliberately language-neutral.

## Capability negotiation

A renderer should advertise features rather than force the core to identify it by brand name.

Possible capabilities include:

```text
html
css
javascript
wasm
canvas
webgl
media
pdf
accessibility-tree
reader-text
full-dom-snapshot
```

A site or navigation may request capabilities. The core can choose another renderer if the currently attached one cannot satisfy them.

## Hot swap

A renderer swap is:

1. Ask the current renderer for the best neutral recoverable state it can provide.
2. Persist any browser-owned state that changed.
3. Detach and destroy the old render session.
4. Select another renderer by capability and policy.
5. Attach it to the same tab and current history entry.
6. Restore neutral state where semantics overlap.

Exact live DOM/JavaScript heap continuity across unrelated engines is not a requirement. The invariant is continuity of the browser-owned record, not bit-identical renderer internals.

## Neutral view state

The first neutral state should stay small:

```text
scroll_x
scroll_y
focused_element_hint
text_selection_hint
zoom
```

Later we can add standardized form-state restoration where it is safe and useful. Secret fields must not be persisted into ordinary state files.

## Renderer-specific cache

A renderer may produce an opaque recovery blob for fast wake-up, but:

- it is optional
- it is disposable
- it is versioned with the renderer
- the browser remains usable without it
- it never becomes the only copy of history or organization state

This lets a Chromium renderer use Chromium-specific acceleration while still allowing the same tab to reopen in Servo or a text renderer.

## Failure isolation

Renderer crashes should be treated like worker failures. The core keeps the tab and history record and may retry, choose a different renderer, or leave the tab sleeping.

A renderer crash must not imply loss of the browsing session.
