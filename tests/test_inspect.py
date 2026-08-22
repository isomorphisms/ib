from __future__ import annotations

import json
import os
import tempfile
import unittest
from pathlib import Path

from ib import IndexBuilder, ingest_history, write_history_store
from ib.inspect import StorageInspector


class StorageInspectorTests(unittest.TestCase):
    def test_classification_uses_schema_paths_not_basenames(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "indexes").mkdir()
            (root / "cache").mkdir()
            (root / "snapshots" / "abc").mkdir(parents=True)
            (root / "tabs" / "tab-a").mkdir(parents=True)

            (root / "visits.jsonl").write_text("{}\n", encoding="utf-8")
            (root / "indexes" / "hosts.tsv").write_text("host\n", encoding="utf-8")
            (root / "indexes" / "opaque.bin").write_bytes(b"derived binary")
            (root / "cache" / "visits.jsonl").write_text("{}\n", encoding="utf-8")
            (root / "snapshots" / "abc" / "body").write_bytes(b"snapshot")
            (root / "tabs" / "tab-a" / "tab.txt").write_text("id tab-a\n", encoding="utf-8")
            (root / "tab.txt").write_text("not a tab record\n", encoding="utf-8")

            files = {row.path: row.kind for row in StorageInspector(root).files()}

            self.assertEqual(files["visits.jsonl"], "canonical")
            self.assertEqual(files["indexes/hosts.tsv"], "derived")
            self.assertEqual(files["indexes/opaque.bin"], "derived")
            self.assertEqual(files["cache/visits.jsonl"], "cache")
            self.assertEqual(files["snapshots/abc/body"], "snapshot")
            self.assertEqual(files["tabs/tab-a/tab.txt"], "canonical")
            self.assertEqual(files["tab.txt"], "unknown")


    def test_history_store_keeps_canonical_visits_out_of_disposable_indexes(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            indices = IndexBuilder().build(
                ingest_history("https://example.test/one\nhttps://example.test/two\n")
            )

            write_history_store(indices, root)

            self.assertTrue((root / "visits.jsonl").is_file())
            self.assertTrue((root / "indexes" / "chronology.tsv").is_file())
            self.assertFalse((root / "indexes" / "visits.jsonl").exists())
            files = {row.path: row.kind for row in StorageInspector(root).files()}
            self.assertEqual(files["visits.jsonl"], "canonical")
            self.assertEqual(files["indexes/chronology.tsv"], "derived")

    def test_secret_detection_is_conservative(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            root.mkdir(exist_ok=True)
            inspector = StorageInspector(root)

            self.assertEqual(inspector.classify("indexes/auth_token.txt"), "secret")
            self.assertEqual(inspector.classify("cache/session-cookies.sqlite"), "secret")
            self.assertEqual(inspector.classify("renderer/Login Data"), "secret")
            self.assertEqual(inspector.classify("renderer/Web Data"), "secret")

    def test_reads_tab_manifest_and_preserves_repeated_fields(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            tab = root / "tabs" / "01JTEST"
            tab.mkdir(parents=True)
            (tab / "tab.txt").write_text(
                "id 01JTEST\n"
                "state sleeping\n"
                "current_history 17\n"
                "label math\n"
                "label later\n",
                encoding="utf-8",
            )

            records = StorageInspector(root).tabs()

            self.assertEqual(len(records), 1)
            self.assertEqual(records[0].tab_id, "01JTEST")
            self.assertEqual(records[0].fields["state"], ("sleeping",))
            self.assertEqual(records[0].fields["label"], ("math", "later"))

    def test_physical_files_can_be_paged_without_materializing_the_whole_tree(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("a.txt", "b.txt", "c.txt", "d.txt"):
                (root / name).write_text(name, encoding="utf-8")

            inspector = StorageInspector(root)

            self.assertEqual([row.path for row in inspector.files(limit=2, offset=1)], ["b.txt", "c.txt"])
            self.assertEqual(inspector.overview()["files"], 4)

    def test_tabs_are_bounded_and_overview_does_not_parse_manifests(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for number in range(5):
                tab = root / "tabs" / f"T{number}"
                tab.mkdir(parents=True)
                (tab / "tab.txt").write_text(f"id T{number}\n", encoding="utf-8")

            inspector = StorageInspector(root)
            self.assertEqual(inspector.overview()["tab_records"], 5)
            self.assertEqual([row.tab_id for row in inspector.tabs(limit=2, offset=1)], ["T1", "T2"])

    def test_recent_visits_are_a_reverse_read_tail_with_byte_offsets(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            indexes = root / "indexes"
            indexes.mkdir()
            with (root / "visits.jsonl").open("w", encoding="utf-8") as output:
                for number in range(5000):
                    output.write(json.dumps({"url": f"https://example.test/{number}", "source": "test"}) + "\n")

            rows = StorageInspector(root).visits(limit=2)

            self.assertEqual([row["url"] for row in rows], ["https://example.test/4998", "https://example.test/4999"])
            self.assertLess(rows[0]["_offset"], rows[1]["_offset"])
            self.assertEqual(StorageInspector(root).visit_count(), 5000)

    def test_visit_tail_has_a_byte_budget(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            huge = json.dumps({"url": "https://example.test/" + "x" * 4096})
            (root / "visits.jsonl").write_text(huge + "\n" + huge + "\n", encoding="utf-8")

            rows = StorageInspector(root).visits(limit=2, max_bytes=1024)

            self.assertEqual(rows, [])

    def test_overview_uses_derived_visit_count_without_scanning_canonical_log(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            indexes = root / "indexes"
            indexes.mkdir()
            (root / "visits.jsonl").write_text("{}\n{}\n", encoding="utf-8")
            (indexes / "summary.json").write_text(json.dumps({"entries": 2}), encoding="utf-8")

            overview = StorageInspector(root).overview()

            self.assertEqual(overview["indexed_visits"], 2)

    @unittest.skipUnless(hasattr(os, "symlink"), "symlinks unavailable")
    def test_automatic_parsers_never_follow_symlinks(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "state"
            outside = Path(directory) / "outside"
            (root / "tabs").mkdir(parents=True)
            (root / "indexes").mkdir()
            outside.mkdir()
            (outside / "tab.txt").write_text("id LEAKED\nlabel secret\n", encoding="utf-8")
            (outside / "visits.jsonl").write_text(
                json.dumps({"url": "https://outside.test/"}) + "\n",
                encoding="utf-8",
            )

            os.symlink(outside / "tab.txt", root / "tabs" / "leak")
            tab_dir = root / "tabs" / "T1"
            tab_dir.mkdir()
            os.symlink(outside / "tab.txt", tab_dir / "tab.txt")
            os.symlink(outside / "visits.jsonl", root / "visits.jsonl")

            inspector = StorageInspector(root)
            files = {row.path: row for row in inspector.files()}

            self.assertEqual(inspector.tabs(), [])
            with self.assertRaises(OSError):
                inspector.visits()
            with self.assertRaises(OSError):
                inspector.visit_count()
            self.assertEqual(files["tabs/T1/tab.txt"].file_type, "symlink")
            self.assertFalse(files["tabs/T1/tab.txt"].readable)

    def test_unknown_cache_and_snapshot_content_are_metadata_only(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "cache").mkdir()
            (root / "snapshots" / "abc").mkdir(parents=True)
            (root / "mystery.txt").write_text("unknown", encoding="utf-8")
            (root / "cache" / "debug.txt").write_text("cache", encoding="utf-8")
            (root / "snapshots" / "abc" / "body").write_text("snapshot", encoding="utf-8")

            inspector = StorageInspector(root)

            (root / "indexes").mkdir()
            (root / "indexes" / "opaque.bin").write_bytes(b"derived binary")
            files = {row.path: row for row in inspector.files()}

            for path in ("mystery.txt", "cache/debug.txt", "snapshots/abc/body", "indexes/opaque.bin"):
                self.assertFalse(files[path].readable)
                with self.assertRaises(PermissionError):
                    inspector.read_text(path)

    def test_secret_files_and_path_escape_are_not_readable(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "indexes").mkdir()
            (root / "indexes" / "auth_token.txt").write_text("nope", encoding="utf-8")
            outside = root.parent / "outside-inspector-test.txt"
            outside.write_text("outside", encoding="utf-8")
            self.addCleanup(outside.unlink, missing_ok=True)

            inspector = StorageInspector(root)

            with self.assertRaises(PermissionError):
                inspector.read_text("indexes/auth_token.txt")
            with self.assertRaises(ValueError):
                inspector.read_text("../outside-inspector-test.txt")
            with self.assertRaises(ValueError):
                inspector.read_text(outside)

    def test_render_text_uses_full_category_names(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "indexes").mkdir()
            (root / "tabs" / "T1").mkdir(parents=True)
            (root / "visits.jsonl").write_text(
                json.dumps({"url": "https://example.test/", "source": "raw_urls"}) + "\n",
                encoding="utf-8",
            )
            (root / "indexes" / "summary.json").write_text(
                json.dumps({"entries": 1}),
                encoding="utf-8",
            )
            (root / "tabs" / "T1" / "tab.txt").write_text(
                "id T1\nstate sleeping\nlabel reference\n",
                encoding="utf-8",
            )

            rendered = StorageInspector(root).render_text(limit=10)

            self.assertIn("IB STORAGE", rendered)
            self.assertIn("indexed_visits 1", rendered)
            self.assertIn("canonical", rendered)
            self.assertIn("TAB RECORDS", rendered)
            self.assertIn("T1 state=sleeping labels=reference", rendered)
            self.assertIn("RECENT IMPORTED VISITS", rendered)
            self.assertIn("https://example.test/", rendered)


if __name__ == "__main__":
    unittest.main()
