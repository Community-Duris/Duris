#!/usr/bin/env python3
"""Focused contracts for the fail-closed Session 14 readiness gate."""

from __future__ import annotations

import importlib.util
import json
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


gate = load_module("session14_gate", ROOT / "scripts/session14_gate.py")
faults = load_module("session14_fault_adapter", ROOT / "tests/async/session14_fault_adapter.py")
reconcile = load_module("session14_reconcile", ROOT / "tests/async/session14_reconcile.py")
load_client = load_module("session14_load_client", ROOT / "tests/async/session14_load_client.py")
MANIFEST_PATH = ROOT / "tests/async/session14_gate_manifest.json"
EXAMPLE_CONFIG_PATH = ROOT / "tests/async/session14_gate_config.example.json"


class Session14GateTests(unittest.TestCase):
    def setUp(self):
        self.manifest = gate.validate_manifest(gate.load_json(MANIFEST_PATH))

    def qualified_config(self):
        return {
            "schema_version": 1,
            "configuration_id": "isolated-gate-v1",
            "environment": "test",
            "target_kind": "isolated_representative_clone",
            "production_unreachable": True,
            "backup_evidence_id": "backup-proof-v1",
            "restore_evidence_id": "restore-proof-v1",
            "rpo_policy_id": "rpo-approved-v1",
            "rpo_max_msec": 300000,
            "lifecycle_policy_id": "lifecycle-approved-v1",
            "lifecycle_policy_status": "approved",
            "identity_count": 200,
            "qualification_evidence_id": "qualification-proof-v1",
            "aggregate_table_counts": dict(self.manifest["representative_tables"]),
            "ports": {"database": 13306, "redis": 16379, "game": 14000},
            "qualification_adapter_argv": ["/safe/qualification-adapter"],
            "workload_adapter_argv": ["/safe/workload-adapter"],
            "fault_adapter_argv": ["/safe/fault-adapter"],
            "reconcile_adapter_argv": ["/safe/reconcile-adapter"]
        }

    def passing_metrics(self):
        metrics = {
            "pulse_p99_msec": 249,
            "event_p99_msec": 24,
            "critical_oldest_msec": 999,
            "sustained_event_debt": 0,
            "main_thread_external_io": 0,
            "checkpoint_age_msec": 1000,
        }
        metrics.update({name: True for name in self.manifest["required_true_metrics"]})
        return metrics

    def test_manifest_is_the_complete_binding_gate(self):
        self.assertEqual(self.manifest["ramps"], [25, 50, 100, 200])
        self.assertEqual(self.manifest["minimum_hold_seconds"], 1800)
        self.assertEqual(len(self.manifest["profiles"]), 8)
        self.assertEqual(len(self.manifest["faults"]), 28)
        self.assertIn("backup_restore_after_erasure", self.manifest["faults"])
        self.assertIn("critical_outbox_delivery", self.manifest["reconciliations"])
        self.assertIn("boot_drift_prewrite_rejection", self.manifest["privacy_cases"])

    def test_checked_in_config_example_is_deliberately_unqualified(self):
        config = gate.load_json(EXAMPLE_CONFIG_PATH)
        reasons = gate.qualify(config, self.manifest)
        self.assertIn("PRODUCTION_UNREACHABILITY_UNPROVEN", reasons)
        self.assertIn("LIFECYCLE_POLICY_NOT_APPROVED", reasons)
        self.assertIn("RPO_NOT_APPROVED", reasons)
        self.assertIn("LOAD_IDENTITIES_BELOW_200", reasons)
        self.assertTrue(any(reason.startswith("TABLE_BELOW_THRESHOLD:") for reason in reasons))
        self.assertIn("DATABASE_PORT_NOT_ISOLATED", reasons)
        self.assertIn("REDIS_PORT_NOT_ISOLATED", reasons)
        self.assertIn("GAME_PORT_NOT_ISOLATED", reasons)

    def test_qualified_config_requires_every_external_decision(self):
        config = self.qualified_config()
        self.assertEqual(gate.qualify(config, self.manifest), [])
        cases = {
            "unsafe environment": ("environment", "production", "ENVIRONMENT_NOT_SAFE"),
            "production reachability": ("production_unreachable", False,
                                         "PRODUCTION_UNREACHABILITY_UNPROVEN"),
            "pending policy": ("lifecycle_policy_status", "pending",
                               "LIFECYCLE_POLICY_NOT_APPROVED"),
            "unknown RPO": ("rpo_max_msec", 0, "RPO_NOT_APPROVED"),
            "too few identities": ("identity_count", 199, "LOAD_IDENTITIES_BELOW_200"),
        }
        for label, (key, value, reason) in cases.items():
            with self.subTest(label):
                changed = self.qualified_config()
                changed[key] = value
                self.assertIn(reason, gate.qualify(changed, self.manifest))

    def test_independent_qualification_response_must_match_declared_evidence(self):
        config = self.qualified_config()
        response = {
            "schema_version": 1,
            "state": "qualified",
            "qualification_evidence_id": config["qualification_evidence_id"],
            "target_kind": config["target_kind"],
            "production_unreachable": True,
            "identity_count": config["identity_count"],
            "aggregate_table_counts": config["aggregate_table_counts"],
            "ports": config["ports"],
        }
        self.assertEqual(gate.validate_qualification_response(
            config, self.manifest, response), [])
        for key in ("state", "qualification_evidence_id", "target_kind",
                    "production_unreachable", "identity_count",
                    "aggregate_table_counts", "ports"):
            with self.subTest(key):
                changed = dict(response)
                changed[key] = None
                self.assertIn(f"QUALIFICATION_RESPONSE_MISMATCH:{key}",
                              gate.validate_qualification_response(
                                  config, self.manifest, changed))

    def test_every_representative_threshold_and_isolated_port_is_enforced(self):
        for table, minimum in self.manifest["representative_tables"].items():
            with self.subTest(table):
                changed = self.qualified_config()
                changed["aggregate_table_counts"][table] = minimum - 1
                self.assertIn(f"TABLE_BELOW_THRESHOLD:{table}",
                              gate.qualify(changed, self.manifest))
        for service, default in gate.DEFAULT_PORTS.items():
            with self.subTest(service):
                changed = self.qualified_config()
                changed["ports"][service] = default
                self.assertIn(f"{service.upper()}_PORT_NOT_ISOLATED",
                              gate.qualify(changed, self.manifest))

    def test_sensitive_keys_and_values_never_enter_a_report(self):
        with self.assertRaises(gate.GateError):
            gate.sanitized({"password": "no"}, set())
        with self.assertRaises(gate.GateError):
            gate.sanitized({"detail": "non-ascii-\u00e9"}, set())
        with self.assertRaises(gate.GateError):
            gate.sanitized({"detail": "x" * 513}, set())
        changed = self.qualified_config()
        changed["ports"]["password"] = 1
        self.assertIn("SENSITIVE_CONFIG_KEY", gate.qualify(changed, self.manifest))

    def test_metric_limits_fail_closed(self):
        passing = self.passing_metrics()
        self.assertEqual(gate.validate_metrics(passing, self.manifest, 1000), [])
        for name in ("pulse_p99_msec", "event_p99_msec", "critical_oldest_msec",
                     "sustained_event_debt", "main_thread_external_io",
                     "checkpoint_age_msec"):
            changed = dict(passing)
            changed[name] = 1000000
            self.assertTrue(gate.validate_metrics(changed, self.manifest, 1000))
        for invalid in (True, -1, float("nan"), float("inf")):
            changed = dict(passing)
            changed["pulse_p99_msec"] = invalid
            self.assertIn("METRIC_FAILED:pulse_p99_msec",
                          gate.validate_metrics(changed, self.manifest, 1000))
        for name in self.manifest["required_true_metrics"]:
            changed = dict(passing)
            changed[name] = False
            self.assertIn(f"METRIC_FAILED:{name}",
                          gate.validate_metrics(changed, self.manifest, 1000))

    def test_complete_case_matrix_is_required_for_a_pass(self):
        original = gate.run_json_adapter

        def passing_adapter(argv, request, timeout_seconds):
            del argv, timeout_seconds
            case_id = request["case_id"]
            evidence_id = "evidence-" + gate.stable_hash(request)[:32]
            if "reconciliation_id" in request:
                value = request["reconciliation_id"]
                return ({"schema_version": 1, "reconciliation_id": value,
                         "mismatches": 0, "evidence_id": evidence_id}, 0.0)
            if "privacy_case_id" in request:
                value = request["privacy_case_id"]
                return ({"schema_version": 1, "privacy_case_id": value,
                         "state": "passed", "evidence_id": evidence_id}, 0.0)
            if case_id.startswith("workload:"):
                return ({"schema_version": 1, "case_id": case_id, "state": "passed",
                         "evidence_id": evidence_id,
                         "metrics": self.passing_metrics()},
                        float(request["minimum_duration_seconds"]))
            states = {"preflight": "ready", "inject": "injected",
                      "verify": "verified", "teardown": "restored"}
            return ({"schema_version": 1, "action": request["action"],
                     "state": states[request["phase"]],
                     "detail_id": evidence_id}, 0.0)

        gate.run_json_adapter = passing_adapter
        try:
            report = gate.execute(self.qualified_config(), self.manifest, ROOT / "tmp")
        finally:
            gate.run_json_adapter = original
        self.assertEqual(report["result"], "PASS")
        self.assertTrue(report["readiness_claim"])
        self.assertEqual(report["case_count"], 70)
        self.assertEqual(report["case_count"], report["expected_case_count"])
        self.assertEqual(report["failures"], [])
        evidence_ids = [evidence_id for case in report["cases"]
                        for evidence_id in case["evidence_ids"]]
        self.assertEqual(len(evidence_ids), len(set(evidence_ids)))

    def test_adapter_failures_produce_complete_fail_closed_case_evidence(self):
        original = gate.run_json_adapter

        def failing_adapter(argv, request, timeout_seconds):
            del argv, request, timeout_seconds
            raise gate.GateError("simulated adapter failure")

        gate.run_json_adapter = failing_adapter
        try:
            report = gate.execute(self.qualified_config(), self.manifest, ROOT / "tmp")
        finally:
            gate.run_json_adapter = original
        self.assertEqual(report["result"], "FAIL")
        self.assertFalse(report["readiness_claim"])
        self.assertEqual(report["case_count"], report["expected_case_count"])
        self.assertTrue(report["failures"])

    def test_preflight_writes_permission_restricted_ignored_evidence(self):
        gate.DEFAULT_OUTPUT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=gate.DEFAULT_OUTPUT) as directory:
            output = Path(directory) / "evidence"
            report = gate.preflight_report(self.manifest, ["RPO_NOT_APPROVED"])
            gate.write_report(output, report)
            report_path = output / "sanitized-report.json"
            self.assertEqual(json.loads(report_path.read_text())["result"], "UNQUALIFIED")
            self.assertEqual(stat.S_IMODE(output.stat().st_mode), 0o700)
            self.assertEqual(stat.S_IMODE(report_path.stat().st_mode), 0o600)
            ignored = subprocess.run(
                ["git", "check-ignore", "-q", str(report_path.relative_to(ROOT))],
                cwd=ROOT, check=False,
            )
            self.assertEqual(ignored.returncode, 0)

    def test_report_output_cannot_escape_ignored_gate_directory(self):
        report = gate.preflight_report(self.manifest, ["RPO_NOT_APPROVED"])
        with self.assertRaisesRegex(gate.GateError, "must stay under"):
            gate.write_report(ROOT / "docs", report)

    def test_argv_protocol_rejects_shell_and_schema_shortcuts(self):
        self.assertEqual(faults.validate_argv(["/bin/echo", "safe"]), ["/bin/echo", "safe"])
        with self.assertRaises(ValueError):
            faults.validate_argv("/bin/echo unsafe")
        with self.assertRaises(ValueError):
            faults.run_adapter([], "fault", "inject")
        with self.assertRaisesRegex(ValueError, "allow-listed"):
            faults.run_adapter(["/bin/echo"], "not-a-fault", "inject")
        with self.assertRaises(ValueError):
            reconcile.validate_result("owners", {"schema_version": 1})
        with self.assertRaises(ValueError):
            reconcile.validate_result("owners", {
                "schema_version": 1, "reconciliation_id": "owners",
                "mismatches": False, "checked": 1, "evidence_id": "evidence-1",
            })
        with self.assertRaises(ValueError):
            reconcile.run_reconciliation([], "owners")

    def test_load_client_lines_are_bounded_and_commands_are_capped(self):
        self.assertEqual(load_client._line("look"), b"look\n")
        for invalid in ("", "bad\nline", "x" * 257):
            with self.subTest(invalid=invalid[:8]):
                with self.assertRaises(ValueError):
                    load_client._line(invalid)
        identity = load_client.ClientIdentity("a", "b", "c")
        with self.assertRaises(ValueError):
            load_client.run_client("127.0.0.1", 1, identity, ["look"] * 129, 0)

    def test_missing_config_cli_produces_no_readiness_claim(self):
        gate.DEFAULT_OUTPUT.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=gate.DEFAULT_OUTPUT) as directory:
            output = Path(directory) / "output"
            missing = Path(directory) / "absent.json"
            completed = subprocess.run(
                [sys.executable, str(ROOT / "scripts/session14_gate.py"),
                 "--config", str(missing), "--preflight-only", "--output", str(output)],
                cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                check=False,
            )
            self.assertEqual(completed.returncode, 2)
            report = json.loads((output / "sanitized-report.json").read_text())
            self.assertEqual(report["result"], "UNQUALIFIED")
            self.assertFalse(report["readiness_claim"])


if __name__ == "__main__":
    unittest.main()
