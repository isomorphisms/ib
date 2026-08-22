from __future__ import annotations

import json
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
    Google/YouTube Takeout JSON, or a raw URL text/list input.
    """
    adapter_name = detect_history_source(value) if source == "auto" else source
    try:
        adapter = _ADAPTERS[adapter_name]
    except KeyError as exc:
        raise ValueError(f"unknown history source: {adapter_name}") from exc

    entries: list[HistoryEntry] = []
    for order, entry in enumerate(adapter(value)):
        url = entry.url.strip()
        if not url:
            continue
        entries.append(replace(entry, url=url, import_order=order))
    return entries


def detect_history_source(value: Any) -> str:
    if isinstance(value, (str, Path)):
        path = Path(value)
        if path.exists() and path.is_file():
            if _looks_like_sqlite(path):
                tables = _sqlite_tables(path)
                if {"urls", "visits"} <= tables:
                    return "chrome_history"
                if {"moz_places", "moz_historyvisits"} <= tables:
                    return "firefox_history"
            if path.suffix.lower() == ".json":
                return _detect_json_source(_load_json(path))
            return "raw_urls"
        if isinstance(value, str) and value.lstrip().startswith(("[", "{")):
            try:
                return _detect_json_source(json.loads(value))
            except json.JSONDecodeError:
                pass
        return "raw_urls"

    if isinstance(value, Mapping):
        return _detect_json_source(value)
    if isinstance(value, list) and value and isinstance(value[0], Mapping):
        return _detect_json_source(value)
    return "raw_urls"


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
        path = Path(value)
        if path.exists():
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
    if any(_looks_like_youtube_activity(record) for record in records[:25]):
        return "youtube_activity"
    if any(_looks_like_google_activity(record) for record in records[:25]):
        return "google_activity"
    return "json_urls"


def _looks_like_youtube_activity(record: Mapping[str, Any]) -> bool:
    header = str(record.get("header", "")).lower()
    title = str(record.get("title", "")).lower()
    url = str(record.get("titleUrl") or record.get("url") or "").lower()
    return "youtube" in header or "youtube.com" in url or title.startswith("searched for")


def _looks_like_google_activity(record: Mapping[str, Any]) -> bool:
    return any(key in record for key in ("titleUrl", "header", "time"))


def _parse_time(value: Any) -> datetime | None:
    if value is None:
        return None
    if isinstance(value, datetime):
        return value if value.tzinfo else value.replace(tzinfo=timezone.utc)
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


def _raw_urls(value: Any) -> Iterator[HistoryEntry]:
    if isinstance(value, (str, Path)):
        path = Path(value)
        if path.exists() and path.is_file():
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
                    visited_at=_parse_time(item.get("visited_at") or item.get("time")),
                    title=_clean_title(item.get("title")),
                    source=str(item.get("source") or "raw_urls"),
                )
            continue
        text = str(item).strip()
        if text and not text.startswith("#"):
            yield HistoryEntry(url=text, source="raw_urls")


def _google_activity(value: Any) -> Iterator[HistoryEntry]:
    for record in _activity_records(_load_json(value)):
        url = record.get("titleUrl") or record.get("url")
        if not url:
            continue
        title = _clean_title(record.get("title"))
        yield HistoryEntry(
            url=str(url),
            visited_at=_parse_time(record.get("time") or record.get("timestamp")),
            title=title,
            source="google_activity",
            kind="visit",
        )


def _youtube_activity(value: Any) -> Iterator[HistoryEntry]:
    for record in _activity_records(_load_json(value)):
        url = record.get("titleUrl") or record.get("url")
        title = _clean_title(record.get("title"))
        query = _youtube_query(str(url) if url else None, title)
        if not url and query:
            from urllib.parse import quote_plus
            url = f"https://www.youtube.com/results?search_query={quote_plus(query)}"
        if not url:
            continue
        yield HistoryEntry(
            url=str(url),
            visited_at=_parse_time(record.get("time") or record.get("timestamp")),
            title=title,
            source="youtube_activity",
            kind="search" if query else "visit",
            query=query,
        )


def _youtube_query(url: str | None, title: str | None) -> str | None:
    if url:
        values = parse_qs(urlparse(url).query).get("search_query")
        if values and values[0].strip():
            return values[0].strip()
    if title:
        prefix = "Searched for "
        if title.startswith(prefix):
            query = title[len(prefix):].strip()
            return query or None
    return None


def _chrome_history(value: Any) -> Iterator[HistoryEntry]:
    path = Path(value)
    query = """
        SELECT urls.url, urls.title, visits.visit_time
        FROM visits
        JOIN urls ON urls.id = visits.url
        ORDER BY visits.id
    """
    epoch = datetime(1601, 1, 1, tzinfo=timezone.utc)
    with _open_sqlite_readonly(path) as db:
        for url, title, visit_time in db.execute(query):
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
        SELECT moz_places.url, moz_places.title, moz_historyvisits.visit_date
        FROM moz_historyvisits
        JOIN moz_places ON moz_places.id = moz_historyvisits.place_id
        ORDER BY moz_historyvisits.id
    """
    with _open_sqlite_readonly(path) as db:
        for url, title, visit_date in db.execute(query):
            visited_at = None
            if visit_date is not None:
                visited_at = datetime.fromtimestamp(int(visit_date) / 1_000_000, tz=timezone.utc)
            yield HistoryEntry(
                url=str(url),
                visited_at=visited_at,
                title=_clean_title(title),
                source="firefox_history",
            )


def _json_urls(value: Any) -> Iterator[HistoryEntry]:
    data = _load_json(value)

    def walk(node: Any) -> Iterator[HistoryEntry]:
        if isinstance(node, Mapping):
            url = node.get("url") or node.get("titleUrl") or node.get("href")
            if url:
                yield HistoryEntry(
                    url=str(url),
                    visited_at=_parse_time(node.get("visited_at") or node.get("time") or node.get("timestamp")),
                    title=_clean_title(node.get("title") or node.get("name")),
                    source="json_urls",
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
    if title.startswith("Visited "):
        title = title[len("Visited "):]
    return title or None


register_history_adapter("raw_urls", _raw_urls)
register_history_adapter("google_activity", _google_activity)
register_history_adapter("youtube_activity", _youtube_activity)
register_history_adapter("chrome_history", _chrome_history)
register_history_adapter("firefox_history", _firefox_history)
register_history_adapter("json_urls", _json_urls)
