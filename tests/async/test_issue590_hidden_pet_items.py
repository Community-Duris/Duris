#!/usr/bin/env python3
"""Execute real hidden-item equipment/proc boundaries with synthetic actors.

Effect, presentation, and scheduler endpoints are doubled; the production
policy, equipment boundary, packed weapon proc and both proclib dispatchers
are compiled, including ASan/UBSan. No world or database credentials are used.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(relative, signature):
    text = (ROOT / relative).read_text()
    start = text.index(signature)
    cursor = text.index("{", start) + 1
    depth = 1
    while depth:
        depth += (text[cursor] == "{") - (text[cursor] == "}")
        cursor += 1
    return text[start:cursor]


HARNESS = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "world/db.h"
#include "magic/spells.h"
#include "item/objmisc.h"
#include "item/weapon_actions.h"
#include "cmd/interp.h"
#include "net/comm.h"
#include "classes/necromancy.h"
#include "combat/training_dummy.h"
#include "combat/attack_continuation.h"
#include "economy/collector_presence.h"
#include <string>
using std::string;
#include <cassert>
#include <cstring>
#include <iostream>

static char_data player = {}, pet = {}, wild = {}, victim = {};
static pc_only_data player_pc = {};
static npc_only_data pet_npc = {}, wild_npc = {};
static index_data indexes[4] = {};
P_index obj_index = indexes;
P_index mob_index = indexes;
Skill skills[MAX_SKILLS] = {};
static int native_calls = 0, instance_calls = 0, spell_calls = 0;
static bool linked = true;
static P_char fixture_master = &player;
P_char get_linked_char(P_char actor, ush_int type) {
    return linked && actor == &pet && type == LNK_PET ? fixture_master : nullptr;
}
bool training_dummy_is(P_char) { return false; }
bool training_dummy_capture_target_allowed(P_char) { return true; }
bool collector_presence_is_npc(P_char) { return false; }
P_obj get_globe(P_char) { return nullptr; }
bool has_innate(P_char, int) { return false; }
void clearMemory(P_char) {}
void remember(P_char, P_char) {}
void remove_plushit_bits(P_char) {}
char *str_dup(const char *text) { return strdup(text); }
void linked_affect_to_char(P_char subject, affected_type *, P_char owner, int type) {
    assert(subject == &pet && type == LNK_PET);
    linked = true; fixture_master = owner;
}
void logit(const char *, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
void balance_affects(P_char) {}
void artifact_update_location_sql(P_obj) {}
int char_light(P_char) { return 0; }
int room_light(int, int) { return 0; }
void mark_char_or_owner_dirty(P_char) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
obj_affect *get_obj_affect(P_obj, int) { return nullptr; }
int encumbrance_weight(int weight) { return weight; }
void obj_to_char(P_obj object, P_char actor) {
    object->loc_p = LOC_CARRIED; object->loc.carrying = actor;
    object->next_content = actor->carrying; actor->carrying = object;
}
void obj_to_room(P_obj object, int room) {
    object->loc_p = LOC_ROOM; object->loc.room = room;
}
P_obj unequip_char(P_char actor, int slot, bool) {
    P_obj object = actor->equipment[slot]; actor->equipment[slot] = nullptr;
    object->loc_p = LOC_NOWHERE; object->loc.wearing = nullptr; return object;
}
int native_hook(P_obj, P_char, int, char *) { ++native_calls; return FALSE; }
int instance_hook(P_obj, P_char, int, char *) { ++instance_calls; return TRUE; }
int (*proclib_chain_prev(int))(P_obj, P_char, int, char *) { return native_hook; }
int strn_cmp(const char *a, const char *b, uint count) { return strncmp(a,b,count); }
int number(int low, int) { return low; }
nevent_schedule_result add_event(event_func, int, P_char, P_char, P_obj, int, const void *, int) { return {}; }
struct fixture_proc { const char *procName; int (*func)(P_obj,P_char,int,char *); };
static fixture_proc object_proc_libs[] = {{"test", instance_hook}};
void proclib_obj_event(P_char, P_char, P_obj, void *) {}
int proclib_obj_proc(P_obj, P_char, int, char *);
bool native_artifact_owns(int) { return false; }
attack_continuation begin_attack_continuation(P_char actor, P_char target, P_obj weapon,
                                              int slot) noexcept {
    attack_continuation state{};
    state.actor = actor; state.target = target; state.weapon = weapon;
    state.weapon_slot = slot; return state;
}
attack_continuation_result check_attack_continuation(
    const attack_continuation &state) noexcept {
    return {attack_continuation_outcome::continue_attack, state.actor, state.target, state.weapon};
}
item_action_start selected_packed_weapon_action(P_obj,P_char,P_char) {
    return item_action_start::legacy;
}
bool isname(const char *, const char *) { return false; }
int is_char_in_room(P_char, int) { return TRUE; }
bool affected_by_spell(P_char, int) { return false; }
void test_spell(int,P_char,char *,int,P_char,P_obj) { ++spell_calls; }
// PRODUCTION_FUNCTIONS

int main() {
    player.only.pc = &player_pc;
    pet.specials.act = wild.specials.act = ACT_ISNPC;
    pet.only.npc = &pet_npc; wild.only.npc = &wild_npc;
    pet.specials.position = wild.specials.position = victim.specials.position = STAT_NORMAL;
    indexes[0].virtual_number = 28154; indexes[0].func.obj = native_hook;
    indexes[1].virtual_number = 28155; indexes[1].func.obj = proclib_obj_cmd_bridge;
    obj_data hidden = {}, visible = {}, container = {};
    hidden.R_num = 0; hidden.extra_flags = ITEM_NOSHOW;
    hidden.obj_uid = 59001; visible.R_num = 0;
    assert(item_restricted_for_player_pet(&pet, &hidden));
    assert(!item_restricted_for_player_pet(&wild, &hidden));
    assert(!item_restricted_for_player_pet(&pet, &visible));
    assert(!item_restricted_for_player_pet(&player, &hidden));

    // All physical root locations and nested custody resolve the holder even
    // when periodic/speech dispatch has no actor or an unrelated actor.
    for (int location : {LOC_CARRIED, LOC_WORN}) {
        hidden.loc_p = location; hidden.loc.carrying = &pet;
        for (int command : {CMD_WEAR, CMD_WIELD, CMD_GRAB, CMD_MELEE_HIT,
                            CMD_GOTHIT, CMD_GOTNUKED, CMD_PERIODIC, CMD_SAY, 0}) {
            assert(!invoke_object_special(&hidden, nullptr, command, nullptr));
            assert(!invoke_object_special(&hidden, &wild, command, nullptr));
        }
    }
    assert(native_calls == 0);
    container.loc_p = LOC_CARRIED; container.loc.carrying = &pet;
    hidden.loc_p = LOC_INSIDE; hidden.loc.inside = &container;
    assert(!invoke_object_special(&hidden,nullptr,CMD_PERIODIC,nullptr));
    assert(native_calls == 0 && hidden.obj_uid == 59001);
    hidden.loc_p = LOC_ROOM; hidden.loc.room = 0;
    assert(!invoke_object_special(&hidden,&pet,CMD_SAY,nullptr));
    invoke_object_special(&hidden,&wild,CMD_SAY,nullptr);
    invoke_object_special(&visible,&pet,CMD_SAY,nullptr);
    assert(native_calls == 2);
    hidden.loc_p = LOC_WORN; hidden.loc.wearing = &wild;
    invoke_object_special(&hidden,&pet,CMD_GOTHIT,nullptr);
    assert(native_calls == 3); // Defender-owned proc; actor is the attacking pet.
    hidden.loc_p = LOC_INSIDE; hidden.loc.inside = &hidden;
    assert(!invoke_object_special(&hidden,nullptr,CMD_PERIODIC,nullptr));
    assert(native_calls == 3); // Malformed cycle fails closed without mutation.

    // Legacy/corrupt worn equipment cannot execute packed spell values.
    skills[1].spell_pointer = test_spell;
    hidden.value[5] = 1; hidden.value[6] = 57; hidden.value[7] = 1;
    hidden.loc_p = LOC_WORN; hidden.loc.wearing = &pet;
    assert(!weapon_proc(&hidden,&pet,&victim) && spell_calls == 0);
    hidden.loc.wearing = &wild;
    assert(weapon_proc(&hidden,&wild,&victim) && spell_calls == 1);
    visible.value[5] = 1; visible.value[6] = 57; visible.value[7] = 1;
    visible.loc_p = LOC_WORN; visible.loc.wearing = &pet;
    assert(weapon_proc(&visible,&pet,&victim) && spell_calls == 2);

    // A bridged native callback and instance hook are both blocked, including
    // direct hub/event invocation which does not use the command dispatcher.
    extra_descr_data ed = {};
    char keyword[] = "_proclib_test0", description[] = "synthetic";
    ed.keyword = keyword; ed.description = description;
    hidden.ex_description = &ed; hidden.extra_flags |= ITEM_PROCLIB;
    hidden.R_num = 1; hidden.loc_p = LOC_CARRIED; hidden.loc.carrying = &pet;
    const int native_before = native_calls;
    assert(!proclib_obj_cmd_bridge(&hidden,&pet,CMD_WIELD,nullptr));
    assert(!proclib_obj_proc(&hidden,nullptr,0,nullptr));
    assert(native_calls == native_before && instance_calls == 0);
    hidden.loc.carrying = &wild;
    assert(proclib_obj_cmd_bridge(&hidden,&wild,CMD_WIELD,nullptr));
    assert(native_calls == native_before+1 && instance_calls == 1);
    assert(proclib_obj_proc(&hidden,nullptr,0,nullptr));
    assert(instance_calls == 2);

    // Shared equipment entry retains rejected detached items on the pet.
    hidden.loc_p = LOC_NOWHERE; hidden.loc.carrying = nullptr;
    hidden.next_content = nullptr; pet.carrying = nullptr;
    equip_char(&pet,&hidden,PRIMARY_WEAPON,9);
    assert(!pet.equipment[PRIMARY_WEAPON] && pet.carrying == &hidden);
    assert(OBJ_CARRIED_BY(&hidden,&pet) && hidden.obj_uid == 59001);
    // Restore/charm normalization preserves UID, graph and owner, not a grant
    // to the player. Repeated normalization is harmless.
    pet.carrying = nullptr; hidden.next_content = nullptr;
    pet.equipment[PRIMARY_WEAPON] = &hidden;
    hidden.loc_p = LOC_WORN; hidden.loc.wearing = &pet;
    container.obj_uid = 59002; hidden.contains = &container;
    container.loc_p = LOC_INSIDE; container.loc.inside = &hidden;
    item_restrict_player_pet_equipment(&pet);
    item_restrict_player_pet_equipment(&pet);
    assert(!pet.equipment[PRIMARY_WEAPON] && pet.carrying == &hidden);
    assert(hidden.contains == &container && container.obj_uid == 59002);
    assert(hidden.obj_uid == 59001 && !player.carrying);
    linked = false; wild.equipment[PRIMARY_WEAPON] = nullptr;
    hidden.loc_p = LOC_NOWHERE; hidden.loc.wearing = nullptr;
    equip_char(&wild,&hidden,PRIMARY_WEAPON,9);
    assert(wild.equipment[PRIMARY_WEAPON] == &hidden);
    visible.loc_p = LOC_NOWHERE; visible.loc.wearing = nullptr; linked = true;
    equip_char(&pet,&visible,HOLD,9);
    assert(pet.equipment[HOLD] == &visible);
    // charm_generic may add a follower before establishing LNK_PET. Execute
    // the actual setup boundary: early normalization is a no-op, post-link
    // normalization must retain the same hidden graph in the pet's inventory.
    pet.carrying = nullptr; hidden.next_content = nullptr;
    pet.equipment[PRIMARY_WEAPON] = &hidden;
    hidden.loc_p = LOC_WORN; hidden.loc.wearing = &pet;
    linked = false;
    item_restrict_player_pet_equipment(&pet);
    assert(pet.equipment[PRIMARY_WEAPON] == &hidden);
    pet_npc.summon_kind = 1;
    assert(setup_pet(&pet,&player,15,PET_RESTORE|PET_NOAGGRO) == 15);
    assert(linked && !pet.equipment[PRIMARY_WEAPON] && pet.carrying == &hidden);
    assert(hidden.obj_uid == 59001 && hidden.contains == &container && !player.carrying);
    // NPC-owned followers are not PC pets and retain intended helper gear.
    pet.carrying = nullptr; pet.equipment[PRIMARY_WEAPON] = &hidden;
    hidden.loc_p = LOC_WORN; hidden.loc.wearing = &pet; linked = false;
    setup_pet(&pet,&wild,15,PET_RESTORE|PET_NOAGGRO);
    assert(pet.equipment[PRIMARY_WEAPON] == &hidden && !pet.carrying);
    std::cout << "issue590 equipment, custody, packed/native/instance/periodic policy passed\n";
}
'''


def main():
    objmisc = ROOT / "src/item/objmisc.c"
    assert "bool item_restricted_for_player_pet(" in objmisc.read_text(), \
        "shared hidden-pet policy missing (pre-fix regression)"
    production = "\n".join([
        function("src/world/handler.c", "void equip_char(P_char"),
        function("src/classes/necromancy.c", "int setup_pet(P_char"),
        function("src/combat/attack_effects.c", "bool weapon_proc("),
        function("src/mob/studioproclib.c", "int proclib_obj_cmd_bridge("),
        function("src/specs/specs.library.c", "int proclib_obj_proc(P_obj obj, P_char ch, int cmd, char *argument)\n{"),
    ])
    with tempfile.TemporaryDirectory(prefix="duris-issue590-") as directory:
        source = Path(directory) / "fixture.cpp"
        source.write_text(HARNESS.replace("// PRODUCTION_FUNCTIONS", production))
        for sanitizer in (False, True):
            binary = Path(directory) / ("fixture-san" if sanitizer else "fixture")
            command = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                       "-ffunction-sections", "-fdata-sections", "-Isrc", str(source),
                       str(objmisc), "-Wl,--gc-sections", "-o", str(binary)]
            if sanitizer:
                command[1:1] = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie"]
            subprocess.run(command, cwd=ROOT, check=True)
            environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                               UBSAN_OPTIONS="halt_on_error=1")
            subprocess.run([str(binary)], cwd=ROOT, env=environment, check=True)

    # Commands enter wear before any binding/placement effect; restore normalizes
    # before enchant activation. A source census prevents new native bypasses.
    wear = function("src/cmd/actobj.c", "int wear(P_char")
    assert wear.index("item_restricted_for_player_pet(") < wear.index("can_equip_soulbound_item(")
    auto = function("src/mob/mobact.c", "void CheckEqWorthUsing(")
    assert "item_restricted_for_player_pet(" in auto
    follower = function("src/net/sparser.c", "void add_follower(")
    assert "item_restrict_player_pet_equipment(" in follower
    setup = function("src/classes/necromancy.c", "int setup_pet(P_char")
    assert setup.index("linked_affect_to_char(") < setup.index("item_restrict_player_pet_equipment(")
    hydration = function("src/player/player_load_items.c", "void attach_loaded_inventory(")
    assert "IS_NPC(character) && (object->extra_flags & ITEM_NOSHOW)" in hydration
    import re
    bypass = re.compile(r"\(\*obj_index\[[^;]{1,160}?\]\.func\.obj\)\(")
    offenders = [str(path.relative_to(ROOT)) for path in (ROOT / "src").rglob("*.c")
                 if path.name != "objmisc.c" and bypass.search(path.read_text())]
    assert not offenders, f"unguarded object-special dispatch: {offenders}"
    print("issue590 shared-boundary and native-dispatch census passed")


if __name__ == "__main__":
    main()
