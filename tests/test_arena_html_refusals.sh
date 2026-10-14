#!/bin/sh
# A v1 body-only PR #152 receipt must never become an admitted HTML page.
set -eu
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir -p "$work/arena/receipts"
manifest="$work/arena/receipts/request-1.tsv"
cat > "$manifest" <<'EOF'
schema	ib-icu-arena-v1
request_id	request-1
requested_url	https://example.com/
resolved_url	UNOBSERVED
http_status	UNOBSERVED
mime_type	UNOBSERVED
transport	icu-app-process-via-binder-shizuku
arena_name	responses.arena
arena_offset	0
arena_end	2
byte_count	2
sha256	aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
reopen_check	MATCH
navigation_admitted	NO
EOF
page="$work/display.prepaint"
cp "$root/tests/fixtures/ib-html/example.prepaint" "$page"
cp "$page" "$work/last-valid"
refuse() {
  reason=$1
  if sh "$root/bin/ib_arena_to_html_prepaint.grease" \
      request-1 "$work/arena" generation-1 "$page" \
      > "$work/result" 2> "$work/error"; then
    echo "unexpected admitted source: $reason" >&2
    exit 1
  fi
  grep -F "$reason" "$work/error"
  cmp -s "$page" "$work/last-valid"
}
refuse 'navigation not admitted'
sed 's/navigation_admitted.*$/navigation_admitted	YES/' "$manifest" > "$work/replaced"
mv "$work/replaced" "$manifest"
refuse 'out-of-order or missing generation'
sed '/^mime_type/d' "$manifest" > "$work/replaced"
mv "$work/replaced" "$manifest"
refuse 'unadmitted content type'
printf 'arena-v1-unobserved-refused\tPASS\n'
