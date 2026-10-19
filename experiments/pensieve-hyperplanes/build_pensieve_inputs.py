#!/usr/bin/env python3
"""Build deterministic body-text inputs from distilled Pensieve items."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import unicodedata
from pathlib import Path


SOURCE_PREFERENCE = (
    "text/from-html.txt",
    "text/from-pdf.txt",
    "text/from-abstract.txt",
)
BUILDER_FORMAT = "ib-pensieve-body-sketch-v1"


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_path(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def normalized_words(text: str) -> list[str]:
    normalized = unicodedata.normalize("NFKC", text)
    return normalized.split()


def distributed_sample(words: list[str], max_words: int, windows: int) -> tuple[str, list[list[int]]]:
    if not words:
        raise ValueError("body text contains no words")
    if len(words) <= max_words:
        return " ".join(words), [[0, len(words)]]

    window_count = min(windows, max_words)
    base = max_words // window_count
    remainder = max_words % window_count
    widths = [base + (1 if index < remainder else 0) for index in range(window_count)]

    ranges: list[list[int]] = []
    pieces: list[str] = []
    for index, width in enumerate(widths):
        if window_count == 1:
            start = 0
        else:
            start = round(index * (len(words) - width) / (window_count - 1))
        stop = min(len(words), start + width)
        start = max(0, stop - width)
        ranges.append([start, stop])
        pieces.extend(words[start:stop])

    return " ".join(pieces), ranges


def read_ids(path: Path | None, source_root: Path) -> list[str]:
    if path is None:
        return sorted(item.name for item in source_root.iterdir() if item.is_dir())

    values: list[str] = []
    for line_number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        value = raw.split("#", 1)[0].strip()
        if not value:
            continue
        if "/" in value or "\\" in value or value in {".", ".."}:
            raise ValueError(f"ids line {line_number}: expected a source-local identifier")
        values.append(value)
    if not values or len(values) != len(set(values)):
        raise ValueError("ids must be nonempty and unique")
    return values


def select_body(item: Path) -> Path:
    for relative in SOURCE_PREFERENCE:
        candidate = item / relative
        if candidate.is_file() and candidate.stat().st_size:
            return candidate
    raise ValueError(f"no nonempty distilled body text under {item}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pensieve", required=True)
    parser.add_argument("--source", default="arxiv")
    parser.add_argument("--ids")
    parser.add_argument("--output", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--max-words", type=int, default=320)
    parser.add_argument("--windows", type=int, default=5)
    args = parser.parse_args()

    if args.max_words < 1 or args.windows < 1:
        raise SystemExit("--max-words and --windows must be positive")

    pensieve = Path(args.pensieve)
    source_root = pensieve / args.source
    if not source_root.is_dir():
        raise SystemExit(f"missing Pensieve source directory: {source_root}")

    ids = read_ids(Path(args.ids) if args.ids else None, source_root)
    rows: list[tuple[str, str]] = []
    manifest_rows: list[dict] = []

    for local_id in ids:
        item = source_root / local_id
        if not item.is_dir():
            raise ValueError(f"missing Pensieve item {item}")
        body = select_body(item)
        body_bytes = body.read_bytes()
        words = normalized_words(body_bytes.decode("utf-8", errors="strict"))
        sample, ranges = distributed_sample(words, args.max_words, args.windows)
        row_id = f"{args.source}:{local_id}"
        rows.append((row_id, sample))
        manifest_rows.append(
            {
                "id": row_id,
                "pensieve_item": str(item),
                "selected_body": str(body),
                "selected_body_sha256": sha256_bytes(body_bytes),
                "selected_body_bytes": len(body_bytes),
                "selected_body_words": len(words),
                "sample_word_ranges": ranges,
                "sample_words": len(sample.split()),
                "sample_sha256": sha256_bytes(sample.encode("utf-8")),
            }
        )

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("id", "text"))
        writer.writerows(rows)

    manifest = {
        "format": BUILDER_FORMAT,
        "pensieve": str(pensieve),
        "source": args.source,
        "source_preference": list(SOURCE_PREFERENCE),
        "title_or_url_used": False,
        "max_words": args.max_words,
        "windows": args.windows,
        "rows": manifest_rows,
        "output_sha256": sha256_path(output),
    }
    manifest_path = Path(args.manifest)
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(output)
    print(manifest_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
