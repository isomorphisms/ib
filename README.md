# ib

An experimental browser built around persistent browsing state rather than a renderer-owned tab model.

The browser core owns navigation history, sleeping/waking, snapshots, organization, indexes, and renderer selection. Rendering engines are adapters that can be replaced without changing the stored browsing model.

## Implementation language

IB is implemented in **Idriç**. Browser-owned behavior belongs in `.idric` source under `src/`.

Python and Ithon are not implementation layers for IB. A disposable comparison or fixture generator may exist outside the runtime, but the browser core, storage/index policy, inspector model, and phone-facing application logic must not depend on it.

Android/native code is a narrow platform boundary for facilities Idriç cannot yet reach directly: NativeActivity/EGL/renderers, kernel-enforced filesystem operations, and similar FFI edges. Those boundaries do not own browser state.

`android-prepaint/` is a deliberately small phone-visible harness for the Idriç
information prepaint. It uses native Android views rather than `WebView`, applies
a fixed dark presentation to extracted text, preserves fetched image colors, and
replaces partial projections with later complete revisions.

The first executable Idriç slice lives on the `idric-browser-core` branch. It covers ordered history values, rebuildable indices, storage classification/read policy, and the renderer-independent inspector model.
