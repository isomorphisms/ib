from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


_CANONICAL_NAMES = {"visits.jsonl", "tab.txt", "history.log", "view.txt"}
_DERIVED_NAMES = {
    "chronology.tsv",
    "urls.tsv",
    "hosts.tsv",
    "sources.tsv",
    "days.tsv",
    "queries.tsv",
    "terms.tsv",
    "summary.json",
}
_DERIVED_DIRS = {"index", "indexes"}
_CACHE_DIRS = {
    "cache",
    "caches",
    "renderer-cache",
    "renderer_cache",
    "scratch",
    "tmp",
}
_SECRET_PARTS = {
    "secret",
    "secrets",
    "credential",
    "credentials",
    "cookie",
    "cookies",
    "token",
    "tokens",
}


@dataclass(frozen=True, slots=True)
class StoreFile:
    path: str
    bytes: int
    kind: str


@dataclass(frozen=True, slots=True)
class TabRecord:
    path: str
    tab_id: str
    fields: dict[str, tuple[str, ...]]


class StorageInspector:
    """Read-only inspection of IB-owned persistent storage.

    The inspector intentionally understands only browser-owned, transparent
    records. Renderer profiles and protected secret stores may be listed but
    are never opened by this class.
    """

    def __init__(self, root: str | Path):
        self.root = Path(root)

    def files(self) -> list[StoreFile]:
        if not self.root.exists():
            return []
        rows: list[StoreFile] = []
        for path in sorted(p for p in self.root.rglob("*") if p.is_file()):
            rows.append(
                StoreFile(
                    path=path.relative_to(self.root).as_posix(),
                    bytes=path.stat().st_size,
                    kind=self.classify(path),
                )
            )
        return rows

    def classify(self, path: str | Path) -> str:
        path = Path(path)
        try:
            relative = path.relative_to(self.root)
        except ValueError:
            relative = path
        parts = {part.casefold() for part in relative.parts}
        name = relative.name.casefold()

        if parts & _SECRET_PARTS:
            return "secret"
        if name in _CANONICAL_NAMES:
            return "canonical"
        if "snapshots" in parts:
            return "snapshot"
        if parts & _CACHE_DIRS:
            return "cache"
        if name in _DERIVED_NAMES or parts & _DERIVED_DIRS:
            return "derived"
        return "unknown"

    def overview(self) -> dict[str, object]:
        files = self.files()
        bytes_by_kind: dict[str, int] = {}
        files_by_kind: dict[str, int] = {}
        for row in files:
            bytes_by_kind[row.kind] = bytes_by_kind.get(row.kind, 0) + row.bytes
            files_by_kind[row.kind] = files_by_kind.get(row.kind, 0) + 1
        return {
            "root": str(self.root),
            "files": len(files),
            "bytes": sum(row.bytes for row in files),
            "files_by_kind": dict(sorted(files_by_kind.items())),
            "bytes_by_kind": dict(sorted(bytes_by_kind.items())),
            "tabs": len(self.tabs()),
            "visits": self.visit_count(),
        }

    def tabs(self) -> list[TabRecord]:
        if not self.root.exists():
            return []
        records: list[TabRecord] = []
        for path in sorted(self.root.rglob("tab.txt")):
            relative = path.relative_to(self.root)
            if "tabs" not in {part.casefold() for part in relative.parts}:
                continue
            fields: dict[str, list[str]] = {}
            for raw_line in path.read_text(encoding="utf-8", errors="replace").splitlines():
                line = raw_line.strip()
                if not line or line.startswith("#"):
                    continue
                key, separator, value = line.partition(" ")
                if not separator:
                    value = ""
                fields.setdefault(key, []).append(value)
            frozen_fields = {key: tuple(values) for key, values in fields.items()}
            default_id = path.parent.name
            tab_id = frozen_fields.get("id", (default_id,))[0] or default_id
            records.append(
                TabRecord(
                    path=relative.as_posix(),
                    tab_id=tab_id,
                    fields=frozen_fields,
                )
            )
        return records

    def visit_count(self) -> int:
        count = 0
        for path in self._visit_files():
            with path.open("r", encoding="utf-8", errors="replace") as source:
                count += sum(1 for line in source if line.strip())
        return count

    def visits(self, limit: int = 20) -> list[dict[str, object]]:
        if limit < 0:
            raise ValueError("limit must be non-negative")
        if limit == 0:
            return []

        rows: list[dict[str, object]] = []
        for path in self._visit_files():
            relative = path.relative_to(self.root).as_posix()
            with path.open("r", encoding="utf-8", errors="replace") as source:
                for line_number, raw_line in enumerate(source, start=1):
                    line = raw_line.strip()
                    if not line:
                        continue
                    try:
                        value = json.loads(line)
                    except json.JSONDecodeError:
                        value = {"_invalid_json": line}
                    if not isinstance(value, dict):
                        value = {"value": value}
                    value = dict(value)
                    value["_file"] = relative
                    value["_line"] = line_number
                    rows.append(value)
                    if len(rows) > limit:
                        del rows[0]
        return rows

    def read_text(self, relative_path: str | Path, max_bytes: int = 64 * 1024) -> str:
        if max_bytes < 0:
            raise ValueError("max_bytes must be non-negative")
        relative_path = Path(relative_path)
        path = (self.root / relative_path).resolve()
        root = self.root.resolve()
        if path != root and root not in path.parents:
            raise ValueError("path escapes storage root")
        if not path.is_file():
            raise FileNotFoundError(path)
        if self.classify(path) == "secret":
            raise PermissionError("protected storage is not readable through the inspector")
        with path.open("rb") as source:
            data = source.read(max_bytes + 1)
        truncated = len(data) > max_bytes
        if truncated:
            data = data[:max_bytes]
        text = data.decode("utf-8", errors="replace")
        return text + ("\n[truncated]\n" if truncated else "")

    def render_text(self, limit: int = 20) -> str:
        overview = self.overview()
        lines = [
            "IB STORAGE",
            f"root {overview['root']}",
            f"files {overview['files']}",
            f"bytes {overview['bytes']}",
            f"tabs {overview['tabs']}",
            f"visits {overview['visits']}",
        ]
        for kind, size in overview["bytes_by_kind"].items():
            count = overview["files_by_kind"][kind]
            lines.append(f"{kind} {count} files {size} bytes")

        lines.append("")
        lines.append("FILES")
        for row in self.files():
            lines.append(f"{row.kind[0].upper()} {row.bytes:>9} {row.path}")

        tabs = self.tabs()
        if tabs:
            lines.append("")
            lines.append("TAB RECORDS")
            for tab in tabs[:limit]:
                state = _first(tab.fields, "state")
                current = _first(tab.fields, "current_history")
                labels = ",".join(tab.fields.get("label", ()))
                detail = " ".join(
                    piece
                    for piece in (
                        f"state={state}" if state else "",
                        f"current_history={current}" if current else "",
                        f"labels={labels}" if labels else "",
                    )
                    if piece
                )
                lines.append(f"{tab.tab_id} {detail}".rstrip())

        visits = self.visits(limit=limit)
        if visits:
            lines.append("")
            lines.append("RECENT IMPORTED VISITS")
            for row in visits:
                visited_at = row.get("visited_at") or "-"
                source = row.get("source") or "-"
                url = row.get("url") or row.get("_invalid_json") or row.get("value") or "-"
                lines.append(f"{visited_at} {source} {url}")

        return "\n".join(lines) + "\n"

    def _visit_files(self) -> list[Path]:
        if not self.root.exists():
            return []
        return sorted(
            path
            for path in self.root.rglob("visits.jsonl")
            if path.is_file() and self.classify(path) == "canonical"
        )


def _first(fields: dict[str, tuple[str, ...]], key: str) -> str:
    values = fields.get(key, ())
    return values[0] if values else ""


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Inspect IB persistent storage")
    parser.add_argument("root", nargs="?", default="state", help="IB storage root")
    parser.add_argument("--limit", type=int, default=20, help="records shown per section")
    parser.add_argument("--read", metavar="PATH", help="print one non-secret text file")
    args = parser.parse_args(list(argv) if argv is not None else None)

    inspector = StorageInspector(args.root)
    if args.read:
        print(inspector.read_text(args.read), end="")
    else:
        print(inspector.render_text(limit=args.limit), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
