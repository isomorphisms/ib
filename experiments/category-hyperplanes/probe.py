#!/usr/bin/env python3
"""Disposable explicit-hyperplane comparison for IB category design."""

from __future__ import annotations

import argparse
import csv
import hashlib
import itertools
import json
import math
import platform
import re
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable

import numpy as np
import sklearn
from sklearn.svm import SVC


ALLOWED_AUTHORITIES = {"human_assertion", "accepted_decision"}
ALLOWED_POLARITIES = {"positive", "negative"}
SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")
REVISION_PATTERN = re.compile(r"[0-9a-f]{40}")


@dataclass(frozen=True)
class Label:
    row_id: str
    category: str
    polarity: str
    authority: str
    asserted_at: str
    assertion_source: str
    source_artifact_sha256: str
    source_row: int


def label_record(label: Label) -> dict:
    result = asdict(label)
    result["id"] = result.pop("row_id")
    return result


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_model_file_checksums(values: list[str]) -> dict[str, str]:
    result: dict[str, str] = {}
    for value in values:
        name, separator, checksum = value.rpartition("=")
        if not separator or not name or not SHA256_PATTERN.fullmatch(checksum):
            raise ValueError("--model-file-sha256 must be NAME=64-lowercase-hex-digits")
        if name in result:
            raise ValueError(f"duplicate model-file checksum name {name!r}")
        result[name] = checksum
    return dict(sorted(result.items()))


def normalize_rows(vectors: np.ndarray) -> np.ndarray:
    vectors = np.asarray(vectors, dtype=np.float32)
    if vectors.ndim != 2 or vectors.shape[0] == 0 or vectors.shape[1] == 0:
        raise ValueError("vectors must be a nonempty two-dimensional matrix")
    if not np.isfinite(vectors).all():
        raise ValueError("vectors contain a non-finite value")
    norms = np.linalg.norm(vectors, axis=1, keepdims=True)
    if np.any(norms == 0):
        raise ValueError("vectors contain a zero row")
    return vectors / norms


def load_vectors(path: Path) -> tuple[list[str], np.ndarray]:
    with np.load(path, allow_pickle=False) as archive:
        if set(archive.files) != {"ids", "vectors"}:
            raise ValueError("vector archive must contain exactly ids and vectors")
        ids = [str(value) for value in archive["ids"].tolist()]
        vectors = normalize_rows(archive["vectors"])
    if len(ids) != vectors.shape[0] or len(ids) != len(set(ids)):
        raise ValueError("ids must be unique and align one-to-one with vectors")
    return ids, vectors


def validate_input_texts(path: Path, expected_ids: list[str]) -> None:
    actual_ids: list[str] = []
    with path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        expected_header = ["id", "text"]
        if reader.fieldnames != expected_header:
            raise ValueError(f"input-text header must be {expected_header!r}")
        for line_number, row in enumerate(reader, start=2):
            if not row["id"] or not row["text"]:
                raise ValueError(f"input-text line {line_number}: empty id or text")
            actual_ids.append(row["id"])
    if actual_ids != expected_ids:
        raise ValueError("input-text ids must exactly match vector ids and row order")


def load_labels(path: Path, known_ids: set[str]) -> list[Label]:
    labels: list[Label] = []
    seen: set[tuple[str, str]] = set()
    with path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        expected = [
            "id",
            "category",
            "polarity",
            "authority",
            "asserted_at",
            "assertion_source",
            "source_artifact_sha256",
            "source_row",
        ]
        if reader.fieldnames != expected:
            raise ValueError(f"labels header must be {expected!r}")
        for line_number, row in enumerate(reader, start=2):
            try:
                source_row = int(row["source_row"])
            except ValueError as error:
                raise ValueError(f"labels line {line_number}: invalid source row") from error
            label = Label(
                row["id"],
                row["category"],
                row["polarity"],
                row["authority"],
                row["asserted_at"],
                row["assertion_source"],
                row["source_artifact_sha256"],
                source_row,
            )
            if not label.row_id or not label.category:
                raise ValueError(f"labels line {line_number}: empty id or category")
            if label.row_id not in known_ids:
                raise ValueError(f"labels line {line_number}: unknown id {label.row_id!r}")
            if label.polarity not in ALLOWED_POLARITIES:
                raise ValueError(f"labels line {line_number}: invalid polarity")
            if label.authority not in ALLOWED_AUTHORITIES:
                raise ValueError(f"labels line {line_number}: invalid authority")
            if not label.asserted_at or not label.assertion_source:
                raise ValueError(f"labels line {line_number}: missing assertion provenance")
            if not SHA256_PATTERN.fullmatch(label.source_artifact_sha256):
                raise ValueError(f"labels line {line_number}: invalid source artifact checksum")
            if label.source_row < 1:
                raise ValueError(f"labels line {line_number}: invalid source row")
            key = (label.row_id, label.category)
            if key in seen:
                raise ValueError(f"labels line {line_number}: duplicate category assertion for id")
            seen.add(key)
            labels.append(label)
    if not labels:
        raise ValueError("labels file is empty")
    return labels


