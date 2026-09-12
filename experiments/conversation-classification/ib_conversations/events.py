"""Append-only evidence and correction events."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

from .identity import canonical_json_bytes, stable_id


AXES = {"existing_location", "concept", "filing_destination"}
POLARITIES = {"positive", "negative"}
AUTHORITIES = {
    "weak": 0,
    "classifier_proposal": 1,
    "accepted_decision": 2,
    "explicit_user_assertion": 3,
    "manual_correction": 4,
}
PROVENANCES = {
    "inherited_existing_location",
    "assistant_cleanup_proposal",
    "inferred_conceptual_membership",
    "classifier_prediction",
    "accepted_classifier_prediction",
    "explicit_user_classification",
    "manually_corrected_prediction",
}


@dataclass(frozen=True)
class EvidenceEvent:
    event_id: str
    target_id: str
    axis: str
    value: str
    polarity: str
    provenance: str
    authority: str
    asserted_at: str
    source_reference: str
    reason: str

    @classmethod
    def from_dict(cls, row: dict[str, Any]) -> "EvidenceEvent":
        event = cls(**{field: str(row.get(field, "")) for field in cls.__dataclass_fields__})
        event.validate()
        return event

    def validate(self) -> None:
        if self.axis not in AXES:
            raise ValueError(f"invalid evidence axis {self.axis!r}")
        if self.polarity not in POLARITIES:
            raise ValueError(f"invalid evidence polarity {self.polarity!r}")
        if self.authority not in AUTHORITIES:
            raise ValueError(f"invalid evidence authority {self.authority!r}")
        if self.provenance not in PROVENANCES:
            raise ValueError(f"invalid evidence provenance {self.provenance!r}")
        for name in ("event_id", "target_id", "value", "asserted_at", "source_reference"):
            if not getattr(self, name):
                raise ValueError(f"evidence event has empty {name}")
        if self.axis == "existing_location" and self.polarity == "negative":
            raise ValueError("existing location is an observation, not negative category evidence")

    def as_dict(self) -> dict[str, str]:
        return {field: getattr(self, field) for field in self.__dataclass_fields__}


def load_events(path: Path) -> list[EvidenceEvent]:
    if not path.exists():
        return []
    paths = sorted(path.glob("*.jsonl")) if path.is_dir() else [path]
    events = [
        EvidenceEvent.from_dict(json.loads(line))
        for source in paths
        for line in source.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]
    ids = [event.event_id for event in events]
    if len(ids) != len(set(ids)):
        raise ValueError("evidence log contains duplicate event ids")
    return events


def append_correction(
    path: Path,
    *,
    target_id: str,
    axis: str,
    value: str,
    polarity: str,
    asserted_at: str,
    source_reference: str,
    reason: str,
) -> EvidenceEvent:
    directory = path if path.is_dir() else None
    output_path = path / "corrections.jsonl" if directory is not None else path
    provenance = "manually_corrected_prediction"
    authority = "manual_correction"
    event_id = stable_id(
        "assertion",
        target_id,
        "\0".join((axis, value, polarity, asserted_at, source_reference, reason)),
    )
    event = EvidenceEvent(
        event_id,
        target_id,
        axis,
        value,
        polarity,
        provenance,
        authority,
        asserted_at,
        source_reference,
        reason,
    )
    event.validate()
    existing = load_events(directory or output_path)
    if any(row.event_id == event.event_id for row in existing):
        return event
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("ab") as stream:
        stream.write(canonical_json_bytes(event.as_dict()))
        stream.flush()
    return event


def current_evidence(
    events: Iterable[EvidenceEvent],
    *,
    axis: str,
    minimum_authority: str = "accepted_decision",
) -> dict[str, dict[str, EvidenceEvent]]:
    if axis not in AXES:
        raise ValueError(f"invalid evidence axis {axis!r}")
    minimum = AUTHORITIES[minimum_authority]
    selected: dict[tuple[str, str], EvidenceEvent] = {}
    for event in events:
        if event.axis != axis or AUTHORITIES[event.authority] < minimum:
            continue
        key = (event.target_id, event.value)
        prior = selected.get(key)
        if prior is None or (AUTHORITIES[event.authority], event.asserted_at, event.event_id) > (
            AUTHORITIES[prior.authority],
            prior.asserted_at,
            prior.event_id,
        ):
            selected[key] = event
    result: dict[str, dict[str, EvidenceEvent]] = {}
    for (target_id, value), event in selected.items():
        result.setdefault(target_id, {})[value] = event
    return result


def current_labels(
    events: Iterable[EvidenceEvent],
    *,
    axis: str,
    minimum_authority: str = "accepted_decision",
) -> dict[str, dict[str, bool]]:
    selected = current_evidence(events, axis=axis, minimum_authority=minimum_authority)
    return {
        target_id: {value: event.polarity == "positive" for value, event in values.items()}
        for target_id, values in selected.items()
    }
