# Program attempt: neutral-address

## Pinned foundation compiler compatibility

The foundation job at reader head `59716dabd375f0d49b8fb67a18dfd604b33a923a`
failed to parse the `after_address_prefix` clause because its pinned Idriç
`7050c116326c9c8e8d0a56c2db7baf2119cbae19` reserves `prefix`. The qualified
`94dfd99b` host checks had accepted that local pattern name. Rename the local
tail to `expected_suffix`; the validation rules and accepted addresses remain
unchanged. The existing constructor, independent and lifecycle controls are
the acceptance cases; this compatibility repair does not add another oracle.

Status: host semantic controls PASS; DEX/ART and physical acceptance NOT_RUN.

## Purpose and type-system sketch

Close the existing `IB.LongViewTask.neutral_address` constructor boundary before
a restart address enters the ordinary durable task record. This is a bounded
validation repair on the retained no-Java branch, not a new navigation model.

The input is untrusted decoded `Text`. The output is `Maybe NeutralAddress`.
Successful construction preserves the supplied HTTP(S) address exactly.
Failure returns `Nothing`; it does not redact a guessed origin or store rejected
input. Only the existing constructor may produce the value consumed by task
serialization. Existing accepted exact paths remain supported.

The constructor must reject an unsupported or missing scheme, missing authority
or host, user-info in authority, query/fragment delimiters, control/whitespace,
and backslash interpretation ambiguity. Host/port syntax receives conservative
structural checks; this is not DNS resolution, an RFC-complete URI parser, or a
proof that a permitted path cannot contain site-specific sensitive material.

The control program calls the actual constructor and exact-text observer. Its
only effect is printing each result or exiting with failure. It is a host
semantic acceptance program, separate from DEX/ART and physical-device proof.

## Source and compiler

- IB starting source: `912ae8905ae137868ce6e9aff1a82c915c40ee85`.
- Host compiler bundle: Idriç `94dfd99bd3e376507fedc8611053b7173b2519f0`.
- Bundle archive SHA-256:
  `0be57deab9410e8d4c09c7198bf7e8ee0ec3e24121f76640147efa4a53d120da`.
- The bundle archive and all `payload.sha256` entries matched before execution.
- Backend: the existing Idriç/Chez host semantic path; no RefC fallback.

The new checks live in `NeutralAddressChecks.idric`; eleven regression cases also
extend the maintained `src/LongViewLifecycleCheck.idric` suite.

## Executed commands

The unchanged `src/` tree was copied to a fresh baseline source directory, and
the control module was copied beside it. The same process staged the candidate
source separately, so compiler products did not modify the source worktree or
any historical bundle. With each source directory as the observed working
directory, the verified bundle entrypoint was invoked as:

```text
bin/idric NeutralAddressChecks.idric -o neutral-address-checks
bin/idric-env <staged-source>/build/exec/neutral-address-checks
bin/idric LongViewLifecycleCheck.idric -o longview-lifecycle-check
bin/idric-env <staged-source>/build/exec/longview-lifecycle-check
```

Here `bin/idric` and `bin/idric-env` are the absolute entrypoints beneath the
verified bundle named above, not ambient commands. The maintained vocabulary
check and `git diff --check` also passed.

## Result and fallback

The baseline compiled successfully, preserved the five valid examples, refused
empty input, then exited 1 at `FAIL password-bearing authority refused`. This
reproduced the defect in the actual semantic constructor.

The first repair compiled and passed its 29 constructor cases and 45 lifecycle
checks. Independent Moon verification then found three missed cases: an
international `café.test` authority was accidentally refused, and EM SPACE
(U+2003) and LINE SEPARATOR (U+2028) in paths were admitted. The inherited
`isAlphaNum` and `isSpace` operations did not cover those Unicode boundaries.
That initial result is not a passing result for the expanded requirement.

The follow-up preserves safe international authority text and registered-name
punctuation, including underscore, without normalizing it or adding IDNA/DNS
machinery. It explicitly checks Unicode White_Space outside the C0/C1 ranges.
Percent-encoded authority remains refused to avoid a decoded-delimiter
ambiguity; legitimate percent escapes in a path remain supported.

The corrected constructor compiled and passed all 36 constructor cases. The
maintained lifecycle suite compiled and passed all 48 existing and new controls,
including same-host recreation, host/renderer replacement, identity checks,
separate session/heap uncertainty, credential/scheme refusals, and preserved
safe exact paths. Both candidate executables exited 0.

No alternative implementation is used as acceptance. These runs do not prove
DEX lowering, Android navigation, a signed APK, or physical A1/C67 behavior.

## Language work exposed

No language change was needed for this bounded host semantic repair. The
independently tracked direct-DEX runtime and navigation dependencies remain;
passing the Chez program does not qualify those backends.

## Independent Moon controls

`NeutralAddressIndependentChecks.idric` independently exercises the exported
constructor and exact-text observer. Its inputs are decoded address `Text`;
its output is a checked refusal or exact preservation result, followed by an
IO report. It does not parse the address into a competing URL model. The
controls distinguish authority user-info and encoded authority ambiguity from
valid at-signs and percent-escaped delimiters in a path. They also cover case,
Unicode path/host text, IPv6, leading/trailing controls and non-HTTP input.

This checks the bounded durable-record invariant. Numeric port range, complete
IPv6 syntax, DNS/IDNA normalization and site-specific sensitive path contents
remain outside that invariant; no claim of full URI validity is made.

The independent suite was compiled and executed on separately staged source
trees with the verified `94dfd99b` host bundle. The tar archive digest above
and every `payload.sha256` entry were checked again. The same independent
source bytes (SHA-256
`ce5eb661a9305ed24a088e594dec56c64255b28db6ccb5b8cf485a34f1ce5458`)
were used for every comparison:

- Original source `912ae8905ae137868ce6e9aff1a82c915c40ee85` failed 19 of
  31 independent cases and exited 1, including authority user-info refusal.
- Initial repair `8792e8861543a6d2e716db43b8babae3536b39bf` passed the 29
  original constructor and 45 lifecycle controls, but failed three independent
  cases: preserving `https://café.test/résumé`, refusing EM SPACE in a path,
  and refusing LINE SEPARATOR in a path. This failure was returned to Earth.
- Corrected source `bdb9078cdf2c69b4014763be2c8637ea83960d38` passed all 31
  independent cases, all 36 updated constructor cases and all 48 updated
  lifecycle cases. Every compilation and executable exited 0.

The independent program was built with `bin/idric
NeutralAddressIndependentChecks.idric -o neutral-address-independent` and run
through `bin/idric-env` in a fresh staged source directory. These names denote
the absolute entrypoints in the verified bundle, as in the commands above.
The corrected independent host program SHA-256 is
`6a9abcc0e1773d7bfc14d2a105dff98273b4d7c9d2aa10c470e0825031d1c378`.
The exact evidence is summarized in `moon-host.tsv`. No DEX, ART, APK, network,
DNS or physical-device execution was performed by this check.
