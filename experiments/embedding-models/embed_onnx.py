#!/usr/bin/env python3
"""Produce reproducible IB vector rows from a pinned local ONNX model.

This is an experiment adapter, not a browser implementation layer.  It keeps
model comparison executable while the selected inference path is moved behind
IB's native/Grease boundary.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import re
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import TextIO

# Official ONNX Runtime wheels enable cross-platform telemetry by default.
# The browser experiment neither needs nor permits it.
os.environ.setdefault("ORT_DISABLE_TELEMETRY", "1")

import numpy as np
import onnxruntime as ort
import tokenizers
from tokenizers import Tokenizer


SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")
REVISION_PATTERN = re.compile(r"[0-9a-f]{40}")
SAFE_PATH_PATTERN = re.compile(r"[A-Za-z0-9._-]+(?:/[A-Za-z0-9._-]+)*")
REQUIRED_FIELDS = {
    "slug",
    "repository",
    "revision",
    "license",
    "runtime",
    "weight_precision",
    "dimensions",
    "max_tokens",
    "pooling",
    "normalization",
    "token_policy",
    "padding",
    "query_prefix",
    "document_prefix",
    "tokenizer_file",
    "onnx_file",
    "onnx_output",
}


@dataclass(frozen=True)
class Artifact:
    relative_path: str
    byte_count: int
    sha256: str


@dataclass(frozen=True)
class ModelManifest:
    path: Path
    fields: dict[str, str]
    artifacts: tuple[Artifact, ...]

    def value(self, name: str) -> str:
        return self.fields[name]

    @property
    def dimensions(self) -> int:
        return int(self.value("dimensions"))

    @property
    def max_tokens(self) -> int:
        return int(self.value("max_tokens"))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def safe_relative_path(value: str) -> bool:
    return bool(SAFE_PATH_PATTERN.fullmatch(value)) and all(
        part not in {".", ".."} for part in value.split("/")
    )


def load_manifest(path: Path) -> ModelManifest:
    lines = path.read_text(encoding="utf-8").splitlines()
    if not lines or lines[0] != "ib-embedding-model 1":
        raise ValueError("unsupported embedding-model manifest header")
    fields: dict[str, str] = {}
    artifacts: list[Artifact] = []
    artifact_paths: set[str] = set()
    for line_number, line in enumerate(lines[1:], start=2):
        parts = line.split(" ")
        if parts[0] == "artifact":
            if len(parts) != 4 or not safe_relative_path(parts[1]):
                raise ValueError(f"manifest line {line_number}: invalid artifact row")
            try:
                byte_count = int(parts[2])
            except ValueError as error:
                raise ValueError(
                    f"manifest line {line_number}: invalid artifact byte count"
                ) from error
            if byte_count <= 0 or not SHA256_PATTERN.fullmatch(parts[3]):
                raise ValueError(f"manifest line {line_number}: invalid artifact facts")
            if parts[1] in artifact_paths:
                raise ValueError(f"manifest line {line_number}: duplicate artifact")
            artifact_paths.add(parts[1])
            artifacts.append(Artifact(parts[1], byte_count, parts[3]))
        elif len(parts) == 2 and parts[0] in REQUIRED_FIELDS:
            if parts[0] in fields:
                raise ValueError(f"manifest line {line_number}: duplicate field")
            fields[parts[0]] = parts[1]
        else:
            raise ValueError(f"manifest line {line_number}: unknown or malformed row")
    missing = REQUIRED_FIELDS - fields.keys()
    if missing:
        raise ValueError(f"manifest is missing fields: {', '.join(sorted(missing))}")
    if not REVISION_PATTERN.fullmatch(fields["revision"]):
        raise ValueError("manifest revision must be a full commit SHA")
    if not re.fullmatch(r"[a-z0-9][a-z0-9._-]*", fields["slug"]):
        raise ValueError("manifest slug is unsafe")
    if not re.fullmatch(
        r"[A-Za-z0-9._-]+/[A-Za-z0-9._-]+", fields["repository"]
    ):
        raise ValueError("manifest repository is invalid")
    if not artifacts:
        raise ValueError("manifest has no artifacts")
    if fields["tokenizer_file"] not in artifact_paths:
        raise ValueError("manifest tokenizer_file is not a pinned artifact")
    if fields["onnx_file"] not in artifact_paths:
        raise ValueError("manifest onnx_file is not a pinned artifact")
    if int(fields["dimensions"]) <= 0 or int(fields["max_tokens"]) <= 0:
        raise ValueError("manifest dimensions and max_tokens must be positive")
    if fields["normalization"] != "l2":
        raise ValueError("this adapter requires l2-normalized output")
    if fields["query_prefix"] != "none" or fields["document_prefix"] != "none":
        raise ValueError("this adapter does not implement input prefixes")
    expected_policy = {
        "model2vec-onnx": {
            "pooling": "static-token-mean",
            "token_policy": "exclude-special-and-unknown",
            "padding": "none",
        },
        "sentence-transformer-onnx": {
            "pooling": "attention-mask-mean",
            "token_policy": "include-special",
            "padding": "dynamic-right",
        },
    }
    if fields["runtime"] not in expected_policy:
        raise ValueError(f"unsupported model runtime: {fields['runtime']}")
    for name, expected in expected_policy[fields["runtime"]].items():
        if fields[name] != expected:
            raise ValueError(
                f"manifest {name}={fields[name]!r} is incompatible with "
                f"runtime {fields['runtime']!r}"
            )
    return ModelManifest(path, fields, tuple(artifacts))


def verify_model(manifest: ModelManifest, model_directory: Path) -> None:
    for artifact in manifest.artifacts:
        path = model_directory / artifact.relative_path
        if not path.is_file():
            raise ValueError(f"model artifact is missing: {artifact.relative_path}")
        if path.stat().st_size != artifact.byte_count:
            raise ValueError(f"model artifact has the wrong size: {artifact.relative_path}")
        if sha256(path) != artifact.sha256:
            raise ValueError(f"model artifact has the wrong SHA-256: {artifact.relative_path}")


def read_inputs(path: Path) -> tuple[list[str], list[str]]:
    row_ids: list[str] = []
    texts: list[str] = []
    with path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames != ["id", "text"]:
            raise ValueError("input TSV header must be exactly: id<TAB>text")
        for line_number, row in enumerate(reader, start=2):
            if None in row or not row["id"] or not row["text"]:
                raise ValueError(f"input TSV row {line_number}: empty or extra field")
            if "\t" in row["id"] or "\n" in row["id"] or "\r" in row["id"]:
                raise ValueError(f"input TSV row {line_number}: unsafe id")
            row_ids.append(row["id"])
            texts.append(row["text"])
    if not row_ids:
        raise ValueError("input TSV has no data rows")
    if len(row_ids) != len(set(row_ids)):
        raise ValueError("input TSV ids must be unique")
    return row_ids, texts


def session_for(
    manifest: ModelManifest, model_directory: Path, threads: int
) -> ort.InferenceSession:
    options = ort.SessionOptions()
    options.execution_mode = ort.ExecutionMode.ORT_SEQUENTIAL
    options.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL
    options.intra_op_num_threads = threads
    options.inter_op_num_threads = 1
    options.log_severity_level = 3
    return ort.InferenceSession(
        str(model_directory / manifest.value("onnx_file")),
        sess_options=options,
        providers=["CPUExecutionProvider"],
    )


def encode_model2vec(
    session: ort.InferenceSession,
    tokenizer: Tokenizer,
    texts: list[str],
    max_tokens: int,
    output_name: str,
) -> np.ndarray:
    tokenizer.no_padding()
    tokenizer.no_truncation()
    unknown_id = tokenizer.token_to_id("[UNK]")
    batches: list[list[int]] = []
    for encoding in tokenizer.encode_batch(texts, add_special_tokens=False):
        token_ids = encoding.ids
        if unknown_id is not None:
            token_ids = [token_id for token_id in token_ids if token_id != unknown_id]
        token_ids = token_ids[:max_tokens]
        if not token_ids:
            raise ValueError("Model2Vec input produced no known tokens")
        batches.append(token_ids)
    offsets = np.asarray(
        np.cumsum([0] + [len(token_ids) for token_ids in batches[:-1]]),
        dtype=np.int64,
    )
    input_ids = np.asarray(
        [token_id for token_ids in batches for token_id in token_ids], dtype=np.int64
    )
    return session.run(
        [output_name], {"input_ids": input_ids, "offsets": offsets}
    )[0]


def encode_sentence_transformer(
    session: ort.InferenceSession,
    tokenizer: Tokenizer,
    texts: list[str],
    max_tokens: int,
    output_name: str,
) -> np.ndarray:
    tokenizer.no_padding()
    tokenizer.no_truncation()
    tokenizer.enable_truncation(max_length=max_tokens)
    pad_id = tokenizer.token_to_id("[PAD]")
    if pad_id is None:
        raise ValueError("sentence-transformer tokenizer has no [PAD] token")
    tokenizer.enable_padding(direction="right", pad_id=pad_id, pad_token="[PAD]")
    encodings = tokenizer.encode_batch(texts, add_special_tokens=True)
    input_ids = np.asarray([encoding.ids for encoding in encodings], dtype=np.int64)
    attention_mask = np.asarray(
        [encoding.attention_mask for encoding in encodings], dtype=np.int64
    )
    return session.run(
        [output_name],
        {"input_ids": input_ids, "attention_mask": attention_mask},
    )[0]


def normalize_rows(vectors: np.ndarray, dimensions: int) -> np.ndarray:
    vectors = np.asarray(vectors, dtype=np.float32)
    if vectors.ndim != 2 or vectors.shape[1] != dimensions:
        raise ValueError(
            f"model emitted shape {vectors.shape!r}, expected (*, {dimensions})"
        )
    if not np.isfinite(vectors).all():
        raise ValueError("model emitted a non-finite value")
    norms = np.linalg.norm(vectors, axis=1, keepdims=True)
    if np.any(norms == 0):
        raise ValueError("model emitted a zero vector")
    return np.asarray(vectors / norms, dtype=np.float32)


def encode(
    manifest: ModelManifest,
    model_directory: Path,
    texts: list[str],
    batch_size: int,
    threads: int,
) -> np.ndarray:
    tokenizer = Tokenizer.from_file(str(model_directory / manifest.value("tokenizer_file")))
    session = session_for(manifest, model_directory, threads)
    outputs: list[np.ndarray] = []
    for start in range(0, len(texts), batch_size):
        batch = texts[start : start + batch_size]
        if manifest.value("runtime") == "model2vec-onnx":
            vectors = encode_model2vec(
                session,
                tokenizer,
                batch,
                manifest.max_tokens,
                manifest.value("onnx_output"),
            )
        elif manifest.value("runtime") == "sentence-transformer-onnx":
            vectors = encode_sentence_transformer(
                session,
                tokenizer,
                batch,
                manifest.max_tokens,
                manifest.value("onnx_output"),
            )
        else:
            raise ValueError(f"unsupported model runtime: {manifest.value('runtime')}")
        outputs.append(normalize_rows(vectors, manifest.dimensions))
    return np.concatenate(outputs, axis=0)


def write_rows(stream: TextIO, row_ids: list[str], vectors: np.ndarray) -> None:
    for row_id, vector in zip(row_ids, vectors, strict=True):
        values = " ".join(f"{float(value):+.9e}" for value in vector)
        stream.write(f"{row_id}\t{values}\n")


def atomic_text_output(path: Path, row_ids: list[str], vectors: np.ndarray) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{path.name}.", suffix=".tmp", dir=path.parent
    )
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="") as stream:
            write_rows(stream, row_ids, vectors)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary_name, path)
    except BaseException:
        try:
            os.unlink(temporary_name)
        except FileNotFoundError:
            pass
        raise


def provenance(
    manifest: ModelManifest,
    input_path: Path,
    row_count: int,
    batch_size: int,
    threads: int,
) -> dict[str, object]:
    return {
        "format": "ib-embedding-run 1",
        "model": {
            "slug": manifest.value("slug"),
            "repository": manifest.value("repository"),
            "revision": manifest.value("revision"),
            "runtime": manifest.value("runtime"),
            "weight_precision": manifest.value("weight_precision"),
            "dimensions": manifest.dimensions,
            "max_tokens": manifest.max_tokens,
            "pooling": manifest.value("pooling"),
            "normalization": manifest.value("normalization"),
            "token_policy": manifest.value("token_policy"),
            "padding": manifest.value("padding"),
            "query_prefix": manifest.value("query_prefix"),
            "document_prefix": manifest.value("document_prefix"),
        },
        "manifest_sha256": sha256(manifest.path),
        "input_sha256": sha256(input_path),
        "row_count": row_count,
        "adapter": {"batch_size": batch_size, "threads": threads},
        "artifacts": [
            {
                "path": artifact.relative_path,
                "bytes": artifact.byte_count,
                "sha256": artifact.sha256,
            }
            for artifact in manifest.artifacts
        ],
        "software": {
            "numpy": np.__version__,
            "onnxruntime": ort.__version__,
            "tokenizers": tokenizers.__version__,
        },
    }


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--model-manifest", type=Path, required=True)
    parser.add_argument("--model-directory", type=Path, required=True)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", default="-", help="vector rows TSV, or - for stdout")
    parser.add_argument("--npz", type=Path)
    parser.add_argument("--provenance", type=Path)
    parser.add_argument("--batch-size", type=int, default=64)
    parser.add_argument("--threads", type=int, default=1)
    return parser.parse_args()


def main() -> None:
    arguments = parse_arguments()
    if arguments.batch_size <= 0 or arguments.threads <= 0:
        raise ValueError("batch size and thread count must be positive")
    manifest = load_manifest(arguments.model_manifest)
    verify_model(manifest, arguments.model_directory)
    row_ids, texts = read_inputs(arguments.input)
    vectors = encode(
        manifest,
        arguments.model_directory,
        texts,
        arguments.batch_size,
        arguments.threads,
    )
    if arguments.output == "-":
        import sys

        write_rows(sys.stdout, row_ids, vectors)
    else:
        atomic_text_output(Path(arguments.output), row_ids, vectors)
    if arguments.npz:
        arguments.npz.parent.mkdir(parents=True, exist_ok=True)
        np.savez(
            arguments.npz,
            ids=np.asarray(row_ids, dtype=str),
            vectors=vectors,
        )
    if arguments.provenance:
        arguments.provenance.parent.mkdir(parents=True, exist_ok=True)
        arguments.provenance.write_text(
            json.dumps(
                provenance(
                    manifest,
                    arguments.input,
                    len(row_ids),
                    arguments.batch_size,
                    arguments.threads,
                ),
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )


if __name__ == "__main__":
    main()
