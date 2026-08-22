from __future__ import annotations

import json
import math
import sqlite3
from dataclasses import dataclass, replace
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any, Callable, Iterable, Iterator, Mapping
from urllib.parse import parse_qs, urlparse


@dataclass(frozen=True, slots=True)
class HistoryEntry:
    url: str
    visited_at: datetime | None = None
    title: str | None = None
    source: str = "unknown"
    kind: str = "visit"
    query: str | None = None
    import_order: int = -1

    def as_dict(self) -> dict[str, Any]:
        return {
            "url": self.url,
            "visited_at": self.visited_at.isoformat() if self.visited_at else None,
            "title": self.title,
            "source": self.source,
            "kind": self.kind,
            "query": self.query,
            "import_order": self.import_order,
        }


Adapter = Callable[[Any], Iterable[HistoryEntry]]
_ADAPTERS: dict[str, Adapter] = {}


def register_history_adapter(name: str, adapter: Adapter) -> None:
    if not name or not name.replace("_", "").isalnum():
        raise ValueError("adapter names must contain letters, digits, or underscores")
    _ADAPTERS[name] = adapter


def available_history_adapters() -> tuple[str, ...]:
    return tuple(sorted(_ADAPTERS))


def ingest_history(value: Any, source: str = "auto") -> list[HistoryEntry]:
    """Normalize a history export into HistoryEntry values.

    `source="auto"` detects local Chrome/Chromium and Firefox SQLite files,
    Google/YouTube Takeout JSON, canonical history JSONL, or raw URL inputs.
    """
    adapter_name = detect_history_source(value) if source == "auto" else source
    try:
        adapter = _ADAPTERS[adapter_name]
    except KeyError as exc:
        raise ValueError(f"unknown history source: {adapter_name}") from exc

    entries: list[HistoryEntry] = []
    for entry in adapter(value):
        url = entry.url.strip()
        if not url:
            continue
        visited_at = _parse_time(entry.visited_at) if entry.visited_at is not None else None
        entries.append(replace(entry, url=url, visited_at=visited_at, import_order=len(entries)))
    return entries


def detect_history_source(value: Any) -> str:
    if isinstance(value, str):
        stripped = value.lstrip()
        if stripped.startswith(("[", "{")):
            try:
                return _detect_json_source(json.loads(value))
            except json.JSONDecodeError:
                pass
        path = _existing_file(value)
        if path:
            return _detect_file_source(path)
        return "raw_urls"

    if isinstance(value, Path):
        path = _existing_file(value)
        if path:
            return _detect_file_source(path)
        return "raw_urls"

    if isinstance(value, Mapping):
        return _detect_json_source(value)
    if isinstance(value, list) and value:
        if all(item is None or isinstance(item, Mapping) for item in value) and any(
            isinstance(item, Mapping) for item in value
        ):
            return _detect_json_source(value)
    return "raw_urls"


def _detect_file_source(path: Path) -> str:
    if _looks_like_sqlite(path):
        tables = _sqlite_tables(path)
        if {"urls", "visits"} <= tables:
            return "chrome_history"
        if {"moz_places", "moz_historyvisits"} <= tables:
            return "firefox_history"
    if path.suffix.lower() == ".jsonl":
        return "history_jsonl"
    if path.suffix.lower() == ".json":
        return _detect_json_source(_load_json(path))
    return "raw_urls"


def _existing_file(value: str | Path) -> Path | None:
    if isinstance(value, str) and ("\n" in value or "\r" in value):
        return None
    try:
        path = Path(value)
        return path if path.exists() and path.is_file() else None
    except (OSError, ValueError):
        return None


def _looks_like_sqlite(path: Path) -> bool:
    try:
        with path.open("rb") as handle:
            return handle.read(16) == b"SQLite format 3\x00"
    except OSError:
        return False


def _sqlite_tables(path: Path) -> set[str]:
    with _open_sqlite_readonly(path) as db:
        return {row[0] for row in db.execute("SELECT name FROM sqlite_master WHERE type='table'")}


def _open_sqlite_readonly(path: Path) -> sqlite3.Connection:
    return sqlite3.connect(f"file:{path.resolve()}?mode=ro", uri=True)


def _load_json(value: Any) -> Any:
    if isinstance(value, Path):
        return json.loads(value.read_text(encoding="utf-8"))
    if isinstance(value, str):
        stripped = value.lstrip()
        if stripped.startswith(("[", "{")):
            return json.loads(value)
        path = _existing_file(value)
        if path:
            return json.loads(path.read_text(encoding="utf-8"))
        return json.loads(value)
    return value


def _activity_records(data: Any) -> list[Mapping[str, Any]]:
    if isinstance(data, list):
        return [item for item in data if isinstance(item, Mapping)]
    if isinstance(data, Mapping):
        for key in ("items", "events", "activity", "records"):
            records = data.get(key)
            if isinstance(records, list):
                return [item for item in records if isinstance(item, Mapping)]
        return [data]
    return []


