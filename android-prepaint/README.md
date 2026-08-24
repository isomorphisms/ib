# IB Prepaint APK

This is a deliberately small display harness for IB's renderer-neutral information
view. It paints successive prepaint revisions as a dark, linear document made from
native Android text, rows, forms, links, and images.

It is not a browser engine. The APK has:

- no `WebView`;
- no network permission;
- no HTML, CSS, or JavaScript parser;
- no durable tab or history ownership;
- no tint, inversion, or crop applied to fetched images.

The bundled `sample.prepaint` starts with the useful prefix from
`InformationSmoke.idric`, waits 1.4 seconds, then atomically replaces it with the
complete projection. **Replay** runs the repaint again. Hack Regular is bundled for
all visible text. **Open** accepts another local `.prepaint` artifact through
Android's document picker; the app does not need broad storage access.

## Build

With Android SDK 36 and Gradle 8.13 available:

```text
cd android-prepaint
gradle --no-daemon :app:testDebugUnitTest :app:lintDebug :app:verifyPrepaintBoundary
```

The APK is written to `app/build/outputs/apk/debug/app-debug.apk`. The package name
is `org.isomorphisms.ib.prepaint`, so this harness can remain installed beside the
IB storage inspector or a later browser shell.

The boundary check rejects `android.webkit`, `WebView`, an Internet permission, or
a debug APK larger than 2 MiB.

## Boundary

Idriç owns HTML extraction and the `InformationView`. This APK only parses the
disposable display interchange described in `docs/prepaint-display-contract.md`
and maps its already-extracted blocks to Android views. The interchange is a cache
format, not canonical browsing state.

Hack is from Source Foundry's Hack project. Its license is retained in
`licenses/Hack-LICENSE.md`.
