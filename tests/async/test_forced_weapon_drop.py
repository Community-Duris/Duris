#!/usr/bin/env python3
"""Exercise fumble publication and its player-ownership failure boundaries."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC, extract_function, rel


makefile = (SRC / "Makefile").read_text(encoding="utf-8")
fight = (SRC / "fight.c").read_text(encoding="utf-8")
specs = (SRC / "specs.object.c").read_text(encoding="utf-8")

assert "item/forced_weapon_drop.o" in makefile
fumble = fight[fight.index("if (sic == 1") : fight.index("if (IS_GRAPPLED", fight.index("if (sic == 1"))]
critical = extract_function("fight.c", "bool critical_disarm(P_char ch, P_char victim)")
gauntlets = extract_function("specs.object.c", "int fumblegaunts(P_obj obj, P_char ch, int cmd")
for body in (fumble, critical, gauntlets):
    assert "forced_weapon_drop(" in body
assert "obj_to_room(weap" not in fumble
assert "obj_to_room(obj, victim->in_room)" not in critical
assert "obj_to_room(weap" not in gauntlets


harness = r'''
#include "core/utils.h"
#include "item/forced_weapon_drop.h"
#include "item/item_command_policy.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "persistence/persistence_checkpoint.h"
#include "redis/redis_floor_runtime.h"

#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

room_data rooms[2] = {};
P_room world = rooms;
extern const int top_of_world = 1;
P_obj object_list = nullptr;

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
    std::abort();
}

static bool durable = true;
static bool submit_ok = true;
static bool runtime_found = true;
static bool locker_destination = false;
static int submit_calls = 0;
static int floor_calls = 0;
static int dirty_calls = 0;
static int char_light_calls = 0;
static int room_light_calls = 0;
static int act_calls = 0;
static item_ownership_runtime_entry runtime_entry = {};
static item_owner_identity submitted_source = {};
static item_owner_identity submitted_destination = {};
static item_transfer_reason submitted_reason = item_transfer_reason::unknown;
static item_movement_publication_fn submitted_publication = nullptr;
static std::array<uint8_t, ITEM_MOVEMENT_CONTEXT_MAX_BYTES> submitted_context = {};
static size_t submitted_context_size = 0;
static std::vector<std::string> recorded_messages;

bool item_command_uses_durable_ownership(P_obj)
{
    return durable;
}

bool item_command_resolve_drop_destination(P_char actor, item_owner_identity *destination,
                                           item_transfer_reason *reason, int64_t *reason_id)
{
    if (!actor || !destination || !reason || !reason_id)
        return false;
    if (locker_destination)
    {
        *destination = {item_owner_type::locker, 7, 8};
        *reason = item_transfer_reason::locker_deposit;
        *reason_id = 0;
    }
    else
    {
        *destination = {item_owner_type::room,
                        static_cast<uint64_t>(world[actor->in_room].number), 0};
        *reason = item_transfer_reason::player_drop;
        *reason_id = world[actor->in_room].number;
    }
    return true;
}

bool item_ownership_runtime_lookup(uint64_t uid, item_ownership_runtime_entry *entry)
{
    if (!runtime_found || !entry || uid != runtime_entry.item_uid)
        return false;
    *entry = runtime_entry;
    return true;
}

bool item_owner_identity_equal(const item_owner_identity &left,
                               const item_owner_identity &right)
{
    return left.type == right.type && left.id == right.id &&
           left.context_id == right.context_id;
}

const char *item_movement_reject_name(item_movement_reject)
{
    return "test_rejection";
}

bool item_movement_transaction_submit(
    P_char, P_obj, P_obj, const item_owner_identity &from_owner,
    const item_owner_identity &to_owner, item_transfer_reason reason, int64_t,
    item_movement_completion_fn, const void *context, size_t context_size, P_obj,
    item_movement_reject *reject, item_movement_publication_fn publication)
{
    ++submit_calls;
    submitted_source = from_owner;
    submitted_destination = to_owner;
    submitted_reason = reason;
    submitted_publication = publication;
    submitted_context_size = context_size;
    assert(context && context_size <= submitted_context.size());
    std::memcpy(submitted_context.data(), context, context_size);
    if (!submit_ok)
    {
        if (reject)
            *reject = item_movement_reject::pending_conflict;
        return false;
    }
    return true;
}

P_obj unequip_char(P_char actor, int slot, bool)
{
    assert(actor && slot >= 0 && slot < MAX_WEAR);
    P_obj object = actor->equipment[slot];
    assert(object);
    actor->equipment[slot] = nullptr;
    object->loc_p = LOC_NOWHERE;
    object->loc.wearing = nullptr;
    return object;
}

void obj_to_char(P_obj object, P_char actor)
{
    assert(object && actor && OBJ_NOWHERE(object));
    object->loc_p = LOC_CARRIED;
    object->loc.carrying = actor;
    object->next_content = actor->carrying;
    actor->carrying = object;
}

void obj_from_char(P_obj object)
{
    assert(object && OBJ_CARRIED(object));
    P_char actor = object->loc.carrying;
    assert(actor && actor->carrying == object);
    actor->carrying = object->next_content;
    object->next_content = nullptr;
    object->loc_p = LOC_NOWHERE;
    object->loc.carrying = nullptr;
}

void obj_to_room(P_obj object, int room)
{
    assert(object && OBJ_NOWHERE(object));
    object->loc_p = LOC_ROOM;
    object->loc.room = room;
}

void act(const char *message, int, P_char, P_obj, void *, int)
{
    ++act_calls;
    recorded_messages.emplace_back(message ? message : "");
}

void logit(const char *, const char *, ...)
{
}

int char_light(P_char)
{
    ++char_light_calls;
    return 0;
}

int room_light(int, int)
{
    ++room_light_calls;
    return 0;
}

void redis_log_floor_drop(P_obj, int)
{
    ++floor_calls;
}

void mark_player_dirty_components(int, player_component_mask_t)
{
    ++dirty_calls;
}

static pc_only_data player = {};
static char_data actor = {};
static obj_data weapon = {};

static void reset(bool actor_is_npc = false)
{
    durable = true;
    submit_ok = true;
    runtime_found = true;
    locker_destination = false;
    submit_calls = 0;
    floor_calls = 0;
    dirty_calls = 0;
    char_light_calls = 0;
    room_light_calls = 0;
    act_calls = 0;
    submitted_source = {};
    submitted_destination = {};
    submitted_reason = item_transfer_reason::unknown;
    submitted_publication = nullptr;
    submitted_context_size = 0;
    submitted_context.fill(0);
    recorded_messages.clear();

    rooms[0] = {};
    rooms[1] = {};
    rooms[0].number = 100;
    rooms[1].number = 200;
    player = {};
    player.pid = 42;
    actor = {};
    actor.only.pc = &player;
    actor.in_room = 0;
    actor.player.level = 20;
    if (actor_is_npc)
        SET_BIT(actor.specials.act, ACT_ISNPC);
    weapon = {};
    weapon.obj_uid = 9001;
    weapon.type = ITEM_WEAPON;
    weapon.loc_p = LOC_WORN;
    weapon.loc.wearing = &actor;
    actor.equipment[WIELD] = &weapon;
    object_list = &weapon;
    runtime_entry = {};
    runtime_entry.item_uid = weapon.obj_uid;
    runtime_entry.root_item_uid = weapon.obj_uid;
    runtime_entry.owner = {item_owner_type::player, 42, 0};
    runtime_entry.state = item_custody_state::active;
}

static bool publish(bool committed)
{
    assert(submitted_publication && submitted_context_size);
    item_transfer_result result = {};
    return submitted_publication(&actor, committed, result, 0,
                                 submitted_context.data(), submitted_context_size);
}

int main()
{
    reset();
    assert(forced_weapon_drop(&actor, &weapon,
                              forced_weapon_drop_cause::combat_fumble) ==
           forced_weapon_drop_result::pending);
    assert(submit_calls == 1 && OBJ_CARRIED_BY(&weapon, &actor));
    assert(submitted_source.type == item_owner_type::player && submitted_source.id == 42);
    assert(submitted_destination.type == item_owner_type::room &&
           submitted_destination.id == 100);
    assert(submitted_reason == item_transfer_reason::player_drop);
    assert(act_calls == 0 && floor_calls == 0);
    assert(publish(true));
    assert(OBJ_ROOM(&weapon) && weapon.loc.room == 0);
    assert(floor_calls == 1 && dirty_calls == 1 && act_calls == 2);
    const int published_acts = act_calls;
    weapon.loc.room = 1;
    assert(publish(true));
    assert(act_calls == published_acts && floor_calls == 1 && dirty_calls == 1);

    reset();
    submit_ok = false;
    assert(forced_weapon_drop(&actor, &weapon,
                              forced_weapon_drop_cause::combat_fumble) ==
           forced_weapon_drop_result::retained);
    assert(OBJ_CARRIED_BY(&weapon, &actor) && submit_calls == 1);
    assert(act_calls == 2 && floor_calls == 0);

    reset();
    assert(forced_weapon_drop(&actor, &weapon,
                              forced_weapon_drop_cause::combat_fumble) ==
           forced_weapon_drop_result::pending);
    assert(publish(false));
    assert(OBJ_CARRIED_BY(&weapon, &actor));
    assert(act_calls == 2 && floor_calls == 0 && dirty_calls == 0);

    reset();
    runtime_entry.owner.id = 999;
    assert(forced_weapon_drop(&actor, &weapon,
                              forced_weapon_drop_cause::combat_fumble) ==
           forced_weapon_drop_result::rejected);
    assert(OBJ_WORN_BY(&weapon, &actor) && submit_calls == 0 && act_calls == 2);

    reset();
    assert(forced_weapon_drop(&actor, &weapon,
                              forced_weapon_drop_cause::critical_disarm) ==
           forced_weapon_drop_result::pending);
    assert(OBJ_CARRIED_BY(&weapon, &actor) && act_calls == 0);
    assert(publish(true));
    assert(OBJ_ROOM(&weapon) && act_calls == 0);

    reset();
    locker_destination = true;
    assert(forced_weapon_drop(&actor, &weapon,
                              forced_weapon_drop_cause::combat_fumble) ==
           forced_weapon_drop_result::pending);
    assert(publish(true));
    assert(OBJ_ROOM(&weapon) && floor_calls == 0);

    reset();
    durable = false;
    assert(forced_weapon_drop(&actor, &weapon,
                              forced_weapon_drop_cause::combat_fumble) ==
           forced_weapon_drop_result::dropped);
    assert(OBJ_ROOM(&weapon) && submit_calls == 0 && act_calls == 2);

    reset(true);
    assert(forced_weapon_drop(&actor, &weapon,
                              forced_weapon_drop_cause::combat_fumble) ==
           forced_weapon_drop_result::dropped);
    assert(OBJ_ROOM(&weapon) && submit_calls == 0);

    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-forced-weapon-drop-") as directory:
    test_source = Path(directory) / "forced_weapon_drop.cpp"
    binary = Path(directory) / "forced_weapon_drop"
    test_source.write_text(harness, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc",
            str(test_source),
            rel("item/forced_weapon_drop.c"),
            "-Wl,--gc-sections",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=5)

print("forced weapon drop ownership and publication regressions passed")
