# App-owned Shizuku platform boundary

The checked Activity must receive the real Shizuku Binder and own the private
ingress capability throughout its lifecycle. The maintained Shizuku API uses
`moe.shizuku.api.BinderContainer`: Parcelable class identity, one per-instance
IBinder reference, a static Creator, and a Parcel containing exactly that strong
Binder. Preserve this wire representation; browser policy does not belong here.

`ParcelableBoundary.idric` describes only the mechanical platform/JNI classes
through android-NDK's shared typed writer. `ib_shizuku_parcelable.c` implements
the narrow SDK Parcel calls. It does not parse documents, grant authorization,
own history, launch acquisition or substitute a network client. Array creation
is explicitly bounded to 4096; unsupported sizes fail rather than allocating
unboundedly. Native signatures and candidate classes require independent ART
and actual Binder-transport qualification before application admission.

The reusable instance-reference prerequisite is
[isomorphisms/android-NDK PR #36, “Encode native instance references for app-owned Binder containers”](https://github.com/isomorphisms/android-NDK/pull/36).
Its older static-only writer refuses this layout. Protocol source is
RikkaApps/Shizuku-API a27f6e4151ba7b39965ca47edb2bf0aeed7102e5;
no Shizuku Java source is used as the candidate implementation.

This continuation remains incomplete until provider identity, authorization,
worker service delivery/death/revocation, the native Activity and retained
source/readback path are connected and independently exercised. A generated
Parcelable class is not a working reader or physical tablet observation.

Checkpoint `8523d49005d0ca975ecea85a46cd1015b4c88d65` passes the independent
SDK Parcel wire checks and compiled Binder-loss control on actual API 29/34 ART
in run 41247706260. Its official evidence archive is 156615771 bytes with SHA-256
`3594071d47f0c4632e2eba26418920b0e2140f37f0374cc94a6b70abb8f0ba8c`.
This is a wire result, not a provider/worker transaction.

The next continuation adds native ContentProvider, application callback and
death-recipient declarations, with the checked Activity observing/requesting
the narrow acquisition capability. The platform adapter checks its provider
identity, official manager signer/permission owner, kernel caller UID, private
callback object, API 13 attachment, bounded parcels, actual permission and
connection epoch. API 13's getVersion/getUid calls require permission; they run
only after attachment and checkSelfPermission. Claimed permission booleans do
not grant authority. Old death/callback failures cannot invalidate a later epoch;
revocation hooks carry the old epoch and detached recipients are unlinked.

`prepare-client.grease` builds the actual checked Activity, platform DEX and
three generic NDK ABI libraries, then uses the maintained android-NDK packager.
The output is explicitly `ib-native-boundary.apk`, not the requested finished
reader. The existing public test signer and package/version remain unchanged.
Compiled UID-bypass and death-no-op native variants are separately signed for
independent rejection on fresh emulators. No tablet ABI or installed signer is
inferred from these generic builds.

ai-ci's installed observer uses pinned official Shizuku 13.6.0, source
`2650830c5b099ae0dd34fedf614d4f592ca05d65`, with its actual authorization UI.
It must verify installed native launch, external-caller refusal, ordinary-app
delivery refusal, denial, grant, independent identity queries, Binder death and
legitimate reconnection. The fake peer appears only in the ordinary-app refusal
control and cannot count as actual Shizuku evidence. These installed checks are
pending. UI-thread SDK RPC timeout behavior, private acquisition capability
distribution, Grease worker launch, Arena/document wiring and physical acceptance
remain unfinished; the campaign is INCOMPLETE.

Installed boundary run 41250102609 at 4ab6ffef did not reach a compiler or
Android execution. Ubuntu mirror downloads stalled in the ICK action's package
prerequisite until the 60-minute job deadline. No compiler cache/build was
entered and no Android receipt was produced. The next workflow pins ai-ci's
bounded signed-HTTPS Ubuntu provisioning, selects the SDK's required JDK 17,
and allows 90 minutes for the actual multi-stage build and two emulators.
