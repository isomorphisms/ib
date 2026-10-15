#!/bin/sh
# Fast host contract test: synthetic pointer + v2 receipts, NOT canonical appendfat.
set -eu
helper=${IB_TEST_VERIFIED_ARENA_PIPE:-}
test -n "$helper" && test -x "$helper" || exit 2
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -m 700 "$work/private" "$work/private/receipts"
printf '<p>IB C67</p>\n' > "$work/fixture.html"
test "$(wc -c < "$work/fixture.html" | tr -d '[:space:]')" -eq 14
truncate -s 16777216 "$work/private/responses.arena"
dd if="$work/fixture.html" of="$work/private/responses.arena" conv=notrunc status=none
printf '14\n' > "$work/private/responses.arena.used"
: > "$work/private/responses.arena.lock"
digest=$(sha256sum "$work/fixture.html" | cut -d ' ' -f 1)
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
        printf 'arena_offset\t0\n'
        printf 'arena_end\t14\n'
        printf 'byte_count\t14\n'
        printf 'sha256\t%s\n' "$digest"
        printf 'reopen_check\tMATCH\n'
        printf 'acquisition_complete\tYES\n'
        printf 'navigation_admitted\tYES\n'
    } > "$work/private/receipts/$id.tsv"
}
receipt_v2 verified 1
receipt_v2 verified2 2
{
    printf 'schema\tib-icu-arena-v1\n'
    printf 'request_id\tlegacy\n'
    printf 'requested_url\thttps://example.com/\n'
    printf 'resolved_url\tUNOBSERVED\n'
    printf 'http_status\tUNOBSERVED\n'
    printf 'mime_type\tUNOBSERVED\n'
    printf 'transport\ticu-app-process-via-binder-shizuku\n'
    printf 'arena_name\tresponses.arena\n'
    printf 'arena_offset\t0\n'
    printf 'arena_end\t14\n'
    printf 'byte_count\t14\n'
    printf 'sha256\t%s\n' "$digest"
    printf 'reopen_check\tMATCH\n'
    printf 'navigation_admitted\tNO\n'
} > "$work/private/receipts/legacy.tsv"
"$helper" "$work/private"
printf 'fixture\tsynthetic-pointer-not-canonical-appendfat\n'
