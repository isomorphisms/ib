#!/usr/bin/env python3
"""Tiny reference pre-renderers for ib.

This is an executable design probe, not a choice of implementation language or
real rendering engine. It turns already-fetched HTML into:

* preview.txt: a small plaintext first paint
* screen-XX.pbm: up to three 1-bit, phone-shaped, text-derived rough previews

No CSS, JavaScript, images, fonts, forms, cookies, or subresources are used.
"""

from __future__ import annotations

import argparse
import html as html_module
from html.parser import HTMLParser
from pathlib import Path
import re
import sys
from typing import Iterable


DEFAULT_WIDTH = 180
DEFAULT_HEIGHT = 320
DEFAULT_SCREENS = 3
DEFAULT_TEXT_BYTES = 8 * 1024


class VisibleTextParser(HTMLParser):
    """Extract a title and boring visible-ish text from HTML."""

    SKIP = {"script", "style", "noscript", "template", "svg", "canvas"}
    BREAK = {
        "address", "article", "aside", "blockquote", "br", "div", "dl", "dt",
        "dd", "fieldset", "figcaption", "figure", "footer", "form", "h1", "h2",
        "h3", "h4", "h5", "h6", "header", "hr", "li", "main", "nav", "ol",
        "p", "pre", "section", "table", "tr", "ul",
    }

    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.skip_depth = 0
        self.in_title = False
        self.title_parts: list[str] = []
        self.parts: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        tag = tag.lower()
        if tag in self.SKIP:
            self.skip_depth += 1
            return
        if tag == "title":
            self.in_title = True
            return
        if self.skip_depth:
            return
        if tag in self.BREAK:
            self.parts.append("\n")
        if tag == "img":
            alt = next((value for key, value in attrs if key.lower() == "alt"), None)
            if alt:
                self.parts.append(f" [{alt}] ")

    def handle_endtag(self, tag: str) -> None:
        tag = tag.lower()
        if tag in self.SKIP:
            if self.skip_depth:
                self.skip_depth -= 1
            return
        if tag == "title":
            self.in_title = False
            return
        if not self.skip_depth and tag in self.BREAK:
            self.parts.append("\n")

    def handle_data(self, data: str) -> None:
        if self.in_title:
            self.title_parts.append(data)
        elif not self.skip_depth:
            self.parts.append(data)


SPACE_RUN = re.compile(r"[ \t\f\v]+")
BLANK_RUN = re.compile(r"\n{3,}")


def clean_text(parts: Iterable[str]) -> str:
    raw = html_module.unescape("".join(parts)).replace("\r", "")
    lines = [SPACE_RUN.sub(" ", line).strip() for line in raw.split("\n")]
    text = "\n".join(line for line in lines if line)
    return BLANK_RUN.sub("\n\n", text).strip()


def extract(html: str) -> tuple[str, str]:
    parser = VisibleTextParser()
    parser.feed(html)
    parser.close()
    title = SPACE_RUN.sub(" ", " ".join(parser.title_parts)).strip()
    return title, clean_text(parser.parts)


def utf8_prefix(text: str, byte_limit: int) -> str:
    if byte_limit <= 0:
        return ""
    encoded = text.encode("utf-8")
    if len(encoded) <= byte_limit:
        return text
    clipped = encoded[:byte_limit]
    while clipped:
        try:
            return clipped.decode("utf-8") + "\n[… clipped …]"
        except UnicodeDecodeError:
            clipped = clipped[:-1]
    return "[… clipped …]"


def make_text_preview(title: str, source: str, body: str, byte_limit: int) -> str:
    header = []
    if title:
        header.append(title)
    if source:
        header.append(source)
    if header and body:
        header.append("")
    result = "\n".join(header) + body if not header else "\n".join(header + ([body] if body else []))
    return utf8_prefix(result.strip() + "\n", byte_limit)


