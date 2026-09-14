"""Deterministic filing projection over conceptual memberships.

Concept classification and ChatGPT filing are deliberately different axes.  This
module consumes accepted concept proposals plus weak existing-location evidence
and applies a versioned human-inspectable policy.  Classifier margins are kept
for explanation but are not compared across concept categories as if they were
calibrated probabilities.
"""

from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Any, Iterable

from .events import AUTHORITIES, EvidenceEvent, current_evidence
from .identity import sha256_bytes


POLICY_REVISION = "conversation-filing-policy-v1"


def load_filing_policy(path: Path) -> dict[str, Any]:
    raw = path.read_bytes()
    value = json.loads(raw)
    if not isinstance(value, dict):
        raise ValueError("filing policy must be a JSON object")
    required = {
        "policy_id",
        "version",
        "created_at",
        "provenance",
        "minimum_support",
        "ambiguity_gap",
        "destinations",
    }
    missing = required - set(value)
    if missing:
        raise ValueError(f"filing policy is missing fields: {sorted(missing)!r}")
    if not isinstance(value["policy_id"], str) or not value["policy_id"]:
        raise ValueError("filing policy policy_id must be a nonempty string")
    if not isinstance(value["version"], int) or value["version"] < 1:
        raise ValueError("filing policy version must be a positive integer")
    for field in ("created_at", "provenance"):
        if not isinstance(value[field], str) or not value[field]:
            raise ValueError(f"filing policy {field} must be a nonempty string")
    for field, allow_zero in (("minimum_support", False), ("ambiguity_gap", True)):
        number = value[field]
        if not isinstance(number, (int, float)) or not math.isfinite(float(number)):
            raise ValueError(f"filing policy {field} must be finite")
        if float(number) < 0 or (not allow_zero and float(number) == 0):
            raise ValueError(f"filing policy {field} has an invalid value")
    if not isinstance(value["destinations"], list) or not value["destinations"]:
        raise ValueError("filing policy needs at least one destination")

    seen: set[str] = set()
    for destination in value["destinations"]:
        if not isinstance(destination, dict):
            raise ValueError("each filing destination must be an object")
        expected = {
            "destination_id",
            "chatgpt_location",
            "concept_weights",
            "existing_location_weights",
        }
        if set(destination) != expected:
            raise ValueError(
                f"filing destination fields must be exactly {sorted(expected)!r}; "
                f"found {sorted(destination)!r}"
            )
        destination_id = destination["destination_id"]
        if not isinstance(destination_id, str) or not destination_id:
            raise ValueError("destination_id must be a nonempty string")
        if destination_id in seen:
            raise ValueError(f"duplicate destination_id {destination_id!r}")
        seen.add(destination_id)
        if not isinstance(destination["chatgpt_location"], str) or not destination["chatgpt_location"]:
            raise ValueError(f"destination {destination_id!r} has no ChatGPT location")
        for field in ("concept_weights", "existing_location_weights"):
            weights = destination[field]
            if not isinstance(weights, dict):
                raise ValueError(f"destination {destination_id!r} {field} must be an object")
            for key, weight in weights.items():
                if not isinstance(key, str) or not key:
                    raise ValueError(f"destination {destination_id!r} has an invalid {field} key")
                if not isinstance(weight, (int, float)) or not math.isfinite(float(weight)) or float(weight) < 0:
                    raise ValueError(f"destination {destination_id!r} has an invalid weight for {key!r}")
                if field == "concept_weights" and float(weight) == 0:
                    raise ValueError(f"destination {destination_id!r} concept weight for {key!r} must be positive")
        if not destination["concept_weights"] and not any(
            float(weight) > 0 for weight in destination["existing_location_weights"].values()
        ):
            raise ValueError(f"destination {destination_id!r} has no supporting evidence mapping")

    return {
        **value,
        "policy_revision": POLICY_REVISION,
        "policy_sha256": sha256_bytes(raw),
    }


def _accepted_concepts(row: dict[str, Any]) -> dict[str, dict[str, Any]]:
    categories = row.get("categories", {})
    if not isinstance(categories, dict):
        return {}
    return {
        str(category): evidence
        for category, evidence in categories.items()
        if isinstance(evidence, dict) and evidence.get("accepted") is True
    }


def _locations(events: Iterable[EvidenceEvent]) -> dict[str, dict[str, EvidenceEvent]]:
    return current_evidence(events, axis="existing_location", minimum_authority="weak")


def _filing_assertions(events: Iterable[EvidenceEvent]) -> dict[str, dict[str, EvidenceEvent]]:
    return current_evidence(events, axis="filing_destination", minimum_authority="explicit_user_assertion")


def _authoritative_choice(values: dict[str, EvidenceEvent]) -> tuple[str | None, list[EvidenceEvent]]:
    positives = [event for event in values.values() if event.polarity == "positive"]
    if not positives:
        return None, []
    highest_authority = max(AUTHORITIES[event.authority] for event in positives)
    strongest = [event for event in positives if AUTHORITIES[event.authority] == highest_authority]
    latest = max(event.asserted_at for event in strongest)
    latest_events = [event for event in strongest if event.asserted_at == latest]
    destinations = sorted({event.value for event in latest_events})
    if len(destinations) == 1:
        return destinations[0], latest_events
    return None, latest_events


