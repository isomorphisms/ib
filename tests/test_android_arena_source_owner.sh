#!/bin/sh
# Source-owner test consumes the real canonical appendfat_arena, not a mock.
# Admitted v2 HTML source is required before private PFD handoff.
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
printf '<p>IB C67</p>\n' > "$work/fixture.html"
test "$(wc -c < "$work/fixture.html" | tr -d '[:space:]')" -eq 14
append=$("$arena_tool" append "$work/private/responses.arena" "$work/fixture.html")
old=$(printf '%s\n' "$append" | sed -n 's/^APPENDED old_used=\([0-9][0-9]*\) new_used=[0-9][0-9]* capacity_bytes=[0-9][0-9]*$/\1/p')
new=$(printf '%s\n' "$append" | sed -n 's/^APPENDED old_used=[0-9][0-9]* new_used=\([0-9][0-9]*\) capacity_bytes=[0-9][0-9]*$/\1/p')
test "$old" = 0 && test "$new" = 14
"$slice" slice "$work/private/responses.arena" "$old" 14 > "$work/readback"
cmp "$work/fixture.html" "$work/readback"
digest=$(sha256sum "$work/fixture.html" | cut -d ' ' -f 1)
mkdir -m 700 "$work/private/receipts" "$work/private/staging"

receipt_v2() {
    id=$1 gen=$2
    {
        printf 'schema\tib-icu-arena-v2\n'
        printf 'request_id\t%s\n' "$id"
        printf 'source_key\tarena:%s\n' "$id"
        printf 'source_generation\t%s\n' "$gen"
        printf 'requested_url\thttps://example.com/start\n'
        printf 'resolved_url\thttps://example.com/final\n'
        printf 'http_status\t200\n'
        printf 'mime_type\ttext/html\n'
        printf 'charset\tutf-8\n'
        printf 'content_encoding\tidentity\n'
        printf 'utf8\tvalid\n'
        printf 'redirect_count\t1\n'
        printf 'declared_length\t14\n'
        printf 'received_length\t14\n'
        printf 'transport\ticu-app-process-via-binder-shizuku\n'
        printf 'arena_name\tresponses.arena\n'
        printf 'arena_offset\t%s\n' "$old"
        printf 'arena_end\t%s\n' "$new"
        printf 'byte_count\t14\n'
        printf 'sha256\t%s\n' "$digest"
        printf 'reopen_check\tMATCH\n'
        printf 'acquisition_complete\tYES\n'
        printf 'navigation_admitted\tYES\n'
    } > "$work/private/receipts/$id.tsv"
}
receipt_v2 verified 1
receipt_v2 verified2 2

# A fully hash-matching legacy body is nevertheless not authorized as HTML.
{
    printf 'schema\tib-icu-arena-v1\n'
    printf 'request_id\tlegacy\n'
    printf 'requested_url\thttps://example.com/\n'
    printf 'resolved_url\tUNOBSERVED\n'
    printf 'http_status\tUNOBSERVED\n'
    printf 'mime_type\tUNOBSERVED\n'
    printf 'transport\ticu-app-process-via-binder-shizuku\n'
    printf 'arena_name\tresponses.arena\n'
    printf 'arena_offset\t%s\n' "$old"
    printf 'arena_end\t%s\n' "$new"
    printf 'byte_count\t14\n'
    printf 'sha256\t%s\n' "$digest"
    printf 'reopen_check\tMATCH\n'
    printf 'navigation_admitted\tNO\n'
} > "$work/private/receipts/legacy.tsv"
# A separate AI-CI consumer independently checks this fixture's observed
# admission and committed bytes before the native PFD source owner runs.
aici=${IB_AICI_SOURCE_VERIFY:-}
test -n "$aici" && test -f "$aici" || {
    printf 'source admission requires pinned independent AI-CI verifier\n' >&2
    exit 1
}
sh "$aici" "$work/private" verified 1 https://example.com/start
sh "$aici" "$work/private" verified2 2 https://example.com/start
if sh "$aici" "$work/private" legacy 1 https://example.com/ > "$work/legacy.aici" 2>/dev/null; then
    printf 'independent verifier admitted a v1 receipt\n' >&2
    exit 1
fi
test ! -s "$work/legacy.aici"
"$helper" "$work/private"
printf '%s\n' 'checked_arena=actual_appends' 'pfd_source=v2_admitted_html_only' 'reader_body_sha256=verified'
