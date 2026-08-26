#!/bin/sh
set -eu

program=${1:?usage: vector-index-smoke.sh PROGRAM}
temporary=$(mktemp -d)
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
index=$temporary/views/organizing-the-information/vector-spaces/test-model/pages

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
grep -Fx 'text_slot_bytes=16' "$temporary/check.txt"

printf '%s\n' '0.9 0.1 0' |
  "$program" query "$index" 2 > "$temporary/results.txt"
sed -n '1s/	.*//p' "$temporary/results.txt" | grep -Fx 'book-page'
sed -n '2s/	.*//p' "$temporary/results.txt" | grep -Fx 'mixed-page'

printf '%s\n' '0.9 0.1 0' |
  "$program" query-text "$index" 2 > "$temporary/text-results.txt"
cmp "$temporary/results.txt" "$temporary/text-results.txt"

"$program" column "$index" 1 > "$temporary/first-coordinate.txt"
grep -Fx 'book-page	+1.00000000e+00' "$temporary/first-coordinate.txt"
grep -Fx 'tool-page	+0.00000000e+00' "$temporary/first-coordinate.txt"

cp "$index/format.txt" "$temporary/format-before-failed-build.txt"
printf '%s\n' 'broken-page	1 0' |
  if "$program" build "$index" 3 cosine > /dev/null 2> "$temporary/failed-build.txt"; then
    echo 'wrong-dimension build unexpectedly succeeded' >&2
    exit 1
  fi
cmp "$temporary/format-before-failed-build.txt" "$index/format.txt"
printf '%s\n' '0.9 0.1 0' |
  "$program" query "$index" 1 | sed -n '1s/	.*//p' | grep -Fx 'book-page'

vectors_text=$(sed -n 's/^vectors_text //p' "$index/format.txt")
vectors_cache=$(sed -n 's/^vectors_cache //p' "$index/format.txt")
test -n "$vectors_text"
test -n "$vectors_cache"
test "$(wc -c < "$index/$vectors_text" | tr -d ' ')" = 144
test "$(wc -c < "$index/$vectors_cache" | tr -d ' ')" = 36
third_coordinate=$(dd if="$index/$vectors_text" bs=1 skip=32 count=15 2>/dev/null)
test "$third_coordinate" = '+0.00000000e+00'

mv "$index/$vectors_cache" "$temporary/original-cache.f32"
printf '%s\n' '0.9 0.1 0' |
  "$program" query-text "$index" 1 | sed -n '1s/	.*//p' | grep -Fx 'book-page'
if "$program" check "$index" >/dev/null 2>"$temporary/missing-cache.txt"; then
  echo 'check unexpectedly accepted a missing Float32 cache' >&2
  exit 1
fi
"$program" compile-cache "$index" > "$temporary/compile-cache.txt"
grep -Fx 'compile-cache=ok' "$temporary/compile-cache.txt"
cmp "$temporary/original-cache.f32" "$index/$vectors_cache"

cp "$index/$vectors_cache" "$temporary/cache-before-corruption.f32"
printf '\001' | dd of="$index/$vectors_cache" bs=1 seek=0 conv=notrunc 2>/dev/null
if "$program" check "$index" >/dev/null 2> "$temporary/cache-corruption.txt"; then
  echo 'check unexpectedly accepted a cache value that differs from text' >&2
  exit 1
fi
grep -F 'cache does not match' "$temporary/cache-corruption.txt"
"$program" compile-cache "$index" >/dev/null
cmp "$temporary/cache-before-corruption.f32" "$index/$vectors_cache"

cp "$index/$vectors_text" "$temporary/text-before-corruption.txt"
printf 'X' | dd of="$index/$vectors_text" bs=1 seek=15 conv=notrunc 2>/dev/null
if "$program" check "$index" >/dev/null 2> "$temporary/text-corruption.txt"; then
  echo 'check unexpectedly accepted a malformed fixed-width text slot' >&2
  exit 1
fi
grep -F 'invalid fixed-width slot' "$temporary/text-corruption.txt"
cp "$temporary/text-before-corruption.txt" "$index/$vectors_text"
"$program" check "$index" >/dev/null

cp "$index/format.txt" "$temporary/strict-format.txt"
printf '%s\n' 'unknown extra' >> "$index/format.txt"
if "$program" inspect "$index" >/dev/null 2> "$temporary/extra-manifest-row.txt"; then
  echo 'inspect unexpectedly accepted an unknown manifest row' >&2
  exit 1
fi
grep -F 'unsupported format' "$temporary/extra-manifest-row.txt"
cp "$temporary/strict-format.txt" "$index/format.txt"

printf '%s\n' '1 0' |
  if "$program" query "$index" 1 > /dev/null 2> "$temporary/wrong-dimension.txt"; then
    echo 'wrong-dimension query unexpectedly succeeded' >&2
    exit 1
  fi
grep -F 'wrong vector dimension' "$temporary/wrong-dimension.txt"

printf '%s\n' '0.9 0.1 0' '' '1 0 0' |
  if "$program" query "$index" 1 > /dev/null 2> "$temporary/extra-query.txt"; then
    echo 'query unexpectedly accepted a second nonempty vector' >&2
    exit 1
  fi
grep -F 'exactly one vector' "$temporary/extra-query.txt"

printf '%s\n' 'zero	0 0 0' |
  if "$program" build "$temporary/zero-index" 3 cosine > /dev/null 2> "$temporary/zero.txt"; then
    echo 'zero cosine vector unexpectedly succeeded' >&2
    exit 1
  fi
grep -F 'nonzero norm' "$temporary/zero.txt"

"$program" build "$temporary/empty-index" 2 cosine </dev/null \
  > "$temporary/empty-build.txt"
grep -Fx 'count=0' "$temporary/empty-build.txt"
"$program" check "$temporary/empty-index" >/dev/null
printf '%s\n' '1 0' |
  "$program" query-text "$temporary/empty-index" 1 \
    > "$temporary/empty-results.txt"
test ! -s "$temporary/empty-results.txt"
empty_cache=$(sed -n 's/^vectors_cache //p' "$temporary/empty-index/format.txt")
rm -f "$temporary/empty-index/$empty_cache"
"$program" compile-cache "$temporary/empty-index" >/dev/null
test -f "$temporary/empty-index/$empty_cache"
test ! -s "$temporary/empty-index/$empty_cache"

printf '%s\n' 'large	3e38' |
  "$program" build "$temporary/dot-overflow-index" 1 dot >/dev/null
printf '%s\n' '3e38' |
  if "$program" query "$temporary/dot-overflow-index" 1 \
    >/dev/null 2> "$temporary/dot-overflow.txt"; then
    echo 'dot query unexpectedly emitted a non-finite score' >&2
    exit 1
  fi
grep -F 'score overflowed' "$temporary/dot-overflow.txt"

printf '%s\n' 'vector-index-smoke=ok'
