#!/usr/bin/env python3
"""Exercise the static quest's durable offering and publication callbacks."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function, source


quest = source("world/quest.c").read_text(encoding="utf-8")
grant = extract_function("world/quest.c", "void give_reward(struct quest_complete_data *qcp")
assert grant.count("economic_source_kind::quest_completion,") == 2
assert grant.count("source_id)") == 2
live_ready = extract_function(
    "item/item_movement_transaction.c", "bool destruction_publication_live_ready("
)
publish_movement = extract_function("item/item_movement_transaction.c", "void publish(")
assert "std::all_of(seen.begin(), seen.end(), [](uint8_t count) { return count == 0; })" in live_ready
assert publish_movement.index("pending.erase(current);") < publish_movement.index(
    "completion_fn(actor, committed, result, error_code"
)
context_start = quest.index("constexpr size_t QUEST_DURABLE_MAX_OFFERINGS")
context_end = quest.index("static P_obj quest_object_by_uid(", context_start)
functions = "\n".join(
    extract_function("world/quest.c", signature)
    for signature in (
        "static uint64_t legacy_quest_reward_source_id(",
        "static P_obj quest_object_by_uid(",
        "static struct quest_complete_data *quest_completion_by_index(",
        "static P_char quest_mobile_for(",
        "static bool publish_quest_offering(",
        "static void complete_quest_offering(",
        "static bool submit_durable_quest_offering(",
    )
)
program = r'''
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <climits>
#include <set>

struct character;
struct object {
    uint64_t obj_uid = 0;
    int vnum = 0;
    character *carrier = nullptr;
    object *next = nullptr;
    object *next_content = nullptr;
};
struct character {
    bool npc = false;
    int pid = 0, rnum = 0, vnum = 0, in_room = 0;
    character *next = nullptr;
    object *carrying = nullptr;
};
using P_char = character *;
using P_obj = object *;
struct goal_data { char goal_type; int number; goal_data *next = nullptr; };
struct quest_complete_data {
    goal_data *give = nullptr, *receive = nullptr;
    bool echoAll = false;
    const char *message = "done";
    quest_complete_data *next = nullptr;
};
struct quest_data { int quester = 0; quest_complete_data *quest_complete = nullptr; };
struct item_transfer_result {};
enum class item_owner_type { player, destruction };
struct item_owner_identity { item_owner_type type; uint64_t id, context_id; };
enum class item_transfer_reason { destruction };
enum class item_movement_reject { none };
enum class economic_source_kind { intentional_destruction };
namespace economic_gameplay_authority { bool active() { return false; } }
constexpr size_t ITEM_MOVEMENT_CONTEXT_MAX_BYTES = 128;
constexpr int QUEST_GOAL_ITEM = 'I', QUEST_GOAL_COINS = 'C';
constexpr int PLAYER_COMPONENT_STATUS = 1, PLAYER_COMPONENT_EQUIPMENT = 2;
constexpr int PLAYER_COMPONENT_INVENTORY = 4, LOG_DEBUG = 1;
constexpr int TRUE = 1, FALSE = 0;
#define IS_NPC(ch) ((ch)->npc)
#define GET_RNUM(ch) ((ch)->rnum)
#define GET_VNUM(ch) ((ch)->vnum)
#define GET_PID(ch) ((ch)->pid)
#define OBJ_VNUM(obj) ((obj)->vnum)
#define OBJ_CARRIED_BY(obj, ch) ((obj)->carrier == (ch))
#define TO_ROOM 1
#define TO_VICT 2

quest_data quest_index[1];
int number_of_quests = 1;
P_char character_list = nullptr;
P_obj object_list = nullptr;
int submissions = 0, rewards = 0, messages = 0, dirty = 0, removed = 0;
uint64_t rewarded_offering_uid = 0;
bool item_command_uses_durable_ownership(P_obj object) { return object != nullptr; }
const char *item_movement_reject_name(item_movement_reject) { return "none"; }
void logit(int, const char *, ...) {}
void send_to_char(const char *, P_char) { ++messages; }
void act(const char *, int, P_char, int, P_char, int) { ++messages; }
void mark_player_dirty_components(int, int) { ++dirty; }
void finish_quest_reward(quest_complete_data *, P_char, P_char, uint64_t offering_uid) {
    ++rewards;
    rewarded_offering_uid = offering_uid;
}
void extract_obj(P_obj object, int) {
    P_obj *link = &object_list;
    while (*link && *link != object) link = &(*link)->next;
    assert(*link == object);
    *link = object->next;
    link = &object->carrier->carrying;
    while (*link && *link != object) link = &(*link)->next_content;
    assert(*link == object);
    *link = object->next_content;
    object->carrier = nullptr;
    ++removed;
}
unsigned char saved_context[128] = {};
size_t saved_size = 0;
bool item_movement_transaction_submit_batch(
    P_char, P_obj const *, size_t, P_obj, const item_owner_identity &,
    const item_owner_identity &, item_transfer_reason, int64_t,
    auto, const void *context, size_t size, P_obj, item_movement_reject *, auto,
    economic_source_kind) {
    assert(size <= sizeof(saved_context));
    memcpy(saved_context, context, size);
    saved_size = size;
    ++submissions;
    return true;
}
''' + quest[context_start:context_end] + "\n" + functions + r'''
int main() {
    const uint64_t first = legacy_quest_reward_source_id(1, 1);
    assert(first && first <= INT64_MAX);
    assert(first == legacy_quest_reward_source_id(1, 1));
    assert(first != legacy_quest_reward_source_id(1, 2));
    assert(first != legacy_quest_reward_source_id(4, 1));
    assert(!legacy_quest_reward_source_id(0, 1));
    std::set<uint64_t> claims;
    for (uint64_t uid = 1; uid <= 1000; ++uid)
        for (uint32_t reward = 1; reward <= 14; ++reward) {
            const uint64_t source = legacy_quest_reward_source_id(uid, reward);
            assert(source && source <= INT64_MAX && claims.insert(source).second);
        }
    character actor{false, 7, 0, 0, 42};
    character mob{true, 0, 11, 77, 42};
    actor.next = &mob;
    character_list = &actor;
    goal_data feather{'I', 103};
    goal_data branch{'I', 102, &feather};
    goal_data acorn{'I', 101, &branch};
    quest_complete_data completion{&acorn};
    quest_index[0] = {11, &completion};
    object a{1, 101, &actor};
    actor.carrying = object_list = &a;

    // Presenting a partial set leaves custody untouched.
    assert(submit_durable_quest_offering(&mob, &actor, 0, &a));
    assert(submissions == 0 && removed == 0 && actor.carrying == &a);

    object b{2, 102, &actor};
    object c{3, 103, &actor};
    a.next = a.next_content = &b;
    b.next = b.next_content = &c;
    assert(submit_durable_quest_offering(&mob, &actor, 0, &a));
    assert(submissions == 1 && saved_size == sizeof(quest_durable_context));
    assert(removed == 0 && rewards == 0);

    // An interrupted publication retains every item and can retry.
    b.carrier = nullptr;
    assert(!publish_quest_offering(&actor, true, {}, 0, saved_context, saved_size));
    assert(removed == 0 && rewards == 0);
    b.carrier = &actor;
    assert(publish_quest_offering(&actor, true, {}, 0, saved_context, saved_size));
    assert(removed == 3 && actor.carrying == nullptr && dirty == 1);
    complete_quest_offering(&actor, true, {}, 0, saved_context, saved_size);
    assert(rewards == 1 && rewarded_offering_uid == 1);

    // Reconnected inventory omits a previously committed destruction.
    object d{4, 101, &actor};
    object e{5, 102, &actor};
    object f{6, 103, &actor};
    d.next = d.next_content = &e;
    e.next = e.next_content = &f;
    actor.carrying = object_list = &d;
    assert(submit_durable_quest_offering(&mob, &actor, 0, &d));
    actor.carrying = object_list = nullptr;
    assert(publish_quest_offering(&actor, true, {}, 0, saved_context, saved_size));
    complete_quest_offering(&actor, true, {}, 0, saved_context, saved_size);
    assert(rewards == 2 && removed == 3 && rewarded_offering_uid == 4);
}
'''

with tempfile.TemporaryDirectory(prefix="duris-quest-offering-") as directory:
    cpp = Path(directory) / "quest.cpp"
    exe = Path(directory) / "quest.exe"
    cpp.write_text(program, encoding="utf-8")
    subprocess.run(["g++", "-std=c++20", "-O0", str(cpp), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

print("durable quest offering regression passed")
