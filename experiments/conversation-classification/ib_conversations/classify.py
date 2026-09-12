"""Rules, sparse/dense independent separators, and filing-oriented evaluation."""

from __future__ import annotations

import collections
import hashlib
import io
import json
import math
import platform
import re
import time
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

import joblib
import numpy as np
import scipy
import sklearn
from scipy import sparse
from sklearn.decomposition import TruncatedSVD
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.preprocessing import Normalizer, StandardScaler
from sklearn.svm import LinearSVC

from .corpus import Document
from .identity import canonical_json_bytes, sha256_bytes, write_bytes_if_changed, write_json_if_changed, write_jsonl_if_changed


MODEL_REVISION = "conversation-independent-separators-v2"


@dataclass(frozen=True)
class FeatureSpace:
    matrix: Any
    names: tuple[str, ...]
    representation: str
    provenance: dict[str, Any]


@dataclass
class SparseEncoder:
    word: TfidfVectorizer
    character: TfidfVectorizer
    scaler: StandardScaler
    structural_names: tuple[str, ...]
    view: str
    provenance: dict[str, Any]

    def transform(self, documents: list[Document]) -> FeatureSpace:
        texts = [document.text(self.view) for document in documents]
        word_matrix = self.word.transform(texts)
        character_matrix = self.character.transform(texts)
        structural = np.asarray(
            [[document.structural_features()[name] for name in self.structural_names] for document in documents],
            dtype=np.float64,
        )
        structural_matrix = sparse.csr_matrix(self.scaler.transform(structural))
        matrix = sparse.hstack((word_matrix, character_matrix, structural_matrix), format="csr")
        names = (
            tuple(f"word:{name}" for name in self.word.get_feature_names_out())
            + tuple(f"char:{name}" for name in self.character.get_feature_names_out())
            + tuple(f"structure:{name}" for name in self.structural_names)
        )
        return FeatureSpace(matrix, names, f"tfidf_word_character_structure:{self.view}", self.provenance)


@dataclass
class StructuralEncoder:
    scaler: StandardScaler
    structural_names: tuple[str, ...]
    provenance: dict[str, Any]

    def transform(self, documents: list[Document]) -> FeatureSpace:
        structural = np.asarray(
            [[document.structural_features()[name] for name in self.structural_names] for document in documents],
            dtype=np.float64,
        )
        matrix = self.scaler.transform(structural)
        return FeatureSpace(
            np.asarray(matrix, dtype=np.float32),
            tuple(f"structure:{name}" for name in self.structural_names),
            "scaled_structural_features",
            self.provenance,
        )


@dataclass
class WeightedEncoder:
    vectorizer: TfidfVectorizer
    user_weight: float
    assistant_weight: float
    title_weight: float
    provenance: dict[str, Any]

    def transform(self, documents: list[Document]) -> FeatureSpace:
        matrix = (
            self.user_weight * self.vectorizer.transform([document.text("user") for document in documents])
            + self.assistant_weight * self.vectorizer.transform([document.text("assistant") for document in documents])
            + self.title_weight * self.vectorizer.transform([document.text("title") for document in documents])
        )
        matrix = Normalizer(copy=False).transform(matrix.tocsr())
        names = tuple(f"weighted:{name}" for name in self.vectorizer.get_feature_names_out())
        return FeatureSpace(matrix, names, "weighted_shared_tfidf", self.provenance)


@dataclass
class DenseEncoder:
    base: Any
    decomposition: TruncatedSVD
    provenance: dict[str, Any]

    def transform(self, documents: list[Document]) -> FeatureSpace:
        base_space = self.base.transform(documents)
        matrix = Normalizer(copy=False).transform(self.decomposition.transform(base_space.matrix))
        dimensions = int(self.decomposition.n_components)
        return FeatureSpace(
            np.asarray(matrix, dtype=np.float32),
            tuple(f"lsa:{index}" for index in range(dimensions)),
            f"dense_lsa_from:{base_space.representation}",
            self.provenance,
        )


def _seed(value: str) -> int:
    return int(hashlib.sha256(value.encode("utf-8")).hexdigest()[:8], 16)


def _quantiles(values: Iterable[float]) -> dict[str, float] | None:
    array = np.asarray(list(values), dtype=np.float64)
    if not len(array):
        return None
    return {
        "minimum": float(np.min(array)),
        "p10": float(np.quantile(array, 0.10)),
        "median": float(np.quantile(array, 0.50)),
        "p90": float(np.quantile(array, 0.90)),
        "maximum": float(np.max(array)),
    }


def fit_sparse_encoder(documents: list[Document], train_indices: list[int], view: str) -> SparseEncoder:
    texts = [document.text(view) for document in documents]
    train_texts = [texts[index] for index in train_indices]
    minimum = 2 if len(train_indices) >= 50 else 1
    word = TfidfVectorizer(
        lowercase=True,
        ngram_range=(1, 2),
        min_df=minimum,
        sublinear_tf=True,
        max_features=30000,
        strip_accents="unicode",
    )
    character = TfidfVectorizer(
        lowercase=True,
        analyzer="char_wb",
        ngram_range=(3, 5),
        min_df=minimum,
        sublinear_tf=True,
        max_features=30000,
    )
    word.fit(train_texts)
    character.fit(train_texts)
    structural_names = sorted(documents[0].structural_features()) if documents else []
    structural = np.asarray(
        [[document.structural_features()[name] for name in structural_names] for document in documents],
        dtype=np.float64,
    )
    scaler = StandardScaler(with_mean=False)
    scaler.fit(structural[train_indices])
    return SparseEncoder(
        word,
        character,
        scaler,
        tuple(structural_names),
        view,
        {
            "view": view,
            "word_ngram_range": [1, 2],
            "character_ngram_range": [3, 5],
            "min_document_frequency": minimum,
            "maximum_word_features": 30000,
            "maximum_character_features": 30000,
            "structural_features": structural_names,
            "fit_scope": "train only",
        },
    )


def fit_structural_encoder(documents: list[Document], train_indices: list[int]) -> StructuralEncoder:
    structural_names = tuple(sorted(documents[0].structural_features())) if documents else tuple()
    structural = np.asarray(
        [[document.structural_features()[name] for name in structural_names] for document in documents],
        dtype=np.float64,
    )
    scaler = StandardScaler(with_mean=True)
    scaler.fit(structural[train_indices])
    return StructuralEncoder(
        scaler,
        structural_names,
        {
            "structural_features": list(structural_names),
            "fit_scope": "train only",
            "text_features": False,
        },
    )


def fit_view_encoder(documents: list[Document], train_indices: list[int], view: str) -> Any:
    if view == "structural":
        return fit_structural_encoder(documents, train_indices)
    return fit_sparse_encoder(documents, train_indices, view)


