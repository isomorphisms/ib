#!/usr/bin/env python3
"""Stage public arXiv PDF text in a temporary Pensieve-shaped experiment fixture.

This is deliberately not an IB acquisition implementation. Production acquisition remains
behind the 0.2 ICU boundary. This helper exists only so CI can evaluate body-text geometry
without committing paper bodies to Git.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
import time
import urllib.request
from pathlib import Path


USER_AGENT = "isomorphisms-ib-body-hyperplane-experiment/0.1"
FORMAT = "ib-live-arxiv-pensieve-fixture-v1"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def read_ids(path: Path) -> list[str]:
    values = [line.split("#", 1)[0].strip() for line in path.read_text(encoding="utf-8").splitlines()]
    values = [value for value in values if value]
    if not values or len(values) != len(set(values)):
        raise ValueError("ids must be nonempty and unique")
    return values


def download(url: str, output: Path) -> None:
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(request, timeout=90) as response, output.open("wb") as stream:
        shutil.copyfileobj(response, stream)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pensieve", required=True)
    parser.add_argument("--ids", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--delay-seconds", type=float, default=1.0)
    args = parser.parse_args()

    if args.delay_seconds < 0:
        raise SystemExit("--delay-seconds must be nonnegative")
    if shutil.which("pdftotext") is None:
        raise SystemExit("pdftotext is required for this disposable live fixture")

    pensieve = Path(args.pensieve)
    identifiers = read_ids(Path(args.ids))
    records: list[dict] = []

    with tempfile.TemporaryDirectory(prefix="ib-live-arxiv-") as temporary_directory:
        temporary = Path(temporary_directory)
        for offset, identifier in enumerate(identifiers):
            if offset and args.delay_seconds:
                time.sleep(args.delay_seconds)
            url = f"https://arxiv.org/pdf/{identifier}.pdf"
            pdf = temporary / f"{identifier}.pdf"
            download(url, pdf)
            if pdf.stat().st_size < 1024 or pdf.read_bytes()[:5] != b"%PDF-":
                raise ValueError(f"arXiv response for {identifier} was not a usable PDF")

            item = pensieve / "arxiv" / identifier
            text_directory = item / "text"
            text_directory.mkdir(parents=True, exist_ok=True)
            body = text_directory / "from-pdf.txt"
            subprocess.run(
                ["pdftotext", "-layout", str(pdf), str(body)],
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True,
            )
            if not body.is_file() or body.stat().st_size < 512:
                raise ValueError(f"pdftotext produced no useful body for {identifier}")

            (item / "url").write_text(f"https://arxiv.org/abs/{identifier}\n", encoding="utf-8")
            (item / "source").write_text("disposable-live-arxiv-fixture\n", encoding="utf-8")
            records.append(
                {
                    "id": f"arxiv:{identifier}",
                    "url": url,
                    "pdf_sha256": sha256(pdf),
                    "pdf_bytes": pdf.stat().st_size,
                    "body_sha256": sha256(body),
                    "body_bytes": body.stat().st_size,
                }
            )

    version = subprocess.run(
        ["pdftotext", "-v"], capture_output=True, text=True, check=False
    )
    version_text = (version.stderr or version.stdout).splitlines()[0] if (version.stderr or version.stdout) else "unknown"
    manifest = {
        "format": FORMAT,
        "warning": "network staging for experiment only; not the IB acquisition boundary",
        "user_agent": USER_AGENT,
        "pdftotext_version": version_text,
        "rows": records,
    }
    output = Path(args.manifest)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
