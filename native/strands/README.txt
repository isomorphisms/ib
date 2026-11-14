IB native fragment/strand byte backend — local review candidate (13 Nov 2026)

Owners and layering
-------------------
This is the native byte-window/index component for IB, not a new canonical
representation. The source of truth is the upstream immutable source
representation and the checked IB.Occurrence / IB.FragmentStore graph from
IB PR #123. IB.Strand.Strand is a separate semantic information-block view;
IB.Occurrence.Strand is the ordered fragment-reference view. Both remain
separate; this native component implements the physical projection consumed
by the latter. It must not take over browser history, reading observations,
user assertions, or the FastChat live request journal.

Admit the exact bytes with SHA-256 before using any span. A source is opened
read-only with O_NOFOLLOW and its digest checked; a pread window is limited to
4096 bytes and checks descriptor metadata for changes. This metadata check
is not a substitute for owner-enforced immutability and fresh hash admission
when the source can be externally modified.

ib_strand_index validates source, fragment/visit/link identity and ownership,
constructs cumulative visit byte offsets, sorts both incoming/outgoing link
projections and rejects parent cycles. ib_strand_window uses binary search
on the offsets, then sequential pread across whichever source fragments are
needed, never a prefix rescan. Its RAM cost is proportional to index records,
not source body lengths.

The binary index is a REBUILDABLE cache, not source authority. The reopen API
ib_index_load_checked takes an independently admitted canonical ib_strand
and compares the ENTIRE serialized derived index before loading. A forged
self-consistent index carrying the same advertised snapshot digest is not
sufficient for acceptance. Never call an unverified loader on an untrusted
index as a reading authority.

Coordinate units
----------------
IB.Occurrence.SourceCoordinate and the current IB.FragmentStore graph use
Unicode code-point offsets in decoded text. The source-bound translator
ib_utf8_map_spans converts sorted, disjoint source code-point ranges to exact
UTF-8 byte boundaries in one sequential bounded scan. Invalid UTF-8 is
rejected. The Idriç adapter must carry the representation, decoder revision
and decoded digest through this conversion, and verify the original bytes
against the same admitted revision. In contrast, opaque conversation message
bodies have original byte coordinates and do NOT enter the UTF-8 mapper.

A zero-byte conversation message is a message graph node even though it has
no nonempty Idriç SourceFragment. The C reader may maintain a zero-length
internal visit anchor so Q/A message navigation does not disappear. The
Idriç semantic adapter must not claim it is a nonempty source fragment.

Validation
----------
cd native/strands && make check
From the Conversations worktree: cd native && make; cd .. && go test ./...

Current host evidence only: GCC and Clang (ASan/UBSan) on Linux x86-64.
No actual Idriç frontend build, ICK/native direct-backend executable, Android
NDK build, emulator, physical MIRO C67 or production IB source integration
is claimed. The checked binary file I/O boundary in Idric#147 is still an
open prerequisite for an Idriç-only native implementation.
