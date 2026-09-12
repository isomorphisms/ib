"""Stable identities, hashes, and deterministic writes."""

from __future__ import annotations

import hashlib
import json
import os
import tempfile
from pathlib import Path
from typing import Any, Iterable


def canonical_json_bytes(value: Any) -> bytes:
    return (json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":")) + "\n").encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_text(value: str) -> str:
    return sha256_bytes(value.encode("utf-8"))


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def stable_id(kind: str, namespace: str, source_id: str) -> str:
    digest = hashlib.sha256(f"{namespace}\0{source_id}".encode("utf-8")).hexdigest()
    return f"{kind}-{digest[:24]}"


def write_bytes_if_changed(path: Path, value: bytes) -> bool:
    if path.exists() and path.read_bytes() == value:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(value)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary_name, path)
    except BaseException:
        try:
            os.unlink(temporary_name)
        except FileNotFoundError:
            pass
        raise
    return True


def write_json_if_changed(path: Path, value: Any) -> bool:
    return write_bytes_if_changed(path, canonical_json_bytes(value))


def write_jsonl_if_changed(path: Path, rows: Iterable[dict[str, Any]]) -> bool:
    value = b"".join(canonical_json_bytes(row) for row in rows)
    return write_bytes_if_changed(path, value)