def sparse_features(documents: list[Document], train_indices: list[int], view: str) -> FeatureSpace:
    return fit_view_encoder(documents, train_indices, view).transform(documents)


def fit_weighted_encoder(
    documents: list[Document],
    train_indices: list[int],
    *,
    user_weight: float = 4.0,
    assistant_weight: float = 1.0,
    title_weight: float = 2.0,
) -> WeightedEncoder:
    user = [document.text("user") for document in documents]
    assistant = [document.text("assistant") for document in documents]
    title = [document.text("title") for document in documents]
    training = [user[index] for index in train_indices] + [assistant[index] for index in train_indices] + [title[index] for index in train_indices]
    minimum = 2 if len(train_indices) >= 50 else 1
    vectorizer = TfidfVectorizer(
        lowercase=True,
        ngram_range=(1, 2),
        min_df=minimum,
        sublinear_tf=True,
        max_features=50000,
        strip_accents="unicode",
    )
    vectorizer.fit(training)
    return WeightedEncoder(
        vectorizer,
        user_weight,
        assistant_weight,
        title_weight,
        {
            "user_weight": user_weight,
            "assistant_weight": assistant_weight,
            "title_weight": title_weight,
            "ngram_range": [1, 2],
            "min_document_frequency": minimum,
            "maximum_features": 50000,
            "fit_scope": "train only",
        },
    )


def weighted_user_features(
    documents: list[Document],
    train_indices: list[int],
    *,
    user_weight: float = 4.0,
    assistant_weight: float = 1.0,
    title_weight: float = 2.0,
) -> FeatureSpace:
    encoder = fit_weighted_encoder(
        documents,
        train_indices,
        user_weight=user_weight,
        assistant_weight=assistant_weight,
        title_weight=title_weight,
    )
    return encoder.transform(documents)


def fit_dense_encoder(
    base_encoder: Any,
    documents: list[Document],
    train_indices: list[int],
    requested_dimensions: int = 128,
) -> DenseEncoder:
    space = base_encoder.transform(documents)
    maximum = min(space.matrix.shape[0] - 1, space.matrix.shape[1] - 1, requested_dimensions)
    if maximum < 2:
        raise ValueError("dense LSA needs at least two available dimensions")
    decomposition = TruncatedSVD(n_components=maximum, random_state=0, algorithm="randomized")
    decomposition.fit(space.matrix[train_indices])
    return DenseEncoder(
        base_encoder,
        decomposition,
        {
            "source_representation": space.representation,
            "requested_dimensions": requested_dimensions,
            "actual_dimensions": maximum,
            "explained_variance_ratio_sum": float(decomposition.explained_variance_ratio_.sum()),
            "fit_scope": "train only",
        },
    )


def dense_lsa(space: FeatureSpace, train_indices: list[int], requested_dimensions: int = 128) -> FeatureSpace:
    """Legacy in-memory helper retained for small direct tests."""
    maximum = min(space.matrix.shape[0] - 1, space.matrix.shape[1] - 1, requested_dimensions)
    if maximum < 2:
        raise ValueError("dense LSA needs at least two available dimensions")
    decomposition = TruncatedSVD(n_components=maximum, random_state=0, algorithm="randomized")
    decomposition.fit(space.matrix[train_indices])
    matrix = Normalizer(copy=False).transform(decomposition.transform(space.matrix))
    return FeatureSpace(
        np.asarray(matrix, dtype=np.float32),
        tuple(f"lsa:{index}" for index in range(maximum)),
        f"dense_lsa_from:{space.representation}",
        {
            "source_representation": space.representation,
            "requested_dimensions": requested_dimensions,
            "actual_dimensions": maximum,
            "explained_variance_ratio_sum": float(decomposition.explained_variance_ratio_.sum()),
            "fit_scope": "train only",
        },
    )


def _category_sets(
    ids: list[str],
    labels: dict[str, dict[str, bool]],
    category: str,
    indices: list[int],
    closed_world_single_label: bool = False,
) -> tuple[list[int], list[int], list[int]]:
    positives: list[int] = []
    negatives: list[int] = []
    unlabeled: list[int] = []
    for index in indices:
        value = labels.get(ids[index], {}).get(category)
        if value is True:
            positives.append(index)
        elif value is False or (closed_world_single_label and any(labels.get(ids[index], {}).values())):
            negatives.append(index)
        else:
            unlabeled.append(index)
    return positives, negatives, unlabeled


def _positive_recall_threshold(scores: np.ndarray, positives: list[int], target_recall: float) -> float:
    positive_scores = np.sort(scores[positives])
    allowed_misses = min(
        max(int(math.floor((1.0 - target_recall) * len(positive_scores) + 1e-9)), 0),
        len(positive_scores) - 1,
    )
    return float(positive_scores[allowed_misses] - 1e-9)


def _calibrate_threshold(
    scores: np.ndarray,
    ids: list[str],
    labels: dict[str, dict[str, bool]],
    category: str,
    partitions: dict[str, str],
    train_positives: list[int],
    *,
    target_precision: float,
    target_recall: float,
    closed_world_single_label: bool,
) -> tuple[float, dict[str, Any]]:
    development = [index for index, target_id in enumerate(ids) if partitions[target_id] == "development"]
    positives, negatives, _unlabeled = _category_sets(
        ids,
        labels,
        category,
        development,
        closed_world_single_label,
    )
    if positives and negatives:
        candidates = sorted({float(scores[index]) for index in positives + negatives}, reverse=True)
        feasible: list[tuple[float, float, float]] = []
        for threshold in candidates:
            tp = sum(scores[index] >= threshold for index in positives)
            fp = sum(scores[index] >= threshold for index in negatives)
            precision = tp / (tp + fp) if tp + fp else 0.0
            recall = tp / len(positives)
            if tp and precision >= target_precision:
                feasible.append((recall, precision, threshold))
        if feasible:
            recall, precision, threshold = max(feasible, key=lambda row: (row[0], row[1], row[2]))
            return threshold, {
                "source": "development_precision_constraint",
                "development_positives": len(positives),
                "development_negatives": len(negatives),
                "target_precision": target_precision,
                "achieved_precision": precision,
                "achieved_recall": recall,
            }
        threshold = float(max(scores[index] for index in positives + negatives) + 1e-9)
        return threshold, {
            "source": "development_abstention_no_threshold_met_precision",
            "development_positives": len(positives),
            "development_negatives": len(negatives),
            "target_precision": target_precision,
        }
    if positives:
        return _positive_recall_threshold(scores, positives, target_recall), {
            "source": "development_positive_recall_only",
            "development_positives": len(positives),
            "development_negatives": 0,
            "target_recall": target_recall,
        }
    if negatives:
        return float(max(scores[index] for index in negatives) + 1e-9), {
            "source": "development_negative_ceiling_only",
            "development_positives": 0,
            "development_negatives": len(negatives),
        }
    return _positive_recall_threshold(scores, train_positives, target_recall), {
        "source": "training_positive_recall_fallback",
        "development_positives": 0,
        "development_negatives": 0,
        "target_recall": target_recall,
    }


