#!/usr/bin/env python3

import tempfile
from pathlib import Path
import unittest

import prerender


class PreRendererTests(unittest.TestCase):
    def test_extracts_visible_text_but_not_script_or_style(self) -> None:
        html = """
        <html><head>
          <title>Small Example</title>
          <style>BODY SHOULD NOT APPEAR</style>
        </head><body>
          <h1>Hello</h1>
          <script>SECRET_SCRIPT_TEXT</script>
          <p>Useful words.</p>
          <img src="x" alt="diagram of a group">
        </body></html>
        """
        title, body = prerender.extract(html)
        self.assertEqual(title, "Small Example")
        self.assertIn("Hello", body)
        self.assertIn("Useful words.", body)
        self.assertIn("diagram of a group", body)
        self.assertNotIn("SECRET_SCRIPT_TEXT", body)
        self.assertNotIn("SHOULD NOT APPEAR", body)

    def test_text_preview_has_a_hard_byte_bound(self) -> None:
        preview = prerender.make_text_preview(
            "Title", "https://example.test/", "é" * 10000, 512
        )
        # The clipping marker is intentionally outside the payload limit, so
        # permit its small fixed overhead while ensuring the body is bounded.
        self.assertLess(len(preview.encode("utf-8")), 550)
        self.assertIn("clipped", preview)

    def test_bitmap_preview_is_small_and_capped_at_three_screens(self) -> None:
        pages = prerender.render_pbm_pages(
            "A recognizable line of text. " * 5000,
            width=180,
            height=320,
            max_screens=3,
        )
        self.assertEqual(len(pages), 3)
        for page in pages:
            self.assertTrue(page.startswith(b"P4\n180 320\n"))
            self.assertLess(len(page), 8 * 1024)
            self.assertNotEqual(set(page.split(b"\n", 2)[2]), {0})

    def test_short_page_produces_only_one_bitmap(self) -> None:
        pages = prerender.render_pbm_pages("HELLO", 180, 320, 3)
        self.assertEqual(len(pages), 1)

    def test_default_outputs_fit_small_preview_budget(self) -> None:
        html = "<title>Example</title><h1>Heading</h1><p>" + ("words " * 2000) + "</p>"
        title, body = prerender.extract(html)
        text = prerender.make_text_preview(title, "https://example.test/", body, 8192)
        pages = prerender.render_pbm_pages(title + "\n" + body, 180, 320, 3)
        self.assertLess(len(text.encode("utf-8")), 8300)
        self.assertLessEqual(sum(map(len, pages)), 24 * 1024)


if __name__ == "__main__":
    unittest.main()
