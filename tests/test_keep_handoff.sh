#!/bin/sh
set -eu
root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir "$work/bin"
cat > "$work/bin/am" <<'AM'
#!/bin/sh
# No real Android; fixture proves command arguments and receives no credentials.
printf '%s\000' "$@" > "$IB_KEEP_ARGUMENTS"
case "${IB_KEEP_TEST_STATUS:-started}" in
  started) echo 'Starting: Intent { act=android.intent.action.SEND typ=text/plain pkg=com.google.android.keep (has extras) }' ;;
  missing) echo 'Error: Activity not started, unable to resolve Intent' ;;
  failed) echo 'Error: Permission Denial' ;;
  ambiguous) exit 1 ;;
  *) exit 2 ;;
esac
AM
cat > "$work/bin/rish" <<'RISH'
#!/bin/sh
test "$#" -eq 2 && test "$1" = -c || exit 99
sh -c "$2"
RISH
chmod +x "$work/bin/am" "$work/bin/rish"
PATH="$work/bin:$PATH"; export PATH
IB_KEEP_ARGUMENTS="$work/args"; export IB_KEEP_ARGUMENTS
IB_KEEP_TEST_STATUS=started; export IB_KEEP_TEST_STATUS
printf 'Tuğba\nsource: it\047s $literal; $(touch /tmp/ib-keep-UNEXPECTED)\n\n' > "$work/body"
sh "$root/lib/android_keep_handoff.grease" direct "$work/body" > "$work/receipt"
grep -Fx "$(printf 'keep-handoff\tstarted-unconfirmed')" "$work/receipt"
# NUL-delimited argv establishes exact payload bytes, not shell execution.
cc -std=c11 -Wall -Wextra -Werror -pedantic \
  "$root/tests/verify_keep_argv.c" -o "$work/verify-args"
"$work/verify-args" "$work/args" "$work/body"
sh "$root/lib/android_keep_handoff.grease" rish "$work/body" > "$work/receipt"
grep -Fx "$(printf 'keep-handoff\tstarted-unconfirmed')" "$work/receipt"
"$work/verify-args" "$work/args" "$work/body"
for status in missing failed ambiguous; do
  IB_KEEP_TEST_STATUS=$status; export IB_KEEP_TEST_STATUS
  if sh "$root/lib/android_keep_handoff.grease" direct "$work/body" > "$work/receipt"; then
    echo "incorrect success: $status" >&2; exit 1
  fi
  case "$status" in
    missing) grep -Fx "$(printf 'keep-handoff\tunavailable')" "$work/receipt" ;;
    *) grep -Fx "$(printf 'keep-handoff\toutcome-unknown')" "$work/receipt" ;;
  esac
done
: > "$work/empty"
if sh "$root/lib/android_keep_handoff.grease" direct "$work/empty" > "$work/receipt"; then exit 1; fi
grep -Fx "$(printf 'keep-handoff\tinvalid-payload')" "$work/receipt"
test ! -e /tmp/ib-keep-UNEXPECTED
printf '%s\n' 'keep-handoff-shell=PASS (simulated Android only)'
