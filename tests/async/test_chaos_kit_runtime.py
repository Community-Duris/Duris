#!/usr/bin/env python3
"""Compile production CHAOS preparation/placement against deterministic objects.

Unit/runtime checks only: stub prototype loading and skill eligibility, no
server, database, Redis, networking, or durable state. Requires Linux g++ and
ASan/UBSan. Generated role/flag tables and actual preparation functions are used.
"""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, SRC, extract_function

NANNY = (SRC / "nanny.c").read_text()
BUILDER = NANNY[NANNY.index("struct chaos_kit_objects"):NANNY.index("static bool chaos_kit_skill_available")]
PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "account/chaos_eq_data.h"
#include "item/item_movement_transaction.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>

static index_data indexes[1]{};
P_index obj_index = indexes;
static obj_data loaded{};
static int required_level = 1;
static bool slot_available = true, usable = true, nesting = true, object_available = true;
static int extracts = 0, keywords = 0, book_additions = 0;
int equipment_pos_table[CUR_MAX_WEAR][3]{};
int flag2idx(int flags) { int i = 0; while (flags) { ++i; flags >>= 1; } return i; }
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }
int GET_CLASS(P_char actor, uint cls) { return actor->player.m_class & cls; }
int GET_LVL_FOR_SKILL(P_char, int) { return required_level; }
int get_spell_circle(P_char, int spell) { return spell == FIRST_SPELL ? 1 : 2; }
int AddSpellToSpellBook(P_char, P_obj, int) { ++book_additions; return 1; }
static void add_newbie_keyword(P_obj) { ++keywords; }
bool has_eq_slot(P_char, int) { return slot_available; }
int can_char_use_item(P_char, P_obj) { return usable; }
P_obj read_object(int vnum, int) { indexes[0].virtual_number = vnum; return object_available ? &loaded : nullptr; }
void extract_obj(P_obj, int) { ++extracts; }
bool obj_can_nest(P_obj, P_obj) { return nesting; }
void obj_to_obj(P_obj object, P_obj bag) { object->loc_p = LOC_INSIDE; object->loc.inside = bag; bag->contains = object; }
void logit(const char *, const char *, ...) {}
'''
DRIVER = r'''
int main()
{
    pc_only_data pc{};
    pc.pid = 1;
    char_data actor{};
    actor.only.pc = &pc;
    actor.player.m_class = CLASS_WARRIOR;
    loaded.R_num = 0;
    loaded.type = ITEM_ARMOR;
    loaded.extra_flags = ITEM_TRANSIENT | ITEM_NODROP | ITEM_INVISIBLE | ITEM_SECRET |
                         ITEM_NOSHOW | ITEM_BURIED | ITEM_NORENT | ITEM_GLOW | ITEM_HUM;
    loaded.extra2_flags = ITEM2_CRUMBLELOOT | ITEM2_BLESS;
    loaded.affected[0] = {APPLY_CURSE, 10};
    loaded.affected[1] = {APPLY_STR, 3};
    loaded.affected[MAX_OBJ_AFFECT - 1] = {APPLY_CURSE, -7};
    loaded.bitvector2 = AFF2_GLOBE;
    prepare_chaos_kit_item(&actor, &loaded);
    assert(loaded.cost == 1 && keywords == 1);
    assert(loaded.extra_flags == (ITEM_GLOW | ITEM_HUM));
    assert(loaded.extra2_flags == ITEM2_BLESS);
    assert(loaded.affected[0].location == 0 && loaded.affected[0].modifier == 0);
    assert(loaded.affected[MAX_OBJ_AFFECT - 1].location == 0 && loaded.affected[MAX_OBJ_AFFECT - 1].modifier == 0);
    assert(loaded.affected[1].location == APPLY_STR && loaded.affected[1].modifier == 3);
    assert(loaded.bitvector2 == AFF2_GLOBE);
    prepare_chaos_kit_item(&actor, &loaded);
    assert(loaded.extra_flags == (ITEM_GLOW | ITEM_HUM) && loaded.bitvector2 == AFF2_GLOBE);
    loaded.type = ITEM_SPELLBOOK;
    indexes[0].virtual_number = MASTER_SPELLBOOK_VNUM;
    prepare_chaos_kit_item(&actor, &loaded);
    assert(book_additions == 0);
    indexes[0].virtual_number = 999;
    prepare_chaos_kit_item(&actor, &loaded);
    assert(book_additions == 1 && loaded.value[3] == 1);
    for (int level : {-1, 0, 1, 56, 57}) {
        required_level = level;
        assert(chaos_kit_skill_available(&actor, SKILL_TRAP) == (level > 0 && level <= 56));
    }
    required_level = 1;
    obj_data bag{};
    bag.type = ITEM_CONTAINER;
    loaded = {};
    loaded.R_num = 0;
    loaded.type = ITEM_ARMOR;
    loaded.wear_flags = ITEM_WEAR_NECK;
    equipment_pos_table[0][0] = ITEM_WEAR_NECK;
    equipment_pos_table[0][2] = WEAR_NECK_1;
    const chaos_kit_item wearable = {WEAR_NECK_1, 999};
    const chaos_kit_item support = {WEAR_NONE, 999};
    for (int cls : {CLASS_WARRIOR, CLASS_MONK, CLASS_THIEF, CLASS_SORCERER}) {
        actor.player.m_class = cls;
        loaded.bitvector2 = 0;
        chaos_kit_objects kit;
        assert(append_chaos_kit_item(&actor, &bag, &wearable, kit));
        assert(kit.count == 1 && kit.roots[0] == &loaded && bag.contains == nullptr);
        assert(bool(loaded.bitvector2 & AFF2_GLOBE) == (cls != CLASS_SORCERER));
        kit.count = 0;
    }
    actor.player.m_class = CLASS_MONK;
    for (int type : {ITEM_WEAPON, ITEM_FIREWEAPON, ITEM_MISSILE}) {
        loaded.type = type;
        chaos_kit_objects kit;
        assert(append_chaos_kit_item(&actor, &bag, &support, kit));
        assert(kit.count == 0 && bag.contains == nullptr);
    }
    loaded.type = ITEM_ARMOR;
    for (int slot : {PRIMARY_WEAPON, SECONDARY_WEAPON, THIRD_WEAPON, FOURTH_WEAPON}) {
        chaos_kit_objects kit;
        const chaos_kit_item weapon_slot = {slot, 999};
        assert(append_chaos_kit_item(&actor, &bag, &weapon_slot, kit));
        assert(!kit.count && !bag.contains);
    }
    loaded.wear_flags |= ITEM_WIELD;
    { chaos_kit_objects kit; assert(append_chaos_kit_item(&actor, &bag, &support, kit)); assert(!kit.count && !bag.contains); }
    loaded.wear_flags = ITEM_WEAR_NECK;
    actor.player.m_class = CLASS_WARRIOR;
    for (int failure = 0; failure < 3; ++failure) {
        chaos_kit_objects kit;
        object_available = failure != 0;
        usable = failure != 1;
        loaded.wear_flags = failure == 2 ? ITEM_WEAR_FEET : ITEM_WEAR_NECK;
        assert(!append_chaos_kit_item(&actor, &bag, &wearable, kit));
        assert(!kit.count && !bag.contains);
    }
    object_available = usable = true;
    loaded.wear_flags = ITEM_WEAR_NECK;
    slot_available = false;
    { chaos_kit_objects kit; assert(append_chaos_kit_item(&actor, &bag, &wearable, kit)); assert(!kit.count); }
    slot_available = true;
    nesting = false;
    { chaos_kit_objects kit; assert(!append_chaos_kit_item(&actor, &bag, &support, kit)); assert(!kit.count); }
    nesting = true;
    { chaos_kit_objects kit; assert(append_chaos_kit_item(&actor, &bag, &support, kit)); assert(!kit.count && bag.contains == &loaded); }
    puts("CHAOS preparation/role/placement runtime passed");
}
'''


def main():
    functions = ["static void prepare_chaos_kit_item", "static bool chaos_kit_skill_available",
                 "static bool chaos_kit_weapon_slot", "static bool chaos_kit_fits_slot",
                 "static bool append_chaos_kit_item"]
    harness = "\n".join([PRELUDE, BUILDER, *[extract_function("nanny.c", sig) for sig in functions], DRIVER])
    with tempfile.TemporaryDirectory(prefix="chaos-kit-unit-") as directory:
        source, binary = Path(directory) / "kit.cpp", Path(directory) / "kit"
        source.write_text(harness)
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-Isrc", str(source), "-o", str(binary)],
                       cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)

if __name__ == "__main__":
    main()
