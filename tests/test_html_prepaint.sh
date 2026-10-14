#!/bin/sh
set -eu
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
  tests/prepaint_v1_oracle.c -o "$work/prepaint-v1-oracle"
export IB_PREPAINT_ORACLE="$work/prepaint-v1-oracle"
export IB_HTML_PREPAINT_EXEC="${IB_HTML_PREPAINT_EXEC:-$root/src/build/exec/ib-html-prepaint}"
export IB_ALLOW_FIXTURE=1
export IB_EXPECTED_SOURCE_GENERATION=generation-1
test -x "$IB_HTML_PREPAINT_EXEC"

make_manifest() {
  kind=$1 body=$2 url=$3 manifest=$4
  sha=$(sha256sum "$body" | cut -d ' ' -f 1)
  bytes=$(wc -c < "$body" | tr -d '[:space:]')
  {
    printf 'schema\tib-html-admission-v1\n'
    printf 'source_kind\tfixture\n'
    printf 'source_key\tfixture:%s\n' "$kind"
    printf 'source_generation\tgeneration-1\n'
    printf 'requested_url\t%s\n' "$url"
    printf 'resolved_url\t%s\n' "$url"
    printf 'http_status\t200\n'
    printf 'mime_type\ttext/html\n'
    printf 'charset\tutf-8\n'
    printf 'sha256\t%s\n' "$sha"
    printf 'byte_count\t%s\n' "$bytes"
    printf 'acquisition_complete\tYES\n'
    printf 'arena_reopen\tNOT_RUN\n'
  } > "$manifest"
}
run_fixture() {
  kind=$1 url=$2
  body="tests/fixtures/ib-html/$kind.html"
  key="fixture:$kind"
  expected="tests/fixtures/ib-html/$kind.prepaint"
  manifest="$work/$kind.tsv"
  output="$work/$kind.prepaint"
  make_manifest "$kind" "$body" "$url" "$manifest"
  sh bin/ib_emit_html_prepaint.grease "$manifest" "$body" "$key" "$output" \
    > "$work/$kind.receipt"
  "$IB_PREPAINT_ORACLE" "$output"
  cmp "$expected" "$output"
  if test -n "${IB_AICI_PREPAINT_VERIFIER:-}"; then
    test -f "$IB_AICI_PREPAINT_VERIFIER" || exit 1
    {
      printf 'source_key\t%s\n' "$key"
      printf 'source_generation\tgeneration-1\n'
      printf 'source_sha256\t%s\n' "$(sha256sum "$body" | cut -d ' ' -f 1)"
      printf 'source_bytes\t%s\n' "$(wc -c < "$body" | tr -d '[:space:]')"
    } > "$work/$kind.ai-source.tsv"
    sh "$IB_AICI_PREPAINT_VERIFIER" "$output" "$expected" \
      "$work/$kind.ai-source.tsv" "$body" "$key" generation-1
  fi
  if test -n "${IB_HTML_PREPAINT_RECEIPT_ROOT:-}"; then
    mkdir -p "$IB_HTML_PREPAINT_RECEIPT_ROOT"
    cp "$output" "$IB_HTML_PREPAINT_RECEIPT_ROOT/$kind.prepaint"
    cp "$manifest" "$IB_HTML_PREPAINT_RECEIPT_ROOT/$kind.admission.tsv"
    cp "$work/$kind.receipt" "$IB_HTML_PREPAINT_RECEIPT_ROOT/$kind.conversion.tsv"
  fi
  grep -Fx "$(printf 'conversion\tPASS')" "$work/$kind.receipt"
  printf 'fixture\t%s\tPASS\n' "$kind"
}
run_fixture example https://example.com/
run_fixture arxiv https://arxiv.org/html/2401.00100

sourcefile="$work/example.prepaint"
manifest="$work/example.tsv"
body=tests/fixtures/ib-html/example.html
key=fixture:example
cp "$sourcefile" "$work/last_valid"
test_failure_preserves() {
  label=$1
  if sh bin/ib_emit_html_prepaint.grease "$manifest" "$body" "$key" "$sourcefile" \
      > "$work/mutant.out" 2> "$work/mutant.err"; then
    echo "failure accepted: $label" >&2
    exit 1
  fi
  cmp "$sourcefile" "$work/last_valid"
  printf 'refusal\t%s\tPASS\n' "$label"
}
cp "$manifest" "$work/good.tsv"
sed 's@^sha256.*@sha256\t0000000000000000000000000000000000000000000000000000000000000000@' \
  "$work/good.tsv" > "$manifest"
test_failure_preserves wrong_digest
cp "$work/good.tsv" "$manifest"
sed 's@^source_key.*@source_key\tfixture:wrong@' "$work/good.tsv" > "$manifest"
test_failure_preserves changed_source_identity
cp "$work/good.tsv" "$manifest"
IB_EXPECTED_SOURCE_GENERATION=generation-2
export IB_EXPECTED_SOURCE_GENERATION
test_failure_preserves stale_source_generation
IB_EXPECTED_SOURCE_GENERATION=generation-1
export IB_EXPECTED_SOURCE_GENERATION
sed 's@^mime_type.*@mime_type\tapplication/octet-stream@' "$work/good.tsv" > "$manifest"
test_failure_preserves unsupported_mime
cp "$work/good.tsv" "$manifest"
sed 's@^acquisition_complete.*@acquisition_complete\tNO@' "$work/good.tsv" > "$manifest"
test_failure_preserves incomplete_source
cp "$work/good.tsv" "$manifest"
# Same published metadata, mutated body: cannot publish changed bytes.
printf '\377' > "$work/invalid.html"
body="$work/invalid.html"
test_failure_preserves corrupted_source
make_manifest example "$body" https://example.com/ "$manifest"
test_failure_preserves invalid_utf8
body=tests/fixtures/ib-html/example.html
cp "$work/good.tsv" "$manifest"

# Wrong-output mutants must not pass independent expected-content comparison.
mutation_refused() {
  label=$1
  test ! "$1" = ""
  if cmp -s "$sourcefile" "$work/$label.prepaint"; then
    echo "undetected mutation: $label" >&2; exit 1
  fi
  printf 'semantic-mutant\t%s\tREFUSED_BY_GOLDEN\n' "$label"
}
sed '/^text\tThis domain/d' "$sourcefile" > "$work/dropped_text.prepaint"
mutation_refused dropped_text
sed 's@^heading\t1\t@heading\t2\t@' "$sourcefile" > "$work/wrong_heading.prepaint"
mutation_refused wrong_heading
sed 's@www.iana.org/domains/example@www.iana.org/domains/wrong@' "$sourcefile" > "$work/wrong_link.prepaint"
mutation_refused wrong_link
sed 's@^heading\t1\t@heading\t7\t@' "$sourcefile" > "$work/invalid_heading.prepaint"
if "$IB_PREPAINT_ORACLE" "$work/invalid_heading.prepaint"; then
  echo 'independent oracle accepted heading 7' >&2; exit 1
fi
printf 'syntax-mutant\tinvalid_heading\tREFUSED\n'
sed 's@^title\t.*@title\tbad\\q@' "$sourcefile" > "$work/invalid_escaping.prepaint"
if "$IB_PREPAINT_ORACLE" "$work/invalid_escaping.prepaint"; then
  echo 'independent oracle accepted invalid escape' >&2; exit 1
fi
printf 'syntax-mutant\tinvalid_escaping\tREFUSED\n'
printf 'semantic\tPASS\n'
