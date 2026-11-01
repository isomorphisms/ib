# Complete responses, HTTP refusals and transfer observations

The maintained ICU response owner writes capture metadata only after framing
finishes and preserves complete HTTP rejection bytes with exit 10. The actual
Grease worker now transports that complete capture rather than discarding it.
Other nonzero ICU statuses still exit without emitting a frame. Its private,
bounded stderr records the original integer exit; no URL or authorization
capability is printed.

The existing app-owned frame parser accepts canonical three-digit final
statuses 200 through 599, verifies metadata/body completeness as before, and
retains the exact status. Only status 200 can set navigation_admitted YES.
Strict original admitted-source opens continue to refuse HTTP errors; the
separate checked decoder input preserves them as verified original evidence.
Idriç admission rejects them before extraction/presentation or history
acknowledgment. The native Activity formats that refusal with the actual HTTP
status. Previous visible/committed state is kept. Empty bodies still refuse the
current positive-length source contract; no empty response is claimed readable.

The same response-owner capture has an optional paired content_range and
transfer_decoding observation. Existing whole-document range/encoding guards
remain. A present identity/chunked observation is now retained in the canonical
v2 receipt and copied by its one maintained readback parser. Legacy receipts
may omit it; empty readback then means unobserved, never inferred identity.
Unknown and duplicate observations refuse. This does not change source IDs,
generation rules, immutable receipt publication or storage ownership.

Type sketch: verified original response evidence is independent of admitted
HTML. A bounded HTTP status and optional closed transfer-decoding observation
belong to the original acquisition. A readable document requires checked status,
MIME, encoding, digest, generation and complete-byte gates. Grease supervises
processes; the existing native platform boundary validates framing/files;
checked Idriç owns document admission and the refusal message.

The canonical host fixture now retains its real Arena root for independent
ai-ci identity/chunked/legacy byte verification. Changed transfer token and
duplicate-field copies must fail. Compiled producer controls erase the observed
transfer field or broaden status admission; each must fail its designated
semantic assertion. Complete 300/400/404/429/500/599 captures preserve exact
status/bytes while refusing original admission; 100/199/600/999/0200 are invalid.
The prior committed source is reopened after each response. These source edits
remain NOT_RUN until the exact-head CI executes. Android HTTP failure input,
the SDK decoder carrier and physical TAB_P10 are not inferred from host tests.
