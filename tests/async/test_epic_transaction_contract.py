#!/usr/bin/env python3
"""Source and schema contracts for the transactional epic balance boundary."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


class EpicTransactionContractTests(unittest.TestCase):
    def test_schema_and_operator_tools_are_wired(self):
        migration = (ROOT / "migrations/epic_ledger_balance.sql").read_text()
        bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        runner = (ROOT / "migrations/run_migration.sh").read_text()
        for token in (
            "epic_balance_baseline",
            "epic_ledger",
            "epic_revision",
            "uq_epic_ledger_pid_revision",
            "epic_ledger_operation_fk",
        ):
            self.assertIn(token, migration)
            self.assertIn(token, bootstrap)
        self.assertIn("epic_ledger_balance.sql", runner)
        self.assertIn("verify_epic_ledger_schema.sh", runner)
        for script in (
            "baseline_epic_balances.sh",
            "reconcile_epic_balances.sh",
            "verify_epic_ledger_schema.sh",
        ):
            self.assertTrue((ROOT / "migrations" / script).stat().st_mode & 0o111)

    def test_command_is_typed_fixed_and_funds_guarded(self):
        header = (SRC / "epic_command.h").read_text()
        implementation = (SRC / "epic_command.c").read_text()
        self.assertIn("EPIC_COMMAND_PAYLOAD_BYTES = 32", header)
        self.assertIn("EPIC_RESULT_PAYLOAD_BYTES = 24", header)
        self.assertIn("EPIC_COMMAND_REQUIRE_FUNDS", header)
        self.assertIn("numeric_limits<int64_t>::min()", implementation)
        self.assertIn("command.expected_revisions.size() == 1", implementation)

    def test_repository_commits_balance_ledger_result_and_outbox_together(self):
        repository = (SRC / "critical_command_repository.c").read_text()
        start = repository.index("bool execute_epic_state")
        apply = repository.index("critical_apply_result critical_command_repository_apply")
        epic_state = repository[start:apply]
        for token in (
            "FOR UPDATE",
            "EPIC_COMMAND_REQUIRE_FUNDS",
            "UPDATE player_data SET epics=?,epic_revision=?",
            "INSERT INTO epic_ledger",
        ):
            self.assertIn(token, epic_state)
        epic_apply = repository[apply:]
        epic_branch = epic_apply[epic_apply.index("if (epic_command)"):epic_apply.index("int64_t value")]
        commit = epic_branch.index('execute(connection, "COMMIT")')
        self.assertLess(epic_branch.index("insert_outbox"), commit)
        self.assertLess(epic_branch.index("finish_inbox"), commit)

    def test_checkpoint_and_flat_file_replay_cannot_overwrite_epics(self):
        capture = (SRC / "player_snapshot_capture.c").read_text()
        replay = (SRC / "player_snapshot_repository.c").read_text()
        sql_player = (SRC / "sql_player.c").read_text()
        flat_file = (SRC / "files.c").read_text()
        self.assertNotRegex(capture, r"snapshot\s*->\s*epics")
        self.assertIn("row.field == player_status_field::epics", replay)
        self.assertIn("epics=epics", sql_player)
        self.assertIn("INSERT INTO epic_balance_baseline", sql_player)
        self.assertIn("Legacy flat-file epic balance is parsed", flat_file)

    def test_no_gameplay_file_directly_mutates_epic_balance(self):
        assignment = re.compile(
            r"(?:only\.pc->epics|GET_EPIC_POINTS\([^)]*\))\s*(?:[+\-]=|=(?!=))"
        )
        allowed = {
            "critical_command_repository.c",
            "epic_transaction.c",
            "nanny.c",       # new-character initialization only
            "sql_player.c",  # authoritative hydration only
            "player_load_materialize.c",  # authoritative worker snapshot hydration
        }
        violations = []
        for path in SRC.rglob("*.c"):
            if path.name in allowed:
                continue
            for line_number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
                if line.lstrip().startswith("//"):
                    continue
                if assignment.search(line):
                    violations.append(f"{path.relative_to(ROOT)}:{line_number}:{line.strip()}")
        self.assertEqual([], violations)

    def test_awards_bonus_and_game_thread_completion_use_ack_boundary(self):
        epic = (SRC / "epic.c").read_text()
        bonus = (SRC / "epic_bonus.c").read_text()
        transaction = (SRC / "epic_transaction.c").read_text()
        comm = (SRC / "comm.c").read_text()
        self.assertIn("epic_transaction_submit_identified(", epic)
        self.assertIn("epic_award_committed", epic)
        self.assertIn("FROM epic_ledger", bonus)
        self.assertIn("find_player_by_pid", transaction)
        self.assertIn("epic_transaction_handle_completions", comm)
        self.assertIn("epic_transaction_player_ready", (SRC / "nanny.c").read_text())


if __name__ == "__main__":
    unittest.main()
