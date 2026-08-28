# IB D-native APK

This directory now builds the ARMv7 phone shell as `libib.so`. D constructs the
Android View tree through JNI and owns imported text, shared text, clipboard text,
UTF-8 decoding, bounds, and status transitions. It is compiled with `-betterC`, so
the APK does not contain druntime, Phobos, a garbage collector, or a Java
application layer.

The only packaged Java source is `IbActivity.java`. Its complete job is to load
`libib.so` and forward three Android-only events as primitive callbacks:

- document-picker results;
- a warm `ACTION_SEND` intent;
- a button's integer action identifier.

The native shell currently provides:

- a D-constructed Android View screen, with no `WebView`;
- `ACTION_OPEN_DOCUMENT` with multi-select for up to four documents;
- detached `ParcelFileDescriptor` reads, so URI grants do not escape into durable
  D state;
- bounded UTF-8 to UTF-16 pre-painting (24 KiB per file, 48 Ki UTF-16 total);
- cold- and warm-start shared text or URL intake;
- explicit, user-initiated clipboard intake;
- HTTPS GET from a detached native worker using statically linked ARMv7 curl and
  Mbed TLS, with redirects and certificate/hostname verification;
- a pinned Mozilla CA bundle loaded by D from APK assets and passed to curl as a
  memory blob;
- cancellation, empty-result, unreadable-result, truncation, and imported-byte
  states painted in the interface.

The manifest has `INTERNET` and forbids cleartext traffic. Runtime URL policy also
accepts only an ASCII `https://` URL. Share or paste one URL, then choose **Fetch
current HTTPS URL**. The native response is bounded to 64 KiB and painted as UTF-8;
HTML extraction and the complete Idriç `InformationView` projection remain later
layers. Local/import/share behavior remains independent of network availability.

## Build

The reproducible inputs are Android SDK 36, Android NDK `27.2.12479018`, LDC
`1.41.0`, Gradle `8.13`, and Java 17. With `ANDROID_NDK_HOME` set:

```text
cd android-prepaint
gradle --no-daemon :app:testDebugUnitTest :app:lintDebug :app:verifyPrepaintBoundary
```

Gradle runs `build-native-armv7.sh` before packaging. The script emits an ELF32
ARM EABI library, links only the Android/Bionic boundary, strips it, verifies the
native entry points, and rejects dynamic dependencies on a D, JVM, curl, or TLS
runtime. It also downloads digest-pinned curl, Mbed TLS, and CA sources, then
cross-compiles and statically links the transport. The APK is written to
`app/build/outputs/apk/debug/app-debug.apk`.

The boundary check additionally requires exactly one packaged Java source, one
`armeabi-v7a` library, a callback-router DEX no larger than 64 KiB, and a complete
debug APK no larger than 2 MiB. The old Java pre-paint parser remains only under
`src/test`; it is a test oracle and is not packaged.

## Ownership

Android owns lifecycle delivery, system UI, content grants, share delivery, and
clipboard authorization. D owns the screen composition and all application
meaning. Idriç remains the intended owner of canonical browsing state and the
renderer-neutral `InformationView`; this phone shell is the native platform
adapter described in `docs/android-jni.md` and
`docs/d-native-apk-completion-plan.md`.
