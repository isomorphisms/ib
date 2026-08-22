from __future__ import annotations

import json
import re
from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable
from urllib.parse import unquote, urlparse

from .history import HistoryEntry

_TOKEN = re.compile(r"[\w]+", re.UNICODE)


@dataclass(slots=True)
class HistoryIndices:
    entries: list[HistoryEntry]
    chronology: list[int]
    by_url: dict[str, list[int]]
    by_host: dict[str, list[int]]
    by_source: dict[str, list[int]]
    by_day: dict[str, list[int]]
    by_query: dict[str, list[int]]
    terms: dict[str, list[int]]

    def summary(self) -> dict[str, object]:
        return {
            "entries": len(self.entries),
            "unique_urls": len(self.by_url),
            "hosts": len(self.by_host),
            "sources": {key: len(value) for key, value in sorted(self.by_source.items())},
            "search_queries": len(self.by_query),
            "days": len(self.by_day),
        }


class IndexBuilder:
    """Build cheap in-memory indices without collapsing the visit stream."""

    def build(self, entries: Iterable[HistoryEntry]) -> HistoryIndices:
        rows = list(entries)
        by_url: dict[str, list[int]] = defaultdict(list)
        by_host: dict[str, list[int]] = defaultdict(list)
        by_source: dict[str, list[int]] = defaultdict(list)
        by_day: dict[str, list[int]] = defaultdict(list)
        by_query: dict[str, list[int]] = defaultdict(list)
        terms: dict[str, list[int]] = defaultdict(list)

        for index, entry in enumerate(rows):
            by_url[entry.url].append(index)
            host = urlparse(entry.url).hostname
            if host:
                by_host[host.lower()].append(index)
            by_source[entry.source].append(index)
            if entry.visited_at:
                by_day[entry.visited_at.astimezone(timezone.utc).date().isoformat()].append(index)
            if entry.query:
                by_query[entry.query].append(index)
            for token in _tokens(entry):
                if not terms[token] or terms[token][-1] != index:
                    terms[token].append(index)

        chronology = sorted(
            range(len(rows)),
            key=lambda i: (
                rows[i].visited_at is not None,
                rows[i].visited_at or datetime.min.replace(tzinfo=timezone.utc),
                rows[i].import_order,
            ),
            reverse=True,
        )

        return HistoryIndices(
            entries=rows,
            chronology=chronology,
            by_url=dict(by_url),
            by_host=dict(by_host),
            by_source=dict(by_source),
            by_day=dict(by_day),
            by_query=dict(by_query),
            terms=dict(terms),
        )


def _tokens(entry: HistoryEntry) -> set[str]:
    parsed = urlparse(entry.url)
    text = " ".join(
        part
        for part in (
            parsed.hostname or "",
            unquote(parsed.path),
            unquote(parsed.query),
            entry.title or "",
            entry.query or "",
        )
        if part
    )
    return {token.casefold() for token in _TOKEN.findall(text) if len(token) > 1}


def write_plaintext_indices(indices: HistoryIndices, root: str | Path) -> None:
    """Persist transparent test indices as JSONL/TSV files.

    These are deliberately derived data. The original HistoryEntry stream remains
    reconstructable from visits.jsonl, while all other files can be deleted and rebuilt.
    """
    root = Path(root)
    root.mkdir(parents=True, exist_ok=True)

    with (root / "visits.jsonl").open("w", encoding="utf-8") as out:
        for entry in indices.entries:
            out.write(json.dumps(entry.as_dict(), ensure_ascii=False, sort_keys=True) + "\n")

    with (root / "chronology.tsv").open("w", encoding="utf-8") as out:
        out.write("index\tvisited_at\tsource\tkind\turl\n")
        for index in indices.chronology:
            entry = indices.entries[index]
            out.write(
                f"{index}\t{_safe(entry.visited_at.isoformat() if entry.visited_at else '')}\t"
                f"{_safe(entry.source)}\t{_safe(entry.kind)}\t{_safe(entry.url)}\n"
            )

    _write_count_index(root / "urls.tsv", indices.by_url)
    _write_count_index(root / "hosts.tsv", indices.by_host)
    _write_count_index(root / "queries.tsv", indices.by_query)
    _write_count_index(root / "terms.tsv", indices.terms)
    (root / "summary.json").write_text(
        json.dumps(indices.summary(), ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def _write_count_index(path: Path, mapping: dict[str, list[int]]) -> None:
    with path.open("w", encoding="utf-8") as out:
        out.write("key\tcount\tentry_indices\n")
        for key, values in sorted(mapping.items(), key=lambda item: (-len(item[1]), item[0])):
            out.write(f"{_safe(key)}\t{len(values)}\t{','.join(map(str, values))}\n")


def _safe(value: str) -> str:
    return value.replace("\\", "\\\\").replace("\t", "\\t").replace("\r", "\\r").replace("\n", "\\n")
