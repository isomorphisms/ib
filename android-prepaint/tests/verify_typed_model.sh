#!/bin/sh
set -eu

android_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
classes=$(mktemp -d)
diagnostics=$(mktemp -d)
cleanup() {
  rm -rf "$classes" "$diagnostics"
}
trap cleanup EXIT HUP INT TERM

javac --release 17 -d "$classes" \
  "$android_root/app/src/main/java/org/isomorphisms/ib/prepaint/UrlRecognition.java" \
  "$android_root/app/src/main/java/org/isomorphisms/ib/prepaint/PrepaintDocument.java" \
  "$android_root/tests/TypedPrepaintModelSmoke.java"

java -cp "$classes" org.isomorphisms.ib.prepaint.TypedPrepaintModelSmoke

for refusal in "$android_root"/tests/refusal/*.java; do
  log="$diagnostics/$(basename "$refusal").log"
  if javac --release 17 -cp "$classes" -d "$classes" "$refusal" >"$log" 2>&1; then
    printf 'expected compile-time refusal: %s\n' "$refusal" >&2
    exit 1
  fi
  grep -Eq 'cannot find symbol|incompatible types|has private access' "$log"
done

printf 'typed prepaint model: positive behavior and refusals passed\n'
