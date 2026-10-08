# Program attempt: retention clearance

Status: PARTIAL

## Purpose and type sketch

SUN IB-S11 separates an artifact's origin/retention obligation from pathname,
temperature and placement. The scientific prefetch producer has acquired
information; a cache directory does not make those bytes reconstructible.

`clearance_decision : retention_kind → clearance_request → RebuildEvidence → clearance_result`
is pure Idriç policy. Canonical data, acquired bytes, accepted decisions,
last-complete results and unknown retention are preserved. A derived projection
can be cleared only after an explicit request and complete offline rebuild
evidence tied to retained input/decision/generator identities and matching
output digests. Unknown/unavailable prerequisites refuse clearance.

Effects: Grease records acquisition origin and source trigger; it does not
infer retention from path, temperature or SD availability, and performs no
new eviction or real-user deletion. A platform store owns actual no-follow,
fsync and delete operations in later execution.

Laws: temperature/path changes cannot authorize deletion. Absence is not
deletion. Retention references survive projection loss. An interrupted derived
clearance leaves canonical input untouched; no network/model is required to
rebuild the qualified fixture.

## Idriç attempt and runtime boundary

Base IB main `10fa7bbe93ff9ffa1ba7b9f62e0dd8f6c10a86bc`. Actual clean compiler
`ff4d852862a3942592f8ade9afde8d409d9803be` can typecheck and evaluate pure
cases. It is not a qualified ICK/NDK consumer executable build. No Chez,
generated-C, Java, Python or stock-compiler substitution is presented as
deployment acceptance.

Source, typechecker, compiler evaluator, Grease producer-fixture execution,
artifact/deployment and device evidence remain separately reported.

## Fallback

None.

## Remaining language/build work

Publish a qualified Idriç executable for this semantic boundary, bind the
stored rebuild evidence to exact input/decision/generator hashes, and execute
the kernel-enforced store consumer before any real cache-clear operation.
Physical storage/media behavior remains NOT_RUN.
