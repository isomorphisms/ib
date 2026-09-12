#!/usr/bin/env python3
"""Inspectable command surface for the disposable conversation experiment."""

from __future__ import annotations

import argparse
import json
from dataclasses import asdict
from pathlib import Path

from ib_conversations.acquire import import_export, verify_public
from ib_conversations.title_ledger import import_title_ledger


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)

    importer = commands.add_parser("import-export", help="screen and import one bulk ChatGPT export")
    importer.add_argument("--source", required=True, type=Path)
    importer.add_argument("--corpus", required=True, type=Path)
    importer.add_argument("--acquired-at", required=True)
    importer.add_argument("--source-label", default="chatgpt-data-export")

    verifier = commands.add_parser("verify-public", help="rescan public raw and metadata files")
    verifier.add_argument("--corpus", required=True, type=Path)

    ledger = commands.add_parser(
        "import-title-ledger",
        help="retain the legacy cleanup ledger as a title-only diagnostic",
    )
    ledger.add_argument("--source", required=True, type=Path)
    ledger.add_argument("--destination", required=True, type=Path)
    return parser


def main() -> int:
    arguments = build_parser().parse_args()
    if arguments.command == "import-export":
        result = asdict(
            import_export(
                arguments.source,
                arguments.corpus,
                arguments.acquired_at,
                arguments.source_label,
            )
        )
        print(json.dumps(result, indent=2, sort_keys=True))
        return 1 if result["blocking_manual_review"] else 0
    if arguments.command == "verify-public":
        result = verify_public(arguments.corpus)
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0 if result["pass"] else 1
    if arguments.command == "import-title-ledger":
        print(json.dumps(import_title_ledger(arguments.source, arguments.destination), indent=2, sort_keys=True))
        return 0
    raise AssertionError(arguments.command)


if __name__ == "__main__":
    raise SystemExit(main())
