"""One-time bulk ChatGPT-export acquisition into the public experiment corpus."""

from __future__ import annotations

import json
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
    redactions_by_id: dict[str, list[dict[str, Any]]] = {}
    for row in previous_redactions:
        redactions_by_id.setdefault(str(row.get("corpus_id")), []).append(row)
    exclusions_by_id = {str(row.get("corpus_id")): row for row in previous_exclusions}

    current_metadata: dict[str, dict[str, Any]] = dict(existing)
    current_redactions: dict[str, list[dict[str, Any]]] = dict(redactions_by_id)
    current_exclusions: dict[str, dict[str, Any]] = dict(exclusions_by_id)
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
            and (corpus / "raw" / f"{corpus_id}.md").exists()
        ):
            retained += 1
            reused += 1
            redaction_count += int(prior.get("redaction_count", 0))
            continue
        if (
            prior_exclusion
            and prior_exclusion.get("source_record_sha256") == source_record_sha256
            and prior_exclusion.get("scanner_revision") == SCANNER_REVISION
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
                "source_id_sha256": sha256_bytes(source_id.encode("utf-8")),
                "title_sha256": sha256_bytes(title.encode("utf-8")),
                "source_record_sha256": source_record_sha256,
                "reasons": exclusion_reasons,
                "scanner_revision": SCANNER_REVISION,
            }
            current_metadata.pop(corpus_id, None)
            current_redactions.pop(corpus_id, None)
            excluded += 1
            continue

        title_findings = scan_text(title)
        screened_title = redact_text(title, title_findings)
        receipts = [
            finding.receipt(corpus_id=corpus_id, message_id="title", text=title)
            for finding in title_findings
        ]
        screened_messages: list[dict[str, Any]] = []
        internal_role_count = 0
        for row in messages:
            if row["role"] not in ALLOWED_ROLES:
                internal_role_count += 1
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
            "existing_location": _known_location(conversation),
            "message_count": len(screened_messages),
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
        raw_text = _render_markdown(metadata, screened_messages)
        metadata["raw_sha256"] = sha256_bytes(raw_text.encode("utf-8"))
        written += int(write_bytes_if_changed(corpus / "raw" / f"{corpus_id}.md", raw_text.encode("utf-8")))
        written += int(write_json_if_changed(corpus / "metadata" / f"{corpus_id}.json", metadata))
        current_metadata[corpus_id] = metadata
        current_redactions[corpus_id] = receipts
        current_exclusions.pop(corpus_id, None)
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
            "raw_path": f"raw/{row['corpus_id']}.md",
        }
        for row in sorted(current_metadata.values(), key=lambda item: item["corpus_id"])
    ]
    redaction_rows = [row for corpus_id in sorted(current_redactions) for row in current_redactions[corpus_id]]
    exclusion_rows = [current_exclusions[key] for key in sorted(current_exclusions)]
    written += int(write_jsonl_if_changed(corpus / "index.jsonl", index_rows))
    written += int(write_jsonl_if_changed(corpus / "safety" / "redactions.jsonl", redaction_rows))
    written += int(write_jsonl_if_changed(corpus / "safety" / "exclusions.jsonl", exclusion_rows))
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
        "reused_conversations": reused,
        "blocking_manual_review": blocking,
    }
    written += int(write_json_if_changed(corpus / "safety" / "report.json", report))
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
    for directory in (corpus / "raw", corpus / "metadata"):
        if not directory.exists():
            continue
        for path in sorted(directory.glob("*")):
            if not path.is_file():
                continue
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
    return {
        "scanner_revision": SCANNER_REVISION,
        "files_scanned": sum(1 for directory in (corpus / "raw", corpus / "metadata") if directory.exists() for path in directory.glob("*") if path.is_file()),
        "unexpected_sensitive_spans": unexpected,
        "pass": not unexpected,
    }
