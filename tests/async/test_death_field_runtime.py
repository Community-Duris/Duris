#!/usr/bin/env python3
"""Exercise production NPC cast completion, area selection and spell defenses.

World/event services and the final raw HP application are deterministic fixtures;
MobCastSpell, event_spellcast, Death Field, spell_damage and wards are real code.
"""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, SRC, extract_function

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "world/events.h"
#include "magic/spells.h"
#include "combat/damage.h"
#include "combat/dam_mods.h"
#include "combat/grapple.h"
#include "combat/guard.h"
#include "cmd/interp.h"
#include "core/mm.h"
#include "sql/sql.h"
#include "world/specs.prototypes.h"
#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>
#include <map>
#include <string>

static room_data rooms[2]{};
P_room world = rooms;
static index_data indexes[1]{};
P_index mob_index = indexes, obj_index = indexes;
Skill skills[MAX_AFFECT_TYPES + 1];
spell_target_data common_target_data{};
extern const stat_data stat_factor[256] = {};
int proccing_slots[] = {PRIMARY_WEAPON}; // fixture has no equipment
static void no_modifier(P_char, P_char, double, int, uint, damage_mod *, damage_messages *) {}
dam_mod_predicate spell_damage_modifiers[NUM_SPELL_PREDICATES];
static std::mt19937 rng(12345);
static std::map<P_char, int> damaged;
static std::map<P_char, std::string> transcript;
static bool resist = false, evasion = false;
static float minimum = 60;
static int announcements;
static spellcast_datatype queued{};
static bool event_pending;
static P_char queued_caster, queued_victim;
void event_spellcast(P_char, P_char, P_obj, void *);
bool is_obj_in_list_vis(P_char, P_obj, P_obj) { return true; }
void DelayCommune(P_char, int) {}
P_char misfire_check(P_char, P_char target, int) { return target; }
void perform_chaos_check(P_char, P_char, spellcast_datatype *) {}
void say_silent_spell(P_char, int) {}
bool devotion_spell_check(int) { return false; }
bool check_disruptive_blow(P_char) { return false; }
int cast_as_damage_area(P_char, void (*)(int,P_char,char*,int,P_char,P_obj),int,P_char,float,float);
bool divine_blessing_check(P_char, P_char, int) { return false; }
int devotion_skill_check(P_char) { return 0; }
void logit(const char *, const char *, ...) {}
void debug(const char *, ...) {}
void __free(void *p, const char *, int) { free(p); }
int number(int low, int high) { return std::uniform_int_distribution<int>(low, high)(rng); }
float get_property(const char *name, double fallback)
{
    if (!strcmp(name, "spell.area.minChance.deathField")) return minimum;
    return fallback;
}
int IS_MORPH(P_char) { return 0; }
bool ac_can_see(P_char, P_char, bool) { return true; }
void send_to_char(const char *s, P_char ch) { transcript[ch] += s; }
void send_to_char(const char *s, P_char ch, int) { transcript[ch] += s; }
void act(const char *s, int, P_char ch, P_obj, void *vict, int target)
{
    if ((target & ~ACT_NOTTERSE) == TO_VICT) transcript[(P_char)vict] += s;
    if ((target & ~ACT_NOTTERSE) == TO_CHAR) transcript[ch] += s;
}
int raw_damage(P_char, P_char victim, double dam, uint, damage_messages *msg, int *)
{
    ++damaged[victim];
    victim->points.hit -= (int)dam;
    transcript[victim] += msg->victim;
    return DAM_NONEDEAD;
}
int get_property(const char *name, int fallback) { return (int)get_property(name, (double)fallback); }
void *__malloc(size_t n, const char *, const char *, int) { return calloc(1, n); }
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }
bool AdjacentInRoom(P_char, P_char) { return true; }
bool grouped(P_char a, P_char b) { return a == b; }
int is_char_in_room(P_char ch, int room)
{
    for (auto *t = world[room].people; t; t = t->next_in_room) if (ch == t) return true;
    return false;
}
bool has_innate(P_char, int innate) { return evasion && innate == INNATE_EVASION; }
affected_type *get_spell_from_char(P_char, int, void *, int) { return nullptr; }
affected_type *get_ward_from_char(P_char) { return nullptr; }
void affect_remove(P_char, affected_type *) {}
affected_type *affect_to_char(P_char, affected_type *) { return nullptr; }
void affect_from_char(P_char ch, int spell)
{
    if (spell == SPELL_DEFLECT) REMOVE_BIT(ch->specials.affected_by4, AFF4_DEFLECT);
}
void wear_off_message(P_char, affected_type *) {}
bool affected_by_spell(P_char, int) { return false; }
bool affected_by_spell_flagged(P_char, int, uint) { return false; }
P_char get_linking_char(P_char, ush_int) { return nullptr; }
P_char get_linked_char(P_char, ush_int) { return nullptr; }
void clear_links(P_char, ush_int) {}
char_link_data *link_char(P_char, P_char, ush_int) { return nullptr; }
int GET_CHAR_SKILL_P(P_char, int) { return 0; }
int GET_CLASS(P_char ch, uint cls) { return ch->player.m_class & cls; }
int GET_PRIME_CLASS(P_char ch, uint cls) { return GET_CLASS(ch, cls); }
int GET_SECONDARY_CLASS(P_char, uint) { return 0; }
bool notch_skill(P_char, int, float) { return false; }
P_char stack_area(P_char, int, int) { return nullptr; }
void zone_spellmessage(int, bool, const char *, const char *) { ++announcements; }
void CharWait(P_char, int) {}
bool cast_common_generic(P_char, int) { return true; }
void StopCasting(P_char ch) { REMOVE_BIT(ch->specials.affected_by2, AFF2_CASTING); }
void appear(P_char, bool) {}
int BOUNDED(int low, int val, int high) { return std::clamp(val, low, high); }
float BOUNDEDF(float low, float val, float high) { return std::clamp(val, low, high); }
nevent_schedule_result add_event(event_func fn, int, P_char ch, P_char victim, P_obj, int,
                                 const void *data, int size)
{
    assert(fn == event_spellcast && size == sizeof queued);
    queued = *(const spellcast_datatype *)data;
    queued_caster = ch; queued_victim = victim; event_pending = true;
    return {nevent_schedule_status::scheduled, {}};
}
void use_spell(P_char, int) {}
void wizlog(int, const char *, ...) {}
void sql_log(P_char, const char *, const char *, ...) {}
P_char get_random_char_in_room(int, P_char, int) { return nullptr; }
P_char grapple_attack_check(P_char) { return nullptr; }
int grapple_misfire_chance(P_char, P_char, int) { return 0; }
P_char guard_check(P_char, P_char target) { return target; }
bool is_silent(P_char, bool) { return false; }
void say_spell(P_char, int) {}
int char_in_list(P_char ch) { return ch && is_char_in_room(ch, ch->in_room); }
void MobRetaliateRange(P_char, P_char) {}
bool lightbringer_proc(P_char, P_char, bool) { return false; }
int GetLowestSpellCircle(int) { return 9; }
int SpellCastTime(P_char, int) { return 12; }
void SpellCastShow(P_char, int) {}
int STAT_INDEX(int) { return 0; }
void MobStartFight(P_char, P_char) {}
bool hit(P_char, P_char, P_obj, int *) { return false; }
int vamp(P_char, double, double) { return 0; }
void update_pos(P_char) {}
void do_alert(P_char, char *, int) {}
void remember(P_char, P_char) {}
int attack_back(P_char, P_char, int) { return DAM_NONEDEAD; }
bool resists_spell(P_char, P_char) { return resist; }
int get_max_circle(P_char) { return 0; }
int max_spells_in_circle(P_char, int) { return 0; }
void send_to_char_f(P_char, const char *, ...) {}
void DamageStuff(P_char, int) {}

