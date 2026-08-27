# D-native IB APK completion plan

This is a planning boundary, not a claim that the D APK already implements these pieces. It replaces the vague statement “Java is needed for Android” with an exact list of framework crossings, an exact small managed callback bridge, and D-owned typed results for every stage.

## Current APK facts

The present `android-prepaint` harness is an ordinary Java `Activity`. It already demonstrates:

- one-file `ACTION_OPEN_DOCUMENT` selection;
- reading a selected `content://` URI through `ContentResolver.openInputStream`;
- parsing a bounded UTF-8 plain-text or `ib-prepaint` artifact;
- painting `LinearLayout`, `ScrollView`, `TextView`, `ImageView`, `EditText`, and `Button` objects;
- Android View click/editor callbacks;
- copying an `icu get ...` command to the clipboard.

It does **not** contain the intended D shell, select several fixtures, receive a URL shared by another app, read pasted clipboard text, fetch a URL inside the APK, preserve imported bytes in IB's durable store, or run the raw-byte → derived pre-paint pipeline as one application. Main deliberately rejects `android.permission.INTERNET`; another branch can hand a request to Termux and the external `icu` executable, but that is not a self-contained APK.

## What actually needs a managed bridge

JNI can construct an `Intent`, call `ContentResolver`, use `ClipboardManager`, and construct Android View objects from D. Those operations do not require application logic written in Java.

The crack is callback delivery. Stock `android.app.NativeActivity` forwards lifecycle, window, input-queue, configuration, and low-memory events through `ANativeActivityCallbacks`. It does not forward `Activity.onActivityResult` or `Activity.onNewIntent`. Launching the system picker from D is therefore straightforward, but receiving its result is not. A running single-top activity also needs `onNewIntent` to receive a newly shared URL. Android View buttons and editor actions similarly require managed objects implementing listener interfaces.

The planned bridge is one deliberately stupid class, approximately this shape:

```java
final class IbActivity extends NativeActivity implements View.OnClickListener {
    private static native void nativeActivityResult(
            int requestCode, int resultCode, Intent result);
    private static native void nativeNewIntent(Intent intent);
    private static native void nativeAction(int action);

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        nativeActivityResult(request, result, data);
    }

    @Override protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        nativeNewIntent(intent);
    }

    @Override public void onClick(View view) {
        nativeAction(((Integer) view.getTag()).intValue());
    }
}
```

The exact implementation may split the listener into a second tiny class if Android's type/lifecycle rules make that cleaner. It must not parse URLs, read documents, fetch, store browser state, own the display model, or decide what an action means. Native methods are registered explicitly from `JNI_OnLoad`.

This means the installed application will contain a small DEX callback router. Avoiding even that DEX would mean giving up the system picker/share callbacks, replacing Android Views with a wholly native surface and input system, or hand-generating equivalent DEX. None removes ART or Android's framework; it only makes the route less maintainable.

## D/Idriç-shaped boundary

Raw `jobject` values must not spread into IB. The JNI adapter turns them immediately into tagged D values. The names below describe intent; D will use an `enum` plus a `union`/payload structs to express them.

```text
android_event
  activity_started
  picker_cancelled request_id
  documents_selected request_id [selected_document]
  shared_text source text
  clipboard_empty
  clipboard_text text
  view_action action_id
  platform_failed operation failure

selected_document
  source_uri
  display_name? 
  detached_file_descriptor
  grant_lifetime

view_action
  open_documents
  replay
  submit_address
  open_link resource_id
  copy_text text_id
  paste_address
```

The adapter checks for a pending managed exception after every fallible crossing. Local references remain on the callback thread and are deleted or popped before return. Durable D state contains copied strings, owned file descriptors, IDs, and bytes—not local JNI handles.

## 1. Native shell and build boundary

Build `libib.so` for 32-bit `armeabi-v7a`, with `ANativeActivity_onCreate` as the native entry. The D runtime choice must be explicit: use only the runtime pieces that actually cross-compile and record every linked dependency. The first artifact does nothing except load, paint an exact native-owned status string, respond to one routed action, and survive pause/resume.

