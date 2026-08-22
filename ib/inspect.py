from __future__ import annotations

import argparse
import json
import os
import re
import stat
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


_CANONICAL_TAB_NAMES = {"tab.txt", "history.log", "view.txt"}
_DERIVED_DIRS = {"indexes"}
_TRANSPARENT_DERIVED_NAMES = {
    "chronology.tsv",
    "urls.tsv",
    "hosts.tsv",
    "sources.tsv",
    "days.tsv",
    "queries.tsv",
    "terms.tsv",
    "summary.json",
}
_CACHE_DIRS = {
    "cache",
    "caches",
    "renderer-cache",
    "renderer_cache",
    "scratch",
    "tmp",
}
_SECRET_WORDS = {
    "auth",
    "authentication",
    "bearer",
    "cookie",
    "cookies",
    "credential",
    "credentials",
    "keychain",
    "keystore",
    "login",
    "logins",
    "oauth",
    "passwd",
    "password",
    "passwords",
    "secret",
    "secrets",
    "session",
    "sessions",
    "token",
    "tokens",
}
_SECRET_NAMES = {"login_data", "web_data"}
_VISITS_PATH = Path("visits.jsonl")
_TAB_MANIFEST_LIMIT = 64 * 1024
_TAIL_CHUNK_BYTES = 64 * 1024
_TAIL_MAX_BYTES = 1024 * 1024


@dataclass(frozen=True, slots=True)
class StoreFile:
    path: str
    bytes: int
    kind: str
    file_type: str
    readable: bool


@dataclass(frozen=True, slots=True)
class TabRecord:
    path: str
    tab_id: str
    fields: dict[str, tuple[str, ...]]
    truncated: bool = False


