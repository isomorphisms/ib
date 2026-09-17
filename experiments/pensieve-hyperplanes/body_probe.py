#!/usr/bin/env python3
"""Fit explicit positive/unlabeled affine planes over Pensieve body embeddings."""

from __future__ import annotations

import argparse
import csv
import hashlib
import itertools
import json
import math
from dataclasses import dataclass
from pathlib import Path

import numpy as np
import sklearn
from sklearn.svm import SVC


ROLES = {
    "fit_positive",
    "fit_negative",
    "development_positive",
    "development_negative",
    "held_out_positive",
    "held_out_negative",
}
FORMAT = "ib-pensieve-body-hyperplane-probe-v2"
POLICY_FORMAT = "ib-pensieve-hyperplane-proposal-policy-v2"


@dataclass(frozen=True)
class Label:
    row_id: str
    concept: str
    role: str
    provenance: str


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load_vectors(path: Path) -> tuple[list[str], np.ndarray]:
    with np.load(path, allow_pickle=False) as archive:
        if set(archive.files) != {"ids", "vectors"}:
            raise ValueError("vector archive must contain exactly ids and vectors")
        ids = [str(value) for value in archive["ids"].tolist()]
        vectors = np.asarray(archive["vectors"], dtype=np.float32)
    if not ids or len(ids) != len(set(ids)) or vectors.shape[0] != len(ids):
        raise ValueError("vector ids must be nonempty, unique, and aligned")
    norms = np.linalg.norm(vectors, axis=1, keepdims=True)
    if np.any(~np.isfinite(norms)) or np.any(norms == 0):
        raise ValueError("vectors contain non-finite or zero rows")
    return ids, vectors / norms


def validate_inputs(path: Path, ids: list[str]) -> None:
    actual: list[str] = []
    with path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames != ["id", "text"]:
            raise ValueError("input text artifact must have id and text columns")
        for line_number, row in enumerate(reader, start=2):
            if not row["id"] or not row["text"]:
                raise ValueError(f"input line {line_number}: empty id or text")
            actual.append(row["id"])
    if actual != ids:
        raise ValueError("input text ids must exactly match vector ids and order")


def load_labels(path: Path, known_ids: set[str]) -> list[Label]:
    labels: list[Label] = []
    seen: set[tuple[str, str]] = set()
    with path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames != ["id", "concept", "role", "provenance"]:
            raise ValueError("labels must have id, concept, role, provenance columns")
        for line_number, row in enumerate(reader, start=2):
            label = Label(row["id"], row["concept"], row["role"], row["provenance"])
            if label.row_id not in known_ids:
                raise ValueError(f"labels line {line_number}: unknown id {label.row_id!r}")
            if not label.concept or label.role not in ROLES or not label.provenance:
                raise ValueError(f"labels line {line_number}: invalid concept, role, or provenance")
            key = (label.row_id, label.concept)
            if key in seen:
                raise ValueError(f"labels line {line_number}: duplicate id/concept")
            seen.add(key)
            labels.append(label)
    if not labels:
        raise ValueError("labels file is empty")
    return labels


def load_policy(path: Path) -> dict:
    policy = json.loads(path.read_text(encoding="utf-8"))
    if policy.get("format") != POLICY_FORMAT:
        raise ValueError("unexpected proposal policy format")
    fraction = policy.get("score_floor_fraction_of_fit_positive_floor")
    vote = policy.get("minimum_zero_surface_vote_fraction")
    if not isinstance(fraction, (int, float)) or not 0 < fraction <= 1:
        raise ValueError("proposal policy score-floor fraction must be in (0, 1]")
    if not isinstance(vote, (int, float)) or not 0 <= vote <= 1:
        raise ValueError("proposal policy vote fraction must be in [0, 1]")
    if not isinstance(policy.get("concept"), str) or not policy["concept"]:
        raise ValueError("proposal policy must name a concept")
    return policy


def choose_fit_positive_floor(
    scores: np.ndarray, positives: np.ndarray, target_recall: float
) -> float:
    positive_scores = np.sort(scores[positives])
    allowed_misses = int(math.floor((1.0 - target_recall) * len(positive_scores)))
    allowed_misses = min(max(allowed_misses, 0), len(positive_scores) - 1)
    return float(positive_scores[allowed_misses] - 1e-7)


