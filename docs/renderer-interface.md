# Renderer adapter

A renderer is a replaceable live-page implementation behind a browser-owned interface.

The contract should be narrow enough that Servo, Chromium/WebView, a text-oriented engine, or a future renderer can coexist without becoming the source of truth for browser state.

A renderer is not a frontend. The visual pre-paint frontend may attach one, while the text task frontend may complete useful work through acquisition and extraction without attaching any renderer. A text-oriented page renderer is likewise not the ChatGPT-like task frontend.

## Core responsibilities

The browser core supplies:

- navigation request;
- browser-owned tab and history-event identities;
- task context when relevant;
- viewport dimensions;
- policy and permissions;
- cookies or session access through a protected interface;
- any recoverable neutral view state;
- access to stored response or snapshot material when appropriate.

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

The visual frontend may also send ephemeral visual-focus and visible-operative-link hints to the acquisition scheduler. These hints can reprioritize bounded safe work as the user scrolls or changes focus. A focus hint may cause a safe discovered link to become queued under the active prefetch budget; it does not itself imply that bytes already exist, create a tab, or bypass unsafe-link policy. Raw layout geometry need not become canonical browsing state.

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

A renderer advertises features rather than forcing the core to identify it by brand name.

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

A site, navigation, or task may request capabilities. The core can choose another renderer if the currently attached one cannot satisfy them. Capabilities supplied by acquisition or extraction adapters need not be mislabeled as renderer capabilities merely because they can eventually be painted.

## Hot swap

A renderer swap is:

1. Ask the current renderer for the best neutral recoverable state it can provide.
2. Persist any browser-owned state that changed.
3. Detach and destroy the old render session.
4. Select another renderer by capability and policy.
5. Attach it to the same tab and current history event.
6. Restore neutral state where semantics overlap.

Exact live DOM or JavaScript-heap continuity across unrelated engines is not a requirement. The invariant is continuity of browser-owned resource, tab, event, task, and view identities, not bit-identical renderer internals.

## Neutral view state

The first neutral state should stay small:

```text
scroll_x
scroll_y
focused_element_hint
text_selection_hint
zoom
```

Later work may add standardized form-state restoration where it is safe and useful. Secret fields must not be persisted into ordinary state files.

## Renderer-specific cache

A renderer may produce an opaque recovery blob for fast wake-up, but:

- it is optional;
- it is disposable;
- it is versioned with the renderer;
- the browser remains usable without it;
- it never becomes the only copy of history, task, or organization state.

This lets a Chromium renderer use Chromium-specific acceleration while the same tab can later open through Servo or a text renderer.

## Failure isolation

Renderer crashes are worker failures. The core keeps the tab, task, and history record and may retry, choose another renderer, fall back to extraction, or leave the tab sleeping.

A renderer crash must not imply loss of the browsing session or investigation.
