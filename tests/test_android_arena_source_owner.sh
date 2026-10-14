#!/bin/sh
# Source owner test consumes the real canonical appendfat_arena, not a mock.
set -eu
arena_tool=${IB_APPEND_ARENA:-}
helper=${IB_TEST_VERIFIED_ARENA_PIPE:-}
slice=${IB_ARENA_SLICE:-}
test -n "$arena_tool" && test -x "$arena_tool" &&
test -n "$helper" && test -x "$helper" &&
test -n "$slice" && test -x "$slice" || {
    echo 'canonical Arena, compiled reader and test helper required' >&2
    exit 2
}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -m 700 "$work/private"
"$arena_tool" create "$work/private/responses.arena" 16777216 > "$work/create.txt"
printf 'IB\000C67\377\n' > "$work/fixture.bin"
test "$(wc -c < "$work/fixture.bin" | tr -d '[:space:]')" -eq 8
append=$("$arena_tool" append "$work/private/responses.arena" "$work/fixture.bin")
old=$(printf '%s\n' "$append" | sed -n 's/^APPENDED old_used=\([0-9][0-9]*\) new_used=[0-9][0-9]* capacity_bytes=[0-9][0-9]*$/\1/p')
new=$(printf '%s\n' "$append" | sed -n 's/^APPENDED old_used=[0-9][0-9]* new_used=\([0-9][0-9]*\) capacity_bytes=[0-9][0-9]*$/\1/p')
test "$old" = 0 && test "$new" = 8
"$slice" slice "$work/private/responses.arena" "$old" 8 > "$work/readback"
cmp "$work/fixture.bin" "$work/readback"
digest=$(sha256sum "$work/fixture.bin" | cut -d ' ' -f 1)
mkdir -m 700 "$work/private/receipts"
{
    printf 'schema\tib-icu-arena-v1\n'
    printf 'request_id\tverified\n'
    printf 'requested_url\thttps://example.com/\n'
    printf 'resolved_url\tUNOBSERVED\n'
    printf 'http_status\tUNOBSERVED\n'
    printf 'mime_type\tUNOBSERVED\n'
    printf 'transport\ticu-app-process-via-binder-shizuku\n'
    printf 'arena_name\tresponses.arena\n'
    printf 'arena_offset\t%s\n' "$old"
    printf 'arena_end\t%s\n' "$new"
    printf 'byte_count\t8\n'
    printf 'sha256\t%s\n' "$digest"
    printf 'reopen_check\tMATCH\n'
    printf 'navigation_admitted\tNO\n'
} > "$work/private/receipts/verified.tsv"
"$helper" "$work/private"
printf '%s\n' 'checked_arena=actual_appends' 'reader_body_sha256=verified'
