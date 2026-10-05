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
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>

Skill skills[MAX_AFFECT_TYPES];
room_data rooms[2] = {};
P_room world = rooms;
unsigned long long ne_event_tick = 1000;
int spl_table[TOTALLVLS][MAX_CIRCLE];
__TABLES__
__ENGINE_FUNCTIONS__
std::map<P_char, P_char> masters, songs;
std::set<int> recipes;
bool chaos = false;
int infuse = 100, healing_skill = 100, terrain = 0;
int exhaustion = 0, stopped = 0;
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
affected_type *affect_to_char(P_char ch, affected_type *af) {
    auto *copy = new affected_type(*af); copy->next = ch->affected; ch->affected = copy; return copy;
}
void act(const char *message, int, P_char owner, P_obj, void *, int) {
    assert(std::strstr(message, "looks too exhausted for that")); ++exhaustion; message_owner = owner;
}
P_nevent get_scheduled(P_char, event_func_type) { return nullptr; }
nevent_schedule_result add_event(event_func, int, P_char, P_char, P_obj, int, const void *, int) {
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
}
bool player_save_pipeline_mark(int, player_component_mask_t) { return true; }
bool chaos_mud_enabled() { return chaos; }
bool sql_has_spellbook_mob(int, int vnum) { return recipes.contains(vnum); }
bool sql_add_spellbook_mob(int, int vnum) { recipes.insert(vnum); return true; }
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }

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
        for (int i = 0; i < 10; ++i) ch.base_stats[i] = ch.curr_stats[i] = 100;
        ch.in_room = 1;
        ch.points.hit = ch.points.max_hit = ch.points.base_hit = 90000;
        ch.points.mana = ch.points.max_mana = ch.points.base_mana = 30000;
    }
    ~Fixture() { while (ch.affected) { auto *af = ch.affected; ch.affected = af->next; delete af; } }
};
void publish(Fixture &pet, Fixture &owner) {
    summoner_pet_configure(&pet.ch, &owner.ch);
    masters[&pet.ch] = &owner.ch;
    summoner_pet_sync_resources(&pet.ch);
}
void clear_reserve(Fixture &owner) {
    for (auto *af = owner.ch.affected; af; af = af->next)
        if (af->type == TAG_SUMMONER_RESOURCE) af->modifier = 0;
}

int main() {
    SetSpellCircles();
    __SKILLS__
    Fixture owner(false, RACE_HUMAN, CLASS_SUMMONER, 56);
    Fixture bran(true, RACE_WIGHT, CLASS_WARRIOR | CLASS_CLERIC | CLASS_ANTIPALADIN, 61);
    bran.ch.player.spec = 6;
    bran.ch.specials.affected_by4 = AFF4_MULTI_CLASS;
    bran.ch.specials.act |= ACT_ELITE | ACT_NO_BASH | ACT_IGNORE;
    publish(bran, owner);
    assert(GET_LEVEL((&bran.ch)) == 56 && bran.ch.player.spec == 6);
    assert(bran.ch.player.m_class == (CLASS_WARRIOR | CLASS_CLERIC | CLASS_ANTIPALADIN));
    assert(!(bran.ch.specials.act & (ACT_ELITE | ACT_NO_BASH | ACT_IGNORE)));
    assert(GET_MAX_MANA(&bran.ch) == 448 && GET_MAX_HIT(&bran.ch) <= necro_hp_ceiling(&owner.ch));
    const int bran_hp = GET_MAX_HIT(&bran.ch);
    bran.ch.points.base_hit = GET_MAX_HIT(&bran.ch) = 999999;
    summoner_pet_configure(&bran.ch, &owner.ch, true);
    assert(GET_MAX_HIT(&bran.ch) == bran_hp); // prototype difficulty does not change trained body
    Fixture tiny(true, RACE_GNOME, CLASS_WARRIOR, 56);
    Fixture giant(true, RACE_SGIANT, CLASS_WARRIOR, 56);
    summoner_pet_configure(&tiny.ch, &owner.ch, true);
    summoner_pet_configure(&giant.ch, &owner.ch, true);
    assert(GET_MAX_HIT(&tiny.ch) < GET_MAX_HIT(&giant.ch));
    assert(GET_MAX_HIT(&giant.ch) <= necro_hp_ceiling(&owner.ch));
    Fixture early_owner(false, RACE_HUMAN, CLASS_SUMMONER, 21);
    Fixture early_pet(true, RACE_SGIANT, CLASS_WARRIOR, 26);
    summoner_pet_configure(&early_pet.ch, &early_owner.ch, true);
    assert(GET_LEVEL((&early_pet.ch)) == 21);
    assert(GET_MAX_HIT(&early_pet.ch) <= necro_hp_ceiling(&early_owner.ch));

    for (int habitat : {-1, 0, 1}) {
        terrain = habitat;
        Fixture elemental(true, RACE_E_ELEMENTAL, CLASS_WARRIOR | CLASS_CLERIC, 56);
        Fixture conjured(true, RACE_E_ELEMENTAL, CLASS_WARRIOR, 56);
        Fixture heater(true, RACE_F_ELEMENTAL, CLASS_WARRIOR, 56);
        summoner_pet_configure(&elemental.ch, &owner.ch, true);
        // Preview intentionally uses neutral terrain; live creation uses the shared builder.
        summoner_elemental_body(&elemental.ch, &owner.ch, true, habitat, 800, 30);
        summoner_elemental_body(&conjured.ch, &owner.ch, true, habitat, 800, 30);
        summoner_elemental_body(&heater.ch, &owner.ch, true, habitat, 700, 25);
        assert(GET_MAX_HIT(&elemental.ch) <= necro_hp_ceiling(&owner.ch));
        assert(GET_MAX_HIT(&elemental.ch) <= GET_MAX_HIT(&heater.ch));
        assert(elemental.ch.points.base_damroll <= heater.ch.points.base_damroll);
        assert(GET_LEVEL((&elemental.ch)) == 55);
        assert(GET_MAX_HIT(&elemental.ch) == GET_MAX_HIT(&conjured.ch));
        assert(elemental.ch.points.base_damroll == conjured.ch.points.base_damroll);
        assert(elemental.ch.points.damnodice == conjured.ch.points.damnodice);
        assert(elemental.ch.points.damsizedice == conjured.ch.points.damsizedice);
        assert(GET_SIZE(&elemental.ch) == GET_SIZE(&conjured.ch));
        assert(elemental.ch.player.m_class == (CLASS_WARRIOR | CLASS_CLERIC));
    }
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
    std::puts("summoner class preservation, body caps, elemental parity, mana, useful songs, 60-second flight, swaps/restoration, recovery and Chaos passed");
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
])
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
assert interpreter.index("summoner_pet_skill(exec_char, cmd)") < interpreter.index("// Execute the bloody thing!!!")
assert "summoner_owned_pet(ch)" in function("src/world/events.c", "void event_mana_regen(")
assert "summoner_pet_flight_regen(ch, elapsed_ticks)" in function("src/world/events.c", "void event_move_regen(")
assert "summoner_pet_recovery_blocked(ch)" in function("src/classes/memorize.c", "void handle_undead_mem(P_char ch)")
assert "summoner_pet_sync_resources(ch)" in function("src/classes/memorize.c", "void use_spell(P_char ch, int spell)")
assert (ROOT / "src/classes/bard.c").read_text().count("summoner_pet_song(") == 3
assert "summoner_chaos_recipes(ch)" in function("src/classes/drannak.c", "void do_conjure(")
print("summoner order, spell, song, recovery and Chaos integration gates passed")
