#!/usr/bin/env python3
"""Prove container weights remain conserved and normalized across locker sort and load lifecycles."""
from pathlib import Path
from _paths import extract_function
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

# Include the complete production weight implementation, including its local helpers.
weight_function = extract_function("world/handler.c", "void add_weight(P_obj obj, int weight)\n{")
weight_start = handler_c.index("// A little more complicated than it would first seem.")
weight_code = handler_c[weight_start:handler_c.index(weight_function) + len(weight_function)]

# Compile production movement and sorting; unrelated world services are stubbed.
handler = "\n".join(
    weight_code if signature.startswith("void add_weight") else
    extract_function("world/handler.c", signature) for signature in (
    "static bool obj_is_container_type(",
    "static int obj_prototype_weight(",
    "static int container_weight_reduction_pct(",
    "static int sum_direct_contents_weight(",
    "static void encumbrance_adjust(",
    "void recalc_container_weight(",
    "static void resync_reducing_container(",
    "void add_weight(P_obj obj, int weight)\n{",
    "bool obj_can_nest(",
    "bool obj_is_in_container(",
    "void obj_to_obj(",
    "static void append_obj_to_list(",
    "void obj_to_obj_at_end(",
    "void obj_from_obj(",
))
chests = "\n".join(extract_function("item/storage_lockers.c", signature) for signature in (
    "bool locker_eq_type_fits_for_storage(",
    "bool ComboChest::ItemFits(",
    "bool EqApplyChest::ItemFits(",
    "bool EqAffectChest::ItemFits(",
))
harness = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/files.h"
#include "item/storage_lockers.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

void world_activity_object_enter(P_obj) {}
void world_activity_object_leave(P_obj) {}
int top_of_objt = 1000;
static index_data indices[10] = {};
P_index obj_index = indices;
static int prototype_weights[] = {0, 5, -3000, 20, 10, 4};

