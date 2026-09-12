from __future__ import annotations

import hashlib
import json
import tempfile
import unittest
from pathlib import Path

from ib_conversations.classify import (
    classify_geometric_model,
    compare,
    filing_projection,
    merge_incremental_proposals,
    multilabel_metrics,
    train_geometric_model,
)
from ib_conversations.corpus import Document
from ib_conversations.events import EvidenceEvent, current_labels
from ib_conversations.partitions import partition_rows
from ib_conversations.query import query_evidence, query_proposals, resolve_proposals
from ib_conversations.identity import write_jsonl_if_changed


def document(number: int, kind: str, *, branch: bool = False) -> Document:
    vocabulary = {
        "cooling": "compressor refrigerant evaporator pressure HVAC service valve",
        "algebra": "eigenvalue vector matrix proof theorem linear algebra",
        "both": "compressor refrigerant HVAC eigenvalue vector matrix boundary",
        "neither": "garden soil tomatoes rainfall planting schedule",
    }[kind]
    title = f"{kind} task {number}"
    if branch:
        title = f"Branch · {title}"
    user = f"Please reason about {vocabulary}. Unique case {number}."
    assistant = f"Response to the request about {vocabulary}."
    digest = hashlib.sha256(f"{number}:{kind}:{title}:{user}:{assistant}".encode()).hexdigest()
    return Document(
        corpus_id=f"conv-{number:03d}",
        title=title,
        created_at=float(number),
        updated_at=float(number),
        messages=(
            {"role": "user", "active_branch": True, "text": user},
            {"role": "assistant", "active_branch": True, "text": assistant},
        ),
        existing_location={"project_title": "Uncategorized"},
        repository_references=tuple(),
        redacted=False,
        source_sha256=digest,
    )


