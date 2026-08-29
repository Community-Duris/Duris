#!/usr/bin/env python3
"""Codec, hot-path, schema, and transactional contracts for boon/zone batching."""

from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

HARNESS = r'''
#include "boon_reward_command.h"
#include "boon_shop_command.h"
#include "zone_touch_command.h"
#include <cassert>

int main()
{
    critical_operation_id operation = {};
    assert(critical_operation_id_generate(&operation));
    boon_reward_payload boon = {42, 1, 55, 100, 3, 1.5, 700, 9, 3};
    critical_command boon_command = {};
    assert(boon_reward_command_build(&boon_command, operation, boon));
    boon_reward_payload decoded_boon = {};
    assert(boon_reward_command_decode_payload(boon_command, &decoded_boon));
    assert(decoded_boon.pid == 42 && decoded_boon.data == 1.5);
    boon_reward_result boon_result = {};
    boon_result.pid = 42;
    boon_result.entry_count = 1;
    boon_result.entries[0] = {5, 10, 3, 3, 0, 2, 700, 4, 0, -1};
    std::array<uint8_t, BOON_REWARD_RESULT_BYTES> boon_bytes = {};
    assert(boon_reward_command_encode_result(boon_result, &boon_bytes));
    boon_reward_result decoded_result = {};
    assert(boon_reward_command_decode_result(boon_bytes.data(), boon_bytes.size(),
                                             &decoded_result));
    assert(decoded_result.entries[0].boon_id == 5);

    boon_shop_payload shop = {42, 3};
    critical_command shop_command = {};
    assert(boon_shop_command_build(&shop_command, operation, shop));
    boon_shop_payload decoded_shop = {};
    assert(boon_shop_command_decode_payload(shop_command, &decoded_shop));
    assert(decoded_shop.pid == 42 && decoded_shop.stat_index == 3);
    boon_shop_result shop_result = {42, 3, 61, 4, 9};
    std::array<uint8_t, BOON_SHOP_RESULT_BYTES> shop_bytes = {};
    assert(boon_shop_command_encode_result(shop_result, &shop_bytes));
    boon_shop_result decoded_shop_result = {};
    assert(boon_shop_command_decode_result(shop_bytes.data(), shop_bytes.size(),
                                           &decoded_shop_result));
    assert(decoded_shop_result.stat_value == 61 &&
           decoded_shop_result.remaining_stat_points == 4);

    zone_touch_payload zone = {};
    zone.zone_number = 900;
    zone.toucher_pid = 42;
    zone.boot_time = 1000;
    zone.touched_at = 1100;
    zone.group_size = 3;
    zone.participant_pids[0] = 42;
    zone.participant_pids[1] = 43;
    zone.participant_pids[2] = 44;
    zone.epic_value = 12;
    zone.alignment_delta = 1;
    zone.reset_requested = 1;
    critical_command zone_command = {};
    assert(zone_touch_command_build(&zone_command, operation, zone));
    assert(zone_command.keys.size() == 4);
    zone_touch_payload decoded_zone = {};
    assert(zone_touch_command_decode_payload(zone_command, &decoded_zone));
    assert(decoded_zone.participant_pids[2] == 44);
    std::array<uint8_t, ZONE_TOUCH_RESULT_BYTES> zone_bytes = {};
    assert(zone_touch_command_encode_result(zone, &zone_bytes));
    assert(zone_touch_command_decode_result(zone_bytes.data(), zone_bytes.size(), &decoded_zone));
    assert(decoded_zone.group_size == 3);
    zone.participant_pids[2] = 42;
    assert(!zone_touch_command_build(&zone_command, operation, zone));
}
'''