def _detect_json_source(data: Any) -> str:
    records = _activity_records(data)
    candidates = [
        record
        for record in records
        if record.get("titleUrl")
        or record.get("url")
        or (
            "youtube" in str(record.get("header", "")).casefold()
            and _searched_for_title(_clean_title(record.get("title")))
        )
    ]
    if candidates and all(_looks_like_youtube_activity(record) for record in candidates):
        return "youtube_activity"
    if candidates and all(
        _looks_like_google_activity(record) or _looks_like_youtube_activity(record)
        for record in candidates
    ):
        return "google_activity"
    return "json_urls"


def _looks_like_youtube_activity(record: Mapping[str, Any]) -> bool:
    header = str(record.get("header", "")).casefold()
    if "youtube" in header:
        return True
    url = record.get("titleUrl") or record.get("url")
    return bool(url) and _is_youtube_url(str(url))


def _looks_like_google_activity(record: Mapping[str, Any]) -> bool:
    header = str(record.get("header", "")).strip()
    has_time = any(key in record for key in ("time", "timestamp"))
    has_url = any(key in record for key in ("titleUrl", "url"))
    return bool(header) and has_time and has_url


def _parse_time(value: Any) -> datetime | None:
    if value is None:
        return None
    if isinstance(value, datetime):
        return value if value.tzinfo else value.replace(tzinfo=timezone.utc)
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        number = float(value)
        if not math.isfinite(number):
            return None
        magnitude = abs(number)
        if magnitude >= 10**17:
            number /= 1_000_000_000
        elif magnitude >= 10**14:
            number /= 1_000_000
        elif magnitude >= 10**11:
            number /= 1_000
        try:
            return datetime.fromtimestamp(number, tz=timezone.utc)
        except (OverflowError, OSError, ValueError):
            return None
    text = str(value).strip()
    if not text:
        return None
    if text.endswith("Z"):
        text = text[:-1] + "+00:00"
    try:
        parsed = datetime.fromisoformat(text)
        return parsed if parsed.tzinfo else parsed.replace(tzinfo=timezone.utc)
    except ValueError:
        return None


def _record_time(record: Mapping[str, Any], *keys: str) -> datetime | None:
    for key in keys:
        if key not in record:
            continue
        value = record.get(key)
        if value is None or value == "":
            return None
        parsed = _parse_time(value)
        if parsed is None:
            raise ValueError(f"unsupported history timestamp in {key}: {value!r}")
        return parsed
    return None


def _raw_urls(value: Any) -> Iterator[HistoryEntry]:
    if isinstance(value, (str, Path)):
        path = _existing_file(value)
        if path:
            lines = path.read_text(encoding="utf-8").splitlines()
        elif isinstance(value, str):
            lines = value.splitlines()
        else:
            lines = [str(value)]
    else:
        lines = value

    for item in lines:
        if isinstance(item, HistoryEntry):
            yield item
            continue
        if isinstance(item, Mapping):
            url = item.get("url") or item.get("titleUrl")
            if url:
                yield HistoryEntry(
                    url=str(url),
                    visited_at=_record_time(item, "visited_at", "time", "timestamp"),
                    title=_clean_title(item.get("title")),
                    source=str(item.get("source") or "raw_urls"),
                    kind=str(item.get("kind") or "visit"),
                    query=_optional_text(item.get("query")),
                )
            continue
        if item is None:
            continue
        text = str(item).strip()
        if text and not text.startswith("#"):
            yield HistoryEntry(url=text, source="raw_urls")


def _google_activity(value: Any) -> Iterator[HistoryEntry]:
    for record in _activity_records(_load_json(value)):
        if _looks_like_youtube_activity(record):
            entry = _youtube_entry(record)
            if entry:
                yield entry
            continue
        url = record.get("titleUrl") or record.get("url")
        if not url:
            continue
        title = _activity_title(record.get("title"))
        query = _google_query(str(url), title)
        yield HistoryEntry(
            url=str(url),
            visited_at=_record_time(record, "time", "timestamp"),
            title=title,
            source="google_activity",
            kind="search" if query else "visit",
            query=query,
        )


def _youtube_activity(value: Any) -> Iterator[HistoryEntry]:
    for record in _activity_records(_load_json(value)):
        entry = _youtube_entry(record)
        if entry:
            yield entry


def _youtube_entry(record: Mapping[str, Any]) -> HistoryEntry | None:
    url = record.get("titleUrl") or record.get("url")
    title = _activity_title(record.get("title"))
    query = _youtube_query(str(url) if url else None, title)
    if not url and query:
        from urllib.parse import quote_plus

        url = f"https://www.youtube.com/results?search_query={quote_plus(query)}"
    if not url:
        return None
    return HistoryEntry(
        url=str(url),
        visited_at=_record_time(record, "time", "timestamp"),
        title=title,
        source="youtube_activity",
        kind="search" if query else "visit",
        query=query,
    )


def _youtube_query(url: str | None, title: str | None) -> str | None:
    if url and _is_youtube_url(url):
        try:
            parsed = urlparse(url)
            if parsed.path.rstrip("/") == "/results":
                values = parse_qs(parsed.query).get("search_query")
                if values and values[0].strip():
                    return values[0].strip()
        except ValueError:
            pass
    query = _searched_for_title(title)
    return query or None


