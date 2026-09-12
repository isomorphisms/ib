"""Shell-oriented inspection over immutable proposal generations."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from .events import AUTHORITIES, EvidenceEvent, current_evidence


def _jsonl(path: Path) -> list[dict[str, Any]]:
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def query_proposals(
    proposals: Path,
    *,
    operation: str,
    category: str | None = None,
    other_category: str | None = None,
    corpus_id: str | None = None,
    limit: int = 50,
) -> list[dict[str, Any]]:
    rows = _jsonl(proposals)
    if operation == "unclassified":
        result = [row for row in rows if not any(value.get("accepted") for value in row.get("categories", {}).values())]
    elif operation == "category":
        if not category:
            raise ValueError("category query requires --category")
        result = [row for row in rows if row.get("categories", {}).get(category, {}).get("accepted")]
    elif operation == "boundary":
        if not category:
            raise ValueError("boundary query requires --category")
        result = []
        for row in rows:
            value = row.get("categories", {}).get(category)
            if value is not None:
                result.append(
                    {
                        "corpus_id": row["corpus_id"],
                        "category": category,
                        "score": value["score"],
                        "proposal_threshold": value["proposal_threshold"],
                        "distance_to_boundary": abs(float(value["score"]) - float(value["proposal_threshold"])),
                        "accepted": value["accepted"],
                    }
                )
        result.sort(key=lambda row: (row["distance_to_boundary"], row["corpus_id"]))
    elif operation == "overlap":
        if not category or not other_category:
            raise ValueError("overlap query requires --category and --other-category")
        result = [
            row
            for row in rows
            if row.get("categories", {}).get(category, {}).get("accepted")
            and row.get("categories", {}).get(other_category, {}).get("accepted")
        ]
    elif operation == "why":
        if not corpus_id:
            raise ValueError("why query requires --corpus-id")
        result = [row for row in rows if row.get("corpus_id") == corpus_id]
    else:
        raise ValueError(f"unknown query operation {operation!r}")
    return result[:limit]


def changed_proposals(previous: Path, current: Path, limit: int = 100) -> list[dict[str, Any]]:
    previous_rows = {row["corpus_id"]: row for row in _jsonl(previous)}
    current_rows = {row["corpus_id"]: row for row in _jsonl(current)}
    changes: list[dict[str, Any]] = []
    for target_id in sorted(set(previous_rows) | set(current_rows)):
        before = {
            category
            for category, value in previous_rows.get(target_id, {}).get("categories", {}).items()
            if value.get("accepted")
        }
        after = {
            category
            for category, value in current_rows.get(target_id, {}).get("categories", {}).items()
            if value.get("accepted")
        }
        if before != after:
            changes.append(
                {
                    "corpus_id": target_id,
                    "accepted_before": sorted(before),
                    "accepted_after": sorted(after),
                    "added": sorted(after - before),
                    "removed": sorted(before - after),
                }
            )
    return changes[:limit]


def query_evidence(
    events: list[EvidenceEvent],
    *,
    operation: str,
    target_id: str | None = None,
    category: str | None = None,
    limit: int = 100,
) -> list[dict[str, Any]]:
    if operation == "weak":
        selected = [event for event in events if event.authority == "weak"]
    elif operation == "target":
        if not target_id:
            raise ValueError("target evidence query requires --target-id")
        selected = [event for event in events if event.target_id == target_id]
    elif operation == "category":
        if not category:
            raise ValueError("category evidence query requires --category")
        selected = [event for event in events if event.value == category]
    else:
        raise ValueError(f"unknown evidence query operation {operation!r}")
    selected.sort(key=lambda event: (AUTHORITIES[event.authority], event.asserted_at, event.event_id))
    return [event.as_dict() for event in selected[:limit]]


def resolve_proposals(
    proposals: Path,
    events: list[EvidenceEvent],
    *,
    axis: str,
    minimum_authority: str = "accepted_decision",
) -> list[dict[str, Any]]:
    """Overlay authoritative evidence without modifying source text or classifier output."""
    rows = _jsonl(proposals)
    authoritative = current_evidence(events, axis=axis, minimum_authority=minimum_authority)
    output: list[dict[str, Any]] = []
    for row in rows:
        resolved = json.loads(json.dumps(row))
        resolved["resolution_axis"] = axis
        resolved["classifier_categories"] = json.loads(json.dumps(row.get("categories", {})))
        categories = resolved.setdefault("categories", {})
        for category, event in authoritative.get(row["corpus_id"], {}).items():
            prior = categories.get(category, {})
            categories[category] = {
                **prior,
                "classifier_accepted": prior.get("accepted"),
                "accepted": event.polarity == "positive",
                "authoritative_override": True,
                "authority": event.authority,
                "provenance": event.provenance,
                "evidence_event_id": event.event_id,
                "reason": event.reason,
            }
        output.append(resolved)
    return output