# Five-by-seven uppercase bitmap font. Lowercase is intentionally mapped to
# uppercase: this renderer is for recognizable rough previews, not typography.
FONT: dict[str, tuple[str, ...]] = {
    " ": ("00000",) * 7,
    "A": ("01110","10001","10001","11111","10001","10001","10001"),
    "B": ("11110","10001","10001","11110","10001","10001","11110"),
    "C": ("01111","10000","10000","10000","10000","10000","01111"),
    "D": ("11110","10001","10001","10001","10001","10001","11110"),
    "E": ("11111","10000","10000","11110","10000","10000","11111"),
    "F": ("11111","10000","10000","11110","10000","10000","10000"),
    "G": ("01111","10000","10000","10111","10001","10001","01111"),
    "H": ("10001","10001","10001","11111","10001","10001","10001"),
    "I": ("11111","00100","00100","00100","00100","00100","11111"),
    "J": ("00111","00010","00010","00010","10010","10010","01100"),
    "K": ("10001","10010","10100","11000","10100","10010","10001"),
    "L": ("10000","10000","10000","10000","10000","10000","11111"),
    "M": ("10001","11011","10101","10101","10001","10001","10001"),
    "N": ("10001","11001","10101","10011","10001","10001","10001"),
    "O": ("01110","10001","10001","10001","10001","10001","01110"),
    "P": ("11110","10001","10001","11110","10000","10000","10000"),
    "Q": ("01110","10001","10001","10001","10101","10010","01101"),
    "R": ("11110","10001","10001","11110","10100","10010","10001"),
    "S": ("01111","10000","10000","01110","00001","00001","11110"),
    "T": ("11111","00100","00100","00100","00100","00100","00100"),
    "U": ("10001","10001","10001","10001","10001","10001","01110"),
    "V": ("10001","10001","10001","10001","10001","01010","00100"),
    "W": ("10001","10001","10001","10101","10101","10101","01010"),
    "X": ("10001","10001","01010","00100","01010","10001","10001"),
    "Y": ("10001","10001","01010","00100","00100","00100","00100"),
    "Z": ("11111","00001","00010","00100","01000","10000","11111"),
    "0": ("01110","10001","10011","10101","11001","10001","01110"),
    "1": ("00100","01100","00100","00100","00100","00100","01110"),
    "2": ("01110","10001","00001","00010","00100","01000","11111"),
    "3": ("11110","00001","00001","01110","00001","00001","11110"),
    "4": ("00010","00110","01010","10010","11111","00010","00010"),
    "5": ("11111","10000","10000","11110","00001","00001","11110"),
    "6": ("01110","10000","10000","11110","10001","10001","01110"),
    "7": ("11111","00001","00010","00100","01000","01000","01000"),
    "8": ("01110","10001","10001","01110","10001","10001","01110"),
    "9": ("01110","10001","10001","01111","00001","00001","01110"),
    ".": ("00000","00000","00000","00000","00000","01100","01100"),
    ",": ("00000","00000","00000","00000","01100","01100","01000"),
    ":": ("00000","01100","01100","00000","01100","01100","00000"),
    ";": ("00000","01100","01100","00000","01100","01100","01000"),
    "!": ("00100","00100","00100","00100","00100","00000","00100"),
    "?": ("01110","10001","00001","00010","00100","00000","00100"),
    "-": ("00000","00000","00000","11111","00000","00000","00000"),
    "_": ("00000","00000","00000","00000","00000","00000","11111"),
    "/": ("00001","00010","00010","00100","01000","01000","10000"),
    "\\": ("10000","01000","01000","00100","00010","00010","00001"),
    "(": ("00010","00100","01000","01000","01000","00100","00010"),
    ")": ("01000","00100","00010","00010","00010","00100","01000"),
    "[": ("01110","01000","01000","01000","01000","01000","01110"),
    "]": ("01110","00010","00010","00010","00010","00010","01110"),
    "'": ("00100","00100","00000","00000","00000","00000","00000"),
    '"': ("01010","01010","00000","00000","00000","00000","00000"),
    "@": ("01110","10001","10111","10101","10111","10000","01110"),
    "#": ("01010","01010","11111","01010","11111","01010","01010"),
    "%": ("11001","11010","00100","01000","10110","00110","00000"),
    "&": ("01100","10010","10100","01000","10101","10010","01101"),
    "+": ("00000","00100","00100","11111","00100","00100","00000"),
    "=": ("00000","11111","00000","11111","00000","00000","00000"),
    "<": ("00010","00100","01000","10000","01000","00100","00010"),
    ">": ("01000","00100","00010","00001","00010","00100","01000"),
}


