"""Fail-closed public-corpus screening with mechanical receipts."""

from __future__ import annotations

import hashlib
import re
from dataclasses import dataclass
from typing import Iterable
from urllib.parse import parse_qsl, urlsplit

from . import SCANNER_REVISION


PLACEHOLDER = re.compile(r"\[REDACTED:[A-Z0-9_]+\]")


@dataclass(frozen=True)
class Finding:
    kind: str
    start: int
    end: int
    blocking: bool
    detector: str

    def receipt(self, *, corpus_id: str, message_id: str, text: str) -> dict:
        removed = text[self.start : self.end]
        return {
            "corpus_id": corpus_id,
            "message_id": message_id,
            "kind": self.kind,
            "detector": self.detector,
            "start": self.start,
            "end": self.end,
            "original_characters": len(removed),
            "original_sha256": hashlib.sha256(removed.encode("utf-8")).hexdigest(),
            "scanner_revision": SCANNER_REVISION,
        }


SPAN_PATTERNS: tuple[tuple[str, str, re.Pattern[str]], ...] = (
    (
        "PRIVATE_KEY",
        "private_key_block_v1",
        re.compile(
            r"-----BEGIN(?: [A-Z0-9]+)? PRIVATE KEY-----.*?-----END(?: [A-Z0-9]+)? PRIVATE KEY-----",
            re.DOTALL,
        ),
    ),
    ("OPENAI_KEY", "openai_key_v1", re.compile(r"\bsk-[A-Za-z0-9_-]{16,}\b")),
    ("GITHUB_TOKEN", "github_token_v1", re.compile(r"\bgh[pousr]_[A-Za-z0-9]{20,}\b")),
    ("SLACK_TOKEN", "slack_token_v1", re.compile(r"\bxox[baprs]-[A-Za-z0-9-]{16,}\b")),
    ("AWS_ACCESS_KEY", "aws_access_key_v1", re.compile(r"\b(?:AKIA|ASIA)[A-Z0-9]{16}\b")),
    (
        "JWT",
        "jwt_v1",
        re.compile(r"\beyJ[A-Za-z0-9_-]{8,}\.[A-Za-z0-9_-]{8,}\.[A-Za-z0-9_-]{8,}\b"),
    ),
    (
        "AUTH_HEADER",
        "auth_header_v1",
        re.compile(r"(?im)^\s*(?:authorization|proxy-authorization)\s*:\s*\S.*$"),
    ),
    (
        "COOKIE_HEADER",
        "cookie_header_v1",
        re.compile(r"(?im)^\s*(?:cookie|set-cookie)\s*:\s*\S.*$"),
    ),
    (
        "SECRET_ASSIGNMENT",
        "secret_assignment_v1",
        re.compile(
            r"(?i)\b(?:api[_ -]?key|access[_ -]?token|auth[_ -]?token|password|passwd|client[_ -]?secret|session[_ -]?id|cookie)\b\s*[:=]\s*[^\s,;]{6,}"
        ),
    ),
    ("SSN", "ssn_v1", re.compile(r"(?<!\d)\d{3}-\d{2}-\d{4}(?!\d)")),
    (
        "ACCOUNT_NUMBER",
        "account_number_context_v1",
        re.compile(r"(?i)\b(?:account|acct)\s*(?:number|no\.?|#)?\s*[:=-]?\s*\d[\d -]{5,20}\d\b"),
    ),
    (
        "ROUTING_NUMBER",
        "routing_number_context_v1",
        re.compile(r"(?i)\brouting\s*(?:number|no\.?|#)?\s*[:=-]?\s*\d{9}\b"),
    ),
    (
        "STREET_ADDRESS",
        "street_address_v1",
        re.compile(
            r"(?i)\b\d{1,6}\s+(?:[A-Z0-9][A-Z0-9.'’-]*\s+){0,5}"
            r"(?:street|st\.?|avenue|ave\.?|road|rd\.?|boulevard|blvd\.?|drive|dr\.?|lane|ln\.?|court|ct\.?|way|trail|trl\.?|highway|hwy\.?)\b"
            r"(?:\s*,?\s*[A-Z][A-Za-z.'’-]+){0,2}(?:\s*,?\s*[A-Z]{2}\s+\d{5}(?:-\d{4})?)?"
        ),
    ),
    (
        "EMAIL_ADDRESS",
        "email_v1",
        re.compile(r"\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}\b", re.IGNORECASE),
    ),
    (
        "PHONE_NUMBER",
        "north_american_phone_v1",
        re.compile(r"(?<!\d)(?:\+?1[ .-]?)?\(?\d{3}\)?[ .-]\d{3}[ .-]\d{4}(?!\d)"),
    ),
)