The Android SDK is still needed to package resources, compile the tiny callback class to DEX, assemble, align, and sign the APK. Gradle is optional. The plan is to keep the existing Gradle harness until the native slice works, then decide whether direct `aapt2`/`d8`/`apksigner` commands are clearer. Java tooling runs on the build machine; it is not where IB's browser logic lives.

## 2. System document picker

D constructs `ACTION_OPEN_DOCUMENT`, adds `CATEGORY_OPENABLE`, asks for accepted text/import MIME types, grants read access, and sets `EXTRA_ALLOW_MULTIPLE` for the two-or-three-fixture acceptance run. `IbActivity.onActivityResult` forwards the result object without interpreting it.

D distinguishes cancellation, a malformed result, one `Intent.getData` URI, and ordered `ClipData` items. Duplicate selections remain duplicate import events. The first acceptance copies each selected document immediately into IB's internal durable store, so success does not depend on retaining provider authority after reboot.

Persistable URI permission is a later optional capability for “watch this external document,” not a prerequisite for ordinary import. If it is used, D preserves the actual read flags returned by the picker rather than inventing a grant.

## 3. `content://` document reader

D calls `ContentResolver.openFileDescriptor(uri, "r")` through JNI, then `ParcelFileDescriptor.detachFd`. From that point D owns an ordinary Linux file descriptor and closes it on every exit path. This is preferable to pulling an entire managed `InputStream` through thousands of JNI reads.

Provider metadata such as display name and reported size is optional and untrusted. The byte reader applies an actual limit while reading. The raw bytes are appended before decoding; UTF-8 text decoding and `.prepaint` recognition create derived records. A provider error, permission failure, read failure, over-budget document, malformed UTF-8, malformed structured artifact, and unsupported input are distinct outcomes.

File reading and parsing move off the UI thread after the descriptor is detached. The callback itself only validates the result, acquires owned inputs, queues work, and returns.

## 4. Shared URL receiver

The manifest gains an `ACTION_SEND` / `text/plain` filter. D examines the launch intent on a cold start; the tiny activity override forwards later intents while the application is already running. A later `ACTION_VIEW` HTTP(S) filter is a separate choice because “share this text” and “make IB an address opener” have different user-visible effects.

The receiver first preserves the exact shared text and source event. It then recognizes one bounded absolute HTTP(S) URL. Commentary containing a URL, several URLs, malformed text, and unsupported schemes remain explicit unsupported inputs rather than silently becoming the first URL found.

## 5. Clipboard

Clipboard access is invoked only by a visible user action. D reaches `ClipboardManager` and `ClipData` through JNI, distinguishes no clip, non-text clip, empty text, valid address, and arbitrary text, and records what the user chose to import. It does not poll the clipboard in the background.

Clipboard writes use `ClipData.newPlainText` and `setPrimaryClip`. The existing “copy an ICU command” behavior becomes an ordinary copy action; it is no longer the network implementation.

## 6. Android View display

D owns the `InformationView`/pre-paint revision and constructs the small Android View tree through JNI on the UI thread. The initial set remains intentionally boring: vertical root, chrome row, scrollable page, headings, selectable paragraphs, links, images, rows, one address field, and a few buttons. Hack remains an APK asset.

Views carry small integer action IDs in their tags. The managed click router sends only that ID back to D. D resolves it against the current displayed revision. This avoids one Java listener class or lambda per block and prevents managed objects from becoming browser-state identity.

The first pass uses the Search/Open buttons and omits keyboard-editor callbacks if that keeps the bridge smaller. Editor action routing can be added to the same callback class after the button path is accepted. Accessibility labels, selection behavior, scroll preservation, and focus are properties to verify on the real phone, not reasons to move the document model into Java.

## 7. Network callout inside the APK

The APK gains `android.permission.INTERNET`. This is a normal install-time permission and has no runtime prompt. Fetches run on a bounded worker thread, never in a View callback.

Here `icu` means the repository's deliberately small **Idriç HTTP client**, not the Unicode library. The existing Termux route requires an external executable and an Idriç renderer, so it remains a useful development/oracle path but is not the self-contained APK runtime.

