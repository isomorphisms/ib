#!/bin/sh
# Execute with a real compiled canonical appendfat_arena (pin source in CI).
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
mkdir -p "$work/lib" "$work/bin" "$work/test-commands"
cp "$root/lib/android_shizuku_icu_provider.grease" "$root/lib/android_binder_icu.grease" "$work/lib/"
cp "$root/bin/ib_android_icu_arena.grease" "$work/bin/"

# A fake Binder authority adapter exercises only the host boundary. Physical
# Shizuku, app_process, TLS and real ICU remain explicitly NOT_RUN here.
cat > "$work/lib/android_shizuku_host.grease" <<'SHIZUKU'
ib_shizuku_authority() {
  if test "${IB_TEST_DENIED:-0}" = 1; then printf 'uid:10123\n';
  else printf 'shell\n'; fi
}
ib_shizuku_run_bounded() {
  test "$1" = 30 || return 99
  printf '%s\n' "$2" >> "$IB_TEST_REMOTE_COMMANDS"
  case "$2" in
    *"app_process -Dicu.library="*" Idric.Generated get "*) ;;
    *) return 91 ;;
  esac
  case "${IB_TEST_MODE:-ok}" in
    ok) printf '<html><body>C67 π ☼</body></html>\n' ;;
    binary) printf 'bin\000ary\377\n' ;;
    fail) printf '<html>partial'; return 29 ;;
    big) head -c 4194305 /dev/zero ;;
    *) return 98 ;;
  esac
}
SHIZUKU

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
run() { sh "$work/bin/ib_android_icu_arena.grease" "$1" "$2" "$work/private"; }

run first https://example.com/ > "$work/output1"
test "$(cat "$work/private/responses.arena.used")" -gt 0
first_bytes=$(sed -n 's/^byte_count[[:space:]]*//p' "$work/private/receipts/first.tsv")
test -n "$first_bytes"
test "$(sed -n 's/^arena_offset[[:space:]]*//p' "$work/private/receipts/first.tsv")" = 0
"$reader" slice "$work/private/responses.arena" 0 "$first_bytes" > "$work/first-body"
printf '<html><body>C67 π ☼</body></html>\n' > "$work/expected"
cmp "$work/expected" "$work/first-body"
grep -q "^http_status[[:space:]]*UNOBSERVED$" "$work/private/receipts/first.tsv"
grep -q "^navigation_admitted[[:space:]]*NO$" "$work/private/receipts/first.tsv"

export IB_TEST_MODE=binary
run second 'https://example.com/?x=1&y=2' > "$work/output2"
second_offset=$(sed -n 's/^arena_offset[[:space:]]*//p' "$work/private/receipts/second.tsv")
second_bytes=$(sed -n 's/^byte_count[[:space:]]*//p' "$work/private/receipts/second.tsv")
"$reader" slice "$work/private/responses.arena" "$second_offset" "$second_bytes" > "$work/second-body"
printf 'bin\000ary\377\n' > "$work/expected-binary"
cmp "$work/expected-binary" "$work/second-body"
used=$(cat "$work/private/responses.arena.used")

# Refuse duplicate request identity, a partial producer and oversized input.
if run second https://example.com/ 2>/dev/null; then exit 1; fi
export IB_TEST_MODE=fail
if run incomplete https://example.com/ > /dev/null 2>&1; then exit 1; fi
test ! -e "$work/private/receipts/incomplete.tsv"
test "$(cat "$work/private/responses.arena.used")" = "$used"
export IB_TEST_MODE=big
if run oversized https://example.com/ > /dev/null 2>&1; then exit 1; fi
test "$(cat "$work/private/responses.arena.used")" = "$used"

# No remote command must run for denied authority or invalid/untrusted URL.
export IB_TEST_MODE=ok
export IB_TEST_DENIED=1
before=$(wc -l < "$IB_TEST_REMOTE_COMMANDS")
if run denied https://example.com/ > /dev/null 2>&1; then exit 1; fi
test "$(wc -l < "$IB_TEST_REMOTE_COMMANDS")" = "$before"
unset IB_TEST_DENIED
if run injected "https://example.com/a'\$(touch '$work/INJECTED')" > /dev/null 2>&1; then exit 1; fi
test ! -e "$work/INJECTED"
test "$(wc -l < "$IB_TEST_REMOTE_COMMANDS")" = "$before"
if run unsupported file:///etc/passwd >/dev/null 2>&1; then exit 1; fi
test "$(cat "$work/private/responses.arena.used")" = "$used"

# Independent re-reader guards uncommitted capacity and malformed used pointer.
if "$reader" slice "$work/private/responses.arena" "$used" 1 >/dev/null 2>&1; then exit 1; fi
if "$reader" slice "$work/private/responses.arena" 0 4194305 >/dev/null 2>&1; then exit 1; fi
cp "$work/private/responses.arena.used" "$work/used.bak"
printf '99999999999999999\n' > "$work/private/responses.arena.used"
if "$reader" slice "$work/private/responses.arena" 0 1 >/dev/null 2>&1; then exit 1; fi
cp "$work/used.bak" "$work/private/responses.arena.used"
"$reader" slice "$work/private/responses.arena" 0 "$first_bytes" > "$work/restored"
cmp "$work/first-body" "$work/restored"

printf '%s\n' 'PASS: mocked Binder/Shizuku handoff, committed arena read, binary bytes, failure/oversize/duplicate/denial/injection/corruption refusal' \
  'NOT_RUN: actual ICU, real HTTPS, physical Shizuku, C67 reader IPC/UI or durable navigation'