URL_PATTERN = re.compile(r"https?://[^\s<>\]\[(){}\"']+", re.IGNORECASE)
SECRET_QUERY_NAMES = {
    "access_token",
    "auth",
    "auth_token",
    "client_secret",
    "credential",
    "jwt",
    "key",
    "password",
    "session",
    "session_id",
    "signature",
    "sig",
    "token",
}


def _secret_url_findings(text: str) -> Iterable[Finding]:
    for match in URL_PATTERN.finditer(text):
        candidate = match.group(0).rstrip(".,;:!?")
        try:
            parsed = urlsplit(candidate)
        except ValueError:
            continue
        query_names = {name.lower() for name, _ in parse_qsl(parsed.query, keep_blank_values=True)}
        auth_code = "code" in query_names and re.search(r"(?i)(?:auth|oauth|login|signin|callback)", parsed.path)
        if query_names & SECRET_QUERY_NAMES or auth_code or parsed.username or parsed.password:
            yield Finding(
                "SECRET_BEARING_URL",
                match.start(),
                match.start() + len(candidate),
                True,
                "secret_url_v1",
            )


CHILD_CONTEXT = re.compile(r"(?i)\b(?:my|our)\s+(?:daughter|son|child|kid|children|kids)\b")
CHILD_IDENTIFIER = re.compile(
    r"(?i)\b(?:named|name\s+is|birthday|born|turn(?:ed|ing)?\s+\d|\d{1,2}\s+years?\s+old|school|teacher|classroom|daycare|camp|custody|pickup|drop[- ]?off)\b"
)
PRIVATE_CORRESPONDENCE = re.compile(
    r"(?im)(?:^\s*(?:from|to|cc|bcc|subject)\s*:\s*\S|"
    r"\b(?:draft|reply|respond|forward|send)\s+(?:an?\s+)?(?:email|message|text)\s+(?:to|from)\b|"
    r"\b(?:private|direct)\s+(?:email|message|correspondence)\b)"
)


def scan_text(text: str) -> list[Finding]:
    findings: list[Finding] = []
    masked = PLACEHOLDER.sub(lambda match: " " * (match.end() - match.start()), text)
    for kind, detector, pattern in SPAN_PATTERNS:
        findings.extend(Finding(kind, match.start(), match.end(), True, detector) for match in pattern.finditer(masked))
    findings.extend(_secret_url_findings(masked))
    return _merge_findings(findings)


def conversation_exclusion_reasons(texts: Iterable[str]) -> list[str]:
    combined = "\n".join(texts)
    reasons: list[str] = []
    if CHILD_CONTEXT.search(combined) and CHILD_IDENTIFIER.search(combined):
        reasons.append("child_private_context")
    if PRIVATE_CORRESPONDENCE.search(combined):
        reasons.append("private_third_party_correspondence")
    return reasons


def _merge_findings(findings: Iterable[Finding]) -> list[Finding]:
    ordered = sorted(findings, key=lambda item: (item.start, -(item.end - item.start), item.kind))
    selected: list[Finding] = []
    for finding in ordered:
        if any(finding.start < kept.end and kept.start < finding.end for kept in selected):
            continue
        selected.append(finding)
    return sorted(selected, key=lambda item: item.start)


def redact_text(text: str, findings: Iterable[Finding]) -> str:
    result: list[str] = []
    cursor = 0
    for finding in sorted(findings, key=lambda item: item.start):
        result.append(text[cursor : finding.start])
        result.append(f"[REDACTED:{finding.kind}]")
        cursor = finding.end
    result.append(text[cursor:])
    return "".join(result)

