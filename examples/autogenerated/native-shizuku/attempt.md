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
