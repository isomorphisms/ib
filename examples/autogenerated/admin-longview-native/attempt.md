# Native protected renderer integration

## Domains and ownership

`IB.LongViewTask` remains the browser-owned task/tab/navigation and lifecycle
model. `IB.LongViewNative` decodes its bounded ordinary record and applies the
existing lifecycle operations. The current NativeReader Activity exports those
checked functions directly to Android DEX.

The native adapter owns only kernel descriptors, UTF-8 transfer, Android view
references, monotonic observations, and applying checked effects. It never
parses a second task model or infers authentication from a visible page.

## Interfaces and invariants

The checked exports accept/return bounded ordinary task records. Native record
storage uses Android's trusted files root and opens IB-owned children with
`O_NOFOLLOW`. The selected task reference is committed only after task/tab
metadata and the first navigation history entry are durable. A broken existing
selection is refused; discovery does not invent a replacement task.
Every commit rechecks the existing selection. Starting a different task cannot
silently conceal a torn selector or an incomplete first publication.

One interactive protected renderer is attached at a time. The source reader and
Google search-acquisition adapter retain their separate responsibilities. The
new explicit action opens the user-entered URL transiently; the durable task
contains only the permitted neutral origin. After a cold launch the person
uses **Resume protected page**. App switching preserves an existing renderer;
Activity or renderer replacement reconstructs from the neutral origin. A hidden
task still observes an Activity replacement; reopening its view later does not
invent an additional Activity transition. While protected work owns the Android
surface, deferred reader layout restoration is suppressed, pending source work
is cancelled, and reader-only Back, Forward, and Reflow controls are hidden.

No page values, cookies, full URLs, JavaScript payloads, form fields, or sensitive
terminal results are copied into ordinary records or diagnostics. Callback
records capture their task/tab/navigation/host/renderer generations. Stale
callbacks are refused by checked Idriç before any checkpoint or metric is used.
Page commit does not prove a usable control or an authenticated session.

A timing callback also captures the native `documentEpoch` of its current
renderer attachment. Main-frame starts advance that diagnostic scalar. A later
callback from the preceding document is discarded even if the durable task and
renderer identities still match. This scalar is not a second browser navigation
model and is not persisted. The Android field is a boxed `java.lang.Integer`,
matching the pinned framework writer's supported instance-reference fields.

Android's commit, finish, and error callbacks do not carry an immutable
document identity. Attribution is therefore limited to the first start in a
renderer attachment. Native code retains that start URL only in memory, bounded
to 8192 UTF-16 units, and requires the callback URL to equal both that reference
and the WebView's current URL. Destroy/reset releases the reference. Any later
start or mismatch discards it and leaves lifecycle attribution unknown for the
rest of that attachment. No URL is logged or added to a task record.

Unknown commit/finish callbacks neither checkpoint the task nor declare a page
visible, and they do not initiate timing sampling. Redirects, subsequent
documents, repeated URLs, and callback URLs exceeding the transient bound
therefore remain usable browsing paths with unavailable attributed lifecycle
timing. The independent action observer can still establish a useful control
or server-side effect; these callbacks cannot establish either result.

## Native boundary

The kernel adapter is `native/android/ib_longview_store.c`, independently usable
on the host. The framework adapter is `native/android/ib_longview.c`. Its
WebViewClient and ValueCallback classes are emitted from maintained Idriç
framework declarations beside the existing platform classes. No Java/Kotlin
application or Gradle build is introduced.

New first-party kernel/JNI source and the kernel tests use literal Icky `←`
assignments and initializers. The selected ICK frontend consumes those exact
files; Android assembly and linkage are separate NDK stages. The narrow reader
hooks only consult current Android surface ownership.

The application keeps `org.isomorphisms.ib.prepaint` and its existing public
test signer. A loopback-only HTTP exception permits the explicit local fixture;
other cleartext network traffic remains disallowed.

## Numeric observation protocol

`IBLongView` emits numeric keys only. Every event includes `event`, `host_pid`,
`host_generation`, `activity_generation`, `renderer_generation`, and
`document_epoch`. Ordinary events also include `code` and `elapsed_ms`.

