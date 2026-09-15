#!/usr/bin/env python3
"""Regression test for mixed-owner bulk get source selection.

A player can leave a durable item in the room through an NPC or pet path
without a corresponding room transfer.  A later ``get all`` may therefore
select that player-owned item beside ordinary room stock.  The source owner
must come from the runtime custody record, not from whichever root happens to
be first in the scan order.
"""

from pathlib import Path
import subprocess
import tempfile

from _paths import SRC


source = (SRC / "actobj.c").read_text(encoding="utf-8")
signature = "static bool bulk_get_source_for_roots("
start = source.index(signature)
brace = source.index("{", start)
depth = 1
end = brace + 1
while depth:
    depth += (source[end] == "{") - (source[end] == "}")
    end += 1
function = source[start:end]

prelude = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

enum class item_owner_type : uint8_t {
    unknown = 0, player, container, room, corpse, locker, auction,
    system, destruction, shopkeeper
};
struct item_owner_identity {
    item_owner_type type = item_owner_type::unknown;
    uint64_t id = 0;
    uint64_t context_id = 0;
};
struct item_ownership_runtime_entry { item_owner_identity owner; };
struct char_data { uint32_t pid = 42; bool pc = true; };
struct obj_data { uint64_t obj_uid = 0; };
using P_char = char_data *;
using P_obj = obj_data *;

static std::unordered_map<uint64_t, item_ownership_runtime_entry> runtime_catalog;
static bool fallback_called = false;

static bool item_ownership_runtime_lookup(
    uint64_t uid, item_ownership_runtime_entry *entry)
{
    const auto found = runtime_catalog.find(uid);
    if (found == runtime_catalog.end())
        return false;
    *entry = found->second;
    return true;
}

static bool item_owner_identity_equal(const item_owner_identity &left,
                                      const item_owner_identity &right)
{
    return left.type == right.type && left.id == right.id &&
           left.context_id == right.context_id;
}

static bool item_owner_identity_valid(const item_owner_identity &owner)
{
    return owner.type != item_owner_type::unknown && owner.id != 0;
}

static bool get_item_source_owner(P_char, P_obj, P_obj, item_owner_identity *source)
{
    fallback_called = true;
    *source = { item_owner_type::room, 9001, 0 };
    return true;
}
'''

driver = r'''
int main()
{
    char_data actor;
    obj_data ordinary_room_item{ 1 };
    obj_data pet_dropped_item{ 2 };
    item_owner_identity source = {};

    // The ordinary room object is first, but the selected durable item is
    // still authoritative for the player who dropped it through a pet.
    runtime_catalog.emplace(
        pet_dropped_item.obj_uid,
        item_ownership_runtime_entry{ { item_owner_type::player, actor.pid, 0 } });
    assert(bulk_get_source_for_roots(
               &actor, nullptr, { &ordinary_room_item, &pet_dropped_item }, &source));
    assert(source.type == item_owner_type::player && source.id == actor.pid);
    assert(!fallback_called);

    // A genuinely mixed-owner selection cannot be represented by one atomic
    // player transfer and must be rejected instead of silently choosing one.
    obj_data other_owner_item{ 3 };
    runtime_catalog.emplace(
        other_owner_item.obj_uid,
        item_ownership_runtime_entry{ { item_owner_type::room, 9001, 0 } });
    assert(!bulk_get_source_for_roots(
        &actor, nullptr, { &pet_dropped_item, &other_owner_item }, &source));

    // With no runtime records, preserve the existing physical-source fallback.
    runtime_catalog.clear();
    fallback_called = false;
    obj_data unregistered_item{ 4 };
    assert(bulk_get_source_for_roots(&actor, nullptr, { &unregistered_item }, &source));
    assert(fallback_called && source.type == item_owner_type::room && source.id == 9001);

    std::puts("bulk get source selection runtime: ok");
}
'''

with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    cpp = path / "bulk_get_source_selection.cpp"
    binary = path / "bulk_get_source_selection"
    cpp.write_text(prelude + function + driver, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
         "-fsanitize=address,undefined", "-g", str(cpp), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)
