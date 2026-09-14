#!/usr/bin/env python3
"""Offline contracts for collector SQL composition and its disposable journey."""

from __future__ import annotations

import stat
import unittest
from pathlib import Path

from _paths import SRC


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "tests/async/run_collector_repository_schema_mysql.sh"
HARNESS = ROOT / "tests/async/collector_repository_mysql_harness.cpp"


class CollectorRepositoryTest(unittest.TestCase):
    def test_repository_composes_every_authority_under_row_locks(self) -> None:
        source = (SRC / "collector_repository.c").read_text()
        for token in (
            "FOR UPDATE",
            "collector_catalog_state",
            "collector_listings",
            "collector_ledger",
            "item_current_owner",
            "item_owner_revision",
            "item_ownership_ledger",
            "currency_ledger",
            "player_items",
            "corpse_items",
            "saved_items",
        ):
            self.assertIn(token, source)
        self.assertIn("payload.listing >= catalog.next_listing", source)
        self.assertIn("mysql_affected_rows(connection) != 1", source)

    def test_restart_catalog_read_is_bounded_buffered_and_validated(self) -> None:
        source = (SRC / "collector_repository.c").read_text()
        read = source[source.index("bool collector_repository_read_bootstrap") :
                      source.index("bool collector_repository_read_catalog")]
        self.assertIn("mysql_store_result", read)
        self.assertNotIn("mysql_use_result", read)
        self.assertIn("LIMIT 262145", read)
        self.assertIn("projection_matches", read)
        self.assertIn("collector::valid_catalog", read)
        self.assertIn("item_current_owner", read)
        self.assertIn("item_owner_revision", read)
        self.assertIn("held_items", read)
        self.assertNotIn("FOR UPDATE", read)

    def test_listing_detail_read_is_bounded_and_nonlocking(self) -> None:
        source = (SRC / "collector_repository.c").read_text()
        loader = source[source.index("bool load_listing") : source.index("bool owner_less")]
        read = source[source.index("bool collector_repository_read_listing") :
                      source.index("bool collector_repository_execute")]
        self.assertIn("OCTET_LENGTH(item_blob)", loader)
        self.assertIn("LEFT(HEX(item_blob),262146)", loader)
        self.assertIn('for_update ? " FOR UPDATE" : ""', loader)
        self.assertIn("load_listing(connection, listing, false", read)
        self.assertNotIn("FOR UPDATE", read)

    def test_generic_journal_dispatches_and_publishes_collector_atomically(self) -> None:
        source = (SRC / "critical_command_repository.c").read_text()
        contract = (SRC / "collector_command.h").read_text()
        branch = source[source.index("if (collector_command)") :]
        self.assertIn("collector_repository_execute", branch)
        self.assertIn("collector_command_encode_result", branch)
        self.assertLess(branch.index("insert_outbox"), branch.index("finish_inbox"))
        self.assertLess(branch.index("finish_inbox"), branch.index('execute(connection, "COMMIT")'))
        self.assertIn("COLLECTOR_OUTBOX_DESTINATION = 11", contract)
        self.assertIn("COLLECTOR_OUTBOX_DESTINATION", source)

    def test_purchase_reason_and_exact_room_uid_are_registered(self) -> None:
        currency = (SRC / "currency_command.h").read_text()
        validation = (SRC / "currency_command.c").read_text()
        player_save = (SRC / "sql_player.c").read_text()
        self.assertIn("collector_purchase", currency)
        self.assertIn("currency_reason_type::collector_purchase", validation)
        self.assertIn("obj_uid", player_save)

    def test_disposable_journey_is_wired_into_database_tests(self) -> None:
        runner = RUNNER.read_text()
        makefile = (ROOT / "Makefile").read_text()
        self.assertTrue(RUNNER.stat().st_mode & stat.S_IXUSR)
        self.assertIn("never reads .env", runner)
        self.assertIn("COLLECTOR_REPOSITORY_DB_IMAGE", runner)
        self.assertIn("bootstrap_multithread_safe.sql", runner)
        self.assertIn("collector_repository_mysql_harness.cpp", runner)
        self.assertIn("-Wpedantic -Werror", runner)
        self.assertIn("run_collector_repository_schema_mysql.sh", makefile)

    def test_journey_covers_replay_and_cross_authority_effects(self) -> None:
        harness = HARNESS.read_text()
        for token in (
            "critical_apply_outcome::already_applied",
            "critical_apply_outcome::terminal_failure",
            "collector_action::collect",
            "collector_action::purchase",
            "collector_action::expire",
            "collector_action::pause",
            "collector_action::resume",
            "collector::reason::claimed",
            "collector::reason::quarantined",
            "item_ownership_ledger",
            "currency_ledger",
            "player_item_affects",
            "player_item_extra_descr",
        ):
            self.assertIn(token, harness)


if __name__ == "__main__":
    unittest.main()
