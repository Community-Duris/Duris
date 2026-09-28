#!/usr/bin/env python3
"""Execute the world quest item-grant completion against isolated live state."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function, source


def main() -> None:
    world_quest = source("world_quest.c").read_text(encoding="utf-8")
    start = world_quest.index("struct world_quest_reward_context\n{")
    context = world_quest[start:world_quest.index("};", start) + 2]
    next_started = extract_function("world_quest.c", "static int world_quest_next_started(")
    source_id = extract_function("world_quest.c", "static uint64_t world_quest_reward_source_id(")
    completion = extract_function("world_quest.c", "static void world_quest_reward_completed(")
    reset = extract_function("world_quest.c", "void resetQuest(")
    assert "quest_started = 0" not in reset
    assert "world_quest_next_started(ch->only.pc->quest_started, time(NULL))" in world_quest
    share = world_quest[world_quest.index('if (isname(name, "share"))'):]
    assert share.index("const int recipient_started = world_quest_next_started(") < share.index(
        "resetQuest(victim);"
    ) < share.index("victim->only.pc->quest_started = recipient_started")
    program = r'''
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <ctime>

struct pc_data {
    int quest_started = 0;
    int quest_mob_vnum = 0;
    int quest_kill_original = 0;
    int quest_kill_how_many = 0;
};
struct character {
    struct { pc_data *pc = nullptr; } only;
    int pid = 0;
    bool npc = false;
};
struct object {
    uint64_t obj_uid = 0;
    object *next = nullptr;
    character *carrier = nullptr;
    const char *short_description = "a reward";
};
struct item_transfer_result {
    uint64_t root_item_uid = 0;
    uint32_t item_count = 0;
};
using P_char = character *;
using P_obj = object *;
P_obj object_list = nullptr;
#define IS_PC(ch) (!(ch)->npc)
#define GET_PID(ch) ((ch)->pid)
#define OBJ_CARRIED_BY(obj, ch) ((obj)->carrier == (ch))
constexpr int LOG_FILE = 1;
constexpr int PLAYER_COMPONENT_STATUS = 1;
constexpr int FIND_AND_KILL = 1;
constexpr int EXP_WORLD_QUEST = 1;
constexpr size_t ITEM_MOVEMENT_CONTEXT_MAX_BYTES = 128;
int logged = 0, notified = 0, dirty = 0, gmcp = 0, epic = 0, sql = 0, resets = 0;
int experience = 0;
void logit(int, const char *, ...) { ++logged; }
void send_to_char(const char *, P_char) { ++notified; }
void send_to_char_f(P_char, const char *, ...) { ++notified; }
void mark_player_dirty_components(int, int) { ++dirty; }
void gmcp_quest_status(P_char) { ++gmcp; }
int quest_exp_reward(P_char, int) { return 90; }
void gain_exp(P_char, P_char, int amount, int) { experience += amount; }
void quest_epic_reward(P_char, int) { ++epic; }
void sql_world_quest_finished(P_char, P_obj) { ++sql; }
void resetQuest(P_char ch) {
    ++resets;
    ch->only.pc->quest_mob_vnum = 0;
}
''' + context + "\n" + next_started + "\n" + source_id + "\n" + completion + r'''
void clear_counts() {
    logged = notified = dirty = gmcp = epic = sql = resets = experience = 0;
}
int main() {
    pc_data state;
    state.quest_started = 100;
    state.quest_mob_vnum = 55;
    state.quest_kill_original = 3;
    state.quest_kill_how_many = 3;
    character actor;
    actor.only.pc = &state;
    actor.pid = 42;
    const uint64_t first_source = world_quest_reward_source_id(&actor);
    state.quest_started = world_quest_next_started(state.quest_started, 100);
    assert(state.quest_started == 101);
    assert(world_quest_reward_source_id(&actor) != first_source);
    assert(world_quest_next_started(101, 99) == 102);
    assert(world_quest_next_started(101, 105) == 105);
    assert(world_quest_next_started(INT_MAX, INT_MAX) == 0);
    state.quest_started = 100;
    object item;
    item.obj_uid = 1000;
    item.carrier = &actor;
    object_list = &item;
    const world_quest_reward_context request = {
        world_quest_reward_source_id(&actor), 1000, 42, 55, FIND_AND_KILL, false
    };
    const item_transfer_result result = {1000, 1};

    world_quest_reward_completed(&actor, false, result, 5,
        reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    assert(state.quest_kill_how_many == 2);
    assert(resets == 0 && epic == 0 && experience == 0 && sql == 0);
    assert(dirty == 1 && gmcp == 1 && notified == 1);

    clear_counts();
    state.quest_kill_how_many = 3;
    object_list = nullptr;
    world_quest_reward_completed(&actor, true, result, 0,
        reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    assert(logged == 1 && resets == 0 && epic == 0 && experience == 0);

    clear_counts();
    object_list = &item;
    state.quest_started = 101;
    world_quest_reward_completed(&actor, true, result, 0,
        reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    assert(logged == 1 && resets == 0 && epic == 0 && sql == 0);

    clear_counts();
    state.quest_started = 100;
    world_quest_reward_completed(&actor, true, result, 0,
        reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    assert(experience == 30 && epic == 1 && sql == 1 && resets == 1);
    assert(state.quest_started == 100);
    world_quest_reward_completed(&actor, true, result, 0,
        reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    assert(experience == 30 && epic == 1 && sql == 1 && resets == 1);

    clear_counts();
    state.quest_started = 100;
    state.quest_mob_vnum = 55;
    const world_quest_reward_context full_request = {
        world_quest_reward_source_id(&actor), 1000, 42, 55, 2, true
    };
    world_quest_reward_completed(&actor, true, result, 0,
        reinterpret_cast<const uint8_t *>(&full_request), sizeof(full_request));
    assert(experience == 90 && epic == 1 && sql == 1 && resets == 1);
}
'''
    with tempfile.TemporaryDirectory(prefix="duris-world-quest-item-") as directory:
        source_path = Path(directory) / "harness.cpp"
        binary = Path(directory) / "harness"
        source_path.write_text(program, encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                        str(source_path), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("world-quest item completion callback passed")


if __name__ == "__main__":
    main()
