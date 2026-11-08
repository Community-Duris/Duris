#!/usr/bin/env python3
"""Assess sanitized telemetry readiness evidence; never connect or deploy.

Observations must come from a separately authorized inspection. This command
validates their shape and classifies missing evidence; it is not a substitute
for native schema validation, live readback, or operator approval.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys
from typing import Any

SCHEMA_VERSION = 1
MAX_INPUT_BYTES = 65_536
FIELDS = {
    "runtime": {
        "source_revision": "revision", "binary_sha256": "hash",
        "expected_source_revision": "revision", "expected_binary_sha256": "hash",
        "backend": "backend", "schema_head": "migration", "expected_schema_head": "migration",
    },
    "writer": {
        "credentials_present": "bool", "dedicated_identity_verified": "bool",
        "schema_contract_verified": "bool", "least_privilege_verified": "bool",
    },
    "catalog": {
        "present": "bool", "effective_digest_verified": "bool",
        "historical_mappings_preserved": "bool",
    },
    "outage": {
        "durable_path_writable": "bool", "exclusive_writer_verified": "bool",
        "historical_ledger_verified": "bool",
    },
    "operations": {
        "observational_only_reviewed": "bool", "privacy_lifecycle_reviewed": "bool",
        "rollback_verified": "bool", "restart_approved": "bool", "config_change_approved": "bool",
    },
    "ingestion": {
        "enabled": "bool", "new_session_record_verified": "bool",
        "new_progression_record_verified": "bool", "health_progress_verified": "bool",
        "gameplay_save_verified": "bool",
    },
}


class EvidenceError(ValueError):
    """Invalid observations; diagnostics intentionally never echo values."""


def template() -> dict[str, Any]:
    return {"schema_version": SCHEMA_VERSION,
            **{section: {name: None for name in fields} for section, fields in FIELDS.items()}}


def validate(observations: Any) -> dict[str, Any]:
    if not isinstance(observations, dict) or set(observations) != {"schema_version", *FIELDS}:
        raise EvidenceError("unsupported or missing observation section")
    if type(observations["schema_version"]) is not int or observations["schema_version"] != SCHEMA_VERSION:
        raise EvidenceError("unsupported observation schema version")
    for section, fields in FIELDS.items():
        values = observations[section]
        if not isinstance(values, dict) or set(values) != set(fields):
            raise EvidenceError("unsupported or missing observation field")
        for name, kind in fields.items():
            value = values[name]
            if value is None:
                continue
            if kind == "bool":
                valid = type(value) is bool
            elif kind == "backend":
                valid = type(value) is str and value in {"sql", "flatfile", "unknown"}
            else:
                pattern = {"revision": r"[0-9a-f]{40}", "hash": r"[0-9a-f]{64}",
                           "migration": r"[0-9]{4}_[a-z0-9_]{1,120}"}[kind]
                valid = type(value) is str and re.fullmatch(pattern, value) is not None
            if not valid:
                raise EvidenceError("invalid typed observation value")
    return observations


def assess(observations: Any) -> dict[str, Any]:
    evidence = validate(observations)
    blockers: list[str] = []
    actions: list[str] = []
    live_checks: list[str] = []
    runtime = evidence["runtime"]
    if runtime["backend"] != "sql":
        blockers.append("sql_authority_not_verified")
    for name in ("source_revision", "binary_sha256", "expected_source_revision", "expected_binary_sha256"):
        if runtime[name] is None:
            blockers.append("runtime_" + name + "_unverified")
    for actual, expected in (("source_revision", "expected_source_revision"),
                             ("binary_sha256", "expected_binary_sha256")):
        if runtime[actual] is not None and runtime[expected] is not None and runtime[actual] != runtime[expected]:
            actions.append("approved_candidate_deployment_required_" + actual)
    if runtime["schema_head"] is None or runtime["expected_schema_head"] is None:
        blockers.append("schema_head_unverified")
    elif runtime["schema_head"] != runtime["expected_schema_head"]:
        blockers.append("binary_schema_pair_mismatch_requires_review")
    for section in ("writer", "catalog", "outage"):
        for name, value in evidence[section].items():
            if value is not True:
                blockers.append(section + "_" + name + "_unverified")
    for name in ("observational_only_reviewed", "privacy_lifecycle_reviewed", "rollback_verified"):
        if evidence["operations"][name] is not True:
            blockers.append("operations_" + name + "_unverified")
    for name in ("restart_approved", "config_change_approved"):
        if evidence["operations"][name] is not True:
            actions.append("explicit_" + name + "_required_before_mutation")
    if evidence["ingestion"]["enabled"] is not True:
        actions.append("approved_observational_enablement_required")
    for name, value in evidence["ingestion"].items():
        if name != "enabled" and value is not True:
            live_checks.append(name)
    state = ("blocked" if blockers else "awaiting_authorized_rollout" if actions
             else "awaiting_live_verification" if live_checks else "verified_collecting")
    return {
        "schema_version": SCHEMA_VERSION, "state": state,
        "evidence_assessment_only": True, "preparation_blockers": blockers,
        "rollout_actions": actions, "required_live_checks": live_checks,
        "deployment_performed": False, "database_or_account_changes_performed": False,
        "automatic_balance_changes_performed": False,
    }


def _unique_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for name, value in pairs:
        if name in result:
            raise EvidenceError("duplicate observation field")
        result[name] = value
    return result


def read_observations(path: Path) -> dict[str, Any]:
    with path.open("rb") as stream:
        raw = stream.read(MAX_INPUT_BYTES + 1)
    if len(raw) > MAX_INPUT_BYTES:
        raise EvidenceError("observation input exceeds byte bound")
    try:
        value = json.loads(raw, object_pairs_hook=_unique_object)
    except (json.JSONDecodeError, UnicodeDecodeError, RecursionError) as error:
        raise EvidenceError("observation input is not valid JSON") from error
    return validate(value)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("observations", nargs="?", type=Path)
    parser.add_argument("--template", action="store_true", help="print an all-unknown, non-secret evidence template")
    args = parser.parse_args(argv)
    if args.template:
        if args.observations is not None:
            parser.error("choose either observations or --template")
        print(json.dumps(template(), indent=2, sort_keys=True))
        return 0
    if args.observations is None:
        parser.error("an observations file is required")
    try:
        result = assess(read_observations(args.observations))
    except (EvidenceError, OSError):
        # Never include file contents, connector errors, secrets, or supplied
        # invalid values in the public diagnostic.
        print("telemetry preflight: evidence input refused", file=sys.stderr)
        return 2
    print(json.dumps(result, indent=2, sort_keys=True))
    return 2 if result["state"] == "blocked" else 0 if result["state"] == "verified_collecting" else 3


if __name__ == "__main__":
    raise SystemExit(main())
