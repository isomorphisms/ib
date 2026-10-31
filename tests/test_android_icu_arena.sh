#!/bin/sh
# Host contract test with real compiled canonical appendfat_arena and native
# reader. The Shizuku + ICU #35 producer is deliberately mocked here.
set -eu
root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
arena_tool=${IB_APPEND_ARENA:-}
reader=${IB_ARENA_SLICE:-}
test -x "$arena_tool" && test -x "$reader" || {
  printf 'IB_APPEND_ARENA and IB_ARENA_SLICE executables required\n' >&2
  exit 2
}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/lib" "$work/bin" "$work/test-commands" "$work/mock"
cp "$root/lib/android_shizuku_icu_provider.grease" "$root/lib/android_binder_icu.grease" "$work/lib/"
cp "$root/bin/ib_android_icu_arena.grease" "$work/bin/"
cp "$root/bin/ib_android_arena_read.grease" "$work/bin/"

# Simulate the *framed* output of a single successful PR #35 ICU invocation.
# The expected remote command must contain both paired capture env variables,
# signed artifact checks and the actual Idric.Generated get path. These tests
# say nothing about a deployed ICU binary, TLS or real rish.
cat > "$work/lib/android_shizuku_host.grease" <<'SHIZUKU'
ib_shizuku_authority() {
  if test "${IB_TEST_DENIED:-0}" = 1; then printf 'uid:10123\n'
  else printf 'shell\n'; fi
}
ib_shizuku_run_bounded() {
  test "$1" = 30 || return 99
  printf '%s\n' "$2" >> "$IB_TEST_REMOTE_COMMANDS"
  case "$2" in
    *ICU_CAPTURE_BODY_PATH*ICU_CAPTURE_METADATA_PATH*Idric.Generated\ --http1.1\ get*) ;;
    *) return 91 ;;
  esac
  case "$2" in *sha256sum*'IBICU01 %08d %08d'*) ;; *) return 92 ;; esac
  stub_work=$(mktemp -d "$IB_TEST_TMP/stub.XXXXXXXX") || return 90
  stub_body="$stub_work/body"
  stub_meta="$stub_work/meta"
  mode=${IB_TEST_MODE:-ok}
  status=200
  resolved=https://example.com/final
  redirect_count=2
  content_type='text/html; charset=utf-8'
  content_encoding=identity
  content_range=absent
  transfer_decoding=identity
  utf8=valid
  case "$mode" in
    ok|legacy_eight|chunked|range|partial_extensions|unknown_transfer|duplicate_extension|wrong_length|wrong_declared|duplicate|bad_status|bad_resolved|missing) printf '<html><body>C67 π ☼</body></html>\n' > "$stub_body" ;;
    binary) printf 'bin\000ary\377\n' > "$stub_body"; content_type=application/octet-stream; utf8=invalid ;;
    utf8_invalid) printf 'bad\377' > "$stub_body"; utf8=invalid ;;
    no_charset) printf '<p>no charset</p>' > "$stub_body"; content_type=text/html ;;
    gzip) printf '<p>gzip marker</p>' > "$stub_body"; content_encoding=gzip ;;
    fail) printf '<html>partial'; rm -rf "$stub_work"; return 29 ;;
    bad_frame) printf 'not-an-ICU-envelope\n'; rm -rf "$stub_work"; return 0 ;;
    oversized) head -c 4194305 /dev/zero > "$stub_body" ;;
    *) rm -rf "$stub_work"; return 98 ;;
  esac
  case "$mode" in
    bad_status) status=404 ;;
    bad_resolved) resolved='javascript:alert(1)' ;;
    range) content_range='bytes 0-4/10' ;;
    chunked) transfer_decoding=chunked ;;
    unknown_transfer) transfer_decoding=gzip ;;
  esac
  body_bytes=$(wc -c < "$stub_body" | tr -d '[:space:]')
  declared=$body_bytes
  received=$body_bytes
  case "$mode" in
    wrong_length) received=$((body_bytes - 1)) ;;
    wrong_declared) declared=$((body_bytes - 1)) ;;
  esac
  {
    printf 'http_status\t%s\n' "$status"
    printf 'content_type\t%s\n' "$content_type"
    printf 'content_encoding\t%s\n' "$content_encoding"
    printf 'declared_length\t%s\n' "$declared"
    printf 'received_length\t%s\n' "$received"
    printf 'utf8\t%s\n' "$utf8"
    printf 'resolved_url\t%s\n' "$resolved"
    printf 'redirect_count\t%s\n' "$redirect_count"
    if test "$mode" != legacy_eight; then
      printf 'content_range\t%s\n' "$content_range"
      if test "$mode" != partial_extensions; then
        printf 'transfer_decoding\t%s\n' "$transfer_decoding"
      fi
    fi
    if test "$mode" = duplicate_extension; then printf 'content_range\tabsent\n'; fi
    if test "$mode" = duplicate; then printf 'http_status\t200\n'; fi
  } > "$stub_meta"
  if test "$mode" = missing; then
    sed -i '/^redirect_count/d' "$stub_meta"
  fi
  meta_bytes=$(wc -c < "$stub_meta" | tr -d '[:space:]')
  printf 'IBICU01 %08d %08d\n' "$meta_bytes" "$body_bytes"
  cat "$stub_meta"
  cat "$stub_body"
  rm -rf "$stub_work"
}
SHIZUKU