def choose_threshold(
    scores: np.ndarray, positive_indices: np.ndarray, target_recall: float
) -> float:
    positive_scores = np.sort(scores[positive_indices])
    allowed_misses = int(math.floor((1.0 - target_recall) * len(positive_scores)))
    allowed_misses = min(max(allowed_misses, 0), len(positive_scores) - 1)
    return float(positive_scores[allowed_misses] - 1e-7)


def fit_category(
    category: str,
    ids: list[str],
    vectors: np.ndarray,
    labels: list[Label],
    evaluation_labels: list[Label],
    global_evaluation_indices: set[int],
    bags: int,
    unlabeled_per_positive: float,
    c_value: float,
    positive_weight: float,
    target_positive_recall: float,
    seed: int,
) -> dict:
    index = {row_id: offset for offset, row_id in enumerate(ids)}
    positives = np.array(
        sorted(
            index[label.row_id]
            for label in labels
            if label.category == category and label.polarity == "positive"
        ),
        dtype=np.int64,
    )
    negatives = np.array(
        sorted(
            index[label.row_id]
            for label in labels
            if label.category == category and label.polarity == "negative"
        ),
        dtype=np.int64,
    )
    if len(positives) < 2:
        raise ValueError(f"category {category!r} needs at least two authoritative positives")

    evaluation_positives = np.array(
        sorted(
            index[label.row_id]
            for label in evaluation_labels
            if label.category == category and label.polarity == "positive"
        ),
        dtype=np.int64,
    )
    evaluation_negatives = np.array(
        sorted(
            index[label.row_id]
            for label in evaluation_labels
            if label.category == category and label.polarity == "negative"
        ),
        dtype=np.int64,
    )
    positive_ids = set(positives.tolist())
    negative_ids = set(negatives.tolist())
    evaluation_positive_ids = set(evaluation_positives.tolist())
    evaluation_negative_ids = set(evaluation_negatives.tolist())
    fit_ids = positive_ids | negative_ids
    held_out_ids = evaluation_positive_ids | evaluation_negative_ids
    if fit_ids & held_out_ids:
        raise ValueError(f"category {category!r} has an id in both fit and evaluation labels")

    fixed = fit_ids | global_evaluation_indices
    unlabeled = np.array(
        [offset for offset in range(len(ids)) if offset not in fixed], dtype=np.int64
    )
    if len(negatives) == 0 and len(unlabeled) == 0:
        raise ValueError(f"category {category!r} has no negative or unlabeled comparison rows")

    requested_count = int(math.ceil(len(positives) * unlabeled_per_positive))
    sample_count = min(len(unlabeled), requested_count)
    if len(negatives) == 0 and sample_count == 0:
        raise ValueError(
            f"category {category!r} needs an explicit negative or a positive "
            "unlabeled sampling ratio"
        )

    rng = np.random.default_rng(seed)
    if sample_count == 0:
        provisional_samples = [tuple()]
    elif sample_count == len(unlabeled):
        provisional_samples = [tuple(unlabeled.tolist())]
    else:
        possible_samples = math.comb(len(unlabeled), sample_count)
        retained_count = min(bags, possible_samples)
        if possible_samples <= bags:
            provisional_samples = list(itertools.combinations(unlabeled.tolist(), sample_count))
            rng.shuffle(provisional_samples)
        else:
            unique_samples: set[tuple[int, ...]] = set()
            while len(unique_samples) < retained_count:
                sample = rng.choice(unlabeled, size=sample_count, replace=False)
                unique_samples.add(tuple(sorted(sample)))
            provisional_samples = sorted(unique_samples)

    normalized_scores: list[np.ndarray] = []
    planes: list[dict] = []

    for bag_number, provisional_values in enumerate(provisional_samples):
        provisional = np.asarray(provisional_values, dtype=np.int64)
        bag_negatives = np.unique(np.concatenate([negatives, provisional]))
        train_indices = np.concatenate([positives, bag_negatives])
        y = np.concatenate([np.ones(len(positives)), -np.ones(len(bag_negatives))])

        classifier = SVC(
            C=c_value,
            kernel="linear",
            class_weight={1.0: positive_weight, -1.0: 1.0},
        )
        classifier.fit(vectors[train_indices], y)
        w = np.asarray(classifier.coef_[0], dtype=np.float64)
        b = float(classifier.intercept_[0])
        w_norm = float(np.linalg.norm(w))
        if not math.isfinite(w_norm) or w_norm == 0:
            raise ValueError(f"category {category!r} bag {bag_number}: degenerate plane")

        raw_all = vectors @ w + b
        normalized_scores.append(raw_all / w_norm)
        raw_train = raw_all[train_indices]
        slack = np.maximum(0.0, 1.0 - y * raw_train)
        support_indices = train_indices[classifier.support_]
        violations = np.flatnonzero(slack > 1e-9)
        largest = violations[np.argsort(slack[violations])[::-1][:10]] if len(violations) else []

        planes.append(
            {
                "bag": bag_number,
                "w": w.tolist(),
                "b": b,
                "w_norm": w_norm,
                "positive_ids": [ids[offset] for offset in positives],
                "explicit_negative_ids": [ids[offset] for offset in negatives],
                "provisional_unlabeled_ids": [ids[offset] for offset in provisional],
                "support_ids": [ids[offset] for offset in support_indices],
                "slack_nonzero_count": int(len(violations)),
                "largest_margin_violations": [
                    {"id": ids[int(train_indices[offset])], "slack": float(slack[offset])}
                    for offset in largest
                ],
            }
        )

    score_matrix = np.vstack(normalized_scores)
    aggregate = score_matrix.mean(axis=0)
    votes = (score_matrix >= 0.0).mean(axis=0)
    threshold = choose_threshold(aggregate, positives, target_positive_recall)
    model_proposed = aggregate >= threshold
    candidate_included = model_proposed.copy()
    asserted_positive_ids = positive_ids | evaluation_positive_ids
    asserted_negative_ids = negative_ids | evaluation_negative_ids
    candidate_included[list(asserted_positive_ids)] = True
    candidate_included[list(asserted_negative_ids)] = False
    positive_overrides = np.zeros(len(ids), dtype=bool)
    negative_overrides = np.zeros(len(ids), dtype=bool)
    positive_overrides[list(asserted_positive_ids)] = ~model_proposed[list(asserted_positive_ids)]
    negative_overrides[list(asserted_negative_ids)] = model_proposed[list(asserted_negative_ids)]

    ranking = np.argsort(aggregate)[::-1]
    nearest = np.argsort(np.abs(aggregate - threshold))[: min(20, len(ids))]

    def recall(indices: np.ndarray) -> float | None:
        return float(model_proposed[indices].mean()) if len(indices) else None

    def false_proposal_rate(indices: np.ndarray) -> float | None:
        return float(model_proposed[indices].mean()) if len(indices) else None

    return {
        "category": category,
        "positive_count": int(len(positives)),
        "explicit_negative_count": int(len(negatives)),
        "fit_eligible_unlabeled_count": int(len(unlabeled)),
        "requested_plane_count": int(bags),
        "retained_unique_plane_count": int(len(planes)),
        "provisional_unlabeled_per_plane": int(sample_count),
        "fit_mode": (
            "bagged linear soft-margin SVM with provisional-unlabeled resampling"
            if len(planes) > 1
            else "single linear soft-margin SVM; no provisional-resample diversity"
        ),
        "evaluation_positive_count": int(len(evaluation_positives)),
        "evaluation_negative_count": int(len(evaluation_negatives)),
        "aggregate_score_meaning": "mean signed geometric distance across retained planes",
        "inclusion_threshold": threshold,
        "target_training_positive_recall": target_positive_recall,
        "in_sample_model_positive_recall_before_override": recall(positives),
        "candidate_asserted_positive_inclusion_recall": float(
            candidate_included[list(asserted_positive_ids)].mean()
        ),
        "candidate_asserted_negative_exclusion_recall": float(
            (~candidate_included[list(asserted_negative_ids)]).mean()
        ) if asserted_negative_ids else None,
        "explicit_negative_false_proposal_rate": false_proposal_rate(negatives),
        "held_out_positive_recall": recall(evaluation_positives),
        "held_out_negative_false_proposal_rate": false_proposal_rate(evaluation_negatives),
        "planes": planes,
        "ranking": [
            {
                "id": ids[offset],
                "score": float(aggregate[offset]),
                "zero_surface_vote_fraction": float(votes[offset]),
                "model_proposed": bool(model_proposed[offset]),
                "candidate_included_after_assertion_overrides": bool(candidate_included[offset]),
                "human_positive_override_applied": bool(positive_overrides[offset]),
                "human_negative_override_applied": bool(negative_overrides[offset]),
                "fit_positive": bool(offset in positive_ids),
                "fit_negative": bool(offset in negative_ids),
                "evaluation_positive": bool(offset in evaluation_positive_ids),
                "evaluation_negative": bool(offset in evaluation_negative_ids),
            }
            for offset in ranking
        ],
        "nearest_model_policy_boundary": [
            {"id": ids[offset], "score": float(aggregate[offset])} for offset in nearest
        ],
    }


