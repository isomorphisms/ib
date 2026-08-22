from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from ib.inspect import StorageInspector


class StorageInspectorTests(unittest.TestCase):
    def test_classifies_storage_without_treating_indices_as_canonical(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "index").mkdir()
            (root / "cache").mkdir()
            (root / "snapshots" / "abc").mkdir(parents=True)
            (root / "secrets").mkdir()
            (root / "tabs" / "tab-a").mkdir(parents=True)

            (root / "index" / "visits.jsonl").write_text("{}\n", encoding="utf-8")
            (root / "index" / "hosts.tsv").write_text("host\n", encoding="utf-8")
            (root / "cache" / "renderer.bin").write_bytes(b"cache")
            (root / "snapshots" / "abc" / "body").write_bytes(b"snapshot")
            (root / "secrets" / "cookies.txt").write_text("private", encoding="utf-8")
            (root / "tabs" / "tab-a" / "tab.txt").write_text("id tab-a\n", encoding="utf-8")

            files = {row.path: row.kind for row in StorageInspector(root).files()}

            self.assertEqual(files["index/visits.jsonl"], "canonical")
            self.assertEqual(files["index/hosts.tsv"], "derived")
            self.assertEqual(files["cache/renderer.bin"], "cache")
            self.assertEqual(files["snapshots/abc/body"], "snapshot")
            self.assertEqual(files["secrets/cookies.txt"], "secret")
            self.assertEqual(files["tabs/tab-a/tab.txt"], "canonical")

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

    def test_recent_visits_are_a_bounded_tail_of_canonical_import_order(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            index = root / "index"
            index.mkdir()
            with (index / "visits.jsonl").open("w", encoding="utf-8") as output:
                for number in range(5):
                    output.write(json.dumps({"url": f"https://example.test/{number}", "source": "test"}) + "\n")

            rows = StorageInspector(root).visits(limit=2)

            self.assertEqual([row["url"] for row in rows], ["https://example.test/3", "https://example.test/4"])
            self.assertEqual([row["_line"] for row in rows], [4, 5])
            self.assertEqual(StorageInspector(root).visit_count(), 5)

    def test_secret_files_and_path_escape_are_not_readable(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "secrets").mkdir()
            (root / "secrets" / "tokens.txt").write_text("nope", encoding="utf-8")
            outside = root.parent / "outside-inspector-test.txt"
            outside.write_text("outside", encoding="utf-8")
            self.addCleanup(outside.unlink, missing_ok=True)

            inspector = StorageInspector(root)

            with self.assertRaises(PermissionError):
                inspector.read_text("secrets/tokens.txt")
            with self.assertRaises(ValueError):
                inspector.read_text("../outside-inspector-test.txt")

    def test_render_text_separates_files_tabs_and_visits(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "index").mkdir()
            (root / "tabs" / "T1").mkdir(parents=True)
            (root / "index" / "visits.jsonl").write_text(
                json.dumps({"url": "https://example.test/", "source": "raw_urls"}) + "\n",
                encoding="utf-8",
            )
            (root / "tabs" / "T1" / "tab.txt").write_text(
                "id T1\nstate sleeping\nlabel reference\n",
                encoding="utf-8",
            )

            rendered = StorageInspector(root).render_text(limit=10)

            self.assertIn("IB STORAGE", rendered)
            self.assertIn("FILES", rendered)
            self.assertIn("TAB RECORDS", rendered)
            self.assertIn("T1 state=sleeping labels=reference", rendered)
            self.assertIn("RECENT IMPORTED VISITS", rendered)
            self.assertIn("https://example.test/", rendered)


if __name__ == "__main__":
    unittest.main()