def project_filing_destinations(
    concept_proposals: list[dict[str, Any]],
    policy: dict[str, Any],
    events: Iterable[EvidenceEvent] = (),
) -> list[dict[str, Any]]:
    """Project concepts to one filing destination while preserving abstention.

    The policy score is a deterministic support score, not a probability.  Raw
    per-concept classifier margins are retained in explanations but deliberately
    do not get added together across independently calibrated category planes.
    """

    event_rows = list(events)
    location_by_target = _locations(event_rows)
    filing_by_target = _filing_assertions(event_rows)
    minimum_support = float(policy["minimum_support"])
    ambiguity_gap = float(policy["ambiguity_gap"])
    output: list[dict[str, Any]] = []

    for row in concept_proposals:
        target_id = str(row["corpus_id"])
        base = {
            "corpus_id": target_id,
            "input_sha256": row.get("input_sha256"),
            "model_generation_id": row.get("model_generation_id"),
            "filing_policy_id": policy["policy_id"],
            "filing_policy_version": policy["version"],
            "filing_policy_sha256": policy["policy_sha256"],
            "score_meaning": "sum of explicit filing-policy evidence weights; not a calibrated probability",
        }
        filing_values = filing_by_target.get(target_id, {})
        authoritative, strongest_events = _authoritative_choice(filing_values)
        if authoritative is not None:
            output.append(
                {
                    **base,
                    "outcome": "confident_destination",
                    "proposed_destination": authoritative,
                    "authority_override": True,
                    "evidence": [event.as_dict() for event in strongest_events],
                    "candidates": [],
                }
            )
            continue
        if strongest_events:
            output.append(
                {
                    **base,
                    "outcome": "several_plausible_destinations",
                    "authority_override": True,
                    "reason": "conflicting equally authoritative filing assertions",
                    "candidates": [
                        {
                            "destination_id": event.value,
                            "authority": event.authority,
                            "asserted_at": event.asserted_at,
                            "source_reference": event.source_reference,
                        }
                        for event in sorted(strongest_events, key=lambda item: (item.value, item.event_id))
                    ],
                }
            )
            continue

        blocked_destinations = {
            value for value, event in filing_values.items() if event.polarity == "negative"
        }
        accepted = _accepted_concepts(row)
        locations = location_by_target.get(target_id, {})
        candidates: list[dict[str, Any]] = []
        for destination in policy["destinations"]:
            destination_id = destination["destination_id"]
            if destination_id in blocked_destinations:
                continue
            support: list[dict[str, Any]] = []
            score = 0.0
            for concept, weight in destination["concept_weights"].items():
                evidence = accepted.get(concept)
                if evidence is None:
                    continue
                numeric_weight = float(weight)
                score += numeric_weight
                support.append(
                    {
                        "kind": "concept",
                        "value": concept,
                        "weight": numeric_weight,
                        "classifier_margin": evidence.get("ranking_margin"),
                        "vote_fraction": evidence.get("vote_fraction"),
                        "rule_override": bool(evidence.get("rule_override", False)),
                    }
                )
            for location, event in locations.items():
                weight = destination["existing_location_weights"].get(location)
                if weight is None or float(weight) == 0.0:
                    continue
                numeric_weight = float(weight)
                score += numeric_weight
                support.append(
                    {
                        "kind": "existing_location",
                        "value": location,
                        "weight": numeric_weight,
                        "provenance": event.provenance,
                        "authority": event.authority,
                    }
                )
            if score > 0.0:
                candidates.append(
                    {
                        "destination_id": destination_id,
                        "chatgpt_location": destination["chatgpt_location"],
                        "policy_support_score": score,
                        "support": support,
                    }
                )

        candidates.sort(
            key=lambda candidate: (-candidate["policy_support_score"], candidate["destination_id"])
        )
        if not candidates or candidates[0]["policy_support_score"] < minimum_support:
            output.append(
                {
                    **base,
                    "outcome": "no_sufficiently_supported_destination",
                    "authority_override": False,
                    "candidates": candidates[:5],
                    "minimum_support": minimum_support,
                }
            )
            continue

        gap = (
            math.inf
            if len(candidates) == 1
            else candidates[0]["policy_support_score"] - candidates[1]["policy_support_score"]
        )
        if len(candidates) > 1 and gap < ambiguity_gap:
            output.append(
                {
                    **base,
                    "outcome": "several_plausible_destinations",
                    "authority_override": False,
                    "candidates": candidates[:5],
                    "top_policy_support_gap": gap,
                    "ambiguity_gap": ambiguity_gap,
                }
            )
            continue

        output.append(
            {
                **base,
                "outcome": "confident_destination",
                "proposed_destination": candidates[0]["destination_id"],
                "chatgpt_location": candidates[0]["chatgpt_location"],
                "authority_override": False,
                "evidence": candidates[0],
                "alternatives": candidates[1:5],
                "top_policy_support_gap": gap,
            }
        )

    return output
