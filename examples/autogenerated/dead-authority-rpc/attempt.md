# Recover when the authority dies during a checked RPC

An independent IB966a replay (ai-ci 0eeea2d96c3fe2f48fef529d516602cb6f153e7f,
run41319549929, API34 decoding job124886683886) fails the positive revocation
check before reaching decoding. Earlier assertion messages in that log are
the expected UID/strict-transport mutations, not extra product failures.
Other exact-byte API29/34 runs pass, so the failure is retained and is not
renamed a generic emulator flake.

The native client has a real race: authorization can observe a dead RPC
before binderDied. The old generic platform-error invalidation detaches the
death recipient; the later callback is then stale and cannot change that
state to unavailable. The corrected handler consumes a DeadObjectException
or independently checks IBinder.isBinderAlive, revokes the same owned epoch,
and reports unavailable for a dead authority. A live broker's malformed RPC
remains a platform error. A stale failure cannot revoke a newer connection
or return authorized without authenticating/filling that call's UID/epoch.

Type sketch: actual Binder liveness, RPC validity, capability state and epoch
are distinct. This changes an Android capability-loss classification; it
does not alter Idriç navigation, network transport, response/source admission,
package identity, signing, stored bytes or browser history. No direct network
fallback is added.

The separate missing-death-callback APK now serves as a focused positive for
RPC revocation: after real Shizuku is stopped, polling must still report
unavailable. Its old isolated mutation is no longer necessarily a bad
polling result once that fallback is correct. A new compiled
stale-authorization control disables both notification and failed-RPC
revocation and must fail the exact death assertion. Both source variants,
DEX/ELF/APK hashes and exact phases are retained. Only the verified emulator
runs these fixture substitutions; no user installation is changed.

ai-ci 50bb6015febe6df1cc9e365557fd8cf0e43fb3a8 adds the explicit reader-v2
profile and bounded failure-state diagnostics. The SDK waits for EOF from
its fixed server-stop command rather than immediately closing its descriptor.
The prior reader profile preserves old frozen semantics. The actual normal
client's denial/grant/death/reconnect and all source/UI controls remain
required. No failed assertion is accepted as a passing client.

This source incorporates, rather than overwrites, 569ad902a37bc63df6298d7eea283724313e2f79's
safe optional Unicode-break repair and new generation mutation. It pins the
qualified android-NDK PR head 6e1c9619a96f28494ad891a32a06edfb72f3e25b;
its compiler implementation is the same repaired code used at the parent
0f69ef2, with the current source-bound qualification wiring.

At authoring, this changed head's compilation/ART/installed/source/search
checks are NOT_RUN. Earlier signed IB966a remains an immutable historical
candidate. Physical TAB_P10 remains BLOCKED; no merge or release is made.
