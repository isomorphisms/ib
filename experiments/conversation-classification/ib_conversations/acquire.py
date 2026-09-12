"""One-time bulk ChatGPT-export acquisition into the public experiment corpus."""

from __future__ import annotations

import json
import collections
import time
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

from . import SCANNER_REVISION, SCHEMA_REVISION
from .identity import (
    canonical_json_bytes,
    sha256_bytes,
    stable_id,
    write_bytes_if_changed,
    write_json_if_changed,
    write_jsonl_if_changed,
)
from .safety import conversation_exclusion_reasons, redact_text, scan_text


ALLOWED_ROLES = {"user", "assistant"}
INTERNAL_ROLES = {"system", "developer", "tool"}


@dataclass(frozen=True)
class ImportResult:
    source_conversations: int
    retained_conversations: int
    excluded_conversations: int
    redactions: int
    reused_conversations: int
    written_files: int
    elapsed_seconds: float
    blocking_manual_review: bool


def _read_export(path: Path) -> tuple[list[dict[str, Any]], str]:
    if path.suffix.lower() == ".zip":
        with zipfile.ZipFile(path) as archive:
            names = [name for name in archive.namelist() if Path(name).name == "conversations.json"]
            if len(names) != 1:
                raise ValueError("export ZIP must contain exactly one conversations.json")
            info = archive.getinfo(names[0])
            if info.file_size > 2 * 1024 * 1024 * 1024:
                raise ValueError("conversations.json exceeds the 2 GiB experiment limit")
            raw = archive.read(info)
    else:
        raw = path.read_bytes()
    value = json.loads(raw)
    if not isinstance(value, list) or not all(isinstance(row, dict) for row in value):
        raise ValueError("ChatGPT export must be a JSON array of conversation objects")
    return value, sha256_bytes(raw)


def _message_text(message: dict[str, Any]) -> tuple[str, list[str]]:
    content = message.get("content") or {}
    parts = content.get("parts") if isinstance(content, dict) else None
    omitted: list[str] = []
    texts: list[str] = []
    if not isinstance(parts, list):
        return "", ["missing_or_nonlist_parts"]
    for part in parts:
        if isinstance(part, str):
            texts.append(part)
        elif isinstance(part, dict) and isinstance(part.get("text"), str):
            texts.append(part["text"])
            omitted.append("structured_part_metadata")
        else:
            omitted.append(type(part).__name__)
    return "\n".join(texts), sorted(set(omitted))


def _active_nodes(conversation: dict[str, Any]) -> set[str]:
    mapping = conversation.get("mapping")
    current = conversation.get("current_node")
    if not isinstance(mapping, dict) or not isinstance(current, str):
        return set()
    active: set[str] = set()
    cursor: str | None = current
    while cursor and cursor not in active:
        active.add(cursor)
        row = mapping.get(cursor)
        cursor = row.get("parent") if isinstance(row, dict) and isinstance(row.get("parent"), str) else None
    return active


def _messages(conversation: dict[str, Any]) -> list[dict[str, Any]]:
    mapping = conversation.get("mapping")
    if not isinstance(mapping, dict):
        return []
    active = _active_nodes(conversation)
    rows: list[dict[str, Any]] = []
    for input_order, (node_id, node) in enumerate(mapping.items()):
        if not isinstance(node, dict) or not isinstance(node.get("message"), dict):
            continue
        message = node["message"]
        author = message.get("author") if isinstance(message.get("author"), dict) else {}
        role = author.get("role") if isinstance(author.get("role"), str) else "unknown"
        text, omitted = _message_text(message)
        message_id = str(message.get("id") or node_id)
        rows.append(
            {
                "source_message_id": message_id,
                "source_node_id": str(node_id),
                "role": role,
                "created_at": message.get("create_time"),
                "updated_at": message.get("update_time"),
                "parent_node_id": node.get("parent") if isinstance(node.get("parent"), str) else None,
                "children_node_ids": [str(value) for value in node.get("children", []) if isinstance(value, str)],
                "active_branch": str(node_id) in active,
                "input_order": input_order,
                "text": text,
                "omitted_part_kinds": omitted,
            }
        )
    return sorted(
        rows,
        key=lambda row: (
            row["created_at"] is None,
            row["created_at"] if row["created_at"] is not None else 0,
            row["input_order"],
            row["source_node_id"],
        ),
    )


