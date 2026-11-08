#!/usr/bin/env python3
"""Executable, synthetic-only rehearsal of the non-mutating readiness tool."""
from __future__ import annotations

from copy import deepcopy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/telemetry/preflight.py"
spec = importlib.util.spec_from_file_location("telemetry_preflight", SCRIPT)
assert spec is not None and spec.loader is not None
preflight = importlib.util.module_from_spec(spec)
spec.loader.exec_module(preflight)


def synthetic_complete_evidence():
    evidence = preflight.template()
    for section, fields in preflight.FIELDS.items():
        for name, kind in fields.items():
            evidence[section][name] = (True if kind == "bool" else "sql" if kind == "backend"
                                      else "a" * 40 if kind == "revision"
                                      else "b" * 64 if kind == "hash" else "0042_synthetic_fixture")
    return evidence


class PreflightTests(unittest.TestCase):
    def test_unknown_evidence_never_means_zero_or_ready(self):
        report = preflight.assess(preflight.template())
        self.assertEqual(report["state"], "blocked")
        self.assertTrue(report["preparation_blockers"])
        self.assertTrue(report["required_live_checks"])
        self.assertTrue(report["evidence_assessment_only"])
        self.assertFalse(report["deployment_performed"])
        self.assertFalse(report["database_or_account_changes_performed"])

    def test_every_required_proof_is_individually_enforced(self):
        good = synthetic_complete_evidence()
        self.assertEqual(preflight.assess(good)["state"], "verified_collecting")
        for section in ("writer", "catalog", "outage"):
            for name in good[section]:
                with self.subTest(section=section, name=name):
                    candidate = deepcopy(good)
                    candidate[section][name] = None
                    self.assertEqual(preflight.assess(candidate)["state"], "blocked")
        for name in ("observational_only_reviewed", "privacy_lifecycle_reviewed", "rollback_verified"):
            candidate = deepcopy(good)
            candidate["operations"][name] = False
            self.assertEqual(preflight.assess(candidate)["state"], "blocked")

    def test_authorization_is_separate_from_technical_readiness(self):
        candidate = synthetic_complete_evidence()
        candidate["operations"]["restart_approved"] = False
        candidate["operations"]["config_change_approved"] = None
        candidate["ingestion"]["enabled"] = False
        report = preflight.assess(candidate)
        self.assertEqual(report["state"], "awaiting_authorized_rollout")
        self.assertEqual(report["preparation_blockers"], [])
        self.assertEqual(len(report["rollout_actions"]), 3)
        self.assertFalse(report["automatic_balance_changes_performed"])

    def test_candidate_deployment_is_an_action_not_a_silent_success(self):
        candidate = synthetic_complete_evidence()
        candidate["runtime"]["binary_sha256"] = "c" * 64
        report = preflight.assess(candidate)
        self.assertEqual(report["state"], "awaiting_authorized_rollout")
        self.assertIn("approved_candidate_deployment_required_binary_sha256", report["rollout_actions"])

    def test_live_readback_is_required(self):
        candidate = synthetic_complete_evidence()
        for name in candidate["ingestion"]:
            if name != "enabled":
                candidate["ingestion"][name] = None
                self.assertEqual(preflight.assess(candidate)["state"], "awaiting_live_verification")
                candidate["ingestion"][name] = True

    def test_incompatible_binary_schema_pair_refuses(self):
        candidate = synthetic_complete_evidence()
        candidate["runtime"]["schema_head"] = "0043_synthetic_mismatch"
        self.assertEqual(preflight.assess(candidate)["state"], "blocked")
        candidate = synthetic_complete_evidence()
        candidate["runtime"]["backend"] = "flatfile"
        self.assertEqual(preflight.assess(candidate)["state"], "blocked")

    def test_no_truthy_or_repaired_identifiers(self):
        candidate = synthetic_complete_evidence()
        candidate["writer"]["credentials_present"] = 1
        with self.assertRaises(preflight.EvidenceError):
            preflight.assess(candidate)
        candidate = synthetic_complete_evidence()
        candidate["runtime"]["source_revision"] = " a" + "a" * 39
        with self.assertRaises(preflight.EvidenceError):
            preflight.assess(candidate)
        candidate = synthetic_complete_evidence()
        candidate["writer"]["password"] = "NEVER_ECHO_THIS_SECRET"
        with self.assertRaises(preflight.EvidenceError):
            preflight.assess(candidate)

    def test_real_cli_template_and_unknown_rehearsal(self):
        result = subprocess.run([sys.executable, str(SCRIPT), "--template"], text=True, capture_output=True, check=True)
        self.assertEqual(json.loads(result.stdout), preflight.template())
        with tempfile.TemporaryDirectory(prefix="telemetry-preflight-synthetic-") as directory:
            path = Path(directory) / "observations.json"
            path.write_text(result.stdout)
            result = subprocess.run([sys.executable, str(SCRIPT), str(path)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 2)
            self.assertEqual(json.loads(result.stdout)["state"], "blocked")

    def test_cli_refuses_duplicates_bounds_and_secret_fields_without_echo(self):
        with tempfile.TemporaryDirectory(prefix="telemetry-preflight-synthetic-") as directory:
            path = Path(directory) / "input.json"
            bad_secret = synthetic_complete_evidence()
            bad_secret["writer"]["password"] = "NEVER_ECHO_THIS_SECRET"
            for payload in (json.dumps(bad_secret), '{"schema_version":1,"schema_version":1}',
                            "x" * (preflight.MAX_INPUT_BYTES + 1), "{invalid",
                            "[" * 2048 + "]" * 2048):
                with self.subTest(kind=payload[:20]):
                    path.write_text(payload)
                    result = subprocess.run([sys.executable, str(SCRIPT), str(path)], text=True, capture_output=True)
                    self.assertEqual(result.returncode, 2)
                    self.assertNotIn("NEVER_ECHO_THIS_SECRET", result.stdout + result.stderr)
                    self.assertNotIn("Traceback", result.stderr)


if __name__ == "__main__":
    unittest.main()
