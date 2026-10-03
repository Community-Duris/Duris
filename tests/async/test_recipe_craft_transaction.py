#!/usr/bin/env python3
"""Execute the actual recipe planner with refused and deferred craft admission."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/economy/crafting.c").read_text()
helper = source[source.index("namespace\n{"):source.index("\nstatic int crafting_level_gate")]
harness = r'''
#include <cassert>
#include <cstdint>
#include <climits>
#include <cstring>
#include <new>
#include <string>
#include <vector>
struct object { uint64_t obj_uid; int vnum; object *next_content = nullptr; };
using P_obj = object *;
struct character { P_obj carrying = nullptr; int pid = 7; };
using P_char = character *;
#define OBJ_VNUM(o) ((o)->vnum)
#define GET_NAME(ch) "RecipeFixture"
#define GET_PID(ch) ((ch)->pid)
enum class craft_recipe_discipline : uint32_t { craft = 1, forge = 2 };
struct craft_recipe_continuation { uint32_t player_pid = 0;
 craft_recipe_discipline discipline = craft_recipe_discipline::craft;
 uint32_t experience = 0, recipe_vnum = 0; uint64_t output_uid = 0; };
enum crafting_mode { CRAFTING_MODE_CRAFT, CRAFTING_MODE_FORGE };
struct crafting_plan { int item_value, low_material_vnum, high_material_vnum;
 int low_material_count, high_material_count; bool magical; };
struct chaos_material_pouch_usage { int vnum; uint64_t count; };
enum class chaos_pouch_usage_mode { generated, collected };
enum class item_movement_reject { none };
struct item_transfer_result {};
constexpr size_t ITEM_TRANSFER_MAX_ITEMS = 3000;
constexpr int SKILL_CRAFT=11, SKILL_FORGE=12, EXP_BOON=3, LOG_DEBUG=1;
constexpr int TRUE=1, FALSE=0, TO_ROOM=1, TO_CHAR=2;
using completion_fn = void (*)(P_char,bool,const item_transfer_result &,unsigned int,const uint8_t *,size_t);
int notches=0, xp=0, saves=0, submissions=0, rate=1000;
bool accept=true;
object pouch{99,400300};
P_obj retained=nullptr;
std::vector<P_obj> selected;
std::vector<chaos_material_pouch_usage> generated;
std::vector<uint8_t> context;
completion_fn callback=nullptr;
int crafting_experience_per_ival() { return rate; }
int crafting_essence_vnum(crafting_mode mode) { return mode==CRAFTING_MODE_CRAFT?31:41; }
int crafting_tool_vnum(crafting_mode mode) { return mode==CRAFTING_MODE_CRAFT?32:42; }
P_obj chaos_material_pouch_find(P_char) { return &pouch; }
const char *item_movement_reject_name(item_movement_reject) { return "fixture"; }
void send_to_char(const char *,P_char) {}
void logit(int,const char *,...) {}
void notch_skill(P_char,int skill,double) { assert(skill==SKILL_CRAFT||skill==SKILL_FORGE);++notches; }
int gain_exp(P_char,P_char,int amount,int kind) { assert(kind==EXP_BOON);xp+=amount;return 0; }
void act(const char *,int,P_char,P_obj,P_char,int) {}
bool do_save_silent(P_char,int) { ++saves;return true; }
bool item_movement_transaction_submit_craft(P_char,P_obj const *inputs,size_t count,
 P_obj const *outputs,size_t output_count,int64_t recipe,completion_fn done,
 const void *data,size_t size,item_movement_reject *,P_obj kept,
 const chaos_material_pouch_usage *usage,size_t usage_count,
 chaos_pouch_usage_mode mode=chaos_pouch_usage_mode::generated,
 const craft_recipe_continuation *terms=nullptr)
{
 ++submissions;assert(output_count==1&&recipe==outputs[0]->vnum);
 assert(mode==chaos_pouch_usage_mode::generated);
 assert(terms&&terms->player_pid==7&&terms->experience==7000);
 assert(terms->recipe_vnum==static_cast<uint32_t>(outputs[0]->vnum)&&terms->output_uid==outputs[0]->obj_uid);
 selected.assign(inputs,inputs+count);retained=kept;generated.clear();
 if(usage_count) generated.assign(usage,usage+usage_count);
 context.assign(static_cast<const uint8_t *>(data),static_cast<const uint8_t *>(data)+size);
 callback=done;return accept;
}
''' + helper + r'''
int main()
{
 for (auto mode : {CRAFTING_MODE_CRAFT,CRAFTING_MODE_FORGE})
 for (bool virtual_material : {false,true}) for (bool admitted : {false,true})
 for (int low_count : {0,2})
 {
  std::vector<object> inventory;
  for(int i=0;i<low_count;++i) inventory.push_back({static_cast<uint64_t>(i+1),21});
  inventory.push_back({10,25});inventory.push_back({11,25}); // one high material remains
  inventory.push_back({12,crafting_essence_vnum(mode)});
  inventory.push_back({13,crafting_tool_vnum(mode)});
  inventory.push_back({14,crafting_tool_vnum(mode)}); // one tool remains
  inventory.push_back({15,777});
  for(size_t i=0;i+1<inventory.size();++i) inventory[i].next_content=&inventory[i+1];
  character actor{&inventory.front()};object output{80,100};
  crafting_plan plan{7,21,25,low_count,1,true};
  accept=admitted;submissions=notches=xp=saves=0;
  assert(submit_recipe_craft(&actor,&output,plan,mode,virtual_material)==admitted);
  assert(submissions==1&&notches==0&&xp==0&&saves==0);
  assert(actor.carrying==&inventory.front());
  for(size_t i=0;i+1<inventory.size();++i) assert(inventory[i].next_content==&inventory[i+1]);
  assert(selected.size()==static_cast<size_t>(virtual_material?2:low_count+3));
  assert(selected.back()->obj_uid==13);
  if(virtual_material) {
   assert(retained==&pouch&&generated.size()==static_cast<size_t>(low_count?2:1));
   assert(generated.back().vnum==25&&generated.back().count==1);
  } else assert(!retained&&generated.empty());
  callback(&actor,false,{},0,context.data(),context.size());
  assert(notches==0&&xp==0&&saves==0);
  if(admitted) {
   output.next_content=actor.carrying;actor.carrying=&output;
   callback(&actor,true,{},0,context.data(),context.size());
   assert(notches==0&&xp==0&&saves==0);
  }
 }
 character missing;object output{81,101};crafting_plan plan{7,21,25,0,1,true};
 submissions=0;assert(!submit_recipe_craft(&missing,&output,plan,CRAFTING_MODE_CRAFT,false));
 assert(submissions==0);
 rate=INT_MAX;assert(!submit_recipe_craft(&missing,&output,plan,CRAFTING_MODE_CRAFT,true));
 assert(submissions==0);
}
'''
with tempfile.TemporaryDirectory(prefix="duris-recipe-craft-") as temporary:
    cpp = Path(temporary) / "recipe.cpp"
    binary = Path(temporary) / "recipe"
    cpp.write_text(harness)
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra",
                    "-Werror", "-O1", "-g", "-fsanitize=address,undefined", "-fno-pie",
                    "-no-pie", str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
print("recipe craft: exact requirements, retained pouch, zero-low costs, refusals, deferred effects and XP bounds passed")
