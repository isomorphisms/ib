#!/usr/bin/env python3
"""Inspectable command surface for the disposable conversation experiment."""

from __future__ import annotations

import argparse
import json
import time
from dataclasses import asdict
from pathlib import Path

from ib_conversations.acquire import import_export, verify_public
from ib_conversations.classify import (
    classify_geometric_model,
    compare,
    filing_projection,
    merge_incremental_proposals,
    train_geometric_model,
)
from ib_conversations.corpus import INPUT_VIEWS, load_corpus, load_title_diagnostic, source_fingerprints
from ib_conversations.events import AUTHORITIES, AXES, append_correction, current_labels, load_events
from ib_conversations.identity import write_json_if_changed, write_jsonl_if_changed
from ib_conversations.partitions import partition_rows
from ib_conversations.query import changed_proposals, query_evidence, query_proposals, resolve_proposals
from ib_conversations.title_ledger import import_title_ledger


def add_document_source(parser: argparse.ArgumentParser) -> None:
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--corpus", type=Path)
    source.add_argument("--title-diagnostic", type=Path)


def documents_from(arguments: argparse.Namespace, only_ids: set[str] | None = None):
    if arguments.corpus:
        return load_corpus(arguments.corpus, only_ids)
    return load_title_diagnostic(arguments.title_diagnostic, only_ids)


