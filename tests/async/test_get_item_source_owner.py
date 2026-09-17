#!/usr/bin/env python3
"""Execute pickup source resolution against valid and cyclic containment chains."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC, rel

harness = r'''
#include "core/utils.h"
#include "classes/necromancy.h"
#include "item/item_get_policy.h"
#include "item/item_ownership_runtime.h"
#include <cassert>
#include <cstring>
#include <cstdlib>

room_data rooms[1] = {};
P_room world = rooms;
extern const int top_of_world = 0;
int top_of_objt = 4;
static int malformed_messages = 0;
static bool runtime_found = false;
static item_ownership_runtime_entry runtime_entry = {};
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
    std::abort();
}
bool item_ownership_runtime_lookup(uint64_t uid, item_ownership_runtime_entry *entry)
{
    if (!runtime_found || !entry || uid != runtime_entry.item_uid)
        return false;
    *entry = runtime_entry;
    return true;
}
void send_to_char(const char *message, P_char)
{
    assert(!strcmp(message, "That container has a malformed item.\r\n"));
    ++malformed_messages;
}
int main()
{
    pc_only_data player = {};
    player.pid = 42;
    char_data actor = {};
    actor.only.pc = &player;
    actor.in_room = 0;
    rooms[0].number = 100;
    obj_data item = {};
    obj_data containers[7] = {};
    item_owner_identity owner = {};
    item.loc_p = LOC_INSIDE;
    item.loc.inside = containers;

    // A room root at the traversal limit remains valid.
    for (int i = 0; i < 5; ++i)
    {
        containers[i].loc_p = LOC_INSIDE;
        containers[i].loc.inside = &containers[i + 1];
    }
    containers[5].loc_p = LOC_ROOM;
    containers[5].loc.room = 0;
    assert(item_get_source_owner(&actor, &item, containers, &owner));
    assert(owner.type == item_owner_type::room && owner.id == 100);
    assert(malformed_messages == 0);

    // One additional hop must fail closed rather than reading indefinitely.
    containers[5].loc_p = LOC_INSIDE;
    containers[5].loc.inside = &containers[6];
    containers[6].loc_p = LOC_ROOM;
    containers[6].loc.room = 0;
    assert(!item_get_source_owner(&actor, &item, containers, &owner));
    assert(malformed_messages == 1 && !item_owner_identity_valid(owner));

    containers[0].loc.inside = &containers[0];
    assert(!item_get_source_owner(&actor, &item, containers, &owner));
    assert(malformed_messages == 2);
    containers[0].loc.inside = &containers[1];
    containers[1].loc.inside = &containers[0];
    assert(!item_get_source_owner(&actor, &item, containers, &owner));
    assert(malformed_messages == 3);

    containers[0].loc_p = LOC_CARRIED;
    containers[0].loc.carrying = &actor;
    assert(item_get_source_owner(&actor, &item, containers, &owner));
    assert(owner.type == item_owner_type::player && owner.id == 42);

    // A recorded player owner must agree with the live floor placement.
    item.obj_uid = 77;
    item.loc_p = LOC_ROOM;
    item.loc.room = 0;
    runtime_entry = {};
    runtime_entry.item_uid = item.obj_uid;
    runtime_entry.owner = { item_owner_type::player, 42, 0 };
    runtime_entry.state = item_custody_state::active;
    runtime_found = true;
    assert(!item_get_source_owner(&actor, &item, nullptr, &owner));
    runtime_entry.owner = { item_owner_type::room, 100, 0 };
    assert(item_get_source_owner(&actor, &item, nullptr, &owner));
    assert(owner.type == item_owner_type::room && owner.id == 100);

    // A matching player placement remains eligible, while inactive custody does not.
    item.loc_p = LOC_CARRIED;
    item.loc.carrying = &actor;
    runtime_entry.owner = { item_owner_type::player, 42, 0 };
    assert(item_get_source_owner(&actor, &item, nullptr, &owner));
    runtime_entry.state = item_custody_state::destroyed;
    assert(!item_get_source_owner(&actor, &item, nullptr, &owner));
    runtime_entry.state = item_custody_state::active;

    // Virtual locker custody is an intentional authority boundary, not room custody.
    item.loc_p = LOC_ROOM;
    item.loc.room = 0;
    runtime_entry.owner = { item_owner_type::locker, 9, 10 };
    assert(item_get_source_owner(&actor, &item, nullptr, &owner));
    assert(owner.type == item_owner_type::locker && owner.id == 9 && owner.context_id == 10);

    // NPC custody has no durable source; stale player ownership fails closed.
    char_data npc = {};
    item.loc_p = LOC_CARRIED;
    item.loc.carrying = &npc;
    runtime_entry.owner = { item_owner_type::player, 42, 0 };
    assert(!item_get_source_owner(&actor, &item, nullptr, &owner));

    assert(!item_get_source_owner(nullptr, &item, containers, &owner));
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-get-source-owner-") as directory:
    test_source = Path(directory) / "get_source_owner.cpp"
    binary = Path(directory) / "get_source_owner"
    test_source.write_text(harness)
    subprocess.run(
        ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
         "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
         "-ffunction-sections", "-fdata-sections", "-Isrc", str(test_source),
         rel("item_transfer_command.c"), rel("item/item_get_policy.c"),
         "-Wl,--gc-sections", "-o", str(binary)],
        cwd=ROOT, check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=5)
print("pickup source owner boundary and cycle regressions passed")