def _conversation_source_id(conversation: dict[str, Any]) -> str:
    for key in ("conversation_id", "id"):
        value = conversation.get(key)
        if isinstance(value, str) and value:
            return value
    raise ValueError("conversation lacks a stable conversation_id/id")


def _known_location(conversation: dict[str, Any]) -> dict[str, Any]:
    fields = ("project_id", "project_title", "gizmo_id", "conversation_template_id", "is_archived", "is_starred")
    return {key: conversation[key] for key in fields if key in conversation and conversation[key] is not None}


def _repository_references(text: str) -> list[str]:
    import re

    references = set(
        match.group(0).rstrip(".,;:!?)]")
        for match in re.finditer(r"https?://github\.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+(?:/(?:issues|pull)/\d+)?", text)
    )
    references.update(
        match.group(0)
        for match in re.finditer(r"(?<![A-Za-z0-9_.-])[A-Za-z][A-Za-z0-9_.çÇ-]{1,40}\s+#\d+\b", text)
    )
    return sorted(references)


def _render_markdown(metadata: dict[str, Any], messages: list[dict[str, Any]]) -> str:
    lines = [
        f"# {metadata['title']}",
        "",
        f"- corpus_id: `{metadata['corpus_id']}`",
        f"- source: `{metadata['source_kind']}`",
        f"- source_conversation_id: `{metadata['source_conversation_id']}`",
        f"- created_at: `{metadata.get('created_at')}`",
        f"- updated_at: `{metadata.get('updated_at')}`",
        "",
    ]
    for row in messages:
        lines.extend(
            [
                f"## message `{row['source_message_id']}` — {row['role']}",
                "",
                f"- created_at: `{row.get('created_at')}`",
                f"- parent_node_id: `{row.get('parent_node_id')}`",
                f"- active_branch: `{str(row['active_branch']).lower()}`",
                "",
                row["text"],
                "",
            ]
        )
    return "\n".join(lines).rstrip() + "\n"


def _raw_source(metadata: dict[str, Any], messages: list[dict[str, Any]]) -> dict[str, Any]:
    return {
        "schema_revision": metadata["schema_revision"],
        "corpus_id": metadata["corpus_id"],
        "source_kind": metadata["source_kind"],
        "source_conversation_id": metadata["source_conversation_id"],
        "title": metadata["title"],
        "created_at": metadata.get("created_at"),
        "updated_at": metadata.get("updated_at"),
        "current_node_id": metadata.get("current_node_id"),
        "messages": messages,
    }


def _load_existing_metadata(directory: Path) -> dict[str, dict[str, Any]]:
    result: dict[str, dict[str, Any]] = {}
    if not directory.exists():
        return result
    for path in directory.glob("conv-*.json"):
        row = json.loads(path.read_text(encoding="utf-8"))
        if isinstance(row, dict) and isinstance(row.get("corpus_id"), str):
            result[row["corpus_id"]] = row
    return result


def _load_jsonl(path: Path) -> list[dict[str, Any]]:
    if not path.exists():
        return []
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def _location_label(metadata: dict[str, Any]) -> str:
    location = metadata.get("existing_location", {})
    for field in ("project_title", "project_id", "gizmo_id", "conversation_template_id"):
        if location.get(field):
            return f"{field}:{location[field]}"
    return "unprojected_or_unknown"


