"""Source-level guards for the atomic crafting conservation contract."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def section(text: str, start: str, end: str) -> str:
    begin = text.index(start)
    finish = text.index(end, begin)
    return text[begin:finish]


def require_in_order(text: str, *needles: str) -> None:
    position = -1
    for needle in needles:
        position = text.index(needle, position + 1)


command_h = (ROOT / "src/item/item_transfer_command.h").read_text()
command_c = (ROOT / "src/item/item_transfer_command.c").read_text()
repository_c = (ROOT / "src/item/item_transfer_repository.c").read_text()
runtime_c = (ROOT / "src/item/item_ownership_runtime.c").read_text()
movement_c = (ROOT / "src/item/item_movement_transaction.c").read_text()
flatfile_c = (ROOT / "src/flatfile/flatfile_item_repository.c").read_text()
materialization_c = (ROOT / "src/flatfile/flatfile_shop_trade_materialization.c").read_text()
salchemist_c = (ROOT / "src/classes/salchemist.c").read_text()
drannak_c = (ROOT / "src/classes/drannak.c").read_text()
mysql_runner = (ROOT / "tests/async/run_item_transfer_schema_mysql.sh").read_text()

assert "craft," in command_h
assert "case item_transfer_reason::craft:" in command_c
assert "decode_craft_outputs" in command_c
assert "ITEM_TRANSFER_ABSENT_REVISION" in command_c
assert "output_key" in command_c
assert "payload.reason != item_transfer_reason::craft" in command_c

# A craft may retire inputs without producing an object, but successful outputs
# must be fenced as absent and decoded from a complete snapshot list.
require_in_order(
    command_c,
    "const bool craft = payload.reason == item_transfer_reason::craft;",
    "decode_craft_outputs(payload, &outputs)",
    "outputs.empty() && !find_payload_item(payload, payload.selected_item_uid)",
)

assert "bool execute_craft" in repository_c
assert "sync_restitution_runtime_payload(connection, payload)" in repository_c
assert "payload.reason == item_transfer_reason::craft" in repository_c
assert "Craft payloads carry output snapshots" in repository_c
assert "event_index_base + index" in repository_c
assert "insert_craft_snapshot_rows" in repository_c
assert "update_owner_revision(connection, payload.from_owner" in repository_c
assert "src/sql/item_extra_descr_codec.c" in mysql_runner
assert "tests/async/item_extra_descr_codec_sql_escape_stub.cpp" in mysql_runner
assert "src/persistence/player_death_restitution_command.c" in mysql_runner
assert "src/persistence/player_death_restitution_repository.c" in mysql_runner
assert "item_transfer_reason::craft" in (ROOT / "tests/async/item_transfer_mysql_harness.cpp").read_text()
assert "payload.reason == item_transfer_reason::craft" in runtime_c
assert "item_ownership_runtime_hydrate_many_atomic" in runtime_c

assert "unsigned int apply_craft" in flatfile_c
assert "apply_craft(&candidate, payload, &result)" in flatfile_c
assert "flatfile_item_transfer_materialization_prepare" in materialization_c
assert "payload.reason == item_transfer_reason::craft" in materialization_c

# Gameplay writers must submit before durable completion; the old direct
# extraction/publication paths must not remain in the relevant command spans.
poison = section(salchemist_c, "void do_mixpoison", "bool is_neg_good")
encrust = section(salchemist_c, "void do_encrust", "int encrusted_eq_proc")
pvp = section(drannak_c, "int pvp_store", "// Proc for weapon")

for writer in (poison, encrust, pvp):
    assert "item_movement_transaction_submit_craft" in writer

assert "extract_used_poison_ingredients" not in salchemist_c
assert "obj_to_char" not in poison
assert "obj_to_char(new_item, ch)" not in encrust
assert "chaos_material_pouch_record_generated" not in encrust
assert "Virtual Chaos-pouch encrust is temporarily unavailable" not in encrust
assert "virtual_jewel ? &pouch_usage : nullptr" in encrust
assert "craft_recipe_discipline::poison" in poison
assert "while (outputs.size() < max_poison_batch)" in poison
assert "used.insert(ingredient)" in poison
assert "void do_mix(" not in salchemist_c
assert "do_mix," not in (ROOT / "src/cmd/interp.c").read_text()
assert "craft_recipe_discipline::encrust_failure" in encrust
assert "craft_recipe_discipline::encrust" in encrust
assert "vnum_from_inv" not in pvp
assert "obj_to_char(orb, pl)" not in pvp

# Zero-output failure crafts must be admitted without dereferencing outputs.
zero_output = section(
    movement_c,
    "bool item_movement_transaction_submit_craft",
    "bool item_creation_grant_submit_to_player",
)
assert "output_count && (!outputs" in zero_output
assert "nullptr, 0" in encrust

print("Issue 551 crafting conservation contract passed")

# Compile the actual poison planner/command with stubbed game services. This
# protects bulk behavior and pre-commit conservation beyond the source guards.
import subprocess
import tempfile
helper = section(salchemist_c, "bool collect_required_objects", "struct spellbind_context")
vial_helper = section(salchemist_c, "P_obj get_vial", "char *print_poison_ingredients")
harness = r'''
#include "item/craft_recipe_continuation.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <unordered_set>
#include <string>
struct economic_gameplay_authority { static bool active() { return true; } };
struct object { int vnum; const char *name; const char *short_description; object *next_content = nullptr; };
using P_obj = object *;
struct character { P_obj carrying = nullptr; int skill = 100; int level = 56; };
using P_char = character *;
#define GET_CHAR_SKILL(ch, skill_id) ((ch)->skill)
#define GET_PID(ch) 7
#define GET_LEVEL(ch) ((ch)->level)
#define GET_NAME(ch) "Assassin"
#define OBJ_VNUM(obj) ((obj)->vnum)
constexpr int MAX_INGREDIENTS = 3;
constexpr int MAX_STRING_LENGTH = 1024;
constexpr int PULSE_VIOLENCE = 1;
constexpr int SKILL_MIXPOISON = 111;
constexpr int VOBJ_POISON_VIALS = 102;
constexpr int TRUE=1, FALSE=0, TO_ROOM=1, TO_CHAR=2, VIRTUAL=1;
enum class chaos_pouch_usage_mode { generated };
struct chaos_material_pouch_usage {};
enum class item_movement_reject { none };
using completion_fn = void (*)(P_char,bool,const item_transfer_result &,unsigned int,const uint8_t *,size_t);
struct recipe {int poison_type,level_required,skill_required,vnum;int ingredients[MAX_INGREDIENTS+1];};
recipe poison_data[] = {{1,1,1,300,{200,201,0,0}},{0,0,0,0,{0,0,0,0}}};
int notches=0, allocations=0, submissions=0, allocation_limit=1000;
bool accept=true;
std::vector<P_obj> submitted_inputs, submitted_outputs;
std::vector<uint8_t> saved_context;
craft_recipe_continuation saved_terms;
void notch_skill(P_char,int,double) {++notches;}
skill_notch_outcome skill_notch_prepare(P_char,int,float) { return {100,100,0,0}; }
void send_to_char(const char *,P_char) {}
void send_to_char_f(P_char,const char *,...) {}
void act(const char *,int,P_char,int,int,int) {}
void wizlog(int,const char *,...) {}
void CharWait(P_char,int) {}
void one_argument(char *argument,char *result) {result[0]=*argument;result[1]=0;}
P_obj read_object(int vnum,int) {if (allocations==allocation_limit) return nullptr; ++allocations;return new object{vnum,"poison","a poison"};}
void extract_obj(P_obj obj) {delete obj;}
char *str_dup(const char *value) {return const_cast<char *>(value);}
void set_short_description(P_obj,const char *) {}
char *print_poison_ingredients(int *) {static char text[]="ingredients";return text;}
bool item_movement_transaction_submit_craft(P_char,P_obj const *inputs,size_t input_count,
 P_obj const *outputs,size_t output_count,int,completion_fn callback,const void *context,size_t context_size,item_movement_reject *,
 P_obj,const chaos_material_pouch_usage *,size_t,chaos_pouch_usage_mode,const craft_recipe_continuation *terms)
{
 ++submissions;
 submitted_inputs.assign(inputs,inputs+input_count);
 submitted_outputs.assign(outputs,outputs+output_count);
 assert(std::unordered_set<P_obj>(submitted_inputs.begin(),submitted_inputs.end()).size()==input_count);
 assert(!callback && !context && !context_size && terms);
 assert(terms->player_pid==7 && terms->discipline==craft_recipe_discipline::poison && terms->output_count==output_count);
 saved_terms=*terms;
 return accept;
}
'''
harness += helper + vial_helper + poison
harness += r'''
int main()
{
 for (int count : {1,3,65}) for (bool admitted : {false,true})
 {
  character ch;
  std::vector<object> inventory(static_cast<size_t>(count)*3);
  for (size_t i=0;i<inventory.size();++i) {
   inventory[i]={i%3==0 ? 102 : i%3==1 ? 200 : 201,"vial ingredient","an input",i+1<inventory.size()?&inventory[i+1]:nullptr};
  }
  ch.carrying=&inventory[0];
  accept=admitted;allocations=notches=submissions=0;
  char argument[]="";
  do_mixpoison(&ch,argument,0);
  const size_t planned=static_cast<size_t>(count>64?64:count);
  assert(submissions==1 && submitted_outputs.size()==planned && submitted_inputs.size()==planned*3);
  assert(allocations==static_cast<int>(planned) && notches==0);
  assert(ch.carrying==&inventory[0]);
  for (size_t i=0;i+1<inventory.size();++i) assert(inventory[i].next_content==&inventory[i+1]);
  if (admitted) {
   assert(saved_terms.output_count==planned && notches==0);
   for (P_obj output:submitted_outputs) delete output;
  }
 }
 // A failed allocation can freeze a partial batch. It must select only the
 // recipes that produced complete candidates, and refusal leaves every input.
 for (int limit : {0,2}) for (bool admitted : {false,true}) {
  character ch; std::vector<object> inventory(30);
  for (size_t i=0;i<inventory.size();++i)
   inventory[i]={i%3==0?102:i%3==1?200:201,"vial ingredient","input",i+1<inventory.size()?&inventory[i+1]:nullptr};
  ch.carrying=&inventory[0]; allocation_limit=limit; accept=admitted;
  allocations=submissions=0; char argument[]=""; do_mixpoison(&ch,argument,0);
  assert(ch.carrying==&inventory[0] && allocations==limit);
  assert(submissions==(limit?1:0));
  if (limit) {
   assert(submitted_inputs.size()==6 && submitted_outputs.size()==2 && saved_terms.output_count==2);
   if (admitted) for (P_obj output:submitted_outputs) delete output;
  }
 }
 allocation_limit=1000;
 character missing;
 object vial{102,"vial","a vial"};missing.carrying=&vial;
 submissions=allocations=0;char argument[]="";do_mixpoison(&missing,argument,0);
 assert(submissions==0 && allocations==0 && missing.carrying==&vial);
}
'''
with tempfile.TemporaryDirectory(prefix="duris-poison-craft-") as temporary:
    source = Path(temporary) / "poison.cpp"
    binary = Path(temporary) / "poison"
    source.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc", str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Issue 551 executable poison batch and refusal regression passed")

# Exercise the production notch implementation, including the old balance
# gates and learning rolls, with the random stream and effects observable.
notch_source = (ROOT / "src/guild/guild.c").read_text()
notch_functions = section(notch_source, "skill_notch_outcome skill_notch_prepare", "void spell_learning")
notch_harness = r'''
#include "guild/skill_notch.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>
struct learned_skill { int learned=60,taught=100; };
struct player { learned_skill skills[200]; };
struct pc_union_type { player *pc; };
struct character { pc_union_type only; bool alive=true,npc=false,safe=false,learning=false,chaos=false;
 bool physical_timer=false; int level=56,intelligence=50; };
using P_char=character *;
#define IS_ALIVE(ch) ((ch)->alive)
#define IS_NPC(ch) ((ch)->npc)
#define IS_ROOM(room,flags) false
#define IS_FIGHTING(ch) false
#define IS_PC_PET(ch) false
#define GET_OPPONENT(ch) (ch)
#define GET_LEVEL(ch) ((ch)->level)
#define GET_C_INT(ch) ((ch)->intelligence)
#define IS_HARDCORE(ch) false
#define BOUNDED(lo,v,hi) std::clamp((v),(lo),(hi))
#define MAX(a,b) std::max((a),(b))
#define IS_SET(value,flag) ((value)&(flag))
constexpr int FALSE=0,TRUE=1,MAX_STRING_LENGTH=1024,TAR_PHYS=1;
constexpr int TAG_PHYS_SKILL_NOTCH=1000,TAG_MENTAL_SKILL_NOTCH=1001,WAIT_MIN=60,SPELL_LEARNING=1;
struct skill_definition { const char *name="poison"; int targets=TAR_PHYS; } skills[200];
struct teacher { int vnum,pre_requisite,pre_req_lvl,skill; } epic_teachers[]={{1,111,62,112},{0,0,0,0}};
struct hardcore { float bonus_skill_notch_multiplier=1; } configuration;
hardcore *hardcore_config_get() { return &configuration; }
std::vector<int> rolls; size_t roll_index=0; int messages=0,timers=0;
int number(int low,int high) { assert(roll_index<rolls.size()); int v=rolls[roll_index++]; assert(v>=low&&v<=high); return v; }
int get_property(const char *,int value) { return value; }
bool chaos_mud_enabled() { return false; }
bool affected_by_skill(P_char,int) { return false; }
bool affected_by_spell(P_char ch,int) { return ch->learning; }
bool affect_timer(P_char,int duration,int tag) { assert(duration==300&&tag==1000); ++timers; return true; }
void send_to_char(const char *,P_char) { ++messages; }
'''
notch_harness += notch_functions + r'''
int main() {
 player pc; character ch{{&pc}};
 rolls={1251}; roll_index=0;
 auto failed=skill_notch_prepare(&ch,111,6.25);
 assert(failed.learned_before==60&&failed.learned_after==60&&roll_index==1);
 assert(pc.skills[111].learned==60&&messages==0&&timers==0);
 rolls={1250,0}; roll_index=0;
 auto frozen=skill_notch_prepare(&ch,111,6.25);
 assert(frozen.learned_after==61&&pc.skills[111].learned==60&&messages==0&&timers==0);
 const auto used=roll_index; rolls.clear(); // Any publication reroll now fails.
 assert(skill_notch_apply(&ch,111,frozen));
 assert(pc.skills[111].learned==61&&messages==1&&timers==1&&roll_index==used);
 assert(!skill_notch_apply(&ch,111,frozen)&&messages==1&&timers==1);
 pc.skills[111].learned=60; ch.learning=true; rolls={1,1,1,0}; roll_index=0;
 frozen=skill_notch_prepare(&ch,111,6.25);
 assert(frozen.learned_after==63&&roll_index==4&&pc.skills[111].learned==60);
 rolls.clear(); assert(skill_notch_apply(&ch,111,frozen));
 assert(pc.skills[111].learned==63&&messages==5&&timers==2);
 pc.skills[111].learned=70; pc.skills[111].taught=50;
 frozen=skill_notch_prepare(&ch,111,6.25);
 assert(frozen.learned_after==50&&pc.skills[111].learned==70);
 assert(!skill_notch_apply(&ch,111,frozen)&&pc.skills[111].learned==50);
 pc.skills[111].learned=60; pc.skills[111].taught=100; ch.level=10;
 frozen=skill_notch_prepare(&ch,111,6.25);
 assert(frozen.learned_after==30); assert(!skill_notch_apply(&ch,111,frozen));
 assert(pc.skills[111].learned==30);
}
'''
with tempfile.TemporaryDirectory(prefix="duris-frozen-notch-") as temporary:
    source = Path(temporary) / "notch.cpp"
    binary = Path(temporary) / "notch"
    source.write_text(notch_harness)
    subprocess.run(["g++", "-std=c++20", "-Dwipe2011=1", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-fno-pie", "-no-pie", "-Isrc", str(source),
                    "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
print("Issue 551 frozen skill planning, probability, learning, caps and idempotent application passed")