def _provisional_samples(unlabeled: list[int], count: int, bags: int, seed: int) -> list[list[int]]:
    if count <= 0:
        return [[]]
    count = min(count, len(unlabeled))
    if count == len(unlabeled):
        return [list(unlabeled)]
    random = np.random.default_rng(seed)
    samples: set[tuple[int, ...]] = set()
    maximum_attempts = max(100, bags * 20)
    for _ in range(maximum_attempts):
        sample = tuple(sorted(int(value) for value in random.choice(unlabeled, size=count, replace=False)))
        samples.add(sample)
        if len(samples) >= bags:
            break
    return [list(sample) for sample in sorted(samples)]


def _top_features(weights: np.ndarray, names: tuple[str, ...], count: int = 20) -> dict[str, list[dict[str, Any]]]:
    count = min(count, len(weights))
    positive = np.argsort(weights)[-count:][::-1]
    negative = np.argsort(weights)[:count]
    return {
        "positive": [{"feature": names[int(index)], "weight": float(weights[index])} for index in positive],
        "negative": [{"feature": names[int(index)], "weight": float(weights[index])} for index in negative],
    }


def _local_contributions(
    matrix: Any,
    index: int,
    weights: np.ndarray,
    names: tuple[str, ...],
    count: int = 10,
) -> dict[str, list[dict[str, Any]]]:
    row = matrix[index]
    if sparse.issparse(row):
        row = row.tocsr()
        contributions = row.data * weights[row.indices]
        pairs = list(zip(row.indices.tolist(), contributions.tolist()))
    else:
        values = np.asarray(row, dtype=np.float64).ravel() * weights
        pairs = [(index, float(value)) for index, value in enumerate(values) if value != 0.0]
    positive = sorted((pair for pair in pairs if pair[1] > 0), key=lambda pair: (-pair[1], names[pair[0]]))[:count]
    negative = sorted((pair for pair in pairs if pair[1] < 0), key=lambda pair: (pair[1], names[pair[0]]))[:count]
    return {
        "supporting": [{"feature": names[index], "contribution": float(value)} for index, value in positive],
        "opposing": [{"feature": names[index], "contribution": float(value)} for index, value in negative],
    }


