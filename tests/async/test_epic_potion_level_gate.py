#!/usr/bin/env python3
"""Exercise the production quaff command at the configured epic award boundary."""

import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, extract_function, source


PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "combat/damage.h"
#include "world/db.h"
#include "world/epic.h"
#include "world/vnum.obj.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "item/item_command_policy.h"
#include "item/item_movement_transaction.h"
#include "persistence/persistence_checkpoint.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

P_index obj_index;
P_room world;
Skill skills[MAX_SKILLS]{};
static int minimum_level = 50;
static int awards, consumed, waits, unequips, potion_effects;
static std::string output;
static P_obj live_item;
static item_movement_completion_fn pending_completion;
static item_movement_publication_fn pending_publication;
static uint8_t pending_context[ITEM_MOVEMENT_CONTEXT_MAX_BYTES];
static size_t pending_context_size;

float get_property(const char *key, double fallback) {
    return !strcmp(key, "epic.gain.minLevel") ? minimum_level : fallback;
}
void send_to_char(const char *text, P_char) { output += text; }
void send_to_char_f(P_char, const char *format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);
    output += buffer;
}
void act(const char *text, int, P_char, P_obj, void *, int) { output += text; }
char *one_argument(const char *text, char *out) {
    std::strcpy(out, text);
    return const_cast<char *>(text + std::strlen(text));
}
P_obj get_obj_in_list_vis(P_char, const char *, P_obj list, bool) { return list; }
bool isname(const char *a, const char *b) { return !strcmp(a, b); }
bool affected_by_spell(P_char, int) { return false; }
bool has_innate(P_char, int) { return false; }
int number(int, int high) { return high; }
void CharWait(P_char, int) { ++waits; }
P_obj unequip_char(P_char ch, int slot, bool) {
    ++unequips;
    P_obj result = ch->equipment[slot];
    ch->equipment[slot] = nullptr;
    return result;
}
void gain_epic(P_char ch, int type, int, int amount) {
    assert(type == EPIC_BOTTLE && amount == 75);
    if (epic_level_can_gain(ch)) ++awards;
}
void extract_obj(P_obj object, int) {
    ++consumed;
    if (live_item == object) live_item = nullptr;
}
void mark_player_dirty_components(int, player_component_mask_t) {}
bool item_command_uses_durable_ownership(P_obj object) { return object->obj_uid != 0; }
bool item_tree_has_durable_ownership(P_obj object) { return object->obj_uid != 0; }
bool item_movement_transaction_submit(P_char, P_obj, P_obj,
    const item_owner_identity &, const item_owner_identity &, item_transfer_reason,
    int64_t, item_movement_completion_fn completion, const void *context, size_t context_size,
    P_obj, item_movement_reject *, item_movement_publication_fn publication) {
    assert(completion && publication && context_size <= sizeof pending_context);
    pending_completion = completion;
    pending_publication = publication;
    pending_context_size = context_size;
    std::memcpy(pending_context, context, context_size);
    return true;
}
const char *item_movement_reject_name(item_movement_reject) { return "test"; }
void logit(const char *, const char *, ...) {}
static P_obj find_actoth_item(uint64_t uid) {
    return live_item && live_item->obj_uid == uid ? live_item : nullptr;
}
int spell_damage(P_char, P_char, double, int, uint, damage_messages *, int *) {
    return 0;
}
affected_type *affect_to_char(P_char, affected_type *) { ++potion_effects; return nullptr; }
int char_in_list(const P_char) { return 1; }
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
'''

DRIVER = r'''
static void check_quaff(int level, int gate, bool held, bool epic, bool allowed) {
    char_data ch{};
    pc_only_data pc{};
    obj_data bottle{};
    index_data index[1]{};
    room_data room[1]{};
    obj_index = index;
    world = room;
    index[0].virtual_number = epic ? VOBJ_EPIC_BOTTLE_EPICS : 1;
    ch.specials.position = STAT_NORMAL;
    ch.only.pc = &pc;
    pc.pid = 42;
    ch.player.level = level;
    bottle.type = ITEM_POTION;
    bottle.R_num = 0;
    bottle.name = const_cast<char *>("potion");
    if (held) {
        ch.equipment[HOLD] = &bottle;
        bottle.loc_p = LOC_WORN;
        bottle.loc.wearing = &ch;
    } else {
        ch.carrying = &bottle;
        bottle.loc_p = LOC_CARRIED;
        bottle.loc.carrying = &ch;
    }
    minimum_level = gate;
    awards = consumed = waits = unequips = potion_effects = 0;
    live_item = nullptr;
    pending_completion = nullptr;
    pending_publication = nullptr;
    output.clear();
    char argument[] = "potion";
    do_quaff(&ch, argument, 0);
    assert(consumed == (allowed ? 1 : 0));
    assert(awards == (allowed && epic ? 1 : 0));
    assert(waits == (allowed ? 1 : 0));
    assert(unequips == (allowed && held ? 1 : 0));
    assert(potion_effects == (allowed && !epic ? 1 : 0));
    if (!allowed) {
        assert(output.find(std::to_string(gate)) != std::string::npos);
        assert(output.find("suddenly feel.. epic") == std::string::npos);
        assert(held ? ch.equipment[HOLD] == &bottle : ch.carrying == &bottle);
    }
}
static void check_durable_quaff(bool held, bool epic) {
    char_data ch{};
    pc_only_data pc{};
    obj_data bottle{};
    index_data index[1]{};
    room_data room[1]{};
    obj_index = index;
    world = room;
    index[0].virtual_number = epic ? VOBJ_EPIC_BOTTLE_EPICS : 1;
    ch.specials.position = STAT_NORMAL;
    ch.player.level = 50;
    ch.only.pc = &pc;
    pc.pid = 42;
    bottle.obj_uid = 1234;
    bottle.type = ITEM_POTION;
    bottle.R_num = 0;
    bottle.name = const_cast<char *>("potion");
    if (held) {
        ch.equipment[HOLD] = &bottle;
        bottle.loc_p = LOC_WORN;
        bottle.loc.wearing = &ch;
    } else {
        ch.carrying = &bottle;
        bottle.loc_p = LOC_CARRIED;
        bottle.loc.carrying = &ch;
    }
    live_item = &bottle;
    minimum_level = 50;
    awards = consumed = waits = unequips = potion_effects = 0;
    pending_completion = nullptr;
    pending_publication = nullptr;
    output.clear();
    char argument[] = "potion";
    do_quaff(&ch, argument, 0);
    assert(pending_completion && pending_publication);
    assert(reinterpret_cast<const quaff_context *>(pending_context)->item_uid == 1234);
    assert(reinterpret_cast<const quaff_context *>(pending_context)->local_item == nullptr);
    assert(consumed == 0 && awards == 0 && waits == 0 && potion_effects == 0);
    assert(held ? ch.equipment[HOLD] == &bottle : ch.carrying == &bottle);
    item_transfer_result result{};
    bottle.loc_p = LOC_NOWHERE;
    assert(!OBJ_CARRIED_BY(&bottle, &ch));
    assert(!pending_publication(&ch, true, result, 0, pending_context, pending_context_size));
    assert(consumed == 0 && awards == 0);
    bottle.loc_p = held ? LOC_WORN : LOC_CARRIED;
    assert(pending_publication(&ch, true, result, 0, pending_context, pending_context_size));
    assert(consumed == 1 && awards == 0 && potion_effects == 0);
    pending_completion(&ch, true, result, 0, pending_context, pending_context_size);
    assert(awards == (epic ? 1 : 0));
    assert(potion_effects == (epic ? 0 : 1));
    assert(waits == 1 && unequips == (held ? 1 : 0));
}
int main() {
    for (bool held : {false, true}) {
        for (int level : {46, 47, 48, 49}) check_quaff(level, 50, held, true, false);
        check_quaff(50, 50, held, true, true);
        check_quaff(55, 56, held, true, false);
        check_quaff(56, 56, held, true, true);
        check_quaff(39, 40, held, true, false);
        check_quaff(40, 40, held, true, true);
        check_quaff(1, 50, held, false, true);
    }
    for (bool held : {false, true}) {
        check_durable_quaff(held, false);
        check_durable_quaff(held, true);
    }
    std::puts("Epic potion level gate: inventory, held, default/custom boundaries and ordinary potions passed.");
}
'''

quaff_source = source("actoth.c").read_text(encoding="utf-8")
context_start = quaff_source.index("struct quaff_context\n{")
context_end = quaff_source.index("};", context_start) + 2
quaff_helpers = quaff_source[context_start:context_end] + "\n" + "\n".join(
    extract_function("actoth.c", signature) for signature in (
        "void apply_quaff_effects(", "bool quaff_publication(", "void quaff_completed("
    )
)

code = PRELUDE + extract_function("utility.c", "int BOUNDED(") + "\n".join(
    extract_function("epic.c", signature) for signature in (
        "int epic_gain_min_level()", "bool epic_level_can_gain(P_char ch)"
    )
) + quaff_helpers + extract_function("actoth.c", "void do_quaff(") + DRIVER

with tempfile.TemporaryDirectory(prefix="duris-epic-potion-level-") as directory:
    source = Path(directory) / "quaff.cpp"
    binary = Path(directory) / "quaff"
    source.write_text(code)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie",
        "-I" + str(ROOT / "src"), str(source), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
