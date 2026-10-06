#!/bin/sh
set -eu

repository_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM

mkdir -p "$work/good" "$work/bad"
printf '%s\n' \
  'module Good' \
  'public export' \
  'count : Number → List Number' \
  'count value = [value]' \
  >"$work/good/Good.idric"

IB_IDRIC_SOURCE_ROOT="$work/good" \
  "$repository_root/tests/idric-public-vocabulary.sh" >/dev/null

printf '%s\n' \
  'module Bad' \
  'public export' \
  'data PublicShape = Shape (Vect 2 Nat)' \
  >"$work/bad/Bad.idric"

if IB_IDRIC_SOURCE_ROOT="$work/bad" \
    "$repository_root/tests/idric-public-vocabulary.sh" \
    >"$work/out" 2>"$work/err"; then
  printf 'public vocabulary self-test accepted Nat/Vect\n' >&2
  exit 1
fi
grep -F 'inherited Nat/Vect on public Idriç surface' "$work/err" >/dev/null

printf 'public Idriç vocabulary self-test PASS\n'
