# Category hyperplane probe

This is a disposable comparison outside the IB runtime. It does not make Python a browser implementation layer. Its purpose is to test frozen embedding models and explicit affine category separators before reproducing the selected policy in Idriç, Grease, or a narrow native adapter.

## Question

Given human organization and an embedding matrix, can independent inclusion-oriented planes recover useful accepted memberships without forcing one winning category?

For category `c`, each fitted plane records:

```text
s_c(x) = w_c dot x + b_c
```

The experiment also records a proposal-policy threshold separately from the fitted SVM zero surface. Several categories may propose the same row. Human assertions are never reversed by the probe, and a candidate produced by a model never silently becomes accepted durable membership.

## Candidate embedding models

Start with meaningfully different small encoders rather than treating one model as ground truth:

| Candidate | Output | Initial role |
| --- | ---: | --- |
| [`mixedbread-ai/mxbai-embed-xsmall-v1`](https://huggingface.co/mixedbread-ai/mxbai-embed-xsmall-v1) | 384, with Matryoshka truncation | default; Apache-2.0; official INT8 ONNX is roughly 24 MB |
| [`intfloat/e5-small-v2`](https://huggingface.co/intfloat/e5-small-v2) | 384 | independent linear-probe challenger; MIT; prefix inputs consistently with `query: ` |
| [`sentence-transformers/static-retrieval-mrl-en-v1`](https://huggingface.co/sentence-transformers/static-retrieval-mrl-en-v1) | 1024, truncatable to 512/256/128 | compute-speed floor; Apache-2.0; its roughly 125 MB lookup table is not the smallest memory footprint, and token-vector averaging loses word order |
| [`Snowflake/snowflake-arctic-embed-xs`](https://huggingface.co/Snowflake/snowflake-arctic-embed-xs) | 384 | optional browser-oriented challenger; Apache-2.0; CLS pooling and query prefix policy matter |

The first run should use float normalized output vectors even if model weights are INT8. Binary output-vector quantization can wait until the float decision surfaces are understood.

Pin and record the exact model revision, files and checksums, backend, weight precision, tokenizer, input prefix, pooling, maximum token count, truncation side and strategy, output dimension, input-field grammar, and normalization. A model name alone is not reproducible provenance. A weak-device comparison should normally begin with a fixed 128- or 256-token input budget even when the encoder permits more.

Canonical input text should use stable labeled fields such as decoded host and path, title, and source-backed snippet. Do not silently fetch private pages merely to improve an experiment.

## Input

`probe.py` expects a NumPy archive containing:

- `ids`: a one-dimensional string array of stable resource, tab, or event ids;
- `vectors`: a two-dimensional float array in the same order.

It also requires the exact UTF-8 input-text artifact used to create those vectors. This is a two-column TSV, in the same row order as the archive:

```text
id	text
```

The probe verifies the ids and records the artifact hash plus the immutable input-builder revision. This makes cross-model comparisons use identical text rather than merely similar preprocessing descriptions.

The labels file is UTF-8 TSV with this header:

```text
id	category	polarity	authority
```

`polarity` is `positive` or `negative`. `authority` must be `human_assertion` or `accepted_decision`. A missing row is unlabeled, not negative. The same id may have positive membership in several categories. A separate file with the same grammar may be supplied through `--evaluation-labels`; those rows are excluded from fitting and provisional-negative sampling.

The current repository does not contain the complete accepted labels from the conversational 491-row exercise or the earlier 227-row reading/non-reading split. Do not reconstruct them from domain stereotypes. Exact replication begins only when the ordered rows, stable identities, accepted memberships, and explicit corrections are available together.

## Run

```text
python3 experiments/category-hyperplanes/probe.py \
  --vectors /path/to/embeddings.npz \
  --input-texts /path/to/canonical-inputs.tsv \
  --labels /path/to/accepted-labels.tsv \
  --evaluation-labels /path/to/held-out-labels.tsv \
  --model-id mixedbread-ai/mxbai-embed-xsmall-v1 \
  --model-revision <40-character-commit> \
  --model-file-sha256 onnx/model_quantized.onnx=<sha256> \
  --backend onnxruntime \
  --backend-version <version> \
  --weight-precision int8 \
  --tokenizer-revision <40-character-commit> \
  --tokenizer-sha256 <sha256> \
  --pooling mean \
  --input-prefix '' \
  --input-grammar 'host: <host> path: <path> title: <title> text: <snippet>' \
  --input-builder-revision <40-character-commit> \
  --max-input-tokens 256 \
  --truncation-side right \
  --truncation-strategy longest_first \
  --truncation-dimension 384 \
  --output /tmp/ib-category-probe.json
```

The default requests bagged positive-unlabeled SVMs. Each retained plane has a unique provisional-unlabeled sample and preserves its explicit `w`, `b`, support examples, and margin violations before reduction. If no resampling diversity is possible, the probe fits one plane instead of presenting repeated identical fits as an ensemble. Explicit human negatives are always included in the training set; sampled unlabeled rows are provisional negatives for that plane only and never become assertions. The report preserves the untouched model proposal separately from a candidate/retrieval view after authoritative positive and negative overrides. Neither view mutates accepted durable membership.

Run the network-free in-memory contract check with:

```text
python3 experiments/category-hyperplanes/probe.py --self-test
```

## Evaluation

Do not report training fit as reproduction. Construct the separate evaluation-label file by navigation thread, resource, domain, or time so repeated URLs cannot leak across train and test. Evaluation rows are excluded globally from every category's fitting and provisional-negative pool. A category with assertions but no fitted positive set is rejected rather than silently omitted. The probe reports held-out positive recall and held-out explicit-negative false-proposal rate on the raw model proposal; it then applies those held-out assertions when showing the candidate/retrieval view. Report at least:

- recall and false omissions for held-out human positives;
- false proposals on explicit human negatives;
- recovered overlap, rather than forced primary-label accuracy;
- each plane's support examples and margin violations;
- examples nearest each model-policy proposal boundary;
- disagreement across bags and embedding models;
- the effect of two or three new human assertions without erasing older categories.

Vote fraction, fitted margin, normalized distance, and calibrated probability are different quantities. Preserve them separately.
