#!/bin/sh
# Executable synthetic transport and crash-safe opaque-storage checks.
# Does not claim a live Keep installation or an Idriç native/DEX runtime.
set -eu
root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
  "$root/native/android/ib_keep_share.c" "$root/tests/test_keep_native.c" \
  -o "$work/native-utf8"
"$work/native-utf8"
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
  "$root/native/android/ib_keep_snapshot.c" "$root/tests/test_keep_snapshot.c" \
  -o "$work/snapshot"
"$work/snapshot"
sh "$root/tests/test_keep_handoff.sh"
sh -n "$root/lib/android_keep_handoff.grease"
if test -n "${JAVA_HOME:-}" && test -f "$JAVA_HOME/include/jni.h"; then
  cc -D__ANDROID__ -std=c11 -Wall -Wextra -Werror -pedantic -fsyntax-only \
    -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux" \
    "$root/native/android/ib_keep_share.c"
  printf 'keep-native-jni-syntax=PASS\n'
else
  printf 'keep-native-jni-syntax=NOT_RUN (JNI headers not supplied)\n'
fi
printf 'keep-device=NOT_RUN (TAB_P10 is not connected)\n'
