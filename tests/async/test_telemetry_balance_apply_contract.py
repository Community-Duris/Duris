#!/usr/bin/env python3
"""Offline contracts for the #275 application/rollback decision boundary."""
from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts" / "telemetry" / "balance"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from application import ApplicationInputError, evaluate_command, main  # noqa: E402
from telemetry_balance_apply_fixtures import apply_command, copy_command  # noqa: E402


class BalanceApplicationContractTest(unittest.TestCase):
    def test_approved_apply_returns_safe_boundary_intent_and_audit(self):
        result = evaluate_command(apply_command())

        self.assertEqual(result["status"], "accepted_for_safe_boundary")
        self.assertFalse(result["applied"])
        self.assertEqual(result["next_config"]["revision"], 8)
        self.assertEqual(result["next_config"]["value_milli"], 250)
        self.assertEqual(result["fallback_value_milli"], 200)
        self.assertEqual(result["application_intent"]["requires_config_owner_commit"], True)
        self.assertEqual(result["audit_record"]["previous_value_milli"], 200)
        self.assertEqual(result["audit_record"]["applied_value_milli"], 250)
        self.assertEqual(result["audit_record"]["config_revision"], 8)
        self.assertEqual(result["constraints"]["writes_performed"], 0)
        self.assertFalse(result["constraints"]["gameplay_mutation"])
        self.assertFalse(result["constraints"]["production_activation"])

    def test_same_action_and_payload_is_idempotent(self):
        first = evaluate_command(apply_command())
        replay = copy_command()
        replay["history"] = [first["audit_record"]]

        result = evaluate_command(replay)

        self.assertEqual(result["status"], "idempotent_replay")
        self.assertFalse(result["applied"])
        self.assertEqual(result["audit_record"], first["audit_record"])
        self.assertEqual(result["constraints"]["writes_performed"], 0)

    def test_reused_action_id_with_changed_proposal_is_rejected(self):
        first = evaluate_command(apply_command())
        conflict = copy_command()
        conflict["proposal"]["target"]["proposed_value_milli"] = 300
        conflict["history"] = [first["audit_record"]]
        result = evaluate_command(conflict)

        self.assertEqual(result["status"], "rejected")
        self.assertIn("action_id_conflict", result["reason_codes"])
        self.assertEqual(result["fallback_value_milli"], 200)

    def test_concurrent_edit_rejects_stale_expected_revision(self):
        payload = copy_command()
        payload["current_config"]["revision"] = 8
        payload["current_config"]["value_milli"] = 300
        result = evaluate_command(payload)

        self.assertEqual(result["status"], "rejected")
        self.assertIn("stale_config", result["reason_codes"])
        self.assertEqual(result["next_config"]["value_milli"], 300)
        self.assertEqual(result["constraints"]["writes_performed"], 0)

    def test_expiry_kill_switch_and_disabled_boundary_reject(self):
        expired = copy_command()
        expired["now_utc"] = "2026-02-15T00:00:00Z"
        expired_result = evaluate_command(expired)
        self.assertEqual(expired_result["status"], "rejected")
        self.assertIn("proposal_expired", expired_result["reason_codes"])

        kill_switch = copy_command()
        kill_switch["controls"]["kill_switch"] = True
        kill_result = evaluate_command(kill_switch)
        self.assertEqual(kill_result["status"], "rejected")
        self.assertIn("kill_switch_active", kill_result["reason_codes"])

        disabled = copy_command()
        disabled["controls"]["application_enabled"] = False
        disabled_result = evaluate_command(disabled)
        self.assertEqual(disabled_result["status"], "rejected")
        self.assertIn("application_disabled", disabled_result["reason_codes"])

    def test_rollback_restores_the_known_previous_value(self):
        applied = evaluate_command(apply_command())
        rollback = copy_command()
        rollback["operation"] = "rollback"
        rollback["now_utc"] = "2026-02-03T00:00:00Z"
        rollback["approval"]["action_id"] = "action-rollback-1"
        rollback["approval"]["approved_at_utc"] = "2026-02-02T12:00:00Z"
        rollback["rollback_of_action_id"] = applied["audit_record"]["action_id"]
        rollback["current_config"] = applied["next_config"]
        rollback["expected_config"] = applied["next_config"]
        rollback["history"] = [applied["audit_record"]]

        result = evaluate_command(rollback)

        self.assertEqual(result["status"], "accepted_for_safe_boundary")
        self.assertEqual(result["next_config"]["revision"], 9)
        self.assertEqual(result["next_config"]["value_milli"], 200)
        self.assertEqual(result["audit_record"]["operation"], "rollback")
        self.assertEqual(result["audit_record"]["previous_value_milli"], 250)
        self.assertEqual(result["audit_record"]["applied_value_milli"], 200)

    def test_rollback_without_current_source_rejects(self):
        rollback = copy_command()
        rollback["operation"] = "rollback"
        rollback["rollback_of_action_id"] = "missing-action"
        result = evaluate_command(rollback)

        self.assertEqual(result["status"], "rejected")
        self.assertIn("rollback_source_missing", result["reason_codes"])

    def test_rollback_must_reference_the_same_proposal_and_policy(self):
        applied = evaluate_command(apply_command())
        rollback = copy_command()
        rollback["operation"] = "rollback"
        rollback["now_utc"] = "2026-02-03T00:00:00Z"
        rollback["approval"]["action_id"] = "action-rollback-mismatch"
        rollback["approval"]["approved_at_utc"] = "2026-02-02T12:00:00Z"
        rollback["rollback_of_action_id"] = applied["audit_record"]["action_id"]
        rollback["current_config"] = applied["next_config"]
        rollback["expected_config"] = applied["next_config"]
        rollback["history"] = [applied["audit_record"]]
        rollback["proposal"]["recommendation_id"] = "shadow-different-proposal"

        result = evaluate_command(rollback)

        self.assertEqual(result["status"], "rejected")
        self.assertIn("rollback_proposal_mismatch", result["reason_codes"])

    def test_raw_identity_is_rejected(self):
        payload = copy_command()
        payload["approval"]["account_id"] = 7

        with self.assertRaisesRegex(ApplicationInputError, "forbidden raw field"):
            evaluate_command(payload)

    def test_cli_output_is_deterministic(self):
        payload = apply_command()
        with tempfile.TemporaryDirectory() as directory:
            input_path = Path(directory) / "command.json"
            output_path = Path(directory) / "result.json"
            second_path = Path(directory) / "result-second.json"
            input_path.write_text(json.dumps(payload), encoding="utf-8")

            self.assertEqual(main(["--input", str(input_path), "--output", str(output_path)]), 0)
            self.assertEqual(main(["--input", str(input_path), "--output", str(second_path)]), 0)
            self.assertEqual(output_path.read_bytes(), second_path.read_bytes())
            self.assertEqual(
                json.loads(output_path.read_text(encoding="utf-8"))["status"],
                "accepted_for_safe_boundary",
            )


if __name__ == "__main__":
    unittest.main()
