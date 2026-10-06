# Drive authorization on the MIRO C67

The adapter is reconciled with current IB main independently of the PiP loading
experiment, whose existing components and tests are preserved. LongViewActivity
remains the launcher. The experiment delegates private handoffs to
DriveAuthorizationActivity, which
owns only explicit Google consent and one local code handoff; it has no WebView,
journal, clipboard, corpus store or model path. Unknown/duplicate URI fields and
overlapping requests are refused. Granted scopes must include each requested
scope. The default is `drive.readonly`; the only additional allowed set is
`drive.readonly + drive.file` for application-owned archives/copies.

Package identity remains `org.isomorphisms.ib.webview`. Minimum SDK 26 fits the
[retained C67 API 34 profile](https://github.com/isomorphisms/catfood/blob/main/android/devices/miro-c67.md).
Target/compile SDK 36 does not require runtime API 36. The Java/Play services
package has no native ABI payload; an arm64 port is unnecessary. Play services,
account state and actual authorization remain physical runtime gates.

Historical exact-head artifact e8164d978e35e2b4188c3a0fbd403e181b29e74d is
[artifact 10707349978](https://github.com/isomorphisms/ib/actions/runs/35755114882/artifacts/10707349978).
It predates private stable signing and is not a qualified update. New hosted
builds retain main's stable public test signer and replacement-install fixture.
Those are inspectable compilation/package and emulator fixture evidence, not
private signing, C67 installation or OAuth acceptance. Without a supplied
keystore the build emits an unsigned candidate rather than creating another
debug identity. The inherited Java/Gradle adapter remains
existing platform debt, not an ICK/NDK/direct-DEX rewrite.

## One live signer and registration

Do not register the shared public test key or a runner-local debug key for live
Google authorization. Supply one stable private IB certificate, preserve it for
replacement installs, and inspect the installed package before replacement.
Do not create a C67-specific package/signer or uninstall to hide a mismatch.

In the same user-owned Google project, enable Drive API, configure consent and
the intended test user, register an Android client with this package/private
signer SHA-1, and create the matching Web OAuth client for offline server-code
exchange. Import the Web client through cloud-storage-api `init-android` into
mode-0600 credential state. Read-only is default; explicitly request
`--stream-upload`/`--copy-tree` for the narrow write scope.

Official references:
<https://developer.android.com/identity/authorization>
<https://developers.google.com/android/reference/com/google/android/gms/auth/api/identity/AuthorizationRequest.Builder>
<https://developers.google.com/workspace/guides/create-credentials>

Private signer material, client registration, C67 installation/replacement,
physical consent/code return, durable refresh state and an authorized Drive
request remain external gates. CI cannot accept these on the user's behalf.
