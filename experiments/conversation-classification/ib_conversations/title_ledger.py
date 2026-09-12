"""Import the legacy 219-row title ledger without calling it a conversation corpus."""

from __future__ import annotations

import collections
import json
import shutil
from pathlib import Path
from typing import Any

from .identity import sha256_file, stable_id, write_json_if_changed, write_jsonl_if_changed
from .safety import conversation_exclusion_reasons, scan_text


LEDGER_REVISION = "legacy-cleanup-title-ledger-v1"


def _table_rows(text: str) -> list[dict[str, str]]:
    marker = "## Conversation ledger"
    if marker not in text:
        raise ValueError("cleanup ledger lacks the Conversation ledger section")
    lines = text.split(marker, 1)[1].splitlines()
    table = [line for line in lines if line.startswith("|")]
    if len(table) < 3:
        raise ValueError("cleanup ledger conversation table is empty")
    header = [cell.strip() for cell in table[0].strip("|").split("|")]
    expected = ["Created (UTC)", "Conversation title", "Category before", "Category after", "Status", "Ambiguity"]
    if header != expected:
        raise ValueError(f"unexpected cleanup-ledger header: {header!r}")
    rows: list[dict[str, str]] = []
    for line_number, line in enumerate(table[2:], start=3):
        cells = [cell.strip() for cell in line.strip("|").split("|")]
        if len(cells) != len(header):
            raise ValueError(f"cleanup-ledger table row {line_number} has {len(cells)} cells")
        rows.append(dict(zip(header, cells)))
    return rows


def import_title_ledger(source: Path, destination: Path) -> dict[str, Any]:
    source = source.resolve()
    destination = destination.resolve()
    text = source.read_text(encoding="utf-8")
    findings = scan_text(text)
    exclusion_reasons = conversation_exclusion_reasons([text])
    if findings or exclusion_reasons:
        kinds = sorted({finding.kind for finding in findings})
        raise ValueError(f"title ledger failed public screening: spans={kinds!r}, exclusions={exclusion_reasons!r}")

    source_sha256 = sha256_file(source)
    rows = _table_rows(text)
    index_rows: list[dict[str, Any]] = []
    evidence_rows: list[dict[str, Any]] = []
    for source_row, row in enumerate(rows, start=1):
        row_id = stable_id("title", f"cleanup-ledger:{source_sha256}", str(source_row))
        index_rows.append(
            {
                "title_row_id": row_id,
                "identity_quality": "artifact_row_only",
                "stable_conversation_id_available": False,
                "source_row": source_row,
                "created_at": row["Created (UTC)"],
                "title": row["Conversation title"],
                "existing_location": row["Category before"],
                "proposed_filing_destination": None if row["Category after"] == "Unchanged" else row["Category after"],
                "status": row["Status"],
                "ambiguity": row["Ambiguity"],
                "body_available": False,
                "source_artifact_sha256": source_sha256,
            }
        )
        evidence_rows.append(
            {
                "event_id": stable_id("evidence", row_id, f"existing-location:{row['Category before']}"),
                "target_id": row_id,
                "axis": "existing_location",
                "value": row["Category before"],
                "polarity": "positive",
                "provenance": "inherited_existing_location",
                "authority": "weak",
                "source_artifact_sha256": source_sha256,
                "source_row": source_row,
            }
        )
        if row["Category after"] != "Unchanged":
            evidence_rows.append(
                {
                    "event_id": stable_id("evidence", row_id, f"filing-proposal:{row['Category after']}"),
                    "target_id": row_id,
                    "axis": "filing_destination",
                    "value": row["Category after"],
                    "polarity": "positive",
                    "provenance": "assistant_cleanup_proposal",
                    "authority": "weak",
                    "source_artifact_sha256": source_sha256,
                    "source_row": source_row,
                }
            )

    before = collections.Counter(row["Category before"] for row in rows)
    after = collections.Counter(row["Category after"] for row in rows)
    report = {
        "schema_revision": LEDGER_REVISION,
        "source_artifact_sha256": source_sha256,
        "rows": len(rows),
        "stable_conversation_ids": 0,
        "conversation_bodies": 0,
        "weak_filing_proposals": sum(value for key, value in after.items() if key != "Unchanged"),
        "ambiguous_unchanged": after.get("Unchanged", 0),
        "existing_location_distribution": dict(sorted(before.items())),
        "proposed_destination_distribution": dict(sorted(after.items())),
        "valid_for_primary_classifier_evaluation": False,
        "limitations": [
            "titles only",
            "no stable account conversation identifiers",
            "assistant-proposed destinations are weak evidence",
            "no conceptual multilabel annotations",
            "no user/assistant text comparison",
        ],
    }
    destination.mkdir(parents=True, exist_ok=True)
    copied = destination / "source.md"
    if not copied.exists() or copied.read_bytes() != source.read_bytes():
        shutil.copyfile(source, copied)
    write_jsonl_if_changed(destination / "index.jsonl", index_rows)
    write_jsonl_if_changed(destination / "weak_evidence.jsonl", evidence_rows)
    write_json_if_changed(destination / "report.json", report)
    return report

