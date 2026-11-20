#!/bin/sh
# Host-only integration against the EXACT external PR #152 ICU/Arena producer
# and reader with a marked MOCK ICU packet. No live network or Android device.
set -eu
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
test -f "$root/.deps/ib-arena/bin/ib_android_icu_arena.grease" || {
  echo 'pinned upstream Arena source unavailable' >&2; exit 2;
}
arena_tool=${IB_APPEND_ARENA:-}
slice_tool=${IB_ARENA_SLICE:-}
converter=${IB_HTML_PREPAINT_EXEC:-}
oracle=${IB_PREPAINT_ORACLE:-}
test -x "$arena_tool" && test -x "$slice_tool" &&
  test -x "$converter" && test -x "$oracle" || {
    echo 'compiled Arena/Idriç/independent consumer executables required' >&2; exit 2;
  }
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/lib" "$work/bin" "$work/private" "$work/output"
chmod 700 "$work/private"
cp "$root/.deps/ib-arena/bin/ib_android_icu_arena.grease" "$work/bin/"
cp "$root/.deps/ib-arena/bin/ib_android_arena_read.grease" "$work/bin/"
cp "$root/.deps/ib-arena/lib/android_shizuku_icu_provider.grease" "$work/lib/"
cp "$root/.deps/ib-arena/lib/android_binder_icu.grease" "$work/lib/"

# This mock replaces *only* the Shizuku command invocation. Production
# ICU PR #35 capture, framing, validation, canonical Arena and source
# reopen logic remain unchanged, from exact upstream SHA.
cat > "$work/lib/android_shizuku_host.grease" <<'MOCK'
ib_shizuku_authority() { printf 'shell\n'; }
ib_shizuku_run_bounded() {
  test "$1" = 30 || return 90
  case "$2" in *ICU_CAPTURE_BODY_PATH*ICU_CAPTURE_METADATA_PATH*Idric.Generated\ get*) ;; *) return 91 ;; esac
  body=$IB_MOCK_SOURCE_BODY
  test -f "$body" || return 92
  body_bytes=$(wc -c < "$body" | tr -d '[:space:]')
  {
    printf 'http_status\t200\n'
    printf 'content_type\ttext/html; charset=utf-8\n'
    printf 'content_encoding\tidentity\n'
    printf 'declared_length\t%s\n' "$body_bytes"
    printf 'received_length\t%s\n' "$body_bytes"
    printf 'utf8\tvalid\n'
    printf 'resolved_url\thttps://example.com/\n'
    printf 'redirect_count\t0\n'
  } > "$IB_MOCK_METADATA"
  metadata_bytes=$(wc -c < "$IB_MOCK_METADATA" | tr -d '[:space:]')
  printf 'IBICU01 %08d %08d\n' "$metadata_bytes" "$body_bytes"
  cat "$IB_MOCK_METADATA"
  cat "$body"
}
MOCK

export IB_MOCK_SOURCE_BODY="$root/tests/fixtures/ib-html/example.html"
export IB_MOCK_METADATA="$work/mock-icu-metadata.tsv"
export IB_APPEND_ARENA="$arena_tool"
export IB_ARENA_SLICE="$slice_tool"
export IB_C67_ICU_DEX=/data/local/tmp/ib-icu/classes.dex
export IB_C67_ICU_SO=/data/local/tmp/ib-icu/libicu_transport.so
export IB_C67_ICU_DEX_SHA256=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
export IB_C67_ICU_SO_SHA256=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
export IB_HTML_PREPAINT_EXEC="$converter"
export IB_PREPAINT_ORACLE="$oracle"
"$arena_tool" create "$work/private/responses.arena" 16777216 >/dev/null
sh "$work/bin/ib_android_icu_arena.grease" host-example \
  https://example.com/ "$work/private" 17 > "$work/producer.tsv"
grep -Fx "$(printf 'committed\thost-example\t0\t%s\tYES' \
  "$(wc -c < "$IB_MOCK_SOURCE_BODY" | tr -d '[:space:]')")" "$work/producer.tsv"
receipt="$work/private/receipts/host-example.tsv"
test "$(awk -F '\t' '$1=="schema" {print $2}' "$receipt")" = ib-icu-arena-v2
test "$(awk -F '\t' '$1=="source_generation" {print $2}' "$receipt")" = 17
test "$(awk -F '\t' '$1=="navigation_admitted" {print $2}' "$receipt")" = YES

