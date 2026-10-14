# Native reader migration evidence

This is partial migration evidence, not acceptance of a finished browser.

Java/Kotlin/Gradle application source/configuration and their two build
workflows are removed on this line. The actual source absence gate passes.
An isolated Java source path and javac recipe each fail with the required
diagnostic; clean recovery passes. The existing public Idriç vocabulary check
also passes. Source absence does not prove restored browser features.

The actual NativeReader.idric source hash is
`93413f2f8f7f8c11555442a52c5bc74dbbe340bbd9e69f612ae9280358616cb1`.
It compiles directly with the new checked Activity/cast backend and Idriç
2734ac5a86610eddfab73b1644d72b4ac85a65ee. No Java, javac, Gradle, d8 or
handwritten candidate DEX participates in source compilation.

Actual DEX: 2,072 bytes, SHA-256
`f3b217e29c6e722fe3cd7d568ee77827fe4133cb3f334e9812e730e968a7c4ec`.
AOSP dexdump verifies checksum and independently reports the Activity
superclass, constructor, onCreate/invoke-super, click receiver forwarding,
TextView checked casts, setText and range invocation placement.

Android resource linking succeeds and verifies all declared layout/view IDs
against the packaged resource table. The signed candidate is 12,782 bytes,
SHA-256 `688c6649e280608e21258cae01000da5576aa1dbf1b0fd08ff19503095b11ffa`.
Package org.isomorphisms.ib.prepaint, versionCode 3, min SDK 26, target SDK 36,
launcher Idric.Generated. Its v2/v3 signature verifies against the existing
test certificate de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9.
The packaged classes.dex is byte-identical to the emitted candidate, and final
ZIP alignment passes. No new signer, package migration or uninstall occurred.

Source checks used the actual Grease entry and the build recipe executes
source/backend, independent parsing, resources, packaging and signing as
separate visible stages. The signing tool works on package metadata; it does
not compile Java application source. Initial resource-ID and missing dynamic
library/classpath failures were fixed before the passing package stage.

ART launch/lifecycle, visible reflow, source text/click behavior, accessibility,
touch and physical C67 replacement installation: NOT_RUN. This DEX-only reader
has no ABI-specific native payload; that does not prove either ARM device ran.
The installed device package/version/certificate remains UNKNOWN.

The actual NativeSourceProjection consumer imports IB.Information and
IB.RecoveredInformation and passes source checking. DEX lowering rejects it
with `DEX tail-loop parameter needs a supported first-order runtime representation`.
No source.dex is emitted. Full retained-source recovery, image projections,
acquisition, Binder workers and Longview/durable navigation remain INCOMPLETE.
The inherited compiler bootstrap and independent ai-ci producer promotion
remain UNQUALIFIED/NOT_RUN. No release, merge or complete migration is claimed.
