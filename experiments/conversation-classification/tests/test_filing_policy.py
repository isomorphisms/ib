from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from ib_conversations.events import EvidenceEvent
from ib_conversations.filing_policy import load_filing_policy, project_filing_destinations


def policy_value() -> dict:
    return {
        "policy_id": "fixture-policy",
        "version": 1,
        "created_at": "2026-09-14T13:00:00Z",
        "provenance": "test fixture",
        "minimum_support": 1.0,
        "ambiguity_gap": 0.25,
        "destinations": [
            {
                "destination_id": "idric-project",
                "chatgpt_location": "Idriç",
                "concept_weights": {"compiler_backend": 2.0, "arm": 0.5},
                "existing_location_weights": {"project_title:Idriç": 0.25},
            },
            {
                "destination_id": "ib-project",
                "chatgpt_location": "IB",
                "concept_weights": {"ib_architecture": 2.0, "browser_ui": 0.5},
                "existing_location_weights": {"project_title:IB": 0.25},
            },
        ],
    }


def event(
    event_id: str,
    target_id: str,
    axis: str,
    value: str,
    polarity: str,
    authority: str,
    asserted_at: str,
) -> EvidenceEvent:
    provenance = (
        "inherited_existing_location"
        if axis == "existing_location"
        else "manually_corrected_prediction"
    )
    return EvidenceEvent(
        event_id,
        target_id,
        axis,
        value,
        polarity,
        provenance,
        authority,
        asserted_at,
        "fixture",
        "fixture evidence",
    )


class FilingPolicyTest(unittest.TestCase):
    def load_fixture(self) -> dict:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "policy.json"
            path.write_text(json.dumps(policy_value()), encoding="utf-8")
            return load_filing_policy(path)

    def test_concepts_and_destinations_are_not_the_same_axis(self) -> None:
        policy = self.load_fixture()
        rows = project_filing_destinations(
            [
                {
                    "corpus_id": "one",
                    "categories": {
                        "compiler_backend": {
                            "accepted": True,
                            "ranking_margin": 0.01,
                            "vote_fraction": 1.0,
                        }
                    },
                }
            ],
            policy,
        )
        self.assertEqual(rows[0]["proposed_destination"], "idric-project")
        self.assertNotEqual(rows[0]["proposed_destination"], "compiler_backend")
        self.assertEqual(rows[0]["chatgpt_location"], "Idriç")
        self.assertEqual(rows[0]["evidence"]["policy_support_score"], 2.0)

    def test_classifier_margin_is_explanation_not_cross_category_weight(self) -> None:
        policy = self.load_fixture()
        rows = project_filing_destinations(
            [
                {
                    "corpus_id": "one",
                    "categories": {
                        "compiler_backend": {"accepted": True, "ranking_margin": 0.0001},
                        "browser_ui": {"accepted": True, "ranking_margin": 1000.0},
                    },
                }
            ],
            policy,
        )
        self.assertEqual(rows[0]["proposed_destination"], "idric-project")
        self.assertEqual(rows[0]["evidence"]["policy_support_score"], 2.0)
        self.assertEqual(rows[0]["alternatives"][0]["policy_support_score"], 0.5)

    def test_projection_can_abstain_or_return_several_destinations(self) -> None:
        policy = self.load_fixture()
        rows = project_filing_destinations(
            [
                {
                    "corpus_id": "ambiguous",
                    "categories": {
                        "compiler_backend": {"accepted": True},
                        "ib_architecture": {"accepted": True},
                    },
                },
                {
                    "corpus_id": "weak",
                    "categories": {"arm": {"accepted": True}},
                },
                {"corpus_id": "none", "categories": {}},
            ],
            policy,
        )
        self.assertEqual(rows[0]["outcome"], "several_plausible_destinations")
        self.assertEqual(rows[1]["outcome"], "no_sufficiently_supported_destination")
        self.assertEqual(rows[2]["outcome"], "no_sufficiently_supported_destination")

    def test_existing_location_is_weak_support_not_ground_truth(self) -> None:
        policy = self.load_fixture()
        rows = project_filing_destinations(
            [{"corpus_id": "one", "categories": {}}],
            policy,
            [
                event(
                    "location",
                    "one",
                    "existing_location",
                    "project_title:Idriç",
                    "positive",
                    "weak",
                    "2026-09-14T13:00:00Z",
                )
            ],
        )
        self.assertEqual(rows[0]["outcome"], "no_sufficiently_supported_destination")
        self.assertEqual(rows[0]["candidates"][0]["policy_support_score"], 0.25)

    def test_manual_filing_correction_is_authoritative(self) -> None:
        policy = self.load_fixture()
        rows = project_filing_destinations(
            [
                {
                    "corpus_id": "one",
                    "categories": {"compiler_backend": {"accepted": True}},
                }
            ],
            policy,
            [
                event(
                    "correction",
                    "one",
                    "filing_destination",
                    "blackball-project",
                    "positive",
                    "manual_correction",
                    "2026-09-14T13:01:00Z",
                )
            ],
        )
        self.assertEqual(rows[0]["outcome"], "confident_destination")
        self.assertEqual(rows[0]["proposed_destination"], "blackball-project")
        self.assertTrue(rows[0]["authority_override"])

    def test_negative_correction_blocks_policy_destination(self) -> None:
        policy = self.load_fixture()
        rows = project_filing_destinations(
            [
                {
                    "corpus_id": "one",
                    "categories": {"compiler_backend": {"accepted": True}},
                }
            ],
            policy,
            [
                event(
                    "negative",
                    "one",
                    "filing_destination",
                    "idric-project",
                    "negative",
                    "manual_correction",
                    "2026-09-14T13:01:00Z",
                )
            ],
        )
        self.assertEqual(rows[0]["outcome"], "no_sufficiently_supported_destination")


if __name__ == "__main__":
    unittest.main()
