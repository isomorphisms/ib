#!/bin/sh
# Compiles and executes the real Idriç KeepBridgeCases against maintained
# HumanDecisions, Pensieve and KeepBridge source. Not an Android/DEX test.
set -eu
root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
idric=${IB_IDRIC_COMPILER:-/tmp/idric/bin/idris2}
if test ! -x "$idric"; then
  printf '%s\n' 'KeepBridgeCases: pinned Idriç compiler missing' >&2
  exit 2
fi
(
  cd "$root/src"
  "$idric" KeepBridgeCases.idric -o ib-keep-bridge-cases
  test -x build/exec/ib-keep-bridge-cases
  actual=$(build/exec/ib-keep-bridge-cases)
  test "$actual" = 'keep-bridge-cases=PASS' || {
    printf 'unexpected semantic test result: %s\n' "$actual" >&2
    exit 1
  }
  printf '%s\n' "$actual"
)
