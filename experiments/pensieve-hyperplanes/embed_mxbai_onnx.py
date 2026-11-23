#!/usr/bin/env python3
"""Embed the pinned category-hyperplane input artifact with real mxbai ONNX weights."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path

import numpy as np
import onnxruntime as ort
from huggingface_hub import hf_hub_download
from transformers import AutoTokenizer


MODEL_ID = "mixedbread-ai/mxbai-embed-xsmall-v1"
MODEL_REVISION = "b0561d9a97e6b298da39f0ef3e7d3cf153b1b29a"
MODEL_FILENAME = "onnx/model_quantized.onnx"
EXPECTED_MODEL_SHA256 = "952f996d8cf46c311ee8654a750fa942b71c8b94aabe69d043dbb2bcaff5528e"
TOKENIZER_FILENAME = "tokenizer.json"
EXPECTED_DIMENSION = 384


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load_inputs(path: Path) -> tuple[list[str], list[str]]:
    ids: list[str] = []
    texts: list[str] = []
    with path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames != ["id", "text"]:
            raise ValueError("input artifact must have id and text columns")
        for line_number, row in enumerate(reader, start=2):
            if not row["id"] or not row["text"]:
                raise ValueError(f"input line {line_number}: empty id or text")
            ids.append(row["id"])
            texts.append(row["text"])
    if not ids or len(ids) != len(set(ids)):
        raise ValueError("input ids must be nonempty and unique")
    return ids, texts


def mean_pool(token_vectors: np.ndarray, attention_mask: np.ndarray) -> np.ndarray:
    weights = attention_mask.astype(np.float32)[..., None]
    denominator = weights.sum(axis=1)
    if np.any(denominator == 0):
        raise ValueError("tokenizer produced an empty attention mask")
    return (token_vectors.astype(np.float32) * weights).sum(axis=1) / denominator


def l2_normalize(vectors: np.ndarray) -> np.ndarray:
    vectors = np.asarray(vectors, dtype=np.float32)
    norms = np.linalg.norm(vectors, axis=1, keepdims=True)
    if np.any(~np.isfinite(norms)) or np.any(norms == 0):
        raise ValueError("embedding output contains a non-finite or zero vector")
    return vectors / norms


def embed_batch(
    session: ort.InferenceSession,
    tokenizer,
    texts: list[str],
    max_input_tokens: int,
) -> np.ndarray:
    encoded = tokenizer(
        texts,
        padding=True,
        truncation=True,
        max_length=max_input_tokens,
        return_tensors="np",
    )
    feed: dict[str, np.ndarray] = {}
    for input_metadata in session.get_inputs():
        name = input_metadata.name
        if name in encoded:
            feed[name] = np.asarray(encoded[name], dtype=np.int64)
        elif name == "token_type_ids":
            feed[name] = np.zeros_like(encoded["input_ids"], dtype=np.int64)
        else:
            raise ValueError(f"unsupported ONNX input {name!r}")

    outputs = session.run(None, feed)
    token_output = next(
        (
            np.asarray(value)
            for value in outputs
            if np.asarray(value).ndim == 3
            and np.asarray(value).shape[-1] == EXPECTED_DIMENSION
        ),
        None,
    )
    if token_output is not None:
        return mean_pool(token_output, np.asarray(encoded["attention_mask"]))

    sentence_output = next(
        (
            np.asarray(value)
            for value in outputs
            if np.asarray(value).ndim == 2
            and np.asarray(value).shape[-1] == EXPECTED_DIMENSION
        ),
        None,
    )
    if sentence_output is None:
        shapes = [list(np.asarray(value).shape) for value in outputs]
        raise ValueError(f"no 384-dimensional ONNX embedding output; got {shapes!r}")
    return sentence_output.astype(np.float32)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input-texts", required=True)
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--provenance", required=True)
    parser.add_argument("--max-input-tokens", type=int, default=256)
    parser.add_argument("--batch-size", type=int, default=32)
    args = parser.parse_args()
    if args.max_input_tokens < 1 or args.batch_size < 1:
        raise SystemExit("token and batch sizes must be positive")

    input_path = Path(args.input_texts)
    ids, texts = load_inputs(input_path)

    model_path = Path(
        hf_hub_download(
            repo_id=MODEL_ID,
            filename=MODEL_FILENAME,
            revision=MODEL_REVISION,
        )
    )
    model_checksum = sha256(model_path)
    if model_checksum != EXPECTED_MODEL_SHA256:
        raise ValueError(
            f"model checksum {model_checksum} does not match pinned {EXPECTED_MODEL_SHA256}"
        )

    tokenizer_path = Path(
        hf_hub_download(
            repo_id=MODEL_ID,
            filename=TOKENIZER_FILENAME,
            revision=MODEL_REVISION,
        )
    )
    tokenizer_checksum = sha256(tokenizer_path)
    tokenizer = AutoTokenizer.from_pretrained(MODEL_ID, revision=MODEL_REVISION)
    tokenizer.truncation_side = "right"

    session = ort.InferenceSession(str(model_path), providers=["CPUExecutionProvider"])
    vectors: list[np.ndarray] = []
    for start in range(0, len(texts), args.batch_size):
        batch = texts[start : start + args.batch_size]
        vectors.append(embed_batch(session, tokenizer, batch, args.max_input_tokens))
    matrix = l2_normalize(np.vstack(vectors))
    if matrix.shape != (len(ids), EXPECTED_DIMENSION):
        raise ValueError(f"unexpected embedding matrix shape {matrix.shape!r}")

    vector_path = Path(args.vectors)
    vector_path.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(
        vector_path,
        ids=np.asarray(ids, dtype=np.str_),
        vectors=np.asarray(matrix, dtype=np.float32),
    )

    provenance = {
        "model_id": MODEL_ID,
        "model_revision": MODEL_REVISION,
        "model_filename": MODEL_FILENAME,
        "model_file_sha256": model_checksum,
        "tokenizer_filename": TOKENIZER_FILENAME,
        "tokenizer_revision": MODEL_REVISION,
        "tokenizer_sha256": tokenizer_checksum,
        "backend": "onnxruntime",
        "backend_version": ort.__version__,
        "weight_precision": "int8",
        "pooling": "mean",
        "max_input_tokens": args.max_input_tokens,
        "truncation_side": tokenizer.truncation_side,
        "truncation_strategy": "longest_first",
        "output_dimension": EXPECTED_DIMENSION,
        "output_l2_normalized": True,
        "input_text_sha256": sha256(input_path),
        "rows": len(ids),
    }
    provenance_path = Path(args.provenance)
    provenance_path.parent.mkdir(parents=True, exist_ok=True)
    provenance_path.write_text(
        json.dumps(provenance, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(vector_path)
    print(provenance_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