'''

DRIVER = r'''
int main()
{
    for (auto &modifier : spell_damage_modifiers) modifier = no_modifier;
    skills[SPELL_DEATH_FIELD].spell_pointer = spell_death_field;
    skills[SPELL_DEATH_FIELD].targets = TAR_AREA;
    char_data caster{}, targets[8]{};
    npc_only_data npc{};
    pc_only_data players[9]{};
    caster.only.npc = &npc;
    caster.specials.act = ACT_ISNPC;
    caster.player.level = 50;
    caster.player.m_class = CLASS_PSIONICIST;
    caster.specials.position = POS_STANDING | STAT_NORMAL;
    caster.points.hit = 10000;
    for (int i = 0; i < 8; ++i)
    {
        targets[i].only.pc = &players[i];
        targets[i].specials.position = POS_STANDING | STAT_NORMAL;
        targets[i].specials.fighting = &caster;
        targets[i].next_in_room = i == 7 ? nullptr : &targets[i + 1];
    }
    caster.specials.fighting = &targets[0];
    caster.next_in_room = targets;
    world[0].people = &caster;
    auto cast = [&](P_char explicit_target) {
        damaged.clear(); transcript.clear(); announcements = 0;
        for (auto &target : targets) target.points.hit = 10000;
        assert(MobCastSpell(&caster, explicit_target, nullptr, SPELL_DEATH_FIELD, 50));
        int pulses = 0;
        while (event_pending)
        {
            assert(++pulses < 20);
            event_pending = false;
            event_spellcast(queued_caster, queued_victim, nullptr, &queued);
        }
        assert(pulses > 0);
        assert(!IS_CASTING(&caster));
        assert(announcements == 1);
    };
    // Solo opponent reaches real damage handling on every completed cast.
    targets[0].next_in_room = nullptr;
    for (int i = 0; i < 1000; ++i)
    {
        cast(targets);
        assert(damaged[targets] == 1 && targets[0].points.hit < 10000);
        assert(transcript[targets].find("wave of death") != std::string::npos);
    }
    targets[0].next_in_room = &targets[1];
    int secondary_misses = 0;
    for (int i = 0; i < 1000; ++i)
    {
        cast(&targets[7]);
        assert(damaged[targets] == 1 && damaged[&targets[7]] == 1);
        if (!damaged[&targets[1]]) ++secondary_misses;
    }
    assert(secondary_misses > 0); // other eligible PCs still undergo pruning
    // Low hit settings must not spin when both remaining PCs are protected.
    minimum = 0;
    targets[1].next_in_room = nullptr;
    for (int i = 0; i < 100; ++i) { cast(&targets[1]); assert(damaged[targets] && damaged[&targets[1]]); }
    minimum = 60;
    targets[1].next_in_room = &targets[2];
    // Eligibility remains authoritative even for the NPC melee opponent.
    targets[0].specials.z_cord = 1;
    cast(&targets[7]); assert(!damaged[targets]);
    targets[0].specials.z_cord = 0;
    world[0].room_flags = ROOM_SAFE;
    cast(&targets[7]); assert(damaged.empty());
    world[0].room_flags = 0;
    targets[0].specials.position = STAT_DEAD;
    cast(&targets[7]); assert(!damaged[targets]);
    targets[0].specials.position = POS_STANDING | STAT_NORMAL;
    // Full ward absorption and resistance remain legitimate no-HP-loss outcomes.
    targets[0].next_in_room = nullptr;
    targets[0].points.ward = 10000;
    cast(targets); assert(!damaged[targets] && targets[0].points.ward < 10000);
    assert(transcript[targets].find("ward around you") != std::string::npos);
    targets[0].points.ward = 0;
    resist = true;
    cast(targets); assert(!damaged[targets]);
    resist = false;
    targets[0].specials.affected_by4 = AFF4_DEFLECT;
    cast(targets);
    assert(!damaged[targets] && damaged[&caster] == 1);
    assert(transcript[targets].find("deflecting") != std::string::npos);
    targets[0].player.m_class = CLASS_MONK;
    targets[0].player.spec = SPEC_WAYOFSNAKE;
    targets[0].player.level = 200; // force the real area-evasion threshold
    evasion = true;
    cast(targets); assert(!damaged[targets]);
    assert(transcript[targets].find("twist out of the way") != std::string::npos);
    evasion = false;
    targets[0].player.m_class = 0;
    // PC casters retain the existing explicit-target-only pruning policy.
    targets[0].next_in_room = &targets[1];
    caster.specials.act = 0;
    caster.only.pc = &players[8];
    int melee_misses = 0;
    for (int i = 0; i < 1000; ++i)
    {
        damaged.clear();
        spell_death_field(50, &caster, nullptr, SPELL_TYPE_SPELL, &targets[7], nullptr);
        assert(damaged[&targets[7]] == 1);
        if (!damaged[targets]) ++melee_misses;
    }
    assert(melee_misses > 0);
    // Charmed pets and switched immortal bodies use player-caster pruning policy.
    caster.only.npc = &npc;
    caster.specials.act = ACT_ISNPC;
    SET_BIT(caster.specials.affected_by, AFF_CHARM);
    melee_misses = 0;
    for (int i = 0; i < 1000; ++i)
    {
        damaged.clear();
        spell_death_field(50, &caster, nullptr, SPELL_TYPE_SPELL, &targets[7], nullptr);
        assert(damaged[&targets[7]] == 1);
        if (!damaged[targets]) ++melee_misses;
    }
    assert(melee_misses > 0);
    REMOVE_BIT(caster.specials.affected_by, AFF_CHARM);
    descriptor_data descriptor{};
    char_data immortal{};
    pc_only_data immortal_pc{};
    immortal.only.pc = &immortal_pc;
    descriptor.original = &immortal;
    caster.desc = &descriptor;
    melee_misses = 0;
    for (int i = 0; i < 1000; ++i)
    {
        damaged.clear();
        spell_death_field(50, &caster, nullptr, SPELL_TYPE_SPELL, &targets[7], nullptr);
        assert(damaged[&targets[7]] == 1);
        if (!damaged[targets]) ++melee_misses;
    }
    assert(melee_misses > 0);
    puts("Death Field cast completion: autonomous NPC targeting, eligibility, defenses, and player-controlled pruning passed");
}
'''


def main():
    build = ROOT / 'bin/tests'
    build.mkdir(parents=True, exist_ok=True)
    functions = [
        ('utility.c', 'bool should_area_hit(P_char ch, P_char victim)'),
        ('utility.c', 'int cast_as_damage_area(P_char ch, void (*spell_func)(int, P_char, char *, int, P_char, P_obj),\n\t\t\tint level, P_char victim, float min_chance, float /*chance_step*/,\n\t\t\tbool (*select_func)(P_char, P_char))'),
        ('utility.c', 'int cast_as_damage_area(P_char ch, void (*spell_func)(int, P_char, char *, int, P_char, P_obj),\n\t\t\tint level, P_char victim, float min_chance, float chance_step)'),
        ('fight.c', 'int check_damage_ward(P_char attacker,'),
        ('fight.c', 'int spell_damage(P_char ch,'),
        ('psionics.c', 'void spell_single_death_field('),
        ('psionics.c', 'void spell_death_field('),
        ('sparser.c', 'void event_spellcast(P_char ch,'),
        ('mobact.c', 'bool MobCastSpell(P_char ch,'),
    ]
    with tempfile.TemporaryDirectory(prefix='death-field-', dir=build) as directory:
        source, binary = Path(directory) / 'harness.cpp', Path(directory) / 'harness'
        source.write_text('\n'.join([PRELUDE, *[extract_function(*f) for f in functions], DRIVER]))
        subprocess.run(['g++', '-std=c++20', '-g', '-O1', '-fsanitize=address,undefined',
                        '-Isrc', '-D__NO_MYSQL__', '-Isrc/no_mysql', str(source), '-o', str(binary)], cwd=ROOT, check=True, timeout=120)
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == '__main__':
    main()
