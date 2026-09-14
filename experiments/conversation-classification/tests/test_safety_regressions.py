from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from ib_conversations.safety import conversation_exclusion_reasons


class SafetyRegressionTest(unittest.TestCase):
    def test_ordinary_message_drafting_is_not_whole_conversation_exclusion(self) -> None:
        reasons = conversation_exclusion_reasons(
            ["Draft an email to the mechanic asking about compressor pressure and the service valve."]
        )
        self.assertEqual(reasons, [])

    def test_received_correspondence_header_is_excluded(self) -> None:
        reasons = conversation_exclusion_reasons(
            ["From: somebody@example.test\nSubject: private matter\nHere is the message body."]
        )
        self.assertIn("private_third_party_correspondence", reasons)


if __name__ == "__main__":
    unittest.main()
