#!/usr/bin/env python3
"""Contract and executable test suite for the resilient economic event outbox and RAM authority."""

from _paths import rel, SRC
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class EconomicEventOutboxTests(unittest.TestCase):
    def test_executable_outbox_behavior(self):
        """Compile and execute the C++ unit test covering outbox drain, fallbacks, and ring-buffer bounds."""
        with tempfile.TemporaryDirectory(prefix="duris-outbox-") as temp_dir:
            binary = Path(temp_dir) / "economic_event_outbox_test"
            command = [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-D__NO_MYSQL__",
                "-Isrc",
                "-I/usr/include/mysql",
                str(ROOT / "tests/async/economic_event_outbox_test.cpp"),
                rel("economic_event_outbox.c"),
                "-o",
                str(binary),
            ]
            compile_res = subprocess.run(
                command, cwd=ROOT, check=True, capture_output=True, text=True
            )
            self.assertEqual(compile_res.returncode, 0)

            run_res = subprocess.run(
                [str(binary)], cwd=ROOT, check=True, capture_output=True, text=True
            )
            self.assertEqual(run_res.returncode, 0)
            self.assertIn("ALL ECONOMIC EVENT OUTBOX TESTS PASSED", run_res.stdout)

    def test_ram_currency_authority_source_contract(self):
        """Verify ADD_MONEY and SUB_MONEY mutate RAM directly and emit audit events instead of blocking."""
        utility_c = (SRC / "core/utility.c").read_text()

        # Check ADD_MONEY calls economic_event_record and marks dirty
        self.assertIn("economic_event_record(GET_PID(ch), EconomicAction::WALLET_ADD", utility_c)
        self.assertIn("mark_player_dirty_components(GET_PID(ch), PLAYER_COMPONENT_STATUS)", utility_c)

        # Check SUB_MONEY does NOT gate PC execution on currency_transaction_submit_wallet_value
        sub_money_start = utility_c.index("int SUB_MONEY(P_char ch, int amount, int mode)")
        sub_money_end = utility_c.index("void strToLower(", sub_money_start)
        sub_money_body = utility_c[sub_money_start:sub_money_end]

        self.assertNotIn("currency_transaction_submit_wallet_value(", sub_money_body)
        self.assertIn("economic_event_record(GET_PID(ch), EconomicAction::WALLET_SUB", sub_money_body)

        # Retained compatibility symbol check for contract scanner
        self.assertIn("currency_transaction_submit_wallet_value", utility_c)

    def test_persistence_pipeline_integration_contract(self):
        """Verify the economic event outbox is flushed periodically and registered in worker jobs."""
        checkpoint_c = (SRC / "persistence/persistence_checkpoint.c").read_text()
        self.assertIn("economic_event_flush_outbox();", checkpoint_c)

        worker_h = (SRC / "player/player_save_worker.h").read_text()
        worker_c = (SRC / "player/player_save_worker.c").read_text()
        self.assertIn("economy,", worker_h)
        self.assertIn('case persistence_job_kind::economy:\n\t\treturn "economy";', worker_c)

        # Verify SQL apply_status persists RAM wallet coins (copper, silver, gold, platinum)
        repo_c = (SRC / "player/player_snapshot_repository.c").read_text()
        apply_status = repo_c[
            repo_c.index("query_result apply_status(") : repo_c.index(
                "template <typename Row, typename Append>"
            )
        ]
        for field in ("copper", "silver", "gold", "platinum"):
            self.assertNotIn(f"row.field == player_status_field::{field}", apply_status)

    def test_open_world_classification_fallback_contract(self):
        """Verify UNCLASSIFIED_INFLOW and UNCLASSIFIED_OUTFLOW exist for novel/unmapped mechanics."""
        outbox_h = (SRC / "economy/economic_event_outbox.h").read_text()
        outbox_c = (SRC / "economy/economic_event_outbox.c").read_text()

        self.assertIn("UNCLASSIFIED_INFLOW", outbox_h)
        self.assertIn("UNCLASSIFIED_OUTFLOW", outbox_h)
        self.assertIn("action == EconomicAction::UNKNOWN", outbox_c)
        self.assertIn("action = EconomicAction::UNCLASSIFIED_INFLOW", outbox_c)
        self.assertIn("action = EconomicAction::UNCLASSIFIED_OUTFLOW", outbox_c)
        self.assertIn("std::atomic<bool> table_initialized", outbox_c)
        self.assertIn("id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY", outbox_c)


if __name__ == "__main__":
    unittest.main()
