"""Read screened canonical conversations and construct explicit input views."""

from __future__ import annotations

import json
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Any, Iterable

from .identity import canonical_json_bytes, sha256_bytes


INPUT_VIEWS = {"full", "user", "assistant", "title_user", "title", "structural"}


@dataclass(frozen=True)
class Document:
    corpus_id: str
    title: str
    created_at: float | None
    updated_at: float | None
    messages: tuple[dict[str, Any], ...]
    existing_location: dict[str, Any]
    repository_references: tuple[str, ...]
    redacted: bool
    source_sha256: str

    def messages_for_role(self, role: str, active_only: bool = True) -> list[str]:
        return [
            str(row.get("text", ""))
            for row in self.messages
            if row.get("role") == role and (not active_only or row.get("active_branch", False))
        ]

    def text(self, view: str, active_only: bool = True) -> str:
        if view not in INPUT_VIEWS:
            raise ValueError(f"unknown input view {view!r}")
        user = "\n\n".join(self.messages_for_role("user", active_only))
        assistant = "\n\n".join(self.messages_for_role("assistant", active_only))
        if view == "user":
            return user
        if view == "assistant":
            return assistant
        if view == "title_user":
            return f"title: {self.title}\n\nuser:\n{user}"
        if view == "title":
            return self.title
        if view == "structural":
            return ""
        return f"title: {self.title}\n\nuser:\n{user}\n\nassistant:\n{assistant}"

    def structural_features(self) -> dict[str, float]:
        user_messages = self.messages_for_role("user")
        assistant_messages = self.messages_for_role("assistant")
        user_characters = sum(len(value) for value in user_messages)
        assistant_characters = sum(len(value) for value in assistant_messages)
        return {
            "user_message_count": float(len(user_messages)),
            "assistant_message_count": float(len(assistant_messages)),
            "user_characters": float(user_characters),
            "assistant_characters": float(assistant_characters),
            "assistant_to_user_character_ratio": float(assistant_characters / max(user_characters, 1)),
            "repository_reference_count": float(len(self.repository_references)),
            "redacted": float(self.redacted),
        }


def _jsonl(path: Path) -> list[dict[str, Any]]:
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def _timestamp(value: Any) -> float | None:
    if isinstance(value, (int, float)):
        return float(value)
    if isinstance(value, str) and value:
        try:
            return datetime.fromisoformat(value.replace("Z", "+00:00")).timestamp()
        except ValueError:
            return None
    return None


def load_corpus(corpus: Path, only_ids: set[str] | None = None) -> list[Document]:
    corpus = corpus.resolve()
    documents: list[Document] = []
    for index_row in _jsonl(corpus / "index.jsonl"):
        if only_ids is not None and index_row["corpus_id"] not in only_ids:
            continue
        metadata = json.loads((corpus / index_row["metadata_path"]).read_text(encoding="utf-8"))
        raw = json.loads((corpus / index_row["raw_path"]).read_text(encoding="utf-8"))
        if metadata["corpus_id"] != raw["corpus_id"] or metadata["raw_sha256"] != index_row["raw_sha256"]:
            raise ValueError(f"identity/hash mismatch for {index_row.get('corpus_id')}")
        documents.append(
            Document(
                corpus_id=raw["corpus_id"],
                title=raw["title"],
                created_at=_timestamp(raw.get("created_at")),
                updated_at=_timestamp(raw.get("updated_at")),
                messages=tuple(raw["messages"]),
                existing_location=metadata.get("existing_location", {}),
                repository_references=tuple(metadata.get("repository_references", [])),
                redacted=bool(metadata.get("redacted", False)),
                source_sha256=index_row["raw_sha256"],
            )
        )
    ids = [document.corpus_id for document in documents]
    if len(ids) != len(set(ids)):
        raise ValueError("corpus index contains duplicate corpus ids")
    return documents


def load_title_diagnostic(directory: Path, only_ids: set[str] | None = None) -> list[Document]:
    documents: list[Document] = []
    for row in _jsonl(directory / "index.jsonl"):
        if only_ids is not None and row["title_row_id"] not in only_ids:
            continue
        documents.append(
            Document(
                corpus_id=row["title_row_id"],
                title=row["title"],
                created_at=_timestamp(row.get("created_at")),
                updated_at=None,
                messages=tuple(),
                existing_location={"legacy_title_ledger": row["existing_location"]},
                repository_references=tuple(),
                redacted=False,
                source_sha256=sha256_bytes(canonical_json_bytes(row)),
            )
        )
    return documents


def source_fingerprints(directory: Path, *, title_diagnostic: bool = False) -> dict[str, str]:
    """Read cheap index fingerprints without reparsing any conversation body."""
    rows = _jsonl(directory / "index.jsonl")
    if title_diagnostic:
        return {
            row["title_row_id"]: sha256_bytes(canonical_json_bytes(row))
            for row in rows
        }
    return {row["corpus_id"]: row["raw_sha256"] for row in rows}
