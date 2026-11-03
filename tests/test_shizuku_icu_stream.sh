#!/bin/sh
# Shell/Binder-shaped provider contract. No actual Shizuku, network or
# imported Android Binder handle exists in this host test.
set -eu
root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
. "$root/lib/android_shizuku_icu_provider.grease"
. "$root/lib/android_binder_icu.grease"

IB_C67_ICU_DEX=/data/local/tmp/ib-icu/classes.dex
IB_C67_ICU_SO=/data/local/tmp/ib-icu/libicu_transport.so
IB_C67_ICU_DEX_SHA256=1111111111111111111111111111111111111111111111111111111111111111
IB_C67_ICU_SO_SHA256=2222222222222222222222222222222222222222222222222222222222222222
export IB_C67_ICU_DEX IB_C67_ICU_SO IB_C67_ICU_DEX_SHA256 IB_C67_ICU_SO_SHA256

ib_shizuku_authority() {
    if test "${TEST_DENIED:-0}" = 1; then printf 'uid:10222\n'
    else printf 'shell\n'; fi
}
ib_shizuku_run_bounded() {
    test "$1" = 30 || return 61
    printf '%s\n' "$2" >> "$work/commands"
    printf 'call\n' >> "$work/invocations"
    case "$2" in
        *"app_process -Dicu.library="*" Idric.Generated --http1.1 get "*) ;;
        *) return 62 ;;
    esac
    case "${TEST_RESPONSE:-success}" in
        success) printf 'IB\000C67\377\n' ;;
        partial) printf 'fragment\000'; return 29 ;;
        *) return 63 ;;
    esac
}
ib_binder_icu_stream https://example.com/ > "$work/body"
printf 'IB\000C67\377\n' > "$work/expected"
cmp "$work/expected" "$work/body"
grep -F "sha256sum '/data/local/tmp/ib-icu/classes.dex'" "$work/commands" >/dev/null
test "$(wc -l < "$work/invocations")" -eq 1

TEST_RESPONSE=partial
if ib_binder_icu_stream https://example.com/ > "$work/partial"; then
    echo 'partial stdout falsely succeeded' >&2
    exit 1
fi
test -s "$work/partial"
# The byte stream is discarded by the app when FINISH gets nonzero 29.
TEST_RESPONSE=success
TEST_DENIED=1
before=$(wc -l < "$work/invocations")
if ib_binder_icu_stream https://example.com/ > "$work/denied"; then
    echo 'unauthorized Shizuku erroneously succeeded' >&2
    exit 1
fi
test ! -s "$work/denied"
test "$(wc -l < "$work/invocations")" -eq "$before"
TEST_DENIED=0
if ib_binder_icu_stream "https://example.com/a'" \
        > "$work/injected"; then
    echo 'URL command injection erroneously succeeded' >&2
    exit 1
fi
test ! -e "$work/INJECTED"
test "$(wc -l < "$work/invocations")" -eq "$before"
printf 'PASS: pinned ICU binary stdout, nonzero producer propagation, denial and quoting refusal\n'
printf 'NOT_RUN: actual Shizuku/rish, Android Binder, ICU HTTPS, physical C67\n'