def pairwise_normal_cosines(planes: list[dict]) -> dict | None:
    if len(planes) < 2:
        return None
    normals = np.vstack(
        [np.asarray(plane["normal"], dtype=np.float64) / plane["normal_norm"] for plane in planes]
    )
    cosine = normals @ normals.T
    upper = cosine[np.triu_indices(len(normals), 1)]
    return {
        "minimum": float(upper.min()),
        "median": float(np.median(upper)),
        "maximum": float(upper.max()),
    }


def fit_concept(
    concept: str,
    ids: list[str],
    vectors: np.ndarray,
    labels: list[Label],
    policy: dict,
    bags: int,
    unlabeled_per_positive: float,
    c_value: float,
    positive_weight: float,
    target_recall: float,
    seed: int,
) -> dict:
    if policy["concept"] != concept:
        raise ValueError(
            f"proposal policy concept {policy['concept']!r} does not match {concept!r}"
        )

    index = {row_id: offset for offset, row_id in enumerate(ids)}

    def role_indices(role: str) -> np.ndarray:
        return np.asarray(
            sorted(
                index[label.row_id]
                for label in labels
                if label.concept == concept and label.role == role
            ),
            dtype=np.int64,
        )

    positives = role_indices("fit_positive")
    negatives = role_indices("fit_negative")
    development_positive = role_indices("development_positive")
    development_negative = role_indices("development_negative")
    held_positive = role_indices("held_out_positive")
    held_negative = role_indices("held_out_negative")
    if len(positives) < 2:
        raise ValueError(f"concept {concept!r} needs at least two fit positives")

    fit_ids = set(positives.tolist()) | set(negatives.tolist())
    excluded_ids = {
        index[label.row_id]
        for label in labels
        if label.concept == concept
        and label.role
        in {
            "development_positive",
            "development_negative",
            "held_out_positive",
            "held_out_negative",
        }
    }
    if fit_ids & excluded_ids:
        raise ValueError(f"concept {concept!r} has fit/evaluation overlap")

    fixed = fit_ids | excluded_ids
    unlabeled = np.asarray(
        [offset for offset in range(len(ids)) if offset not in fixed], dtype=np.int64
    )
    requested = int(math.ceil(len(positives) * unlabeled_per_positive))
    sample_count = min(len(unlabeled), requested)
    if len(negatives) == 0 and sample_count == 0:
        raise ValueError(
            f"concept {concept!r} has no explicit negative or provisional unlabeled rows"
        )

    rng = np.random.default_rng(seed)
    if sample_count == 0:
        samples = [tuple()]
    elif sample_count == len(unlabeled):
        samples = [tuple(unlabeled.tolist())]
    else:
        possible = math.comb(len(unlabeled), sample_count)
        retained = min(bags, possible)
        if possible <= bags:
            samples = list(itertools.combinations(unlabeled.tolist(), sample_count))
            rng.shuffle(samples)
        else:
            unique: set[tuple[int, ...]] = set()
            while len(unique) < retained:
                choice = rng.choice(unlabeled, size=sample_count, replace=False)
                unique.add(tuple(sorted(int(value) for value in choice)))
            samples = sorted(unique)

    plane_scores: list[np.ndarray] = []
    planes: list[dict] = []
    for bag, provisional_values in enumerate(samples):
        provisional = np.asarray(provisional_values, dtype=np.int64)
        bag_negatives = np.unique(np.concatenate([negatives, provisional]))
        train = np.concatenate([positives, bag_negatives])
        y = np.concatenate([np.ones(len(positives)), -np.ones(len(bag_negatives))])

        classifier = SVC(
            C=c_value,
            kernel="linear",
            class_weight={1.0: positive_weight, -1.0: 1.0},
        )
        classifier.fit(vectors[train], y)
        normal = np.asarray(classifier.coef_[0], dtype=np.float64)
        offset = float(classifier.intercept_[0])
        normal_norm = float(np.linalg.norm(normal))
        if not math.isfinite(normal_norm) or normal_norm == 0:
            raise ValueError(f"concept {concept!r} bag {bag}: degenerate plane")

        raw_all = vectors @ normal + offset
        plane_scores.append(raw_all / normal_norm)
        raw_train = raw_all[train]
        slack = np.maximum(0.0, 1.0 - y * raw_train)
        violating = np.flatnonzero(slack > 1e-9)
        largest = (
            violating[np.argsort(slack[violating])[::-1][:10]]
            if len(violating)
            else []
        )
        support = train[classifier.support_]

        planes.append(
            {
                "bag": bag,
                "normal": normal.tolist(),
                "offset": offset,
                "normal_norm": normal_norm,
                "fit_positive_ids": [ids[value] for value in positives],
                "explicit_negative_ids": [ids[value] for value in negatives],
                "provisional_unlabeled_ids": [ids[value] for value in provisional],
                "support_ids": [ids[value] for value in support],
                "slack_nonzero_count": int(len(violating)),
                "largest_margin_violations": [
                    {"id": ids[int(train[position])], "slack": float(slack[position])}
                    for position in largest
                ],
            }
        )

    matrix = np.vstack(plane_scores)
    aggregate = matrix.mean(axis=0)
    zero_votes = (matrix >= 0.0).mean(axis=0)
    fit_positive_floor = choose_fit_positive_floor(
        aggregate, positives, target_recall
    )
    score_fraction = float(
        policy["score_floor_fraction_of_fit_positive_floor"]
    )
    minimum_vote = float(policy["minimum_zero_surface_vote_fraction"])
    proposal_threshold = fit_positive_floor * score_fraction
    strict_proposed = aggregate >= fit_positive_floor
    proposed = (aggregate >= proposal_threshold) & (zero_votes >= minimum_vote)
    ranking = np.argsort(aggregate)[::-1]

    def rate(values: np.ndarray, mask: np.ndarray = proposed) -> float | None:
        return float(mask[values].mean()) if len(values) else None

    positive_set = set(positives.tolist())
    negative_set = set(negatives.tolist())
    development_positive_set = set(development_positive.tolist())
    development_negative_set = set(development_negative.tolist())
    held_positive_set = set(held_positive.tolist())
    held_negative_set = set(held_negative.tolist())

    return {
        "concept": concept,
        "fit_positive_count": int(len(positives)),
        "explicit_negative_count": int(len(negatives)),
        "development_positive_count": int(len(development_positive)),
        "development_negative_count": int(len(development_negative)),
        "held_out_positive_count": int(len(held_positive)),
        "held_out_negative_count": int(len(held_negative)),
        "fit_eligible_unlabeled_count": int(len(unlabeled)),
        "requested_plane_count": bags,
        "retained_unique_plane_count": len(planes),
        "provisional_unlabeled_per_plane": sample_count,
        "fit_mode": "bagged linear soft-margin SVM with provisional-unlabeled resampling",
        "aggregate_score_definition": "mean signed geometric distance across retained planes",
        "strict_fit_positive_floor": fit_positive_floor,
        "proposal_policy": {
            "name": policy["name"],
            "score_floor_fraction_of_fit_positive_floor": score_fraction,
            "minimum_zero_surface_vote_fraction": minimum_vote,
            "proposal_threshold": proposal_threshold,
            "rule": policy["proposal_rule"],
        },
        "unlabeled_policy": "unasserted rows may be sampled provisionally as comparison rows; sampling does not create a negative assertion",
        "target_fit_positive_recall": target_recall,
        "fit_positive_recall": rate(positives),
        "development_positive_recall": rate(development_positive),
        "development_negative_false_proposal_rate": rate(development_negative),
        "held_out_positive_recall": rate(held_positive),
        "explicit_negative_false_proposal_rate": rate(negatives),
        "held_out_negative_false_proposal_rate": rate(held_negative),
        "strict_held_out_positive_recall": rate(held_positive, strict_proposed),
        "plane_normal_cosine": pairwise_normal_cosines(planes),
        "planes": planes,
        "ranking": [
            {
                "id": ids[value],
                "score": float(aggregate[value]),
                "zero_surface_vote_fraction": float(zero_votes[value]),
                "strict_proposed": bool(strict_proposed[value]),
                "proposed": bool(proposed[value]),
                "fit_positive": bool(value in positive_set),
                "fit_negative": bool(value in negative_set),
                "development_positive": bool(value in development_positive_set),
                "development_negative": bool(value in development_negative_set),
                "held_out_positive": bool(value in held_positive_set),
                "held_out_negative": bool(value in held_negative_set),
            }
            for value in ranking
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--input-texts", required=True)
    parser.add_argument("--input-manifest", required=True)
    parser.add_argument("--embedding-provenance", required=True)
    parser.add_argument("--labels", required=True)
    parser.add_argument("--proposal-policy", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--concept", action="append", default=[])
    parser.add_argument("--bags", type=int, default=16)
    parser.add_argument("--unlabeled-per-positive", type=float, default=0.5)
    parser.add_argument("--c-value", type=float, default=1.0)
    parser.add_argument("--positive-weight", type=float, default=2.0)
    parser.add_argument("--target-positive-recall", type=float, default=1.0)
    parser.add_argument("--seed", type=int, default=1729)
    args = parser.parse_args()

    if (
        args.bags < 1
        or args.unlabeled_per_positive < 0
        or args.c_value <= 0
        or args.positive_weight <= 0
    ):
        raise SystemExit("invalid fit parameter")
    if not 0 < args.target_positive_recall <= 1:
        raise SystemExit("--target-positive-recall must be in (0, 1]")

    vector_path = Path(args.vectors)
    input_path = Path(args.input_texts)
    manifest_path = Path(args.input_manifest)
    embedding_path = Path(args.embedding_provenance)
    labels_path = Path(args.labels)
    policy_path = Path(args.proposal_policy)

    ids, vectors = load_vectors(vector_path)
    validate_inputs(input_path, ids)
    labels = load_labels(labels_path, set(ids))
    policy = load_policy(policy_path)
    available = sorted(
        {label.concept for label in labels if label.role == "fit_positive"}
    )
    concepts = sorted(set(args.concept)) if args.concept else available
    if not concepts or set(concepts) - set(available):
        raise ValueError("requested concept lacks a fit-positive set")

    input_manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    embedding_provenance = json.loads(embedding_path.read_text(encoding="utf-8"))
    input_sha256 = sha256(input_path)
    if input_manifest.get("title_or_url_used") is not False:
        raise ValueError(
            "body probe requires a manifest asserting title_or_url_used=false"
        )
    if input_manifest.get("output_sha256") != input_sha256:
        raise ValueError(
            "body-input manifest does not match the body-text input artifact"
        )
    if embedding_provenance.get("input_text_sha256") != input_sha256:
        raise ValueError(
            "embedding provenance does not match the body-text input artifact"
        )

    report = {
        "format": FORMAT,
        "warning": "test concept labels are experimental supervision, not durable user organization",
        "vector_source": {
            "path": str(vector_path),
            "sha256": sha256(vector_path),
            "rows": len(ids),
            "dimensions": int(vectors.shape[1]),
        },
        "body_input": {
            "path": str(input_path),
            "sha256": input_sha256,
            "manifest_path": str(manifest_path),
            "manifest_sha256": sha256(manifest_path),
            "format": input_manifest.get("format"),
            "title_or_url_used": False,
        },
        "embedding": embedding_provenance,
        "labels": {
            "path": str(labels_path),
            "sha256": sha256(labels_path),
            "rows": [label.__dict__ for label in labels],
        },
        "proposal_policy": {
            "path": str(policy_path),
            "sha256": sha256(policy_path),
            "definition": policy,
        },
        "runtime": {
            "numpy": np.__version__,
            "scikit_learn": sklearn.__version__,
        },
        "fit": {
            "kind": "linear soft-margin SVM planes with provisional-unlabeled resampling",
            "bags": args.bags,
            "unlabeled_per_positive": args.unlabeled_per_positive,
            "C": args.c_value,
            "positive_weight": args.positive_weight,
            "target_positive_recall": args.target_positive_recall,
            "seed": args.seed,
        },
        "concepts": [
            fit_concept(
                concept,
                ids,
                vectors,
                labels,
                policy,
                args.bags,
                args.unlabeled_per_positive,
                args.c_value,
                args.positive_weight,
                args.target_positive_recall,
                args.seed + index * 1009,
            )
            for index, concept in enumerate(concepts)
        ],
    }

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