def _distribution(metadata_rows: Iterable[dict[str, Any]]) -> dict[str, Any]:
    rows = list(metadata_rows)
    length_buckets: collections.Counter[str] = collections.Counter()
    for row in rows:
        characters = int(row.get("user_character_count", 0)) + int(row.get("assistant_character_count", 0))
        bucket = "short:<2k" if characters < 2000 else "medium:2k-20k" if characters < 20000 else "long:20k-100k" if characters < 100000 else "very_long:>=100k"
        length_buckets[bucket] += 1
    chronology = [float(row["created_at"]) for row in rows if isinstance(row.get("created_at"), (int, float))]
    return {
        "conversations": len(rows),
        "source_location_distribution": dict(sorted(collections.Counter(_location_label(row) for row in rows).items())),
        "length_distribution": dict(sorted(length_buckets.items())),
        "repository_referencing_conversations": sum(bool(row.get("repository_references")) for row in rows),
        "redacted_conversations": sum(bool(row.get("redacted")) for row in rows),
        "created_at_minimum_epoch": min(chronology) if chronology else None,
        "created_at_maximum_epoch": max(chronology) if chronology else None,
    }


def import_export(source: Path, corpus: Path, acquired_at: str, source_label: str) -> ImportResult:
    started = time.perf_counter()
    source = source.resolve()
    corpus = corpus.resolve()
    if not source.is_file():
        raise ValueError(f"source does not exist: {source}")
    if corpus == source.parent or corpus in source.parents:
        raise ValueError("unfiltered export must remain outside the corpus directory")

    conversations, source_artifact_sha256 = _read_export(source)
    existing = _load_existing_metadata(corpus / "metadata")
    previous_redactions = _load_jsonl(corpus / "safety" / "redactions.jsonl")
    previous_exclusions = _load_jsonl(corpus / "safety" / "exclusions.jsonl")
    previous_message_exclusions = _load_jsonl(corpus / "safety" / "message-exclusions.jsonl")
    previous_source_evidence = _load_jsonl(corpus / "assertions" / "source-evidence.jsonl")
    redactions_by_id: dict[str, list[dict[str, Any]]] = {}
    for row in previous_redactions:
        redactions_by_id.setdefault(str(row.get("corpus_id")), []).append(row)
    exclusions_by_id = {str(row.get("corpus_id")): row for row in previous_exclusions}
    message_exclusions_by_id: dict[str, list[dict[str, Any]]] = {}
    for row in previous_message_exclusions:
        message_exclusions_by_id.setdefault(str(row.get("corpus_id")), []).append(row)

    current_metadata: dict[str, dict[str, Any]] = dict(existing)
    current_redactions: dict[str, list[dict[str, Any]]] = dict(redactions_by_id)
    current_exclusions: dict[str, dict[str, Any]] = dict(exclusions_by_id)
    current_message_exclusions: dict[str, list[dict[str, Any]]] = dict(message_exclusions_by_id)
    current_source_evidence = {str(row.get("event_id")): row for row in previous_source_evidence}
    retained = excluded = reused = redaction_count = written = 0

    for conversation in conversations:
        source_id = _conversation_source_id(conversation)
        corpus_id = stable_id("conv", "chatgpt-export", source_id)
        source_record_sha256 = sha256_bytes(canonical_json_bytes(conversation))
        prior = existing.get(corpus_id)
        prior_exclusion = exclusions_by_id.get(corpus_id)
        if (
            prior
            and prior.get("source_record_sha256") == source_record_sha256
            and prior.get("scanner_revision") == SCANNER_REVISION
            and prior.get("schema_revision") == SCHEMA_REVISION
            and (corpus / "raw" / f"{corpus_id}.json").exists()
        ):
            retained += 1
            reused += 1
            redaction_count += int(prior.get("redaction_count", 0))
            continue
        if (
            prior_exclusion
            and prior_exclusion.get("source_record_sha256") == source_record_sha256
            and prior_exclusion.get("scanner_revision") == SCANNER_REVISION
            and prior_exclusion.get("schema_revision") == SCHEMA_REVISION
        ):
            excluded += 1
            reused += 1
            continue

        title = conversation.get("title") if isinstance(conversation.get("title"), str) else ""
        messages = _messages(conversation)
        source_texts = [title] + [row["text"] for row in messages if row["role"] in ALLOWED_ROLES]
        exclusion_reasons = conversation_exclusion_reasons(source_texts)
        if exclusion_reasons:
            current_exclusions[corpus_id] = {
                "corpus_id": corpus_id,
                "schema_revision": SCHEMA_REVISION,
                "source_id_sha256": sha256_bytes(source_id.encode("utf-8")),
                "title_sha256": sha256_bytes(title.encode("utf-8")),
                "source_record_sha256": source_record_sha256,
                "reasons": exclusion_reasons,
                "scanner_revision": SCANNER_REVISION,
            }
            current_metadata.pop(corpus_id, None)
            current_redactions.pop(corpus_id, None)
            current_message_exclusions.pop(corpus_id, None)
            for relative in (
                Path("raw") / f"{corpus_id}.json",
                Path("metadata") / f"{corpus_id}.json",
                Path("views") / "source" / f"{corpus_id}.md",
            ):
                (corpus / relative).unlink(missing_ok=True)
            excluded += 1
            continue

        title_findings = scan_text(title)
        screened_title = redact_text(title, title_findings)
        receipts = [
            finding.receipt(corpus_id=corpus_id, message_id="title", text=title)
            for finding in title_findings
        ]
        screened_location: dict[str, Any] = {}
        for key, value in _known_location(conversation).items():
            if isinstance(value, str):
                findings = scan_text(value)
                receipts.extend(
                    finding.receipt(corpus_id=corpus_id, message_id=f"metadata:{key}", text=value)
                    for finding in findings
                )
                screened_location[key] = redact_text(value, findings)
            else:
                screened_location[key] = value
        screened_messages: list[dict[str, Any]] = []
        internal_role_count = 0
        message_exclusion_receipts: list[dict[str, Any]] = []
        for row in messages:
            if row["role"] not in ALLOWED_ROLES:
                internal_role_count += 1
                message_exclusion_receipts.append(
                    {
                        "corpus_id": corpus_id,
                        "message_id": row["source_message_id"],
                        "role": row["role"],
                        "reason": "internal_or_unknown_role_outside_public_model_input_boundary",
                        "original_characters": len(row["text"]),
                        "original_sha256": sha256_bytes(row["text"].encode("utf-8")),
                        "scanner_revision": SCANNER_REVISION,
                    }
                )
                continue
            findings = scan_text(row["text"])
            receipts.extend(
                finding.receipt(corpus_id=corpus_id, message_id=row["source_message_id"], text=row["text"])
                for finding in findings
            )
            screened = dict(row)
            screened["text"] = redact_text(row["text"], findings)
            screened_messages.append(screened)

        joined_text = "\n".join(row["text"] for row in screened_messages)
        user_character_count = sum(len(row["text"]) for row in screened_messages if row["role"] == "user")
        assistant_character_count = sum(len(row["text"]) for row in screened_messages if row["role"] == "assistant")
        metadata = {
            "schema_revision": SCHEMA_REVISION,
            "scanner_revision": SCANNER_REVISION,
            "corpus_id": corpus_id,
            "source_kind": "chatgpt_export",
            "source_conversation_id": source_id,
            "source_record_sha256": source_record_sha256,
            "source_label_at_first_import": source_label,
            "acquired_at_first_import": acquired_at,
            "title": screened_title,
            "created_at": conversation.get("create_time"),
            "updated_at": conversation.get("update_time"),
            "current_node_id": conversation.get("current_node") if isinstance(conversation.get("current_node"), str) else None,
            "existing_location": screened_location,
            "message_count": len(screened_messages),
            "user_character_count": user_character_count,
            "assistant_character_count": assistant_character_count,
            "internal_or_unknown_messages_excluded": internal_role_count,
            "message_graph": [
                {
                    key: row[key]
                    for key in (
                        "source_message_id",
                        "source_node_id",
                        "role",
                        "created_at",
                        "updated_at",
                        "parent_node_id",
                        "children_node_ids",
                        "active_branch",
                        "input_order",
                        "omitted_part_kinds",
                    )
                }
                for row in screened_messages
            ],
            "repository_references": _repository_references(joined_text),
            "redacted": bool(receipts),
            "redaction_count": len(receipts),
        }
        raw_bytes = canonical_json_bytes(_raw_source(metadata, screened_messages))
        metadata["raw_sha256"] = sha256_bytes(raw_bytes)
        source_view = _render_markdown(metadata, screened_messages)
        metadata["source_view_sha256"] = sha256_bytes(source_view.encode("utf-8"))
        written += int(write_bytes_if_changed(corpus / "raw" / f"{corpus_id}.json", raw_bytes))
        written += int(
            write_bytes_if_changed(
                corpus / "views" / "source" / f"{corpus_id}.md",
                source_view.encode("utf-8"),
            )
        )
        written += int(write_json_if_changed(corpus / "metadata" / f"{corpus_id}.json", metadata))
        current_metadata[corpus_id] = metadata
        current_redactions[corpus_id] = receipts
        current_message_exclusions[corpus_id] = message_exclusion_receipts
        current_exclusions.pop(corpus_id, None)
        location_value = _location_label(metadata)
        location_event_id = stable_id(
            "evidence",
            corpus_id,
            f"existing-location:{location_value}:{source_record_sha256}",
        )
        current_source_evidence[location_event_id] = {
            "event_id": location_event_id,
            "target_id": corpus_id,
            "axis": "existing_location",
            "value": location_value,
            "polarity": "positive",
            "provenance": "inherited_existing_location",
            "authority": "weak",
            "asserted_at": acquired_at,
            "source_reference": f"metadata/{corpus_id}.json",
            "reason": "Location observed in the bulk export; useful evidence but not conceptual truth.",
        }
        retained += 1
        redaction_count += len(receipts)

    index_rows = [
        {
            "corpus_id": row["corpus_id"],
            "title": row["title"],
            "created_at": row.get("created_at"),
            "updated_at": row.get("updated_at"),
            "message_count": row["message_count"],
            "existing_location": row["existing_location"],
            "redacted": row["redacted"],
            "raw_sha256": row["raw_sha256"],
            "metadata_path": f"metadata/{row['corpus_id']}.json",
            "raw_path": f"raw/{row['corpus_id']}.json",
            "source_view_path": f"views/source/{row['corpus_id']}.md",
        }
        for row in sorted(current_metadata.values(), key=lambda item: item["corpus_id"])
    ]
    redaction_rows = [row for corpus_id in sorted(current_redactions) for row in current_redactions[corpus_id]]
    exclusion_rows = [current_exclusions[key] for key in sorted(current_exclusions)]
    message_exclusion_rows = [
        row
        for corpus_id in sorted(current_message_exclusions)
        for row in current_message_exclusions[corpus_id]
    ]
    retained_ids = set(current_metadata)
    source_evidence_rows = [
        row
        for _, row in sorted(current_source_evidence.items())
        if row.get("target_id") in retained_ids
    ]
    written += int(write_jsonl_if_changed(corpus / "index.jsonl", index_rows))
    written += int(write_jsonl_if_changed(corpus / "safety" / "redactions.jsonl", redaction_rows))
    written += int(write_jsonl_if_changed(corpus / "safety" / "exclusions.jsonl", exclusion_rows))
    written += int(
        write_jsonl_if_changed(
            corpus / "safety" / "message-exclusions.jsonl",
            message_exclusion_rows,
        )
    )
    written += int(
        write_jsonl_if_changed(
            corpus / "assertions" / "source-evidence.jsonl",
            source_evidence_rows,
        )
    )
    blocking = False
    report = {
        "schema_revision": SCHEMA_REVISION,
        "scanner_revision": SCANNER_REVISION,
        "source_artifact_sha256": source_artifact_sha256,
        "source_label": source_label,
        "acquired_at": acquired_at,
        "source_conversations": len(conversations),
        "retained_conversations_this_run": retained,
        "excluded_conversations_this_run": excluded,
        "corpus_conversations_after_run": len(index_rows),
        "redactions_this_run": redaction_count,
        "total_redaction_receipts": len(redaction_rows),
        "total_exclusion_receipts": len(exclusion_rows),
        "total_message_exclusion_receipts": len(message_exclusion_rows),
        "reused_conversations": reused,
        "blocking_manual_review": blocking,
    }
    written += int(write_json_if_changed(corpus / "safety" / "report.json", report))
    written += int(write_json_if_changed(corpus / "reports" / "acquisition-distribution.json", _distribution(current_metadata.values())))
    return ImportResult(
        len(conversations),
        retained,
        excluded,
        redaction_count,
        reused,
        written,
        time.perf_counter() - started,
        blocking,
    )


