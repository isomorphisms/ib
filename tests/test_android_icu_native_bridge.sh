#!/bin/sh
# Host integration of actual ICU #35 native transport through IB's unchanged
# Shizuku framing + canonical Arena producer and verified reader.
# The app_process wrapper, Shizuku authority and HTTP server are TEST ONLY.
# This does not qualify Idriç DEX, real rish, Android or physical C67.
set -eu
export LC_ALL=C
root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
arena_tool=${IB_APPEND_ARENA:-}
slice_tool=${IB_ARENA_SLICE:-}
probe=${IB_ICU_NATIVE_PROBE:-}
native_so=${IB_ICU_NATIVE_SO:-}
for executable in "$arena_tool" "$slice_tool" "$probe"; do
    test -n "$executable" && test -x "$executable" || {
        echo 'compiled native test prerequisites absent' >&2
        exit 2
    }
done
test -n "$native_so" && test -f "$native_so" || exit 2
work=$(mktemp -d)
server_pid=
cleanup() {
    if test -n "$server_pid"; then kill "$server_pid" 2>/dev/null || :; fi
    rm -rf "$work"
}
trap cleanup EXIT HUP INT TERM
mkdir -p "$work/lib" "$work/bin" "$work/fake-bin" "$work/private-shell" "$work/remote-commands"
cp "$root/lib/android_shizuku_icu_provider.grease" "$root/lib/android_binder_icu.grease" "$work/lib/"
cp "$root/bin/ib_android_icu_arena.grease" "$root/bin/ib_android_arena_read.grease" "$work/bin/"
cp "$native_so" "$work/private-shell/libicu_transport.so"
printf '%s\n' 'fixture-not-real-android-dex' > "$work/private-shell/classes.dex"
chmod 700 "$work/private-shell"
export IB_ICU_TEST_SHELL_ROOT="$work/private-shell"
export IB_TEST_REMOTE_COMMANDS="$work/remote-commands/commands.txt"
export IB_ICU_NATIVE_PROBE="$probe"
export IB_C67_ICU_DEX=/data/local/tmp/ib-icu/classes.dex
export IB_C67_ICU_SO=/data/local/tmp/ib-icu/libicu_transport.so
export IB_C67_ICU_DEX_SHA256
IB_C67_ICU_DEX_SHA256=$(sha256sum "$work/private-shell/classes.dex" | cut -d ' ' -f 1)
export IB_C67_ICU_SO_SHA256
IB_C67_ICU_SO_SHA256=$(sha256sum "$work/private-shell/libicu_transport.so" | cut -d ' ' -f 1)

cat > "$work/lib/android_shizuku_host.grease" <<'SHIZUKU'
ib_shizuku_authority() { printf 'shell\n'; }
ib_shizuku_run_bounded() {
    test "$1" = 30 || return 90
    case "$2" in
        *ICU_CAPTURE_BODY_PATH*ICU_CAPTURE_METADATA_PATH*Idric.Generated\ get*) ;;
        *) return 91 ;;
    esac
    printf '%s\n' "$2" >> "$IB_TEST_REMOTE_COMMANDS"
    # Host-only translation of the Android shell-owned private directory.
    # The production provider still requires the pinned Android path atoms.
    translated=$(printf '%s\n' "$2" |
        sed "s|/data/local/tmp/ib-icu|$IB_ICU_TEST_SHELL_ROOT|g")
    sh -c "$translated"
}
SHIZUKU
cat > "$work/fake-bin/app_process" <<'WRAPPER'
#!/bin/sh
# This wrapper invokes ICU #35's *actual compiled native transport*, but is
# deliberately not a claim of running a real Android Idriç app_process DEX.
set -eu
test "$#" = 5
case "$1" in -Dicu.library=*) ;; *) exit 80 ;; esac
test "$2" = /system/bin
test "$3" = Idric.Generated
test "$4" = get
exec "$IB_ICU_NATIVE_PROBE" "$5"
WRAPPER
chmod 700 "$work/fake-bin/app_process"
PATH="$work/fake-bin:$PATH"
export PATH

http_port=18932
tls_port=18933
openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "$work/server.key" -out "$work/server.crt" -days 1 \
    -subj '/CN=localhost' -addext 'subjectAltName=DNS:localhost' \
    > /dev/null 2>&1
export SSL_CERT_FILE="$work/server.crt"
python3 - "$http_port" "$tls_port" "$work/server.crt" "$work/server.key" \
    > "$work/server.log" 2>&1 <<'PY' &
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import ssl
import sys
import threading

plain_port, tls_port = map(int, sys.argv[1:3])
cert_file, key_file = sys.argv[3:5]

