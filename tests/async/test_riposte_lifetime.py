#!/usr/bin/env python3
"""Production riposte control flow with destructive attack callbacks under sanitizers.

Callback doubles isolate invalid continuation; real reflective/proc extraction
requires the separate server journey described in the issue's validation record.
"""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

PREFIX = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "combat/damage.h"
#include "combat/attack_continuation.h"
#include <cassert>
#include <cstdio>

enum class riposte_branch { ordinary, expert, elite, innate, berserker, followup };
enum class mutation { none, kill_actor, kill_target, extract_actor, extract_target,
                      replace_actor, replace_target, move_actor, move_target,
                      height_target, drop_weapon, replace_weapon };
static riposte_branch path;
static mutation change;
static int trigger = 1, hits = 0, damages = 0, melees = 0, skill_reads = 0;
static bool actor_listed = true, target_listed = true, mutate_damage = false;
static char_data actor = {}, target = {};
static pc_only_data actor_pc = {}, target_pc = {};
static obj_data weapon = {}, secondary = {};
P_obj object_list = nullptr;

int number(int low, int) { return low; }
bool innate_two_daggers(P_char) { return path == riposte_branch::innate; }
int GET_CLASS(P_char, uint cls) {
    return (path == riposte_branch::elite && cls == CLASS_WARRIOR) ||
           (path == riposte_branch::berserker && cls == CLASS_BERSERKER);
}
int GET_CHAR_SKILL_P(P_char ch, int id) {
    ++skill_reads;
    assert(ch && IS_ALIVE(ch) && (IS_NPC(ch) || ch->only.pc));
    if (id == SKILL_RIPOSTE) return 100;
    if (id == SKILL_EXPERT_RIPOSTE) return path == riposte_branch::expert ? 100 : 0;
    if (id == SKILL_FOLLOWUP_RIPOSTE) return path == riposte_branch::followup ? 100 : 0;
    return 0;
}
bool notch_skill(P_char, int, float) { return false; }
int get_property(const char *, int fallback) { return fallback; }
void act(const char *, int, P_char ch, P_obj, void *victim, int) {
    assert(IS_ALIVE(ch) && (!victim || IS_ALIVE(static_cast<P_char>(victim))));
}
int char_in_list(const P_char ch) {
    return (ch == &actor && actor_listed) || (ch == &target && target_listed);
}
P_char find_character_by_runtime_id(uint64_t id) {
    if (actor_listed && actor.runtime_id == id) return &actor;
    if (target_listed && target.runtime_id == id) return &target;
    return nullptr;
}
bool affected_by_spell(P_char, int) { return path == riposte_branch::berserker; }
void send_to_char(const char *, P_char) {}
static void mutate() {
    switch (change) {
    case mutation::none: break;
    case mutation::kill_actor: SET_POS(&actor, STAT_DEAD); actor.only.pc = nullptr; break;
    case mutation::kill_target: SET_POS(&target, STAT_DEAD); target.only.pc = nullptr; break;
    case mutation::extract_actor: actor_listed = false; actor.only.pc = nullptr; break;
    case mutation::extract_target: target_listed = false; target.only.pc = nullptr; break;
    case mutation::replace_actor: ++actor.runtime_id; break;
    case mutation::replace_target: ++target.runtime_id; break;
    case mutation::move_actor: ++actor.in_room; break;
    case mutation::move_target: ++target.in_room; break;
    case mutation::height_target: ++target.specials.z_cord; break;
    case mutation::drop_weapon:
        actor.equipment[PRIMARY_WEAPON] = actor.equipment[SECONDARY_WEAPON] = nullptr;
        break;
    case mutation::replace_weapon: ++weapon.obj_uid; ++secondary.obj_uid; break;
    }
}
bool hit(P_char ch, P_char victim, P_obj, int *) {
    assert(char_in_list(ch) && char_in_list(victim) && IS_ALIVE(ch) && IS_ALIVE(victim));
    assert(ch->in_room == victim->in_room && ch->specials.z_cord == victim->specials.z_cord);
    ++hits;
    if (!mutate_damage && hits == trigger) mutate();
    return false; // This boolean deliberately does not promise participant survival.
}
int dice(int, int) { return 1; }
bool damage(P_char ch, P_char victim, double, int) {
    assert(IS_ALIVE(ch) && IS_ALIVE(victim));
    ++damages;
    if (mutate_damage) mutate();
    return false;
}
int melee_damage(P_char ch, P_char victim, double, int, damage_messages *, int *) {
    assert(IS_ALIVE(ch) && IS_ALIVE(victim));
    ++melees;
    return 0;
}
static void reset(riposte_branch next, mutation effect = mutation::none, int on_hit = 1) {
    actor = {}; target = {}; weapon = {}; secondary = {};
    actor.only.pc = &actor_pc; target.only.pc = &target_pc;
    actor.runtime_id = 10; target.runtime_id = 20;
    actor.in_room = target.in_room = 1;
    SET_POS(&actor, STAT_NORMAL + POS_STANDING);
    SET_POS(&target, STAT_NORMAL + POS_STANDING);
    actor.specials.fighting = &target;
    weapon.obj_uid = 100; secondary.obj_uid = 200;
    weapon.next = &secondary; secondary.next = nullptr;
    object_list = &weapon;
    actor.equipment[PRIMARY_WEAPON] = &weapon;
    path = next; change = effect; trigger = on_hit;
    hits = damages = melees = skill_reads = 0;
    actor_listed = target_listed = true; mutate_damage = false;
    if (path == riposte_branch::elite) actor.specials.act = ACT_ELITE | ACT_ISNPC;
    if (path == riposte_branch::innate) actor.equipment[SECONDARY_WEAPON] = &secondary;
}
'''
SUFFIX = r'''
int main() {
    for (auto path : {riposte_branch::ordinary, riposte_branch::expert, riposte_branch::elite,
                      riposte_branch::innate, riposte_branch::berserker, riposte_branch::followup}) {
        reset(path);
        assert(try_riposte(&actor, &target, &weapon));
        const int normal_hits = hits;
        assert(hits == (path == riposte_branch::expert || path == riposte_branch::elite ? 3 :
                       path == riposte_branch::innate || path == riposte_branch::berserker ? 2 : 1));
        assert(damages == (path == riposte_branch::followup ? 1 : 0));
        assert(melees == (path == riposte_branch::followup ? 1 : 0));
        for (auto effect : {mutation::kill_actor, mutation::kill_target,
                            mutation::extract_actor, mutation::extract_target,
                            mutation::replace_actor, mutation::replace_target,
                            mutation::move_actor, mutation::move_target,
                            mutation::height_target, mutation::drop_weapon,
                            mutation::replace_weapon}) {
            for (int on_hit = 1; on_hit <= normal_hits; ++on_hit) {
                reset(path, effect, on_hit);
                assert(try_riposte(&actor, &target, &weapon));
                assert(hits == on_hit && damages == 0 && melees == 0);
            }
            reset(riposte_branch::followup, effect);
            mutate_damage = true;
            assert(try_riposte(&actor, &target, &weapon));
            assert(hits == 1 && damages == 1 && melees == 0);
        }
        printf("riposte riposte_branch=%u: living attack counts and all destructive continuations passed\n", unsigned(path));
    }
    for (bool dead_actor : {false, true}) {
        reset(riposte_branch::expert);
        auto dead = dead_actor ? &actor : &target;
        SET_POS(dead, STAT_DEAD);
        dead->only.pc = nullptr;
        assert(!hit_entry(&actor, &target, &weapon, nullptr));
        assert(skill_reads == 0);
    }
    assert(!hit_entry(nullptr, &target, nullptr, nullptr));
    puts("hit entry rejects dead/cleared player storage before skill reads");
}
'''

body = extract_function('fight.c', 'int try_riposte(P_char ch, P_char victim, P_obj wpn)')
# Also execute the unchanged production hit entry through its initial guards.
# This narrowly probes ordering before the first gameplay effect, not the full
# hit implementation; the full server is built and tested separately.
entry = extract_function('fight.c', 'bool hit(P_char ch, P_char victim, P_obj weapon, int *damAccumulator)')
entry = entry[:entry.index('\tif (IS_AFFECTED(ch, AFF_BOUND))')] + '\nreturn FALSE;\n}\n'
entry = entry.replace('bool hit(', 'bool hit_entry(', 1)
with tempfile.TemporaryDirectory(prefix='duris-riposte-lifetime-') as temporary:
    source = Path(temporary) / 'riposte.cpp'
    binary = Path(temporary) / 'riposte'
    source.write_text(PREFIX + body + entry + SUFFIX)
    subprocess.run(['g++', '-std=c++20', '-g', '-Og', '-D__NO_MYSQL__',
                    '-Isrc', '-Isrc/no_mysql', '-I/usr/include/libxml2',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    '-fno-pie', '-no-pie', str(source),
                    str(ROOT / 'src' / 'combat' / 'attack_continuation.c'),
                    '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
