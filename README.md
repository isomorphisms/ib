# ib

An experimental browser built around persistent browsing state rather than a renderer-owned tab model.

The browser core owns navigation history, sleeping/waking, snapshots, organization, indexes, and renderer selection. Rendering engines are adapters that can be replaced without changing the stored browsing model.

## Implementation languages

IB is implemented in **Idriç**. Browser-owned state, policy, and invariants belong in `.idric` source under `src/`.

**Grease** is the shell/operating-system language for orchestration. HTTP fetching, temporary directories, file movement, invoking compilers/parsers, cache-maintenance commands, and low-priority language-model batch passes belong in `.grease` programs rather than being reimplemented as Idriç application logic.

Python and Ithon are not implementation layers for IB. A disposable comparison may exist outside the runtime, but the browser core, storage/index policy, inspector model, and phone-facing application logic must not depend on them.

Android/native code is a narrow platform boundary for facilities Idriç and Grease cannot yet reach directly: NativeActivity/EGL/renderers, kernel-enforced filesystem operations, and similar FFI edges. Those boundaries do not own browser state.

`android-prepaint/` is a deliberately small phone-visible harness for the Idriç
information prepaint. It uses native Android views rather than `WebView`, applies
a fixed dark presentation to extracted text, preserves fetched image colors, and
replaces partial projections with later complete revisions.

The first executable Idriç slice lives on the `idric-browser-core` line. It covers ordered history values, rebuildable indices, storage classification/read policy, and the renderer-independent inspector model. The scientific-media work adds HTML-first arXiv harvesting, ordered image downloads, caption/alt naming, PDF fallback, and a low-priority second naming pass.