def split_map(directory: Path, field: str) -> dict[str, str]:
    rows = [
        json.loads(line)
        for line in (directory / "partitions.jsonl").read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]
    return {row["corpus_id"]: row[field] for row in rows}


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

    partition = commands.add_parser("partition", help="make duplicate-grouped random and chronological splits")
    add_document_source(partition)
    partition.add_argument("--events", required=True, type=Path)
    partition.add_argument("--axis", choices=sorted(AXES), default="concept")
    partition.add_argument("--minimum-authority", choices=sorted(AUTHORITIES, key=AUTHORITIES.get), default="accepted_decision")
    partition.add_argument("--near-duplicate-threshold", type=float, default=0.92)
    partition.add_argument("--seed", default="ib-conversation-v1")
    partition.add_argument("--output", required=True, type=Path)

    comparison = commands.add_parser("compare", help="fit and evaluate independent classifier baselines")
    add_document_source(comparison)
    comparison.add_argument("--events", required=True, type=Path)
    comparison.add_argument("--axis", choices=sorted(AXES), default="concept")
    comparison.add_argument("--minimum-authority", choices=sorted(AUTHORITIES, key=AUTHORITIES.get), default="accepted_decision")
    comparison.add_argument("--partitions", required=True, type=Path)
    comparison.add_argument("--split-field", choices=("random_split", "chronological_split"), default="random_split")
    comparison.add_argument("--view", action="append", choices=sorted(INPUT_VIEWS), required=True)
    comparison.add_argument("--rules", type=Path)
    comparison.add_argument("--filing-evaluation", action="store_true")
    comparison.add_argument("--output", required=True, type=Path)

    training = commands.add_parser("train-model", help="fit one model generation without classifying")
    add_document_source(training)
    training.add_argument("--events", required=True, type=Path)
    training.add_argument("--axis", choices=sorted(AXES), default="concept")
    training.add_argument("--minimum-authority", choices=sorted(AUTHORITIES, key=AUTHORITIES.get), default="accepted_decision")
    training.add_argument("--partitions", required=True, type=Path)
    training.add_argument("--split-field", choices=("random_split", "chronological_split"), default="random_split")
    training.add_argument("--representation", choices=("sparse", "dense_lsa", "weighted"), required=True)
    training.add_argument("--view", choices=sorted(INPUT_VIEWS), default="user")
    training.add_argument("--category", action="append")
    training.add_argument("--output", required=True, type=Path)

    classification = commands.add_parser("classify-model", help="classify without fitting; optionally reuse unchanged proposals")
    add_document_source(classification)
    classification.add_argument("--model", required=True, type=Path)
    classification.add_argument("--previous-proposals", type=Path)
    classification.add_argument("--only", action="append", help="parse and reclassify only this corpus id")
    classification.add_argument("--output", required=True, type=Path)

    correction = commands.add_parser("correct", help="append an authoritative correction event")
    correction.add_argument("--events", required=True, type=Path)
    correction.add_argument("--target-id", required=True)
    correction.add_argument("--axis", choices=("concept", "filing_destination"), required=True)
    correction.add_argument("--value", required=True)
    correction.add_argument("--polarity", choices=("positive", "negative"), required=True)
    correction.add_argument("--asserted-at", required=True)
    correction.add_argument("--source-reference", required=True)
    correction.add_argument("--reason", required=True)

    query = commands.add_parser("query", help="inspect one immutable proposal generation")
    query.add_argument("--proposals", required=True, type=Path)
    query.add_argument("--operation", choices=("unclassified", "category", "boundary", "overlap", "why"), required=True)
    query.add_argument("--category")
    query.add_argument("--other-category")
    query.add_argument("--corpus-id")
    query.add_argument("--limit", type=int, default=50)

    changed = commands.add_parser("changed", help="compare accepted memberships between generations")
    changed.add_argument("--previous", required=True, type=Path)
    changed.add_argument("--current", required=True, type=Path)
    changed.add_argument("--limit", type=int, default=100)

    evidence = commands.add_parser("evidence", help="inspect label provenance and weak-only evidence")
    evidence.add_argument("--events", required=True, type=Path)
    evidence.add_argument("--operation", choices=("weak", "target", "category"), required=True)
    evidence.add_argument("--target-id")
    evidence.add_argument("--category")
    evidence.add_argument("--limit", type=int, default=100)

    projection = commands.add_parser("project", help="overlay authoritative evidence on immutable proposals")
    projection.add_argument("--proposals", required=True, type=Path)
    projection.add_argument("--events", required=True, type=Path)
    projection.add_argument("--axis", choices=sorted(AXES), required=True)
    projection.add_argument("--minimum-authority", choices=sorted(AUTHORITIES, key=AUTHORITIES.get), default="accepted_decision")
    projection.add_argument("--output", required=True, type=Path)

    destinations = commands.add_parser("destinations", help="derive a deterministic abstaining filing projection")
    destinations.add_argument("--proposals", required=True, type=Path)
    destinations.add_argument("--ambiguity-margin", type=float, default=0.05)
    destinations.add_argument("--output", required=True, type=Path)
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
    if arguments.command == "partition":
        documents = documents_from(arguments)
        labels = current_labels(
            load_events(arguments.events),
            axis=arguments.axis,
            minimum_authority=arguments.minimum_authority,
        )
        rows, report = partition_rows(
            documents,
            labels,
            near_duplicate_threshold=arguments.near_duplicate_threshold,
            seed=arguments.seed,
        )
        arguments.output.mkdir(parents=True, exist_ok=True)
        write_jsonl_if_changed(arguments.output / "partitions.jsonl", rows)
        write_json_if_changed(arguments.output / "report.json", report)
        print(json.dumps(report, indent=2, sort_keys=True))
        return 0
    if arguments.command == "compare":
        if arguments.filing_evaluation and arguments.axis != "filing_destination":
            raise ValueError("--filing-evaluation requires --axis filing_destination")
        documents = documents_from(arguments)
        labels = current_labels(
            load_events(arguments.events),
            axis=arguments.axis,
            minimum_authority=arguments.minimum_authority,
        )
        partitions = split_map(arguments.partitions, arguments.split_field)
        result = compare(
            documents,
            labels,
            partitions,
            arguments.output,
            views=arguments.view,
            rules_path=arguments.rules,
            filing_evaluation=arguments.filing_evaluation,
        )
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    if arguments.command == "train-model":
        documents = documents_from(arguments)
        labels = current_labels(
            load_events(arguments.events),
            axis=arguments.axis,
            minimum_authority=arguments.minimum_authority,
        )
        result = train_geometric_model(
            documents,
            labels,
            split_map(arguments.partitions, arguments.split_field),
            arguments.output,
            representation=arguments.representation,
            view=arguments.view,
            categories=set(arguments.category) if arguments.category else None,
            closed_world_single_label=arguments.axis == "filing_destination",
        )
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    if arguments.command == "classify-model":
        started = time.perf_counter()
        requested = set(arguments.only) if arguments.only else None
        documents = documents_from(arguments, requested)
        artifact = json.loads((arguments.model / "artifact.json").read_text(encoding="utf-8"))
        classified = classify_geometric_model(arguments.model, documents)
        inventory = source_fingerprints(
            arguments.corpus or arguments.title_diagnostic,
            title_diagnostic=bool(arguments.title_diagnostic),
        )
        proposals, reused = merge_incremental_proposals(
            arguments.previous_proposals,
            classified,
            inventory,
            artifact["model_generation_id"],
        )
        arguments.output.mkdir(parents=True, exist_ok=True)
        write_jsonl_if_changed(arguments.output / "proposals.jsonl", proposals)
        report = {
            "operation": "classify",
            "model_generation_id": artifact["model_generation_id"],
            "inventory_rows": len(inventory),
            "conversation_bodies_parsed": len(documents),
            "classified": len(classified),
            "unchanged_proposals_reused": reused,
            "elapsed_seconds": time.perf_counter() - started,
        }
        write_json_if_changed(arguments.output / "classification-report.json", report)
        print(json.dumps(report, indent=2, sort_keys=True))
        return 0
    if arguments.command == "correct":
        event = append_correction(
            arguments.events,
            target_id=arguments.target_id,
            axis=arguments.axis,
            value=arguments.value,
            polarity=arguments.polarity,
            asserted_at=arguments.asserted_at,
            source_reference=arguments.source_reference,
            reason=arguments.reason,
        )
        print(json.dumps(event.as_dict(), indent=2, sort_keys=True))
        return 0
    if arguments.command == "query":
        result = query_proposals(
            arguments.proposals,
            operation=arguments.operation,
            category=arguments.category,
            other_category=arguments.other_category,
            corpus_id=arguments.corpus_id,
            limit=arguments.limit,
        )
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    if arguments.command == "changed":
        print(json.dumps(changed_proposals(arguments.previous, arguments.current, arguments.limit), indent=2, sort_keys=True))
        return 0
    if arguments.command == "evidence":
        result = query_evidence(
            load_events(arguments.events),
            operation=arguments.operation,
            target_id=arguments.target_id,
            category=arguments.category,
            limit=arguments.limit,
        )
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    if arguments.command == "project":
        rows = resolve_proposals(
            arguments.proposals,
            load_events(arguments.events),
            axis=arguments.axis,
            minimum_authority=arguments.minimum_authority,
        )
        arguments.output.mkdir(parents=True, exist_ok=True)
        write_jsonl_if_changed(arguments.output / "resolved-proposals.jsonl", rows)
        report = {
            "rows": len(rows),
            "axis": arguments.axis,
            "minimum_authority": arguments.minimum_authority,
            "authoritative_overrides": sum(
                1
                for row in rows
                for value in row.get("categories", {}).values()
                if value.get("authoritative_override")
            ),
        }
        write_json_if_changed(arguments.output / "projection-report.json", report)
        print(json.dumps(report, indent=2, sort_keys=True))
        return 0
    if arguments.command == "destinations":
        proposals = [
            json.loads(line)
            for line in arguments.proposals.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ]
        rows = filing_projection(proposals, ambiguity_margin=arguments.ambiguity_margin)
        arguments.output.mkdir(parents=True, exist_ok=True)
        write_jsonl_if_changed(arguments.output / "destination-proposals.jsonl", rows)
        report = {
            "rows": len(rows),
            "ambiguity_margin": arguments.ambiguity_margin,
            "outcomes": {
                outcome: sum(row["outcome"] == outcome for row in rows)
                for outcome in (
                    "confident_destination",
                    "several_plausible_destinations",
                    "no_sufficiently_supported_destination",
                )
            },
        }
        write_json_if_changed(arguments.output / "destination-report.json", report)
        print(json.dumps(report, indent=2, sort_keys=True))
        return 0
    raise AssertionError(arguments.command)


if __name__ == "__main__":
    raise SystemExit(main())
