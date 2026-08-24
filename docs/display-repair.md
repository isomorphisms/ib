# Display repair

`ib` is allowed to improve a page at display time when the site omits a useful ordinary browser affordance.

This is an architectural feature, not a special case for one site. The page author supplies content; `ib` remains responsible for presenting that content usefully to its user.

## Boundary

Display repair belongs to the browser-owned UI/policy layer above the replaceable renderer.

A repair may add controls or presentation around rendered content, but it does not rewrite the fetched response, stored snapshot, or canonical history record. The original page remains available as fetched.

Browser-added controls should normally be painted as browser-owned overlays/chrome rather than injected into the page DOM. Page CSS or JavaScript must not be able to hide, restyle, impersonate, or intercept a trusted browser control.

Repairs should be reversible and disableable globally or per site.

## First repair: Copy

The baseline primitive is a browser-owned `Copy` button associated with a semantic text target.

The primitive needs only:

- a target identifier or anchor supplied by the active view;
- the exact text payload that will be copied;
- a coarse target kind such as code, preformatted text, or an ordinary text region.

Activating the button copies that exact payload through the platform clipboard boundary. Clipboard integration is platform/UI glue; deciding that the control exists and what it copies is browser policy.

The Idriç core stub is `IB.DisplayRepair.CopyButton`.

## Detection is separate

The copy-button primitive must not depend on an expensive detector.

Candidate discovery can evolve independently and can combine cheap and expensive signals. Likely cheap signals include:

- `pre` and `code` elements;
- syntax-highlighted blocks;
- terminal/command examples;
- accessibility or semantic roles supplied by a renderer;
- repeated text layouts with an obvious single payload;
- absence of an equivalent nearby site-provided copy control.

A later semantic or language-model pass may handle ambiguous regions, but failure to run that pass must never disable explicit or cheaply detected copy targets.

Detection should return neutral candidate descriptions to browser policy. It should not directly mutate the DOM or perform clipboard writes.

## Renderer relationship

Different renderers may expose candidates differently. A DOM renderer can expose element-backed semantic regions; a text renderer can expose line/range anchors; a pre-renderer can expose extracted structured regions.

The repair policy consumes those neutral candidates and asks the browser UI to paint the trusted control. That keeps copy repair available across Chromium, Servo, text-oriented rendering, and future renderers without making any one engine the source of truth.

## Precedent

Copy is intentionally the first example of a broader rule: `ib` may repair the displayed interface when a page makes a common user action needlessly difficult.

Future repairs should use the same boundary: detect a useful augmentation, describe it in browser-owned state, paint it outside hostile page styling, and keep the fetched page itself unchanged.
