#!/usr/bin/env bash
set -euo pipefail

project_dir=$(cd "$(dirname "$0")" && pwd)
app_dir="$project_dir/app"
native_dir="$app_dir/src/main"
object_dir="$app_dir/build/native/armeabi-v7a"
output_dir="$app_dir/build/generated/jniLibs/armeabi-v7a"
dependency_dir="$app_dir/build/native-deps/armeabi-v7a/install"
curl_prefix="$dependency_dir/curl"
mbedtls_prefix="$dependency_dir/mbedtls"

if [[ -z "${ANDROID_NDK_HOME:-}" || ! -d "$ANDROID_NDK_HOME" ]]; then
    echo "ANDROID_NDK_HOME must name an installed Android NDK." >&2
    exit 2
fi

ldc=${LDC2:-ldc2}
if ! command -v "$ldc" >/dev/null 2>&1; then
    echo "LDC2 must name an installed ldc2 compiler." >&2
    exit 2
fi

toolchain="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin"
clang="$toolchain/armv7a-linux-androideabi26-clang"
readelf="$toolchain/llvm-readelf"
strip="$toolchain/llvm-strip"
for tool in "$clang" "$readelf" "$strip"; do
    if [[ ! -x "$tool" ]]; then
        echo "Missing NDK tool: $tool" >&2
        exit 2
    fi
done

mkdir -p "$object_dir" "$output_dir"

bash "$project_dir/build-network-deps-armv7.sh"

"$ldc" \
    -betterC \
    -O2 \
    -release \
    -boundscheck=off \
    -relocation-model=pic \
    -mtriple=armv7a-linux-androideabi \
    -mcpu=cortex-a7 \
    -c "$native_dir/d/ib_native.d" \
    -of="$object_dir/ib_native.o"

"$clang" \
    -std=c17 \
    -O2 \
    -fPIC \
    -Wall \
    -Wextra \
    -Werror \
    -c "$native_dir/c/jni_calls.c" \
    -o "$object_dir/jni_calls.o"

"$clang" \
    -std=c17 \
    -O2 \
    -fPIC \
    -Wall \
    -Wextra \
    -Werror \
    -I"$curl_prefix/include" \
    -c "$native_dir/c/http_calls.c" \
    -o "$object_dir/http_calls.o"

"$clang" \
    -shared \
    -Wl,--no-undefined \
    -Wl,--gc-sections \
    -Wl,--exclude-libs,ALL \
    -Wl,-z,relro,-z,now \
    -Wl,--build-id=sha1 \
    "$object_dir/ib_native.o" \
    "$object_dir/jni_calls.o" \
    "$object_dir/http_calls.o" \
    "$curl_prefix/lib/libcurl.a" \
    "$mbedtls_prefix/lib/libmbedtls.a" \
    "$mbedtls_prefix/lib/libmbedx509.a" \
    "$mbedtls_prefix/lib/libmbedcrypto.a" \
    -latomic \
    -lm \
    -landroid \
    -llog \
    -o "$output_dir/libib.so"

"$strip" --strip-unneeded "$output_dir/libib.so"

header=$("$readelf" -h "$output_dir/libib.so")
dynamic=$("$readelf" -d "$output_dir/libib.so")
symbols=$("$readelf" -Ws "$output_dir/libib.so")

grep -q 'Class:.*ELF32' <<<"$header"
grep -q 'Machine:.*ARM' <<<"$header"
grep -q 'ANativeActivity_onCreate' <<<"$symbols"
grep -q 'JNI_OnLoad' <<<"$symbols"
if grep -Eq 'libphobos|libdruntime|libjvm|libcurl|libmbedtls|libmbedx509|libmbedcrypto' \
        <<<"$dynamic"; then
    echo "Unexpected dynamic language or transport dependency in libib.so." >&2
    exit 1
fi

echo "Built $output_dir/libib.so"
