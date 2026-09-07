#!/usr/bin/env python3
"""Run the production quest retry/selection functions against deterministic fixtures."""

import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, extract_function


PRELUDE = r'''
#include <algorithm>
#include <cassert>
#include <ctime>
#include <vector>
#include "world/world_quest_policy_math.h"
using namespace std;
constexpr int FIND_AND_KILL = 1, FIND_AND_ASK = 2, MAXLVL = 62, NOWHERE = -1;
constexpr int ACT_SPEC = 1;
#define TRUE true
#define FALSE false
#define MIN(a, b) std::min(a, b)
#define GET_LEVEL(ch) ((ch)->level)
#define IS_NPC(ch) ((ch)->npc)
#define IS_PC(ch) (!IS_NPC(ch))
#define IS_TRUSTED(ch) false
#define GET_NAME(ch) "fixture"
#define GET_VNUM(ch) 16553
#define GET_PID(ch) 1
#define REMOVE_BIT(value, bits) ((value) &= ~(bits))
struct pc_data {
    int quest_shares_left = 0, quest_active = 0, quest_mob_vnum = 0, quest_type = 0;
    int quest_accomplished = 0, quest_zone_number = 0, quest_giver = 0, quest_level = 0;
    int quest_receiver = 0, quest_kill_how_many = 0, quest_kill_original = 0;
    time_t quest_started = 0;
    bool operator==(const pc_data &) const = default;
};
struct character {
    int level = 56;
    bool npc = false;
    struct { pc_data *pc = nullptr; } only;
    struct { int act = 0; } specials;
    int in_room = NOWHERE;
};
using P_char = character *;
#include "world/world_quest.h"
struct index_entry { int number = 2, limit = 10, virtual_number = 0; };
vector<index_entry> mob_index;
int top_of_mobt;
struct zone_entry { int number = 100; };
vector<zone_entry> zone_table;
struct quest_mob_profile {
    int rnum, vnum, level;
    bool can_speak = true, invisible = false, hidden = false;
};
struct quest_zone_profile { vector<quest_mob_profile> mobs; };
vector<quest_zone_profile> zones;
vector<int> history_reads;
vector<int> probe_reads;
int fresh_target = -1, error_target = -1;
int number(int low, int) { return low; }
void wizlog(int, const char *, ...) {}
void debug(const char *, ...) {}
int real_mobile(int vnum) { return vnum - 1000; }
void getQuestZoneList(P_char, vector<int> &out) {
    for (size_t i = 0; i < zones.size(); ++i) out.push_back(static_cast<int>(i));
}
int world_quest_policy_select_zone(P_char, const vector<int> &valid) { return valid.front(); }
int sql_world_quest_done_already(P_char, int target) {
    // A repeated history lookup is a bug even when the request eventually succeeds.
    assert(find(history_reads.begin(), history_reads.end(), target) == history_reads.end());
    history_reads.push_back(target);
    if (target == error_target) return -1;
    return target == fresh_target ? 0 : 1;
}
P_char read_mobile_probe(int vnum, int) {
    probe_reads.push_back(vnum);
    return new character;
}
constexpr int VIRTUAL = 1;
struct mobile_probe_guard {
    P_char value;
    ~mobile_probe_guard() { delete value; }
};
void char_to_room(P_char ch, int room, int) { ch->in_room = room; }
bool aggressive_to(P_char, P_char) { return false; }
'''

ADAPTER = r'''
int world_quest_policy_suggest_mob(int zone, P_char ch, int type, int *budget,
                                  const vector<int> &excluded = {}) {
    return select_cached_mob(zones[zone], ch, type, budget, excluded);
}
'''

