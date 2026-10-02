"""Dependency-free wiring contract for C14's manual synchronous SQL fixture."""
from __future__ import annotations

from pathlib import Path
import unittest

from _source_contract import function_bodies, strip_comments

ROOT = Path(__file__).resolve().parents[2]
ASYNC = ROOT / "tests/async"


class SyncItemStateSourceContract(unittest.TestCase):
    def test_real_synchronous_saver_serializes_dynamic_item_properties(self) -> None:
        sql_player = strip_comments((ROOT / "src/sql/sql_player.c").read_text(encoding="utf-8"))
        saves = function_bodies(
            sql_player,
            r"\bstatic\s+bool\s+sql_save_player_items\s*\(\s*P_char\s+ch\s*\)",
        )
        production_save = next(
            (body for body in saves if "sql_save_player_items_batch_all" in body), None
        )
        self.assertIsNotNone(production_save, "production synchronous save definition not found")
        assert production_save is not None
        self.assertIn("sql_begin_transaction()", production_save)
        self.assertIn("sql_save_player_items_batch_all(pid, ch, save_equipment, save_inventory)",
                      production_save)
        self.assertIn("sql_commit()", production_save)

        batch = function_bodies(
            sql_player,
            r"\bstatic\s+bool\s+sql_save_player_items_batch_all\s*\(",
        )
        self.assertEqual(len(batch), 1, "expected the complete production batch-save body")
        self.assertIn("player_item_properties_sql_column_suffix()", batch[0])
        self.assertIn("sql_player_item_properties_value_suffix(obj, &properties_suffix)", batch[0])
        self.assertIn("properties_suffix.c_str()", batch[0])
        self.assertIn("sql_save_item_affects(item_id, obj)", batch[0])

        properties = function_bodies(
            sql_player,
            r"\bstatic\s+bool\s+sql_player_item_properties_value_suffix\s*\(",
        )
        self.assertEqual(len(properties), 1)
        self.assertIn("obj->affects", properties[0])
        self.assertIn("{ affect->type, affect->data, affect->extra2 }", properties[0])
        self.assertIn("player_item_properties_sql_value_suffix(obj->extra2_flags",
                      properties[0])

    def test_manual_fixture_calls_production_tu_and_reads_real_sql_rows(self) -> None:
        harness = (ASYNC / "pa_sync_item_state_sql.cpp").read_text(encoding="utf-8")
        fixture = (ASYNC / "pa_sync_item_state_fixture.py").read_text(encoding="utf-8")
        runner_path = ASYNC / "run_pa_sync_item_state_sql.py"
        runner = runner_path.read_text(encoding="utf-8")

        self.assertEqual(runner_path.name, "run_pa_sync_item_state_sql.py")
        self.assertFalse(runner_path.name.startswith("test_"),
                         "the database journey must remain manual-only")
        self.assertIn('#include "sql/sql_player.c"', harness)
        self.assertNotIn("player_snapshot_repository_apply", harness)
        self.assertNotIn("player_load_repository_execute", harness)
        self.assertGreaterEqual(harness.count("sql_save_player_items(&character)"), 2)
        self.assertIn("player_item_properties_decode_sql_row", harness)
        self.assertIn("FROM player_items WHERE pid=", harness)
        self.assertIn("FROM player_item_affects ia", harness)
        self.assertIn("logical_parent_uid", harness)
        self.assertIn("same_persisted_state(first, second)", harness)
        self.assertIn("--sabotage-drop-item-properties", harness)
        self.assertIn("UPDATE player_items SET item_properties=NULL", harness)
        self.assertIn("ASSERTION FAILED: item_properties missing from SQL readback", harness)
        self.assertNotIn("INSERT INTO item_current_owner", harness)
        self.assertNotIn("INSERT INTO item_owner_revision", harness)

        self.assertIn("load_base_build(ROOT, required_sources=REQUIRED_SOURCES)", fixture)
        for required_source in (
            "src/sql/sql_player.c",
            "src/player/player_snapshot_codec.c",
            "src/sql/item_extra_descr_codec.c",
            "migrations/immutable/0035_player_item_dynamic_state.sql",
            "migrations/immutable/0035_player_item_dynamic_state.sh",
        ):
            self.assertIn(required_source, fixture)
        self.assertIn('MARIADB_IMAGE = "mariadb:10.11"', fixture)
        self.assertIn("c14_sync_item_state_test_", fixture)
        self.assertIn('"--pull=never"', fixture)
        self.assertIn("mariadb", fixture)
        self.assertIn("_redact", fixture)
        self.assertIn("_verify_removed", fixture)
        self.assertIn("[EXPECTED-RED] C14 sabotage detected item_properties loss", fixture)
        self.assertNotIn('read_text(".env")', fixture)
        self.assertIn("run_sync_item_state_fixture()", runner)

    def test_original_uids_and_created_target_ids_are_not_self_asserted(self) -> None:
        harness = (ASYNC / "pa_sync_item_state_sql.cpp").read_text(encoding="utf-8")
        main = harness.split("int main(", 1)[1]
        first_save = main.index("sql_save_player_items(&character)")
        for capture in ("const unsigned long long root_uid = root.obj_uid;",
                        "const unsigned long long child_uid = child.obj_uid;"):
            self.assertLess(main.index(capture), first_save)
        self.assertIn("first synchronous save changed an original object UID", main)
        fixture = (ASYNC / "pa_sync_item_state_fixture.py").read_text(encoding="utf-8")
        self.assertIn("for name, label in reversed(created):", fixture)
        self.assertIn('created.append((db_container, "database"))', fixture)
        self.assertIn('created.append((tools_container, "tools"))', fixture)
        self.assertIn("_verify_identity(db_container, db_name", fixture)
        self.assertIn("_verify_identity(tools_container, tools_name", fixture)
        self.assertIn('"--network=none"', fixture)

    def test_fixture_provides_empty_runtime_roots_and_fail_closed_panic(self) -> None:
        harness = (ASYNC / "pa_sync_item_state_sql.cpp").read_text(encoding="utf-8")
        self.assertIn("P_acct account_list = nullptr;", harness)
        self.assertIn("P_obj save_equip[MAX_WEAR] = {};", harness)
        panic = function_bodies(harness, r"\bint\s+panic_corruption_int\s*\(")
        self.assertEqual(len(panic), 1)
        self.assertIn("std::abort();", panic[0])


if __name__ == "__main__":
    unittest.main()
