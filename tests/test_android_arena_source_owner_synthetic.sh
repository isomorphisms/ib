#!/bin/sh
# Fast host contract test: synthetic pointer+receipt, NOT canonical appendfat.
set -eu
helper=${IB_TEST_VERIFIED_ARENA_PIPE:-}
test -n "$helper" && test -x "$helper" || exit 2
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -m 700 "$work/private" "$work/private/receipts"
printf 'IB\000C67\377\n' > "$work/fixture.bin"
truncate -s 16777216 "$work/private/responses.arena"
dd if="$work/fixture.bin" of="$work/private/responses.arena" conv=notrunc status=none
printf '8\n' > "$work/private/responses.arena.used"
: > "$work/private/responses.arena.lock"
digest=$(sha256sum "$work/fixture.bin" | cut -d ' ' -f 1)
{
    printf 'schema\tib-icu-arena-v1\n'
    printf 'request_id\tverified\n'
    printf 'requested_url\thttps://example.com/\n'
    printf 'resolved_url\tUNOBSERVED\n'
    printf 'http_status\tUNOBSERVED\n'
    printf 'mime_type\tUNOBSERVED\n'
    printf 'transport\ticu-app-process-via-binder-shizuku\n'
    printf 'arena_name\tresponses.arena\n'
    printf 'arena_offset\t0\n'
    printf 'arena_end\t8\n'
    printf 'byte_count\t8\n'
    printf 'sha256\t%s\n' "$digest"
    printf 'reopen_check\tMATCH\n'
    printf 'navigation_admitted\tNO\n'
} > "$work/private/receipts/verified.tsv"
"$helper" "$work/private"
printf 'fixture\tsynthetic-pointer-not-canonical-appendfat\n'
