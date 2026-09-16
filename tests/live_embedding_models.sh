#!/bin/sh
set -eu

repository_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
program=${1:?usage: live_embedding_models.sh VECTOR_PROGRAM PYTHON MODEL_ROOT}
python=${2:?usage: live_embedding_models.sh VECTOR_PROGRAM PYTHON MODEL_ROOT}
model_root=${3:?usage: live_embedding_models.sh VECTOR_PROGRAM PYTHON MODEL_ROOT}
temporary=$(mktemp -d)
trap 'rm -rf "$temporary"' EXIT HUP INT TERM

for manifest in \
  "$repository_root/models/embedding/potion-base-2m.model" \
  "$repository_root/models/embedding/mxbai-embed-xsmall-v1-int8.model"
do
  slug=$(awk '$1 == "slug" { print $2 }' "$manifest")
  dimensions=$(awk '$1 == "dimensions" { print $2 }' "$manifest")
  model_directory=$model_root/$slug
  index_directory=$temporary/views/organizing-the-information/vector-spaces/$slug/pages

  "$repository_root/bin/ib_embedding_model.grease" fetch \
    "$manifest" "$model_directory" > "$temporary/$slug-fetch.txt"
  "$python" "$repository_root/experiments/embedding-models/embed_onnx.py" \
    --model-manifest "$manifest" \
    --model-directory "$model_directory" \
    --input "$repository_root/tests/fixtures/embedding-corpus.tsv" \
    --output "$temporary/$slug-rows.tsv" \
    --npz "$temporary/$slug-vectors.npz" \
    --provenance "$temporary/$slug-run.json"
  "$repository_root/bin/ib_vector_view.grease" build \
    "$temporary/views" pages "$manifest" cosine "$program" \
    < "$temporary/$slug-rows.tsv" > "$temporary/$slug-build.txt"
  cmp "$manifest" \
    "$temporary/views/organizing-the-information/vector-spaces/$slug/embedding-model.txt"
  if test "$slug" = potion-base-2m; then
    sed 's/^license MIT$/license changed-license/' "$manifest" \
      > "$temporary/conflicting-model.model"
    if "$repository_root/bin/ib_vector_view.grease" build \
      "$temporary/views" pages "$temporary/conflicting-model.model" cosine "$program" \
      </dev/null >/dev/null 2> "$temporary/conflicting-model.txt"; then
      echo 'vector view unexpectedly rebound a model slug to different facts' >&2
      exit 1
    fi
    grep -F 'different immutable manifest' "$temporary/conflicting-model.txt" >/dev/null
  fi
  "$program" check "$index_directory" > "$temporary/$slug-check.txt"

  "$python" "$repository_root/experiments/embedding-models/embed_onnx.py" \
    --model-manifest "$manifest" \
    --model-directory "$model_directory" \
    --input "$repository_root/tests/fixtures/embedding-query.tsv" \
    --output "$temporary/$slug-query-row.tsv"
  cut -f2- "$temporary/$slug-query-row.tsv" > "$temporary/$slug-query.txt"
  "$program" query "$index_directory" 1 \
    < "$temporary/$slug-query.txt" > "$temporary/$slug-result.tsv"
  "$program" query-text "$index_directory" 1 \
    < "$temporary/$slug-query.txt" > "$temporary/$slug-text-result.tsv"
  cmp "$temporary/$slug-result.tsv" "$temporary/$slug-text-result.tsv"
  sed -n '1s/\t.*//p' "$temporary/$slug-result.tsv" | grep -Fx symplectic-page
  grep -F "\"dimensions\": $dimensions" "$temporary/$slug-run.json" >/dev/null
  test -s "$temporary/$slug-vectors.npz"
  printf 'embedding-model=%s end-to-end=ok\n' "$slug"
done
