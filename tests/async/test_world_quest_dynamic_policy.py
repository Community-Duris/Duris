#!/usr/bin/env python3
"""Contracts for the boot-cached dynamic bartender world-quest policy."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, SRC, extract_function, source


WORLD_QUEST = source("world_quest.c").read_text()
COMM = source("comm.c").read_text()
MOBILE = source("specs.mobile.c").read_text()
POLICY = source("world/world_quest_policy.c").read_text()
SQL = source("sql/sql.c").read_text()
DB = source("world/db.c").read_text()
CHAOS = source("combat/chaos.c").read_text()
UTILITY = source("core/utility.c").read_text()
DUAL_BACKEND = (ROOT / "tests" / "async" / "run_world_quest_dual_backend.py").read_text()


def test_policy_math_and_item_floor() -> None:
    """The score is deterministic and the item floor has inclusive boundaries."""
    harness = r'''
#include "world/world_quest_policy_math.h"
#include <cmath>
#include <iostream>
int main() {
    if (world_quest_item_passes_floor(11, 5)) return 1;
    if (!world_quest_item_passes_floor(11, 6)) return 2;
    if (world_quest_item_passes_floor(20, 9)) return 3;
    if (!world_quest_item_passes_floor(20, 10)) return 4;
    if (world_quest_item_passes_floor(41, 20)) return 5;
    if (!world_quest_item_passes_floor(41, 21)) return 6;
    const double score = world_quest_zone_raw_score(45, 46.8, 128.3, 287);
    if (score <= 0.0 || !std::isfinite(score)) return 7;
    const double fit = world_quest_zone_level_fit(45, 46.8);
    if (std::abs(fit - std::exp(-1.8 / 6.0)) > 1e-12) return 8;
    std::cout << score << "\n";
}
'''
    with tempfile.TemporaryDirectory(prefix="duris-world-quest-math-") as directory:
        root = Path(directory)
        source_path = root / "policy_math_test.cpp"
        binary = root / "policy_math_test"
        source_path.write_text(harness)
        subprocess.run(
            ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-Isrc", str(source_path), "-o", str(binary)],
            cwd=ROOT,
            check=True,
        )
        completed = subprocess.run([str(binary)], cwd=ROOT, text=True,
                                   capture_output=True, check=True)
        assert float(completed.stdout.strip()) > 0.0


def test_explicit_deny_controls_are_not_an_approval_list() -> None:
    policy = source("world/world_quest_policy.c").read_text()
    assert "WORLD_QUEST_EXPLICIT_DENY_ZONES" in policy
    for zone in ("0", "292", "536"):
        assert zone in policy
    assert "get_zone_info" not in policy
    assert "ZONE_TOWN" in policy


def test_zone_list_is_global_and_does_not_scan_on_request() -> None:
    function = extract_function("world_quest.c", "void getQuestZoneList(")
    assert "get_zone_info" not in function
    assert "quest_zone" not in function
    assert "curMapExits" not in function
    assert "top_of_mobt" not in function
    assert "top_of_objt" not in function
    assert "world_quest_policy_zone_list" in function


def test_suggest_target_reads_cached_candidates() -> None:
    function = extract_function("world_quest.c", "int suggestQuestMob(")
    assert "world_quest_policy_suggest_mob" in function
    assert "top_of_mobt" not in function


def test_boot_populates_catalog_before_loop_in_all_modes() -> None:
    game = COMM[COMM.index("void run_the_game(int port, int sslport)"):]
    boot_start = game.index("-- Calculating avg mob level and world-quest catalog.")
    boot_end = game.index("--  Done calculating mob level and world-quest catalog.", boot_start)
    boot = game[boot_start:boot_end]
    assert "if (calc_zone_mob_level() < 0)" in boot
    # The catalog is a boot concern, not an abandon/request concern.
    assert "world_quest_policy_bootstrap" not in MOBILE
    assert "world_quest_policy_bootstrap" not in extract_function("world_quest.c", "void getQuestZoneList(")


def test_catalog_failure_is_latched_and_scores_are_boot_cached() -> None:
    assert "bool quest_catalog_failed = false;" in POLICY
    assert "if (quest_catalog_ready || quest_catalog_failed)" in POLICY
    assert "quest_catalog_failed = true;" in POLICY
    assert "zone_score" in POLICY
    assert "load_zone_scores();" in POLICY


def test_target_probes_have_one_request_budget_and_no_failed_state_mutation() -> None:
    create = extract_function("world_quest.c", "bool createQuest(")
    assert "target_probe_budget = WORLD_QUEST_MAX_TARGET_PROBES" in create
    assert "&target_probe_budget" in create
    assert create.index("const int rnum = real_mobile(quest_mob);") < create.index(
        "ch->only.pc->quest_active = 1;")
    assert "quest_kill_how_many = 0;" not in extract_function(
        "world/world_quest_policy.c", "int select_cached_mob(")


def test_mariadb_world_quest_reads_fail_closed() -> None:
    db_done_start = SQL.rfind("int sql_world_quest_done_already(")
    db_done = SQL[db_done_start:SQL.index("const char *sql_select_IP_info", db_done_start)]
    quota_start = SQL.rfind("int sql_world_quest_can_do_another(")
    quota = SQL[quota_start:db_done_start]
    assert "if (!db)" in db_done and "return -1;" in db_done
    assert "NULL == row || NULL == row[0]" in db_done
    assert "if (!db)" in quota and "return -1;" in quota
    assert "NULL == row || NULL == row[0]" in quota


def test_temporary_mobile_load_failures_are_cleaned() -> None:
    read_mobile_start = DB.index("P_char read_mobile(")
    read_mobile = DB[read_mobile_start:]
    assert "mm_release(dead_mob_pool, mob);" in read_mobile
    assert "partial_mobile_name" in read_mobile
    assert "SET_BIT(mob->specials.act, ACT_ISNPC);" in read_mobile
    assert "mobile_probe_guard" in POLICY
    assert "object_probe_guard" in POLICY
    assert "P_char read_mobile_probe(int nr, int type)" in DB
    assert "mobile_probe_mode = true;" in DB
    assert "if (!mobile_probe_mode)" in DB


def test_local_quest_helper_is_not_a_general_mortal_teleport() -> None:
    assert "CHAOS_TEST_ACCOUNT" in CHAOS
    assert "ch->desc->host" in CHAOS
    assert "room_vnum != 16633" in CHAOS
    assert "#ifdef TEST_MUD" not in CHAOS
    assert "currency_transaction_submit_wallet_value" in CHAOS


def test_flatfile_wallet_adjustments_use_backend_neutral_transactions() -> None:
    add_money = extract_function("core/utility.c", "void ADD_MONEY(")
    sub_money = extract_function("core/utility.c", "int SUB_MONEY(")
    for function in (add_money, sub_money):
        assert "currency_transaction_submit_wallet_value" in function
        assert "#ifndef __NO_MYSQL__" not in function


def test_questroom_wallet_adjustment_reports_async_completion_in_all_backends() -> None:
    questroom = extract_function("combat/chaos.c", "static bool chaos_test_questroom(")
    callback = extract_function("combat/chaos.c", "static void chaos_test_funds_committed(")
    assert "currency_transaction_submit_wallet_value" in questroom
    assert "chaos_test_funds_committed" in questroom
    assert "#ifdef __NO_MYSQL__" not in questroom
    assert "#ifndef __NO_MYSQL__" not in callback


def test_zone_average_uses_integer_sum_for_truncation_and_level_boundaries() -> None:
    harness = r'''
#include "world/world_quest_policy_math.h"
int main() {
    // Split Shield's verified 34 mobile prototypes sum to 544.
    if (world_quest_truncated_average_level(544, 34) != 16) return 1;
    if (world_quest_zone_level_window_accepts(11, 16)) return 2;
    if (!world_quest_zone_level_window_accepts(22, 16)) return 3;
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="duris-world-quest-average-") as directory:
        root = Path(directory)
        source_path = root / "average_test.cpp"
        binary = root / "average_test"
        source_path.write_text(harness)
        subprocess.run(
            ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-Isrc", str(source_path), "-o", str(binary)],
            cwd=ROOT,
            check=True,
        )
        subprocess.run([str(binary)], cwd=ROOT, check=True)
    assert "mob_level_sum" in POLICY
    assert "mob_level_count" in POLICY
    assert "static_cast<double>(zone.mob_level_sum)" in POLICY


def test_dual_backend_creates_build_parent_before_temporary_directory() -> None:
    main = DUAL_BACKEND[DUAL_BACKEND.index("def main()") :]
    assert 'build_parent = ROOT / "bin" / "tests"' in main
    assert "build_parent.mkdir(parents=True, exist_ok=True)" in main


def test_dual_backend_publishes_mariadb_on_a_dynamic_host_port() -> None:
    assert "--network=container:hermes" not in DUAL_BACKEND
    assert '"--publish", "127.0.0.1::3306/tcp"' in DUAL_BACKEND
    assert '"docker", "port", container, "3306/tcp"' in DUAL_BACKEND
    assert "return container, port, root_password, db_password" in DUAL_BACKEND


def test_quest_reward_uses_accepted_level_cache() -> None:
    function = extract_function("world_quest.c", "P_obj quest_item_reward(")
    assert "getQuestItemFromZone" in function
    assert "quest_level" in function


def test_failure_reason_reaches_bartender_feedback() -> None:
    assert "quest_creation_failure" in MOBILE
    assert "eligible quest zone" in MOBILE.lower()
    assert "suitable quest target" in MOBILE.lower()


if __name__ == "__main__":
    tests = [
        test_policy_math_and_item_floor,
        test_explicit_deny_controls_are_not_an_approval_list,
        test_zone_list_is_global_and_does_not_scan_on_request,
        test_suggest_target_reads_cached_candidates,
        test_boot_populates_catalog_before_loop_in_all_modes,
        test_catalog_failure_is_latched_and_scores_are_boot_cached,
        test_target_probes_have_one_request_budget_and_no_failed_state_mutation,
        test_mariadb_world_quest_reads_fail_closed,
        test_temporary_mobile_load_failures_are_cleaned,
        test_local_quest_helper_is_not_a_general_mortal_teleport,
        test_flatfile_wallet_adjustments_use_backend_neutral_transactions,
        test_questroom_wallet_adjustment_reports_async_completion_in_all_backends,
        test_zone_average_uses_integer_sum_for_truncation_and_level_boundaries,
        test_dual_backend_creates_build_parent_before_temporary_directory,
        test_dual_backend_publishes_mariadb_on_a_dynamic_host_port,
        test_quest_reward_uses_accepted_level_cache,
        test_failure_reason_reaches_bartender_feedback,
    ]
    for test in tests:
        test()
    print("dynamic world-quest policy contracts passed")
