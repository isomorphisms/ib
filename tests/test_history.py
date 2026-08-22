import json
import sqlite3
import tempfile
import unittest
from datetime import datetime, timezone
from pathlib import Path

from ib import IndexBuilder, ingest_history, write_plaintext_indices


class HistoryIngestionTests(unittest.TestCase):
    def test_raw_urls_keep_duplicates_and_import_order(self):
        entries = ingest_history("https://example.com/a\nhttps://example.com/a\nhttps://example.org/b\n")
        self.assertEqual([entry.import_order for entry in entries], [0, 1, 2])
        self.assertEqual([entry.url for entry in entries[:2]], ["https://example.com/a"] * 2)

    def test_google_takeout_activity(self):
        payload = [
            {
                "header": "Chrome",
                "title": "Visited Example",
                "titleUrl": "https://example.com/",
                "time": "2026-08-20T10:00:00Z",
            }
        ]
        entries = ingest_history(json.dumps(payload))
        self.assertEqual(entries[0].source, "google_activity")
        self.assertEqual(entries[0].title, "Example")
        self.assertEqual(entries[0].visited_at, datetime(2026, 8, 20, 10, tzinfo=timezone.utc))

    def test_youtube_search_activity_extracts_query(self):
        payload = [
            {
                "header": "YouTube",
                "title": "Searched for conway wallpaper groups",
                "titleUrl": "https://www.youtube.com/results?search_query=conway+wallpaper+groups",
                "time": "2026-08-22T14:00:00Z",
            }
        ]
        entries = ingest_history(payload)
        self.assertEqual(entries[0].kind, "search")
        self.assertEqual(entries[0].query, "conway wallpaper groups")

    def test_chrome_sqlite(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "History"
            db = sqlite3.connect(path)
            db.execute("CREATE TABLE urls (id INTEGER PRIMARY KEY, url TEXT, title TEXT)")
            db.execute("CREATE TABLE visits (id INTEGER PRIMARY KEY, url INTEGER, visit_time INTEGER)")
            db.execute("INSERT INTO urls VALUES (1, 'https://example.com/chrome', 'Chrome test')")
            webkit_epoch_us = 13_438_780_800_000_000
            db.execute("INSERT INTO visits VALUES (1, 1, ?)", (webkit_epoch_us,))
            db.commit()
            db.close()
            entries = ingest_history(path)
            self.assertEqual(entries[0].source, "chrome_history")
            self.assertEqual(entries[0].url, "https://example.com/chrome")

    def test_firefox_sqlite(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "places.sqlite"
            db = sqlite3.connect(path)
            db.execute("CREATE TABLE moz_places (id INTEGER PRIMARY KEY, url TEXT, title TEXT)")
            db.execute("CREATE TABLE moz_historyvisits (id INTEGER PRIMARY KEY, place_id INTEGER, visit_date INTEGER)")
            db.execute("INSERT INTO moz_places VALUES (1, 'https://example.org/firefox', 'Firefox test')")
            db.execute("INSERT INTO moz_historyvisits VALUES (1, 1, 1787400000000000)")
            db.commit()
            db.close()
            entries = ingest_history(path)
            self.assertEqual(entries[0].source, "firefox_history")
            self.assertEqual(entries[0].url, "https://example.org/firefox")

    def test_indices_preserve_visits_and_build_searchable_views(self):
        entries = ingest_history(
            [
                {"url": "https://example.com/math/tessellation", "title": "Wallpaper groups", "time": "2026-08-20T10:00:00Z"},
                {"url": "https://example.com/math/tessellation", "title": "Wallpaper groups", "time": "2026-08-21T10:00:00Z"},
                {"url": "https://other.example/browser", "title": "Persistent browser state", "time": "2026-08-22T10:00:00Z"},
            ],
            source="raw_urls",
        )
        indices = IndexBuilder().build(entries)
        self.assertEqual(indices.summary()["entries"], 3)
        self.assertEqual(indices.summary()["unique_urls"], 2)
        self.assertEqual(indices.by_url["https://example.com/math/tessellation"], [0, 1])
        self.assertEqual(indices.chronology, [2, 1, 0])
        self.assertEqual(indices.terms["wallpaper"], [0, 1])

        with tempfile.TemporaryDirectory() as directory:
            write_plaintext_indices(indices, directory)
            self.assertTrue((Path(directory) / "visits.jsonl").exists())
            self.assertIn("example.com", (Path(directory) / "hosts.tsv").read_text())


if __name__ == "__main__":
    unittest.main()
