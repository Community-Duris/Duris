#!/usr/bin/env python3
"""Fail-closed orchestrator for the final Phase 03 readiness gate."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import subprocess
import sys
import time
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "tests/async/session14_gate_manifest.json"
DEFAULT_OUTPUT = ROOT / "tmp/session14-gate"
SAFE_ENVIRONMENTS = {"development", "dev", "test"}
DEFAULT_PORTS = {"database": 3306, "redis": 6379, "game": 7777}
SAFE_ID = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.:-]{0,79}$")
SENSITIVE_KEY = re.compile(
    r"password|passwd|credential|secret|token|email|ip_address|hostname|account_name|character_name|sql|row_value",
    re.IGNORECASE,
)


class GateError(RuntimeError):
    pass


def load_json(path: Path) -> Any:
    with path.open(encoding="ascii") as handle:
        return json.load(handle)


def stable_hash(value: Any) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("ascii")
    return hashlib.sha256(encoded).hexdigest()


def safe_id(value: object) -> bool:
    return isinstance(value, str) and SAFE_ID.fullmatch(value) is not None


def validate_manifest(manifest: object) -> dict[str, Any]:
    if not isinstance(manifest, dict) or manifest.get("schema_version") != 1:
        raise GateError("manifest schema mismatch")
    if manifest.get("ramps") != [25, 50, 100, 200]:
        raise GateError("manifest ramps do not match the binding gate")
    profiles = manifest.get("profiles")
    if not isinstance(profiles, list) or len(profiles) != 8:
        raise GateError("manifest must contain exactly eight profiles")
    profile_ids = [profile.get("id") for profile in profiles if isinstance(profile, dict)]
    if len(profile_ids) != 8 or len(set(profile_ids)) != 8 or not all(safe_id(value) for value in profile_ids):
        raise GateError("manifest profile IDs are invalid")
    if manifest.get("hold_clients") != 200 or manifest.get("minimum_hold_seconds", 0) < 1800:
        raise GateError("manifest weakens the 200-client hold")
    for key in ("faults", "reconciliations", "privacy_cases"):
        values = manifest.get(key)
        if not isinstance(values, list) or not values or len(values) != len(set(values)):
            raise GateError(f"manifest {key} are incomplete or duplicated")
        if not all(safe_id(value) for value in values):
            raise GateError(f"manifest {key} contain invalid IDs")
    required_true = manifest.get("required_true_metrics")
    if (not isinstance(required_true, list) or not required_true or
            len(required_true) != len(set(required_true)) or
            not all(safe_id(value) for value in required_true)):
        raise GateError("manifest required true metrics are invalid")
    tables = manifest.get("representative_tables")
    if not isinstance(tables, dict) or not tables or not all(
        safe_id(key) and isinstance(value, int) and value > 0 for key, value in tables.items()
    ):
        raise GateError("manifest representative thresholds are invalid")
    return manifest


def validate_argv(value: object, name: str) -> list[str]:
    if not isinstance(value, list) or not value or len(value) > 32:
        raise GateError(f"{name} must contain 1-32 argv entries")
    if not all(isinstance(part, str) and part and "\x00" not in part for part in value):
        raise GateError(f"{name} contains an invalid argv entry")
    return list(value)


def qualify(config: object, manifest: dict[str, Any]) -> list[str]:
    reasons: list[str] = []
    if not isinstance(config, dict) or config.get("schema_version") != 1:
        return ["CONFIG_SCHEMA_INVALID"]
    def contains_sensitive_key(value: object) -> bool:
        if isinstance(value, dict):
            return any(SENSITIVE_KEY.search(str(key)) or contains_sensitive_key(item)
                       for key, item in value.items())
        if isinstance(value, list):
            return any(contains_sensitive_key(item) for item in value)
        return False

    if contains_sensitive_key(config):
        reasons.append("SENSITIVE_CONFIG_KEY")
    if config.get("environment") not in SAFE_ENVIRONMENTS:
        reasons.append("ENVIRONMENT_NOT_SAFE")
    if not safe_id(config.get("configuration_id")):
        reasons.append("CONFIGURATION_ID_MISSING")
    if config.get("target_kind") != "isolated_representative_clone":
        reasons.append("TARGET_NOT_ISOLATED_CLONE")
    if config.get("production_unreachable") is not True:
        reasons.append("PRODUCTION_UNREACHABILITY_UNPROVEN")
    for name in ("backup_evidence_id", "restore_evidence_id", "rpo_policy_id", "lifecycle_policy_id"):
        if not safe_id(config.get(name)):
            reasons.append(f"{name.upper()}_MISSING")
    if config.get("lifecycle_policy_status") != "approved":
        reasons.append("LIFECYCLE_POLICY_NOT_APPROVED")
    if (not isinstance(config.get("rpo_max_msec"), int) or
            isinstance(config.get("rpo_max_msec"), bool) or
            config.get("rpo_max_msec", 0) <= 0):
        reasons.append("RPO_NOT_APPROVED")
    if not isinstance(config.get("identity_count"), int) or config.get("identity_count", 0) < 200:
        reasons.append("LOAD_IDENTITIES_BELOW_200")
    counts = config.get("aggregate_table_counts")
    if not isinstance(counts, dict):
        reasons.append("REPRESENTATIVE_COUNTS_MISSING")
    else:
        for table, minimum in manifest["representative_tables"].items():
            observed = counts.get(table)
            if not isinstance(observed, int) or observed < minimum:
                reasons.append(f"TABLE_BELOW_THRESHOLD:{table}")
    ports = config.get("ports")
    if not isinstance(ports, dict):
        reasons.append("ISOLATED_PORTS_MISSING")
    else:
        for service, default in DEFAULT_PORTS.items():
            value = ports.get(service)
            if not isinstance(value, int) or not 1024 <= value <= 65535 or value == default:
                reasons.append(f"{service.upper()}_PORT_NOT_ISOLATED")
    if not safe_id(config.get("qualification_evidence_id")):
        reasons.append("QUALIFICATION_EVIDENCE_ID_MISSING")
    for name in ("qualification_adapter_argv", "workload_adapter_argv",
                 "fault_adapter_argv", "reconcile_adapter_argv"):
        try:
            validate_argv(config.get(name), name)
        except GateError:
            reasons.append(f"{name.upper()}_INVALID")
    return sorted(set(reasons))


def validate_qualification_response(config: dict[str, Any], manifest: dict[str, Any],
                                    response: object) -> list[str]:
    if not isinstance(response, dict) or response.get("schema_version") != 1:
        return ["QUALIFICATION_RESPONSE_SCHEMA_INVALID"]
    reasons = []
    expected = {
        "state": "qualified",
        "qualification_evidence_id": config["qualification_evidence_id"],
        "target_kind": config["target_kind"],
        "production_unreachable": True,
        "identity_count": config["identity_count"],
        "aggregate_table_counts": config["aggregate_table_counts"],
        "ports": config["ports"],
    }
    for key, value in expected.items():
        if response.get(key) != value:
            reasons.append(f"QUALIFICATION_RESPONSE_MISMATCH:{key}")
    response_counts = response.get("aggregate_table_counts")
    if (not isinstance(response_counts, dict) or
            set(response_counts) != set(manifest["representative_tables"])):
        reasons.append("QUALIFICATION_RESPONSE_TABLE_SET_INVALID")
    return sorted(set(reasons))


def run_qualification(config: dict[str, Any], manifest: dict[str, Any]) -> list[str]:
    try:
        response, _ = run_json_adapter(
            validate_argv(config["qualification_adapter_argv"], "qualification_adapter_argv"),
            {"schema_version": 1, "case_id": "qualification",
             "representative_tables": manifest["representative_tables"],
             "minimum_identities": 200, "require_production_unreachable": True},
            120,
        )
    except Exception as error:
        return [f"QUALIFICATION_ADAPTER_FAILED:{type(error).__name__}"]
    return validate_qualification_response(config, manifest, response)


def run_json_adapter(argv: list[str], request: dict[str, Any], timeout_seconds: int) -> tuple[dict[str, Any], float]:
    started = time.monotonic()
    completed = subprocess.run(
        argv,
        input=json.dumps(request, sort_keys=True) + "\n",
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        timeout=timeout_seconds,
        check=False,
        shell=False,
    )
    elapsed = time.monotonic() - started
    if completed.returncode != 0:
        raise GateError(f"adapter failed for case {request.get('case_id', 'unknown')}")
    lines = completed.stdout.splitlines()
    if len(lines) != 1:
        raise GateError("adapter returned an invalid response count")
    response = json.loads(lines[0])
    if not isinstance(response, dict) or response.get("schema_version") != 1:
        raise GateError("adapter response schema mismatch")
    return response, elapsed


def sanitized(value: Any, forbidden_keys: set[str]) -> Any:
    if isinstance(value, dict):
        clean = {}
        for key, item in value.items():
            lowered = str(key).lower()
            if lowered in forbidden_keys or SENSITIVE_KEY.search(lowered):
                raise GateError(f"sensitive report key rejected: {key}")
            clean[str(key)] = sanitized(item, forbidden_keys)
        return clean
    if isinstance(value, list):
        return [sanitized(item, forbidden_keys) for item in value]
    if isinstance(value, str):
        if not value.isascii() or len(value) > 512:
            raise GateError("report string is not bounded ASCII")
        return value
    if value is None or isinstance(value, (bool, int, float)):
        return value
    raise GateError("unsupported report value")


def validate_metrics(metrics: object, manifest: dict[str, Any], rpo_max_msec: int) -> list[str]:
    if not isinstance(metrics, dict):
        return ["METRICS_MISSING"]
    thresholds = manifest["metrics"]
    required_maxima = {
        "pulse_p99_msec": thresholds["pulse_p99_msec_max"],
        "event_p99_msec": thresholds["event_p99_msec_max"],
        "critical_oldest_msec": thresholds["critical_oldest_msec_max"],
        "sustained_event_debt": thresholds["sustained_event_debt_max"],
        "main_thread_external_io": thresholds["main_thread_external_io_max"],
        "checkpoint_age_msec": rpo_max_msec,
    }
    failures = []
    for name, maximum in required_maxima.items():
        observed = metrics.get(name)
        if (not isinstance(observed, (int, float)) or isinstance(observed, bool) or
                not math.isfinite(observed) or observed < 0 or observed > maximum):
            failures.append(f"METRIC_FAILED:{name}")
    for name in manifest.get("required_true_metrics", []):
        if metrics.get(name) is not True:
            failures.append(f"METRIC_FAILED:{name}")
    return failures


def preflight_report(manifest: dict[str, Any], reasons: list[str]) -> dict[str, Any]:
    report = {
        "schema_version": 1,
        "gate_id": manifest["gate_id"],
        "manifest_sha256": stable_hash(manifest),
        "result": "QUALIFIED" if not reasons else "UNQUALIFIED",
        "qualification_reasons": reasons,
        "readiness_claim": False,
    }
    report["evidence_sha256"] = stable_hash(report)
    return report


def safe_evidence_id(response: object) -> str | None:
    if not isinstance(response, dict):
        return None
    value = response.get("evidence_id")
    return value if safe_id(value) else None


def execute(config: dict[str, Any], manifest: dict[str, Any], output: Path) -> dict[str, Any]:
    workload_argv = validate_argv(config["workload_adapter_argv"], "workload_adapter_argv")
    fault_argv = validate_argv(config["fault_adapter_argv"], "fault_adapter_argv")
    reconcile_argv = validate_argv(config["reconcile_adapter_argv"], "reconcile_adapter_argv")
    cases: list[dict[str, Any]] = []
    failures: list[str] = []
    seen_evidence_ids: set[str] = set()

    def record_evidence(value: object, evidence_ids: list[str]) -> bool:
        if not safe_id(value) or value in seen_evidence_ids:
            return False
        seen_evidence_ids.add(value)
        evidence_ids.append(value)
        return True

    minimum_hold = manifest["minimum_hold_seconds"]
    for profile in manifest["profiles"]:
        for clients in manifest["ramps"]:
            case_id = f"workload:{profile['id']}:{clients}"
            requested_seconds = minimum_hold if clients == manifest["hold_clients"] else 0
            case_failures = []
            evidence_ids = []
            elapsed = 0.0
            try:
                response, elapsed = run_json_adapter(
                    workload_argv,
                    {"schema_version": 1, "case_id": case_id, "profile": profile["id"],
                     "clients": clients, "minimum_duration_seconds": requested_seconds},
                    max(300, requested_seconds + 300),
                )
                if response.get("case_id") != case_id or response.get("state") != "passed":
                    case_failures.append("WORKLOAD_NOT_PASSED")
                evidence_id = safe_evidence_id(response)
                if evidence_id is None or not record_evidence(evidence_id, evidence_ids):
                    case_failures.append("WORKLOAD_EVIDENCE_ID_INVALID")
                if requested_seconds and elapsed < requested_seconds:
                    case_failures.append("HOLD_SHORTER_THAN_MINIMUM")
                case_failures.extend(validate_metrics(
                    response.get("metrics"), manifest, config["rpo_max_msec"]))
            except Exception as error:
                case_failures.append(f"WORKLOAD_ADAPTER_FAILED:{type(error).__name__}")
            failures.extend(f"{case_id}:{failure}" for failure in case_failures)
            for reconciliation_id in manifest["reconciliations"]:
                try:
                    response, _ = run_json_adapter(
                        reconcile_argv,
                        {"schema_version": 1, "case_id": case_id,
                         "reconciliation_id": reconciliation_id}, 120,
                    )
                    evidence_id = safe_evidence_id(response)
                    if (response.get("reconciliation_id") != reconciliation_id or
                            response.get("mismatches") != 0 or evidence_id is None or
                            not record_evidence(evidence_id, evidence_ids)):
                        raise GateError("reconciliation result invalid")
                except Exception as error:
                    item = f"RECONCILIATION_FAILED:{reconciliation_id}:{type(error).__name__}"
                    case_failures.append(item)
                    failures.append(f"{case_id}:{item}")
            cases.append({"case_id": case_id, "elapsed_seconds": int(elapsed),
                          "evidence_ids": evidence_ids, "failures": case_failures})
    target_safe = True
    for fault_id in manifest["faults"]:
        case_id = f"fault:{fault_id}"
        evidence_ids = []
        if not target_safe:
            item = f"{case_id}:SKIPPED_AFTER_TEARDOWN_FAILURE"
            failures.append(item)
            cases.append({"case_id": case_id, "evidence_ids": evidence_ids,
                          "failures": [item]})
            continue
        try:
            expected_states = {
                "preflight": "ready", "inject": "injected",
                "verify": "verified", "teardown": "restored",
            }
            for phase in ("preflight", "inject", "verify", "teardown"):
                response, _ = run_json_adapter(
                    fault_argv,
                    {"schema_version": 1, "case_id": case_id, "action": fault_id, "phase": phase},
                    120,
                )
                if (response.get("action") != fault_id or
                        response.get("state") != expected_states[phase]):
                    raise GateError(f"fault phase failed: {fault_id}:{phase}")
                detail_id = response.get("detail_id")
                if not record_evidence(detail_id, evidence_ids):
                    raise GateError(f"fault evidence ID invalid: {fault_id}:{phase}")
        except Exception as error:
            failures.append(f"{case_id}:FAULT_FAILED:{type(error).__name__}")
            try:
                response, _ = run_json_adapter(
                    fault_argv,
                    {"schema_version": 1, "case_id": case_id, "action": fault_id, "phase": "teardown"},
                    120,
                )
                if response.get("action") != fault_id or response.get("state") != "restored":
                    raise GateError("fault compensation teardown was not restored")
                if not record_evidence(response.get("detail_id"), evidence_ids):
                    raise GateError("fault compensation evidence ID invalid")
            except Exception:
                failures.append(f"{case_id}:TEARDOWN_FAILED")
                target_safe = False
        fault_failures = [failure for failure in failures if failure.startswith(f"{case_id}:")]
        for reconciliation_id in manifest["reconciliations"]:
            try:
                response, _ = run_json_adapter(
                    reconcile_argv,
                    {"schema_version": 1, "case_id": case_id,
                     "reconciliation_id": reconciliation_id}, 120,
                )
                evidence_id = safe_evidence_id(response)
                if (response.get("reconciliation_id") != reconciliation_id or
                        response.get("mismatches") != 0 or evidence_id is None or
                        not record_evidence(evidence_id, evidence_ids)):
                    raise GateError("reconciliation result invalid")
            except Exception as error:
                item = f"{case_id}:RECONCILIATION_FAILED:{reconciliation_id}"
                failures.append(item)
                fault_failures.append(f"{item}:{type(error).__name__}")
        cases.append({"case_id": case_id, "evidence_ids": evidence_ids,
                      "failures": fault_failures})
    for privacy_case_id in manifest["privacy_cases"]:
        case_id = f"privacy:{privacy_case_id}"
        if not target_safe:
            item = f"{case_id}:SKIPPED_AFTER_TEARDOWN_FAILURE"
            failures.append(item)
            cases.append({"case_id": case_id, "evidence_ids": [],
                          "failures": [item]})
            continue
        case_failures = []
        evidence_ids = []
        try:
            response, _ = run_json_adapter(
                reconcile_argv,
                {"schema_version": 1, "case_id": case_id,
                 "privacy_case_id": privacy_case_id}, 300,
            )
            evidence_id = safe_evidence_id(response)
            if (response.get("privacy_case_id") != privacy_case_id or
                    response.get("state") != "passed" or evidence_id is None):
                raise GateError("privacy result invalid")
            if not record_evidence(evidence_id, evidence_ids):
                raise GateError("privacy evidence ID invalid or duplicated")
        except Exception as error:
            item = f"{case_id}:PRIVACY_CASE_FAILED:{type(error).__name__}"
            failures.append(item)
            case_failures.append(item)
        cases.append({"case_id": case_id, "evidence_ids": evidence_ids,
                      "failures": case_failures})
    expected_case_count = len(manifest["profiles"]) * len(manifest["ramps"]) + len(
        manifest["faults"]) + len(manifest["privacy_cases"])
    if len(cases) != expected_case_count:
        failures.append("CASE_COVERAGE_INCOMPLETE")
    result = "PASS" if not failures and len(cases) == expected_case_count else "FAIL"
    report = {
        "schema_version": 1,
        "gate_id": manifest["gate_id"],
        "manifest_sha256": stable_hash(manifest),
        "configuration_id": config["configuration_id"],
        "result": result,
        "readiness_claim": result == "PASS",
        "case_count": len(cases),
        "expected_case_count": expected_case_count,
        "cases": cases,
        "failures": failures,
    }
    forbidden = {value.lower() for value in manifest["forbidden_report_keys"]}
    report = sanitized(report, forbidden)
    report["evidence_sha256"] = stable_hash(report)
    return report


def write_report(output: Path, report: dict[str, Any]) -> None:
    resolved_root = DEFAULT_OUTPUT.resolve()
    resolved_output = output.resolve()
    try:
        resolved_root.relative_to(ROOT.resolve())
        resolved_output.relative_to(resolved_root)
    except ValueError as error:
        raise GateError("report output must stay under tmp/session14-gate") from error
    output = resolved_output
    output.mkdir(parents=True, exist_ok=True, mode=0o700)
    os.chmod(output, 0o700)
    path = output / "sanitized-report.json"
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="ascii")
    os.chmod(path, 0o600)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--config", type=Path, required=True,
                        help="separate Session 14 JSON config; repository .env is never read")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--preflight-only", action="store_true")
    args = parser.parse_args()
    manifest = validate_manifest(load_json(args.manifest))
    try:
        config = load_json(args.config)
    except (OSError, ValueError, UnicodeError):
        config = None
    reasons = qualify(config, manifest)
    if not reasons:
        reasons.extend(run_qualification(config, manifest))
    report = preflight_report(manifest, reasons)
    if not reasons and not args.preflight_only:
        try:
            report = execute(config, manifest, args.output)
        except Exception as error:
            report = {
                "schema_version": 1,
                "gate_id": manifest["gate_id"],
                "manifest_sha256": stable_hash(manifest),
                "result": "FAIL",
                "readiness_claim": False,
                "failures": [f"ORCHESTRATOR_FAILED:{type(error).__name__}"],
            }
            report["evidence_sha256"] = stable_hash(report)
    write_report(args.output, report)
    print(f"session14 gate result={report['result']} report={args.output / 'sanitized-report.json'}")
    return 0 if report["result"] in {"QUALIFIED", "PASS"} else 2


if __name__ == "__main__":
    raise SystemExit(main())
