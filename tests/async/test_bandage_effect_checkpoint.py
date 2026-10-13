#!/usr/bin/env python3
"""Execute the actual bandage event/vamp arithmetic and native revision tracking."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "classes/paladins.h"
#include "magic/spells.h"
#include "player/player_revision_state.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>

static int scheduled, poison_removed;
static bool cure_poison;
float get_property(const char *,double fallback) { return fallback; }
int get_property(const char *,int fallback) { return fallback; }
int GET_CHAR_SKILL_P(P_char,int) { return 100; }
int number(int,int high) { return cure_poison && high == 4 ? 0 : (high == 100 ? 50 : high); }
bool notch_skill(P_char,int,float) { return false; }
bool affected_by_spell(P_char,int) { return false; }
bool affected_by_skill(P_char,int) { return false; }
bool has_aura(P_char,int) { return false; }
int aura_mod(P_char,int) { return 0; }
affected_type *get_spell_from_char(P_char,int,void *,int) { return nullptr; }
void wear_off_message(P_char,affected_type *) {}
void affect_remove(P_char,affected_type *) {}
void update_groupies(P_char,bool) {}
void update_pos(P_char) {}
void send_to_char(const char *,P_char) {}
void act(const char *,int,P_char,P_obj,void *,int) {}
int poison_common_remove(P_char target) {
    ++poison_removed; REMOVE_BIT(target->specials.affected_by2,AFF2_POISONED); return 1;
}
void mark_player_dirty_components(int pid,player_component_mask_t mask) {
    assert(player_revision_mark(pid,mask,nullptr));
}
nevent_schedule_result add_event(event_func,int,P_char,P_char,P_obj,int,const void *,int) {
    ++scheduled; return {nevent_schedule_status::scheduled,{nullptr,1}};
}
[[noreturn]] int panic_corruption_int(const char *,const char *,...) { std::abort(); }
struct bandage_data { int room,healed,maxheal; };
'''

DRIVER = r'''
int main() {
    constexpr player_component_mask_t status = PLAYER_COMPONENT_STATUS;
    constexpr player_component_mask_t affects = PLAYER_COMPONENT_AFFECTS;
    for (int target_kind : {0,1,2}) {
        char_data actor{}, other{}; pc_only_data pc{}, victim_pc{}; npc_only_data npc{};
        descriptor_data descriptor{}; pc.pid = 17; victim_pc.pid = 18;
        actor.only.pc = &pc; actor.desc = &descriptor;
        other.only.pc = &victim_pc;
        P_char target = target_kind == 0 ? &actor : &other;
        if (target_kind == 2) { other.specials.act = ACT_ISNPC; other.only.npc = &npc; }
        auto reset = [&] {
            player_revision_reset_for_tests();
            assert(player_revision_hydrate(17,4)); assert(player_revision_hydrate(18,4));
            actor.specials.position = other.specials.position = STAT_NORMAL;
            actor.in_room = other.in_room = 0; actor.specials.fighting = nullptr;
            actor.specials.affected_by2 = other.specials.affected_by2 = 0;
            GET_HIT(&actor) = GET_HIT(&other) = 20;
            GET_MAX_HIT(&actor) = GET_MAX_HIT(&other) = 100;
            GET_VITALITY(&actor) = 80;
            cure_poison = false; scheduled = poison_removed = 0;
        };
        auto dirty = [&](int pid) {
            player_revision_snapshot snapshot{}; assert(player_revision_snapshot_copy(pid,&snapshot));
            return snapshot.dirty_components;
        };
        reset(); bandage_data data{0,0,200};
        event_bandage_check(&actor,target,nullptr,&data);
        assert(GET_HIT(target) == 44 && GET_VITALITY(&actor) == 78 && scheduled == 1 && data.healed == 24);
        assert(dirty(17) == PLAYER_COMPONENT_STATUS);
        assert(dirty(18) == (target_kind == 1 ? status : player_component_mask_t{0}));
        reset(); data = {0,0,200}; actor.specials.fighting = &other;
        event_bandage_check(&actor,target,nullptr,&data);
        assert(GET_HIT(target) == 20 && GET_VITALITY(&actor) == 80 && scheduled == 0);
        assert(dirty(17) == 0 && dirty(18) == 0);
        reset(); data = {0,0,200}; GET_VITALITY(&actor) = 8;
        event_bandage_check(&actor,target,nullptr,&data);
        assert(GET_HIT(target) == 20 && GET_VITALITY(&actor) == 8 && scheduled == 0);
        assert(dirty(17) == 0 && dirty(18) == 0);
        reset(); data = {0,0,200}; GET_HIT(target) = 87;
        event_bandage_check(&actor,target,nullptr,&data);
        assert(GET_HIT(target) == 87 && GET_VITALITY(&actor) == 80 && scheduled == 0);
        assert(dirty(17) == 0 && dirty(18) == 0);
        reset(); data = {0,200,200};
        event_bandage_check(&actor,target,nullptr,&data);
        assert(GET_HIT(target) == 20 && GET_VITALITY(&actor) == 80 && scheduled == 0);
        assert(dirty(17) == 0 && dirty(18) == 0);
        reset(); data = {0,0,200}; cure_poison = true;
        target->specials.affected_by2 = AFF2_POISONED;
        event_bandage_check(&actor,target,nullptr,&data);
        assert(poison_removed == 1 && !IS_AFFECTED2(target,AFF2_POISONED));
        assert(dirty(17) == (target_kind == 0 ? status|affects : status));
        assert(dirty(18) == (target_kind == 1 ? status|affects : player_component_mask_t{0}));
    }
    std::puts("Actual bandage/vamp + player revision: self/PC/NPC, heal/vitality/poison and unchanged refusal/cap controls passed.");
}
'''

code = PRELUDE + extract_function("damage_support.c", "int vamp(")
code += extract_function("tradeskill.c", "void event_bandage_check(") + DRIVER
with tempfile.TemporaryDirectory(prefix="duris-bandage-effects-") as directory:
    harness = Path(directory) / "bandage.cpp"
    binary = Path(directory) / "bandage"
    harness.write_text(code, encoding="utf-8")
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie",
        "-I", str(ROOT / "src"), str(harness), "-x", "c++",
        str(ROOT / "src/player/player_revision_state.c"), "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)
