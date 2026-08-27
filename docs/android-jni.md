# Android JNI boundary

IB's D/native Android boundary uses the JNI 1.6 interface documented in the phone-utilities repository:

- [complete JNI documentation at the pinned revision](https://github.com/isomorphisms/utilities-android-phone-user/tree/8d92473da54ec3ad6b7f7870f9bfe3002fc00d99/jni)
- [continuing `jni` branch](https://github.com/isomorphisms/utilities-android-phone-user/tree/jni/jni)
- [IB picker, shared-URL, and pre-paint route](https://github.com/isomorphisms/utilities-android-phone-user/blob/8d92473da54ec3ad6b7f7870f9bfe3002fc00d99/jni/ib-file-picker-route.md)

The immutable revision is the implementation reference for this IB branch. The moving branch is where the interface notes can continue. This is a direct link rather than a submodule because the dependency is documentation and ABI guidance, not source that must appear in IB's build tree.

JNI is the narrow platform hatch: it lets D ask Android's existing `NativeActivity`, `Intent`, `ContentResolver`, clipboard, and sharing classes to do supported framework work. It does not own browser state, parse imported documents, perform pre-painting, or grant authority that the Android process does not already have.

For the first phone acceptance, keep the boundaries independently observable: launch the system document picker; distinguish cancellation from failure; accept two or three selected `content://` inputs; copy accepted bytes into IB's durable store; recognize URL lines; paint local text immediately; and report network acquisition separately. A missing network permission or HTTP/TLS implementation must not make local selection and pre-paint appear broken.