def run_probe(args: argparse.Namespace) -> dict:
    vector_path = Path(args.vectors)
    input_text_path = Path(args.input_texts)
    label_path = Path(args.labels)
    ids, vectors = load_vectors(vector_path)
    validate_input_texts(input_text_path, ids)
    labels = load_labels(label_path, set(ids))
    evaluation_path = Path(args.evaluation_labels) if args.evaluation_labels else None
    evaluation_labels = load_labels(evaluation_path, set(ids)) if evaluation_path else []
    fit_row_ids = {label.row_id for label in labels}
    evaluation_row_ids = {label.row_id for label in evaluation_labels}
    overlap = fit_row_ids & evaluation_row_ids
    if overlap:
        raise ValueError(
            "fit and evaluation labels must be row-disjoint; overlapping ids: "
            + ", ".join(sorted(overlap))
        )
    id_to_index = {row_id: offset for offset, row_id in enumerate(ids)}
    global_evaluation_indices = {id_to_index[row_id] for row_id in evaluation_row_ids}
    model_files = parse_model_file_checksums(args.model_file_sha256)
    if vectors.shape[1] != args.truncation_dimension:
        raise ValueError(
            f"vector dimension {vectors.shape[1]} does not match --truncation-dimension "
            f"{args.truncation_dimension}"
        )
    known_dimensions = {
        "mixedbread-ai/mxbai-embed-xsmall-v1": {128, 256, 384},
        "intfloat/e5-small-v2": {384},
        "sentence-transformers/static-retrieval-mrl-en-v1": {128, 256, 512, 1024},
        "Snowflake/snowflake-arctic-embed-xs": {384},
    }
    allowed = known_dimensions.get(args.model_id)
    if allowed is not None and vectors.shape[1] not in allowed:
        raise ValueError(f"dimension {vectors.shape[1]} is not declared for {args.model_id}")
    positive_categories = {label.category for label in labels if label.polarity == "positive"}
    categories = sorted(set(args.category) if args.category else positive_categories)
    unknown_categories = set(categories) - positive_categories
    if unknown_categories:
        raise ValueError(
            "requested categories lack a fitted positive set: "
            + ", ".join(sorted(unknown_categories))
        )
    all_fit_categories = {
        label.category
        for label in labels
        if not args.category or label.category in categories
    }
    missing_positive_categories = all_fit_categories - set(categories)
    if missing_positive_categories:
        raise ValueError(
            "fit labels contain categories without a fitted positive set: "
            + ", ".join(sorted(missing_positive_categories))
        )
    evaluation_categories = {
        label.category for label in evaluation_labels if label.category in categories
    }
    missing_fit_categories = evaluation_categories - set(categories)
    if missing_fit_categories:
        raise ValueError(
            "evaluation labels contain categories without a fitted positive set: "
            + ", ".join(sorted(missing_fit_categories))
        )
    return {
        "format": "ib-category-hyperplane-probe-v1",
        "warning": "in-sample fit is not evidence of held-out reproduction",
        "vector_source": {
            "path": str(vector_path),
            "sha256": sha256(vector_path),
            "rows": len(ids),
            "dimensions": int(vectors.shape[1]),
            "l2_normalized_by_probe": True,
        },
        "embedding_input": {
            "path": str(input_text_path),
            "sha256": sha256(input_text_path),
            "rows": len(ids),
            "encoding": "UTF-8 TSV",
            "input_builder_revision": args.input_builder_revision,
        },
        "labels": {
            "path": str(label_path),
            "sha256": sha256(label_path),
            "assertions": [
                label_record(label) for label in labels if label.category in categories
            ],
        },
        "evaluation_labels": (
            {
                "path": str(evaluation_path),
                "sha256": sha256(evaluation_path),
                "assertions": [
                    label_record(label)
                    for label in evaluation_labels
                    if label.category in categories
                ],
            }
            if evaluation_path
            else None
        ),
        "embedding": {
            "model_id": args.model_id,
            "model_revision": args.model_revision,
            "model_files_sha256": model_files,
            "backend": args.backend,
            "backend_version": args.backend_version,
            "weight_precision": args.weight_precision,
            "tokenizer_revision": args.tokenizer_revision,
            "tokenizer_sha256": args.tokenizer_sha256,
            "pooling": args.pooling,
            "input_prefix": args.input_prefix,
            "input_grammar": args.input_grammar,
            "max_input_tokens": args.max_input_tokens,
            "truncation_side": args.truncation_side,
            "truncation_strategy": args.truncation_strategy,
            "truncation_dimension": args.truncation_dimension,
        },
        "probe_runtime": {
            "python": platform.python_version(),
            "numpy": np.__version__,
            "scikit_learn": sklearn.__version__,
        },
        "fit": {
            "kind": (
                "linear soft-margin SVM planes with category-specific "
                "provisional-unlabeled resampling"
            ),
            "requested_planes_per_category": args.bags,
            "C": args.c_value,
            "positive_weight": args.positive_weight,
            "unlabeled_per_positive": args.unlabeled_per_positive,
            "seed": args.seed,
        },
        "categories": [
            fit_category(
                category,
                ids,
                vectors,
                labels,
                evaluation_labels,
                global_evaluation_indices,
                args.bags,
                args.unlabeled_per_positive,
                args.c_value,
                args.positive_weight,
                args.target_positive_recall,
                args.seed + offset * 1009,
            )
            for offset, category in enumerate(categories)
        ],
    }