def glyph(ch: str) -> tuple[str, ...]:
    ch = ch.upper()
    return FONT.get(ch, FONT["?"])


def wrap_lines(text: str, columns: int) -> list[str]:
    if columns < 1:
        return []
    output: list[str] = []
    for paragraph in text.splitlines():
        words = paragraph.split()
        if not words:
            output.append("")
            continue
        line = ""
        for word in words:
            while len(word) > columns:
                if line:
                    output.append(line)
                    line = ""
                output.append(word[:columns])
                word = word[columns:]
            candidate = word if not line else line + " " + word
            if len(candidate) <= columns:
                line = candidate
            else:
                output.append(line)
                line = word
        if line:
            output.append(line)
    return output


def set_black(bitmap: bytearray, row_bytes: int, x: int, y: int) -> None:
    index = y * row_bytes + x // 8
    bitmap[index] |= 1 << (7 - (x % 8))


def draw_line(bitmap: bytearray, width: int, height: int, x: int, y: int, text: str) -> None:
    row_bytes = (width + 7) // 8
    cursor = x
    for ch in text:
        for gy, pattern in enumerate(glyph(ch)):
            py = y + gy
            if py >= height:
                break
            for gx, bit in enumerate(pattern):
                px = cursor + gx
                if bit == "1" and 0 <= px < width:
                    set_black(bitmap, row_bytes, px, py)
        cursor += 6
        if cursor + 5 > width:
            break


def render_pbm_pages(text: str, width: int, height: int, max_screens: int) -> list[bytes]:
    margin_x = 6
    margin_y = 8
    char_width = 6
    line_height = 8
    columns = max(1, (width - 2 * margin_x) // char_width)
    rows = max(1, (height - 2 * margin_y) // line_height)
    lines = wrap_lines(text, columns)
    pages: list[bytes] = []
    row_bytes = (width + 7) // 8

    for screen in range(max_screens):
        page_lines = lines[screen * rows:(screen + 1) * rows]
        if not page_lines:
            break
        bitmap = bytearray(row_bytes * height)
        for row, line in enumerate(page_lines):
            draw_line(bitmap, width, height, margin_x, margin_y + row * line_height, line)
        header = f"P4\n{width} {height}\n".encode("ascii")
        pages.append(header + bytes(bitmap))
    return pages


def read_input(path: str) -> str:
    if path == "-":
        return sys.stdin.read()
    return Path(path).read_text(encoding="utf-8", errors="replace")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", help="already-fetched HTML file, or - for stdin")
    parser.add_argument("--source", default="", help="URL/label to display in the text preview")
    parser.add_argument("--out", type=Path, default=Path("preview-out"))
    parser.add_argument("--width", type=int, default=DEFAULT_WIDTH)
    parser.add_argument("--height", type=int, default=DEFAULT_HEIGHT)
    parser.add_argument("--screens", type=int, default=DEFAULT_SCREENS)
    parser.add_argument("--text-bytes", type=int, default=DEFAULT_TEXT_BYTES)
    args = parser.parse_args()

    if args.width < 32 or args.height < 32:
        parser.error("preview dimensions must be at least 32x32")
    if not 1 <= args.screens <= 3:
        parser.error("--screens must be between 1 and 3")
    if args.text_bytes < 128:
        parser.error("--text-bytes must be at least 128")

    html = read_input(args.input)
    title, body = extract(html)
    text_preview = make_text_preview(title, args.source, body, args.text_bytes)

    visual_source = "\n".join(part for part in (title, args.source, "", body) if part)
    pages = render_pbm_pages(visual_source, args.width, args.height, args.screens)

    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "preview.txt").write_text(text_preview, encoding="utf-8")
    for index, page in enumerate(pages, start=1):
        (args.out / f"screen-{index:02d}.pbm").write_bytes(page)

    print(f"text: {args.out / 'preview.txt'} ({len(text_preview.encode('utf-8'))} bytes)")
    for index, page in enumerate(pages, start=1):
        print(f"screen {index}: {args.out / f'screen-{index:02d}.pbm'} ({len(page)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
