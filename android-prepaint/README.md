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

The bottom input accepts either one absolute HTTP(S) URL or search words. An
absolute URL is preserved byte-for-byte after surrounding whitespace is removed;
other input becomes a percent-encoded Google search URL. Live addresses are
bounded to 2,048 visible ASCII characters. The APK sends that URL as one argument
to `~/opt/ib/bin/termux_prepaint_url.grease` through Termux's
`RUN_COMMAND` service. Grease runs ICU, passes the fetched HTML to the Idriç
information extractor, and returns a complete `ib-prepaint` artifact. The APK
still has no network permission and does not substitute WebView or an Android HTTP
stack.

Termux 0.109 or newer must be installed with `allow-external-apps=true`, and IB
must be granted Termux's **Run commands in Termux environment** permission. The IB
checkout is expected at `~/opt/ib`; `icu` must be on Termux's `PATH`; and
`src/build/exec/ib-html-prepaint` must have been built from `src/HtmlPrepaint.idric`.
Those executable-installation requirements are intentionally explicit rather than
silently falling back to another fetcher.

Build the Termux-side renderer once with:

```sh
cd ~/opt/ib/src
idris2 HtmlPrepaint.idric -o ib-html-prepaint
```

Each request has a private nonce and increasing identifier. The viewer accepts a
result only for the newest request, only when Termux reports untruncated stdout
and stderr, only on a clean exit, and only when the structured artifact names the
exact requested URL and ends in a complete revision. The result is additionally
bounded to 64 KiB. Any missing executable, rejected permission, stale response,
truncation, command failure, or malformed artifact leaves the existing page
visible.

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

Idriç owns HTML extraction and the `InformationView`; Grease owns ICU execution
and its temporary files. The Android boundary recognizes an exact address versus
search input, invokes the fixed Termux command, and validates the returned
artifact without owning browser state.
This APK parses the disposable display interchange described in
`docs/prepaint-display-contract.md`, plus the deliberately narrower plain-text
fallback, and maps the resulting blocks to Android views. It does not parse HTML.
The interchange is a cache format, not canonical browsing state.

Hack is from Source Foundry's Hack project. Its license is retained in
`licenses/Hack-LICENSE.md`.