def self_test() -> None:
    ids = ["a", "b", "c", "d", "overlap", "denied-a", "denied-b", "outside"]
    vectors = normalize_rows(
        np.array(
            [
                [3.0, 0.2, 1.0],
                [2.8, -0.1, 1.0],
                [0.2, 3.0, 1.0],
                [-0.1, 2.8, 1.0],
                [2.4, 2.4, 1.0],
                [3.0, 0.2, 1.0],
                [0.2, 3.0, 1.0],
                [-2.0, -2.0, 1.0],
            ],
            dtype=np.float32,
        )
    )
    def label(row_id: str, category: str, polarity: str, authority: str) -> Label:
        return Label(
            row_id,
            category,
            polarity,
            authority,
            "2026-08-25T00:00:00Z",
            "self-test",
            "0" * 64,
            1,
        )

    labels = [
        label("a", "A", "positive", "human_assertion"),
        label("b", "A", "positive", "human_assertion"),
        label("outside", "A", "negative", "human_assertion"),
        label("c", "B", "positive", "human_assertion"),
        label("d", "B", "positive", "human_assertion"),
        label("outside", "B", "negative", "human_assertion"),
    ]
    evaluation_labels = [
        label("overlap", "A", "positive", "accepted_decision"),
        label("overlap", "B", "positive", "accepted_decision"),
        label("denied-a", "A", "negative", "accepted_decision"),
        label("denied-b", "B", "negative", "accepted_decision"),
    ]
    global_evaluation_indices = {
        ids.index("overlap"),
        ids.index("denied-a"),
        ids.index("denied-b"),
    }
    results = {
        category: fit_category(
            category,
            ids,
            vectors,
            labels,
            evaluation_labels,
            global_evaluation_indices,
            3,
            0.0,
            1.0,
            2.0,
            1.0,
            7,
        )
        for category in ("A", "B")
    }
    for category, result in results.items():
        assert result["in_sample_model_positive_recall_before_override"] == 1.0
        assert result["held_out_positive_recall"] == 1.0
        assert result["held_out_negative_false_proposal_rate"] == 1.0
        assert result["candidate_asserted_negative_exclusion_recall"] == 1.0
        assert result["fit_eligible_unlabeled_count"] == 2
        assert result["retained_unique_plane_count"] == 1
        assert all(not plane["provisional_unlabeled_ids"] for plane in result["planes"])
        assert any(abs(plane["b"]) > 1e-9 for plane in result["planes"]), category
        by_id = {row["id"]: row for row in result["ranking"]}
        assert by_id["overlap"]["model_proposed"], category
        assert not by_id["overlap"]["human_positive_override_applied"], category
        denied_id = "denied-a" if category == "A" else "denied-b"
        assert by_id[denied_id]["model_proposed"], category
        assert not by_id[denied_id]["candidate_included_after_assertion_overrides"], category
        assert by_id[denied_id]["human_negative_override_applied"], category

    resampled = fit_category(
        "A",
        ids,
        vectors,
        labels,
        evaluation_labels,
        global_evaluation_indices,
        10,
        0.5,
        1.0,
        2.0,
        1.0,
        11,
    )
    assert resampled["requested_plane_count"] == 10
    assert resampled["retained_unique_plane_count"] == 2
    provisional_sets = {
        tuple(plane["provisional_unlabeled_ids"]) for plane in resampled["planes"]
    }
    assert provisional_sets == {("c",), ("d",)}
    print("category hyperplane self-test: ok")


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument("--self-test", action="store_true")
    result.add_argument("--vectors")
    result.add_argument("--input-texts")
    result.add_argument("--labels")
    result.add_argument("--evaluation-labels")
    result.add_argument(
        "--category",
        action="append",
        default=[],
        help="fit only this category; repeat for several independent planes",
    )
    result.add_argument("--output")
    result.add_argument("--model-id")
    result.add_argument("--model-revision")
    result.add_argument("--model-file-sha256", action="append")
    result.add_argument("--backend")
    result.add_argument("--backend-version")
    result.add_argument("--weight-precision")
    result.add_argument("--tokenizer-revision")
    result.add_argument("--tokenizer-sha256")
    result.add_argument("--pooling")
    result.add_argument("--input-prefix")
    result.add_argument("--input-grammar")
    result.add_argument("--input-builder-revision")
    result.add_argument("--max-input-tokens", type=int)
    result.add_argument("--truncation-side", choices=("left", "right"))
    result.add_argument("--truncation-strategy")
    result.add_argument("--truncation-dimension", type=int)
    result.add_argument("--bags", type=int, default=16)
    result.add_argument("--unlabeled-per-positive", type=float, default=2.0)
    result.add_argument("--c-value", type=float, default=1.0)
    result.add_argument("--positive-weight", type=float, default=2.0)
    result.add_argument("--target-positive-recall", type=float, default=1.0)
    result.add_argument("--seed", type=int, default=1729)
    return result


