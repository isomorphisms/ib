#!/usr/bin/env bash
set -euo pipefail

project_dir=$(cd "$(dirname "$0")" && pwd)
app_dir="$project_dir/app"
dependency_dir="$app_dir/build/native-deps/armeabi-v7a"
download_dir="$dependency_dir/downloads"
source_dir="$dependency_dir/sources"
build_dir="$dependency_dir/build"
install_dir="$dependency_dir/install"
asset_dir="$app_dir/build/generated/assets"
license_asset_dir="$asset_dir/licenses"

curl_version=8.21.0
curl_sha256=aa1b66a70eace83dc624508745646c08ae561de512ab403adffb93ac87fc72e6
mbedtls_version=4.2.0
mbedtls_sha256=2bed9d713b4668f76553b097e72b8aa30bc8f112a940d7ae228d524bbde6ffea
ca_revision=2026-08-13
ca_sha256=f66dff1bdf8f96060b8177976f8b7d9254bc89bc4db933d769f7384d28480bc9

if [[ -z "${ANDROID_NDK_HOME:-}" || ! -d "$ANDROID_NDK_HOME" ]]; then
    echo "ANDROID_NDK_HOME must name an installed Android NDK." >&2
    exit 2
fi

mkdir -p \
    "$download_dir" \
    "$source_dir" \
    "$build_dir" \
    "$install_dir" \
    "$asset_dir" \
    "$license_asset_dir"

fetch_verified() {
    local url=$1
    local expected=$2
    local destination=$3
    if [[ -f "$destination" ]] &&
       [[ $(sha256sum "$destination" | cut -d ' ' -f 1) == "$expected" ]]; then
        return
    fi
    curl --fail --location --retry 3 --output "$destination.part" "$url"
    echo "$expected  $destination.part" | sha256sum --check --status
    mv "$destination.part" "$destination"
}

curl_archive="$download_dir/curl-$curl_version.tar.xz"
mbedtls_archive="$download_dir/mbedtls-$mbedtls_version.tar.bz2"
ca_bundle="$asset_dir/cacert.pem"

fetch_verified \
    "https://curl.se/download/curl-$curl_version.tar.xz" \
    "$curl_sha256" \
    "$curl_archive"
fetch_verified \
    "https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-$mbedtls_version/mbedtls-$mbedtls_version.tar.bz2" \
    "$mbedtls_sha256" \
    "$mbedtls_archive"
fetch_verified \
    "https://curl.se/ca/cacert-$ca_revision.pem" \
    "$ca_sha256" \
    "$ca_bundle"

curl_source="$source_dir/curl-$curl_version"
mbedtls_source="$source_dir/mbedtls-$mbedtls_version"
if [[ ! -f "$curl_source/configure" ]]; then
    tar --no-same-owner -xf "$curl_archive" -C "$source_dir"
fi
if [[ ! -f "$mbedtls_source/CMakeLists.txt" ]]; then
    tar --no-same-owner -xf "$mbedtls_archive" -C "$source_dir"
fi
cp "$curl_source/COPYING" "$license_asset_dir/curl-COPYING.txt"
cp "$mbedtls_source/LICENSE" "$license_asset_dir/mbedtls-LICENSE.txt"

mbedtls_prefix="$install_dir/mbedtls"
if [[ ! -f "$mbedtls_prefix/lib/libmbedtls.a" ]]; then
    cmake \
        -S "$mbedtls_source" \
        -B "$build_dir/mbedtls" \
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI=armeabi-v7a \
        -DANDROID_PLATFORM=android-26 \
        -DCMAKE_BUILD_TYPE=MinSizeRel \
        -DCMAKE_INSTALL_PREFIX="$mbedtls_prefix" \
        -DENABLE_PROGRAMS=OFF \
        -DENABLE_TESTING=OFF \
        -DUSE_SHARED_MBEDTLS_LIBRARY=OFF \
        -DUSE_STATIC_MBEDTLS_LIBRARY=ON
    cmake --build "$build_dir/mbedtls" --parallel 2
    cmake --install "$build_dir/mbedtls"
fi
test -f "$mbedtls_prefix/lib/libtfpsacrypto.a"
ln -sfn libtfpsacrypto.a "$mbedtls_prefix/lib/libmbedcrypto.a"

toolchain="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin"
curl_prefix="$install_dir/curl"
if [[ ! -f "$curl_prefix/lib/libcurl.a" ]]; then
    mkdir -p "$build_dir/curl"
    pushd "$build_dir/curl" >/dev/null
    CC="$toolchain/armv7a-linux-androideabi26-clang" \
    AR="$toolchain/llvm-ar" \
    RANLIB="$toolchain/llvm-ranlib" \
    STRIP="$toolchain/llvm-strip" \
    CFLAGS="-Os -fPIC -ffunction-sections -fdata-sections" \
    CPPFLAGS="-I$mbedtls_prefix/include" \
    LDFLAGS="-L$mbedtls_prefix/lib" \
    LIBS="-lmbedtls -lmbedx509 -lmbedcrypto -latomic" \
    "$curl_source/configure" \
        --host=arm-linux-androideabi \
        --prefix="$curl_prefix" \
        --disable-shared \
        --enable-static \
        --with-mbedtls="$mbedtls_prefix" \
        --without-ca-bundle \
        --without-ca-path \
        --without-libpsl \
        --without-zlib \
        --without-brotli \
        --without-zstd \
        --without-libidn2 \
        --without-nghttp2 \
        --without-ngtcp2 \
        --without-nghttp3 \
        --without-quiche \
        --without-libssh2 \
        --disable-docs \
        --disable-manual \
        --disable-ftp \
        --disable-file \
        --disable-ldap \
        --disable-ldaps \
        --disable-rtsp \
        --disable-dict \
        --disable-telnet \
        --disable-tftp \
        --disable-pop3 \
        --disable-imap \
        --disable-smtp \
        --disable-gopher \
        --disable-mqtt \
        --disable-smb
    make -j2
    make install
    popd >/dev/null
fi

test -f "$curl_prefix/lib/libcurl.a"
test -f "$mbedtls_prefix/lib/libmbedtls.a"
test -f "$mbedtls_prefix/lib/libmbedcrypto.a"
test -s "$ca_bundle"
echo "Built pinned ARMv7 curl, mbedTLS, and CA inputs."
