# Selected local portable evidence

Owner: [issue #104](https://github.com/isomorphisms/ib/issues/104), SUN IB-S10.
This extends the source intake in [isomorphisms/ib PR #46, “Retain source
generations and rebuild source-linked Pensieve views”](https://github.com/isomorphisms/ib/pull/46).

`bin/ib-evidence.grease` implements local directory export, import and assistant
context preparation. It does not upload, synchronize, inspect renderer profiles,
decode a source again or generate new source IDs. `IB_HOME` is an explicit
absolute root. `GREASE` names the verified Grease runtime for existing intake.

## Selection and bounds

An explicit selection file has three TSV columns, without a header:

| Column | Values |
| --- | --- |
| Existing source ID | 64 lowercase hexadecimal characters |
| Representation | `raw` or `reference` |
| Selection authority | `ordinary` or `private-selected` |

There is no whole-corpus default. A private row requires its explicit
`private-selected` authority. That authority does not override the source's
assistant policy. Source secrets/session/profile representations and unknown
provenance fields are outside this version's admitted schema. This is not a
content scanner: the upstream retained-source classification remains responsible
for identifying credentials embedded inside otherwise selected material.

The existing source kinds `raw_urls`, `chatgpt_export` and `opaque` are supported
as retained bytes; importing them does not imply a supported decoder. The
adapter admits 1–32 distinct source rows, each at most 16 MiB, with at most
64 MiB of supplied raw bytes, 64 KiB per provenance file and 16 KiB for the
selection/manifest. It never recursively copies an untrusted directory. Copies
are capped at declared sizes plus one sentinel byte and checked again.

## Versioned directory

The directory contains only `schema`, `sources.tsv`, `complete`, and
`sources/<source-id>/provenance.tsv` plus optional `raw`. Unknown entries,
symlinks and special files refuse. All requested path ancestors are checked for
symlinks. The schema is `ib-portable-source-v1`; `complete` contains the SHA-256
of the ordered source manifest.

The ten manifest columns are source ID, representation, explicit selection,
present/unavailable state, privacy, assistant allow/deny, source SHA-256, source
byte count, provenance-file SHA-256, and `whole-source-bytes`. The provenance
file is byte-for-byte original, retaining generation, acquisition time,
origin/provider/member and policy. It must reproduce the original source ID.
Equal bytes from distinct origins therefore remain distinct sources.

Hashes detect changes and corruption; they do not authenticate an unsigned
bundle against an attacker who deliberately constructs a new coherent bundle.
Path checks reject the tested static symlink escapes. They do not establish
kernel-enforced containment against an actor changing directory topology
concurrently; an openat/O_NOFOLLOW adapter is a separate stronger boundary.

## Import and recovery

All rows and supplied bytes are verified before the first canonical intake.
A bounded local snapshot is verified again. Present sources go through the
existing `retain-source.grease`; its exact source ID and publication behavior
remain authoritative. Its inherited-shell Grease mode is named implementation
debt (`+O ysh:all`), not a substituted interpreter.

Omitted/missing optional bytes remain unavailable references. They do not
create a canonical source with an empty body. The import receipt at
`cauldron/imports/<manifest-hash>` preserves the ordered reference manifest,
including unavailable status, and appears only after all present sources were
accepted. The unavailable references also retain their exact original provenance
under `references/<source-id>/provenance.tsv`, so generation and origin survive
without inventing source bytes. It is bookkeeping, not another source store.

The import is atomic per existing source intake, not one transaction covering
the bundle. If a later source cannot publish, earlier committed sources stay
usable and no complete bundle receipt appears. Retry is idempotent. Existing
source/provenance conflicts refuse; the importer does not overwrite them.
New ordinary/private projections and indexes are rebuilt by the existing
decoders when requested, rather than exported as canonical truth.

## Assistant context

`context` freezes and verifies the selected bundle, then prepares only references
whose original `model` policy is `allow`. Missing model policy defaults to deny.
The result contains `ib-assistant-source-context-v1`, ordered source references,
byte spans from zero to original byte count, exact hashes and available raw files
under `data/`. An unavailable reference remains unavailable. Source data is never
evaluated or interpreted as a shell command, tool request or instruction.

The context is local preparation, not transmission or permission to an assistant
to access the rest of the corpus. It currently carries whole-source byte spans;
it does not invent decoded-character coordinates or export new task/decision
formats while their owners are still implementing them.

## Execution boundaries

`tests/portable-evidence.grease` runs the native Grease adapter against fixed
synthetic identity goldens, exact round trips, deny/corrupt/path/size controls
and an actual later-source intake-lock interruption/retry. The policy source
and its ten cases are under `examples/autogenerated/portable-evidence/`, checked
and evaluated by `tests/portable-evidence-policy.grease` through the current
Idriç compiler's `--client` interface.

Those are separate OS-adapter and compiler-evaluation results. A qualified
Idriç policy executable integrated into the adapter, ICK/NDK delivery, Android
filesystem/fsync qualification and physical-device acceptance remain unproven.
This bounded host slice does not satisfy every acceptance item in issue #104.
