#!/usr/bin/env python3
"""Focused schema, state-machine, and fail-closed archive execution contracts."""

from __future__ import annotations

import copy
import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "lifecycle_archive.py"
MIGRATION = ROOT / "migrations" / "lifecycle_archive_execution.sql"
BOOTSTRAP = ROOT / "migrations" / "bootstrap_multithread_safe.sql"
MANIFEST = ROOT / "migrations" / "data_lifecycle_manifest.json"
SCHEDULER = (ROOT / "src" / "maintenance_scheduler.c").read_text()
SCHEDULER_HEADER = (ROOT / "src" / "maintenance_scheduler.h").read_text()
MIGRATION_RUNNER = (ROOT / "migrations" / "run_migration.sh").read_text()
SCHEMA_VERIFIER = (ROOT / "migrations" / "verify_lifecycle_archive_schema.sh").read_text()
SPEC = importlib.util.spec_from_file_location("lifecycle_archive", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class LifecycleArchiveExecutionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.snapshot = MODULE.load_policy()

    def approved_snapshot(self):
        manifest = copy.deepcopy(self.snapshot.manifest)
        entries = {key: copy.deepcopy(value) for key, value in self.snapshot.entries.items()}
        manifest["controller_approval"] = {
            "status": "approved",
            "reference": "TEST-GLOBAL-APPROVAL",
            "destructive_rules_enabled": True,
        }
        entry = entries["database:accounts"]
        entry["terminal_action"] = "archive"
        entry["controller_decision"] = {
            "status": "approved",
            "reference": "TEST-ENTRY-APPROVAL",
        }
        return MODULE.PolicySnapshot(manifest, entries, "a" * 64)

    def executable_plan(self):
        snapshot = self.approved_snapshot()
        plan = MODULE.build_plan(
            snapshot, "database:accounts", "archive", "2025-01-01T00:00:00Z",
            "0", "999", MODULE.Budget(rows=4, bytes=128, time_usec=1000),
            dry_run=False,
        )
        authorization = MODULE.authorize_execution(
            snapshot, plan, "test", "127.0.0.1", "lifecycle-admin"
        )
        return snapshot, plan, authorization

    def rows(self):
        return [
            MODULE.ArchiveRow(b"001", b'{"value":1}'),
            MODULE.ArchiveRow(b"002", b'{"value":2}'),
        ]

    def test_schema_is_additive_bounded_and_reconciled_to_manifest(self) -> None:
        migration = MIGRATION.read_text()
        bootstrap = BOOTSTRAP.read_text()
        manifest = json.loads(MANIFEST.read_text())
        entries = {entry["id"]: entry for entry in manifest["entries"]}
        tables = (
            "lifecycle_archive_jobs",
            "lifecycle_archive_batches",
            "lifecycle_archive_rows",
            "lifecycle_archive_evidence",
        )
        for table in tables:
            self.assertIn(f"CREATE TABLE IF NOT EXISTS {table}", migration)
            self.assertIn(f"`{table}`", bootstrap)
            entry = entries[f"database:{table}"]
            self.assertTrue(entry["protected_record"])
            self.assertEqual(entry["terminal_action"], "retain")
        self.assertEqual(migration.count("ENGINE=InnoDB"), 4)
        self.assertIn("row_budget BETWEEN 1 AND 256", migration)
        self.assertIn("byte_budget BETWEEN 1 AND 1048576", migration)
        self.assertIn("time_budget_usec BETWEEN 1 AND 500000", migration)
        self.assertIn("UNIQUE KEY uq_lifecycle_archive_job_key", migration)
        self.assertIn("UNIQUE KEY uq_lifecycle_archive_batch_key", migration)
        self.assertNotIn("DELETE FROM", migration.upper())
        self.assertNotRegex(migration, r"(?im)^\s*UPDATE\s")
        self.assertIn(
            'run_sql_file "apply lifecycle archive execution schema"', MIGRATION_RUNNER
        )
        self.assertIn(
            'run_check "verify lifecycle archive execution schema"', MIGRATION_RUNNER
        )
        self.assertIn("information_schema.referential_constraints", SCHEMA_VERIFIER)

    def test_scheduler_slot_is_explicitly_disabled_by_pending_policy(self) -> None:
        self.assertIn("lifecycle_archive", SCHEDULER_HEADER)
        self.assertRegex(
            SCHEDULER,
            r"maintenance_job_id::lifecycle_archive,\s*2400,\s*7,\s*64,\s*25000,\s*false",
        )
        self.assertIn("durable_scheduler_state_v2", SCHEDULER)
        self.assertIn('memcmp(loaded_v2.magic, "DMSMNT2"', SCHEDULER)
        self.assertIn('memcpy(durable_state.magic, "DMSMNT3"', SCHEDULER)

    def test_canonical_policy_is_scheduler_blocked_and_dry_run_only(self) -> None:
        result = subprocess.run(
            ["python3", str(SCRIPT), "inspect"], cwd=ROOT,
            capture_output=True, text=True, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report["stores"], 177)
        self.assertEqual(report["approved_destructive_rules"], 0)
        self.assertFalse(report["destructive_rules_enabled"])
        self.assertEqual(report["scheduler_state"], "blocked_by_policy")

        plan = MODULE.build_plan(
            self.snapshot, "database:accounts", "archive", "2025-01-01", "0",
            "999", MODULE.Budget(),
        )
        self.assertEqual(plan.status, MODULE.BatchStatus.BLOCKED)
        self.assertIn("entry_approval_missing", plan.reason_codes)
        self.assertIn("global_approval_disabled", plan.reason_codes)
        self.assertNotIn("approval_reference", plan.redacted_report())
        with self.assertRaisesRegex(MODULE.ArchiveContractError, "cannot be authorized"):
            MODULE.authorize_execution(
                self.snapshot, plan, "test", "127.0.0.1", "lifecycle-admin"
            )

    def test_stable_identity_dependency_order_and_fixed_budgets(self) -> None:
        snapshot, plan, authorization = self.executable_plan()
        repeated = MODULE.build_plan(
            snapshot, "database:accounts", "archive", "2025-01-01T00:00:00Z",
            "0", "999", MODULE.Budget(rows=4, bytes=128, time_usec=1000),
            dry_run=False,
        )
        self.assertEqual(plan.job_id, repeated.job_id)
        later_cutoff = MODULE.build_plan(
            snapshot, "database:accounts", "archive", "2026-01-01T00:00:00Z",
            "0", "999", MODULE.Budget(rows=4, bytes=128, time_usec=1000),
            dry_run=False,
        )
        self.assertNotEqual(plan.job_id, later_cutoff.job_id)
        order = MODULE.dependency_order(
            self.snapshot,
            ["database:lifecycle_archive_rows", "database:lifecycle_archive_batches"],
        )
        self.assertEqual(order, [
            "database:lifecycle_archive_batches", "database:lifecycle_archive_rows",
        ])
        self.assertEqual(
            MODULE.dependency_order(
                self.snapshot,
                ["database:lifecycle_archive_rows", "database:lifecycle_archive_batches"],
                finalization=True,
            ),
            ["database:lifecycle_archive_rows", "database:lifecycle_archive_batches"],
        )
        machine = MODULE.ArchiveBatchMachine(plan, 0, b"000", b"999", authorization)
        with self.assertRaisesRegex(MODULE.ArchiveContractError, "row budget"):
            too_many = [MODULE.ArchiveRow(f"{number:03}".encode(), b"x")
                        for number in range(1, 6)]
            machine.copy(too_many, 10)
        with self.assertRaisesRegex(MODULE.ArchiveContractError, "wall-time"):
            machine.copy(self.rows(), 1001)
        with self.assertRaisesRegex(MODULE.ArchiveContractError, "byte budget"):
            oversized = [MODULE.ArchiveRow(b"001", b"x" * 129)]
            machine.copy(oversized, 10)

    def test_copy_retry_verify_finalize_and_restore_are_exact(self) -> None:
        snapshot, plan, authorization = self.executable_plan()
        machine = MODULE.ArchiveBatchMachine(plan, 4, b"000", b"999", authorization)
        original_batch_id = machine.batch_id
        rows = self.rows()
        machine.copy(rows, 100)
        first_checksum = machine.archive_checksum
        machine.copy(rows, 100)
        self.assertEqual(machine.archive_checksum, first_checksum)
        self.assertEqual(len(machine.archived), 2)
        machine.verify(rows, reconciliation_before=True)
        finalize = machine.finalize(snapshot.checksum, "TEST-ENTRY-APPROVAL")
        self.assertEqual(machine.status, MODULE.BatchStatus.FINALIZING)
        machine.complete_finalize(finalize, affected_count=2, remaining_count=0,
                                  reconciliation_after=True)
        self.assertEqual(finalize.batch_id, original_batch_id)
        self.assertEqual(finalize.source_count, 2)
        self.assertEqual(machine.restore(), rows)
        report = machine.redacted_report()
        self.assertNotIn("source_key", report)
        self.assertNotIn("payload", report)

    def test_crash_retry_conflict_corruption_and_policy_change_fail_closed(self) -> None:
        snapshot, plan, authorization = self.executable_plan()
        rows = self.rows()
        first = MODULE.ArchiveBatchMachine(plan, 1, b"000", b"999", authorization)
        first.copy(rows, 100)
        recovered = MODULE.ArchiveBatchMachine(plan, 1, b"000", b"999", authorization)
        recovered.archived = dict(first.archived)
        recovered.status = MODULE.BatchStatus.COPYING
        recovered.copy(rows, 100)
        self.assertEqual(recovered.batch_id, first.batch_id)

        conflict = list(rows)
        conflict[1] = MODULE.ArchiveRow(b"002", b"changed")
        with self.assertRaisesRegex(MODULE.ArchiveContractError, "conflicts"):
            recovered.copy(conflict, 100)

        recovered.status = MODULE.BatchStatus.COPIED
        recovered.archived[b"002"] = MODULE.ArchiveRow(b"002", b"corrupt")
        with self.assertRaisesRegex(MODULE.ArchiveContractError, "checksum mismatch"):
            recovered.verify(rows, reconciliation_before=True)

        policy_change = MODULE.ArchiveBatchMachine(plan, 2, b"000", b"999", authorization)
        policy_change.copy(rows, 100)
        policy_change.verify(rows, reconciliation_before=True)
        with self.assertRaisesRegex(MODULE.ArchiveContractError, "identity changed"):
            policy_change.finalize("b" * 64, "TEST-ENTRY-APPROVAL")

        finalize = policy_change.finalize(snapshot.checksum, "TEST-ENTRY-APPROVAL")
        repeated = policy_change.finalize(snapshot.checksum, "TEST-ENTRY-APPROVAL")
        self.assertEqual(finalize, repeated)
        with self.assertRaisesRegex(MODULE.ArchiveContractError, "count verification"):
            policy_change.complete_finalize(finalize, 1, 1, True)

    def test_reconciliation_authorization_cursor_and_pause_gates(self) -> None:
        snapshot, plan, authorization = self.executable_plan()
        with self.assertRaisesRegex(MODULE.ArchiveContractError, "lacks exact authorization"):
            MODULE.ArchiveBatchMachine(plan, 0, b"000", b"999")
        for environment, host, role, message in (
            ("production", "127.0.0.1", "lifecycle-admin", "non-production"),
            ("test", "database.internal", "lifecycle-admin", "loopback"),
            ("test", "127.0.0.1", "operator", "lifecycle-admin"),
        ):
            with self.assertRaisesRegex(MODULE.ArchiveContractError, message):
                MODULE.authorize_execution(snapshot, plan, environment, host, role)

        machine = MODULE.ArchiveBatchMachine(plan, 0, b"000", b"999", authorization)
        machine.pause()
        self.assertEqual(machine.status, MODULE.BatchStatus.PAUSED)
        machine.resume()
        self.assertEqual(machine.status, MODULE.BatchStatus.PLANNED)
        with self.assertRaisesRegex(MODULE.ArchiveContractError, "cursor window"):
            machine.copy([MODULE.ArchiveRow(b"000", b"x")], 10)

        machine.copy(self.rows(), 100)
        with self.assertRaisesRegex(MODULE.ArchiveContractError, "reconciliation"):
            machine.verify(self.rows(), reconciliation_before=False)

    def test_dry_run_state_is_atomic_redacted_and_stale_policy_fails(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            state_path = Path(temporary) / "state.json"
            result = subprocess.run(
                [
                    "python3", str(SCRIPT), "plan", "--store", "database:accounts",
                    "--action", "archive", "--cutoff", "2025-01-01",
                    "--upper-bound", "999", "--state-file", str(state_path),
                ],
                cwd=ROOT, capture_output=True, text=True, check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(state_path.stat().st_mode & 0o777, 0o600)
            state = MODULE.read_state(state_path)
            self.assertTrue(state["dry_run"])
            self.assertEqual(state["status"], "blocked")
            self.assertNotIn("approval_reference", state)

            link = Path(temporary) / "state-link.json"
            link.symlink_to(state_path)
            with self.assertRaisesRegex(
                MODULE.lifecycle_policy.ValidationError, "regular non-symlink"
            ):
                MODULE.read_state(link)

            malformed = dict(state)
            malformed["row_budget"] = "64"
            malformed_path = Path(temporary) / "malformed.json"
            malformed_path.write_text(json.dumps(malformed))
            with self.assertRaisesRegex(MODULE.ArchiveContractError, "row budget"):
                MODULE.read_state(malformed_path)

            state["manifest_checksum"] = "0" * 64
            state["job_id"], state["job_key"] = MODULE.stable_identity(
                state["policy_id"], str(state["policy_schema_version"]),
                state["manifest_checksum"], state["store_id"], state["action"],
                state["cutoff"], state["cursor"], state["upper_bound"],
            )
            MODULE.write_state(state_path, state)
            stale = subprocess.run(
                ["python3", str(SCRIPT), "report", "--state-file", str(state_path)],
                cwd=ROOT, capture_output=True, text=True, check=False,
            )
            self.assertEqual(stale.returncode, 2)
            self.assertIn("checksum is stale", stale.stderr)


if __name__ == "__main__":
    unittest.main()