export IB_TEST_TMP="$work/mock"
export IB_TEST_REMOTE_COMMANDS="$work/test-commands/commands.txt"
export IB_C67_ICU_DEX=/data/local/tmp/ib-icu/classes.dex
export IB_C67_ICU_SO=/data/local/tmp/ib-icu/libicu_transport.so
export IB_C67_ICU_DEX_SHA256=1111111111111111111111111111111111111111111111111111111111111111
export IB_C67_ICU_SO_SHA256=2222222222222222222222222222222222222222222222222222222222222222
export IB_APPEND_ARENA="$arena_tool"
export IB_ARENA_SLICE="$reader"
mkdir -p "$work/private"
chmod 700 "$work/private"
"$arena_tool" create "$work/private/responses.arena" 16777216 >/dev/null
run() { sh "$work/bin/ib_android_icu_arena.grease" "$1" "$2" "$work/private" "${3:-1}"; }
readback() { sh "$work/bin/ib_android_arena_read.grease" "$1" "$work/private"; }
field() { awk -F '\t' -v name="$2" '$1==name {print $2}' "$work/private/receipts/$1.tsv"; }

run first https://example.com/start 17 > "$work/output1"
test "$(field first schema)" = ib-icu-arena-v2
test "$(field first http_status)" = 200
test "$(field first resolved_url)" = https://example.com/final
test "$(field first redirect_count)" = 2
test "$(field first mime_type)" = text/html
test "$(field first charset)" = utf-8
test "$(field first source_generation)" = 17
test "$(field first source_key)" = arena:first
test "$(field first acquisition_complete)" = YES
test "$(field first navigation_admitted)" = YES
first_bytes=$(field first byte_count)
test "$(field first arena_offset)" = 0
test "$(cat "$work/private/responses.arena.used")" -gt 0
printf '<html><body>C67 π ☼</body></html>\n' > "$work/expected"
readback first > "$work/first-body"
cmp -s "$work/expected" "$work/first-body"

for mode in legacy_eight chunked; do
  export IB_TEST_MODE="$mode"
  run "$mode" https://example.com/start 17 > "$work/output_$mode"
  test "$(field "$mode" navigation_admitted)" = YES
  readback "$mode" > "$work/read_$mode"
  cmp -s "$work/expected" "$work/read_$mode"
done

export IB_TEST_MODE=binary
run second https://example.com/binary 18 > "$work/output2"
test "$(field second navigation_admitted)" = NO
test "$(field second mime_type)" = application/octet-stream
test "$(field second utf8)" = invalid
printf 'bin\000ary\377\n' > "$work/expected-binary"
readback second > "$work/read2"
cmp -s "$work/expected-binary" "$work/read2"

# Correct non-HTML, unsupported encoding, absent charset and invalid UTF-8
# remain readable as bounded bytes but never acquire navigation authority.
for mode in no_charset gzip utf8_invalid; do
  export IB_TEST_MODE="$mode"
  run "$mode" https://example.com/ > /dev/null
  test "$(field "$mode" navigation_admitted)" = NO
  readback "$mode" > "$work/$mode.read"
  test -s "$work/$mode.read"
done

# Mutated receipts must never emit any prefix. Also refuse forged admission
# on an otherwise valid non-HTML committed range.
cp "$work/private/receipts/first.tsv" "$work/first.original.tsv"
sed -i 's/^sha256[[:space:]].*/sha256\t0000000000000000000000000000000000000000000000000000000000000000/' \
  "$work/private/receipts/first.tsv"