The planned APK backend is an upstream `libcurl` cross-build for `armeabi-v7a`, linked into `libib.so`; the curl command-line executable is unnecessary. Build only HTTP/HTTPS support and a pinned TLS backend, initially OpenSSL because the existing ICU transport already uses it and its behavior can be compared. Android does not expose its managed TLS stack directly to libcurl, so the APK build must also supply the TLS library and trust anchors. The build records exact curl, TLS, and CA-bundle revisions.

The D layer deliberately exposes less than libcurl:

```text
web_request
  get http_url request_budget
  get https_url request_budget

fetch_outcome
  response requested_url resolved_url status headers raw_body
  rejected_url reason
  permission_or_socket_failure detail
  dns_failure detail
  connection_failure detail
  tls_failure detail
  redirect_failure detail
  response_too_large observed_limit
  cancelled
```

The initial policy keeps the already-tested limits: 2,048-byte visible-ASCII request address, at most eight redirects, 64 KiB response headers, 4 MiB response body, HTTP(S)-only redirect targets, certificate and host verification always enabled, and no cookies, proxy, credentials, POST, compressed body, or JavaScript. Curl may know more moves; the IB type does not offer them yet.

Requested URL, redirect chain, resolved URL, status, response metadata, and raw bytes are appended before parsing. An HTTP status such as 404 is still a response and can feed IB's Archive.org policy; it is not collapsed into the same failure as DNS or TLS.

## 8. Raw bytes to visible pre-paint

Both selected documents and fetched responses enter the same staged path:

```text
owned input bytes
  → append raw document/resource record
  → classify/decode
  → derive bounded logical document
  → derive pre-paint revision
  → paint Android Views
```

No network callback directly mutates the page. It appends a result and proposes a new display revision. The UI accepts only a revision belonging to the newest live request, so a slow earlier request cannot overwrite a later address. Canonical bytes remain distinct from derived document records and disposable display caches.

The existing Java `PrepaintDocument` and `UrlRecognition` tests become behavioral oracles while their bounded parsing, URL policy, and revision state move into D. They are deleted only after the D tests demonstrate the same accepted and rejected fixtures.

## Build and acceptance order

1. **D load:** ARMv7 APK loads `libib.so`, paints one exact line, and records lifecycle callbacks.
2. **Managed crack:** one button action crosses Java → registered native method → D, under CheckJNI.
3. **Two-file import:** picker returns two ordered fixtures; D detaches descriptors, copies bounded bytes, and pre-paints both without network.
4. **Cold and warm share:** a URL shared while stopped and while already running creates two distinct import events.
5. **Clipboard:** explicit Paste imports exact text; empty/non-text cases remain visible outcomes.
6. **HTTPS:** bundled curl/TLS fetches one fixed HTTPS fixture and records requested/resolved URL, status, byte count, and body hash.
7. **Failure matrix:** missing `INTERNET`, DNS failure, refused connection, certificate failure, redirect loop, oversize body, malformed UTF-8, parser rejection, and display rejection are distinguishable.
8. **Restart:** imported raw bytes and accepted browser events survive process death; derived pre-paint can be rebuilt.

The first installable acceptance APK stops after steps 1–3 if necessary. That is already useful: it establishes that D, JNI, the system picker, provider descriptors, bounded import, and Android View painting work on the actual ARMv7 phone before network and TLS enlarge the artifact.

## Explicit non-goals for this pass

- no WebView;
- no Java-owned browser model;
- no Termux requirement in the installed APK;
- no claim that JNI bypasses Android permissions or lifecycle;
- no general curl option surface exposed to IB;
- no cookies, authenticated browsing, password handling, or secret storage;
- no migration of all IB design contracts into the first APK;
- no deletion of the working Java harness until each replacement slice has an accepted D oracle.

## Primary references

- [Android JNI 1.6 interface map](android-jni.md)
- [AOSP `NativeActivity.java`](https://android.googlesource.com/platform/frameworks/base/+/android-14.0.0_r75/core/java/android/app/NativeActivity.java)
- [Android document-provider flow](https://developer.android.com/training/data-storage/shared/documents-files)
- [curl Android build notes](https://curl.se/docs/install.html#Android)
- [pre-paint display contract](prepaint-display-contract.md)
- [IB storage model](storage-model.md)

