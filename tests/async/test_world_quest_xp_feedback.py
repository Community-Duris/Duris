#!/usr/bin/env python3
"""Execute bartender quest XP awards and feedback with production function bodies."""

from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

from _paths import ROOT, source
from _source_contract import function_body, strip_comments


def extract_function(name: str, signature: str) -> str:
    text = source(name).read_text(encoding="utf-8")
    body = function_body(text, re.escape(signature))
    assert body, signature
    start = strip_comments(text).index(signature)
    opening = text.index("{", start)
    return text[start:opening + len(body)]


PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "core/files.h"
#include "magic/spells.h"
#include "persistence/persistence_checkpoint.h"
#include "net/comm.h"
#include "net/gmcp.h"
#include "world/difficulty.h"
#include "world/rested.h"
#include "world/hardcore_config.h"
#include "world/epic_bonus.h"
#include "combat/justice.h"
#include "combat/frag_cap_config.h"
#include "economy/boon.h"
#include "economy/nexus_stones.h"
#include "item/trophy.h"
#include "item/item_movement_transaction.h"
#include "sql/sql.h"
#include "telemetry/telemetry_runtime.h"
#include <cassert>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

room_data rooms[1]{};
P_room world = rooms;
extern const int top_of_world = 0;
zone_data zones[1]{};
zone_data *zone_table = zones;
index_data mobs[2]{};
P_index mob_index = mobs;
P_obj object_list = nullptr;
long new_exp_table[TOTALLVLS]{};
long global_exp_limit = 100000;
float exp_mods[EXPMOD_MAX + 1]{};
float racial_exp_mods[LAST_RACE + 1]{};
float racial_exp_mod_victims[LAST_RACE + 1]{};
std::string output;
double earned_multiplier = 1.0;
int rested_tag = 0, dirty = 0, epics = 0, finished = 0;
bool grant_allowed = true;
obj_data reward{};
namespace economic_gameplay_authority {
bool enabled = false;
bool active() { return enabled; }
}

int get_property(const char *key, int fallback) {
    if (!std::strcmp(key, "exp.maxExpLevel")) return 20;
    return fallback;
}
float get_property(const char *, double fallback) { return fallback; }
int sql_level_cap(int) { return 20; }
int frag_cap_config_hardcore_level_cap(int cap) { return cap; }
const hardcore_config *hardcore_config_get() { static hardcore_config config{}; return &config; }
P_char get_linked_char(P_char, ush_int) { return nullptr; }
int IS_MORPH(P_char) { return false; }
int GET_CLASS(P_char, unsigned int) { return 0; }
int BOUNDED(int low, int value, int high) { return MAX(low, MIN(value, high)); }
int shop_keeper(P_char, P_char, int, char *) { return 0; }
bool opposite_racewar(P_char, P_char) { return false; }
bool grouped(P_char, P_char) { return false; }
bool has_active_rested_bonus(P_char, int tag) { return tag == rested_tag; }
double difficulty_multiplier(difficulty_dial dial) {
    return dial == DIFFICULTY_EXP_EARNED ? earned_multiplier : 1.0;
}
float gain_global_exp_modifiers(P_char, float xp) { return xp; }
float gain_exp_modifiers(P_char, P_char, float xp) { return xp; }
int check_nexus_bonus(P_char, int xp, int) { return xp; }
float get_epic_bonus(P_char, int) { return 0; }
void check_boon_completion(P_char, P_char, double, int) {}
bool record_zone_trophy_award(P_char, P_char, int, int) { return false; }
void mark_player_dirty_components(int, player_component_mask_t) { ++dirty; }
telemetry_capture_result telemetry_runtime_game_progression(
    P_char, P_desc, telemetry_progression_observation) { return {}; }
