#!/usr/bin/env python3
"""Check recovered assertions against their pinned source rows and probe splits."""

from __future__ import annotations

import csv
from pathlib import Path

from build_url_inputs import EXPECTED_ROWS, EXPECTED_SOURCE_SHA256, row_id, sha256
ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "tests/fixtures/real_world_urls.txt"
DIRECTORY = Path(__file__).resolve().parent
LEDGER = DIRECTORY / "recovered-authoritative-assertions.tsv"
FIT = DIRECTORY / "recovered-reading-fit.tsv"
EVALUATION = DIRECTORY / "recovered-reading-evaluation.tsv"
LEDGER_HEADER = [
    "id",
    "url",
    "category",
    "polarity",
    "authority",
    "asserted_at",
    "assertion_source",
    "recovery_basis",
    "source_artifact_sha256",
    "source_row",
]
PROBE_HEADER = [
    "id",
    "category",
    "polarity",
    "authority",
    "asserted_at",
    "assertion_source",
    "source_artifact_sha256",
    "source_row",
]


def read_tsv(path: Path, expected_header: list[str]) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames != expected_header:
            raise ValueError(f"{path}: unexpected header")
        return list(reader)


def main() -> int:
    if sha256(SOURCE) != EXPECTED_SOURCE_SHA256:
        raise ValueError("sanitized source fixture checksum changed")
    urls = SOURCE.read_text(encoding="utf-8").splitlines()
    if len(urls) != EXPECTED_ROWS:
        raise ValueError("sanitized source fixture row count changed")

    ledger = read_tsv(LEDGER, LEDGER_HEADER)
    by_key: dict[tuple[str, str], dict[str, str]] = {}
    for record in ledger:
        key = (record["id"], record["category"])
        if key in by_key:
            raise ValueError(f"duplicate recovered assertion {key!r}")
        by_key[key] = record
        if record["polarity"] != "positive" or record["authority"] != "human_assertion":
            raise ValueError(f"non-authoritative recovered assertion {key!r}")
        if not record["asserted_at"] or not record["assertion_source"]:
            raise ValueError(f"missing assertion provenance {key!r}")
        if record["source_row"] != "unavailable":
            source_row = int(record["source_row"])
            if record["source_artifact_sha256"] != EXPECTED_SOURCE_SHA256:
                raise ValueError(f"wrong source checksum for {key!r}")
            if record["id"] != row_id(source_row):
                raise ValueError(f"wrong stable row id for {key!r}")
            if urls[source_row - 1] != record["url"]:
                raise ValueError(f"source URL mismatch for {key!r}")
        elif record["source_artifact_sha256"] != "unavailable":
            raise ValueError(f"unavailable source row has an artifact checksum for {key!r}")

    known_ids = {row_id(number) for number in range(1, EXPECTED_ROWS + 1)}
    fit = read_tsv(FIT, PROBE_HEADER)
    evaluation = read_tsv(EVALUATION, PROBE_HEADER)
    fit_ids = {label["id"] for label in fit}
    evaluation_ids = {label["id"] for label in evaluation}
    if fit_ids & evaluation_ids:
        raise ValueError("fit and evaluation rows overlap")
    if len(fit_ids) != 3 or len(evaluation_ids) != 2:
        raise ValueError("expected three fit and two evaluation reading assertions")
    for label in fit + evaluation:
        if label["id"] not in known_ids:
            raise ValueError(f"probe label has unknown source id: {label!r}")
        record = by_key.get((label["id"], label["category"]))
        if record is None:
            raise ValueError(f"probe label absent from recovered ledger: {label!r}")
        if (
            label["polarity"] != record["polarity"]
            or label["authority"] != record["authority"]
            or label["asserted_at"] != record["asserted_at"]
            or label["assertion_source"] != record["assertion_source"]
            or label["source_artifact_sha256"] != record["source_artifact_sha256"]
            or label["source_row"] != record["source_row"]
        ):
            raise ValueError(f"probe label provenance drifted: {label!r}")

    print("recovered authoritative labels: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
