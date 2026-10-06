#!/bin/sh
set -eu

source_root=${IB_IDRIC_SOURCE_ROOT:-src}

test -d "$source_root" || {
  printf 'missing Idriç source root: %s\n' "$source_root" >&2
  exit 2
}

failed=0
source_list=$(mktemp)
trap 'rm -f "$source_list"' EXIT HUP INT TERM
find "$source_root" -type f -name '*.idric' -print | sort >"$source_list"
while IFS= read -r source; do
  awk '
    function inherited(line) {
      return line ~ /(^|[^[:alnum:]_])(Nat|Vect)([^[:alnum:]_]|$)/
    }
    function reject(line) {
      if (inherited(line)) {
        printf "%s:%d: inherited Nat/Vect on public Idriç surface: %s\n", FILENAME, FNR, line > "/dev/stderr"
        bad = 1
      }
    }
    /^[[:space:]]*public[[:space:]]+export([[:space:]]|$)/ {
      waiting = 1
      next
    }
    waiting && (/^[[:space:]]*$/ || /^[[:space:]]*--/) { next }
    waiting {
      reject($0)
      waiting = 0
      if ($0 ~ /^[[:space:]]*(data|record|choice)([[:space:]]|$)/) data_block = 1
      next
    }
    data_block && /^[[:space:]]*$/ { data_block = 0; next }
    data_block { reject($0) }
    END { exit bad }
  ' "$source" || failed=1
done <"$source_list"

test "$failed" -eq 0
printf 'public Idriç vocabulary PASS\n'