if readback first > "$work/read-invalid" 2>/dev/null; then exit 1; fi
test ! -s "$work/read-invalid"
cp "$work/first.original.tsv" "$work/private/receipts/first.tsv"
cp "$work/private/receipts/second.tsv" "$work/second.original.tsv"
sed -i 's/^navigation_admitted[[:space:]].*/navigation_admitted\tYES/' \
  "$work/private/receipts/second.tsv"
if readback second > "$work/forged-admission" 2>/dev/null; then exit 1; fi
test ! -s "$work/forged-admission"
cp "$work/second.original.tsv" "$work/private/receipts/second.tsv"

# v1 can still supply verified bytes but cannot be upgraded to navigation.
{
  printf 'schema\tib-icu-arena-v1\n'
  printf 'request_id\tlegacy\n'
  printf 'requested_url\t%s\n' "$(field first requested_url)"
  printf 'resolved_url\tUNOBSERVED\nhttp_status\tUNOBSERVED\nmime_type\tUNOBSERVED\n'
  printf 'transport\ticu-app-process-via-binder-shizuku\narena_name\tresponses.arena\n'
  printf 'arena_offset\t%s\n' "$(field first arena_offset)"
  printf 'arena_end\t%s\n' "$(field first arena_end)"
  printf 'byte_count\t%s\n' "$(field first byte_count)"
  printf 'sha256\t%s\n' "$(field first sha256)"
  printf 'reopen_check\tMATCH\nnavigation_admitted\tNO\n'
} > "$work/private/receipts/legacy.tsv"
readback legacy > "$work/legacy-body"
cmp -s "$work/expected" "$work/legacy-body"

# None of these producer failures may advance the commit pointer or publish a
# receipt. Fresh request IDs are used because request locks are single-use.
used=$(cat "$work/private/responses.arena.used")
expect_refusal() {
  mode=$1
  export IB_TEST_MODE="$mode"
  if run "reject_$mode" https://example.com/ > "$work/refused" 2>/dev/null; then exit 1; fi
  test ! -e "$work/private/receipts/reject_$mode.tsv"
  test "$(cat "$work/private/responses.arena.used")" = "$used"
}
for mode in fail bad_frame oversized wrong_length wrong_declared duplicate bad_status bad_resolved missing range partial_extensions unknown_transfer duplicate_extension; do
  expect_refusal "$mode"
done
export IB_TEST_MODE=ok
if run second https://example.com/ > /dev/null 2>&1; then exit 1; fi
if run zero https://example.com/ 0 > /dev/null 2>&1; then exit 1; fi
if run leading https://example.com/ 017 > /dev/null 2>&1; then exit 1; fi

# Denied Shizuku or unsafe URLs must not run a remote command.
export IB_TEST_DENIED=1
before=$(wc -l < "$IB_TEST_REMOTE_COMMANDS")
if run denied https://example.com/ > /dev/null 2>&1; then exit 1; fi
test "$(wc -l < "$IB_TEST_REMOTE_COMMANDS")" = "$before"
unset IB_TEST_DENIED
if run injected "https://example.com/a'\$(touch '$work/INJECTED')" > /dev/null 2>&1; then exit 1; fi
test ! -e "$work/INJECTED"
test "$(wc -l < "$IB_TEST_REMOTE_COMMANDS")" = "$before"
if run unsupported file:///etc/passwd > /dev/null 2>&1; then exit 1; fi
test "$(cat "$work/private/responses.arena.used")" = "$used"

# Independent native reader guards out-of-range and malformed used pointers.
if "$reader" slice "$work/private/responses.arena" "$used" 1 >/dev/null 2>&1; then exit 1; fi
if "$reader" slice "$work/private/responses.arena" 0 4194305 >/dev/null 2>&1; then exit 1; fi
cp "$work/private/responses.arena.used" "$work/used.bak"
printf '99999999999999999\n' > "$work/private/responses.arena.used"
if "$reader" slice "$work/private/responses.arena" 0 1 >/dev/null 2>&1; then exit 1; fi
cp "$work/used.bak" "$work/private/responses.arena.used"
"$reader" slice "$work/private/responses.arena" 0 "$first_bytes" > "$work/restored"
cmp -s "$work/expected" "$work/restored"

printf '%s\n' \
  'PASS: framed ICU #35 metadata/body contract, observed redirect, canonical range/digest, v1 refusal, conditional navigation, binary fidelity and negative admission checks' \
  'NOT_RUN: real ICU HTTPS, Android DEX/ARM32, physical Shizuku, C67 UI, private PFD or durable navigation'
