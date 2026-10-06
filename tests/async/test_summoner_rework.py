#!/usr/bin/env python3
"""Execute production summoner bodies/resources with isolated engine services."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(path, signature):
    text = (ROOT / path).read_text()
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


necro = (ROOT / "src/classes/necromancy.c").read_text()
tables = "\n".join(re.search(r"const struct " + name + r"_description " + name +
                            r"_data\[[^;]+?\] = .*?\n?\s*};", necro, re.S)[0]
                   for name in ("golem", "undead"))
skill_source = (ROOT / "src/classes/skills.c").read_text()
skill_setup = ""
for block in skill_source.split("SPELL_CREATE(")[1:]:
    spell = re.search(r'"[^"]+",\s*(SPELL_\w+)', block)
    circle = re.search(r"SPELL_ADD\(CLASS_NECROMANCER,\s*(\d+)\)", block)
    if spell and circle:
        skill_setup += (f"skills[{spell[1]}].m_class[flag2idx(CLASS_NECROMANCER)-1]"
                        f".rlevel[0] = {circle[1]};\n")

HARNESS = r'''
#include "classes/summoner_pet.c"
#include "net/comm.h"
#include "world/epic_bonus.h"
#include "combat/spell_wards.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>

Skill skills[MAX_AFFECT_TYPES];
stat_data stat_factor[LAST_RACE + 1] = {};
room_data rooms[2] = {};
P_room world = rooms;
unsigned long long ne_event_tick = 1000;
int spl_table[TOTALLVLS][MAX_CIRCLE];
int get_innate_regeneration(P_char);
int difficulty_scale_player_regen(P_char, int);
#undef IS_SUNLIT
#define IS_SUNLIT(room) false
__TABLES__
__ENGINE_FUNCTIONS__
std::map<P_char, P_char> masters, songs;
std::set<int> recipes;
bool chaos = false;
int infuse = 100, healing_skill = 100, terrain = 0;
int exhaustion = 0, stopped = 0, recovery_schedules = 0;
P_char message_owner = nullptr;
int flag2idx(int mask) { int index = 0; while (mask) { ++index; mask >>= 1; } return index; }
int GET_CHAR_SKILL_P(P_char, int skill) {
    return skill == SKILL_INFUSE_LIFE ? infuse : skill == SONG_HEALING ? healing_skill : 100;
}
P_char get_linked_char(P_char ch, ush_int type) {
    return type == LNK_PET ? masters[ch] : type == LNK_SONG ? songs[ch] : nullptr;
}
bool is_linked_to(P_char bard, P_char target, ush_int) { return songs[target] == bard; }
bool grouped(P_char, P_char) { return true; }
bool affected_by_spell(P_char ch, int spell) {
    for (auto *af = ch->affected; af; af = af->next) if (af->type == spell) return true;
    return false;
}
bool has_skin_spell(P_char) { return false; }
float get_property(const char *, double value) { return value; }
int get_property(const char *, int value) { return value; }
int get_spell_circle(P_char, int) { assert(false); return 0; }
bool has_innate(P_char, int) { return false; }
int get_innate_regeneration(P_char) { return 100; }
float get_epic_bonus(P_char, int) { return 0; }
int difficulty_scale_player_regen(P_char, int gain) { return gain; }
room_affect *get_spell_from_room(P_room, int) { return nullptr; }
bool IS_TWILIGHT_ROOM(int) { return false; }
bool IS_OUTDOORS(int) { return false; }
char *str_dup(const char *value) { return strdup(value); }
void str_free(const char *value) { free(const_cast<char *>(value)); }
affected_type *affect_to_char(P_char ch, affected_type *af) {
    auto *copy = new affected_type(*af); copy->next = ch->affected; ch->affected = copy;
    ch->specials.affected_by |= af->bitvector; ch->specials.affected_by2 |= af->bitvector2;
    ch->specials.affected_by3 |= af->bitvector3; ch->specials.affected_by4 |= af->bitvector4;
    ch->specials.affected_by5 |= af->bitvector5;
    return copy;
}
void affect_from_char(P_char ch, int spell) {
    auto **entry = &ch->affected;
    while (*entry) {
        auto *af = *entry;
        if (af->type != spell) { entry = &af->next; continue; }
        ch->specials.affected_by &= ~af->bitvector;
        ch->specials.affected_by2 &= ~af->bitvector2;
        ch->specials.affected_by3 &= ~af->bitvector3;
        ch->specials.affected_by4 &= ~af->bitvector4;
        ch->specials.affected_by5 &= ~af->bitvector5;
        *entry = af->next; delete af;
    }
    summoner_pet_finish_affects(ch);
}
affected_type *spell_ward_apply_cast(P_char pet, const affected_type *prototype,
                                   int duration) {
    affected_type af = *prototype; af.duration = duration;
    return affect_to_char(pet, &af);
}
void act(const char *message, int, P_char owner, P_obj, void *, int) {
    if (std::strstr(message, "looks too exhausted for that")) { ++exhaustion; message_owner = owner; }
}
P_nevent get_scheduled(P_char, event_func_type) { return nullptr; }
nevent_schedule_result add_event(event_func fn, int, P_char, P_char, P_obj, int, const void *, int) {
    if (fn == event_summoner_recovery) ++recovery_schedules;
    return {nevent_schedule_status::scheduled, {}};
}
void stop_singing(P_char bard) { ++stopped; bard->specials.affected_by3 &= ~AFF3_SINGING; }
int conjure_terrain_check(P_char, P_char) { return terrain; }
void refresh_npc_spell_slots(P_char pet) {
    for (int circle = 1; circle <= MAX_CIRCLE; ++circle)
        pet->specials.undead_spell_slots[circle] = spl_table[GET_LEVEL(pet)][circle-1];
}
void summoned_pet_mark(P_char pet, summoned_pet_kind kind) {
    pet->only.npc->summon_kind = static_cast<uint32_t>(kind);
    const uint64_t intrinsic[] = {pet->specials.affected_by & ~AFF_CHARM,
      pet->specials.affected_by2, pet->specials.affected_by3,
      pet->specials.affected_by4, pet->specials.affected_by5};
    std::copy(std::begin(intrinsic), std::end(intrinsic), pet->only.npc->summon_intrinsic_affects);
}
bool player_save_pipeline_mark(int, player_component_mask_t) { return true; }
bool chaos_mud_enabled() { return chaos; }
bool sql_has_spellbook_mob(int, int vnum) { return recipes.contains(vnum); }
bool sql_add_spellbook_mob(int, int vnum) { recipes.insert(vnum); return true; }
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
void send_to_char(const char *, P_char) {}
bool isname(const char *, const char *) { return false; }
void stop_follower(P_char) { assert(false); }
int setup_pet(P_char, P_char, int, int) { assert(false); return 0; }
void event_pet_death(P_char, P_char, P_obj, void *) {}

struct Fixture {
    char_data ch = {};
    npc_only_data npc = {};
    pc_only_data pc = {};
    Fixture(bool is_npc, int race, uint32_t classes, int level) {
        ch.specials.act = is_npc ? ACT_ISNPC : 0;
        if (is_npc) ch.only.npc = &npc; else ch.only.pc = &pc;
        pc.pid = 123;
        ch.player.level = level; ch.player.race = race; ch.player.m_class = classes;
        ch.specials.position = STAT_NORMAL + POS_STANDING;
        GET_COND(&ch, FULL) = GET_COND(&ch, THIRST) = 24;
        for (int i = 0; i < 10; ++i) ch.base_stats[i] = ch.curr_stats[i] = 100;
        ch.in_room = 1;
        ch.points.hit = ch.points.max_hit = ch.points.base_hit = 90000;
        ch.points.base_damroll = ch.points.damroll = level >= 51 ? 30 + level / 2 : 20 + level / 2;
        ch.points.mana = ch.points.max_mana = ch.points.base_mana = 30000;
    }
    ~Fixture() {
        while (ch.affected) { auto *af = ch.affected; ch.affected = af->next; delete af; }
        if (npc.str_mask & STRUNG_KEYS) free(ch.player.name);
        if (npc.str_mask & STRUNG_DESC2) free(ch.player.short_descr);
        if (npc.str_mask & STRUNG_DESC1) free(ch.player.long_descr);
    }
};
void publish(Fixture &pet, Fixture &owner) {
    const std::array<uint64_t,5> traits = {pet.ch.specials.affected_by, pet.ch.specials.affected_by2,
      pet.ch.specials.affected_by3, pet.ch.specials.affected_by4, pet.ch.specials.affected_by5};
    summoner_pet_configure(&pet.ch, &owner.ch);
    masters[&pet.ch] = &owner.ch;
    summoner_pet_cast_buffs(&pet.ch, traits);
    summoner_pet_sync_resources(&pet.ch);
}
void clear_reserve(Fixture &owner) {
    for (auto *af = owner.ch.affected; af; af = af->next)
        if (af->type == TAG_SUMMONER_RESOURCE) af->modifier = 0;
}

int main() {
    SetSpellCircles();
    skills[SPELL_HASTE].spell_pointer = spell_haste;
    skills[SPELL_BLUR].spell_pointer = spell_blur;
    skills[SPELL_INVISIBILITY].spell_pointer = spell_invisibility;
    skills[SPELL_DETECT_INVISIBLE].spell_pointer = spell_detect_invisibility;
    skills[SPELL_REGENERATION].spell_pointer = spell_regeneration;
    skills[SPELL_GLOBE].spell_pointer = spell_globe;
    skills[SPELL_VAMPIRE].spell_pointer = spell_vampire;
    __SKILLS__
    Fixture owner(false, RACE_HUMAN, CLASS_SUMMONER, 56);
    Fixture bran(true, RACE_WIGHT, CLASS_WARRIOR | CLASS_CLERIC | CLASS_ANTIPALADIN, 56);
    bran.ch.player.spec = 6;
    bran.ch.specials.affected_by4 = AFF4_MULTI_CLASS;
    bran.ch.specials.affected_by |= AFF_HASTE | AFF_SLEEP;
    bran.ch.specials.affected_by2 |= AFF2_FLURRY | AFF2_CASTING;
    bran.ch.specials.affected_by3 |= AFF3_BLUR | AFF3_FOUR_ARMS;
    bran.ch.specials.affected_by4 |= AFF4_REGENERATION;
    GET_SIZE(&bran.ch) = SIZE_HUGE;
    bran.ch.specials.act |= ACT_ELITE | ACT_NO_BASH | ACT_IGNORE;
    publish(bran, owner);
    assert(GET_LEVEL((&bran.ch)) == 56 && bran.ch.player.spec == 6);
    assert(bran.ch.player.m_class == (CLASS_WARRIOR | CLASS_CLERIC | CLASS_ANTIPALADIN));
    assert(GET_SIZE(&bran.ch) == SIZE_HUGE);
    assert(!(bran.ch.specials.act & (ACT_ELITE | ACT_NO_BASH | ACT_IGNORE)));
    assert(IS_AFFECTED(&bran.ch, AFF_HASTE) && IS_AFFECTED2(&bran.ch, AFF2_FLURRY));
    assert(IS_AFFECTED3(&bran.ch, AFF3_BLUR) && HAS_FOUR_HANDS(&bran.ch));
    assert(affected_by_spell(&bran.ch, SPELL_HASTE) && affected_by_spell(&bran.ch, SPELL_BLUR));
    assert(affected_by_spell(&bran.ch, SPELL_REGENERATION));
    assert(!(bran.npc.summon_intrinsic_affects[0] & AFF_HASTE));
    assert(!(bran.npc.summon_intrinsic_affects[2] & AFF3_BLUR));
    assert(bran.npc.summon_intrinsic_affects[2] & AFF3_FOUR_ARMS);
    int haste_duration = 0, regeneration_duration = 0;
    for (auto *af = bran.ch.affected; af; af = af->next) {
        if (af->type == SPELL_HASTE) haste_duration = af->duration;
        if (af->type == SPELL_REGENERATION) regeneration_duration = af->duration;
    }
    assert(haste_duration == 10 && regeneration_duration == 5); // production spell durations
    affect_from_char(&bran.ch, SPELL_HASTE);
    affect_from_char(&bran.ch, SPELL_BLUR);
    affect_from_char(&bran.ch, SPELL_REGENERATION);
    assert(!IS_AFFECTED(&bran.ch, AFF_HASTE) && !IS_AFFECTED3(&bran.ch, AFF3_BLUR));
    assert(!IS_AFFECTED4(&bran.ch, AFF4_REGENERATION) && HAS_FOUR_HANDS(&bran.ch));
    summoner_pet_configure(&bran.ch, &owner.ch, false, true);
    assert(!affected_by_spell(&bran.ch, SPELL_HASTE)); // retraining does not cast again
    Fixture spawn_bard(true, RACE_GREY, CLASS_BARD, 56);
    spawn_bard.ch.specials.affected_by |= AFF_HASTE | AFF_INVISIBLE | AFF_DETECT_INVISIBLE;
    spawn_bard.ch.specials.affected_by2 |= AFF2_GLOBE;
    publish(spawn_bard, owner);
    assert(affected_by_spell(&spawn_bard.ch, SPELL_INVISIBILITY));
    assert(affected_by_spell(&spawn_bard.ch, SPELL_DETECT_INVISIBLE));
    assert(affected_by_spell(&spawn_bard.ch, SPELL_GLOBE));
    for (auto *af = spawn_bard.ch.affected; af; af = af->next)
        if (af->type == SPELL_GLOBE) assert(af->duration == 8);
    assert(!(spawn_bard.npc.summon_intrinsic_affects[1] & AFF2_GLOBE));
    Fixture vampire_capture(true, RACE_WIGHT, CLASS_WARRIOR | CLASS_CLERIC, 56);
    vampire_capture.ch.specials.affected_by4 |= AFF4_VAMPIRE_FORM;
    publish(vampire_capture, owner);
    assert(affected_by_spell(&vampire_capture.ch, SPELL_VAMPIRE));
    assert(!(vampire_capture.npc.summon_intrinsic_affects[3] & AFF4_VAMPIRE_FORM));
    for (int circle = 1; circle <= get_max_circle(&vampire_capture.ch); ++circle)
        assert(vampire_capture.ch.specials.undead_spell_slots[circle] ==
               max_spells_in_circle(&vampire_capture.ch, circle)); // assimilation cannot erase pool
    Fixture perm_air(true, RACE_A_ELEMENTAL, CLASS_MERCENARY | CLASS_ASSASSIN | CLASS_BARD, 53);
    perm_air.ch.specials.affected_by |= AFF_HASTE | AFF_INVISIBLE | AFF_DETECT_INVISIBLE;
    perm_air.ch.specials.affected_by2 |= AFF2_GLOBE;
    perm_air.ch.specials.affected_by3 |= AFF3_TOWER_IRON_WILL;
    perm_air.ch.specials.affected_by4 |= AFF4_REGENERATION | AFF4_DEFLECT;
    perm_air.ch.specials.act |= ACT_BREATHES_LIGHTNING;
    publish(perm_air, owner);
    assert(!affected_by_spell(&perm_air.ch, SPELL_HASTE)); // intrinsic, no timed casts
    assert(IS_AFFECTED(&perm_air.ch, AFF_HASTE));
    assert(IS_AFFECTED(&perm_air.ch, AFF_INVISIBLE));
    assert(IS_AFFECTED(&perm_air.ch, AFF_DETECT_INVISIBLE));
    assert(IS_AFFECTED2(&perm_air.ch, AFF2_GLOBE));
    assert(IS_AFFECTED3(&perm_air.ch, AFF3_TOWER_IRON_WILL));
    assert(IS_AFFECTED4(&perm_air.ch, AFF4_REGENERATION) && IS_AFFECTED4(&perm_air.ch, AFF4_DEFLECT));
    assert(IS_ACT(&perm_air.ch, ACT_BREATHES_LIGHTNING));
    spell_detect_invisibility(53, &perm_air.ch, nullptr, SPELL_TYPE_SPELL, &perm_air.ch, nullptr);
    affect_from_char(&perm_air.ch, SPELL_DETECT_INVISIBLE);
    assert(IS_AFFECTED(&perm_air.ch, AFF_DETECT_INVISIBLE)); // expiration cannot erase native bit
    affected_type suppress = {}; suppress.type = TAG_SUPPRESS_PERM_BITS;
    suppress.bitvector = AFF_DETECT_INVISIBLE;
    affect_to_char(&perm_air.ch, &suppress);
    perm_air.ch.specials.affected_by &= ~AFF_DETECT_INVISIBLE;
    summoner_pet_finish_affects(&perm_air.ch);
    assert(!IS_AFFECTED(&perm_air.ch, AFF_DETECT_INVISIBLE)); // normal suppression still applies
    affect_from_char(&perm_air.ch, TAG_SUPPRESS_PERM_BITS);
    assert(IS_AFFECTED(&perm_air.ch, AFF_DETECT_INVISIBLE));
    assert(!IS_AFFECTED(&bran.ch, AFF_SLEEP) && !IS_AFFECTED2(&bran.ch, AFF2_CASTING));
    assert(test_strength(&bran.ch, 0, 0) == 118);
    stat_factor[RACE_WIGHT].Str = 500; // wild config cannot change trained Strength
    assert(test_strength(&bran.ch, 0, 0) == 118);
    assert(test_strength(&bran.ch, 20, 20) == 142); // normal max/stat buffs still work
    assert(test_strength(&bran.ch, 0, 0) == 118); // repeated rebuilds do not compound
    assert(summoner_pet_strength_factor(RACE_SNOW_OGRE) == 175);
    assert(summoner_pet_strength_factor(RACE_E_ELEMENTAL) == 145);
    assert(summoner_pet_strength_factor(RACE_DRAGONKIN) == 120);
    assert(summoner_pet_strength_factor(RACE_GREY) == 100);
    assert(GET_MAX_MANA(&bran.ch) == 448 && GET_MAX_HIT(&bran.ch) <= necro_hp_ceiling(&owner.ch));
    const int bran_hp = GET_MAX_HIT(&bran.ch);
    assert(bran.ch.points.base_damroll == 58);
    bran.ch.points.damroll = 200;
    summoner_pet_finish_affects(&bran.ch);
    assert(bran.ch.points.damroll == 100 && bran.ch.points.base_damroll == 58);
    bran.ch.points.damroll = 88;
    summoner_pet_finish_affects(&bran.ch);
    assert(bran.ch.points.damroll == 88); // the cap does not grant damage
    bran.ch.curr_stats.Pow = 500;
    assert(summoner_pet_heal_cap(&bran.ch, bran_hp * 3) == bran_hp * 11 / 10);
    assert(summoner_pet_heal_cap(&bran.ch, bran_hp) == bran_hp);
    bran.ch.points.hit = bran_hp * 3;
    summoner_pet_finish_affects(&bran.ch);
    assert(GET_HIT(&bran.ch) == bran_hp * 11 / 10);
    assert(summoner_pet_vamp_rate(&bran.ch, .8) == .25);
    assert(summoner_pet_vamp_rate(&bran.ch, .5, true) == .10);
    assert(summoner_pet_vamp_rate(&bran.ch, .05) == .05);
    Fixture untouched(true, RACE_F_ELEMENTAL, CLASS_WARRIOR, 55);
    untouched.npc.summon_kind = static_cast<uint32_t>(summoned_pet_kind::conjurer_elemental);
    stat_factor[RACE_F_ELEMENTAL].Str = 210;
    assert(test_strength(&untouched.ch, 0, 0) == 210); // other pet kinds keep racial stats
    untouched.ch.points.damroll = 150;
    summoner_pet_finish_affects(&untouched.ch);
    assert(!summoner_balanced_body(&untouched.ch));
    assert(untouched.ch.points.damroll == 150 && GET_MAX_HIT(&untouched.ch) == 90000);
    assert(summoner_pet_heal_cap(&untouched.ch, 270000) == 270000);
    assert(summoner_pet_vamp_rate(&untouched.ch, .8) == .8);
    assert(summoner_pet_heal_cap(&owner.ch, 270000) == 270000);
    assert(summoner_pet_vamp_rate(&owner.ch, .7) == .7);
    Fixture aden(true, RACE_GREY, CLASS_BARD, 56);
    Fixture xavier(true, RACE_HUMAN, CLASS_WARLOCK | CLASS_ETHERMANCER, 56);
    owner.ch.curr_stats.Cha = 130;
    Fixture actual_bran(true, RACE_WIGHT, CLASS_WARRIOR | CLASS_CLERIC | CLASS_ANTIPALADIN, 56);
    summoner_pet_configure(&actual_bran.ch, &owner.ch, true);
    assert(GET_MAX_HIT(&actual_bran.ch) == 1603);
    assert(actual_bran.ch.points.base_damroll == 58);
    summoner_pet_configure(&aden.ch, &owner.ch, true);
    summoner_pet_configure(&xavier.ch, &owner.ch, true);
    assert(GET_MAX_HIT(&aden.ch) == 835); // racial profile alone would have raised this to 973
    assert(GET_MAX_HIT(&xavier.ch) == 974); // racial profile alone would have raised this to 1317
    assert(GET_MAX_HIT(&aden.ch) == normal_capture_hp(&aden.ch, &owner.ch));
    chaos = true;
    summoner_pet_configure(&aden.ch, &owner.ch, true);
    assert(GET_MAX_HIT(&aden.ch) == 835); // no wild Chaos division by ten
    chaos = false;
    owner.ch.curr_stats.Cha = 100;
    bran.ch.points.base_hit = GET_MAX_HIT(&bran.ch) = 999999;
    summoner_pet_configure(&bran.ch, &owner.ch, true);
    assert(GET_MAX_HIT(&bran.ch) == bran_hp); // prototype difficulty does not change trained body
    Fixture tiny(true, RACE_GNOME, CLASS_WARRIOR, 56);
    Fixture giant(true, RACE_SGIANT, CLASS_WARRIOR, 56);
    summoner_pet_configure(&tiny.ch, &owner.ch, true);
    summoner_pet_configure(&giant.ch, &owner.ch, true);
    assert(GET_MAX_HIT(&tiny.ch) < GET_MAX_HIT(&giant.ch));
    assert(GET_MAX_HIT(&giant.ch) <= 5000);
    assert(GET_MAX_HIT(&giant.ch) <= normal_capture_hp(&giant.ch, &owner.ch));
    Fixture snow_ogre(true, RACE_SNOW_OGRE, CLASS_WARRIOR, 56);
    summoner_pet_configure(&snow_ogre.ch, &owner.ch, true);
    assert(GET_MAX_HIT(&snow_ogre.ch) == 4546); // CHA 100, full infusion
    owner.ch.curr_stats.Cha = 130;
    summoner_pet_configure(&snow_ogre.ch, &owner.ch, true);
    assert(GET_MAX_HIT(&snow_ogre.ch) == 5000); // tough captures can reach the new cap
    assert(normal_capture_hp(&snow_ogre.ch, &owner.ch) == 5427);
    assert(snow_ogre.npc.summoner_hp_ceiling == 5000);
    assert(snow_ogre.ch.points.base_damroll == 58);
    snow_ogre.ch.points.max_hit = snow_ogre.ch.points.hit = 8000;
    snow_ogre.npc.summoner_hp_ceiling = 8000; // an old stored ceiling cannot bypass the hard cap
    summoner_pet_finish_affects(&snow_ogre.ch);
    assert(GET_MAX_HIT(&snow_ogre.ch) == 5000 && GET_HIT(&snow_ogre.ch) == 5000);
    assert(summoner_pet_heal_cap(&snow_ogre.ch, 8000) == 5500);
    chaos = true;
    summoner_pet_configure(&snow_ogre.ch, &owner.ch, true);
    assert(GET_MAX_HIT(&snow_ogre.ch) == 5000);
    chaos = false;
    Fixture ordinary_ogre(true, RACE_OGRE, CLASS_WARRIOR, 56);
    summoner_pet_configure(&ordinary_ogre.ch, &owner.ch, true);
    assert(GET_MAX_HIT(&ordinary_ogre.ch) == 2545); // the higher cap does not grant HP to every race
    owner.ch.curr_stats.Cha = 100;
    Fixture early_owner(false, RACE_HUMAN, CLASS_SUMMONER, 21);
    Fixture early_pet(true, RACE_SGIANT, CLASS_WARRIOR, 26);
    summoner_pet_configure(&early_pet.ch, &early_owner.ch, true);
    assert(GET_LEVEL((&early_pet.ch)) == 21);
    assert(GET_MAX_HIT(&early_pet.ch) <= normal_capture_hp(&early_pet.ch, &early_owner.ch));

    // Ordinary captures retain lower prototype levels; higher ones clamp to the owner.
    // Greater-orb captures retain their above-56 level, including on restoration.
    Fixture lower_pet(true, RACE_HUMAN, CLASS_WARRIOR, 20);
    summoner_pet_configure(&lower_pet.ch, &early_owner.ch, true);
    assert(GET_LEVEL((&lower_pet.ch)) == 20);
    Fixture orb_pet(true, RACE_WIGHT, CLASS_WARRIOR, 61);
    publish(orb_pet, owner);
    assert(GET_LEVEL((&orb_pet.ch)) == 61 && GET_MAX_MANA(&orb_pet.ch) == 488);
    assert(GET_MAX_HIT(&orb_pet.ch) <= 5000);
    assert(summoner_pet_spend(&orb_pet.ch, 24));
    const int orb_count = orb_pet.ch.points.damnodice, orb_sides = orb_pet.ch.points.damsizedice;
    summoner_pet_configure(&orb_pet.ch, &owner.ch, false, true);
    assert(GET_LEVEL((&orb_pet.ch)) == 61 && GET_MANA(&orb_pet.ch) == 464);
    assert(orb_pet.ch.points.damnodice == orb_count && orb_pet.ch.points.damsizedice == orb_sides);

    Fixture capacity_owner(false, RACE_HUMAN, CLASS_SUMMONER, 31);
    Fixture first_pet(true, RACE_HUMAN, CLASS_WARRIOR, 20);
    Fixture second_pet(true, RACE_HUMAN, CLASS_WARRIOR, 20);
    Fixture selected_pet(true, RACE_HUMAN, CLASS_WARRIOR, 35);
    follow_type first = {}, second = {};
    first.follower = &first_pet.ch; first.next = &second; second.follower = &second_pet.ch;
    capacity_owner.ch.followers = &first;
    assert(new_summon_check(&capacity_owner.ch, &selected_pet.ch)); // 20+20+31 <= 72
    selected_pet.ch.player.level = 37;
    assert(!new_summon_check(&capacity_owner.ch, &selected_pet.ch)); // original eligibility stays +5
    capacity_owner.ch.player.level = 56;
    first_pet.ch.player.level = second_pet.ch.player.level = 32;
    assert(!new_summon_check(&capacity_owner.ch, &orb_pet.ch)); // 32+32+61 > 122
    first_pet.ch.player.level = 29;
    assert(new_summon_check(&capacity_owner.ch, &orb_pet.ch)); // 29+32+61 == 122

    for (int habitat : {-1, 0, 1}) {
        terrain = habitat;
        Fixture elemental(true, RACE_E_ELEMENTAL, CLASS_WARRIOR | CLASS_CLERIC, 56);
        Fixture heater(true, RACE_F_ELEMENTAL, CLASS_WARRIOR, 56);
        GET_SIZE(&elemental.ch) = SIZE_GARGANTUAN;
        // Live capture uses terrain and the legacy ceiling; the heater builder is an upper benchmark.
        summoner_pet_configure(&elemental.ch, &owner.ch);
        summoner_elemental_body(&heater.ch, &owner.ch, true, habitat, 700, 25);
        assert(GET_MAX_HIT(&elemental.ch) <= necro_hp_ceiling(&owner.ch));
        assert(GET_MAX_HIT(&elemental.ch) <= GET_MAX_HIT(&heater.ch));
        assert(elemental.ch.points.base_damroll <= heater.ch.points.base_damroll);
        assert(GET_LEVEL((&elemental.ch)) == 56);
        assert(get_max_circle(&elemental.ch) == 12);
        assert(GET_MAX_HIT(&elemental.ch) <= normal_capture_hp(&elemental.ch, &owner.ch));
        assert(elemental.ch.player.m_class == (CLASS_WARRIOR | CLASS_CLERIC));
        assert(GET_SIZE(&elemental.ch) == SIZE_GARGANTUAN);
    }

    // Only specialized captures gain the bonuses; even forged pre-30 specs do not.
    Fixture mental_owner(false, RACE_HUMAN, CLASS_SUMMONER, 29);
    mental_owner.ch.player.spec = SPEC_MENTALIST;
    Fixture mental_pet(true, RACE_A_ELEMENTAL, CLASS_WARRIOR | CLASS_CLERIC, 56);
    GET_SIZE(&mental_pet.ch) = SIZE_TINY;
    terrain = 0;
    summoner_pet_configure(&mental_pet.ch, &mental_owner.ch, true);
    assert(GET_MAX_MANA(&mental_pet.ch) == 29 * 4);
    assert(GET_MAX_HIT(&mental_pet.ch) <= normal_capture_hp(&mental_pet.ch, &mental_owner.ch));
    mental_owner.ch.player.level = 30;
    mental_pet.ch.player.level = 30;
    summoner_pet_configure(&mental_pet.ch, &mental_owner.ch, true);
    assert(GET_MAX_MANA(&mental_pet.ch) == 150);
    assert(GET_MAX_HIT(&mental_pet.ch) == 215); // old 195-HP body dominates the bounded blend
    assert(mental_pet.ch.points.base_damroll == 11); // matched lesser: 10
    assert(GET_MAX_HIT(&mental_pet.ch) > normal_capture_hp(&mental_pet.ch, &mental_owner.ch));
    summoner_pet_finish_affects(&mental_pet.ch); // pre-owner-link affect rebuild retains 5L
    assert(GET_MAX_MANA(&mental_pet.ch) == 150 && GET_SIZE(&mental_pet.ch) == SIZE_TINY);
    masters[&mental_pet.ch] = &mental_owner.ch;
    assert(summoner_pet_memtime(&mental_pet.ch, 100) == 80);
    assert(summoner_pet_memtime(&mental_pet.ch, 1) == 1);
    mental_owner.ch.specials.fighting = &owner.ch;
    assert(summoner_pet_recovery_blocked(&mental_pet.ch));
    mental_owner.ch.specials.fighting = nullptr;
    mental_owner.ch.player.level = 41;
    summoner_pet_configure(&mental_pet.ch, &mental_owner.ch, true);
    assert(GET_MAX_MANA(&mental_pet.ch) == 30 * 6);
    mental_owner.ch.player.level = 56;
    mental_owner.ch.curr_stats.Cha = 130;
    for (int habitat : {-1, 0, 1}) {
        terrain = habitat;
        Fixture fire(true, RACE_F_ELEMENTAL, CLASS_WARRIOR, 56);
        GET_SIZE(&fire.ch) = SIZE_SMALL;
        summoner_pet_configure(&fire.ch, &mental_owner.ch);
        // The old 2494-HP body dominates; terrain only changes the blend by 5%.
        assert(GET_MAX_HIT(&fire.ch) == (habitat == 1 ? 2357 : habitat == 0 ? 2245 : 2133));
        assert(fire.ch.points.base_damroll == (habitat == 1 ? 61 : 33));
        assert(GET_MAX_MANA(&fire.ch) == 336 && GET_LEVEL((&fire.ch)) == 56);
        assert(get_max_circle(&fire.ch) == 12);
        assert(GET_MAX_HIT(&fire.ch) <= necro_hp_ceiling(&mental_owner.ch));
        assert(GET_SIZE(&fire.ch) == SIZE_SMALL);
        assert(bool(fire.ch.specials.affected_by & AFF_HASTE) == (habitat == 1));
    }
    terrain = 0;
    Fixture earth(true, RACE_E_ELEMENTAL, CLASS_WARRIOR, 56);
    summoner_pet_configure(&earth.ch, &mental_owner.ch, true);
    assert(GET_MAX_HIT(&earth.ch) == 2271);
    assert(earth.ch.points.base_damroll == 39); // earth king peak: 35
    Fixture air(true, RACE_A_ELEMENTAL, CLASS_WARRIOR, 56);
    summoner_pet_configure(&air.ch, &mental_owner.ch, true);
    assert(GET_MAX_HIT(&air.ch) == 1190);
    assert(air.ch.points.base_damroll == 28);
    assert(GET_MAX_HIT(&air.ch) < GET_MAX_HIT(&earth.ch));

    // Actual Library capture classes preserve their original HP difference.
    for (int habitat : {-1, 0, 1}) {
        terrain = habitat;
        Fixture library_air(true, RACE_A_ELEMENTAL, 90112U, 53);
        Fixture library_earth(true, RACE_E_ELEMENTAL, 90112U, 53);
        summoner_pet_configure(&library_air.ch, &mental_owner.ch);
        summoner_pet_configure(&library_earth.ch, &mental_owner.ch);
        assert(GET_MAX_HIT(&library_air.ch) == (habitat == 1 ? 953 : habitat == 0 ? 908 : 862));
        assert(GET_MAX_HIT(&library_earth.ch) == (habitat == 1 ? 1713 : habitat == 0 ? 1632 : 1550));
        library_air.ch.points.base_hit = GET_MAX_HIT(&library_air.ch) = 90000;
        chaos = true;
        summoner_pet_configure(&library_air.ch, &mental_owner.ch, false, true);
        assert(GET_MAX_HIT(&library_air.ch) == (habitat == 1 ? 953 : habitat == 0 ? 908 : 862));
        chaos = false;
    }
    terrain = 0;
    Fixture middle_owner(false, RACE_HUMAN, CLASS_SUMMONER, 46);
    middle_owner.ch.player.spec = SPEC_MENTALIST;
    Fixture middle_elemental(true, RACE_A_ELEMENTAL, CLASS_WARRIOR, 51);
    summoner_pet_configure(&middle_elemental.ch, &middle_owner.ch, true);
    assert(GET_LEVEL((&middle_elemental.ch)) == 46); // no separate level-45 elemental cap
    Fixture orb_elemental(true, RACE_E_ELEMENTAL, CLASS_WARRIOR, 61);
    summoner_pet_configure(&orb_elemental.ch, &mental_owner.ch);
    assert(GET_LEVEL((&orb_elemental.ch)) == 61); // no elemental/orb clamp
    assert(GET_MAX_HIT(&orb_elemental.ch) <= necro_hp_ceiling(&mental_owner.ch));
    Fixture human_mental(true, RACE_HUMAN, CLASS_WARRIOR, 56);
    publish(human_mental, mental_owner);
    assert(GET_MAX_MANA(&human_mental.ch) == 448);
    assert(GET_MAX_HIT(&human_mental.ch) <= normal_capture_hp(&human_mental.ch, &mental_owner.ch));
    assert(summoner_pet_memtime(&human_mental.ch, 100) == 100);
    Fixture reserve_owner(false, RACE_HUMAN, CLASS_SUMMONER, 30);
    reserve_owner.ch.player.spec = SPEC_MENTALIST;
    Fixture reserve_pet(true, RACE_A_ELEMENTAL, CLASS_WARRIOR, 30);
    publish(reserve_pet, reserve_owner);
    assert(summoner_pet_spend(&reserve_pet.ch, 24));
    assert(GET_MANA(&reserve_pet.ch) == 126);
    reserve_owner.ch.player.level = 41;
    summoner_pet_configure(&reserve_pet.ch, &reserve_owner.ch, false, true);
    assert(GET_MAX_MANA(&reserve_pet.ch) == 180 && GET_MANA(&reserve_pet.ch) == 126);
    summoner_pet_finish_affects(&reserve_pet.ch);
    assert(GET_MANA(&reserve_pet.ch) == 126); // the bigger restored pool cannot refill debt

    Fixture natural_owner(false, RACE_HUMAN, CLASS_SUMMONER, 29);
    natural_owner.ch.player.spec = SPEC_NATURALIST;
    Fixture warg(true, RACE_CARNIVORE, CLASS_WARRIOR, 29);
    publish(warg, natural_owner);
    assert(summoner_pet_memtime(&warg.ch, 100) == 100);
    assert(summoner_pet_melee_damage(&warg.ch, 100) == 100);
    assert(summoner_pet_physical_damage(&warg.ch, 100) == 100);
    for (int level : {30, 40, 41, 50, 51, 56}) {
        natural_owner.ch.player.level = level;
        assert(summoner_pet_memtime(&warg.ch, 100) == 120);
        assert(std::abs(summoner_pet_melee_damage(&warg.ch, 100) - (level >= 41 ? 110 : 105)) < .001);
        assert(std::abs(summoner_pet_physical_damage(&warg.ch, 100) -
                        (level >= 51 ? 85 : level >= 41 ? 90 : 95)) < .001);
        assert(summoner_pet_hit_regen(&warg.ch, 100) == (level >= 51 ? 120 : level >= 41 ? 115 : 110));
        assert(summoner_pet_hit_regen(&warg.ch, 0) == 0);
        assert(summoner_pet_hit_regen(&warg.ch, -10) == -10);
    }
    const int natural_hp = GET_MAX_HIT(&warg.ch);
    warg.ch.points.hit = natural_hp - 1;
    natural_owner.ch.player.level = 30;
    const int ordinary_regen = hit_regen(&warg.ch, false);
    natural_owner.ch.player.level = 51;
    assert(hit_regen(&warg.ch, false) > ordinary_regen);
    warg.ch.specials.fighting = &owner.ch;
    assert(hit_regen(&warg.ch, false) == 0);
    assert(summoner_pet_hit_regen(&warg.ch, 100) == 100);
    assert(summoner_pet_recovery_blocked(&warg.ch));
    warg.ch.specials.fighting = nullptr;
    rooms[1].room_flags |= ROOM_NO_HEAL;
    assert(hit_regen(&warg.ch, false) == 0);
    rooms[1].room_flags &= ~ROOM_NO_HEAL;
    warg.ch.points.hit = natural_hp + 10;
    assert(hit_regen(&warg.ch, false) < 0); // overheal decay receives no bonus
    natural_owner.ch.player.spec = SPEC_CONTROLLER;
    assert(summoner_pet_memtime(&warg.ch, 100) == 100);
    assert(summoner_pet_melee_damage(&warg.ch, 100) == 100);
    assert(summoner_pet_physical_damage(&warg.ch, 100) == 100);
    assert(summoner_pet_hit_regen(&warg.ch, 100) == 100);
    natural_owner.ch.player.spec = SPEC_NATURALIST;
    Fixture natural_human(true, RACE_HUMAN, CLASS_WARRIOR, 51);
    publish(natural_human, natural_owner);
    assert(summoner_pet_melee_damage(&natural_human.ch, 100) == 100);
    Fixture wild_animal(true, RACE_CARNIVORE, CLASS_WARRIOR, 51);
    masters[&wild_animal.ch] = &natural_owner.ch;
    for (uint32_t kind : {0U, static_cast<uint32_t>(summoned_pet_kind::undead_first)}) {
        wild_animal.npc.summon_kind = kind;
        assert(summoner_pet_melee_damage(&wild_animal.ch, 100) == 100);
        assert(summoner_pet_physical_damage(&wild_animal.ch, 100) == 100);
        assert(summoner_pet_hit_regen(&wild_animal.ch, 100) == 100);
    }
    masters[&untouched.ch] = &natural_owner.ch;
    assert(summoner_pet_melee_damage(&untouched.ch, 100) == 100);
    assert(summoner_pet_physical_damage(&untouched.ch, 100) == 100);
    assert(summoner_pet_hit_regen(&untouched.ch, 100) == 100);
    assert(summoner_pet_memtime(&untouched.ch, 100) == 100);

    // Top Naturalist melee is approximately 20% above the strongest Controller
    // example, without increasing base damroll or applying the bonus to spells.
    Fixture best_controller(false, RACE_HUMAN, CLASS_SUMMONER, 56);
    best_controller.ch.player.spec = SPEC_CONTROLLER;
    Fixture best_naturalist(false, RACE_HUMAN, CLASS_SUMMONER, 56);
    best_naturalist.ch.player.spec = SPEC_NATURALIST;
    Fixture best_bard(true, RACE_GREY, CLASS_BARD, 56);
    Fixture seer(true, RACE_DRAGONKIN, CLASS_CLERIC | CLASS_DRUID | CLASS_SHAMAN, 56);
    best_bard.ch.points.base_damroll = seer.ch.points.base_damroll = 73;
    publish(best_bard, best_controller); publish(seer, best_naturalist);
    assert(best_bard.ch.points.damnodice == 8 && best_bard.ch.points.damsizedice == 7);
    assert(seer.ch.points.damnodice == 9 && seer.ch.points.damsizedice == 8);
    const double controller_hit = 73 + 8 * (7 + 1) / 2.0;
    const double natural_hit = summoner_pet_melee_damage(&seer.ch, 73 + 9 * (8 + 1) / 2.0);
    assert(std::abs(controller_hit - 105) < .001 && std::abs(natural_hit - 124.85) < .001);
    assert(natural_hit / controller_hit >= 1.18 && natural_hit / controller_hit <= 1.22);
    summoner_pet_configure(&seer.ch, &best_naturalist.ch, false, true);
    assert(seer.ch.points.damnodice == 9 && seer.ch.points.damsizedice == 8);
    assert(seer.ch.points.base_damroll == 73);
    for (int level : {51, 55, 56, 61}) {
        best_controller.ch.player.level = best_naturalist.ch.player.level = std::min(level, 56);
        Fixture controller_caster(true, RACE_GREY, CLASS_BARD, level);
        Fixture natural_caster(true, RACE_DRAGONKIN, CLASS_CLERIC | CLASS_DRUID | CLASS_SHAMAN, level);
        const int damroll = (level > 55 ? 45 : 40) + level / 2;
        controller_caster.ch.points.base_damroll = natural_caster.ch.points.base_damroll = damroll;
        publish(controller_caster, best_controller); publish(natural_caster, best_naturalist);
        const double controller_average = damroll + controller_caster.ch.points.damnodice *
            (controller_caster.ch.points.damsizedice + 1) / 2.0;
        const double natural_average = summoner_pet_melee_damage(&natural_caster.ch, damroll +
            natural_caster.ch.points.damnodice * (natural_caster.ch.points.damsizedice + 1) / 2.0);
        assert(natural_average / controller_average >= 1.18 && natural_average / controller_average <= 1.22);
    }
    mental_owner.ch.player.spec = SPEC_CONTROLLER;
    assert(summoner_pet_memtime(&mental_pet.ch, 100) == 100);
    assert(summoner_pet_memtime(&untouched.ch, 100) == 100);

    // Captured payloads use the freshly loaded prototype size, including old saves
    // that contain the former terrain-forced size; other summon payloads keep size.
    pet_restore_state state;
    state.version = 2; state.kind = summoned_pet_kind::summoner_capture;
    state.act = ACT_ISNPC; state.race = RACE_E_ELEMENTAL; state.level = 55;
    state.primary_class = CLASS_WARRIOR; state.resource_slot = 1;
    state.size = SIZE_MEDIUM;
    Fixture restored_size(true, RACE_E_ELEMENTAL, CLASS_WARRIOR, 55);
    restored_size.ch.specials.affected_by = AFF_HASTE;
    restored_size.ch.specials.affected_by2 = AFF2_FLURRY;
    restored_size.ch.specials.affected_by3 = AFF3_BLUR;
    restored_size.ch.specials.act |= ACT_BREATHES_LIGHTNING;
    GET_SIZE(&restored_size.ch) = SIZE_GARGANTUAN;
    assert(summoned_pet_apply(&restored_size.ch, state));
    assert(GET_SIZE(&restored_size.ch) == SIZE_GARGANTUAN);
    assert(IS_AFFECTED(&restored_size.ch, AFF_HASTE)); // old stripped payload regains prototype traits
    assert(IS_AFFECTED2(&restored_size.ch, AFF2_FLURRY) && IS_AFFECTED3(&restored_size.ch, AFF3_BLUR));
    assert(IS_ACT(&restored_size.ch, ACT_BREATHES_LIGHTNING)); // recover older stripped breath flags
    state.level = 61; state.race = RACE_WIGHT;
    Fixture restored_orb(true, RACE_WIGHT, CLASS_WARRIOR, 61);
    assert(summoned_pet_apply(&restored_orb.ch, state));
    summoner_pet_configure(&restored_orb.ch, &owner.ch, false, true);
    assert(GET_LEVEL((&restored_orb.ch)) == 61);
    state.level = 35; state.race = RACE_HUMAN;
    Fixture restored_ordinary(true, RACE_HUMAN, CLASS_WARRIOR, 35);
    assert(summoned_pet_apply(&restored_ordinary.ch, state));
    summoner_pet_configure(&restored_ordinary.ch, &capacity_owner.ch, false, true);
    assert(GET_LEVEL((&restored_ordinary.ch)) == 35);
    capacity_owner.ch.player.level = 31;
    summoner_pet_configure(&restored_ordinary.ch, &capacity_owner.ch, false, true);
    assert(GET_LEVEL((&restored_ordinary.ch)) == 31);
    mental_owner.ch.player.spec = SPEC_MENTALIST;
    summoner_pet_configure(&restored_size.ch, &mental_owner.ch, false, true);
    assert(GET_SIZE(&restored_size.ch) == SIZE_GARGANTUAN);
    assert(IS_AFFECTED(&restored_size.ch, AFF_HASTE)); // neutral terrain retains authored haste
    assert(IS_AFFECTED2(&restored_size.ch, AFF2_FLURRY) && IS_AFFECTED3(&restored_size.ch, AFF3_BLUR));
    assert(test_strength(&restored_size.ch, 0, 0) == 145);
    summoner_pet_configure(&restored_size.ch, &mental_owner.ch, false, true);
    assert(IS_AFFECTED(&restored_size.ch, AFF_HASTE) && test_strength(&restored_size.ch, 0, 0) == 145);
    state.kind = summoned_pet_kind::undead_first;
    assert(summoned_pet_apply(&restored_size.ch, state));
    assert(GET_SIZE(&restored_size.ch) == SIZE_MEDIUM);
    assert(!IS_AFFECTED(&restored_size.ch, AFF_HASTE)); // other summons use saved traits only
    assert(!IS_ACT(&restored_size.ch, ACT_BREATHES_LIGHTNING));
    state.kind = summoned_pet_kind::summoner_capture;
    state.intrinsic_affects[0] |= AFF_HASTE; // favorable terrain from the saved body
    Fixture neutral_restore(true, RACE_E_ELEMENTAL, CLASS_WARRIOR, 55);
    assert(summoned_pet_apply(&neutral_restore.ch, state));
    assert(!IS_AFFECTED(&neutral_restore.ch, AFF_HASTE)); // no authored haste on this prototype
    Fixture timed_restore(true, RACE_WIGHT, CLASS_WARRIOR, 56);
    timed_restore.ch.specials.affected_by |= AFF_HASTE | AFF_DETECT_INVISIBLE;
    timed_restore.ch.specials.affected_by3 |= AFF3_FOUR_ARMS;
    assert(summoned_pet_apply(&timed_restore.ch, state));
    assert(!IS_AFFECTED(&timed_restore.ch, AFF_HASTE));
    assert(!affected_by_spell(&timed_restore.ch, SPELL_HASTE) && HAS_FOUR_HANDS(&timed_restore.ch));
    terrain = 0;
    Fixture bard_owner(false, RACE_HUMAN, CLASS_SUMMONER, 31);
    Fixture bard(true, RACE_GNOME, CLASS_BARD | CLASS_WARRIOR, 35);
    bard.ch.player.spec = SPEC_MINSTREL;
    publish(bard, bard_owner);
    assert(GET_MAX_MANA(&bard.ch) == 186);
    Fixture ally(false, RACE_HUMAN, CLASS_WARRIOR, 31);
    rooms[1].people = &ally.ch;
    ally.ch.points.hit = ally.ch.points.max_hit = 500;
    assert(summoner_pet_song(&bard.ch, SONG_HEALING, false, false, true, 1));
    assert(GET_MANA(&bard.ch) == 186); // a full-health party costs nothing
    ally.ch.points.hit = 450;
    assert(summoner_pet_song(&bard.ch, SONG_HEALING, false, false, true, 1));
    assert(GET_MANA(&bard.ch) == 166);
    clear_reserve(bard_owner); summoner_pet_configure(&bard.ch, &bard_owner.ch);
    bard.ch.specials.affected_by3 |= AFF3_SINGING;
    assert(summoner_pet_song(&bard.ch, SONG_FLIGHT, false, false, true, 1));
    assert(GET_MANA(&bard.ch) == 180);
    affected_type flight = {}; flight.type = SONG_FLIGHT; affect_to_char(&ally.ch, &flight);
    songs[&ally.ch] = &bard.ch;
    for (int tick = 0; tick < 60 * WAIT_SEC; ++tick) {
        ++ne_event_tick;
        // The first regen event may have started before the song was applied.
        assert(summoner_pet_flight_regen(&ally.ch, tick ? 1 : 30 * WAIT_SEC));
        summoner_pet_sync_resources(&bard.ch); // repeated saves preserve fractional expenditure
        assert(summoner_pet_flight_regen(&ally.ch, 1)); // party members do not multiply cost
    }
    assert(GET_MANA(&bard.ch) == 0);
    ++ne_event_tick;
    assert(!summoner_pet_flight_regen(&ally.ch, 1));
    assert(exhaustion == 1 && stopped == 1 && message_owner == &bard_owner.ch);
    assert(!summoner_pet_skill(&bard.ch, CMD_BASH));
    assert(GET_MANA(&bard.ch) == 0);
    assert(summoner_pet_skill(&bard.ch, CMD_LOOK));
    assert(summoner_pet_skill(&bard.ch, CMD_PUNCH)); // a social, not a combat skill
    assert(!summoner_pet_skill(&bard.ch, CMD_LAYHAND));

    // A different prototype using the same prepared slot keeps the existing debt.
    Fixture replacement(true, RACE_HUMAN, CLASS_WARRIOR, 31);
    publish(replacement, bard_owner);
    // A smaller pool can remain overdrawn; zero-cost orders must still work.
    Fixture smaller(true, RACE_A_ELEMENTAL, CLASS_WARRIOR, 31);
    publish(smaller, bard_owner);
    assert(GET_MANA(&smaller.ch) == 0);
    assert(summoner_pet_skill(&smaller.ch, CMD_LOOK));
    assert(summoner_pet_skill(&smaller.ch, CMD_NORTH));
    assert(GET_MANA(&replacement.ch) == 248 - 186);
    assert(summoner_pet_skill(&replacement.ch, CMD_KICK));
    assert(GET_MANA(&replacement.ch) == 50);
    replacement.ch.specials.undead_spell_slots[1]--;
    summoner_pet_sync_resources(&replacement.ch);
    assert(bank(&bard_owner.ch, 1, 1, false)->modifier == 1);
    Fixture restored(true, RACE_HUMAN, CLASS_WARRIOR, 31);
    restored.npc.summoner_resource_slot = 1;
    restored.ch.points.mana = 50;
    std::copy(std::begin(replacement.ch.specials.undead_spell_slots),
              std::end(replacement.ch.specials.undead_spell_slots),
              std::begin(restored.ch.specials.undead_spell_slots));
    summoner_pet_configure(&restored.ch, &bard_owner.ch, false, true);
    assert(GET_MANA(&restored.ch) == 50);
    assert(restored.ch.specials.undead_spell_slots[1] == max_spells_in_circle(&restored.ch, 1) - 1);
    masters[&restored.ch] = &bard_owner.ch;
    follow_type follow = {}; follow.follower = &restored.ch; bard_owner.ch.followers = &follow;
    bard_owner.ch.specials.fighting = &ally.ch;
    const int debt = bank(&bard_owner.ch, 1, mana_bank, false)->modifier;
    event_summoner_recovery(&bard_owner.ch, nullptr, nullptr, nullptr);
    assert(bank(&bard_owner.ch, 1, mana_bank, false)->modifier == debt);
    assert(summoner_pet_recovery_blocked(&restored.ch));
    bard_owner.ch.specials.fighting = nullptr;
    summoner_pet_note_command(&bard_owner.ch, CMD_NORTH);
    event_summoner_recovery(&bard_owner.ch, nullptr, nullptr, nullptr);
    assert(bank(&bard_owner.ch, 1, mana_bank, false)->modifier == debt);
    ne_event_tick += 20 * WAIT_SEC;
    event_summoner_recovery(&bard_owner.ch, nullptr, nullptr, nullptr);
    assert(bank(&bard_owner.ch, 1, mana_bank, false)->modifier < debt);
    assert(!summoner_pet_recovery_blocked(&restored.ch));

    Fixture absent_owner(false, RACE_HUMAN, CLASS_SUMMONER, 31);
    const int scheduled_before = recovery_schedules;
    summoner_pet_start_recovery(&absent_owner.ch);
    assert(recovery_schedules == scheduled_before); // no pet resources, no idle timer
    bank(&absent_owner.ch, 1, mana_bank, true)->modifier = 100000;
    bank(&absent_owner.ch, 1, capacity_bank, true)->modifier = 248;
    summoner_pet_start_recovery(&absent_owner.ch);
    assert(recovery_schedules == scheduled_before + 1); // saved banks, no active pet
    event_summoner_recovery(&absent_owner.ch, nullptr, nullptr, nullptr);
    assert(bank(&absent_owner.ch, 1, mana_bank, false)->modifier < 100000);

    owner.ch.player.spec = SPEC_CONTROLLER;
    summoner_chaos_recipes(&owner.ch); assert(recipes.empty());
    chaos = true; summoner_chaos_recipes(&early_owner.ch); assert(recipes.empty());
    Fixture psi(true, RACE_HUMAN, CLASS_PSIONICIST, 56);
    publish(psi, owner);
    psi.ch.points.mana = 6;
    assert(!summoner_pet_spell_ready(&psi.ch, 1));
    psi.ch.points.mana = 7;
    assert(summoner_pet_spell_ready(&psi.ch, 1));
    Fixture caster(true, RACE_HUMAN, CLASS_SORCERER, 56);
    publish(caster, owner);
    caster.ch.specials.undead_spell_slots[12] = 0;
    assert(!summoner_pet_spell_ready(&caster.ch, 12));
    caster.ch.specials.undead_spell_slots[12] = 1;
    assert(summoner_pet_spell_ready(&caster.ch, 12));
    const std::set<int> approved = {142408,27035,82507,35543,35542,30623,135214,42204,78483};
    for (int spec = 1; spec <= 3; ++spec) {
        recipes.clear(); owner.ch.player.spec = spec;
        summoner_chaos_recipes(&owner.ch); assert(recipes.size() == 3);
        for (int vnum : recipes) assert(approved.contains(vnum));
        summoner_chaos_recipes(&owner.ch); assert(recipes.size() == 3);
    }
    std::puts("summoner class preservation, HP/DR/vamp caps, ordinary/orb levels and capacity, bounded Mentalist HP, blended dice, 20% Naturalist melee target and slower slots, prototype size, mana/slots, useful songs, 60-second flight, restoration and Chaos passed");
}
'''

engine = "\n".join([
    function("src/core/utility.c", "int BOUNDED(int a, int b, int c)"),
    function("src/core/utility.c", "int GET_CLASS(P_char ch, uint m_class)"),
    function("src/core/utility.c", "int GET_PRIME_CLASS(P_char ch, uint m_class)"),
    function("src/core/utility.c", "int GET_SECONDARY_CLASS(P_char ch, uint m_class)"),
    function("src/classes/memorize.c", "int IS_SEMI_CASTER(P_char ch)"),
    function("src/classes/memorize.c", "int IS_PARTIAL_CASTER(P_char ch)"),
    function("src/classes/new_skills.c", "void MonkSetSpecialDie(P_char ch)"),
    function("src/classes/memorize.c", "void SetSpellCircles(void)"),
    function("src/classes/memorize.c", "int get_max_circle(P_char ch)"),
    function("src/classes/memorize.c", "inline int max_spells_in_circle(P_char ch, int circ, int max_circ)"),
    function("src/classes/memorize.c", "int max_spells_in_circle(P_char ch, int circ)"),
    function("src/world/limits.c", "int hit_regen(P_char ch, bool display_only)"),
    function("src/player/pet_restore_runtime.c", "bool summoned_pet_apply(P_char pet, const pet_restore_state &s)"),
    function("src/classes/drannak.c", "bool new_summon_check(P_char ch, P_char selected)"),
    function("src/magic/spell_status_control.c", "void spell_haste("),
    function("src/magic/spell_visibility.c", "void spell_blur("),
    function("src/magic/spell_visibility.c", "void spell_invisibility("),
    function("src/magic/spell_detection.c", "void spell_detect_invisibility("),
    function("src/magic/spell_healing.c", "void spell_regeneration("),
    function("src/magic/spell_globes.c", "void spell_globe("),
    function("src/magic/spell_transformations.c", "void spell_vampire("),
])
affects = (ROOT / "src/magic/affects.c").read_text()
strength_start = affects.index("\tt1 = (!mode || !TmpAffs.r_Str)")
strength_end = affects.index("\tt1 = (!mode || !TmpAffs.r_Dex)", strength_start)
engine += "\nint test_strength(P_char ch, int bonus, int maximum) {\n" + \
    "struct { int r_Str = 0, c_Str, m_Str; } TmpAffs = {0, bonus, maximum};\n" + \
    "bool mode = true; int t1, t2, t3;\n" + affects[strength_start:strength_end] + \
    "return GET_C_STR(ch);\n}\n"
HARNESS = HARNESS.replace("__TABLES__", tables).replace("__ENGINE_FUNCTIONS__", engine).replace("__SKILLS__", skill_setup)
build_dir = ROOT / "bin/tests"
build_dir.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="summoner-", dir=build_dir) as directory:
    source = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    source.write_text(HARNESS)
    subprocess.run(shlex.split(os.environ.get("CXX", "g++")) + ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
                    str(source), "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)

# Integration gates: orders and autonomous casts use the same admission helper;
# ordinary NPC mana regeneration cannot bypass the owner reserve.
parser = (ROOT / "src/net/sparser.c").read_text()
interpreter = (ROOT / "src/cmd/interp.c").read_text()
assert "summoner_pet_spell_ready(ch, circle)" in parser
assert "summoner_pet_spell_ready(ch, get_spell_circle(ch, spl))" in (ROOT / "src/mob/mobact.c").read_text()
creation = function("src/classes/drannak.c", "void do_conjure(")
assert creation.count("summoner_pet_cast_buffs(t_ch, prototype_traits)") == 1
assert creation.index("add_follower(t_ch, ch)") < creation.index("summoner_pet_cast_buffs(t_ch, prototype_traits)")
assert "summoner_pet_cast_buffs" not in (ROOT / "src/player/player_load_pets.c").read_text()
assert "summoner_pet_spend(ch, 42)" in function("src/mob/mobact.c", "void BreathWeapon(")
assert interpreter.index("summoner_pet_skill(exec_char, cmd)") < interpreter.index("// Execute the bloody thing!!!")
assert "summoner_owned_pet(ch)" in function("src/world/events.c", "void event_mana_regen(")
assert "summoner_pet_flight_regen(ch, elapsed_ticks)" in function("src/world/events.c", "void event_move_regen(")
assert "summoner_pet_recovery_blocked(ch)" in function("src/classes/memorize.c", "void handle_undead_mem(P_char ch)")
assert "summoner_pet_memtime(ch, calculate_undead_time" in function("src/classes/memorize.c", "int get_circle_memtime(P_char ch, int circle, bool bStatOnly)")
fight = (ROOT / "src/combat/fight.c").read_text()
assert fight.count("summoner_pet_melee_damage(") == 1
assert "summoner_pet_melee_damage(ch, dam)" in function("src/combat/fight.c", "bool hit(P_char ch, P_char victim, P_obj weapon, int *damAccumulator)")
assert "summoner_pet_physical_damage(victim, dam)" in function("src/combat/fight.c", "int raw_damage(")
assert "messages && (messages->type & (1 << 24)) && !(flags & PHSDAM_NOREDUCE)" in fight
assert "summoner_pet_sync_resources(ch)" in function("src/classes/memorize.c", "void use_spell(P_char ch, int spell)")
assert (ROOT / "src/classes/bard.c").read_text().count("summoner_pet_song(") == 3
assert "summoner_chaos_recipes(ch)" in function("src/classes/drannak.c", "void do_conjure(")
assert "summoner_pet_start_recovery(owner)" in function("src/player/player_load_pets.c", "void player_load_pets_commit(")
staging = function("src/player/player_load_pets.c", "bool player_load_pets_stage(")
assert "capture_level = summoner_pet_level(probe, owner)" in staging
assert "|| capture_level > GET_LEVEL(owner)" not in staging
assert "GET_LEVEL(t_ch) > CONJURE_MAXLVL_NO_ORB" in function("src/classes/drannak.c", "void do_conjure(")
conjuration = (ROOT / "src/magic/spell_conjuration.c").read_text()
assert "summoner_elemental_body(" not in conjuration
assert "summoned_pet_mark(" not in conjuration
assert "pets[summoned].damroll + number(20, 30)" in conjuration
assert "dice(GET_LEVEL(mob) / 2, 12)" in conjuration
vamp_source = (ROOT / "src/combat/damage_support.c").read_text()
assert "cap = summoner_pet_heal_cap(ch, (int)fcap)" in function("src/combat/damage_support.c", "int vamp(")
assert "IS_DRACOLICH(ch) && !summoner_capture(ch)" in vamp_source
assert "summoner_pet_vamp_rate(ch, dam_factor[DF_NPCVAMP], true)" in vamp_source
print("summoner order, spell, song, recovery, Chaos, vamp and conjurer-isolation integration gates passed")
