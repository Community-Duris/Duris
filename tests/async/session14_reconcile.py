#!/usr/bin/env python3
"""Aggregate-only Session 14 reconciliation result validation."""

from __future__ import annotations

import json
import re
import subprocess
from dataclasses import dataclass


SAFE_ID = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.:-]{0,79}$")


@dataclass(frozen=True)
class ReconciliationResult:
    reconciliation_id: str
    mismatches: int
    checked: int
    evidence_id: str


def validate_result(expected_id: str, payload: object) -> ReconciliationResult:
    if not isinstance(payload, dict):
        raise ValueError("reconciliation output must be an object")
    required = {"schema_version", "reconciliation_id", "mismatches", "checked", "evidence_id"}
    if set(payload) != required or payload["schema_version"] != 1:
        raise ValueError("reconciliation output schema mismatch")
    if payload["reconciliation_id"] != expected_id:
        raise ValueError("reconciliation identity mismatch")
    checked = payload["checked"]
    mismatches = payload["mismatches"]
    if (not isinstance(checked, int) or isinstance(checked, bool) or checked < 0 or
            not isinstance(mismatches, int) or isinstance(mismatches, bool) or mismatches < 0):
        raise ValueError("reconciliation counts must be non-negative integers")
    evidence_id = payload["evidence_id"]
    if not isinstance(evidence_id, str) or SAFE_ID.fullmatch(evidence_id) is None:
        raise ValueError("invalid reconciliation evidence ID")
    return ReconciliationResult(expected_id, mismatches, checked, evidence_id)


def run_reconciliation(argv: list[str], reconciliation_id: str, timeout_seconds: int = 60) -> ReconciliationResult:
    if (not isinstance(argv, list) or not argv or len(argv) > 32 or
            not all(isinstance(value, str) and value and "\x00" not in value for value in argv)):
        raise ValueError("reconciliation argv is invalid")
    if SAFE_ID.fullmatch(reconciliation_id) is None:
        raise ValueError("reconciliation ID is invalid")
    request = json.dumps({"schema_version": 1, "reconciliation_id": reconciliation_id}) + "\n"
    completed = subprocess.run(
        argv, input=request, text=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
        timeout=timeout_seconds, check=False, shell=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(f"reconciliation failed: {reconciliation_id}")
    lines = completed.stdout.splitlines()
    if len(lines) != 1:
        raise RuntimeError("reconciliation returned an invalid response count")
    return validate_result(reconciliation_id, json.loads(lines[0]))
