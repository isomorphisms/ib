# Recover a byte-valid but semantically invalid history

Purpose: keep the last committed readable document recoverable when a head's
checksum is valid but its journal names the wrong selected source, generation,
cursor or format. The ordinary-file index adapter continues to treat journals
as opaque; it does not become another browser parser.

Type sketch: source identity + immutable source generation + opaque journal
cross a private JNI call to the maintained checked Idriç Activity export.
That pure export delegates to the existing presented_history/Navigation owner.
An accepted journal still grants no source admission. Arena/PFD owners reopen
and independently verify the selected committed response before display.

The native getter and retained worker now require that checked result. They
try the previous committed head at most once. A failure in a retained lookup
cannot enter network acquisition: only a new navigation has that authority.
A successful previous-head acknowledgement preserves that previous head
rather than rotating the invalid current head into its place. No source file
or Arena range is deleted.

Independent acceptance owner: ai-ci
45b1dc5e29cb9618517dbb4e68034e651b08b24d. Its reader-v6 ART profile checks literal
journals through the actual Activity export and a separately compiled
permissive source mutant. Its navigation reader-v4 profile independently
verifies both original heads, writes an owned checksummed cursor/source
mismatch while the emulator app is stopped, and requires visible previous
source recovery, exact copy and Back/Forward without Shizuku. Final source
digests, ordered history and complete Arena bytes must remain identical.
All existing profiles and negative controls remain required.

The same acceptance checkout contains and descends from the preserved
816bb49b72a3bc58a3af91c2758fd5e120441925 independent cursor fix. GitHub's
compare records f07e4f0 ahead by 12, behind by 0; 45b1dc5 is its direct child.
This avoids divergent duplicate observer checkouts.

Status at authoring: implementation and test source; checked compilation,
ART, installed fault recovery and physical acceptance are NOT_RUN. Physical
TAB_P10 access remains BLOCKED. The prior source-reflow campaign remains
under test; no old result qualifies this changed head.