SCENARIOS = r'''
void setup(int zone_count, int mobs_per_zone, int level, int instances) {
    zones.assign(zone_count, {});
    zone_table.assign(zone_count, {});
    mob_index.assign(zone_count * mobs_per_zone, {});
    history_reads.clear();
    probe_reads.clear();
    fresh_target = error_target = -1;
    top_of_mobt = static_cast<int>(mob_index.size()) - 1;
    for (int i = 0; i <= top_of_mobt; ++i) {
        mob_index[i].number = instances;
        mob_index[i].virtual_number = 1000 + i;
        zones[i / mobs_per_zone].mobs.push_back({i, 1000 + i, level});
    }
}
int main() {
    pc_data pc;
    character player, giver;
    player.only.pc = &pc;
    giver.npc = true;
    quest_creation_failure failure;

    // At level 56 there is only one quest type. Skip completed A and issue B.
    setup(1, 2, 56, 2);
    fresh_target = 1001;
    assert(createQuest(&player, &giver, &failure));
    assert((history_reads == vector<int>{1000, 1001}));
    assert(pc.quest_active == 1 && pc.quest_mob_vnum == 1001);
    assert(pc.quest_type == FIND_AND_KILL && failure == QUEST_CREATION_NO_FAILURE);

    // The same retry must work for ask quests and retain the shared probe budget.
    pc = {};
    player.level = 30;
    setup(1, 2, 30, 1);
    fresh_target = 1001;
    assert(createQuest(&player, &giver, &failure));
    assert((history_reads == vector<int>{1000, 1001}));
    assert(probe_reads == history_reads);
    assert(pc.quest_type == FIND_AND_ASK && pc.quest_mob_vnum == 1001);

    // All duplicates: stop after each distinct target, preserving existing state.
    const pc_data before = pc;
    setup(1, 2, 30, 1);
    assert(!createQuest(&player, &giver, &failure));
    assert((history_reads == vector<int>{1000, 1001}));
    assert(pc == before && failure == QUEST_CREATION_NO_ELIGIBLE_TARGET);

    // A backend error must stop immediately, even if a fresh target follows it.
    setup(1, 2, 30, 1);
    error_target = 1000;
    fresh_target = 1001;
    assert(!createQuest(&player, &giver, &failure));
    assert((history_reads == vector<int>{1000}));
    assert(pc == before && failure == QUEST_CREATION_NO_ELIGIBLE_TARGET);

    // Caps are per request, including retries within and across zones.
    player.level = 56;
    for (int zone_count : {1, 4}) {
        setup(zone_count, 64 / zone_count, 56, 2);
        fresh_target = 1000 + WORLD_QUEST_MAX_HISTORY_CHECKS;
        assert(!createQuest(&player, &giver, &failure));
        assert(history_reads.size() == WORLD_QUEST_MAX_HISTORY_CHECKS);
        assert(pc == before);
    }
    player.level = 30;
    setup(4, 16, 30, 1);
    assert(!createQuest(&player, &giver, &failure));
    assert(probe_reads.size() <= WORLD_QUEST_MAX_TARGET_PROBES);
    assert(history_reads.size() <= WORLD_QUEST_MAX_HISTORY_CHECKS);
    assert(pc == before);

    // The last permitted history check can still grant a valid quest.
    player.level = 56;
    setup(1, WORLD_QUEST_MAX_HISTORY_CHECKS, 56, 2);
    fresh_target = 999 + WORLD_QUEST_MAX_HISTORY_CHECKS;
    assert(createQuest(&player, &giver, &failure));
    assert(history_reads.size() == WORLD_QUEST_MAX_HISTORY_CHECKS);
    assert(pc.quest_mob_vnum == fresh_target);
}
'''


def main() -> None:
    harness = (
        PRELUDE
        + extract_function("world/world_quest_policy.c", "int select_cached_mob(")
        + ADAPTER
        + extract_function("world_quest.c", "bool createQuest(")
        + SCENARIOS
    )
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="world-quest-retries-", dir=build_root) as directory:
        path = Path(directory)
        source = path / "retries.cpp"
        binary = path / "retries"
        source.write_text(harness)
        subprocess.run(
            ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-Isrc", str(source), "-o", str(binary)],
            cwd=ROOT, check=True,
        )
        subprocess.run([str(binary)], cwd=ROOT, check=True)
    print("world quest target retry runtime regression passed")


if __name__ == "__main__":
    main()