class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.0"

    def log_message(self, *args):
        pass

    def do_GET(self):
        if self.path == "/start":
            self.send_response(302)
            self.send_header("Location", "/final")
            self.end_headers()
            return
        if self.path == "/not-found":
            self.send_response(404)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", "0")
            self.end_headers()
            return
        if self.path == "/truncated":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", "50")
            self.end_headers()
            self.wfile.write(b"short")
            return
        if self.path == "/binary":
            body = bytes([0, 255, 10])
            mime = "application/octet-stream"
        elif self.path == "/final":
            body = b"<p>ICU 35 host</p>\n"
            mime = "text/html; charset=utf-8"
        else:
            self.send_response(400)
            self.end_headers()
            return
        self.send_response(200)
        self.send_header("Content-Type", mime)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

plain = ThreadingHTTPServer(("127.0.0.1", plain_port), Handler)
secure = ThreadingHTTPServer(("127.0.0.1", tls_port), Handler)
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(cert_file, key_file)
secure.socket = context.wrap_socket(secure.socket, server_side=True)
threading.Thread(target=plain.serve_forever, daemon=True).start()
threading.Thread(target=secure.serve_forever, daemon=True).start()
threading.Event().wait()
PY
server_pid=$!
ready=NO
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
    if curl --max-time 1 -fsS "http://127.0.0.1:$http_port/final" >/dev/null 2>&1; then
        ready=YES
        break
    fi
    sleep 1
done
test "$ready" = YES || { cat "$work/server.log" >&2; exit 1; }

export IB_APPEND_ARENA="$arena_tool" IB_ARENA_SLICE="$slice_tool"
mkdir -m 700 "$work/private"
"$arena_tool" create "$work/private/responses.arena" 16777216 >/dev/null
run() { sh "$work/bin/ib_android_icu_arena.grease" "$1" "$2" "$work/private" "$3"; }
readback() { sh "$work/bin/ib_android_arena_read.grease" "$1" "$work/private"; }
field() { awk -F '\t' -v name="$2" '$1==name {print $2}' "$work/private/receipts/$1.tsv"; }

# Both HTTPS and HTTP use the actual compiled ICU socket/TLS response parser,
# through IB's remote length-framed capture and verified canonical append.
run secure "https://localhost:$tls_port/start" 17 > "$work/out-secure"
test "$(field secure schema)" = ib-icu-arena-v2
test "$(field secure resolved_url)" = "https://localhost:$tls_port/final"
test "$(field secure redirect_count)" = 1
test "$(field secure http_status)" = 200
test "$(field secure mime_type)" = text/html
test "$(field secure charset)" = utf-8
test "$(field secure navigation_admitted)" = YES
printf '<p>ICU 35 host</p>\n' > "$work/expected.html"
readback secure > "$work/secure.body"
cmp -s "$work/expected.html" "$work/secure.body"

run plain "http://127.0.0.1:$http_port/start" 18 > "$work/out-plain"
test "$(field plain resolved_url)" = "http://127.0.0.1:$http_port/final"
test "$(field plain navigation_admitted)" = YES
readback plain > "$work/plain.body"
cmp -s "$work/expected.html" "$work/plain.body"

run binary "http://127.0.0.1:$http_port/binary" 19 > "$work/out-binary"
test "$(field binary utf8)" = invalid
test "$(field binary navigation_admitted)" = NO
printf '\000\377\n' > "$work/expected-binary"
readback binary > "$work/binary.body"
cmp -s "$work/expected-binary" "$work/binary.body"

# ICU refuses incomplete and 404 acquisitions before any Arena publication.
used=$(cat "$work/private/responses.arena.used")
for mode in not-found truncated; do
    if run "reject_$mode" "https://localhost:$tls_port/$mode" 20 \
        > "$work/reject.stdout" 2> "$work/reject.stderr"; then
        echo "ICU native $mode unexpectedly succeeded" >&2
        exit 1
    fi
    test ! -e "$work/private/receipts/reject_$mode.tsv"
    test "$(cat "$work/private/responses.arena.used")" = "$used"
done
good_digest=$IB_C67_ICU_SO_SHA256
IB_C67_ICU_SO_SHA256=0000000000000000000000000000000000000000000000000000000000000000
export IB_C67_ICU_SO_SHA256
if run wronghash "http://127.0.0.1:$http_port/final" 21 >/dev/null 2>&1; then
    echo 'failed ICU binary pin was accepted' >&2
    exit 1
fi
test ! -e "$work/private/receipts/wronghash.tsv"
test "$(cat "$work/private/responses.arena.used")" = "$used"
IB_C67_ICU_SO_SHA256=$good_digest
export IB_C67_ICU_SO_SHA256
printf '%s\n' \
  'PASS: real compiled pinned ICU #35 native HTTP and HTTPS+redirect captured by IB Shizuku frame into canonical appendfat and verified v2 reader' \
  'PASS: binary fidelity, HTTP 404, truncated length, native hash mismatch, denied publication' \
  'NOT_RUN: actual Android app_process Idric DEX, physical Shizuku/rish, Binder/PFD and C67 UI'
