# Native storage inspector

This is a developer-only Android surface for the read-only storage contract in
`docs/storage-inspector.md`.

It is deliberately not a browser renderer and contains no WebView, Chromium,
JavaScript engine, Python runtime, or app Java/Kotlin code. The APK is a
`NativeActivity`; C performs rooted no-follow storage inspection and uses
Android's ordinary `ScrollView`/`TextView` widgets only to paint the resulting
debug text.

The inspector reads:

```text
<app internal files>/state/
```

It never creates or mutates that directory. Resume the app to refresh the view.

Current sections:

- overview totals and classification totals;
- bounded tab-manifest rows;
- bounded physical-file rows;
- bounded tail of canonical `visits.jsonl`, with byte offsets.

Build from this directory with:

```sh
gradle --no-daemon :app:assembleDebug
```
