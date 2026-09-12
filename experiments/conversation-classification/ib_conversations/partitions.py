"""Leak-resistant deterministic random and chronological corpus partitions."""

from __future__ import annotations

import hashlib
import json
import re
from dataclasses import dataclass
from typing import Iterable

import numpy as np
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.neighbors import NearestNeighbors

from .corpus import Document


BRANCH_TITLE = re.compile(r"^(?:branch\s*[·:-]\s*)+", re.IGNORECASE)
SPACE = re.compile(r"\s+")
NON_WORD = re.compile(r"[^\w]+", re.UNICODE)


class UnionFind:
    def __init__(self, values: Iterable[str]):
        self.parent = {value: value for value in values}

    def find(self, value: str) -> str:
        parent = self.parent[value]
        if parent != value:
            self.parent[value] = self.find(parent)
        return self.parent[value]

    def union(self, left: str, right: str) -> None:
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root != right_root:
            self.parent[max(left_root, right_root)] = min(left_root, right_root)


def normalize_title(value: str) -> str:
    value = BRANCH_TITLE.sub("", value.strip().lower())
    return SPACE.sub(" ", NON_WORD.sub(" ", value)).strip()


def duplicate_groups(documents: list[Document], near_duplicate_threshold: float = 0.92) -> dict[str, str]:
    ids = [document.corpus_id for document in documents]
    union = UnionFind(ids)
    title_owner: dict[str, str] = {}
    exact_owner: dict[str, str] = {}
    texts: list[str] = []
    for document in documents:
        title = normalize_title(document.title)
        if title:
            if title in title_owner:
                union.union(document.corpus_id, title_owner[title])
            else:
                title_owner[title] = document.corpus_id
        user_text = SPACE.sub(" ", document.text("user").strip().lower())
        exact = hashlib.sha256(user_text.encode("utf-8")).hexdigest() if user_text else ""
        if exact:
            if exact in exact_owner:
                union.union(document.corpus_id, exact_owner[exact])
            else:
                exact_owner[exact] = document.corpus_id
        texts.append(f"{title}\n{user_text[:20000]}")

    if len(documents) >= 2 and any(text.strip() for text in texts):
        vectors = TfidfVectorizer(analyzer="char_wb", ngram_range=(4, 6), min_df=1, max_features=50000).fit_transform(texts)
        neighbors = NearestNeighbors(metric="cosine", radius=max(0.0, 1.0 - near_duplicate_threshold), algorithm="brute")
        neighbors.fit(vectors)
        distances, indices = neighbors.radius_neighbors(vectors, return_distance=True)
        for row, (row_distances, row_indices) in enumerate(zip(distances, indices)):
            for distance, other in zip(row_distances, row_indices):
                if other > row and 1.0 - float(distance) >= near_duplicate_threshold:
                    union.union(ids[row], ids[int(other)])

    return {value: union.find(value) for value in ids}


def _hash_bucket(value: str, seed: str) -> int:
    return int(hashlib.sha256(f"{seed}\0{value}".encode("utf-8")).hexdigest()[:16], 16) % 10000


def random_partition(groups: dict[str, str], seed: str = "ib-conversation-v1") -> dict[str, str]:
    group_split: dict[str, str] = {}
    for group in sorted(set(groups.values())):
        bucket = _hash_bucket(group, seed)
        group_split[group] = "train" if bucket < 7000 else "development" if bucket < 8500 else "test"
    return {target_id: group_split[group] for target_id, group in groups.items()}


def chronological_partition(documents: list[Document], groups: dict[str, str]) -> dict[str, str]:
    times: dict[str, list[float]] = {}
    missing: set[str] = set()
    for document in documents:
        group = groups[document.corpus_id]
        if document.created_at is None:
            missing.add(group)
        else:
            times.setdefault(group, []).append(float(document.created_at))
    # Order a duplicate group by its newest member.  Using the oldest member
    # could place a later branch duplicate in training ahead of genuinely older
    # held-out conversations.
    ordered = sorted(times, key=lambda group: (max(times[group]), group))
    train_end = int(round(0.70 * len(ordered)))
    development_end = int(round(0.85 * len(ordered)))
    group_split = {
        group: "train" if position < train_end else "development" if position < development_end else "test"
        for position, group in enumerate(ordered)
    }
    for group in missing:
        group_split.setdefault(group, "missing_chronology")
    return {target_id: group_split[group] for target_id, group in groups.items()}


def partition_rows(
    documents: list[Document],
    labels: dict[str, dict[str, bool]],
    *,
    near_duplicate_threshold: float = 0.92,
    seed: str = "ib-conversation-v1",
) -> tuple[list[dict], dict]:
    groups = duplicate_groups(documents, near_duplicate_threshold)
    random_splits = random_partition(groups, seed)
    chronology = chronological_partition(documents, groups)
    rows = [
        {
            "corpus_id": document.corpus_id,
            "duplicate_group": groups[document.corpus_id],
            "random_split": random_splits[document.corpus_id],
            "chronological_split": chronology[document.corpus_id],
        }
        for document in sorted(documents, key=lambda value: value.corpus_id)
    ]
    distribution: dict[str, dict[str, int]] = {}
    chronological_label_distribution: dict[str, dict[str, int]] = {}
    for row in rows:
        for category, polarity in labels.get(row["corpus_id"], {}).items():
            if polarity:
                distribution.setdefault(category, {}).setdefault(row["random_split"], 0)
                distribution[category][row["random_split"]] += 1
                chronological_label_distribution.setdefault(category, {}).setdefault(row["chronological_split"], 0)
                chronological_label_distribution[category][row["chronological_split"]] += 1
    report = {
        "documents": len(documents),
        "duplicate_groups": len(set(groups.values())),
        "near_duplicate_threshold": near_duplicate_threshold,
        "random_seed": seed,
        "random_distribution": {
            split: sum(row["random_split"] == split for row in rows)
            for split in ("train", "development", "test")
        },
        "chronological_distribution": {
            split: sum(row["chronological_split"] == split for row in rows)
            for split in ("train", "development", "test", "missing_chronology")
        },
        "positive_label_distribution": dict(sorted(distribution.items())),
        "positive_label_distribution_chronological": dict(sorted(chronological_label_distribution.items())),
    }
    return rows, report
