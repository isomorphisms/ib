# Saved-tab question-and-answer mock

This is the first executable vertical slice of the ChatGPT-like text frontend.
It is a console prompt, not the eventual phone UI, and it answers only from
plain-text documents deliberately present below `~/reading`.

The stages are explicit:

1. `mock-token-i8-v1` tokenizes text into a 32-component, bounded 8-bit-count
   test vector and selects one source line as an extractive answer.
2. `ib-vector-index` stores one exact cosine index in the existing model-view
   tree. Authoritative fixed-width text and its rebuildable Float32 cache are
   two query paths over that index. Each query can record whether its dot
   products used a GPU; the current C backend records `False` and `compute=cpu`.
3. `mock-single-member-weighted-reducer-v1` is a one-member ensemble test
   double. It runs three validations, records an aggregate score, and preserves
   the candidate. This is the replaceable slot for voting, bagging, rank
   aggregation, XGBoost, or another reducer; it does not claim to run XGBoost.
4. `ib_tab_qa_null_render` is an identity post-processing step before the text
   is returned.

The bundled model is intentionally not described as an LLM. It is a tiny,
queryable deterministic mock that exercises token, embedding, retrieval,
model-output, reducer, and renderer boundaries without a model download.
`IB_TAB_QA_MODEL_COMMAND` can name a later adapter implementing `inspect`,
`embed`, and `answer`. `IB_TAB_QA_REDUCER_COMMAND` can replace the one-member
reducer. The two pinned Hugging Face embedding manifests remain available for
real model views; this mock writes `model-adapter.txt`, never a misleading
`embedding-model.txt` manifest. A model name is bound to one exact adapter
record so a replacement cannot silently reuse vectors produced by different
facts.

## Filesystem boundaries

Saved source text stays below `${IB_READING_DIR:-~/reading}`. The rebuildable
index defaults to:

```text
${XDG_DATA_HOME:-~/.local/share}/ib/views/
  organizing-the-information/vector-spaces/mock-token-i8-v1/reading/
```

Successful questions are appended as one directory per exchange below the
separate requested folder:

```text
~/questions and answers about tabs that the user has visited/
  <UTC-time>-<process>/
    question.txt
    model-candidate.txt
    reduced-answer.txt
    answer.txt
    source.tsv
    vector-query.tsv
    reducer.tsv
    indexing.tsv
    pipeline.tsv
```

The exchange preserves the raw model candidate, reduction evidence, selected
source, vector execution report, and final response separately. The command
rejects a Q&A root equal to or nested with the reading root.

## Run the mock

```text
make -C native/vector-index
sh bin/ask_saved_pages.grease --index
sh bin/ask_saved_pages.grease "What do feed-forward networks do to input space?"
```

Running the last command without a question displays `question>` and reads one
line from the console. The index is reused until `--index` is run again; corpus
change detection is deliberately left for the next slice.
