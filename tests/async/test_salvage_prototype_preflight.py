#!/usr/bin/env python3
"""Run the production ordinary salvage command against absent output templates."""
from _paths import ROOT, extract_function
import os
from pathlib import Path
import re
import subprocess
import tempfile

command = extract_function("salvage.c", "void do_salvage(")
materials = sorted(set(re.findall(r"case (MAT_[A-Z_]+):", command)))
harness = r'''
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
constexpr int FALSE=0,TRUE=1,TO_CHAR=0,TO_ROOM=1,VIRTUAL=1,REAL=0,LOG_DEBUG=0;
constexpr int MAX_INPUT_LENGTH=256,MAX_STRING_LENGTH=4096,SKILL_SALVAGE=1;
constexpr int LOWEST_MAT_VNUM=400000,HIGHEST_MAT_VNUM=400209;
constexpr int MAG_ESSENCE_VNUM=400240,SALVAGE_RECIPE_VNUM=400300;
constexpr int ITEM_NODROP=1,STRUNG_DESC2=1;
constexpr int VOBJ_RANDOM_ARMOR=98,VOBJ_RANDOM_THRUSTED=99,VOBJ_RANDOM_WEAPON=100;
@MATERIALS@
struct object { int vnum=200,cost=0,material=MAT_LEATHER,extra_flags=0,str_mask=0;
 int value[8]{};const char* short_description="fixture";bool retired=false; };
struct character { object* carrying=nullptr;int in_room=0; };
using P_obj=object*;using P_char=character*;
int skill=1000,quality=20,missing=0,reads=0,grants=0,tools=1,tools_consumed=0,notches=0;
bool eligible=true;
std::vector<std::unique_ptr<object>> outputs;
std::vector<std::unique_ptr<char[]>> strings;
#define GET_CHAR_SKILL(ch,k) skill
#define GET_C_LUK(ch) 1
#define GET_LEVEL(ch) 50
#define IS_TRUSTED(ch) false
#define IS_SET(flags,bit) ((flags)&(bit))
#define SET_BIT(flags,bit) ((flags)|=(bit))
#define OBJ_VNUM(obj) ((obj)->vnum)
#define OBJ_SHORT(obj) ((obj)->short_description)
#define J_NAME(ch) "fixture"
#define ROOM_VNUM(room) (room)
namespace economic_gameplay_authority { bool active(){return false;} }
void one_argument(char* in,char* out){std::strcpy(out,in);}
void send_to_char(const char*,P_char){}
void act(const char*,int,P_char,P_obj,int,int){}
void debug(const char*,...){}
void logit(int,const char*,...){}
void notch_skill(P_char,int,int){++notches;}
void char_light(P_char){}
void room_light(int,int){}
P_obj get_obj_in_list_vis(P_char,char*,P_obj item){return item;}
bool is_salvageable(P_obj){return true;}
bool downgrade_salvage_material(P_char,P_obj,int){assert(false);return false;}
int get_matstart(P_obj){return 0;}
int itemvalue(P_obj){return quality;}
int number(int low,int){return low;}
int crafting_scientific_tools_vnum(){return 123;}
int vnum_in_inv(P_char,int){return tools;}
void vnum_from_inv(P_char,int,int){++tools_consumed;}
bool crafting_scientific_tools_prevent_breakage(){return true;}
int crafting_scientific_tools_recipe_roll_divisor(){return 2;}
int crafting_scientific_tools_recipe_player_multiplier(){return 2;}
double crafting_salvage_essence_luck_multiplier(){return 1;}
double crafting_salvage_essence_chance_multiplier(){return 1;}
bool has_affect(P_obj){return false;}
bool crafting_recipe_target_is_available(P_obj){return eligible;}
void crafting_configure_recipe_scroll(P_obj,P_obj){}
int real_object(int vnum){return vnum==missing ? -1 : 0;}
P_obj read_object(int vnum,int){
 ++reads;if(real_object(vnum)<0)return nullptr;
 outputs.push_back(std::make_unique<object>());outputs.back()->vnum=vnum;
 return outputs.back().get();
}
bool grant_salvage_item(P_char,P_obj obj){assert(obj);++grants;return true;}
void extract_obj(P_obj obj){assert(obj && !obj->retired);obj->retired=true;}
const char* str_dup(const char* text){
 auto buffer=std::make_unique<char[]>(std::strlen(text)+1);std::strcpy(buffer.get(),text);
 const char* result=buffer.get();strings.push_back(std::move(buffer));return result;
}
int checked_snprintf(char* out,size_t size,const char* format,...){
 va_list args;va_start(args,format);int result=vsnprintf(out,size,format,args);va_end(args);return result;
}
@COMMAND@
void reset(){reads=grants=tools_consumed=notches=0;outputs.clear();strings.clear();}
int main(){
 char argument[]="fixture";
 // Every material rarity must refuse before any output or tool/source retirement.
 for(int value:{5,10,15,20,21}){
  reset();quality=value;missing=400045+(value<=5 ? 0 : value<=10 ? 1 : value<=15 ? 2 : value<=20 ? 3 : 4);
  object original;character actor{&original};do_salvage(&actor,argument,0);
  assert(!original.retired && !tools_consumed && !grants && !reads && !notches);
 }
 // Eligible recipe discovery must not crash after granting materials/consuming tools.
 reset();quality=20;missing=SALVAGE_RECIPE_VNUM;
 object original;character actor{&original};do_salvage(&actor,argument,0);
 assert(!original.retired && !tools_consumed && !grants && !reads && !notches);
 // Unavailable recipes do not require a scroll prototype. Material rnum zero is valid.
 reset();eligible=false;object nonrecipe;actor.carrying=&nonrecipe;do_salvage(&actor,argument,0);
 assert(nonrecipe.retired && tools_consumed==1 && grants==2 && reads==2 && notches==1);
 reset();eligible=true;missing=0;object complete;actor.carrying=&complete;do_salvage(&actor,argument,0);
 assert(complete.retired && tools_consumed==1 && grants==3 && reads==3 && notches==1);
}
'''
harness = harness.replace("@MATERIALS@", "enum { " + ",".join(materials) + " };").replace("@COMMAND@", command)
with tempfile.TemporaryDirectory(prefix="duris-salvage-prototypes-") as temporary:
    source = Path(temporary) / "salvage.cpp"
    binary = Path(temporary) / "salvage"
    source.write_text(harness)
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-Wno-unused-parameter", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                    "-fno-pie", "-no-pie", "-g", str(source), "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
print("PASS: production salvage preserves the source/tools on missing material or eligible recipe templates; ordinary successful and unavailable-recipe paths pass")
