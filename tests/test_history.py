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

    def test_google_search_activity_is_not_misclassified_as_youtube(self):
        payload = [
            {
                "header": "Search",
                "title": "Searched for ib browser",
                "titleUrl": "https://www.google.com/search?q=ib+browser",
                "time": "2026-08-22T14:00:00Z",
            }
        ]
        entries = ingest_history(payload)
        self.assertEqual(entries[0].source, "google_activity")
        self.assertEqual(entries[0].kind, "search")
        self.assertEqual(entries[0].query, "ib browser")

    def test_google_detection_ignores_non_url_metadata_records(self):
        payload = [
            {"header": "Search", "products": ["Search"]},
            {
                "header": "Search",
                "title": "Searched for durable tabs",
                "titleUrl": "https://www.google.com/search?q=durable+tabs",
                "time": "2026-08-22T13:00:00Z",
            },
        ]
        entries = ingest_history(payload)
        self.assertEqual(len(entries), 1)
        self.assertEqual(entries[0].source, "google_activity")
        self.assertEqual(entries[0].query, "durable tabs")

    def test_mixed_google_and_youtube_activity_classifies_each_record(self):
        payload = [
            {
                "header": "Search",
                "title": "Searched for browser persistence",
                "titleUrl": "https://www.google.com/search?q=browser+persistence",
                "time": "2026-08-22T13:00:00Z",
            },
            {
                "header": "YouTube",
                "title": "Searched for conway wallpaper groups",
                "titleUrl": "https://www.youtube.com/results?search_query=conway+wallpaper+groups",
                "time": "2026-08-22T14:00:00Z",
            },
        ]
        entries = ingest_history(payload)
        self.assertEqual([entry.source for entry in entries], ["google_activity", "youtube_activity"])
        self.assertEqual([entry.query for entry in entries], ["browser persistence", "conway wallpaper groups"])

    def test_generic_json_time_field_stays_generic_and_keeps_title_exact(self):
        payload = [
            {
                "url": "https://example.com/rome",
                "title": "Visited places in Rome",
                "time": "2026-08-22T14:00:00Z",
            }
        ]
        entries = ingest_history(payload)
        self.assertEqual(entries[0].source, "json_urls")
        self.assertEqual(entries[0].title, "Visited places in Rome")

    def test_generic_json_numeric_millisecond_timestamp(self):
        payload = [{"url": "https://example.com/", "timestamp": 1_787_400_000_000}]
        entries = ingest_history(payload)
        self.assertEqual(entries[0].visited_at, datetime(2026, 8, 22, 12, tzinfo=timezone.utc))

    def test_invalid_generic_timestamp_fails_loudly(self):
        payload = [{"url": "https://example.com/", "timestamp": "sometime yesterday"}]
        with self.assertRaises(ValueError):
            ingest_history(payload)

    def test_fake_youtube_substring_does_not_relabel_generic_json(self):
        payload = [
            {
                "url": "https://example.com/?next=https://youtube.com/results?search_query=cats",
                "title": "Searched for cats",
            }
        ]
        entries = ingest_history(payload)
        self.assertEqual(entries[0].source, "json_urls")
        self.assertEqual(entries[0].kind, "visit")
        self.assertIsNone(entries[0].query)

    def test_long_inline_json_is_not_treated_as_a_filename(self):
        payload = json.dumps([{"url": "https://example.com/" + "x" * 5000}])
        entries = ingest_history(payload)
        self.assertEqual(len(entries), 1)
        self.assertTrue(entries[0].url.startswith("https://example.com/"))

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

    def test_youtube_watch_url_with_search_query_parameter_is_still_a_visit(self):
        payload = [
            {
                "header": "YouTube",
                "title": "Watched Example",
                "titleUrl": "https://www.youtube.com/watch?v=abc&search_query=not-a-search",
                "time": "2026-08-22T14:00:00Z",
            }
        ]
        entries = ingest_history(payload)
        self.assertEqual(entries[0].kind, "visit")
        self.assertIsNone(entries[0].query)

    def test_chrome_sqlite_keeps_duplicate_visits_in_row_order(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "History"
            db = sqlite3.connect(path)
            db.execute("CREATE TABLE urls (id INTEGER PRIMARY KEY, url TEXT, title TEXT)")
            db.execute("CREATE TABLE visits (id INTEGER PRIMARY KEY, url INTEGER, visit_time INTEGER)")
            db.execute("INSERT INTO urls VALUES (1, 'https://example.com/chrome', 'Visited places in Rome')")
            first = 13_438_780_800_000_000
            db.execute("INSERT INTO visits VALUES (10, 1, ?)", (first,))
            db.execute("INSERT INTO visits VALUES (11, 1, ?)", (first + 1_000_000,))
            db.commit()
            db.close()
            entries = ingest_history(path)
            self.assertEqual([entry.import_order for entry in entries], [0, 1])
            self.assertEqual([entry.url for entry in entries], ["https://example.com/chrome"] * 2)
            self.assertEqual([entry.title for entry in entries], ["Visited places in Rome"] * 2)
            self.assertLess(entries[0].visited_at, entries[1].visited_at)

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
            self.assertEqual(entries[0].visited_at, datetime(2026, 8, 22, 12, tzinfo=timezone.utc))

    def test_orphaned_sqlite_visit_fails_instead_of_disappearing(self):
        with tempfile.TemporaryDirectory() as directory:
            chrome = Path(directory) / "History"
            db = sqlite3.connect(chrome)
            db.execute("CREATE TABLE urls (id INTEGER PRIMARY KEY, url TEXT, title TEXT)")
            db.execute("CREATE TABLE visits (id INTEGER PRIMARY KEY, url INTEGER, visit_time INTEGER)")
            db.execute("INSERT INTO visits VALUES (1, 999, 13438780800000000)")
            db.commit()
            db.close()
            with self.assertRaisesRegex(ValueError, "missing URL row"):
                ingest_history(chrome)

            firefox = Path(directory) / "places.sqlite"
            db = sqlite3.connect(firefox)
            db.execute("CREATE TABLE moz_places (id INTEGER PRIMARY KEY, url TEXT, title TEXT)")
            db.execute("CREATE TABLE moz_historyvisits (id INTEGER PRIMARY KEY, place_id INTEGER, visit_date INTEGER)")
            db.execute("INSERT INTO moz_historyvisits VALUES (1, 999, 1787400000000000)")
            db.commit()
            db.close()
            with self.assertRaisesRegex(ValueError, "missing place row"):
                ingest_history(firefox)

    def test_chronology_is_newest_first_but_stable_for_equal_or_missing_times(self):
        entries = ingest_history(
            [
                {"url": "https://example.com/a", "time": "2026-08-22T10:00:00Z"},
                {"url": "https://example.com/b", "time": "2026-08-22T10:00:00Z"},
                {"url": "https://example.com/c", "time": "2026-08-22T11:00:00Z"},
                {"url": "https://example.com/d"},
                {"url": "https://example.com/e"},
            ],
            source="raw_urls",
        )
        indices = IndexBuilder().build(entries)
        self.assertEqual(indices.chronology, [2, 0, 1, 3, 4])

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
            self.assertTrue((Path(directory) / "sources.tsv").exists())
            self.assertTrue((Path(directory) / "days.tsv").exists())

    def test_written_visit_stream_rebuilds_identical_indices(self):
        entries = ingest_history(
            [
                {
                    "url": "https://example.com/a?q=one",
                    "title": "Visited places in Rome",
                    "time": "2026-08-22T10:00:00-04:00",
                    "source": "fixture",
                },
                {
                    "url": "https://example.com/a?q=one",
                    "title": "duplicate",
                    "time": "2026-08-22T10:00:00-04:00",
                    "source": "fixture",
                },
                {"url": "https://例え.テスト/道", "title": "Unicode path", "source": "fixture"},
            ],
            source="raw_urls",
        )
        with tempfile.TemporaryDirectory() as directory:
            first_root = Path(directory) / "first"
            second_root = Path(directory) / "second"
            first = IndexBuilder().build(entries)
            write_plaintext_indices(first, first_root)

            rebuilt_entries = ingest_history(first_root / "visits.jsonl")
            self.assertEqual([entry.as_dict() for entry in rebuilt_entries], [entry.as_dict() for entry in entries])
            second = IndexBuilder().build(rebuilt_entries)
            write_plaintext_indices(second, second_root)

            first_files = sorted(path.name for path in first_root.iterdir())
            second_files = sorted(path.name for path in second_root.iterdir())
            self.assertEqual(first_files, second_files)
            for name in first_files:
                self.assertEqual((first_root / name).read_bytes(), (second_root / name).read_bytes(), name)


if __name__ == "__main__":
    unittest.main()