def main(argv: Iterable[str] | None = None) -> int:
    args = parser().parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    required = (
        "vectors",
        "input_texts",
        "labels",
        "output",
        "model_id",
        "model_revision",
        "model_file_sha256",
        "backend",
        "backend_version",
        "weight_precision",
        "tokenizer_revision",
        "tokenizer_sha256",
        "pooling",
        "input_prefix",
        "input_grammar",
        "input_builder_revision",
        "max_input_tokens",
        "truncation_side",
        "truncation_strategy",
        "truncation_dimension",
    )
    missing = [name for name in required if getattr(args, name) is None]
    if missing:
        arguments = ", ".join("--" + name for name in missing)
        raise SystemExit("missing required arguments: " + arguments)
    if (
        args.bags < 1
        or args.unlabeled_per_positive < 0
        or args.c_value <= 0
        or args.positive_weight <= 0
    ):
        raise SystemExit("invalid fit parameter")
    if not 0 < args.target_positive_recall <= 1:
        raise SystemExit("--target-positive-recall must be in (0, 1]")
    if args.truncation_dimension < 1:
        raise SystemExit("--truncation-dimension must be positive")
    if args.max_input_tokens < 1:
        raise SystemExit("--max-input-tokens must be positive")
    if not REVISION_PATTERN.fullmatch(args.model_revision):
        raise SystemExit("--model-revision must be an immutable 40-character lowercase commit id")
    if not REVISION_PATTERN.fullmatch(args.tokenizer_revision):
        raise SystemExit(
            "--tokenizer-revision must be an immutable 40-character lowercase commit id"
        )
    if not REVISION_PATTERN.fullmatch(args.input_builder_revision):
        raise SystemExit(
            "--input-builder-revision must be an immutable 40-character lowercase commit id"
        )
    if not SHA256_PATTERN.fullmatch(args.tokenizer_sha256):
        raise SystemExit("--tokenizer-sha256 must be 64 lowercase hexadecimal characters")
    report = run_probe(args)
    Path(args.output).write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
