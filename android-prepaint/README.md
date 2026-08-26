# IB Prepaint APK

This is a deliberately small display harness for IB's renderer-neutral information
view. It paints successive prepaint revisions as a dark, linear document made from
native Android text, rows, forms, links, and images. It also accepts an ordinary
UTF-8 text file directly: prose becomes readable paragraphs and a line containing
one absolute HTTP(S) URL becomes a tappable link.

It is not a browser engine. The APK has:

- no `WebView`;
- no network permission;
- no HTML, CSS, or JavaScript parser;
- no durable tab or history ownership;
- no tint, inversion, or crop applied to fetched images.

The bundled `sample.prepaint` is a small article rather than a developer-workbench
fixture. It waits 1.4 seconds, then atomically replaces the useful prefix with the
complete projection. **Replay** runs the repaint again. Hack Regular is bundled for
all visible text. **Open** accepts either another local `.prepaint` artifact or a
plain text file through Android's document picker; the app does not need broad
storage access. A rejected structured artifact or unreadable file leaves the
current page visible.

The bottom search chrome converts spaces to `+`, constructs a Google search URL,
and produces the exact `icu get` request. In this standalone no-network harness it
copies that command and paints the handoff visibly. The integrated IB shell owns
running ICU and replacing the request with fetched pre-paint; the display APK does
not quietly substitute WebView or an Android HTTP stack.

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

Idriç owns HTML extraction, URL policy, ICU execution, and the `InformationView`.
This APK parses the disposable display interchange described in
`docs/prepaint-display-contract.md`, plus the deliberately narrower plain-text
fallback, and maps the resulting blocks to Android views. It does not parse HTML.
The interchange is a cache format, not canonical browsing state.

Hack is from Source Foundry's Hack project. Its license is retained in
`licenses/Hack-LICENSE.md`.
