#!/bin/sh
# Uses the pinned real sd-card-append-fat library and executable, not a
# simulated Arena writer. The binder transport itself is host-simulated.
set -eu
tool=${IB_APPEND_ARENA:-}
helper=${IB_INGRESS_CANONICAL_TEST:-}
test -n "$tool" && test -x "$tool" &&
test -n "$helper" && test -x "$helper" || {
  printf 'real appendfat and compiled ingress test required\n' >&2
  exit 2
}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -m 700 "$work/private"
mkdir -m 700 "$work/private/receipts" "$work/private/locks"
"$tool" create "$work/private/responses.arena" 16777216 > "$work/create.txt"
"$helper" "$work/private"
printf 'producer\tcanonical-sd-card-append-fat\n'
printf 'private_app_root\t0700\n'
printf 'real_icu_and_shizuku\tNOT_RUN\n'
printf 'android_binder_transaction\tNOT_RUN\n'