void gmcp_char_vitals(P_char) {}
void gmcp_quest_status(P_char) {}
void logexp(const char *, ...) {}
void logit(const char *, const char *, ...) {}
void wizlog(int, const char *, ...) {}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
void send_to_char(const char *text, P_char) { output += text; }
void send_to_char_f(P_char, const char *format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    output += buffer;
}
void act(const char *text, int, P_char, P_obj, void *, int target) {
    if (target == TO_VICT || target == TO_CHAR) output += text;
}
void mobsay(P_char, const char *) {}
char *coin_stringv(int, int) { static char text[] = "coins"; return text; }
int number(int low, int) { return low; }
void ADD_MONEY(P_char, int) { assert(false); }
P_obj quest_item_reward(P_char) { return &reward; }
static bool grant_world_quest_reward(P_char, P_obj, int = 0, bool = false) {
    return grant_allowed;
}
bool item_movement_transaction_player_busy(P_char) { return false; }
void quest_epic_reward(P_char, int) { ++epics; }
void sql_world_quest_finished(P_char, P_obj) { ++finished; }
void resetQuest(P_char ch) {
    ch->only.pc->quest_active = 0;
    ch->only.pc->quest_mob_vnum = 0;
}
static void advance_level_impl(P_char, bool, bool, std::uint64_t) { assert(false); }
static void lose_level_impl(P_char, std::uint64_t,
    telemetry_progression_source, telemetry_progression_reason) { assert(false); }