def _google_query(url: str | None, title: str | None) -> str | None:
    query = _searched_for_title(title)
    if query:
        return query
    if not url:
        return None
    try:
        parsed = urlparse(url)
        host = (parsed.hostname or "").casefold().rstrip(".")
        if (host == "google.com" or host.endswith(".google.com")) and parsed.path.rstrip("/") == "/search":
            values = parse_qs(parsed.query).get("q")
            if values and values[0].strip():
                return values[0].strip()
    except ValueError:
        pass
    return None


def _searched_for_title(title: str | None) -> str | None:
    if not title:
        return None
    prefix = "searched for "
    if title[: len(prefix)].casefold() == prefix:
        query = title[len(prefix) :].strip()
        return query or None
    return None


def _is_youtube_url(url: str) -> bool:
    try:
        host = (urlparse(url).hostname or "").casefold().rstrip(".")
    except ValueError:
        return False
    return host == "youtube.com" or host.endswith(".youtube.com") or host == "youtu.be"


def _chrome_history(value: Any) -> Iterator[HistoryEntry]:
    path = Path(value)
    query = """
        SELECT visits.id, urls.url, urls.title, visits.visit_time
        FROM visits
        LEFT JOIN urls ON urls.id = visits.url
        ORDER BY visits.id
    """
    epoch = datetime(1601, 1, 1, tzinfo=timezone.utc)
    with _open_sqlite_readonly(path) as db:
        for visit_id, url, title, visit_time in db.execute(query):
            if url is None:
                raise ValueError(f"Chrome history visit {visit_id} references a missing URL row")
            visited_at = None
            if visit_time is not None:
                visited_at = epoch + timedelta(microseconds=int(visit_time))
            yield HistoryEntry(
                url=str(url),
                visited_at=visited_at,
                title=_clean_title(title),
                source="chrome_history",
            )


def _firefox_history(value: Any) -> Iterator[HistoryEntry]:
    path = Path(value)
    query = """
        SELECT moz_historyvisits.id, moz_places.url, moz_places.title, moz_historyvisits.visit_date
        FROM moz_historyvisits
        LEFT JOIN moz_places ON moz_places.id = moz_historyvisits.place_id
        ORDER BY moz_historyvisits.id
    """
    epoch = datetime(1970, 1, 1, tzinfo=timezone.utc)
    with _open_sqlite_readonly(path) as db:
        for visit_id, url, title, visit_date in db.execute(query):
            if url is None:
                raise ValueError(f"Firefox history visit {visit_id} references a missing place row")
            visited_at = None
            if visit_date is not None:
                visited_at = epoch + timedelta(microseconds=int(visit_date))
            yield HistoryEntry(
                url=str(url),
                visited_at=visited_at,
                title=_clean_title(title),
                source="firefox_history",
            )


def _history_jsonl(value: Any) -> Iterator[HistoryEntry]:
    path = _existing_file(value)
    if not path:
        raise ValueError("history_jsonl requires a JSONL file")
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if not line.strip():
            continue
        try:
            record = json.loads(line)
        except json.JSONDecodeError as exc:
            raise ValueError(f"invalid history JSONL at line {line_number}") from exc
        if not isinstance(record, Mapping) or not record.get("url"):
            raise ValueError(f"invalid history JSONL record at line {line_number}")
        yield HistoryEntry(
            url=str(record["url"]),
            visited_at=_record_time(record, "visited_at"),
            title=_clean_title(record.get("title")),
            source=str(record.get("source") or "history_jsonl"),
            kind=str(record.get("kind") or "visit"),
            query=_optional_text(record.get("query")),
        )


def _json_urls(value: Any) -> Iterator[HistoryEntry]:
    data = _load_json(value)

    def walk(node: Any) -> Iterator[HistoryEntry]:
        if isinstance(node, Mapping):
            url = node.get("url") or node.get("titleUrl") or node.get("href")
            if url:
                yield HistoryEntry(
                    url=str(url),
                    visited_at=_record_time(node, "visited_at", "time", "timestamp"),
                    title=_clean_title(node.get("title") or node.get("name")),
                    source="json_urls",
                    kind=str(node.get("kind") or "visit"),
                    query=_optional_text(node.get("query")),
                )
                return
            for child in node.values():
                yield from walk(child)
        elif isinstance(node, list):
            for child in node:
                yield from walk(child)

    yield from walk(data)


def _clean_title(value: Any) -> str | None:
    if value is None:
        return None
    title = str(value).strip()
    return title or None


def _activity_title(value: Any) -> str | None:
    title = _clean_title(value)
    if title and title.startswith("Visited "):
        title = title[len("Visited ") :]
    return title or None


def _optional_text(value: Any) -> str | None:
    if value is None:
        return None
    text = str(value).strip()
    return text or None


register_history_adapter("raw_urls", _raw_urls)
register_history_adapter("google_activity", _google_activity)
register_history_adapter("youtube_activity", _youtube_activity)
register_history_adapter("chrome_history", _chrome_history)
register_history_adapter("firefox_history", _firefox_history)
register_history_adapter("history_jsonl", _history_jsonl)
register_history_adapter("json_urls", _json_urls)
