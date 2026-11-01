# Publish the source-bound 966a native reader candidate

The user authorizes a directly installable APK candidate and reviewable PRs.
The fixed delivery branch remains delivery/ib-reader-a7349efc; that historical
branch name does not select or rebuild the old application source.

This publisher now consumes only IB966a producer artifact13088228711,
837240680 bytes, SHA-256
8de69e0bbf2e4a443cf5bc4a5f7296e25ea8f2a7a56bc298035947182f5f4de1,
run41315891303. It checks both source receipts, compiler/frontend pins,
package/version/minimum/target API, the unchanged signer, and exact byte
identity of both DEX files and all three native ELF payloads. The APK is
copied from that existing archive; there is no build, resigning, installation
or release stage here.

The new path candidates/ib-reader-966a613a.apk preserves the old A734
candidate. The receipt includes APK length/hash and DEX/ELF hashes. Branch
publication remains a fixed-target leased fast-forward; unrelated source,
application or release refs are not writable through this script.

At authoring the current source's main installed producer passed both
Android APIs, including checked journal recovery and update without
uninstall. Independent current APK legacy/TLS/ChordReader replays and
Google results/click remain pending. Physical TAB_P10 inventory, update and
runtime acceptance remain BLOCKED. Package signing is unchanged and no
user/device data is altered. This is a review candidate, not a release.

The source snapshot in this delivery branch is historical context. The
published artifact's authoritative application source is the exact frozen
IB966a receipt, not this publisher's checkout.
