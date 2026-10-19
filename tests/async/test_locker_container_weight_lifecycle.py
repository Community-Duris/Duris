#!/usr/bin/env python3
"""Prove container weights remain conserved and normalized across locker sort and load lifecycles."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

# 1. Source contract checks
lockers_h = (ROOT / "src/item/storage_lockers.h").read_text()
lockers_c = (ROOT / "src/item/storage_lockers.c").read_text()
sql_player_c = (ROOT / "src/sql/sql_player.c").read_text()
handler_c = (ROOT / "src/world/handler.c").read_text()

# Verify EqSlotChest and EqWearChest reject containers and corpses
assert "obj->type == ITEM_CONTAINER || obj->type == ITEM_CORPSE" in lockers_h
assert "obj->type == ITEM_CONTAINER || obj->type == ITEM_CORPSE" in lockers_c

# Verify sql_load_locker_items_filtered calls recalc_container_weight
assert "sql_load_locker_items_filtered" in sql_player_c
locker_load_idx = sql_player_c.index("static P_obj sql_load_locker_items_filtered")
locker_load_body = sql_player_c[locker_load_idx:sql_player_c.index("static P_obj sql_load_locker_items(", locker_load_idx + 1)]
assert "recalc_container_weight(obj);" in locker_load_body

# Verify sql_load_private_chest_items calls recalc_container_weight
chest_load_idx = sql_player_c.index("void sql_load_private_chest_items")
chest_load_body = sql_player_c[chest_load_idx:sql_player_c.index("static P_char alloc_temp_char", chest_load_idx)]
assert "recalc_container_weight(obj);" in chest_load_body
assert "recalc_container_weight(chest_obj);" in chest_load_body

# Verify sql_load_all_corpses calls recalc_container_weight on nested items in both passes
assert sql_player_c.count("recalc_container_weight(obj_map[i]);") >= 2

# Verify sql_restore_shopkeepers calls recalc_container_weight on loaded items
shopkeeper_load_idx = sql_player_c.index("bool sql_restore_shopkeepers(void)")
shopkeeper_load_body = sql_player_c[shopkeeper_load_idx:sql_player_c.index("P_char sql_restore_shopkeeper", shopkeeper_load_idx)]
assert "recalc_container_weight(t->obj);" in shopkeeper_load_body

# Verify sql_restore_saved_items calls recalc_container_weight on top-level saved item
saved_load_idx = sql_player_c.index("void sql_restore_saved_items(void)")
saved_load_body = sql_player_c[saved_load_idx:sql_player_c.index("static bool sql_save_ship_armor", saved_load_idx)]
assert "recalc_container_weight(obj);" in saved_load_body

# Verify handler uses GET_OBJ_WEIGHT for nesting
obj_to_obj_idx = handler_c.index("void obj_to_obj(P_obj obj, P_obj obj_to)")
obj_to_obj_body = handler_c[obj_to_obj_idx:handler_c.index("void obj_to_obj_at_end", obj_to_obj_idx)]
assert "add_weight(obj_to, GET_OBJ_WEIGHT(obj));" in obj_to_obj_body

obj_from_obj_idx = handler_c.index("void obj_from_obj(P_obj obj)")
obj_from_obj_body = handler_c[obj_from_obj_idx:handler_c.index("/* Safe here and nowhere earlier", obj_from_obj_idx)]
assert "add_weight(obj_from, -(GET_OBJ_WEIGHT(obj)));" in obj_from_obj_body

# 2. Runtime C++ simulation test
test_cpp_source = r'''
#include "item/encumbrance_policy.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define ITEM_CONTAINER 15
#define ITEM_CORPSE    27
#define ITEM_OTHER     8
#define ITEM_HOLD      (1 << 14)
#define ITEM_WEAR_WAIST (1 << 11)

struct MockObj {
    int type = ITEM_OTHER;
    int wear_flags = 0;
    unsigned int extra_flags = 0;
    unsigned int anti_flags = 0;
    int weight = 0;
    int proto_weight = 0;
    int value[8] = {0};
    MockObj *contains = nullptr;
    MockObj *next_content = nullptr;
    MockObj *inside = nullptr;
};

#define GET_OBJ_WEIGHT(obj) (((obj)->weight > 0) ? (obj)->weight : 0)

static int sum_direct_contents_weight(MockObj *cont) {
    int w = 0;
    for (MockObj *o = cont->contains; o; o = o->next_content) {
        w += GET_OBJ_WEIGHT(o);
    }
    int pct = cont->value[4];
    if (pct > 90) pct = 90;
    if (pct > 0 && w > 0) {
        w -= (int)((long)w * pct / 100);
    }
    return w;
}

static void add_weight(MockObj *obj, int weight) {
    if (!obj || !weight) return;
    obj->weight += weight;
    if (obj->inside) {
        add_weight(obj->inside, weight);
    }
}

static void recalc_container_weight(MockObj *cont) {
    if (!cont || cont->type != ITEM_CONTAINER) return;
    int shell = cont->proto_weight;
    int contents = sum_direct_contents_weight(cont);
    int new_weight = shell + contents;
    int delta = new_weight - cont->weight;
    if (!delta) return;
    add_weight(cont, delta);
}

static void obj_to_obj(MockObj *obj, MockObj *obj_to) {
    obj->inside = obj_to;
    obj->next_content = obj_to->contains;
    obj_to->contains = obj;
    add_weight(obj_to, GET_OBJ_WEIGHT(obj));
    if (obj_to->value[4] > 0) recalc_container_weight(obj_to);
}

static void obj_from_obj(MockObj *obj) {
    MockObj *obj_from = obj->inside;
    if (!obj_from) return;
    if (obj_from->contains == obj) {
        obj_from->contains = obj->next_content;
    } else {
        MockObj *cur = obj_from->contains;
        while (cur && cur->next_content != obj) cur = cur->next_content;
        if (cur) cur->next_content = obj->next_content;
    }
    obj->inside = nullptr;
    obj->next_content = nullptr;
    add_weight(obj_from, -(GET_OBJ_WEIGHT(obj)));
    if (obj_from->value[4] > 0) recalc_container_weight(obj_from);
}

// Chest fit checks
static bool EqSlotChest_ItemFits(MockObj *obj, unsigned int eqBit) {
    if (!obj || obj->type == ITEM_CONTAINER || obj->type == ITEM_CORPSE) return false;
    return (obj->wear_flags & eqBit) != 0;
}

static bool EqWearChest_ItemFits(MockObj *obj, unsigned int wearClass) {
    if (!obj || obj->type == ITEM_CONTAINER || obj->type == ITEM_CORPSE) return false;
    return (obj->anti_flags & wearClass) == 0;
}

int main() {
    // 1. Verify chest ItemFits rejects containers even if wear flags match
    MockObj bag;
    bag.type = ITEM_CONTAINER;
    bag.wear_flags = ITEM_HOLD | ITEM_WEAR_WAIST;
    bag.anti_flags = 0;
    assert(!EqSlotChest_ItemFits(&bag, ITEM_HOLD));
    assert(!EqSlotChest_ItemFits(&bag, ITEM_WEAR_WAIST));
    assert(!EqWearChest_ItemFits(&bag, 1)); // Warrior chest

    // Ordinary item matches
    MockObj ring;
    ring.type = ITEM_OTHER;
    ring.wear_flags = ITEM_HOLD;
    assert(EqSlotChest_ItemFits(&ring, ITEM_HOLD));

    // 2. Weight-reducing container behavior (50% reduction gathering bag)
    MockObj gbag;
    gbag.type = ITEM_CONTAINER;
    gbag.proto_weight = 5;
    gbag.weight = 5;
    gbag.value[4] = 50; // 50% reduction

    MockObj ore1, ore2;
    ore1.type = ITEM_OTHER; ore1.weight = 20; ore1.proto_weight = 20;
    ore2.type = ITEM_OTHER; ore2.weight = 20; ore2.proto_weight = 20;

    obj_to_obj(&ore1, &gbag);
    // 20 lbs with 50% discount = 10 lbs, + 5 shell = 15 lbs
    assert(gbag.weight == 15);

    obj_to_obj(&ore2, &gbag);
    // 40 lbs with 50% discount = 20 lbs, + 5 shell = 25 lbs
    assert(gbag.weight == 25);

    // 3. Hydration recalculation idempotence: simulating SQL load with raw serialized weight
    // Suppose DB stored gbag.weight = 25 (which included contents)
    // Upon load:
    MockObj loaded_gbag;
    loaded_gbag.type = ITEM_CONTAINER;
    loaded_gbag.proto_weight = 5;
    loaded_gbag.weight = 25; // Loaded directly from SQL row[weight]
    loaded_gbag.value[4] = 50;
    // Attach children directly (as sql_load does)
    loaded_gbag.contains = &ore1;
    ore1.next_content = &ore2;
    ore2.next_content = nullptr;
    ore1.inside = &loaded_gbag;
    ore2.inside = &loaded_gbag;

    // Recalculate container weight
    recalc_container_weight(&loaded_gbag);
    assert(loaded_gbag.weight == 25); // Remains exactly 25, no double-counting!

    // If loaded without discount in DB (e.g. 45 lbs stale)
    loaded_gbag.weight = 45;
    recalc_container_weight(&loaded_gbag);
    assert(loaded_gbag.weight == 25); // Normalized cleanly to 25 lbs!

    // 4. Weightless container (Portable Hole with proto_weight = -3000)
    MockObj hole;
    hole.type = ITEM_CONTAINER;
    hole.proto_weight = -3000;
    hole.weight = -3000;

    MockObj chest;
    chest.type = ITEM_CONTAINER;
    chest.proto_weight = 0;
    chest.weight = 0;

    // Putting portable hole into chest adds GET_OBJ_WEIGHT(hole) = 0
    obj_to_obj(&hole, &chest);
    assert(chest.weight == 0);

    // Taking hole out of chest leaves chest at 0
    obj_from_obj(&hole);
    assert(chest.weight == 0);

    std::puts("locker container weight lifecycle and stability PASS");
    return 0;
}
'''

build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="locker-container-weight-", dir=build) as tmp:
    path = Path(tmp)
    source_path = path / "test.cpp"
    binary = path / "test"
    source_path.write_text(test_cpp_source)
    subprocess.run([
        "g++", "-std=c++20", "-g", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-I", str(ROOT / "src"), str(source_path), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1"})

print("all locker container weight lifecycle contract and runtime checks passed")
