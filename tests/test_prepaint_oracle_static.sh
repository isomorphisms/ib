#!/bin/sh
set -eu
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
  tests/prepaint_v1_oracle.c -o "$work/oracle"
for case_name in example arxiv; do
  "$work/oracle" "tests/fixtures/ib-html/$case_name.prepaint"
  printf 'independent-golden\t%s\tPASS\n' "$case_name"
done
cp tests/fixtures/ib-html/example.prepaint "$work/sample"
sed 's@^heading\t1\t@heading\t7\t@' "$work/sample" > "$work/level"
if "$work/oracle" "$work/level"; then exit 1; fi
sed 's@^title\t.*@title\tbad\\q@' "$work/sample" > "$work/escape"
if "$work/oracle" "$work/escape"; then exit 1; fi
sed 's@^link\tLearn more\t.*@link\tLearn more\tjavascript:alert(1)@' "$work/sample" > "$work/link"
if "$work/oracle" "$work/link"; then exit 1; fi
# Valid but semantically wrong content MUST fail the independent frozen golden.
sed '/^text\tThis domain/d' "$work/sample" > "$work/drop"
if cmp -s "$work/drop" tests/fixtures/ib-html/example.prepaint; then exit 1; fi
sed 's@^heading\t1\t@heading\t2\t@' "$work/sample" > "$work/wrong-level"
if cmp -s "$work/wrong-level" tests/fixtures/ib-html/example.prepaint; then exit 1; fi
sed 's@www.iana.org/domains/example@www.iana.org/domains/wrong@' "$work/sample" > "$work/wrong-target"
if cmp -s "$work/wrong-target" tests/fixtures/ib-html/example.prepaint; then exit 1; fi
printf 'prepaint-oracle-static\tPASS\n'
