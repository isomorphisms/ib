# Rish bundle observations

The capability adapter compares the existing Cat Food saved snapshot to the
launcher resolved by `IB_RISH_COMMAND` or the actual `rish` on PATH. It never
selects the saved launcher as a replacement, restores a file, starts a service,
or executes a launcher while inspecting local files.

The independent fields mean:

| Field | Observation |
| --- | --- |
| `catfood_receipt_present` | Supported, unambiguous snapshot receipt exists. |
| `catfood_saved_pair_verified` | Both saved file hashes match that receipt. |
| `catfood_active_rish` | Exact explicit/PATH executable selected by IB. |
| `catfood_active_pair_matches` | Selected launcher and its sibling DEX match saved hashes. |
| `catfood_dex_mode_valid` | Active sibling DEX has no Unix write permission bits. |
| `live_authority_observed` | Unique measured UID from the separately executed Binder provider report. |

A byte match does not imply a valid DEX mode or live authority. The sibling
comparison follows Cat Food's rish-bundle convention; it does not prove a modified
launcher loads that sibling, private Android placement is valid, or Android ART
will load it. These remain execution/physical checks.

Missing, duplicate, malformed and mismatched receipts remain unknown, invalid or
stale. A changed `current` pointer suppresses the buffered verification rows.
This is a before/after observation, not an atomic snapshot or proof against a
pointer changing away and back, concurrent in-place file writes, or replacement
after the observation.

Cat Food already owns a per-installation `device-id` and the versioned
`device/inventory.tsv` contract. IB reads that existing identity and accepts the
inventory reference only when its single recorded identity matches. It never
creates an identity or derives one from a model. The earlier general-manifest-
unavailable statement is therefore retired. Inventory time stays reported;
reading an old inventory does not refresh it or establish hardware attestation.

The referenced upstream ownership is `isomorphisms/catfood/device-state.sh`
and `android/target.sh`, inspected at
`609a9628d5a52860f956bf62e0914a0cd03292ae`; `android/preserve-shizuku.sh`
receipt schema and bytes were also refreshed through its current main file.

`tests/test_rish_bundle_provenance.grease` exercises real Grease with inherited
library compatibility explicitly selected. Its executable launcher is an opaque
external `/bin/sh` fixture which must never be invoked. No device, Shizuku
service, shell UID, package, reboot or ART result is claimed by those tests.
