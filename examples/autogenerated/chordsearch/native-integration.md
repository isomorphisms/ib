# Checked SEARCH integration attempt

This extends the maintained native reader, using IB.WebSearch.google_icu_url
and the same checked navigation, worker, Arena and Prepaint path. It does not
add a network client, an external browser fallback or a Java application.

## Type and identity boundary

The framework Activity entry receives ActivityReference and SavedStateReference;
the explicit onNewIntent entry receives ActivityReference and IntentReference.
A View-only SDK interface callback obtains its Activity through the actual View
context. These are SDK reference domains, distinct from ordinary Idriç data and
from managed command or pure-export entry contracts. The existing merged DEX
backend already supports these explicit entry contracts.

A fresh ACTION_SEARCH requires the framework getCallingPackage result to equal
org.isomorphisms.chordreader.dev and PackageManager.checkSignatures to match
the existing IB signer. Referrer extras and a Binder system-server UID are not
caller evidence. Both development packages currently use the reviewed public
test certificate de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9;
no signer or package changes are introduced. Physical installed signers remain
unknown. ChordReader must dispatch a fresh startActivityForResult request.

Null/empty, over-1024-UTF-16-unit and malformed-Unicode query values refuse
before Idriç Text decoding; a fresh UTF-8 CharsetEncoder validates scalar text.
The maintained encoder rejects controls and bounds the complete URL, makes
spaces +, and percent-encodes other non-unreserved UTF-8 octets once. Original
query text is never interpolated into a shell. Saved-state recreation does not
resubmit the initial handoff. New warm intents refuse because API 29/34 cannot
authenticate their new sender through the original getCallingPackage result.
A pending search refusal survives the automatic asynchronous last-good restore.

## Results and limits

Compilation and runtime checks of this exact change are pending. Consent and
verification redirects use observed final URL evidence to refuse; other Google
responses are explicitly partial static HTML, not accepted search results.
In-page challenges, actual Google results and clicked result navigation still
require independent observation. Task restoration after a crash without saved
state also requires qualification; process-local Intent consumption is not a
durable duplicate-delivery proof.

Prior head 0592dd8e passed independent installed navigation on API 29/34 in
ai-ci run 41292339244. Those results do not qualify this source change. The
integration ledger keeps physical acceptance BLOCKED and the campaign open.