# Host sh entrypoint for the unmodified upstream verified Arena reader.
cat > "$work/ib-arena-reader" <<'WRAPPER'
#!/bin/sh
exec sh "$IB_TEST_READER_SCRIPT" "$@"
WRAPPER
chmod 700 "$work/ib-arena-reader"
export IB_TEST_READER_SCRIPT="$work/bin/ib_android_arena_read.grease"
export IB_ARENA_READ_EXEC="$work/ib-arena-reader"
source_digest=$(sha256sum "$IB_MOCK_SOURCE_BODY" | cut -d ' ' -f 1)
source_length=$(wc -c < "$IB_MOCK_SOURCE_BODY" | tr -d '[:space:]')
sh "$IB_ARENA_READ_EXEC" host-example "$work/private" > "$work/reopened.html"
cmp "$work/reopened.html" "$IB_MOCK_SOURCE_BODY"

projection="$work/output/host-example.prepaint"
sh "$root/bin/ib_arena_to_html_prepaint.grease" \
  host-example "$work/private" 17 "$projection" > "$work/conversion.tsv"
cmp "$root/tests/fixtures/ib-html/example.prepaint" "$projection"
"$IB_PREPAINT_ORACLE" "$projection"

if test -n "${IB_AICI_PREPAINT_VERIFIER:-}"; then
  {
    printf 'source_key\tarena:host-example\n'
    printf 'source_generation\t17\n'
    printf 'source_sha256\t%s\n' "$source_digest"
    printf 'source_bytes\t%s\n' "$source_length"
  } > "$work/source-admission.tsv"
  sh "$IB_AICI_PREPAINT_VERIFIER" "$projection" \
    "$root/tests/fixtures/ib-html/example.prepaint" \
    "$work/source-admission.tsv" "$work/reopened.html" arena:host-example 17
fi

cp "$projection" "$work/last-valid.prepaint"
cp "$receipt" "$work/valid-receipt.tsv"
refuse() {
  case_name=$1
  if sh "$root/bin/ib_arena_to_html_prepaint.grease" \
      host-example "$work/private" 17 "$projection" \
      > "$work/refused.out" 2> "$work/refused.err"; then
    echo "accepted mutated v2 Arena source: $case_name" >&2
    exit 1
  fi
  cmp "$projection" "$work/last-valid.prepaint"
  printf 'arena-v2-refusal\t%s\tPASS\n' "$case_name"
}
sed 's@^sha256.*@sha256\t0000000000000000000000000000000000000000000000000000000000000000@' \
  "$work/valid-receipt.tsv" > "$receipt"
refuse corrupted_arena_digest
sed 's@^source_generation.*@source_generation\t18@' "$work/valid-receipt.tsv" > "$receipt"
refuse stale_source_generation
sed 's@^navigation_admitted.*@navigation_admitted\tNO@' "$work/valid-receipt.tsv" > "$receipt"
refuse unadmitted_navigation
cp "$work/valid-receipt.tsv" "$receipt"

if test -n "${IB_HTML_PREPAINT_RECEIPT_ROOT:-}"; then
  mkdir -p "$IB_HTML_PREPAINT_RECEIPT_ROOT"
  cp "$projection" "$IB_HTML_PREPAINT_RECEIPT_ROOT/arena-mocked.prepaint"
  cp "$work/valid-receipt.tsv" "$IB_HTML_PREPAINT_RECEIPT_ROOT/arena-mocked-source.tsv"
  cp "$work/conversion.tsv" "$IB_HTML_PREPAINT_RECEIPT_ROOT/arena-mocked-conversion.tsv"
  {
    printf 'source_kind\tMOCKED_ICU_V2_NOT_LIVE\n'
    printf 'source_sha256\t%s\n' "$source_digest"
    printf 'source_bytes\t%s\n' "$source_length"
    printf 'source_generation\t17\n'
    printf 'upstream_source_commit\t%s\n' "$(git -C "$root/.deps/ib-arena" rev-parse HEAD)"
  } > "$IB_HTML_PREPAINT_RECEIPT_ROOT/arena-mocked-provenance.tsv"
fi
printf 'arena-v2-mock-through-actual-idric\tPASS\n'
printf 'real-icu-https\tNOT_RUN\nphysical-c67\tNOT_RUN\n'
