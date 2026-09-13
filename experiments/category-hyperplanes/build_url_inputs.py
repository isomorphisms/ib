#!/usr/bin/env python3
"""Build pinned URL-only probe inputs from the sanitized browsing fixture."""

from __future__ import annotations

import argparse
import csv
import hashlib
from pathlib import Path


EXPECTED_SOURCE_SHA256 = "4c71746f994ef02959cef9baf9787a74c60e6fe51309d096e6398d56bef0c850"
EXPECTED_ROWS = 219
ID_PREFIX = "real-world-20260825"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def row_id(line_number: int) -> str:
    return f"{ID_PREFIX}-{line_number:06d}"


def build(source: Path, output: Path) -> None:
    checksum = sha256(source)
    if checksum != EXPECTED_SOURCE_SHA256:
        raise ValueError(
            f"source checksum {checksum} does not match pinned {EXPECTED_SOURCE_SHA256}"
        )
    urls = source.read_text(encoding="utf-8").splitlines()
    if len(urls) != EXPECTED_ROWS:
        raise ValueError(f"source has {len(urls)} rows, expected {EXPECTED_ROWS}")
    if any(not url or "\t" in url for url in urls):
        raise ValueError("source contains an empty row or tab")

    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("id", "text"))
        for line_number, url in enumerate(urls, start=1):
            writer.writerow((row_id(line_number), f"url: {url}"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    build(Path(args.source), Path(args.output))
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