static void notify_level_advancement(P_char, int) {}
'''

CASES = r'''
int main() {
    char_data player{}, mob{}, other{};
    pc_only_data pc{}, other_pc{};
    npc_only_data npc{};
    player.only.pc = &pc;
    other.only.pc = &other_pc;
    player.player.name = const_cast<char *>("Tester");
    player.player.level = 20;
    player.player.race = RACE_HUMAN;
    player.specials.position = STAT_NORMAL;
    player.specials.act2 = PLR2_EXP;
    pc.pid = 42;
    mob.only.npc = &npc;
    mob.specials.act = ACT_ISNPC;
    mob.player.name = const_cast<char *>("target");
    mob.player.short_descr = const_cast<char *>("a target");
    mob.player.level = 20;
    mob.player.race = RACE_HUMAN;
    mobs[0].virtual_number = 55;
    mobs[1].virtual_number = 56;
    reward.short_description = const_cast<char *>("a reward");
    for (auto &xp : new_exp_table) xp = 10000;
    for (auto &mod : exp_mods) mod = 1.0f;
    for (auto &mod : racial_exp_mods) mod = 1.0f;
    for (auto &mod : racial_exp_mod_victims) mod = 1.0f;
    auto reset = [&] {
        output.clear();
        dirty = epics = finished = 0;
        pc.quest_active = 1;
        pc.quest_mob_vnum = 55;
        pc.quest_type = FIND_AND_KILL;
        pc.quest_accomplished = 0;
        pc.quest_kill_original = 5;
        pc.quest_kill_how_many = 0;
        player.points.curr_exp = 0;
        player.player.level = 20;
        player.specials.position = STAT_NORMAL;
        player.specials.act2 = PLR2_EXP;
        npc.R_num = 0;
        rooms[0].room_flags = 0;
        global_exp_limit = 100000;
        new_exp_table[21] = 10000;
        racial_exp_mods[RACE_HUMAN] = 1.0f;
        earned_multiplier = 1.0;
        rested_tag = 0;
    };
    auto quest_line = [&](int amount) {
        const std::string line = "&+CQuest EXP:&+G " + std::to_string(amount) + " \r\n";
        assert(output.find(line) != std::string::npos);
        assert(output.find("Quest EXP:", output.find("Quest EXP:") + 1) == std::string::npos);
        assert(output.find("&+CEXP:") == std::string::npos);
    };
    reset();
    quest_kill(&player, &mob);
    assert(player.points.curr_exp == 260 && pc.quest_kill_how_many == 1);
    quest_line(260);
    assert(output.find("you're not done yet") != std::string::npos);
    output.clear();
    pc.quest_kill_how_many = 4;
    quest_kill(&player, &mob);
    assert(player.points.curr_exp == 520 && finished == 1 && epics == 1);
    quest_line(260);
    assert(output.find("you finished your quest!") != std::string::npos);
    assert(output.find("For completing your quest you receive") != std::string::npos);
    assert(pc.quest_active == 0);

    // Qualification failures must neither award XP nor advance the kill count.
    for (int scenario = 0; scenario < 6; ++scenario) {
        reset();
        if (scenario == 0) npc.R_num = 1;
        if (scenario == 1) pc.quest_active = 0;
        if (scenario == 2) pc.quest_accomplished = 1;
        if (scenario == 3) player.specials.position = STAT_DEAD;
        quest_kill(&player, scenario == 4 ? &other : scenario == 5 ? nullptr : &mob);
        assert(player.points.curr_exp == 0 && pc.quest_kill_how_many == 0);
        assert(output.find("Quest EXP:") == std::string::npos);
    }
    // Report the modified, truncated, per-award capped amount, never the request.
    reset();
    rested_tag = TAG_RESTED;
    racial_exp_mods[RACE_HUMAN] = 0.75f;
    earned_multiplier = 0.5;
    quest_kill(&player, &mob);
    assert(player.points.curr_exp == 146);
    quest_line(146);
    reset();
    assert(gain_exp(&player, nullptr, 100000, EXP_WORLD_QUEST) == 3333);
    assert(player.points.curr_exp == 3333);
    quest_line(3333);
    // Zero applied awards: modifiers, integer truncation, XP caps and safe rooms.
    for (int scenario = 0; scenario < 6; ++scenario) {
        reset();
        if (scenario == 0) earned_multiplier = 0;
        if (scenario == 1) racial_exp_mods[RACE_HUMAN] = 0.001f;
        if (scenario == 2) player.points.curr_exp = global_exp_limit;
        if (scenario == 3) player.points.curr_exp = 2 * new_exp_table[21];
        if (scenario == 4) rooms[0].room_flags = ROOM_SAFE;
        if (scenario == 5) new_exp_table[21] = 2;
        const int before = player.points.curr_exp;
        quest_kill(&player, &mob);
        assert(player.points.curr_exp == before);
        assert(output.find("Quest EXP:") == std::string::npos);
    }
    reset();
    player.specials.act2 = 0;
    quest_kill(&player, &mob);
    assert(player.points.curr_exp == 260 && output.find("Quest EXP:") == std::string::npos);
    reset();
    player.player.level = MINLVLIMMORTAL;
    gain_exp(&player, nullptr, 123, EXP_WORLD_QUEST);
    assert(player.points.curr_exp == 0 && output.empty());
    reset();
    display_gain(&player, 0, EXP_WORLD_QUEST);
    display_gain(&player, -10, EXP_WORLD_QUEST);
    assert(output.empty());

    // Turn-ins retain their existing text and emit exactly one numeric XP line.
    reset();
    quest_full_reward(&player, &mob, FIND_AND_ASK);
    assert(player.points.curr_exp == 1300 && finished == 1 && epics == 1);
    quest_line(1300);
    assert(output.find("You gain some experience.") != std::string::npos);

    reset();
    assert(gain_exp(&player, &mob, 100, EXP_KILL) == 100);
    assert(player.points.curr_exp == 100 && output == "&+CEXP:&+G 100 \r\n");
    output.clear();
    quest_kill(&player, &mob);
    assert(player.points.curr_exp == 360);
    quest_line(260);
    reset();
    player.specials.act2 = 0;
    gain_exp(&player, &mob, 100, EXP_KILL);
    assert(player.points.curr_exp == 100 && output.empty());
    reset();
    player.points.curr_exp = global_exp_limit;
    gain_exp(&player, &mob, 100, EXP_KILL);
    assert(player.points.curr_exp == global_exp_limit && output == "&+CEXP:&+G 100 \r\n");
    reset();
    gain_exp(&player, nullptr, 100, EXP_QUEST);
    assert(player.points.curr_exp == 100 && output.empty());

#ifdef QUEST_ITEM_COMPLETION
    // Accounting defers the last kill and turn-in awards until item settlement.
    economic_gameplay_authority::enabled = true;
    reward.obj_uid = 1000;
    reward.loc_p = LOC_CARRIED;
    reward.loc.carrying = &player;
    object_list = &reward;
    for (int scenario = 0; scenario < 4; ++scenario) {
        reset();
        pc.quest_started = 100;
        pc.quest_kill_how_many = 4;
        if (scenario == 1) global_exp_limit = 0;
        if (scenario == 2) player.specials.act2 = 0;
        quest_kill(&player, &mob);
        assert(player.points.curr_exp == 0 && output.empty());
        const world_quest_reward_context context = {
            world_quest_reward_source_id(&player), 1000, 42, 55, FIND_AND_KILL, false
        };
        item_transfer_result result{};
        result.root_item_uid = 1000;
        result.item_count = 1;
        world_quest_reward_completed(&player, scenario != 3, result, 0,
            reinterpret_cast<const uint8_t *>(&context), sizeof(context));
        if (scenario == 1 || scenario == 3) assert(player.points.curr_exp == 0);
        else assert(player.points.curr_exp == 260);
        if (scenario == 1 || scenario == 2 || scenario == 3)
            assert(output.find("Quest EXP:") == std::string::npos);
        else quest_line(260);
        if (scenario == 3) {
            assert(pc.quest_active == 1 && pc.quest_kill_how_many == 4 && finished == 0);
        } else {
            assert(output.find("you finished your quest!") != std::string::npos);
            assert(finished == 1 && epics == 1 && pc.quest_active == 0);
            const std::string published = output;
            world_quest_reward_completed(&player, true, result, 0,
                reinterpret_cast<const uint8_t *>(&context), sizeof(context));
            assert(output == published && finished == 1);
        }
    }
    reset();
    pc.quest_started = 101;
    quest_full_reward(&player, &mob, FIND_AND_ASK);
    assert(player.points.curr_exp == 0 && output.empty());
    const world_quest_reward_context context = {
        world_quest_reward_source_id(&player), 1000, 42, 55, FIND_AND_ASK, true
    };
    item_transfer_result result{};
    result.root_item_uid = 1000;
    result.item_count = 1;
    world_quest_reward_completed(&player, true, result, 0,
        reinterpret_cast<const uint8_t *>(&context), sizeof(context));
    assert(player.points.curr_exp == 1300 && finished == 1 && epics == 1);
    quest_line(1300);
    assert(output.find("You gain some experience.") != std::string::npos);
#endif
}
'''


def main() -> None:
    functions = [
        extract_function("limits.c", signature)
        for signature in (
            "static telemetry_progression_source progression_source_for_type(",
            "static telemetry_progression_reason progression_reason_for_type(",
            "void display_gain(",
            "float gain_exp_modifiers_race_only(",
            "int exp_level_percent_modifier(",
            "int gain_exp(",
        )
    ]
    functions += [extract_function("world_quest.c", signature) for signature in (
        "int quest_exp_reward(", "void quest_full_reward(", "void quest_kill(")]
    quest = source("world_quest.c").read_text(encoding="utf-8")
    if "static void world_quest_reward_completed(" in quest:
        start = quest.index("struct world_quest_reward_context\n{")
        context = quest[start:quest.index("};", start) + 2]
        functions += ["#define QUEST_ITEM_COMPLETION", context]
        functions += [extract_function("world_quest.c", signature) for signature in (
            "static uint64_t world_quest_reward_source_id(",
            "static void world_quest_reward_completed(",
        )]
    directory = ROOT / "bin" / "tests"
    directory.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="quest-xp-", dir=directory) as temporary:
        program = Path(temporary) / "harness.cpp"
        program.write_text(PRELUDE + "\n" + "\n".join(functions) + CASES, encoding="utf-8")
        for backend in ("mariadb", "flatfile"):
            binary = Path(temporary) / backend
            flags = (["-D__NO_MYSQL__", "-Isrc/no_mysql"] if backend == "flatfile" else
                     shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True)))
            subprocess.run(shlex.split(os.environ.get("CXX", "g++")) + [
                "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
                *flags, str(program), "src/telemetry/telemetry_progression.c", "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True, timeout=10)
    print("world quest XP award and feedback executable passed (mariadb + flatfile)")


if __name__ == "__main__":
    main()
