#!/usr/bin/env python3
"""Migration, lifecycle, report, and source-boundary contracts for #270."""

from __future__ import annotations

import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from scripts.telemetry.reward_projection import (  # noqa: E402
    SOURCE_ADAPTERS,
    build_reward_report_query,
    build_reward_state_query,
)
from scripts.telemetry.reward_projection_definitions import (  # noqa: E402
    MAX_REPORT_WINDOW_USEC,
    REWARD_REPORT_DEFINITION,
    SourceKind,
)


class RewardProjectionSchemaTest(unittest.TestCase):
    def setUp(self) -> None:
        self.migration = (ROOT / "migrations/immutable/0023_telemetry_reward_projection.sql").read_text()
        self.verifier = (ROOT / "migrations/immutable/0023_telemetry_reward_projection.sh").read_text()
        self.bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        self.lifecycle = json.loads((ROOT / "migrations/data_lifecycle_manifest.json").read_text())

    def test_migration_is_additive_rerunnable_and_has_two_durable_tables(self) -> None:
        self.assertEqual(self.migration.upper().count("CREATE TABLE IF NOT EXISTS"), 2)
        self.assertIn("telemetry_reward_projection", self.migration)
        self.assertIn("telemetry_reward_projection_state", self.migration)
        self.assertNotIn("ALTER TABLE player_data", self.migration)
        self.assertNotIn("FOR UPDATE", self.migration.upper())
        self.assertIn("PRIMARY KEY (source_kind, operation_id, entry_index, participant_pid)", self.migration)
        self.assertIn("source_payload_digest BINARY(32) NOT NULL", self.migration)
        self.assertIn("acknowledged_through TIMESTAMP(6)", self.migration)
        self.assertIn("source_kind BETWEEN 1 AND 10", self.migration)
        for index_name in (
            "idx_currency_created_operation", "idx_epic_created_operation",
            "idx_combat_frag_created_operation", "idx_combat_outcome_created_operation",
            "idx_boon_reward_created_operation", "idx_zone_touch_outcome_created_operation",
        ):
            self.assertIn(index_name, self.migration)
            self.assertIn(index_name, self.bootstrap)
            self.assertIn(index_name, self.verifier)
        self.assertIn("projection_columns '24'", self.verifier)
        self.assertIn("state_columns '21'", self.verifier)
        self.assertIn("CREATE TABLE IF NOT EXISTS `telemetry_reward_projection`", self.bootstrap)
        self.assertIn("CREATE TABLE IF NOT EXISTS `telemetry_reward_projection_state`", self.bootstrap)

    def test_lifecycle_inventory_covers_projection_and_state_as_protected(self) -> None:
        entries = {entry["id"]: entry for entry in self.lifecycle["entries"]}
        projection = entries["database:telemetry_reward_projection"]
        state = entries["database:telemetry_reward_projection_state"]
        self.assertTrue(projection["protected_record"])
        self.assertTrue(state["protected_record"])
        self.assertEqual(projection["terminal_action"], "retain")
        self.assertEqual(state["terminal_action"], "retain")
        self.assertIn("database:currency_ledger", projection["dependencies"])
        self.assertIn("database:epic_ledger", projection["dependencies"])
        self.assertEqual(state["dependencies"], ["database:telemetry_reward_projection"])

    def test_report_definition_and_queries_are_read_only_and_bounded(self) -> None:
        query, parameters = build_reward_report_query(
            start_usec=1_000_000,
            end_usec=1_000_000 + MAX_REPORT_WINDOW_USEC,
            max_rows=32,
        )
        normalized = " ".join(query.upper().split())
        self.assertEqual(REWARD_REPORT_DEFINITION["name"], "committed_reward_projection")
        self.assertIn("GROUP BY SOURCE_KIND,REWARD_KIND,STATUS", normalized)
        self.assertIn("LIMIT %S", normalized)
        self.assertNotIn("INSERT", normalized)
        self.assertNotIn("UPDATE", normalized)
        self.assertNotIn("DELETE", normalized)
        self.assertNotIn("FOR UPDATE", normalized)
        self.assertEqual(parameters[-1], 32)
        state_query = build_reward_state_query().upper()
        self.assertIn("ORDER BY SOURCE_KIND LIMIT 10", " ".join(state_query.split()))
        self.assertNotIn("SELECT *", state_query)

    def test_context_source_kinds_are_unique_and_legacy_quest_is_explicitly_not_supported(self) -> None:
        supported_kinds = [adapter.source_kind for adapter in SOURCE_ADAPTERS if adapter.supported]
        self.assertEqual(len(set(supported_kinds)), 9)
        self.assertIn(SourceKind.ZONE_OUTCOME_PARTICIPANT, supported_kinds)
        self.assertIn(SourceKind.PVP_OUTCOME_PARTICIPANT, supported_kinds)
        self.assertIn(SourceKind.BOON_OUTCOME_ENTRY, supported_kinds)
        self.assertIn(SourceKind.COMBAT_FRAG_LEDGER, supported_kinds)
        self.assertTrue(all(adapter.context_only for adapter in SOURCE_ADAPTERS
                            if adapter.source_kind == SourceKind.QUEST_OUTCOME))
        self.assertTrue(all(not adapter.supported for adapter in SOURCE_ADAPTERS
                            if adapter.source_kind == SourceKind.QUEST_OUTCOME))


if __name__ == "__main__":
    unittest.main()
