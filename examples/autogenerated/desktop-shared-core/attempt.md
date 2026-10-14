# Shared Linux/Android core slice

Owner: SUN IB-X64-00, https://github.com/isomorphisms/ib/issues/138.
Execution opened by FP first-wave envelope 2bafcec71e7a73ff3d6d341690c294340a2a7493 before edits.

## Type sketch before implementation

Browser persistence policy is already owned by IB.LongViewTask. Both ordinary_tab
and protected_authenticated_transaction must choose durable_browser_task.
Protection controls renderer/session treatment; it must not decide whether history
exists after the renderer exits.

IB.FragmentStore owns RevisionedSource and checked FragmentGraph. SourceCoordinate
is the existing Unicode code-point count. Keep it distinct from UTF-8 byte counts
and UTF-16 code-unit counts even though the inherited representation is Number.
Proposed boundary:
- utf16_code_unit_offset : SourceCoordinate → RevisionedSource → Maybe Number
- fragment_source_text : RevisionedSource → Text → FragmentGraph → Maybe Text

The copy operation accepts an already admitted source revision plus an existing
fragment identity. It refuses changed source/representation/decoded digest/decoder
and changed decoded text, then slices exact source characters using the existing
checked half-open range. It does not parse rendered text, normalize whitespace,
authenticate a caller's claimed digest, or create an information/selection store.

The existing LongViewLifecycle from the selected Idriç Android work is reused
byte-for-byte. Host/renderer observations must preserve durable identities and
refuse callbacks from a detached or earlier generation.

Effects are confined to the separate command/checking layer. These core operations
are pure and must typecheck against the declared Idriç compiler. AICI supplies an
independent input/golden corpus and mutations of actual production source.

## Required services and evidence boundaries

The initial checker is Cat Food's pinned Linux x86-64 Idriç bundle, source
94dfd99bd3e376507fedc8611053b7173b2519f0, tar SHA-256
0be57deab9410e8d4c09c7198bf7e8ee0ec3e24121f76640147efa4a53d120da.
Typechecking/evaluator acceptance is not generated executable, disk recovery,
GUI or Android execution. ICK native qualification and Android followers remain
separate. No fallback source is planned.

## Acceptance intended before edits

Golden decoded text: A東京🙂éB.
Code-point boundaries 0..7 map to UTF-8 byte counts
0,1,4,7,11,12,14,15 and UTF-16 unit counts 0,1,2,3,5,6,7,8.
Unknown fragment, changed revision, equal bytes from another origin, changed
decoded content, wrong source ranges and stale host/renderer observations refuse.
Tabs/newlines/leading spaces must survive exact copy.

Mutants must restore disposable ordinary tabs, count the non-BMP scalar as one
UTF-16 unit, and skip source-revision binding. Each must fail its intended case.

## Result

Linux checker/evaluator PASS: all 38 independent cases, three production-source
mutants rejected for their intended cases, and clean recovery True. FP run
ib-linux-shared-core-20261014T112612.222464Z bound the actual recipe/Ike result.
The borrowed lifecycle blob is 72e891af7b76ca7e33517d7916e227db2c7f70cf,
identical to the selected Android source. A later exported-oracle check also
passed. Exact published-source replay is recorded separately in FP.

The initial recipe used fresh output roots. A follow-up guards against reusing
snapshot directories, which could otherwise leave stale top-level source after
cp -R. The adversarial producer that printed True and exited 7 was rejected
before evaluation. This contract still does not prove Android runtime behavior.
