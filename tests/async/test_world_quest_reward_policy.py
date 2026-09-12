#!/usr/bin/env python3
"""Contracts for bartender quest rewards.

Rewards still come from the quest's zone, but never quest items and never the zone's most
valuable items; each completed quest pays one reward to the player who completed it;
quests are unshareable by default; and the fee is a property.
"""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, extract_function, source


PROPERTIES = (ROOT / "lib" / "duris.properties").read_text()


def _run_harness(body: str, prefix: str) -> None:
    with tempfile.TemporaryDirectory(prefix=prefix) as directory:
        root = Path(directory)
        source_path = root / "harness.cpp"
        binary = root / "harness"
        source_path.write_text(body)
        subprocess.run(
            ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-Isrc", str(source_path), "-o", str(binary)],
            cwd=ROOT,
            check=True,
        )
        subprocess.run([str(binary)], cwd=ROOT, check=True)


def test_value_ceiling_withholds_the_most_valuable_share() -> None:
    _run_harness(r'''
#include "world/world_quest_policy_math.h"
#include <climits>
int main() {
    // Nothing to withhold: every item is below the ceiling.
    if (world_quest_reward_value_ceiling({}, 20) != INT_MAX) return 1;
    if (world_quest_reward_value_ceiling({5, 9, 30}, 0) != INT_MAX) return 2;
    // Rounded up, so a zone's single most valuable item is always withheld.
    if (world_quest_reward_value_ceiling({40}, 20) != 40) return 3;
    if (world_quest_reward_value_ceiling({10, 20, 30, 40, 50}, 20) != 50) return 4;
    if (world_quest_reward_value_ceiling({10, 20, 30, 40, 50, 60}, 20) != 50) return 5;
    // Items tied with the cheapest withheld item are withheld with it.
    if (world_quest_reward_value_ceiling({10, 50, 50, 50, 20}, 20) != 50) return 6;
    // 100 percent or more withholds everything: nothing is below the cheapest item.
    if (world_quest_reward_value_ceiling({7, 3, 9}, 100) != 3) return 7;
    if (world_quest_reward_value_ceiling({7, 3, 9}, 250) != 3) return 8;
    return 0;
}
''', "duris-world-quest-ceiling-")


def test_catalog_never_offers_quest_items() -> None:
    collect = extract_function("world/world_quest_policy.c",
                               "std::unordered_set<int> collect_quest_item_vnums(")
    assert "complete->receive" in collect and "complete->give" in collect
    assert collect.count("goal->goal_type == QUEST_GOAL_ITEM") == 2
    load = extract_function("world/world_quest_policy.c", "void load_reward_profiles(")
    assert "collect_quest_item_vnums()" in load
    eligible = load.index("if (!source_eligible_reward(probe.value))")
    excluded = load.index("if (quest_items.count(obj_index[rnum].virtual_number))")
    pooled = load.index("quest_zones[zone].rewards.push_back(profile);")
    assert eligible < excluded < pooled


def test_catalog_withholds_top_value_items_before_pools_and_scores() -> None:
    load = extract_function("world/world_quest_policy.c", "void load_reward_profiles(")
    assert '"world.quest.reward.top.withheld.percent", 20.000' in load
    ceiling = load.index("world_quest_reward_value_ceiling(values, withheld_percent)")
    assert "if (reward.ivalue >= ceiling)" in load
    # Unleveled pool, per-level pools and the zone scores' inputs all come after it.
    assert ceiling < load.index("zone.reward_vnums.push_back(reward.vnum);")
    assert ceiling < load.index("zone.reward_vnums_by_level[level].push_back(reward.vnum);")
    first_loop = load[: load.index("world_quest_reward_value_ceiling")]
    assert "reward_vnums.push_back" not in first_loop
    policy = source("world/world_quest_policy.c").read_text()
    assert policy.index("load_reward_profiles(withheld);") < policy.index("load_zone_scores();")
    assert "withheld_quest_items=%zu" in policy and "withheld_top_value=%zu" in policy


def test_nofear_block_reads_the_fourth_affect_field() -> None:
    eligible = extract_function("world/world_quest_policy.c", "bool source_eligible_reward(")
    assert "IS_SET(obj->bitvector4, AFF4_NOFEAR)" in eligible
    assert "IS_SET(obj->bitvector, AFF4_NOFEAR)" not in eligible


def test_kill_quests_pay_one_reward_on_completion() -> None:
    kill = extract_function("world_quest.c", "void quest_kill(")
    assert "obj_to_char(reward, quest_mob)" not in kill
    assert "quest_kill_original + 1) <= 2" not in kill
    assert kill.count("quest_item_reward(ch)") == 1
    done = kill[kill.index("quest_kill_how_many - ch->only.pc->quest_kill_original == 0"):]
    assert (done.index("quest_item_reward(ch)") < done.index("obj_to_char(reward, ch)")
            < done.index("resetQuest(ch)"))
    assert "sql_world_quest_finished(ch, reward)" in done


def test_quests_are_unshareable_by_default() -> None:
    limit = extract_function("world_quest.c", "static int world_quest_share_limit(")
    assert '"world.quest.share.max", 0.000' in limit
    create = extract_function("world_quest.c", "bool createQuest(")
    assert "quest_shares_left = world_quest_share_limit();" in create
    assert "quest_shares_left = 4;" not in create
    quest = extract_function("world_quest.c", "void do_quest(")
    share = quest[quest.index('isname(name, "share")'):]
    refusal = share.index("if (world_quest_share_limit() == 0)")
    assert refusal < share.index("quest_receiver != GET_PID(ch)")
    assert refusal < share.index("victim = ParseTarget(ch, who);")
    # Quests granted before the limit dropped cannot share past it.
    assert share.index("MIN(ch->only.pc->quest_shares_left, world_quest_share_limit())") < \
        share.index("if (ch->only.pc->quest_shares_left == 0)")


def test_bartender_fee_is_a_property() -> None:
    mobile = source("specs.mobile.c").read_text()
    assert "temp = 20 * GET_LEVEL(pl);" not in mobile
    assert '"world.quest.cost.per.level", 20.000' in mobile


def test_properties_ship_the_documented_defaults() -> None:
    section = PROPERTIES[PROPERTIES.index("[worldQuest]"):]
    section = section[: section.index("\n[", 1)].splitlines()
    for line in ("world.quest.share.max=0.000", "world.quest.cost.per.level=20.000",
                 "world.quest.reward.top.withheld.percent=20.000"):
        assert line in section, line


if __name__ == "__main__":
    tests = [
        test_value_ceiling_withholds_the_most_valuable_share,
        test_catalog_never_offers_quest_items,
        test_catalog_withholds_top_value_items_before_pools_and_scores,
        test_nofear_block_reads_the_fourth_affect_field,
        test_kill_quests_pay_one_reward_on_completion,
        test_quests_are_unshareable_by_default,
        test_bartender_fee_is_a_property,
        test_properties_ship_the_documented_defaults,
    ]
    for test in tests:
        test()
    print("world-quest reward policy contracts passed")
