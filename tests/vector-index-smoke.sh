#!/bin/sh
set -eu

program=${1:?usage: vector-index-smoke.sh PROGRAM}
temporary=$(mktemp -d)
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
index=$temporary/state/indexes/vectors/pages/test-model

printf '%s\n' \
  'book-page	1 0 0' \
  'tool-page	0 1 0' \
  'mixed-page	1 1 0' |
  "$program" build "$index" 3 cosine > "$temporary/build.txt"

grep -Fx 'build=ok' "$temporary/build.txt"
grep -Fx 'backend=flat-f32-exact' "$temporary/build.txt"
grep -Fx 'scalar=f32' "$temporary/build.txt"
grep -Fx 'metric=cosine' "$temporary/build.txt"
grep -Fx 'dimensions=3' "$temporary/build.txt"
grep -Fx 'count=3' "$temporary/build.txt"

"$program" check "$index" > "$temporary/check.txt"
grep -Fx 'check=ok' "$temporary/check.txt"

printf '%s\n' '0.9 0.1 0' |
  "$program" query "$index" 2 > "$temporary/results.txt"
sed -n '1s/	.*//p' "$temporary/results.txt" | grep -Fx 'book-page'
sed -n '2s/	.*//p' "$temporary/results.txt" | grep -Fx 'mixed-page'

cp "$index/format.txt" "$temporary/format-before-failed-build.txt"
printf '%s\n' 'broken-page	1 0' |
  if "$program" build "$index" 3 cosine > /dev/null 2> "$temporary/failed-build.txt"; then
    echo 'wrong-dimension build unexpectedly succeeded' >&2
    exit 1
  fi
cmp "$temporary/format-before-failed-build.txt" "$index/format.txt"
printf '%s\n' '0.9 0.1 0' |
  "$program" query "$index" 1 | sed -n '1s/	.*//p' | grep -Fx 'book-page'

vectors=$(sed -n 's/^vectors //p' "$index/format.txt")
test -n "$vectors"
test "$(wc -c < "$index/$vectors" | tr -d ' ')" = 36

printf '%s\n' '1 0' |
  if "$program" query "$index" 1 > /dev/null 2> "$temporary/wrong-dimension.txt"; then
    echo 'wrong-dimension query unexpectedly succeeded' >&2
    exit 1
  fi
grep -F 'wrong vector dimension' "$temporary/wrong-dimension.txt"

printf '%s\n' 'zero	0 0 0' |
  if "$program" build "$temporary/zero-index" 3 cosine > /dev/null 2> "$temporary/zero.txt"; then
    echo 'zero cosine vector unexpectedly succeeded' >&2
    exit 1
  fi
grep -F 'nonzero norm' "$temporary/zero.txt"

printf '%s\n' 'vector-index-smoke=ok'
