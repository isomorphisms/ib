# Pinned ONNX embedding comparison

This disposable experiment proves that the two checked-in model descriptions can produce normalized vector rows for the filesystem index. Python is not an IB runtime layer; the model, vector-index, and process contracts are intentionally independent of this comparison adapter.

The two deliberately different views are:

| Slug | Mechanism | Weights fetched | Output | Role |
| --- | --- | ---: | ---: | --- |
| `potion-base-2m` | static token lookup and mean | 7.56 MB ONNX | 64 | cheapest always-available semantic view |
| `mxbai-embed-xsmall-v1-int8` | six-layer transformer, INT8 ONNX | 24.45 MB ONNX | 384 | slower retrieval-quality challenger |

Every required file is tied to an immutable Hugging Face commit, byte count, and SHA-256 in `models/embedding/*.model`. Fetching is explicit and model weights are not committed to this repository.

```text
bin/ib_embedding_model.grease fetch \
  models/embedding/potion-base-2m.model build/models/potion-base-2m

python3 -m venv build/embedding-venv
build/embedding-venv/bin/pip install -r experiments/embedding-models/requirements.txt
build/embedding-venv/bin/python experiments/embedding-models/embed_onnx.py \
  --model-manifest models/embedding/potion-base-2m.model \
  --model-directory build/models/potion-base-2m \
  --input tests/fixtures/embedding-corpus.tsv \
  --output build/potion-rows.tsv \
  --npz build/potion-vectors.npz \
  --provenance build/potion-run.json

bin/ib_vector_view.grease build \
  build/views pages models/embedding/potion-base-2m.model cosine \
  native/vector-index/ib-vector-index < build/potion-rows.tsv
```

The NumPy archive is accepted by the category-hyperplane probe. The row TSV is accepted directly by the C99 index. `provenance` records the exact manifest, source text, artifacts, preprocessing policy, and inference-library versions.