class BoonRewardZoneCutoverTests(unittest.TestCase):
    def test_bounded_canonical_codecs(self):
        with tempfile.TemporaryDirectory() as directory:
            harness = Path(directory) / "boon_zone_codec.cpp"
            binary = Path(directory) / "boon_zone_codec"
            harness.write_text(HARNESS)
            subprocess.run([
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", f"-I{SRC}",
                str(harness), str(SRC / "critical_command.c"),
                str(SRC / "boon_reward_command.c"), str(SRC / "boon_shop_command.c"),
                str(SRC / "zone_touch_command.c"),
                "-lcrypto", "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True)

    def test_boon_callback_has_no_synchronous_io(self):
        source = (SRC / "boon.c").read_text()
        start = source.index("void check_boon_completion(P_char")
        end = source.index("void boon_publish_transaction_result", start)
        callback = source[start:end]
        self.assertIn("boon_reward_transaction_submit", callback)
        for forbidden in ("qry(", "db_query", "mysql_", "redis_", "fopen("):
            self.assertNotIn(forbidden, callback)

    def test_flat_boon_shop_submits_before_live_or_sql_mutation(self):
        source = (SRC / "boon.c").read_text()
        start = source.index("void boon_shop(P_char")
        end = source.index("struct flat_boon_display_filters", start)
        shop = source[start:end]
        branch = shop.index("PERSISTENCE_MODE_FLATFILE_PRIMARY")
        submit = shop.index("boon_shop_transaction_submit", branch)
        live = shop.index("bshop.stats--", submit)
        sql = shop.index('qry("UPDATE boons_shop', live)
        self.assertLess(branch, submit)
        self.assertLess(submit, live)
        self.assertLess(live, sql)
        comm = (SRC / "comm.c").read_text()
        self.assertIn("boon_shop_transaction_handle_completions(completions, count)", comm)

    def test_epic_touch_uses_one_immutable_zone_batch(self):
        source = (SRC / "epic.c").read_text()
        start = source.index("int epic_stone(P_obj")
        end = source.index("void epic_publish_zone_touch", start)
        touch = source[start:end]
        self.assertIn("participant_pids", touch)
        self.assertIn("zone_touch_transaction_submit", touch)
        for forbidden in ("zone_touches", "redis_invalidate_epic_zones", "UPDATE zones"):
            self.assertNotIn(forbidden, touch)
        publish = source[end:source.index("void epic_zone_balance", end)]
        self.assertNotIn("update_epic_zone_alignment", publish)
        self.assertNotIn("db_query", publish)

    def test_repository_schema_and_dispatch_contracts(self):
        boon = (SRC / "boon_reward_repository.c").read_text()
        zone = (SRC / "zone_touch_repository.c").read_text()
        generic = (SRC / "critical_command_repository.c").read_text()
        for token in ("FOR UPDATE", "boons_progress", "boons_shop",
                      "boon_reward_outcome_entry"):
            self.assertIn(token, boon)
        for token in ("FOR UPDATE", "zone_touches", "zone_touch_outcome_participant",
                      "alignment=LEAST"):
            self.assertIn(token, zone)
        self.assertIn("critical_command_type::boon_reward", generic)
        self.assertIn("critical_command_type::zone", generic)
        migration = (ROOT / "migrations/boon_reward_zone_outcome.sql").read_text()
        bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        runner = (ROOT / "migrations/run_migration.sh").read_text()
        for token in ("boon_reward_outcome", "boon_reward_outcome_entry",
                      "zone_touch_outcome", "zone_touch_outcome_participant"):
            self.assertIn(token, migration)
            self.assertIn(token, bootstrap)
        self.assertIn("verify_boon_reward_zone_schema.sh", runner)

    def test_flat_boon_rewards_replay_before_acknowledgement(self):
        repository = (SRC / "flatfile_boon_repository.c").read_text()
        transaction = (SRC / "boon_reward_transaction.c").read_text()
        nanny = (SRC / "nanny.c").read_text()

        for token in (
            "flatfile_boon_find_pending_reward",
            "flatfile_boon_acknowledge_reward",
            "catalog_legacy_version",
            "reward_published",
        ):
            self.assertIn(token, repository)

        ready_start = transaction.index("void boon_reward_transaction_player_ready")
        ready_end = transaction.index("critical_outbox_delivery_result", ready_start)
        ready = transaction[ready_start:ready_end]
        publish = ready.index("boon_publish_transaction_result")
        acknowledge = ready.index("acknowledge_flat_reward")
        self.assertLess(publish, acknowledge)
        self.assertIn("flatfile_boon_find_pending_reward", ready)

        self.assertEqual(nanny.count("boon_reward_transaction_player_ready("), 2)

    def test_flat_boon_query_helpers_route_before_sql(self):
        boon = (SRC / "boon.c").read_text()
        for function, flat_token, sql_token in (
            ("int is_boon_valid", "flatfile_boon_load_definitions", "qry("),
            ("int count_boons", "flatfile_boon_load_definitions", "qry("),
            ("bool get_boon_data", "flatfile_boon_load_definitions", "qry("),
            ("bool get_boon_progress_data", "flatfile_boon_load_progress", "qry("),
            ("bool get_boon_shop_data", "flatfile_boon_load_player", "qry("),
        ):
            start = boon.index(function)
            next_function = boon.find("\n}\n", start) + 3
            body = boon[start:next_function]
            self.assertIn("flat_boon_root", body)
            self.assertLess(body.index(flat_token), body.index(sql_token))

    def test_flat_boon_definition_mutations_route_before_sql(self):
        boon = (SRC / "boon.c").read_text()
        for function, flat_token, sql_token in (
            ("int create_boon", "flatfile_boon_create", "qry("),
            ("int remove_boon", "flatfile_boon_deactivate", "qry("),
            ("int extend_boon", "flatfile_boon_extend", "qry("),
        ):
            start = boon.index(function)
            next_function = boon.find("\n}\n", start) + 3
            body = boon[start:next_function]
            self.assertIn("flat_boon_root", body)
            self.assertLess(body.index(flat_token), body.index(sql_token))

    def test_flat_boon_maintenance_enumerates_the_catalog(self):
        boon = (SRC / "boon.c").read_text()
        start = boon.index("void boon_maintenance()")
        end = boon.index("void boon_random_maintenance()", start)
        maintenance = boon[start:end]
        self.assertIn("flatfile_boon_load_definitions", maintenance)
        self.assertIn("definition.active", maintenance)
        self.assertLess(
            maintenance.index("flatfile_boon_load_definitions"),
            maintenance.index('qry("SELECT id FROM boons'),
        )

    def test_flat_boon_display_routes_before_sql_with_all_filters(self):
        boon = (SRC / "boon.c").read_text()
        start = boon.index("int boon_display(P_char")
        end = boon.index("int create_boon(BoonData", start)
        display = boon[start:end]
        self.assertLess(display.index("boon_display_flat"), display.index("qry(dbqry)"))
        for token in (
            "flat_filters.player_ids",
            "flat_filters.authors",
            "flat_filters.types",
            "flat_filters.options",
            "flat_filters.active",
            "flat_filters.inactive",
            "flat_filters.random",
            "flat_filters.manual",
        ):
            self.assertIn(token, display)

        flat_start = boon.index("static int boon_display_flat")
        flat_display = boon[flat_start:start]
        for token in (
            "flatfile_boon_load_definitions",
            "boon_like_match",
            "definition.target_pid",
            "definition.racewar",
            "boon_display_row(ch, boon)",
            "Displaying %d result(s)",
        ):
            self.assertIn(token, flat_display)
        self.assertIn("boon_display_row(ch, boon)", display)
        renderer_start = boon.index("static const char *boon_affect_label")
        renderer = boon[renderer_start:flat_start]
        for token in (
            "boon_type_description",
            "boon_option_description",
            "boon_mob_label",
            "boon_race_label",
            "nexus_stone_info",
            "get_building_from_id",
            "Guildhall::find_by_id",
            "Invalid Affect",
            "Invalid Spell",
            "Invalid Attribute",
        ):
            self.assertIn(token, renderer)

    def test_account_bound_reward_boundary_remains_covered(self):
        contracts = "\n".join((ROOT / path).read_text() for path in (
            "tests/async/test_account_bound_reward_contract.py",
            "tests/async/test_account_reward_exact_contract.py",
            "tests/async/run_account_bound_reward_schema_mysql.sh",
        ))
        reward = (SRC / "account_reward.c").read_text()
        for token in ("DIVINECLAIM", "cooldown", "recovery_ready"):
            self.assertIn(token.lower(), contracts.lower())
        self.assertIn("FOR UPDATE", reward)
        self.assertIn("sql_begin_transaction", reward)


if __name__ == "__main__":
    unittest.main()
