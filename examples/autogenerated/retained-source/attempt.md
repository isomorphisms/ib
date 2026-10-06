# Retain a source before deriving a corpus view

## Values and types

A retained source has a caller-owned source and corpus identity, an origin
object/generation/member locator, acquisition time, exact byte hash, and an
explicit privacy/model policy. Equal bytes do not imply equal source identity.
An observation points into retained bytes; a derived index points into accepted
observations. Credentials are outside this domain.

`source_privacy` distinguishes ordinary and private material. `model_permission`
distinguishes allow and deny independently. `source_policy` carries both.
`permitted_for_model : source_policy → Bool` is pure. Retention and publication
are OS effects; JSON decoding is a replaceable source adapter. Message identity,
mapping graph and timestamps must remain distinct from view order.

## Smallest Idriç slice

`retained-source.idric` expresses the policy distinction in documented choice
syntax. It is a semantic reference attempt, not a substitute runtime. The
canonical compiler inspected was Idriç branch at
ef83e1627e0a8b84567ec1f461d3a85c26580019. The actual compiler invocation/result
is recorded in `compiler-result.txt` after the attempted one-step emission.

## Narrow implemented path

The existing Grease orchestration is extended at `bin/ib-source.grease`.
`lib/ib/chatgpt-source.jq` is a bounded JSON decoding adapter. Neither adds a
Python runtime. The ordinary-file host tests exercise bytes, provenance,
refusal, graph/reference preservation and rebuilding through the real Grease
implementation. They do not qualify an Idriç-backed policy engine, Android
immutable storage, an actual OpenAI export, or physical C67 behavior.

## First language boundary

The qualified compiler/runtime must be materialized before this policy can be
claimed executable. Next acceptance: compile the choice-based policy and test
all four ordinary/private × allow/deny combinations, then connect that exact
policy executable to Grease admission. JSON source decoding and crash-durable
file publication remain independent adapter boundaries.