def write_npz_deterministic(path: Path, arrays: dict[str, np.ndarray]) -> bool:
    output = io.BytesIO()
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, array in sorted(arrays.items()):
            value = io.BytesIO()
            np.lib.format.write_array(value, np.asarray(array), allow_pickle=False)
            info = zipfile.ZipInfo(f"{name}.npy", date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, value.getvalue(), compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    return write_bytes_if_changed(path, output.getvalue())


def write_encoder(path: Path, encoder: Any) -> bool:
    """Persist a fitted sklearn encoder; load only artifacts produced by this experiment."""
    output = io.BytesIO()
    joblib.dump(encoder, output, compress=("zlib", 9), protocol=5)
    return write_bytes_if_changed(path, output.getvalue())


def load_encoder(path: Path) -> Any:
    """Load a trusted local experiment artifact (joblib/pickle is not an interchange format)."""
    return joblib.load(io.BytesIO(path.read_bytes()))


def fit_independent_hyperplanes(
    space: FeatureSpace,
    documents: list[Document],
    labels: dict[str, dict[str, bool]],
    partitions: dict[str, str],
    *,
    bags: int = 7,
    unlabeled_per_positive: float = 5.0,
    target_positive_recall: float = 0.90,
    target_development_precision: float = 0.90,
    closed_world_single_label: bool = False,
    minimum_train_positives: int = 5,
) -> tuple[list[dict[str, Any]], dict[str, Any], dict[str, np.ndarray]]:
    ids = [document.corpus_id for document in documents]
    train_indices = [index for index, target_id in enumerate(ids) if partitions[target_id] == "train"]
    model_selection_indices = [
        index
        for index, target_id in enumerate(ids)
        if partitions[target_id] in {"train", "development"}
    ]
    categories = sorted({category for values in labels.values() for category in values})
    predictions = {target_id: {"corpus_id": target_id, "categories": {}} for target_id in ids}
    model_rows: list[dict[str, Any]] = []
    arrays: dict[str, np.ndarray] = {}
    skipped: dict[str, str] = {}
    for category in categories:
        positives, negatives, unlabeled = _category_sets(
            ids,
            labels,
            category,
            train_indices,
            closed_world_single_label,
        )
        if len(positives) < minimum_train_positives:
            skipped[category] = f"needs at least {minimum_train_positives} train positives; found {len(positives)}"
            continue
        requested = max(1, int(math.ceil(len(positives) * unlabeled_per_positive))) if not negatives else int(math.ceil(len(positives) * unlabeled_per_positive))
        samples = _provisional_samples(unlabeled, requested, bags, _seed(f"{MODEL_REVISION}:{space.representation}:{category}"))
        if not negatives and not samples:
            skipped[category] = "no explicit negatives or unlabeled rows"
            continue
        bag_scores: list[np.ndarray] = []
        bag_thresholds: list[float] = []
        bag_weights: list[np.ndarray] = []
        bag_offsets: list[float] = []
        bag_rows: list[dict[str, Any]] = []
        for bag_number, provisional in enumerate(samples):
            negative_indices = sorted(set(negatives + provisional))
            if not negative_indices:
                continue
            fitted = positives + negative_indices
            y = np.asarray([1] * len(positives) + [0] * len(negative_indices), dtype=np.int8)
            positive_weight = max(1.0, len(negative_indices) / len(positives))
            classifier = LinearSVC(
                C=1.0,
                class_weight={0: 1.0, 1: positive_weight},
                dual=False,
                max_iter=20000,
                random_state=_seed(f"{category}:{bag_number}"),
            )
            classifier.fit(space.matrix[fitted], y)
            weights = np.asarray(classifier.coef_[0], dtype=np.float64)
            norm = float(np.linalg.norm(weights))
            if not np.isfinite(norm) or norm == 0.0:
                continue
            offset = float(-classifier.intercept_[0] / norm)
            scores = np.asarray(classifier.decision_function(space.matrix), dtype=np.float64) / norm
            threshold, calibration = _calibrate_threshold(
                scores,
                ids,
                labels,
                category,
                partitions,
                positives,
                target_precision=target_development_precision,
                target_recall=target_positive_recall,
                closed_world_single_label=closed_world_single_label,
            )
            normalized_weights = weights / norm
            bag_scores.append(scores)
            bag_thresholds.append(threshold)
            bag_weights.append(normalized_weights.astype(np.float32))
            bag_offsets.append(offset)
            bag_rows.append(
                {
                    "bag": bag_number,
                    "train_positive_ids": [ids[index] for index in positives],
                    "train_explicit_negative_ids": [ids[index] for index in negatives],
                    "train_provisional_unlabeled_ids": [ids[index] for index in provisional],
                    "normal_l2_norm_before_normalization": norm,
                    "offset": offset,
                    "proposal_threshold": threshold,
                    "threshold_calibration": calibration,
                    "margin_violations": [ids[index] for index in fitted if (scores[index] >= 0.0) != (index in positives)],
                    "top_features": _top_features(normalized_weights, space.names),
                }
            )
        if not bag_scores:
            skipped[category] = "all fitted planes were degenerate"
            continue
        score_matrix = np.stack(bag_scores)
        threshold_array = np.asarray(bag_thresholds)[:, None]
        median_scores = np.median(score_matrix, axis=0)
        vote_fraction = np.mean(score_matrix >= threshold_array, axis=0)
        proposal_threshold = float(np.median(bag_thresholds))
        accepted = (median_scores >= proposal_threshold) & (vote_fraction >= 0.60)
        ensemble_normal = np.mean(np.stack(bag_weights), axis=0)
        safe_name = hashlib.sha256(category.encode("utf-8")).hexdigest()[:16]
        arrays[f"normal_{safe_name}"] = np.stack(bag_weights)
        arrays[f"offset_{safe_name}"] = np.asarray(bag_offsets, dtype=np.float32)
        for index, target_id in enumerate(ids):
            predictions[target_id]["categories"][category] = {
                "score": float(median_scores[index]),
                "vote_fraction": float(vote_fraction[index]),
                "accepted": bool(accepted[index]),
                "proposal_threshold": proposal_threshold,
                "ranking_margin": float(median_scores[index] - proposal_threshold),
                "ranking_priority": 0,
                "feature_contributions": _local_contributions(
                    space.matrix,
                    index,
                    ensemble_normal,
                    space.names,
                ),
                "feature_contribution_meaning": "contribution under the ensemble-mean normalized plane; the proposal score remains the median per-bag margin",
            }
        model_rows.append(
            {
                "category": category,
                "category_array_key": safe_name,
                "score_definition": "dot(normal, representation) - offset",
                "independent_membership": True,
                "proposal_threshold": proposal_threshold,
                "vote_threshold": 0.60,
                "bags": bag_rows,
                "model_selection_margin_quantiles": _quantiles(median_scores[model_selection_indices]),
                "model_selection_accepted_count": int(accepted[model_selection_indices].sum()),
            }
        )
    model = {
        "model_revision": MODEL_REVISION,
        "model_kind": "bagged_positive_unlabeled_independent_linear_svc",
        "representation": space.representation,
        "feature_provenance": space.provenance,
        "feature_count": len(space.names),
        "categories": model_rows,
        "skipped_categories": skipped,
        "unlabeled_per_positive": unlabeled_per_positive,
        "target_positive_recall_when_no_labeled_negatives": target_positive_recall,
        "target_development_precision": target_development_precision,
        "closed_world_single_label": closed_world_single_label,
        "minimum_train_positives": minimum_train_positives,
    }
    return [predictions[target_id] for target_id in ids], model, arrays


def write_geometric_model(
    directory: Path,
    encoder: Any,
    space: FeatureSpace,
    model: dict[str, Any],
    arrays: dict[str, np.ndarray],
    training_provenance: dict[str, Any] | None = None,
) -> dict[str, Any]:
    directory.mkdir(parents=True, exist_ok=True)
    write_json_if_changed(directory / "model.json", model)
    write_json_if_changed(directory / "features.json", list(space.names))
    write_encoder(directory / "encoder.joblib", encoder)
    if training_provenance is not None:
        write_json_if_changed(directory / "training.json", training_provenance)
    if arrays:
        write_npz_deterministic(directory / "planes.npz", arrays)
    hashes = {
        name: sha256_bytes((directory / name).read_bytes())
        for name in ("model.json", "features.json", "encoder.joblib", "planes.npz", "training.json")
        if (directory / name).exists()
    }
    generation_id = sha256_bytes(canonical_json_bytes(hashes))
    artifact = {
        "model_generation_id": generation_id,
        "artifact_sha256": hashes,
        "trusted_load_boundary": "encoder.joblib is Python/sklearn-specific and must only be loaded from a trusted experiment branch",
        "train_and_classify_are_separate": True,
    }
    write_json_if_changed(directory / "artifact.json", artifact)
    return artifact


def classify_geometric_model(directory: Path, documents: list[Document]) -> list[dict[str, Any]]:
    """Transform and classify documents without fitting or changing a model."""
    artifact = json.loads((directory / "artifact.json").read_text(encoding="utf-8"))
    for name, expected in artifact["artifact_sha256"].items():
        actual = sha256_bytes((directory / name).read_bytes())
        if actual != expected:
            raise ValueError(f"model artifact hash mismatch for {name}")
    encoder = load_encoder(directory / "encoder.joblib")
    space = encoder.transform(documents)
    model = json.loads((directory / "model.json").read_text(encoding="utf-8"))
    expected_names = json.loads((directory / "features.json").read_text(encoding="utf-8"))
    if list(space.names) != expected_names:
        raise ValueError("encoder output does not match the model feature order")
    arrays = np.load(directory / "planes.npz", allow_pickle=False)
    rows = {
        document.corpus_id: {
            "corpus_id": document.corpus_id,
            "input_sha256": document.source_sha256,
            "model_generation_id": artifact["model_generation_id"],
            "categories": {},
        }
        for document in documents
    }
    for category_row in model["categories"]:
        category = category_row["category"]
        key = category_row["category_array_key"]
        normals = np.asarray(arrays[f"normal_{key}"], dtype=np.float64)
        offsets = np.asarray(arrays[f"offset_{key}"], dtype=np.float64)
        values = space.matrix @ normals.T
        scores = np.asarray(values, dtype=np.float64) - offsets[None, :]
        thresholds = np.asarray(
            [float(bag["proposal_threshold"]) for bag in category_row["bags"]],
            dtype=np.float64,
        )
        median_scores = np.median(scores, axis=1)
        votes = np.mean(scores >= thresholds[None, :], axis=1)
        proposal_threshold = float(category_row["proposal_threshold"])
        accepted = (median_scores >= proposal_threshold) & (votes >= float(category_row["vote_threshold"]))
        ensemble_normal = np.mean(normals, axis=0)
        for index, document in enumerate(documents):
            rows[document.corpus_id]["categories"][category] = {
                "score": float(median_scores[index]),
                "vote_fraction": float(votes[index]),
                "accepted": bool(accepted[index]),
                "proposal_threshold": proposal_threshold,
                "ranking_margin": float(median_scores[index] - proposal_threshold),
                "ranking_priority": 0,
                "feature_contributions": _local_contributions(
                    space.matrix,
                    index,
                    ensemble_normal,
                    space.names,
                ),
                "feature_contribution_meaning": "contribution under the ensemble-mean normalized plane; the proposal score remains the median per-bag margin",
            }
    return [rows[document.corpus_id] for document in documents]


def training_provenance(
    documents: list[Document],
    labels: dict[str, dict[str, bool]],
    partitions: dict[str, str],
) -> dict[str, Any]:
    rows = [
        {
            "corpus_id": document.corpus_id,
            "source_sha256": document.source_sha256,
            "labels": labels.get(document.corpus_id, {}),
        }
        for document in documents
        if partitions[document.corpus_id] in {"train", "development"}
    ]
    for row in rows:
        row["partition"] = partitions[row["corpus_id"]]
    rows = sorted(rows, key=lambda row: row["corpus_id"])
    return {
        "scope": "training fit and development threshold calibration; test excluded",
        "documents": rows,
        "model_selection_input_sha256": sha256_bytes(canonical_json_bytes(rows)),
    }


def train_geometric_model(
    documents: list[Document],
    labels: dict[str, dict[str, bool]],
    partitions: dict[str, str],
    output: Path,
    *,
    representation: str,
    view: str = "user",
    categories: set[str] | None = None,
    closed_world_single_label: bool = False,
) -> dict[str, Any]:
    """Fit and persist a model without creating classification proposals."""
    train_indices = [index for index, document in enumerate(documents) if partitions[document.corpus_id] == "train"]
    if len(train_indices) < 2:
        raise ValueError("training needs at least two training documents")
    selected_labels = labels
    if categories is not None:
        selected_labels = {
            target_id: {category: value for category, value in values.items() if category in categories}
            for target_id, values in labels.items()
        }
    if representation == "sparse":
        encoder: Any = fit_view_encoder(documents, train_indices, view)
    elif representation == "dense_lsa":
        base = fit_view_encoder(documents, train_indices, view)
        encoder = fit_dense_encoder(base, documents, train_indices)
    elif representation == "weighted":
        encoder = fit_weighted_encoder(documents, train_indices)
    else:
        raise ValueError(f"unknown representation {representation!r}")
    space = encoder.transform(documents)
    _training_predictions, model, arrays = fit_independent_hyperplanes(
        space,
        documents,
        selected_labels,
        partitions,
        closed_world_single_label=closed_world_single_label,
    )
    provenance = training_provenance(documents, selected_labels, partitions)
    artifact = write_geometric_model(output, encoder, space, model, arrays, provenance)
    result = {
        "operation": "train",
        "representation": space.representation,
        "view": view,
        "train_documents": len(train_indices),
        "trained_categories": [row["category"] for row in model["categories"]],
        "skipped_categories": model["skipped_categories"],
        "model_generation_id": artifact["model_generation_id"],
        "model_selection_input_sha256": provenance["model_selection_input_sha256"],
        "proposals_written": 0,
        "closed_world_single_label": closed_world_single_label,
    }
    write_json_if_changed(output / "train-report.json", result)
    return result


def merge_incremental_proposals(
    previous_path: Path | None,
    classified: list[dict[str, Any]],
    inventory: dict[str, str],
    model_generation_id: str,
) -> tuple[list[dict[str, Any]], int]:
    """Reuse unchanged proposals after validating both input and model identities."""
    merged: dict[str, dict[str, Any]] = {}
    classified_ids = {row["corpus_id"] for row in classified}
    if previous_path is not None and previous_path.exists():
        for line in previous_path.read_text(encoding="utf-8").splitlines():
            if not line.strip():
                continue
            row = json.loads(line)
            target_id = row["corpus_id"]
            if target_id not in inventory:
                continue
            if row.get("model_generation_id") != model_generation_id:
                raise ValueError("cannot reuse proposals from a different model generation")
            if row.get("input_sha256") != inventory[target_id]:
                continue
            merged[target_id] = row
    for row in classified:
        merged[row["corpus_id"]] = row
    missing = sorted(set(inventory) - set(merged))
    if missing:
        raise ValueError(f"no classification proposal for {len(missing)} corpus rows; first is {missing[0]}")
    reused = sum(target_id not in classified_ids for target_id in merged)
    return [merged[target_id] for target_id in sorted(merged)], reused


def fit_nearest_centroids(
    space: FeatureSpace,
    documents: list[Document],
    labels: dict[str, dict[str, bool]],
    partitions: dict[str, str],
    *,
    target_positive_recall: float = 0.90,
    target_development_precision: float = 0.90,
    closed_world_single_label: bool = False,
    minimum_train_positives: int = 5,
) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    matrix = np.asarray(space.matrix, dtype=np.float64)
    ids = [document.corpus_id for document in documents]
    train = [index for index, target_id in enumerate(ids) if partitions[target_id] == "train"]
    model_selection = [
        index
        for index, target_id in enumerate(ids)
        if partitions[target_id] in {"train", "development"}
    ]
    categories = sorted({category for values in labels.values() for category in values})
    predictions = {target_id: {"corpus_id": target_id, "categories": {}} for target_id in ids}
    rows: list[dict[str, Any]] = []
    skipped: dict[str, str] = {}
    for category in categories:
        positives = [index for index in train if labels.get(ids[index], {}).get(category) is True]
        if len(positives) < minimum_train_positives:
            skipped[category] = f"needs at least {minimum_train_positives} train positives; found {len(positives)}"
            continue
        centroid = np.mean(matrix[positives], axis=0)
        norm = float(np.linalg.norm(centroid))
        if norm == 0.0:
            skipped[category] = "positive centroid is zero"
            continue
        centroid /= norm
        scores = matrix @ centroid
        threshold, calibration = _calibrate_threshold(
            scores,
            ids,
            labels,
            category,
            partitions,
            positives,
            target_precision=target_development_precision,
            target_recall=target_positive_recall,
            closed_world_single_label=closed_world_single_label,
        )
        for index, target_id in enumerate(ids):
            predictions[target_id]["categories"][category] = {
                "score": float(scores[index]),
                "vote_fraction": 1.0,
                "accepted": bool(scores[index] >= threshold),
                "proposal_threshold": threshold,
                "threshold_calibration": calibration,
                "ranking_margin": float(scores[index] - threshold),
                "ranking_priority": 0,
            }
        rows.append(
            {
                "category": category,
                "train_positive_ids": [ids[index] for index in positives],
                "centroid": centroid.astype(np.float32).tolist(),
                "proposal_threshold": threshold,
                "model_selection_score_quantiles": _quantiles(scores[model_selection]),
            }
        )
    return [predictions[target_id] for target_id in ids], {
        "model_revision": MODEL_REVISION,
        "model_kind": "positive_nearest_centroid",
        "representation": space.representation,
        "categories": rows,
        "skipped_categories": skipped,
        "target_development_precision": target_development_precision,
        "closed_world_single_label": closed_world_single_label,
        "minimum_train_positives": minimum_train_positives,
    }


def load_rules(path: Path | None) -> dict[str, list[re.Pattern[str]]]:
    if path is None:
        return {}
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("rules must be a category-to-pattern-list object")
    rules: dict[str, list[re.Pattern[str]]] = {}
    for category, patterns in value.items():
        if not isinstance(category, str) or not isinstance(patterns, list) or not all(isinstance(pattern, str) for pattern in patterns):
            raise ValueError("rules must map category strings to regex string lists")
        rules[category] = [re.compile(pattern, re.IGNORECASE) for pattern in patterns]
    return rules


def rule_predictions(documents: list[Document], rules: dict[str, list[re.Pattern[str]]]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for document in documents:
        text = document.text("title_user")
        categories: dict[str, Any] = {}
        for category, patterns in sorted(rules.items()):
            evidence = [pattern.pattern for pattern in patterns if pattern.search(text)]
            if evidence:
                categories[category] = {
                    "score": float(len(evidence)),
                    "vote_fraction": 1.0,
                    "accepted": True,
                    "proposal_threshold": 1.0,
                    "ranking_margin": float(len(evidence) - 1.0),
                    "ranking_priority": 1,
                    "evidence": evidence,
                }
        rows.append({"corpus_id": document.corpus_id, "categories": categories})
    return rows


def hybrid_predictions(rules: list[dict[str, Any]], geometric: list[dict[str, Any]]) -> list[dict[str, Any]]:
    geometric_by_id = {row["corpus_id"]: row for row in geometric}
    output: list[dict[str, Any]] = []
    for rule_row in rules:
        target_id = rule_row["corpus_id"]
        combined = json.loads(json.dumps(geometric_by_id[target_id]["categories"]))
        for category, evidence in rule_row["categories"].items():
            combined[category] = {
                **combined.get(category, {}),
                "accepted": True,
                "rule_override": True,
                "rule_evidence": evidence.get("evidence", []),
                "score": max(float(combined.get(category, {}).get("score", -math.inf)), float(evidence["score"])),
                "ranking_margin": max(
                    float(combined.get(category, {}).get("ranking_margin", -math.inf)),
                    float(evidence["ranking_margin"]),
                ),
                "ranking_priority": 1,
            }
        output.append({"corpus_id": target_id, "categories": combined})
    return output


def multilabel_metrics(
    predictions: list[dict[str, Any]],
    labels: dict[str, dict[str, bool]],
    partitions: dict[str, str],
    split: str = "test",
    closed_world_single_label: bool = False,
) -> dict[str, Any]:
    by_id = {row["corpus_id"]: row["categories"] for row in predictions}
    categories = sorted(
        {category for values in labels.values() for category in values}
        | {category for row in predictions for category in row.get("categories", {})}
    )
    category_rows: dict[str, Any] = {}
    micro = collections.Counter()
    hamming_errors = hamming_total = 0
    unlabeled_proposals = unlabeled_total = 0
    for category in categories:
        counts = collections.Counter()
        for target_id, split_name in partitions.items():
            if split_name != split:
                continue
            proposed = bool(by_id.get(target_id, {}).get(category, {}).get("accepted", False))
            truth = labels.get(target_id, {}).get(category)
            if truth is None and closed_world_single_label and any(labels.get(target_id, {}).values()):
                truth = False
            if truth is True:
                counts["tp" if proposed else "fn"] += 1
                hamming_total += 1
                hamming_errors += int(not proposed)
            elif truth is False:
                counts["fp" if proposed else "tn"] += 1
                hamming_total += 1
                hamming_errors += int(proposed)
            else:
                unlabeled_total += 1
                unlabeled_proposals += int(proposed)
        tp, fp, fn, tn = (counts[key] for key in ("tp", "fp", "fn", "tn"))
        precision = tp / (tp + fp) if tp + fp else None
        recall = tp / (tp + fn) if tp + fn else None
        if not tp + fn:
            f1 = None
        elif tp == 0:
            f1 = 0.0
        else:
            f1 = 2 * precision * recall / (precision + recall) if precision is not None and recall is not None else 0.0
        false_positive_rate = fp / (fp + tn) if fp + tn else None
        false_negative_rate = fn / (fn + tp) if fn + tp else None
        category_rows[category] = {
            "tp": tp,
            "fp": fp,
            "fn": fn,
            "tn": tn,
            "precision": precision,
            "recall": recall,
            "f1": f1,
            "false_positive_rate": false_positive_rate,
            "false_negative_rate": false_negative_rate,
        }
        micro.update(counts)
    metric_rows = [row for row in category_rows.values() if row["f1"] is not None]
    micro_precision = micro["tp"] / (micro["tp"] + micro["fp"]) if micro["tp"] + micro["fp"] else None
    micro_recall = micro["tp"] / (micro["tp"] + micro["fn"]) if micro["tp"] + micro["fn"] else None
    micro_f1 = (
        2 * micro_precision * micro_recall / (micro_precision + micro_recall)
        if micro_precision is not None and micro_recall is not None and micro_precision + micro_recall
        else None
    )
    multi_ids = [
        target_id
        for target_id, split_name in partitions.items()
        if split_name == split and sum(value is True for value in labels.get(target_id, {}).values()) >= 2
    ]
    sample_f1: list[float] = []
    confusions: collections.Counter[tuple[str, str]] = collections.Counter()
    for target_id in multi_ids:
        truth = {category for category, value in labels[target_id].items() if value is True}
        proposed = {category for category, row in by_id.get(target_id, {}).items() if row.get("accepted")}
        if truth or proposed:
            sample_f1.append(2 * len(truth & proposed) / (len(truth) + len(proposed)))
    for target_id, split_name in partitions.items():
        if split_name != split:
            continue
        truth = {category for category, value in labels.get(target_id, {}).items() if value is True}
        proposed = {category for category, row in by_id.get(target_id, {}).items() if row.get("accepted")}
        for false_category in proposed - truth:
            explicitly_false = labels.get(target_id, {}).get(false_category) is False
            if not explicitly_false and not closed_world_single_label:
                continue
            for true_category in truth:
                confusions[(true_category, false_category)] += 1
    return {
        "split": split,
        "category": category_rows,
        "macro_f1": float(np.mean([row["f1"] for row in metric_rows])) if metric_rows else None,
        "micro_precision": micro_precision,
        "micro_recall": micro_recall,
        "micro_f1": micro_f1,
        "hamming_error_on_explicit_labels": hamming_errors / hamming_total if hamming_total else None,
        "explicit_label_decisions": hamming_total,
        "unlabeled_proposal_rate": unlabeled_proposals / unlabeled_total if unlabeled_total else None,
        "multi_category_conversations": len(multi_ids),
        "multi_category_mean_sample_f1": float(np.mean(sample_f1)) if sample_f1 else None,
        "closed_world_single_label": closed_world_single_label,
        "cross_category_confusions": [
            {"true_category": pair[0], "proposed_category": pair[1], "count": count}
            for pair, count in sorted(confusions.items(), key=lambda item: (-item[1], item[0]))
        ],
    }


def _ranked_categories(category_rows: dict[str, Any]) -> list[dict[str, Any]]:
    return sorted(
        (
            {
                "category": category,
                "priority": int(row.get("ranking_priority", 0)),
                "margin": float(
                    row.get(
                        "ranking_margin",
                        float(row.get("score", 0.0)) - float(row.get("proposal_threshold", 0.0)),
                    )
                ),
                "score": row.get("score"),
                "proposal_threshold": row.get("proposal_threshold"),
                "vote_fraction": row.get("vote_fraction"),
                "rule_evidence": row.get("rule_evidence", row.get("evidence", [])),
            }
            for category, row in category_rows.items()
            if row.get("accepted")
        ),
        key=lambda item: (-item["priority"], -item["margin"], item["category"]),
    )


def filing_projection(
    predictions: list[dict[str, Any]],
    *,
    ambiguity_margin: float = 0.05,
) -> list[dict[str, Any]]:
    output: list[dict[str, Any]] = []
    for row in predictions:
        accepted = _ranked_categories(row.get("categories", {}))
        base = {
            "corpus_id": row["corpus_id"],
            "input_sha256": row.get("input_sha256"),
            "model_generation_id": row.get("model_generation_id"),
        }
        if not accepted:
            output.append({**base, "outcome": "no_sufficiently_supported_destination", "candidates": []})
            continue
        gap = (
            math.inf
            if len(accepted) == 1 or accepted[0]["priority"] != accepted[1]["priority"]
            else accepted[0]["margin"] - accepted[1]["margin"]
        )
        if len(accepted) > 1 and accepted[0]["priority"] == accepted[1]["priority"] and gap < ambiguity_margin:
            output.append(
                {
                    **base,
                    "outcome": "several_plausible_destinations",
                    "candidates": accepted[:5],
                    "top_adjusted_margin_gap": gap,
                }
            )
        else:
            output.append(
                {
                    **base,
                    "outcome": "confident_destination",
                    "proposed_destination": accepted[0]["category"],
                    "evidence": accepted[0],
                    "alternatives": accepted[1:5],
                    "top_adjusted_margin_gap": gap,
                }
            )
    return output


def filing_metrics(
    predictions: list[dict[str, Any]],
    labels: dict[str, dict[str, bool]],
    partitions: dict[str, str],
    *,
    split: str = "test",
    ambiguity_margin: float = 0.05,
) -> dict[str, Any]:
    by_id = {row["corpus_id"]: row["categories"] for row in predictions}
    eligible = confident = wrong = ambiguous = unsupported = 0
    details: list[dict[str, Any]] = []
    for target_id, split_name in partitions.items():
        if split_name != split:
            continue
        truth = sorted(category for category, value in labels.get(target_id, {}).items() if value is True)
        if len(truth) != 1:
            continue
        eligible += 1
        accepted = _ranked_categories(by_id.get(target_id, {}))
        if not accepted:
            unsupported += 1
            details.append({"corpus_id": target_id, "truth": truth[0], "result": "no_supported_destination"})
            continue
        gap = (
            math.inf
            if len(accepted) == 1 or accepted[0]["priority"] != accepted[1]["priority"]
            else accepted[0]["margin"] - accepted[1]["margin"]
        )
        if len(accepted) > 1 and accepted[0]["priority"] == accepted[1]["priority"] and gap < ambiguity_margin:
            ambiguous += 1
            details.append(
                {
                    "corpus_id": target_id,
                    "truth": truth[0],
                    "result": "several_plausible_destinations",
                    "candidates": accepted[:5],
                    "top_adjusted_margin_gap": gap,
                }
            )
            continue
        confident += 1
        is_wrong = accepted[0]["category"] != truth[0]
        wrong += int(is_wrong)
        details.append(
            {
                "corpus_id": target_id,
                "truth": truth[0],
                "result": "confident_destination",
                "destination": accepted[0]["category"],
                "correct": not is_wrong,
                "top_adjusted_margin_gap": gap,
            }
        )
    return {
        "split": split,
        "eligible_single_destination_labels": eligible,
        "automatic_coverage": confident / eligible if eligible else None,
        "misfile_rate_among_automatic": wrong / confident if confident else None,
        "review_burden": (ambiguous + unsupported) / eligible if eligible else None,
        "confident": confident,
        "wrong_confident": wrong,
        "several_plausible": ambiguous,
        "no_supported_destination": unsupported,
        "ambiguity_margin": ambiguity_margin,
        "ranking": "rule priority first, then category score minus that category's proposal threshold",
        "details": details,
    }


def compare(
    documents: list[Document],
    labels: dict[str, dict[str, bool]],
    partitions: dict[str, str],
    output: Path,
    *,
    views: list[str],
    rules_path: Path | None = None,
    filing_evaluation: bool = False,
) -> dict[str, Any]:
    started = time.perf_counter()
    output.mkdir(parents=True, exist_ok=True)
    train_indices = [index for index, document in enumerate(documents) if partitions[document.corpus_id] == "train"]
    if len(train_indices) < 2:
        raise ValueError("comparison needs at least two training documents")
    configurations: list[tuple[str, Any, FeatureSpace]] = []
    for view in views:
        encoder = fit_view_encoder(documents, train_indices, view)
        configurations.append((f"sparse_{view}", encoder, encoder.transform(documents)))
    if {"user", "assistant"} <= set(views):
        encoder = fit_weighted_encoder(documents, train_indices)
        configurations.append(("weighted_user4_assistant1_title2", encoder, encoder.transform(documents)))

    run_rows: list[dict[str, Any]] = []
    predictions_by_model: dict[str, list[dict[str, Any]]] = {}
    for model_name, encoder, space in configurations:
        _fitted_predictions, model, arrays = fit_independent_hyperplanes(
            space,
            documents,
            labels,
            partitions,
            closed_world_single_label=filing_evaluation,
        )
        artifact = write_geometric_model(
            output / model_name,
            encoder,
            space,
            model,
            arrays,
            training_provenance(documents, labels, partitions),
        )
        predictions = classify_geometric_model(output / model_name, documents) if arrays else _fitted_predictions
        predictions_by_model[model_name] = predictions
        write_jsonl_if_changed(output / model_name / "proposals.jsonl", predictions)
        metrics = multilabel_metrics(predictions, labels, partitions, closed_world_single_label=filing_evaluation)
        filing = filing_metrics(predictions, labels, partitions) if filing_evaluation else None
        write_json_if_changed(output / model_name / "metrics.json", metrics)
        if filing is not None:
            write_json_if_changed(output / model_name / "filing.json", filing)
            write_jsonl_if_changed(output / model_name / "destination-proposals.jsonl", filing_projection(predictions))
        run_rows.append({"model": model_name, "representation": space.representation, "model_generation_id": artifact["model_generation_id"], "metrics": metrics, "filing": filing})

        if model_name == f"sparse_{views[0]}":
            try:
                dense_encoder = fit_dense_encoder(encoder, documents, train_indices)
                dense = dense_encoder.transform(documents)
            except ValueError:
                dense = None
            if dense is not None:
                dense_name = f"dense_lsa_{views[0]}"
                _dense_fitted, dense_model, dense_arrays = fit_independent_hyperplanes(
                    dense,
                    documents,
                    labels,
                    partitions,
                    closed_world_single_label=filing_evaluation,
                )
                dense_artifact = write_geometric_model(
                    output / dense_name,
                    dense_encoder,
                    dense,
                    dense_model,
                    dense_arrays,
                    training_provenance(documents, labels, partitions),
                )
                dense_predictions = classify_geometric_model(output / dense_name, documents) if dense_arrays else _dense_fitted
                predictions_by_model[dense_name] = dense_predictions
                write_jsonl_if_changed(output / dense_name / "proposals.jsonl", dense_predictions)
                dense_metrics = multilabel_metrics(dense_predictions, labels, partitions, closed_world_single_label=filing_evaluation)
                dense_filing = filing_metrics(dense_predictions, labels, partitions) if filing_evaluation else None
                write_json_if_changed(output / dense_name / "metrics.json", dense_metrics)
                if dense_filing is not None:
                    write_json_if_changed(output / dense_name / "filing.json", dense_filing)
                    write_jsonl_if_changed(output / dense_name / "destination-proposals.jsonl", filing_projection(dense_predictions))
                run_rows.append({"model": dense_name, "representation": dense.representation, "model_generation_id": dense_artifact["model_generation_id"], "metrics": dense_metrics, "filing": dense_filing})

                centroid_name = f"dense_centroid_{views[0]}"
                centroid_predictions, centroid_model = fit_nearest_centroids(
                    dense,
                    documents,
                    labels,
                    partitions,
                    closed_world_single_label=filing_evaluation,
                )
                predictions_by_model[centroid_name] = centroid_predictions
                write_jsonl_if_changed(output / centroid_name / "proposals.jsonl", centroid_predictions)
                write_json_if_changed(output / centroid_name / "model.json", centroid_model)
                centroid_metrics = multilabel_metrics(centroid_predictions, labels, partitions, closed_world_single_label=filing_evaluation)
                centroid_filing = filing_metrics(centroid_predictions, labels, partitions) if filing_evaluation else None
                write_json_if_changed(output / centroid_name / "metrics.json", centroid_metrics)
                if centroid_filing is not None:
                    write_json_if_changed(output / centroid_name / "filing.json", centroid_filing)
                    write_jsonl_if_changed(output / centroid_name / "destination-proposals.jsonl", filing_projection(centroid_predictions))
                run_rows.append({"model": centroid_name, "representation": dense.representation, "metrics": centroid_metrics, "filing": centroid_filing})

    rules = load_rules(rules_path)
    if rules:
        rule_name = "explicit_rules"
        rule_rows = rule_predictions(documents, rules)
        predictions_by_model[rule_name] = rule_rows
        rule_metrics = multilabel_metrics(rule_rows, labels, partitions, closed_world_single_label=filing_evaluation)
        rule_filing = filing_metrics(rule_rows, labels, partitions) if filing_evaluation else None
        write_jsonl_if_changed(output / rule_name / "proposals.jsonl", rule_rows)
        write_json_if_changed(output / rule_name / "metrics.json", rule_metrics)
        if rule_filing is not None:
            write_json_if_changed(output / rule_name / "filing.json", rule_filing)
            write_jsonl_if_changed(output / rule_name / "destination-proposals.jsonl", filing_projection(rule_rows))
        run_rows.append({"model": rule_name, "representation": "exact_regular_expression_rules", "metrics": rule_metrics, "filing": rule_filing})
        first_geometric = next((name for name, _, _ in configurations if name in predictions_by_model), None)
        if first_geometric:
            hybrid_name = f"hybrid_rules_plus_{first_geometric}"
            hybrid = hybrid_predictions(rule_rows, predictions_by_model[first_geometric])
            hybrid_metrics = multilabel_metrics(hybrid, labels, partitions, closed_world_single_label=filing_evaluation)
            hybrid_filing = filing_metrics(hybrid, labels, partitions) if filing_evaluation else None
            write_jsonl_if_changed(output / hybrid_name / "proposals.jsonl", hybrid)
            write_json_if_changed(output / hybrid_name / "metrics.json", hybrid_metrics)
            if hybrid_filing is not None:
                write_json_if_changed(output / hybrid_name / "filing.json", hybrid_filing)
                write_jsonl_if_changed(output / hybrid_name / "destination-proposals.jsonl", filing_projection(hybrid))
            run_rows.append({"model": hybrid_name, "representation": "rule_override_plus_geometric", "metrics": hybrid_metrics, "filing": hybrid_filing})

    manifest = {
        "model_revision": MODEL_REVISION,
        "created_from_configuration_sha256": sha256_bytes(
            canonical_json_bytes(
                {
                    "documents": [
                        {"corpus_id": document.corpus_id, "source_sha256": document.source_sha256}
                        for document in documents
                    ],
                    "labels": labels,
                    "partitions": partitions,
                    "views": views,
                    "rules_path": str(rules_path) if rules_path else None,
                    "rules_sha256": sha256_bytes(rules_path.read_bytes()) if rules_path else None,
                    "filing_evaluation": filing_evaluation,
                }
            )
        ),
        "documents": len(documents),
        "train_documents": len(train_indices),
        "views": views,
        "filing_evaluation": filing_evaluation,
        "software": {
            "python": platform.python_version(),
            "numpy": np.__version__,
            "scipy": scipy.__version__,
            "scikit_learn": sklearn.__version__,
        },
        "models": run_rows,
        "elapsed_seconds": time.perf_counter() - started,
    }
    write_json_if_changed(output / "manifest.json", manifest)
    return manifest
