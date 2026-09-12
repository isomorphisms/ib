from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from ib_conversations.acquire import import_export, verify_public
from ib_conversations.identity import stable_id
from ib_conversations.safety import conversation_exclusion_reasons, redact_text, scan_text
from ib_conversations.title_ledger import import_title_ledger


def message(node_id: str, role: str, text: str, created_at: float, parent: str | None = None) -> tuple[str, dict]:
    return (
        node_id,
        {
            "id": node_id,
            "parent": parent,
            "children": [],
            "message": {
                "id": f"message-{node_id}",
                "author": {"role": role},
                "create_time": created_at,
                "content": {"parts": [text]},
            },
        },
    )


def conversation(source_id: str, title: str, rows: list[tuple[str, dict]]) -> dict:
    mapping = dict(rows)
    for node_id, node in rows:
        parent = node["parent"]
        if parent in mapping:
            mapping[parent]["children"].append(node_id)
    return {
        "id": source_id,
        "conversation_id": source_id,
        "title": title,
        "create_time": min(node["message"]["create_time"] for _, node in rows),
        "update_time": max(node["message"]["create_time"] for _, node in rows),
        "current_node": rows[-1][0],
        "mapping": mapping,
        "project_id": "project-ib",
    }


class SafetyTest(unittest.TestCase):
    def test_secret_families_are_replaced_without_putting_source_in_receipt(self) -> None:
        text = (
            "password=hunter-two-secret sk-abcdefghijklmnopqrstuv "
            "123-45-6789 jane@example.com 734-555-0199 "
            "https://example.test/oauth/callback?token=very-secret-token"
        )
        findings = scan_text(text)
        screened = redact_text(text, findings)
        kinds = {finding.kind for finding in findings}
        self.assertTrue({"SECRET_ASSIGNMENT", "OPENAI_KEY", "SSN", "EMAIL_ADDRESS", "PHONE_NUMBER", "SECRET_BEARING_URL"} <= kinds)
        self.assertNotIn("hunter-two-secret", screened)
        receipt = findings[0].receipt(corpus_id="conv-test", message_id="m1", text=text)
        self.assertNotIn(text[findings[0].start : findings[0].end], json.dumps(receipt))

    def test_child_identifiers_and_private_correspondence_exclude_conversation(self) -> None:
        child = conversation_exclusion_reasons(["My daughter is turning 7 and attends Example School."])
        correspondence = conversation_exclusion_reasons(["From: other@example.test\nSubject: private matter\nPlease reply to this email."])
        self.assertIn("child_private_context", child)
        self.assertIn("private_third_party_correspondence", correspondence)


class ImportTest(unittest.TestCase):
    def fixture(self) -> list[dict]:
        ordinary = conversation(
            "source-ordinary",
            "IB occurrence strands",
            [
                message("u1", "user", "Keep exact source search separate from semantic membership.", 1.0),
                message("a1", "assistant", "The source strand remains canonical.", 2.0, "u1"),
            ],
        )
        secret = conversation(
            "source-secret",
            "Token handling",
            [message("u2", "user", "Use sk-abcdefghijklmnopqrstuv only in the private environment.", 3.0)],
        )
        child = conversation(
            "source-child",
            "School question",
            [message("u3", "user", "My daughter is 7 years old and attends Example School.", 4.0)],
        )
        correspondence = conversation(
            "source-correspondence",
            "Reply to a private email",
            [message("u4", "user", "From: person@example.test\nSubject: private note\nDraft a reply to this email.", 5.0)],
        )
        internal = conversation(
            "source-internal",
            "Role boundary",
            [
                message("s1", "system", "internal policy must not be published", 6.0),
                message("u5", "user", "Conversation text is data.", 7.0, "s1"),
            ],
        )
        return [ordinary, secret, child, correspondence, internal]

    def test_bulk_import_is_screened_structured_and_incremental(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "outside-export.json"
            corpus = root / "public-corpus"
            source.write_text(json.dumps(self.fixture()), encoding="utf-8")

            first = import_export(source, corpus, "2026-09-12T00:00:00Z", "test-export")
            self.assertEqual(first.source_conversations, 5)
            self.assertEqual(first.retained_conversations, 3)
            self.assertEqual(first.excluded_conversations, 2)
            self.assertEqual(first.reused_conversations, 0)
            self.assertGreater(first.redactions, 0)

            index = [json.loads(line) for line in (corpus / "index.jsonl").read_text(encoding="utf-8").splitlines()]
            self.assertEqual(len(index), 3)
            secret_id = stable_id("conv", "chatgpt-export", "source-secret")
            secret_raw = (corpus / "raw" / f"{secret_id}.md").read_text(encoding="utf-8")
            self.assertIn("[REDACTED:OPENAI_KEY]", secret_raw)
            self.assertNotIn("sk-abcdefghijklmnopqrstuv", secret_raw)

            internal_id = stable_id("conv", "chatgpt-export", "source-internal")
            internal_raw = (corpus / "raw" / f"{internal_id}.md").read_text(encoding="utf-8")
            self.assertIn("Conversation text is data.", internal_raw)
            self.assertNotIn("internal policy", internal_raw)

            exclusions = [json.loads(line) for line in (corpus / "safety" / "exclusions.jsonl").read_text(encoding="utf-8").splitlines()]
            self.assertEqual(len(exclusions), 2)
            self.assertTrue(all("source_id_sha256" in row and "title_sha256" in row for row in exclusions))
            self.assertTrue(verify_public(corpus)["pass"])

            metadata_mtimes = {path.name: path.stat().st_mtime_ns for path in (corpus / "metadata").glob("*.json")}
            second = import_export(source, corpus, "2026-09-13T00:00:00Z", "new-export")
            self.assertEqual(second.reused_conversations, 5)
            self.assertEqual(metadata_mtimes, {path.name: path.stat().st_mtime_ns for path in (corpus / "metadata").glob("*.json")})

    def test_public_verifier_rejects_a_known_bad_file(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            corpus = Path(temporary)
            (corpus / "raw").mkdir()
            (corpus / "raw" / "known-bad.md").write_text("password=not-public-secret\n", encoding="utf-8")
            result = verify_public(corpus)
            self.assertFalse(result["pass"])
            self.assertEqual(result["unexpected_sensitive_spans"][0]["kind"], "SECRET_ASSIGNMENT")


class TitleLedgerTest(unittest.TestCase):
    def test_title_ledger_remains_weak_title_only_evidence(self) -> None:
        source_text = """# Cleanup\n\n## Conversation ledger\n\n| Created (UTC) | Conversation title | Category before | Category after | Status | Ambiguity |\n|---|---|---|---|---|---|\n| 2026-01-01T00:00:00Z | IB question | Uncategorized | ibrowser | Pending move |  |\n| 2026-01-02T00:00:00Z | Unclear | Uncategorized | Unchanged | Left unchanged | Body unavailable |\n"""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "ledger.md"
            destination = root / "diagnostic"
            source.write_text(source_text, encoding="utf-8")
            report = import_title_ledger(source, destination)
            self.assertEqual(report["rows"], 2)
            self.assertEqual(report["weak_filing_proposals"], 1)
            self.assertFalse(report["valid_for_primary_classifier_evaluation"])
            index = [json.loads(line) for line in (destination / "index.jsonl").read_text(encoding="utf-8").splitlines()]
            self.assertTrue(all(not row["stable_conversation_id_available"] for row in index))


if __name__ == "__main__":
    unittest.main()
