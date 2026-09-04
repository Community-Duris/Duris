#!/usr/bin/env python3
"""Execute pickup source resolution against valid and cyclic containment chains."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC, rel

source = (SRC / "actobj.c").read_text()
start = source.index("static bool get_item_source_owner(")
resolver = source[start:source.index("enum class coin_debit_action", start)]

harness = r'''
#include "core/utils.h"
#include "classes/necromancy.h"
#include "item/item_ownership_runtime.h"
#include <cassert>
#include <cstring>
#include <cstdlib>

room_data rooms[1] = {};
P_room world = rooms;
int top_of_objt = 4;
static int malformed_messages = 0;
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
    std::abort();
}
bool item_ownership_runtime_lookup(uint64_t, item_ownership_runtime_entry *)
{
    return false;
}
void send_to_char(const char *message, P_char)
{
    assert(!strcmp(message, "That container has a malformed item.\r\n"));
    ++malformed_messages;
}
''' + resolver + r'''
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

    // A room root at the traversal limit remains valid.
    for (int i = 0; i < 5; ++i)
    {
        containers[i].loc_p = LOC_INSIDE;
        containers[i].loc.inside = &containers[i + 1];
    }
    containers[5].loc_p = LOC_ROOM;
    containers[5].loc.room = 0;
    assert(get_item_source_owner(&actor, &item, containers, &owner));
    assert(owner.type == item_owner_type::room && owner.id == 100);
    assert(malformed_messages == 0);

    // One additional hop must fail closed rather than reading indefinitely.
    containers[5].loc_p = LOC_INSIDE;
    containers[5].loc.inside = &containers[6];
    containers[6].loc_p = LOC_ROOM;
    containers[6].loc.room = 0;
    assert(!get_item_source_owner(&actor, &item, containers, &owner));
    assert(malformed_messages == 1 && !item_owner_identity_valid(owner));

    containers[0].loc.inside = &containers[0];
    assert(!get_item_source_owner(&actor, &item, containers, &owner));
    assert(malformed_messages == 2);
    containers[0].loc.inside = &containers[1];
    containers[1].loc.inside = &containers[0];
    assert(!get_item_source_owner(&actor, &item, containers, &owner));
    assert(malformed_messages == 3);

    containers[0].loc_p = LOC_CARRIED;
    containers[0].loc.carrying = &actor;
    assert(get_item_source_owner(&actor, &item, containers, &owner));
    assert(owner.type == item_owner_type::player && owner.id == 42);
    assert(!get_item_source_owner(nullptr, &item, containers, &owner));
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
         rel("item_transfer_command.c"), "-Wl,--gc-sections", "-o", str(binary)],
        cwd=ROOT, check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=5)
print("pickup source owner boundary and cycle regressions passed")
