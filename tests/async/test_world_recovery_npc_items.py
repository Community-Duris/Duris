#!/usr/bin/env python3
"""Runtime regression for idempotent recovered-NPC zone item rehydration."""

from _paths import rel
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "world_recovery_npc_items.h"
#include "prototypes.h"
#include "utils.h"

#include <cassert>
#include <cstdarg>

index_data mob_indexes[2] = {};
index_data object_indexes[10] = {};
P_index mob_index = mob_indexes;
P_index obj_index = object_indexes;
room_data rooms[2] = {};
room_data *world = rooms;
zone_data zones[1] = {};
zone_data *zone_table = zones;
int top_of_mobt = 1;
int top_of_objt = 9;
int top_of_world = 1;
int top_of_zone_table = 0;
P_obj object_list = nullptr;

void logit(const char *, const char *, ...)
{
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
    std::abort();
}

int get_property(const char *, int fallback)
{
    return fallback;
}

bool get_artifact_data_sql(int, P_arti)
{
    return false;
}

int itemvalue(P_obj)
{
    return 1;
}

bool item_load_check(P_obj, int, int zone_percent)
{
    return zone_percent > 0;
}

void enhance_on_npc_item_reset_skipped(P_char, P_obj)
{
}

int shop_keeper(P_char, P_char, int, char *)
{
    return 0;
}

P_obj read_object(int object_rnum, int)
{
    if (object_rnum < 0 || object_rnum > top_of_objt)
        return nullptr;
    P_obj object = new obj_data{};
    object->R_num = object_rnum;
    object->loc_p = LOC_NOWHERE;
    object->next = object_list;
    object_list = object;
    ++obj_index[object_rnum].number;
    return object;
}

void obj_to_char(P_obj object, P_char mob)
{
    object->loc_p = LOC_CARRIED;
    object->loc.carrying = mob;
    object->next_content = mob->carrying;
    mob->carrying = object;
}

void equip_char(P_char mob, P_obj object, int slot, int)
{
    object->loc_p = LOC_WORN;
    object->loc.wearing = mob;
    mob->equipment[slot] = object;
}

void extract_obj(P_obj object, int)
{
    if (object_list == object)
        object_list = object->next;
    else
        for (P_obj prior = object_list; prior; prior = prior->next)
            if (prior->next == object)
            {
                prior->next = object->next;
                break;
            }
    --obj_index[object->R_num].number;
    delete object;
}

size_t carried_count(P_char mob, int object_rnum)
{
    size_t count = 0;
    for (P_obj object = mob->carrying; object; object = object->next_content)
        if (object->R_num == object_rnum)
            ++count;
    return count;
}

int main()
{
    rooms[0].number = 29201;
    rooms[1].number = 29206;
    mob_indexes[0].virtual_number = 29201;
    mob_indexes[0].qst_func = shop_keeper;
    mob_indexes[1].virtual_number = 29236;
    for (int index = 0; index <= top_of_objt; ++index)
        object_indexes[index].virtual_number = 30000 + index;

    reset_com commands[] = {
        {'M', false, 0, 1, 0, 100},
        {'G', true, 0, 999, 0, 100},
        {'G', true, 0, 999, 0, 100},
        {'G', true, 1, 999, 0, 100},
        {'G', true, 2, 999, 0, 100},
        {'G', true, 3, 999, 0, 100},
        {'G', true, 4, 999, 0, 100},
        {'G', true, 5, 999, 0, 100},
        {'G', true, 6, 999, 0, 100},
        {'M', false, 1, 1, 1, 100},
        {'E', true, 7, 1, 12, 100},
        {'G', true, 8, 1, 0, 100},
        {'S', false, 0, 0, 0, 0},
    };
    zones[0].cmd = commands;

    npc_only_data keeper_npc = {};
    npc_only_data guard_npc = {};
    char_data keeper = {};
    char_data guard = {};
    keeper.only.npc = &keeper_npc;
    guard.only.npc = &guard_npc;
    keeper_npc.R_num = 0;
    guard_npc.R_num = 1;
    keeper.specials.act = ACT_ISNPC;
    guard.specials.act = ACT_ISNPC;
    keeper.player.birthplace = rooms[0].number;
    guard.player.birthplace = rooms[1].number;
    keeper.in_room = 1; // moved since spawning; birthplace must drive association
    guard.in_room = 0;
    P_char recovered[] = {&keeper, &guard};

    assert(world_recovery_rehydrate_npc_items(recovered, 2));
    assert(carried_count(&keeper, 0) == 2);
    for (int object_rnum = 1; object_rnum <= 6; ++object_rnum)
        assert(carried_count(&keeper, object_rnum) == 1);
    assert(guard.equipment[12] && guard.equipment[12]->R_num == 7);
    assert(carried_count(&guard, 8) == 1);

    const int counts_after_first[] = {
        obj_index[0].number, obj_index[1].number, obj_index[2].number,
        obj_index[3].number, obj_index[4].number, obj_index[5].number,
        obj_index[6].number, obj_index[7].number, obj_index[8].number,
    };
    assert(world_recovery_rehydrate_npc_items(recovered, 2));
    for (int object_rnum = 0; object_rnum <= 8; ++object_rnum)
        assert(obj_index[object_rnum].number == counts_after_first[object_rnum]);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-world-npc-items-") as temp_dir:
    source = Path(temp_dir) / "world_recovery_npc_items_test.cpp"
    binary = Path(temp_dir) / "world_recovery_npc_items_test"
    source.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-Isrc",
            str(source),
            rel("world_recovery_npc_items.c"),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("recovered NPC zone items rehydrate once with duplicate stock and equipment intact")