int real_object(const int vnum) { return vnum; }
P_obj read_object(int rnum, int) {
    assert(rnum >= 0 && rnum < 6);
    P_obj obj = new obj_data{};
    obj->R_num = rnum;
    obj->type = rnum == 3 ? ITEM_TREASURE : ITEM_CONTAINER;
    obj->weight = prototype_weights[rnum];
    obj->loc_p = LOC_NOWHERE;
    return obj;
}
void extract_obj(P_obj obj, int) {
    while (obj->contains) {
        P_obj child = obj->contains;
        obj->contains = child->next_content;
        extract_obj(child, TRUE);
    }
    delete obj;
}
void logit(const char *, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
char *str_dup(const char *) { std::abort(); }
P_char training_dummy_item_owner(P_obj) { return nullptr; }
void obj_to_room(P_obj, int) { std::abort(); }
static void mark_container_dirty(P_obj) {}
void artifact_update_location_sql(P_obj) {}
void writeSavedItem(P_obj) { std::abort(); }
void add_weight(P_obj, int);

LockerChest::~LockerChest() {}
void LockerChest::FillExtraDescBuf(char *) {}
ComboChest::~ComboChest() {}
void ComboChest::FillExtraDescBuf(char *) {}

// PRODUCTION

struct SlotChest : EqSlotChest { SlotChest() : EqSlotChest(ITEM_HOLD, "held", "held") {} };
struct WearChest : EqWearChest { WearChest() : EqWearChest(1, "warrior", "warrior") {} };
struct ApplyChest : EqApplyChest { ApplyChest() : EqApplyChest(APPLY_STR, "str", "str") {} };
struct AffectChest : EqAffectChest { AffectChest() : EqAffectChest(1, 1, "affect", "affect") {} };
struct TestOreChest : OreChest { TestOreChest() : OreChest("", "ore", "ore") {} };
struct TestComboChest : ComboChest { TestComboChest() : ComboChest(nullptr) {} };

static void sorting() {
    SlotChest slot;
    WearChest wear;
    ApplyChest apply;
    AffectChest affect;
    TestOreChest ore;
    TestComboChest combo;
    LockerChest *chests[] = {&slot, &wear, &apply, &affect, &ore, &combo};
    char name[] = "test_ore_";
    obj_data obj = {};
    obj.name = name;
    obj.wear_flags = ITEM_HOLD;
    obj.affected[0].location = APPLY_STR;
    obj.affected[0].modifier = 1;
    obj.bitvector = 1;
    for (int type : {ITEM_CONTAINER, ITEM_CORPSE}) {
        obj.type = type;
        for (LockerChest *chest : chests) assert(!chest->ItemFits(&obj));
    }
    obj.type = ITEM_TREASURE;
    for (LockerChest *chest : chests) {
        assert(chest->ItemFits(&obj));
        assert(!chest->ItemFits(nullptr));
    }
}

static void movement(bool append) {
    obj_data chest = {};
    chest.type = ITEM_CONTAINER;
    chest.R_num = 4;
    chest.weight = 10;
    char_data carrier = {};
    chest.loc_p = LOC_CARRIED;
    chest.loc.carrying = &carrier;
    GET_CARRYING_W(&carrier) = 10;
    P_obj hole = read_object(2, REAL);
    P_obj item = read_object(3, REAL);
    auto put = append ? obj_to_obj_at_end : obj_to_obj;
    for (int i = 0; i < 50; ++i) {
        put(hole, &chest);
        assert(chest.weight == 10 && GET_CARRYING_W(&carrier) == 10);
        put(item, &chest);
        assert(chest.weight == 30 && GET_CARRYING_W(&carrier) == 30);
        obj_from_obj(hole);
        assert(chest.weight == 30 && GET_CARRYING_W(&carrier) == 30);
        obj_from_obj(item);
        assert(chest.weight == 10 && GET_CARRYING_W(&carrier) == 10);
    }
    extract_obj(hole, TRUE);
    extract_obj(item, TRUE);
}

static void worn_movement(bool append, bool nested) {
    auto put = append ? obj_to_obj_at_end : obj_to_obj;
    for (int reduction : {0, 50}) {
        // Model the four-pound #494 gathering bag, directly worn or inside
        // another worn container. Preserve eleven pounds of unrelated load.
        char_data carrier = {};
        obj_data outer = {};
        outer.type = ITEM_CONTAINER;
        outer.R_num = 4;
        outer.weight = 10;
        outer.loc_p = LOC_WORN;
        outer.loc.wearing = &carrier;
        P_obj bag = read_object(5, REAL);
        bag->value[4] = reduction;
        bag->loc_p = LOC_WORN;
        bag->loc.wearing = &carrier;
        GET_CARRYING_W(&carrier) = 11 + (nested ? 5 : 2);
        if (nested) {
            bag->loc_p = LOC_NOWHERE;
            put(bag, &outer);
        }
        auto check = [&](int count) {
            const int bag_weight = 4 + count * 2 - count * 2 * reduction / 100;
            const int worn_weight = (nested ? 10 : 0) + bag_weight;
            assert(bag->weight == bag_weight);
            if (nested) assert(outer.weight == worn_weight);
            assert(GET_CARRYING_W(&carrier) == 11 + worn_weight / 2);
        };
        for (int cycle = 0; cycle < 50; ++cycle) {
            P_obj items[3];
            for (int i = 0; i < 3; ++i) {
                items[i] = read_object(3, REAL);
                items[i]->weight = 2;
                put(items[i], bag);
                check(i + 1);
            }
            for (int i = 2; i >= 0; --i) {
                obj_from_obj(items[i]);
                check(i);
                extract_obj(items[i], TRUE);
            }
        }
        if (nested) {
            obj_from_obj(bag);
            assert(outer.weight == 10 && GET_CARRYING_W(&carrier) == 16);
        }
        extract_obj(bag, TRUE);
    }
}

static void weight_changes(bool worn) {
    char_data carrier = {};
    obj_data obj = {};
    obj.weight = -3;
    obj.loc_p = worn ? LOC_WORN : LOC_CARRIED;
    if (worn) obj.loc.wearing = &carrier;
    else obj.loc.carrying = &carrier;
    GET_CARRYING_W(&carrier) = 11;
    for (int cycle = 0; cycle < 50; ++cycle) {
        int expected = -3;
        for (int delta : {2, 2, 1, 1, -1, -1, -2, -2, 4, -4}) {
            expected += delta;
            add_weight(&obj, delta);
            assert(obj.weight == expected);
            assert(GET_CARRYING_W(&carrier) == 11 + (expected > 0 ? expected : 0) / (worn ? 2 : 1));
        }
        assert(obj.weight == -3 && GET_CARRYING_W(&carrier) == 11);
    }
    add_weight(nullptr, 1);
    add_weight(&obj, 0);
    assert(obj.weight == -3 && GET_CARRYING_W(&carrier) == 11);
}

static void worn_recalculation() {
    char_data carrier = {};
    P_obj bag = read_object(5, REAL);
    bag->loc_p = LOC_WORN;
    bag->loc.wearing = &carrier;
    for (int stale : {7, 1, -3}) {
        bag->weight = stale;
        GET_CARRYING_W(&carrier) = 11 + (stale > 0 ? stale : 0) / 2;
        recalc_container_weight(bag);
        assert(bag->weight == 4 && GET_CARRYING_W(&carrier) == 13);
        recalc_container_weight(bag);
        assert(bag->weight == 4 && GET_CARRYING_W(&carrier) == 13);
    }
    extract_obj(bag, TRUE);

    // Loaded children are linked before their serialized container is normalized.
    bag = read_object(1, REAL);
    bag->value[4] = 50;
    bag->contains = read_object(3, REAL);
    bag->contains->next_content = read_object(3, REAL);
    for (P_obj child = bag->contains; child; child = child->next_content) {
        child->loc_p = LOC_INSIDE;
        child->loc.inside = bag;
    }
    for (int stale : {25, 45}) {
        bag->weight = stale;
        recalc_container_weight(bag);
        assert(bag->weight == 25);
        recalc_container_weight(bag);
        assert(bag->weight == 25);
    }
    extract_obj(bag, TRUE);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    if (!strcmp(argv[1], "sorting")) sorting();
    else if (!strcmp(argv[1], "insert")) movement(false);
    else if (!strcmp(argv[1], "append")) movement(true);
    else if (!strcmp(argv[1], "worn-insert")) worn_movement(false, false);
    else if (!strcmp(argv[1], "worn-append")) worn_movement(true, false);
    else if (!strcmp(argv[1], "worn-nested-insert")) worn_movement(false, true);
    else if (!strcmp(argv[1], "worn-nested-append")) worn_movement(true, true);
    else if (!strcmp(argv[1], "worn-weight-changes")) weight_changes(true);
    else if (!strcmp(argv[1], "carried-weight-changes")) weight_changes(false);
    else if (!strcmp(argv[1], "worn-recalculation")) worn_recalculation();
    else std::abort();
    std::printf("locker container weight %s PASS\n", argv[1]);
}
'''.replace("// PRODUCTION", handler + "\n" + chests)

build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="locker-container-weight-", dir=build) as tmp:
    path = Path(tmp)
    source = path / "test.cpp"
    binary = path / "test"
    source.write_text(harness)
    subprocess.run([
        "g++", "-std=c++20", "-g", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-I", str(ROOT / "src"), str(source), "-o", str(binary),
    ], check=True)
    failed = []
    for case in ("sorting", "insert", "append",
                 "worn-insert", "worn-append", "worn-nested-insert", "worn-nested-append",
                 "worn-weight-changes", "carried-weight-changes", "worn-recalculation"):
        result = subprocess.run([str(binary), case], timeout=20,
                                env={**os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1"})
        if result.returncode:
            failed.append(case)
    assert not failed, f"locker container weight regressions: {failed}"