class StorageInspector:
    """Read-only inspection of IB-owned persistent storage.

    Classification is based on the IB storage schema, not just a basename.
    Generic reads are allowed only for declared transparent text records.
    Symlinks are visible as metadata but are never followed by readers.
    """

    def __init__(self, root: str | Path):
        self.root = Path(root).expanduser().resolve(strict=False)

    def files(self, limit: int | None = None, offset: int = 0) -> list[StoreFile]:
        if limit is not None and limit < 0:
            raise ValueError("limit must be non-negative")
        if offset < 0:
            raise ValueError("offset must be non-negative")
        if limit == 0:
            return []

        rows: list[StoreFile] = []
        for index, row in enumerate(self._iter_files()):
            if index < offset:
                continue
            rows.append(row)
            if limit is not None and len(rows) >= limit:
                break
        return rows

    def _iter_files(self) -> Iterable[StoreFile]:
        if not self.root.exists():
            return

        stack: list[tuple[Path, Path]] = [(self.root, Path())]
        while stack:
            directory, relative_directory = stack.pop()
            try:
                with os.scandir(directory) as stream:
                    entries = sorted(stream, key=lambda entry: entry.name)
                    directories: list[tuple[Path, Path]] = []
                    for entry in entries:
                        relative = relative_directory / entry.name
                        try:
                            metadata = entry.stat(follow_symlinks=False)
                        except OSError:
                            continue

                        if stat.S_ISDIR(metadata.st_mode):
                            directories.append((Path(entry.path), relative))
                            continue

                        if stat.S_ISREG(metadata.st_mode):
                            file_type = "file"
                        elif stat.S_ISLNK(metadata.st_mode):
                            file_type = "symlink"
                        else:
                            file_type = "other"

                        kind = self.classify(relative)
                        parts = tuple(part.casefold() for part in relative.parts)
                        yield StoreFile(
                            path=relative.as_posix(),
                            bytes=metadata.st_size,
                            kind=kind,
                            file_type=file_type,
                            readable=(
                                file_type == "file"
                                and kind != "secret"
                                and _is_transparent_text_parts(parts)
                            ),
                        )
            except OSError:
                continue

            stack.extend(reversed(directories))

    def classify(self, path: str | Path) -> str:
        relative = self._relative_path(path)
        if relative is None:
            return "unknown"

        parts = tuple(part.casefold() for part in relative.parts)
        if not parts or any(part == ".." for part in parts):
            return "unknown"

        if parts == ("visits.jsonl",) or _is_tab_record_parts(parts):
            return "canonical"

        if any(_looks_secret(part) for part in parts):
            return "secret"

        top = parts[0]
        if top == "snapshots":
            return "snapshot"
        if top in _CACHE_DIRS:
            return "cache"

        if top in _DERIVED_DIRS:
            return "derived"

        return "unknown"

    def overview(self) -> dict[str, object]:
        bytes_by_kind: dict[str, int] = {}
        files_by_kind: dict[str, int] = {}
        tab_records = 0
        file_count = 0
        byte_count = 0

        for row in self._iter_files():
            file_count += 1
            byte_count += row.bytes
            bytes_by_kind[row.kind] = bytes_by_kind.get(row.kind, 0) + row.bytes
            files_by_kind[row.kind] = files_by_kind.get(row.kind, 0) + 1
            if row.file_type == "file" and _is_tab_manifest_parts(tuple(Path(row.path).parts)):
                tab_records += 1

        return {
            "root": str(self.root),
            "files": file_count,
            "bytes": byte_count,
            "files_by_kind": dict(sorted(files_by_kind.items())),
            "bytes_by_kind": dict(sorted(bytes_by_kind.items())),
            "tab_records": tab_records,
            # This is intentionally not called a canonical count. Reading the
            # whole append log just to open the overview would make first paint
            # scale with all history.
            "indexed_visits": self._indexed_visit_count(),
        }

    def tabs(self, limit: int = 100, offset: int = 0) -> list[TabRecord]:
        if limit < 0:
            raise ValueError("limit must be non-negative")
        if offset < 0:
            raise ValueError("offset must be non-negative")
        if limit == 0:
            return []

        records: list[TabRecord] = []
        paths = self._tab_manifest_paths()
        for relative in paths[offset : offset + limit]:
            try:
                data, truncated = self._read_limited(relative, _TAB_MANIFEST_LIMIT)
            except (FileNotFoundError, NotADirectoryError, PermissionError, OSError):
                # The live store may change between listing and opening.
                continue

            if truncated and data and not data.endswith(b"\n"):
                data = data.rsplit(b"\n", 1)[0] if b"\n" in data else b""

            fields: dict[str, list[str]] = {}
            for raw_line in data.decode("utf-8", errors="replace").splitlines():
                line = raw_line.strip()
                if not line or line.startswith("#"):
                    continue
                key, separator, value = line.partition(" ")
                if not separator:
                    value = ""
                fields.setdefault(key, []).append(value)

            frozen_fields = {key: tuple(values) for key, values in fields.items()}
            default_id = relative.parent.name
            tab_id = frozen_fields.get("id", (default_id,))[0] or default_id
            records.append(
                TabRecord(
                    path=relative.as_posix(),
                    tab_id=tab_id,
                    fields=frozen_fields,
                    truncated=truncated,
                )
            )
        return records

    def visit_count(self) -> int:
        """Return the exact canonical visit count by scanning the append log."""

        try:
            fd = self._open_file_fd(_VISITS_PATH)
        except FileNotFoundError:
            return 0

        count = 0
        with os.fdopen(fd, "rb") as source:
            for line in source:
                if line.strip():
                    count += 1
        return count

    def visits(
        self,
        limit: int = 20,
        max_bytes: int = _TAIL_MAX_BYTES,
    ) -> list[dict[str, object]]:
        if limit < 0:
            raise ValueError("limit must be non-negative")
        if max_bytes < 0:
            raise ValueError("max_bytes must be non-negative")
        if limit == 0 or max_bytes == 0:
            return []

        try:
            lines = self._tail_nonempty_lines(_VISITS_PATH, limit, max_bytes)
        except FileNotFoundError:
            return []

        rows: list[dict[str, object]] = []
        for byte_offset, raw_line in lines:
            line = raw_line.decode("utf-8", errors="replace").strip()
            try:
                value = json.loads(line)
            except json.JSONDecodeError:
                value = {"_invalid_json": line}
            if not isinstance(value, dict):
                value = {"value": value}
            value = dict(value)
            value["_file"] = _VISITS_PATH.as_posix()
            value["_offset"] = byte_offset
            rows.append(value)
        return rows

    def read_text(self, relative_path: str | Path, max_bytes: int = 64 * 1024) -> str:
        if max_bytes < 0:
            raise ValueError("max_bytes must be non-negative")
        relative = self._validated_relative_path(relative_path)
        data, truncated = self._read_limited(relative, max_bytes)
        text = data.decode("utf-8", errors="replace")
        return text + ("\n[truncated]\n" if truncated else "")

    def render_text(self, limit: int = 20) -> str:
        if limit < 0:
            raise ValueError("limit must be non-negative")

        overview = self.overview()
        files = self.files(limit=limit)
        indexed_visits = overview["indexed_visits"]
        lines = [
            "IB STORAGE",
            f"root {overview['root']}",
            f"files {overview['files']}",
            f"bytes {overview['bytes']}",
            f"tab_records {overview['tab_records']}",
            f"indexed_visits {indexed_visits if indexed_visits is not None else '-'}",
        ]
        for kind, size in overview["bytes_by_kind"].items():
            count = overview["files_by_kind"][kind]
            lines.append(f"{kind} {count} files {size} bytes")

        lines.append("")
        lines.append(f"FILES showing {len(files)} of {overview['files']}")
        for row in files:
            access = "read" if row.readable else "meta"
            file_type = "" if row.file_type == "file" else f" {row.file_type}"
            lines.append(f"{row.kind:9} {access:4}{file_type:8} {row.bytes:>9} {row.path}")

        tabs = self.tabs(limit=limit)
        if tabs:
            lines.append("")
            lines.append("TAB RECORDS")
            for tab in tabs:
                state = _first(tab.fields, "state")
                current = _first(tab.fields, "current_history")
                labels = ",".join(tab.fields.get("label", ()))
                detail = " ".join(
                    piece
                    for piece in (
                        f"state={state}" if state else "",
                        f"current_history={current}" if current else "",
                        f"labels={labels}" if labels else "",
                        "truncated=true" if tab.truncated else "",
                    )
                    if piece
                )
                lines.append(f"{tab.tab_id} {detail}".rstrip())

        try:
            visits = self.visits(limit=limit)
        except OSError as exc:
            visits = []
            lines.append("")
            lines.append(f"RECENT IMPORTED VISITS unavailable ({exc.__class__.__name__})")
        if visits:
            lines.append("")
            lines.append("RECENT IMPORTED VISITS")
            for row in visits:
                visited_at = row.get("visited_at") or "-"
                source = row.get("source") or "-"
                url = row.get("url") or row.get("_invalid_json") or row.get("value") or "-"
                lines.append(f"{visited_at} {source} {url}")

        return "\n".join(lines) + "\n"

    def _relative_path(self, path: str | Path) -> Path | None:
        path = Path(path)
        if path.is_absolute():
            try:
                return path.relative_to(self.root)
            except ValueError:
                return None
        return path

    def _validated_relative_path(self, path: str | Path) -> Path:
        relative = Path(path)
        if relative.is_absolute() or not relative.parts or any(part == ".." for part in relative.parts):
            raise ValueError("path must stay relative to storage root")
        return relative

    def _read_limited(self, relative: Path, max_bytes: int) -> tuple[bytes, bool]:
        fd = self._open_file_fd(relative)
        with os.fdopen(fd, "rb") as source:
            data = source.read(max_bytes + 1)
        truncated = len(data) > max_bytes
        return (data[:max_bytes] if truncated else data, truncated)

    def _open_file_fd(self, relative: Path) -> int:
        relative = self._validated_relative_path(relative)
        kind = self.classify(relative)
        if kind == "secret":
            raise PermissionError("protected storage is not readable through the inspector")
        parts = tuple(part.casefold() for part in relative.parts)
        if not _is_transparent_text_parts(parts):
            raise PermissionError("only schema-declared transparent text records are readable")

        no_follow = getattr(os, "O_NOFOLLOW", 0)
        if not no_follow or os.open not in getattr(os, "supports_dir_fd", set()):
            raise OSError("platform lacks rooted no-follow file opens")

        read_flags = os.O_RDONLY | getattr(os, "O_CLOEXEC", 0)
        directory_flags = read_flags | getattr(os, "O_DIRECTORY", 0) | no_follow

        root_fd = os.open(self.root, directory_flags)
        if not stat.S_ISDIR(os.fstat(root_fd).st_mode):
            os.close(root_fd)
            raise NotADirectoryError(self.root)
        directory_fd = root_fd
        try:
            for part in relative.parts[:-1]:
                next_fd = os.open(part, directory_flags, dir_fd=directory_fd)
                if not stat.S_ISDIR(os.fstat(next_fd).st_mode):
                    os.close(next_fd)
                    raise NotADirectoryError(self.root / relative.parent)
                if directory_fd != root_fd:
                    os.close(directory_fd)
                directory_fd = next_fd

            file_fd = os.open(relative.parts[-1], read_flags | no_follow, dir_fd=directory_fd)
            metadata = os.fstat(file_fd)
            if not stat.S_ISREG(metadata.st_mode):
                os.close(file_fd)
                raise FileNotFoundError(self.root / relative)
            return file_fd
        finally:
            if directory_fd != root_fd:
                os.close(directory_fd)
            os.close(root_fd)

    def _tab_manifest_paths(self) -> list[Path]:
        tabs_root = self.root / "tabs"
        try:
            tabs_metadata = tabs_root.lstat()
        except OSError:
            return []
        if not stat.S_ISDIR(tabs_metadata.st_mode):
            return []

        paths: list[Path] = []
        try:
            with os.scandir(tabs_root) as stream:
                entries = sorted(stream, key=lambda entry: entry.name)
                for entry in entries:
                    try:
                        if not entry.is_dir(follow_symlinks=False):
                            continue
                    except OSError:
                        continue
                    manifest = Path(entry.path) / "tab.txt"
                    try:
                        metadata = manifest.lstat()
                    except OSError:
                        continue
                    if stat.S_ISREG(metadata.st_mode):
                        paths.append(Path("tabs") / entry.name / "tab.txt")
        except OSError:
            return []
        return paths

    def _indexed_visit_count(self) -> int | None:
        summary = Path("indexes") / "summary.json"
        try:
            data, truncated = self._read_limited(summary, 64 * 1024)
        except (FileNotFoundError, NotADirectoryError, PermissionError, OSError):
            return None
        if truncated:
            return None
        try:
            value = json.loads(data.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            return None
        if not isinstance(value, dict):
            return None
        entries = value.get("entries")
        return entries if isinstance(entries, int) and not isinstance(entries, bool) and entries >= 0 else None

    def _tail_nonempty_lines(
        self,
        relative: Path,
        limit: int,
        max_bytes: int,
    ) -> list[tuple[int, bytes]]:
        fd = self._open_file_fd(relative)
        with os.fdopen(fd, "rb") as source:
            source.seek(0, os.SEEK_END)
            position = source.tell()
            data = b""
            bytes_read = 0

            while position > 0 and bytes_read < max_bytes:
                step = min(_TAIL_CHUNK_BYTES, position, max_bytes - bytes_read)
                position -= step
                source.seek(position)
                data = source.read(step) + data
                bytes_read += step

                usable = data
                base_offset = position
                if position > 0:
                    first_newline = data.find(b"\n")
                    if first_newline < 0:
                        continue
                    usable = data[first_newline + 1 :]
                    base_offset += first_newline + 1

                rows = _nonempty_lines_with_offsets(usable, base_offset)
                if len(rows) >= limit:
                    return rows[-limit:]

            if position == 0:
                return _nonempty_lines_with_offsets(data, 0)[-limit:]

            first_newline = data.find(b"\n")
            if first_newline < 0:
                return []
            base_offset = position + first_newline + 1
            return _nonempty_lines_with_offsets(data[first_newline + 1 :], base_offset)[-limit:]


def _looks_secret(component: str) -> bool:
    folded = component.casefold()
    normalized = re.sub(r"[^a-z0-9]+", "_", folded).strip("_")
    if normalized in _SECRET_NAMES:
        return True
    words = {word for word in re.split(r"[^a-z0-9]+", folded) if word}
    return bool(words & _SECRET_WORDS)


def _is_tab_record_parts(parts: tuple[str, ...]) -> bool:
    return len(parts) == 3 and parts[0] == "tabs" and parts[2] in _CANONICAL_TAB_NAMES


def _is_transparent_text_parts(parts: tuple[str, ...]) -> bool:
    if parts == ("visits.jsonl",) or _is_tab_record_parts(parts):
        return True
    return (
        len(parts) == 2
        and parts[0] in _DERIVED_DIRS
        and parts[1] in _TRANSPARENT_DERIVED_NAMES
    )


def _is_tab_manifest_parts(parts: tuple[str, ...]) -> bool:
    return len(parts) == 3 and parts[0].casefold() == "tabs" and parts[2].casefold() == "tab.txt"


def _nonempty_lines_with_offsets(data: bytes, base_offset: int) -> list[tuple[int, bytes]]:
    rows: list[tuple[int, bytes]] = []
    offset = base_offset
    for raw_line in data.splitlines(keepends=True):
        line = raw_line.rstrip(b"\r\n")
        if line.strip():
            rows.append((offset, line))
        offset += len(raw_line)
    return rows


def _first(fields: dict[str, tuple[str, ...]], key: str) -> str:
    values = fields.get(key, ())
    return values[0] if values else ""


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Inspect IB persistent storage")
    parser.add_argument("root", nargs="?", default="state", help="IB storage root")
    parser.add_argument("--limit", type=int, default=20, help="records shown per section")
    parser.add_argument("--read", metavar="PATH", help="print one declared transparent text record")
    args = parser.parse_args(list(argv) if argv is not None else None)

    inspector = StorageInspector(args.root)
    if args.read:
        print(inspector.read_text(args.read), end="")
    else:
        print(inspector.render_text(limit=args.limit), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