| Event | Observation |
| --- | --- |
| 1 | Explicit load request |
| 2 | WebView construction and attachment cost |
| 3 | Main-frame document start |
| 4 | Committed task checkpoint cost, including its synchronous durability barrier |
| 5 | Visible page commit; this does not establish usable controls |
| 6 | Main-frame finish callback |
| 7 | Main-frame transport/HTTP error or navigation refusal |
| 8 | Renderer loss |
| 9 | Returning to or explicitly reopening a protected attachment |
| 10 | Navigation Timing numbers, `code=1` for a refused payload, or `code=2` for unknown attribution |
| 11 | Paint and observed subresource numbers |
| 12 | Unavailable/incomplete protected state |

Event 10 contains `dns_ms`, `connect_ms`, `tls_ms`, `first_byte_ms`,
`transfer_ms`, `dom_ms`, `load_ms`, and `transfer_bytes`. Event 11 contains
`first_paint_ms`, `first_contentful_paint_ms`, `observed_resource_count`, and
`observed_resource_transfer_bytes`. Unavailable measurements are `-1`. Only a
12-element array of bounded integers is accepted from JavaScript; strings,
extra values, and other structures are refused without logging their payload.
The observer can copy `observationRecord` and `documentEpoch` from the current
LongViewClient into a real LongViewTimingCallback to exercise that boundary.
Epoch-matching payloads still reach the strict parser after attribution becomes
unknown: invalid input emits the fixed refusal, while valid numbers emit an
unknown event instead of measurements. A first-document positive and a later
one-time-value refusal thus exercise the actual parser separately.

Unknown commit/finish observations use their normal event numbers with
`code=2`, `document_epoch=0`, and `elapsed_ms=-1`. An unattributed main-frame
error retains its observed numeric error code with the same unknown epoch/time
and does not replace the current page status. These explicit unknown values do
not temporarily alter the attachment's actual epoch.

HTTP error responses arrive through Android's separate `onReceivedHttpError`
callback. The native bridge accepts only current-client, main-frame observations
with status codes from 400 through 599, and records that positive code in event 7.
It applies the same transient URL attribution check as transport errors; an
unattributed HTTP response cannot replace the page status. Subresource errors
produce no main-frame event and do not change attribution. The callback does not
checkpoint or initiate timing samples, and it never reads or logs the response
body, headers, or reason phrase.

Attributed commit/finish/error elapsed values start at the first main-frame start;
event 3 preserves initial request-to-start delay. Timing is sampled at visible
commit and finish, so resource counts describe those observations rather than
all later background activity. Page-controlled performance values remain
diagnostic; the independent fixture determines whether an actual control and
server-side effect succeeded. No timing receipt is synchronously persisted by
the application, and no speed improvement is claimed.

## Evidence

`verify-store.grease` compiles and executes the production kernel adapter with
ICK, including trusted Android ancestor aliases, IB-owned descendant escapes,
private modes, hard links, atomic replacement, and torn selection/history
refusal. `verify-dex.grease` compiles the actual NativeReader and framework
declarations with the declared frontend/backend pins and independently
disassembles their exports and callbacks. Both record source and output hashes.
The first primitive-field implementation was refused when the checked platform
generator ran, despite successful compilation of that generator. The verifier
now runs and decodes the platform classes before the larger Activity build;
it explicitly checks the supported boxed field representation.

Those checks establish their named host/compilation boundaries. Installed
Android, physical C67, and Google/YouTube acceptance remain separate. The
independent fixture must exercise actual usable controls and server-side
no-replay evidence through this same native application path.

The `local-*` receipts preserve the first native integration checks. The later
`callback-*` receipts bind the conservative lifecycle-attribution repair to its
own C source and three-ABI outputs; they do not relabel the earlier build or
claim installed callback execution.

The `http-error-*` receipts bind the later missing-callback correction to its
source, generated DEX, and native outputs. Absence of the callback in the prior
source/DEX/native boundary is inspection evidence. The original fixture server
also fails the new 503 assertion because it has no such route; that is a fixture
regression control, not an observed Android failure. The independent installed
observer must separately receive a real main-frame 503 and disregard a real
404 stylesheet response through the same product WebView.
