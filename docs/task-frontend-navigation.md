# Task frontend left navigation

This is a concrete interaction contract for the ChatGPT-like task frontend described in `personal-workbench.md`. It does not choose a frontend toolkit or move browser-owned state into the frontend.

## Left navigation

The task frontend has a left navigation panel that can be opened and closed without disturbing the active task.

The toggle uses ordinary ASCII direction symbols rather than a hamburger icon:

```text
closed   >   open the navigation to the right
open     <   close the navigation to the left
```

The visible symbol always describes the action that will happen when it is pressed.

Opening and closing should slide smoothly, but the interaction should be quicker than a conventional slow drawer animation. Use about 150 ms as the initial transition target and measure on the weak Android hardware used for IB acceptance. Do not delay task interaction until the animation completes.

When reduced motion is requested, remove the sliding transition rather than substituting another animation.

## Behavior

- The active task and its scroll/input state survive opening or closing the navigation.
- Closing the panel removes its width from the primary task surface instead of leaving a blank sidebar-sized gutter.
- The toggle remains available in both states.
- The control remains a real focusable button even though its visible content is only `>` or `<`.
- Accessibility text describes the action as `Open navigation` or `Close navigation`; the punctuation glyph is not the accessible name.
- Keyboard activation and touch activation use the same state transition.
- Repeated toggles during the transition converge on the latest requested state rather than queueing animations.
- Navigation state is frontend presentation state. It does not create, delete, duplicate, or mutate browser-owned tasks, tabs, history, or source material.

## Acceptance

A frontend implementation passes this slice when:

1. the navigation is open, the toggle visibly shows `<`, and activating it closes the panel;
2. the navigation is closed, the toggle visibly shows `>`, and activating it opens the panel;
3. neither state uses a hamburger icon as the open/close control;
4. the transition target is approximately 150 ms on the normal-motion path and immediate on the reduced-motion path;
5. the active task remains usable while the panel moves;
6. rapid `open -> close -> open` input ends open without replaying a stale animation queue; and
7. opening and closing the panel changes no browser-owned semantic state.

The exact panel width, initial open/closed default, and final visual styling remain frontend choices until measured on the target devices.
