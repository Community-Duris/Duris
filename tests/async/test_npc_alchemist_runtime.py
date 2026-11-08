#!/usr/bin/env python3
"""Native virtual alchemist actions, caster cadence and once-per-spawn vial checks."""
from pathlib import Path
import re
import subprocess
import tempfile
from _paths import ROOT
parser = (ROOT / 'src/net/sparser.c').read_text()
start = parser.index('int SpellCastTime(P_char ch, int spl)')
end = parser.index('void SpellCastShow(', start)
cast_time = parser[start:end]
# Use the actual authored potion spell slots rather than fixture spell guesses.
content = (ROOT / 'areas/obj/heavens.obj').read_text()
setup = []
for vnum in (868,866,865,863,859,857,855,853,850):
 block = content.split('#'+str(vnum)+'\n',1)[1].split('\n#',1)[0]
 values = re.search(r'^0 ((?:-?\d+ ?){7})$', block, re.M).group(0).split()
 setup.append('templates['+str(vnum)+'].type = ITEM_POTION;')
 for slot in range(1,4): setup.append('templates['+str(vnum)+'].value['+str(slot)+'] = '+values[slot]+';')
skill_source = (ROOT / 'src/classes/skills.c').read_text()
skill_source = re.sub(r'/\*.*?\*/|//[^\n]*', '', skill_source, flags=re.S)
spell_definitions = re.findall(r'SPELL_CREATE(?:_MSG)?\(\s*"[^"]*"\s*,\s*(SPELL_\w+)\s*,\s*([^,]+),\s*([^,]+),', skill_source)
skill_setup = '\n'.join('skills[' + spell + '].beats = ' + beats + '; skills[' + spell + '].targets = ' + targets + ';' for spell, beats, targets in spell_definitions)
salchemist = (ROOT / 'src/classes/salchemist.c').read_text()
vial_selector = salchemist[salchemist.index('P_obj get_vial(P_char ch)'):salchemist.index('char *print_poison_ingredients(')]
body = r'''
#include "classes/npc_alchemist.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "world/object_template.h"
#include "world/vnum.obj.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <set>
#include <vector>
Skill skills[MAX_SKILLS+1] = {};
index_data object_indices[1] = {};
P_index obj_index = object_indices;
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
float spell_pulse_data[LAST_RACE+1] = {};
room_data rooms[2] = {};
P_room world = rooms;
extern const int top_of_world = 1;
unsigned long long ne_event_tick = 0;
static std::map<int,object_template> templates;
static P_char victim = nullptr;
static P_char pet_master = nullptr;
static std::set<P_char> alive;
static bool slip = false, miss = false, adjacent = true;
static int spawn_roll = 10, rolls = 0, allocations = 0, carries = 0;
static int calls = 0, effect_level = 0, appearances = 0, wakes = 0;
static P_char last_target = nullptr;
static int remove_character = 0;
static obj_data vial = {};
static bool allocation_failure = false;
float get_property(const char *, double value) { return static_cast<float>(value); }
int number(int low, int high) {
 if (low == 1 && high == 100) { ++rolls; return spawn_roll; }
 if (low == 1 && high == 140) return slip ? 140 : 1;
 if (low == 0) return miss ? 0 : 1;
 return high; // deterministically select the highest unlocked effect
}
P_char get_linked_char(P_char, unsigned short) { return pet_master; }
int GET_CLASS(P_char ch, unsigned int flags) { return (ch->player.m_class & flags) != 0; }
P_char pick_target(P_char, unsigned int) { return victim; }
int char_in_list(const P_char ch) { return alive.contains(ch); }
bool affected_by_spell(P_char, int) { return false; }
bool AdjacentInRoom(P_char, P_char) { return adjacent; }
void affect_from_char(P_char, int) { ++wakes; }
void appear(P_char, bool) { ++appearances; }
void CharWait(P_char, int) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
void send_to_char(const char *, P_char) {}
void logit(const char *, const char *, ...) {}
bool cache_object_template(int vnum) { return templates.contains(vnum); }
const object_template *find_object_template(int vnum) {
 auto found = templates.find(vnum);
 return found == templates.end() ? nullptr : &found->second;
}
P_obj read_object(int vnum, int) { assert(vnum == VOBJ_POISON_VIALS); ++allocations; return allocation_failure ? nullptr : &vial; }
void obj_to_char(P_obj obj, P_char ch) { ++carries; ch->carrying = obj; obj->loc_p = LOC_CARRIED; obj->loc.carrying = ch; }
void effect(int level, P_char actor, char *, int, P_char target, P_obj object) {
 assert(!object); ++calls; effect_level = level; last_target = target;
 if (remove_character == 1) alive.erase(actor);
 if (remove_character == 2) alive.erase(target);
}
''' + cast_time + vial_selector + r'''
int main() {
''' + '\n'.join(setup) + '\n' + skill_setup + r'''
 for (auto &[vnum, prototype] : templates) { (void)vnum;
  for (int slot=1; slot<=3; ++slot) if (prototype.value[slot] > 0) {
   skills[prototype.value[slot]].spell_pointer = effect;
  }
 }
 for (auto &racial : spell_pulse_data) racial = 1.0f;
 npc_only_data npc = {}; char_data actor = {}, target = {};
 actor.only.npc = &npc;
 actor.specials.act = ACT_ISNPC;
 actor.player.m_class = CLASS_ALCHEMIST;
 actor.player.level = 5;
 actor.curr_stats.Agi = 200;
 actor.specials.position = target.specials.position = POS_STANDING + STAT_NORMAL;
 actor.specials.base_combat_round = 15;
 actor.in_room = target.in_room = 1;
 actor.specials.fighting = &target;
 victim = &target; alive = {&actor,&target};
 npc_alchemist_cache_templates();
 assert(!npc_alchemist_combat(&actor) && calls == 0);
 for (int level : {6,11,16,21,26,31,36,41,51,65,90}) {
  actor.player.level = level; npc.alchemist_action_until_pulse = 0;
  const int before = calls;
  assert(npc_alchemist_combat(&actor));
  assert(calls == before+1 && effect_level == std::min(level,50));
  assert(last_target == &target); // greater living stone is an offensive summon
  assert(!npc_alchemist_combat(&actor)); // second AI path shares the same budget
 }
 assert(allocations == 0 && carries == 0);
 actor.player.level = 21;
 npc.alchemist_action_until_pulse = 0; actor.curr_stats.Agi = 100; slip = true;
 int before = calls; assert(npc_alchemist_combat(&actor) && calls == before);
 slip = false; actor.curr_stats.Agi = 200; miss = true; npc.alchemist_action_until_pulse = 0;
 assert(npc_alchemist_combat(&actor) && calls == before);
 miss = false; npc.alchemist_action_until_pulse = 0;
 rooms[1].room_flags = ROOM_NO_MAGIC;
 assert(npc_alchemist_combat(&actor) && calls == before);
 rooms[1].room_flags = ROOM_SINGLE_FILE; adjacent = false; npc.alchemist_action_until_pulse = 0;
 assert(npc_alchemist_combat(&actor) && calls == before);
 rooms[1].room_flags = 0; adjacent = true;
 actor.specials.affected_by = AFF_INVISIBLE;
 target.specials.position = POS_PRONE + STAT_SLEEPING;
 npc.alchemist_action_until_pulse = 0;
 assert(npc_alchemist_combat(&actor) && appearances > 0 && GET_STAT(&target) == STAT_NORMAL);
 target.specials.position = POS_STANDING + STAT_NORMAL;
 actor.specials.affected_by = 0;
 // Effect removal ends a multi-slot prototype safely after its first spell.
 templates[863].value[2] = templates[863].value[1];
 for (int removal : {1,2}) {
  alive = {&actor,&target}; remove_character = removal;
  npc.alchemist_action_until_pulse = 0; before = calls;
  assert(npc_alchemist_combat(&actor) && calls == before+1);
 }
 templates[863].value[2] = 0; remove_character = 0; alive = {&actor,&target};
 // Count actual ability actions against caster start opportunities using the
 // real SpellCastTime, varied rounds/modifiers and both AI opportunities.
 for (int level : {6,21,41,65,90}) for (int speed : {-10,0,10}) for (int base_round : {3,7,15,23}) {
  actor.specials.base_combat_round = base_round;
  actor.player.level = level; actor.points.spell_pulse = speed;
  npc.alchemist_action_until_pulse = 0; ne_event_tick = 0;
  const int round = static_cast<int>(actor.specials.base_combat_round)+1;
  const int reference = level >=41 ? SPELL_PRISMATIC_RAY : level >=26 ? SPELL_FIREBALL : level >=21 ? SPELL_CONE_OF_COLD : SPELL_BURNING_HANDS;
  unsigned long long caster_ready = 0;
  int caster_actions = 0, alchemist_actions = 0;
  for (unsigned long long tick = 0; tick < 60000; ++tick) {
   ne_event_tick = tick;
   if (tick % static_cast<unsigned>(round) == 0 && tick >= caster_ready) {
    ++caster_actions;
    caster_ready = tick + SpellCastTime(&actor, reference);
   }
   if (tick % static_cast<unsigned>(round) == 0 || tick % PULSE_MOBILE == 0) {
    if (npc_alchemist_combat(&actor)) ++alchemist_actions;
    assert(!npc_alchemist_combat(&actor));
   }
  }
  const double ratio = static_cast<double>(alchemist_actions)/caster_actions;
  assert(ratio >= .33 && ratio <= .335);
  std::printf("level=%d spell_modifier=%d base_round=%d caster=%d alchemist=%d ratio=%.4f\n", level,speed,base_round,caster_actions,alchemist_actions,ratio);
 }
 assert(allocations == 0 && carries == 0);
 // Missing or malformed prototypes refuse without spending an action.
 actor.player.level = 6; npc.alchemist_action_until_pulse = 0;
 auto saved = templates.at(868); templates.erase(868);
 assert(!npc_alchemist_combat(&actor) && npc.alchemist_action_until_pulse == 0);
 templates[868] = saved; templates[868].value[1] = MAX_SKILLS;
 assert(!npc_alchemist_combat(&actor));
 templates[868] = saved; templates[868].type = ITEM_FOOD;
 assert(!npc_alchemist_combat(&actor)); templates[868] = saved;
 target.in_room = 0; assert(!npc_alchemist_combat(&actor)); target.in_room = 1;
 actor.specials.fighting = nullptr; assert(!npc_alchemist_combat(&actor)); actor.specials.fighting = &target;
 actor.specials.act = 0; assert(!npc_alchemist_combat(&actor)); actor.specials.act = ACT_ISNPC;
 // Exact 10/100 chance boundary; no repeat after depletion or failed allocation.
 actor.player.level = 41;
 for (int roll=1; roll<=100; ++roll) {
  npc.alchemist_vial_roll_done = false; spawn_roll = roll;
  const int loaded = carries, rolled = rolls;
  npc_alchemist_world_spawn(&actor);
  assert(rolls == rolled+1 && carries == loaded+(roll <= 10 ? 1 : 0));
  actor.carrying = nullptr;
  npc_alchemist_world_spawn(&actor);
  assert(rolls == rolled+1);
 }
 npc.alchemist_vial_roll_done = false; allocation_failure = true; spawn_roll = 1;
 npc_alchemist_world_spawn(&actor); before = allocations;
 npc_alchemist_world_spawn(&actor); assert(allocations == before);
 allocation_failure = false;
 npc.alchemist_vial_roll_done = false; npc.summoned_instance = true;
 before = rolls; npc_alchemist_world_spawn(&actor); assert(rolls == before);
 npc.summoned_instance = false; pet_master = &target;
 npc_alchemist_world_spawn(&actor); assert(rolls == before);
 pet_master = nullptr;
 // The real carried VNUM102 is recognized by the actual Assassin vial selector
 // after moving the fixture item from the NPC to a player inventory.
 object_indices[0].virtual_number = VOBJ_POISON_VIALS;
 vial.R_num = 0; vial.name = const_cast<char *>("poison vial");
 npc.alchemist_vial_roll_done = false; npc_alchemist_world_spawn(&actor);
 assert(get_vial(&actor) == &vial);
 actor.carrying = nullptr; obj_to_char(&vial, &target);
 assert(get_vial(&target) == &vial);
}
'''
with tempfile.TemporaryDirectory(prefix='duris-npc-alchemist-') as directory:
 cpp = Path(directory)/'npc.cpp'; binary = Path(directory)/'npc'
 cpp.write_text(body)
 subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror','-D__NO_MYSQL__','-Isrc','-Isrc/no_mysql',str(cpp),'src/classes/npc_alchemist.c','-o',str(binary)],cwd=ROOT,check=True)
 subprocess.run([str(binary)],check=True,timeout=30)
# Real entry points: fresh zone spawn only; no generic read_mobile/restore hook.
db = (ROOT/'src/world/db.c').read_text()
assert db.count('npc_alchemist_world_spawn(mob);') == 4
assert 'npc_alchemist_world_spawn' not in db[db.index('P_char read_mobile('):db.index('void event_object_proc(')]
for path in ROOT.joinpath('src').rglob('*'):
 if path.suffix not in ('.c','.h'): continue
 text = path.read_text()
 for symbol in ('MobAlchemistGetPotions(', 'count_potions(', 'get_potion(', 'spl2potion(', 'void do_mix(', 'get_bottle(', 'got_all_ingredients(', 'extract_used_ingredients('): assert symbol not in text, (path,symbol)
print('NPC alchemist virtual effects, cadence and once-per-spawn vial boundary passed')

fight = (ROOT / 'src/combat/fight.c').read_text()
assert re.search(r'MobCombat\(ch\);\s*//[^\n]*\n\s*if \(!is_char_in_room\(opponent, room\) \|\| !is_char_in_room\(ch, room\)\)\s*continue;\s*appear\(ch\)', fight)