def verify_public(corpus: Path) -> dict[str, Any]:
    corpus = corpus.resolve()
    unexpected: list[dict[str, Any]] = []
    scanned_paths = sorted(
        path
        for path in corpus.rglob("*")
        if path.is_file() and path.suffix.lower() in {".json", ".jsonl", ".md"}
    )
    for path in scanned_paths:
        text = path.read_text(encoding="utf-8")
        for finding in scan_text(text):
            unexpected.append(
                {
                    "path": str(path.relative_to(corpus)),
                    "kind": finding.kind,
                    "start": finding.start,
                    "end": finding.end,
                    "detector": finding.detector,
                }
            )

    integrity_errors: list[str] = []
    index_path = corpus / "index.jsonl"
    index_rows = _load_jsonl(index_path)
    expected_raw = {Path(row["raw_path"]).name for row in index_rows}
    expected_metadata = {Path(row["metadata_path"]).name for row in index_rows}
    expected_views = {Path(row["source_view_path"]).name for row in index_rows}
    actual_raw = {path.name for path in (corpus / "raw").glob("conv-*.json")} if (corpus / "raw").exists() else set()
    actual_metadata = {path.name for path in (corpus / "metadata").glob("conv-*.json")} if (corpus / "metadata").exists() else set()
    actual_views = {path.name for path in (corpus / "views" / "source").glob("conv-*.md")} if (corpus / "views" / "source").exists() else set()
    for label, expected, actual in (
        ("raw", expected_raw, actual_raw),
        ("metadata", expected_metadata, actual_metadata),
        ("source views", expected_views, actual_views),
    ):
        if expected != actual:
            integrity_errors.append(
                f"{label} inventory mismatch: missing={sorted(expected - actual)!r}, orphaned={sorted(actual - expected)!r}"
            )
    for row in index_rows:
        raw_path = corpus / row["raw_path"]
        metadata_path = corpus / row["metadata_path"]
        if not raw_path.exists() or not metadata_path.exists():
            continue
        if sha256_bytes(raw_path.read_bytes()) != row["raw_sha256"]:
            integrity_errors.append(f"raw hash mismatch for {row['corpus_id']}")
        metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        if metadata.get("corpus_id") != row["corpus_id"] or metadata.get("raw_sha256") != row["raw_sha256"]:
            integrity_errors.append(f"metadata/index identity mismatch for {row['corpus_id']}")
    return {
        "scanner_revision": SCANNER_REVISION,
        "files_scanned": len(scanned_paths),
        "unexpected_sensitive_spans": unexpected,
        "integrity_errors": integrity_errors,
        "pass": not unexpected and not integrity_errors,
    }
