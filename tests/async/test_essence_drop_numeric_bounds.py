#!/usr/bin/env python3
"""Exercise the production NPC essence drop thresholds at configured integer bounds."""
from _paths import ROOT, extract_function
import os
from pathlib import Path
import subprocess
import tempfile

HARNESS=r'''
#include "economy/enhancement_essence_reward.h"
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <initializer_list>
#include <string>
#include <vector>
constexpr int VIRTUAL=0,LOG_SYS=0,LNK_PET=0;
struct character { int level=50,in_room=1,vnum=123;bool elite=false;const char* name="fixture"; };
struct object { int vnum=0;const char* short_description="fixture"; };
using P_char=character*;using P_obj=object*;
struct enhance_essence_zone_rule { int primary_roll_max=0,max_roll_max=0,elite_level_multiplier=0; };
int enhance_essence_drop_enabled=1,enhance_essence_minimum_level=1,
 enhance_essence_maximum_level=1000000,enhance_essence_primary_roll_max=3000,
 enhance_essence_max_roll_max=4000,enhance_essence_elite_level_multiplier=1;
enhance_essence_zone_rule* current_rule=nullptr;
std::vector<std::string> trace;bool mutate_roll=false,mutate_debug=false,controlled_ordinal=false;int no_reward_logs=0;character* live_mob=nullptr;
object gift;std::vector<int> rolls;size_t roll_index;int reads,grants;
bool missing_template=false;std::vector<std::string> debug_messages;
#define GET_LEVEL(ch) ((ch)->level)
#define GET_VNUM(ch) ((ch)->vnum)
const char* j_name(P_char mob){return mob->name;}
#define J_NAME(ch) j_name(ch)
#define IS_ELITE(ch) ((ch)->elite)
#define ROOM_ZONE_NUMBER(room) 42
#define IS_PC_PET(ch) false
#define OBJ_VNUM(obj) ((obj)->vnum)
P_char get_linked_char(P_char ch,int){return ch;}
enhance_essence_zone_rule* enhance_find_essence_zone_rule(int zone){assert(zone==42);trace.push_back("zone");return current_rule;}
int number(int low,int high){trace.push_back("roll:"+std::to_string(low)+":"+std::to_string(high));assert(roll_index<rolls.size());int value=rolls[roll_index++];if(mutate_roll&&roll_index==1){enhance_essence_max_roll_max=1;enhance_essence_elite_level_multiplier=1;live_mob->level=1;live_mob->elite=false;live_mob->vnum=999;live_mob->name="changed";}// Only the two explicit policy fallthrough controls can supply an invalid third ordinal.
assert((controlled_ordinal && roll_index==3) || (value>=low && value<=high));return value;}
void debug(const char* format,...){trace.push_back("debug");if(mutate_debug&&debug_messages.empty())enhance_essence_max_roll_max=1;char buf[512];va_list args;va_start(args,format);vsnprintf(buf,sizeof(buf),format,args);va_end(args);debug_messages.emplace_back(buf);}
void logit(int,const char* message){assert(controlled_ordinal);assert(std::string(message)=="enhance_load_essence_drop selected no reward");++no_reward_logs;trace.push_back("no-reward");}
P_obj read_object(int vnum,int){trace.push_back("read:"+std::to_string(vnum));++reads;gift.vnum=vnum;return missing_template ? nullptr : &gift;}
void obj_to_char(P_obj obj,P_char mob){trace.push_back("grant");assert(obj==&gift && mob);++grants;}
@FUNCTION@
void reset(){trace.clear();no_reward_logs=0;reads=grants=0;roll_index=0;debug_messages.clear();rolls.clear();}
int main(){
 character mob,killer;mob.elite=true;
 enhance_essence_elite_level_multiplier=INT_MAX;
 enhance_essence_primary_roll_max=enhance_essence_max_roll_max=INT_MAX;
 reset();rolls={INT_MAX,INT_MAX,8};enhance_load_essence_drop(&mob,&killer);
 assert(reads==1 && grants==1 && gift.vnum==400253 && roll_index==3);
 assert(debug_messages.front().find("107374182350")!=std::string::npos);
 enhance_essence_elite_level_multiplier=1;
 enhance_essence_primary_roll_max=3000;enhance_essence_max_roll_max=4000;
 enhance_essence_zone_rule zone{INT_MAX,INT_MAX,INT_MAX};current_rule=&zone;
 reset();rolls={INT_MAX,INT_MAX,1};enhance_load_essence_drop(&mob,&killer);
 assert(reads==1 && grants==1 && gift.vnum==400239);
 current_rule=nullptr;mob.elite=false;
 reset();rolls={50};enhance_load_essence_drop(&mob,&killer);
 assert(!reads && !grants && roll_index==1);
 reset();rolls={49,50,13};enhance_load_essence_drop(&mob,&killer);
 assert(reads==1 && grants==1 && gift.vnum==400258);
 reset();rolls={49,49,1};enhance_load_essence_drop(&mob,&killer);
 assert(reads==1 && grants==1 && gift.vnum==400239);
 enhance_essence_drop_enabled=0;reset();enhance_load_essence_drop(&mob,&killer);
 assert(!reads && !grants && !roll_index);
 enhance_essence_drop_enabled=1;
 for(int level:{0,1000001}){mob.level=level;reset();enhance_load_essence_drop(&mob,&killer);assert(!reads && !grants && !roll_index);}
 mob.level=50;mob.elite=true;enhance_essence_elite_level_multiplier=2;
 reset();rolls={99,99,8};enhance_load_essence_drop(&mob,&killer);
 assert(reads==1 && grants==1 && gift.vnum==400253);
 missing_template=true;reset();rolls={99,99,8};enhance_load_essence_drop(&mob,&killer);
 assert(reads==1 && !grants);

 missing_template=false;current_rule=nullptr;mob.elite=false;mob.level=50;
 enhance_essence_primary_roll_max=3000;enhance_essence_max_roll_max=4000;enhance_essence_elite_level_multiplier=1;
 for(int selector=1;selector<=8;++selector){reset();rolls={49,49,selector};enhance_load_essence_drop(&mob,&killer);
  assert(gift.vnum==400237+selector*2 && reads==1 && grants==1 && roll_index==3);
  assert((trace==std::vector<std::string>{"zone","roll:1:3000","debug","roll:1:4000","roll:1:8","read:"+std::to_string(gift.vnum),"debug","grant"}));}
 const int ordinary[]={400238,400240,400242,400244,400246,400248,400250,400252,400254,400255,400256,400257,400258};
 for(int selector=1;selector<=13;++selector){reset();rolls={49,50,selector};enhance_load_essence_drop(&mob,&killer);
  assert(gift.vnum==ordinary[selector-1] && reads==1 && grants==1 && roll_index==3);
  assert((trace==std::vector<std::string>{"zone","roll:1:3000","debug","roll:1:4000","roll:1:13","read:"+std::to_string(gift.vnum),"debug","grant"}));}
 enhance_essence_zone_rule sparse{17,-4,0};current_rule=&sparse;reset();rolls={16,50,9};enhance_load_essence_drop(&mob,&killer);
 assert((trace==std::vector<std::string>{"zone","roll:1:17","debug","roll:1:4000","roll:1:13","read:400254","debug","grant"}));
 current_rule=nullptr;mob.elite=true;enhance_essence_elite_level_multiplier=2;live_mob=&mob;mutate_roll=true;
 reset();rolls={99,99,8};enhance_load_essence_drop(&mob,&killer);
 assert(reads==1 && grants==1 && gift.vnum==400253 && debug_messages.front().find("changed' (999) moblvl 100.")!=std::string::npos);
 assert((trace==std::vector<std::string>{"zone","roll:1:3000","debug","roll:1:4000","roll:1:8","read:400253","debug","grant"}));
 mutate_roll=false;mutate_debug=true;mob.level=50;enhance_essence_max_roll_max=4000;
 reset();rolls={49,50,13};enhance_load_essence_drop(&mob,&killer);
 assert(reads==1 && gift.vnum==400258 && trace[3]=="roll:1:4000");mutate_debug=false;
 enhance_essence_drop_enabled=0;reset();enhance_load_essence_drop(&mob,&killer);assert(trace.empty());enhance_essence_drop_enabled=1;
 mob.level=0;reset();enhance_load_essence_drop(&mob,&killer);assert(trace.empty());
 mob.level=50;reset();rolls={50};enhance_load_essence_drop(&mob,&killer);assert((trace==std::vector<std::string>{"zone","roll:1:3000"}));


 // Controlled policy observations outside native RNG's range; original/native cases keep their range assertions.
 controlled_ordinal=true;enhance_essence_primary_roll_max=3000;enhance_essence_max_roll_max=4000;
 reset();rolls={49,49,9};enhance_load_essence_drop(&mob,&killer);
 assert(!reads && !grants && no_reward_logs==1);
 assert((trace==std::vector<std::string>{"zone","roll:1:3000","debug","roll:1:4000","roll:1:8","no-reward"}));
 reset();rolls={49,50,14};enhance_load_essence_drop(&mob,&killer);
 assert(reads==1 && grants==1 && gift.vnum==14 && !no_reward_logs);
 assert((trace==std::vector<std::string>{"zone","roll:1:3000","debug","roll:1:4000","roll:1:13","read:14","debug","grant"}));
 controlled_ordinal=false;
}
'''
with tempfile.TemporaryDirectory(prefix='duris-essence-drop-bounds-') as temporary:
    cpp=Path(temporary)/'drop.cpp';binary=Path(temporary)/'drop'
    cpp.write_text(HARNESS.replace('@FUNCTION@',extract_function('enhance.c','static void enhance_load_essence_drop(')))
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                    '-fno-pie','-no-pie','-Isrc',str(cpp),'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True,timeout=30)
print('NPC essence drops: wide global/zone elite scaling, ordinary thresholds, level gates and missing-template handling passed')