class ClassificationTest(unittest.TestCase):
    def setUp(self) -> None:
        kinds = (["cooling", "algebra", "both", "neither"] * 12)
        self.documents = [document(index, kind) for index, kind in enumerate(kinds)]
        self.partitions = {
            row.corpus_id: ("train" if index < 32 else "development" if index < 40 else "test")
            for index, row in enumerate(self.documents)
        }
        self.labels = {
            row.corpus_id: {
                "HVAC/R": kind in {"cooling", "both"},
                "mathematics": kind in {"algebra", "both"},
            }
            for row, kind in zip(self.documents, kinds)
        }

    def test_train_classify_and_incremental_reuse_are_separate_and_stable(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            model = Path(temporary) / "model"
            report = train_geometric_model(
                self.documents,
                self.labels,
                self.partitions,
                model,
                representation="sparse",
                view="user",
            )
            self.assertEqual(report["proposals_written"], 0)
            self.assertFalse((model / "proposals.jsonl").exists())
            repeated_model = Path(temporary) / "repeated-model"
            repeated_report = train_geometric_model(
                self.documents,
                self.labels,
                self.partitions,
                repeated_model,
                representation="sparse",
                view="user",
            )
            self.assertEqual(report["model_generation_id"], repeated_report["model_generation_id"])
            new_test = document(999, "neither")
            expanded_documents = self.documents + [new_test]
            expanded_labels = {**self.labels, new_test.corpus_id: {"HVAC/R": False, "mathematics": False}}
            expanded_partitions = {**self.partitions, new_test.corpus_id: "test"}
            expanded_report = train_geometric_model(
                expanded_documents,
                expanded_labels,
                expanded_partitions,
                Path(temporary) / "expanded-model",
                representation="sparse",
                view="user",
            )
            self.assertEqual(report["model_generation_id"], expanded_report["model_generation_id"])

            first = classify_geometric_model(model, self.documents)
            second = classify_geometric_model(model, self.documents)
            self.assertEqual(first, second)
            metrics = multilabel_metrics(first, self.labels, self.partitions)
            self.assertEqual(metrics["micro_f1"], 1.0)
            self.assertGreater(metrics["multi_category_conversations"], 0)

            previous = Path(temporary) / "previous.jsonl"
            write_jsonl_if_changed(previous, first)
            changed = document(47, "both")
            selected = classify_geometric_model(model, [changed])
            inventory = {row.corpus_id: row.source_sha256 for row in self.documents}
            inventory[changed.corpus_id] = changed.source_sha256
            generation = json.loads((model / "artifact.json").read_text())["model_generation_id"]
            merged, reused = merge_incremental_proposals(previous, selected, inventory, generation)
            self.assertEqual(reused, len(self.documents) - 1)
            self.assertEqual(len(merged), len(self.documents))
            self.assertEqual({row["corpus_id"] for row in merged}, set(inventory))

    def test_duplicate_partition_and_query_surface(self) -> None:
        duplicate = document(100, "cooling")
        branch = document(101, "cooling", branch=True)
        branch = Document(**{**branch.__dict__, "title": f"Branch · {duplicate.title}"})
        rows, report = partition_rows(self.documents + [duplicate, branch], self.labels)
        by_id = {row["corpus_id"]: row for row in rows}
        self.assertEqual(by_id[duplicate.corpus_id]["duplicate_group"], by_id[branch.corpus_id]["duplicate_group"])
        self.assertEqual(by_id[duplicate.corpus_id]["random_split"], by_id[branch.corpus_id]["random_split"])
        self.assertLess(report["duplicate_groups"], report["documents"])

        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "proposals.jsonl"
            rows = [
                {
                    "corpus_id": "one",
                    "categories": {
                        "A": {"accepted": True, "score": 0.01, "proposal_threshold": 0.0},
                        "B": {"accepted": True, "score": 0.02, "proposal_threshold": 0.0},
                    },
                },
                {"corpus_id": "two", "categories": {}},
            ]
            write_jsonl_if_changed(path, rows)
            self.assertEqual(query_proposals(path, operation="unclassified")[0]["corpus_id"], "two")
            self.assertEqual(query_proposals(path, operation="overlap", category="A", other_category="B")[0]["corpus_id"], "one")
            self.assertEqual(query_proposals(path, operation="boundary", category="A")[0]["distance_to_boundary"], 0.01)

    def test_filing_projection_prioritizes_exact_rules_and_can_abstain(self) -> None:
        rows = filing_projection(
            [
                {
                    "corpus_id": "exact",
                    "categories": {
                        "wrong-geometry": {"accepted": True, "ranking_priority": 0, "ranking_margin": 20.0},
                        "exact-rule": {"accepted": True, "ranking_priority": 1, "ranking_margin": 0.0},
                    },
                },
                {
                    "corpus_id": "ambiguous",
                    "categories": {
                        "one": {"accepted": True, "ranking_margin": 0.10},
                        "two": {"accepted": True, "ranking_margin": 0.08},
                    },
                },
                {"corpus_id": "none", "categories": {}},
            ]
        )
        self.assertEqual(rows[0]["proposed_destination"], "exact-rule")
        self.assertEqual(rows[1]["outcome"], "several_plausible_destinations")
        self.assertEqual(rows[2]["outcome"], "no_sufficiently_supported_destination")

        metrics = multilabel_metrics(
            [{"corpus_id": "one", "categories": {"spurious": {"accepted": True}}}],
            {"one": {"truth": True}},
            {"one": "test"},
            closed_world_single_label=True,
        )
        self.assertEqual(metrics["category"]["truth"]["f1"], 0.0)
        self.assertEqual(metrics["category"]["spurious"]["fp"], 1)

    def test_comparison_exercises_rules_sparse_dense_centroid_weighting_and_hybrid(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "comparison"
            rules = Path(temporary) / "rules.json"
            rules.write_text(json.dumps({"HVAC/R": ["refrigerant"], "mathematics": ["eigenvalue"]}))
            report = compare(
                self.documents,
                self.labels,
                self.partitions,
                output,
                views=["user", "assistant", "full", "title_user", "structural"],
                rules_path=rules,
            )
            names = {row["model"] for row in report["models"]}
            self.assertTrue(
                {
                    "explicit_rules",
                    "sparse_user",
                    "sparse_assistant",
                    "sparse_full",
                    "sparse_title_user",
                    "sparse_structural",
                    "dense_lsa_user",
                    "dense_centroid_user",
                    "weighted_user4_assistant1_title2",
                    "hybrid_rules_plus_sparse_user",
                }
                <= names
            )
            first = {
                path.relative_to(output): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in output.glob("*/proposals.jsonl")
            }
            compare(
                self.documents,
                self.labels,
                self.partitions,
                output,
                views=["user", "assistant", "full", "title_user", "structural"],
                rules_path=rules,
            )
            second = {
                path.relative_to(output): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in output.glob("*/proposals.jsonl")
            }
            self.assertEqual(first, second)


class EvidenceTest(unittest.TestCase):
    def test_correction_overrides_but_does_not_rewrite_prior_evidence(self) -> None:
        weak = EvidenceEvent(
            "event-weak",
            "conv-1",
            "concept",
            "HVAC/R",
            "positive",
            "inferred_conceptual_membership",
            "accepted_decision",
            "2026-01-01T00:00:00Z",
            "review-1",
            "initial review",
        )
        correction = EvidenceEvent(
            "event-correction",
            "conv-1",
            "concept",
            "HVAC/R",
            "negative",
            "manually_corrected_prediction",
            "manual_correction",
            "2026-01-02T00:00:00Z",
            "correction-1",
            "user removed this membership",
        )
        labels = current_labels([weak, correction], axis="concept")
        self.assertFalse(labels["conv-1"]["HVAC/R"])
        self.assertEqual(weak.polarity, "positive")
        self.assertEqual(query_evidence([weak, correction], operation="weak"), [])

        with tempfile.TemporaryDirectory() as temporary:
            proposals = Path(temporary) / "proposals.jsonl"
            write_jsonl_if_changed(
                proposals,
                [{"corpus_id": "conv-1", "categories": {"HVAC/R": {"accepted": True}}}],
            )
            resolved = resolve_proposals(proposals, [weak, correction], axis="concept")
            self.assertFalse(resolved[0]["categories"]["HVAC/R"]["accepted"])
            self.assertTrue(resolved[0]["categories"]["HVAC/R"]["classifier_accepted"])
            self.assertTrue(resolved[0]["categories"]["HVAC/R"]["authoritative_override"])


if __name__ == "__main__":
    unittest.main()
