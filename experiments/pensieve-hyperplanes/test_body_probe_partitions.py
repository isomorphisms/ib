#!/usr/bin/env python3
"""Targeted partition-isolation checks for the affine-plane probe."""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("body_probe", HERE / "body_probe.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("cannot load body_probe.py")
body_probe = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = body_probe
SPEC.loader.exec_module(body_probe)


def check_representation_contracts() -> None:
    checksum = "a" * 64

    body_probe.validate_input_manifest(
        {"output_sha256": checksum, "title_or_url_used": False},
        checksum,
        "pensieve-body-only",
    )
    declared = body_probe.validate_input_manifest(
        {
            "output_sha256": checksum,
            "representation_contract": {
                "kind": "youtube-title-plus-channel",
                "model_text_fields": ["video_title", "channel_title"],
            },
        },
        checksum,
        "declared-text",
    )
    assert declared is not None
    assert declared["kind"] == "youtube-title-plus-channel"

    try:
        body_probe.validate_input_manifest(
            {"output_sha256": checksum},
            checksum,
            "declared-text",
        )
    except ValueError as error:
        assert "representation_contract" in str(error)
    else:
        raise AssertionError("declared text without a representation contract was accepted")

    try:
        body_probe.validate_input_manifest(
            {"output_sha256": checksum, "title_or_url_used": True},
            checksum,
            "pensieve-body-only",
        )
    except ValueError as error:
        assert "title_or_url_used=false" in str(error)
    else:
        raise AssertionError("body-only input with title/url use was accepted")


def main() -> int:
    check_representation_contracts()
    ids = [
        "fit-positive-a",
        "fit-positive-b",
        "fit-unlabeled",
        "development-unlabeled",
        "held-out-unlabeled",
    ]
    vectors = np.asarray(
        [
            [1.0, 0.0, 0.0],
            [0.9, 0.1, 0.0],
            [-1.0, 0.0, 0.0],
            [0.0, 1.0, 0.0],
            [0.0, 0.0, 1.0],
        ],
        dtype=np.float32,
    )
    vectors /= np.linalg.norm(vectors, axis=1, keepdims=True)

    labels = [
        body_probe.Label(
            "fit-positive-a", "fixture", "fit_positive", "synthetic fixture"
        ),
        body_probe.Label(
            "fit-positive-b", "fixture", "fit_positive", "synthetic fixture"
        ),
    ]
    partitions = {
        "fit-positive-a": "fit",
        "fit-positive-b": "fit",
        "fit-unlabeled": "fit",
        "development-unlabeled": "development",
        "held-out-unlabeled": "held_out",
    }
    policy = {
        "format": body_probe.POLICY_FORMAT,
        "name": "synthetic-partition-test",
        "concept": "fixture",
        "score_floor_fraction_of_fit_positive_floor": 1.0,
        "minimum_zero_surface_vote_fraction": 0.0,
        "proposal_rule": "synthetic test only",
    }

    result = body_probe.fit_concept(
        "fixture",
        ids,
        vectors,
        labels,
        partitions,
        policy,
        bags=8,
        unlabeled_per_positive=0.5,
        c_value=1.0,
        positive_weight=2.0,
        target_recall=1.0,
        seed=1729,
    )

    assert result["fit_eligible_unlabeled_count"] == 1
    assert result["partition_excluded_unlabeled_count"] == 2
    assert result["retained_unique_plane_count"] == 1
    for plane in result["planes"]:
        assert plane["provisional_unlabeled_ids"] == ["fit-unlabeled"]
        assert "development-unlabeled" not in plane["support_ids"]
        assert "held-out-unlabeled" not in plane["support_ids"]

    known_bad = dict(partitions)
    known_bad["fit-positive-a"] = "development"
    try:
        body_probe.fit_concept(
            "fixture",
            ids,
            vectors,
            labels,
            known_bad,
            policy,
            bags=8,
            unlabeled_per_positive=0.5,
            c_value=1.0,
            positive_weight=2.0,
            target_recall=1.0,
            seed=1729,
        )
    except ValueError as error:
        assert "requires partition 'fit'" in str(error)
    else:
        raise AssertionError("mismatched semantic label and partition was accepted")

    print("body probe partition isolation: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
