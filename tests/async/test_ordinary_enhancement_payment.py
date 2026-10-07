#!/usr/bin/env python3
"""Run ordinary enhancement with failed wallet admission and configured fees."""
from _paths import ROOT, extract_function
import os
from pathlib import Path
import subprocess
import tempfile

HARNESS = r'''
#include "economy/enhancement_original_search.h"
#include "economy/enhancement_affect_policy.h"
#include "economy/enhancement_price.h"
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <initializer_list>
constexpr int MAX_STRING_LENGTH=4096, NOWHERE=-1, VIRTUAL=0, FALSE=0, TRUE=1, TO_CHAR=0;
constexpr int ITEM_TAKE=1, ITEM_HOLD=2, ITEM_ATTACH_BELT=4, ITEM_WEAR_BACK=8,
 ITEM_GUILD_INSIGNIA=16, ITEM_SECRET=1, ITEM_NODROP=2, ITEM_INVISIBLE=4, ITEM_NOREPAIR=8;
struct character { struct { int level=50; } player; int in_room=NOWHERE; int64_t money=10000; };
struct object { int wear_flags=32, extra_flags=0, R_num=0, value=10; const char* short_description="fixture";
 unsigned long bitvector=0,bitvector2=0,bitvector3=0,bitvector4=0,bitvector5=0; };
using P_char=character*; using P_obj=object*;
struct enhance_index_entry { int ival=11, wear_flags=32, vnum=2; enhance_index_entry* next=nullptr; };
enhance_index_entry entry; enhance_index_entry* enhance_ival_table[1]={&entry};
struct room { int number; }; room world[1];
struct fixture_index { int virtual_number; }; fixture_index obj_index[2]={{1},{3}};
int enhance_material_ival_delta=0, enhance_search_max_attempts=1,
 enhance_guild_insignia_ival_bonus=0, enhance_cost_low_ival_threshold=20,
 enhance_cost_low_amount=500, enhance_cost_high_amount=500,
 enhance_luck_extreme_range=1, enhance_luck_very_range=1, enhance_luck_lucky_range=1,
 enhance_ival_gain_extreme=1, enhance_ival_gain_very=1, enhance_ival_gain_lucky=1,
 enhance_ival_gain_normal=1, enhance_original_max_roll=0, enhance_original_cascade_down_first=1,
 enhance_ival_cap=100;
int attempts, debits, published, input_retired, output_retired, reads;
int luck=0,roll=INT_MAX,maximum_item_value=150;
bool reject_debit, pouch_mode;
object output, source_item, material_item;
#define GET_C_LUK(ch) luck
#define GET_LEVEL(ch) ((ch)->player.level)
#define GET_MONEY(ch) ((ch)->money)
#define GET_NAME(ch) "fixture"
#define OBJ_VNUM(obj) (obj_index[(obj)->R_num].virtual_number)
#define IS_SET(flags, bit) ((flags)&(bit))
#define REMOVE_BIT(flags, bit) ((flags)&=~(bit))
#define SET_BIT(flags, bit) ((flags)|=(bit))
int checked_snprintf(char* buf,size_t size,const char* format,...) {
 va_list args; va_start(args,format); int result=vsnprintf(buf,size,format,args); va_end(args); return result;
}
unsigned long enhance_allow_mask=0,enhance_allow_mask2=0,enhance_allow_mask3=0,
 enhance_allow_mask4=0,enhance_allow_mask5=0;
@POLICY@
bool chaos_material_pouch_is_active(P_obj o) { return pouch_mode && o==&material_item; }
P_obj chaos_material_pouch_find(P_char) { return &material_item; }
int itemvalue(P_obj o) { return o->value; }
#include <vector>
#include <algorithm>
int search_mode=-1;
enhance_index_entry *replacement_next=nullptr;
std::vector<int> searched_values,read_vnums,failed_vnums;
int rng_calls=0;
int number(int,int) { if(search_mode>=0) ++rng_calls;return roll; }
int enhance_hash(int value) { if(search_mode>=0) searched_values.push_back(value);return 0; }
int enhance_maximum_item_value(int) { return maximum_item_value; }
P_obj read_object(int vnum,int) {
 ++reads;
 if(search_mode>=0) {
  read_vnums.push_back(vnum);
  if(search_mode==8 && vnum==2) { enhance_original_max_roll=2;enhance_original_cascade_down_first=0; }
  if(search_mode==8 && vnum==3) enhance_original_cascade_down_first=1;
  if(search_mode==9) enhance_original_max_roll=0;
  if(search_mode==10) enhance_ival_cap=0;
  if(search_mode==14 && vnum==2) entry.next=replacement_next;
  if(search_mode==15 && vnum==2) source_item.R_num=1;
  if(search_mode==16 && vnum==2) enhance_search_max_attempts=0;
  if(search_mode==17 && vnum==2) source_item.wear_flags=material_item.wear_flags=16;
  if(std::find(failed_vnums.begin(),failed_vnums.end(),vnum)!=failed_vnums.end()) return nullptr;
 }
 return &output;
}
void act(const char*,int,P_char,P_obj,void*,int) {}
void send_to_char(const char*,P_char) {}
void statuslog(int,const char*,...) {}
void obj_to_char(P_obj o,P_char) { assert(o==&output); ++published; }
void obj_from_char(P_obj) {}
void extract_obj(P_obj o) {
 if(o==&output) ++output_retired;
 else { assert(o==&source_item || o==&material_item); ++input_retired; }
}
int SUB_MONEY(P_char ch,int amount,int) {
 ++attempts;
 if(reject_debit || amount<=0 || ch->money<amount) return -1;
 ch->money-=amount; ++debits; return 0;
}
@FUNCTION@
void reset_counts() { attempts=debits=published=input_retired=output_retired=reads=0; }
void reset_affect_facts() {
 for (P_obj item : {&source_item,&material_item,&output})
  item->bitvector=item->bitvector2=item->bitvector3=item->bitvector4=item->bitvector5=0;
 enhance_allow_mask=enhance_allow_mask2=enhance_allow_mask3=enhance_allow_mask4=enhance_allow_mask5=0;
}
int main() {
 enhancement_affect_words affects{},allowed{};
 assert(!enhancement_affects_banned(affects,allowed));
 affects.fill(ULONG_MAX); allowed.fill(ULONG_MAX);
 assert(!enhancement_affects_banned(affects,allowed));
 const unsigned long high_bit=ULONG_MAX ^ (ULONG_MAX >> 1);
 unsigned long object::* const members[5]={&object::bitvector,&object::bitvector2,
  &object::bitvector3,&object::bitvector4,&object::bitvector5};
 unsigned long* const masks[5]={&enhance_allow_mask,&enhance_allow_mask2,
  &enhance_allow_mask3,&enhance_allow_mask4,&enhance_allow_mask5};
 assert(is_enhance_banned(nullptr));
 for (unsigned int word=0;word<5;++word) {
  affects.fill(0); allowed.fill(0); affects[word]=high_bit|1UL;
  assert(enhancement_affects_banned(affects,allowed));
  allowed[word]=1UL;
  assert(enhancement_affects_banned(affects,allowed));
  allowed[word]=ULONG_MAX;
  assert(!enhancement_affects_banned(affects,allowed));
  allowed[word]=0; allowed[(word+1)%5]=ULONG_MAX;
  assert(enhancement_affects_banned(affects,allowed));
  object native;
  reset_affect_facts(); native.*members[word]=high_bit|1UL;
  assert(is_enhance_banned(&native));
  *masks[word]=1UL; assert(is_enhance_banned(&native));
  *masks[word]=ULONG_MAX; assert(!is_enhance_banned(&native));
 }
 affects.fill(0); allowed.fill(0); affects[0]=1;
 const bool prepared=enhancement_affects_banned(affects,allowed);
 enhance_allow_mask=ULONG_MAX;
 assert(prepared && enhancement_affects_banned(affects,allowed)==prepared);
 object changed; changed.bitvector=1;
 assert(!is_enhance_banned(&changed));
 reset_affect_facts();

 source_item.value=10; material_item.value=100; entry.ival=11;
 reject_debit=false; pouch_mode=false;
 for (unsigned int word=0;word<5;++word) for (bool source_banned : {false,true}) {
  reset_affect_facts(); reset_counts(); character actor;
  P_obj banned_item=source_banned ? &source_item : &material_item;
  banned_item->*members[word]=high_bit;
  assert(banned_item->*members[word]==high_bit);
  enhance(&actor,&source_item,&material_item);
  assert(actor.money==10000 && !attempts && !debits && !reads && !published && !input_retired && !output_retired);
 }
 reset_affect_facts(); reset_counts(); character pouch_actor;
 material_item.bitvector5=high_bit; pouch_mode=true;
 enhance(&pouch_actor,&source_item,&material_item);
 assert(pouch_actor.money==9500 && attempts==1 && debits==1 && published==1 && input_retired==1);
 assert(material_item.bitvector5==high_bit);
 reset_affect_facts();

 int owned_cost=42;
 assert(enhancement_prepare_ordinary_price(20,20,1000,-1,&owned_cost) && owned_cost==1000);
 assert(!enhancement_prepare_ordinary_price(21,20,1000,-1,&owned_cost) && owned_cost==1000);
 assert(enhancement_prepare_ordinary_price(INT_MIN,20,0,INT_MAX,&owned_cost) && owned_cost==0);
 assert(enhancement_prepare_ordinary_price(INT_MAX,20,-1,INT_MAX,&owned_cost) && owned_cost==INT_MAX);
 assert(!enhancement_prepare_ordinary_price(20,20,1,2,nullptr));
 int repeated=0;
 assert(enhancement_prepare_ordinary_price(INT_MAX,20,-1,INT_MAX,&repeated) && repeated==owned_cost);

 character numeric_actor;
 reject_debit=false; pouch_mode=false;
 source_item.value=10; material_item.value=100; entry.ival=11;
 enhance_material_ival_delta=INT_MIN; reset_counts();
 enhance(&numeric_actor,&source_item,&material_item);
 assert(numeric_actor.money==10000 && !attempts && !published && !input_retired && !reads);
 enhance_material_ival_delta=0; enhance_ival_gain_normal=INT_MAX; reset_counts();
 enhance(&numeric_actor,&source_item,&material_item);
 assert(numeric_actor.money==10000 && !attempts && !published && !input_retired && !reads);
 enhance_ival_gain_normal=1; enhance_ival_cap=INT_MAX; enhance_original_max_roll=1;
 reset_counts(); enhance(&numeric_actor,&source_item,&material_item);
 assert(numeric_actor.money==9500 && published==1 && input_retired==2);
 numeric_actor.money=10000; luck=2;roll=1;enhance_search_max_attempts=INT_MAX;
 reset_counts(); enhance(&numeric_actor,&source_item,&material_item);
 assert(numeric_actor.money==9500 && published==1 && input_retired==2);
 luck=0;roll=INT_MAX;maximum_item_value=INT_MAX;enhance_ival_gain_normal=0;
 source_item.value=INT_MAX;material_item.value=INT_MAX;entry.ival=INT_MAX-1;
 enhance_original_max_roll=INT_MAX;reset_counts();numeric_actor.money=10000;
 enhance(&numeric_actor,&source_item,&material_item);
 assert(numeric_actor.money==9500 && published==1 && input_retired==2);
 maximum_item_value=150;enhance_ival_gain_normal=1;enhance_original_max_roll=0;
 enhance_search_max_attempts=1;enhance_ival_cap=100;
 for(bool pouch : {false,true}) for(int value : {10,21}) {
  source_item.value=value; material_item.value=100; entry.ival=value+1; pouch_mode=pouch;
  for(int fee : {500,0,-1,INT_MIN}) for(bool reject : {true,false}) {
   character actor;
   enhance_cost_low_amount=enhance_cost_high_amount=fee;
   attempts=debits=published=input_retired=output_retired=reads=0;
   reject_debit=reject; output.extra_flags=ITEM_SECRET|ITEM_NODROP|ITEM_INVISIBLE;
   enhance(&actor,&source_item,&material_item);
   const bool success=fee>=0 && (!reject || fee==0);
   assert(actor.money==(success ? 10000-fee : 10000));
   assert(debits==(success && fee>0 ? 1 : 0));
   assert(attempts==(fee>0 ? 1 : 0));
   assert(published==(success ? 1 : 0));
   assert(input_retired==(success ? (pouch ? 1 : 2) : 0));
   assert(output_retired==(fee>0 && reject ? 1 : 0));
   assert(reads==(fee<0 ? 0 : 1));
   assert(output.extra_flags==(success ? ITEM_NOREPAIR : ITEM_SECRET|ITEM_NODROP|ITEM_INVISIBLE));
  }
 }
 auto prepare_search=[&](int mode) {
  search_mode=mode;searched_values.clear();read_vnums.clear();failed_vnums.clear();rng_calls=0;
  reset_counts();reset_affect_facts();source_item.value=10;material_item.value=100;
  source_item.wear_flags=material_item.wear_flags=32;source_item.R_num=0;replacement_next=nullptr;entry={};enhance_ival_table[0]=&entry;
  enhance_cost_low_amount=enhance_cost_high_amount=500;reject_debit=false;pouch_mode=false;
  enhance_material_ival_delta=enhance_guild_insignia_ival_bonus=0;
  enhance_search_max_attempts=10;enhance_ival_cap=100;enhance_original_max_roll=2;
  enhance_original_cascade_down_first=1;enhance_ival_gain_normal=1;luck=0;roll=INT_MAX;
  maximum_item_value=INT_MAX;numeric_actor.money=10000;
 };
 auto check_search=[&](std::initializer_list<int> values,std::initializer_list<int> vnums,bool accepted) {
  assert(searched_values==std::vector<int>(values));assert(read_vnums==std::vector<int>(vnums));
  assert(rng_calls==3 && reads==static_cast<int>(vnums.size()));
  assert(numeric_actor.money==(accepted ? 9500 : 10000));
  assert(attempts==(accepted ? 1 : 0) && debits==(accepted ? 1 : 0));
  assert(published==(accepted ? 1 : 0) && input_retired==(accepted ? 2 : 0) && output_retired==0);
 };
 prepare_search(0);enhance(&numeric_actor,&source_item,&material_item);check_search({11},{2},true);
 prepare_search(1);entry.ival=12;
 enhance(&numeric_actor,&source_item,&material_item);check_search({11,10,12},{2},true);
 prepare_search(2);entry.ival=10;enhance_original_cascade_down_first=0;
 enhance(&numeric_actor,&source_item,&material_item);check_search({11,12,10},{2},true);
 prepare_search(3);enhance_ival_table[0]=nullptr;enhance_search_max_attempts=0;
 enhance(&numeric_actor,&source_item,&material_item);check_search({11},{},false);
 prepare_search(4);enhance_ival_table[0]=nullptr;enhance_search_max_attempts=1;
 enhance(&numeric_actor,&source_item,&material_item);check_search({11,10,12},{},false);
 prepare_search(5);source_item.value=1;enhance_ival_gain_normal=0;entry.ival=2;enhance_original_max_roll=1;
 enhance(&numeric_actor,&source_item,&material_item);check_search({1,2},{2},true);
 prepare_search(5);source_item.value=material_item.value=INT_MAX;enhance_ival_gain_normal=0;
 entry.ival=INT_MAX-1;enhance_ival_cap=INT_MAX;enhance_original_cascade_down_first=0;enhance_original_max_roll=1;
 enhance(&numeric_actor,&source_item,&material_item);check_search({INT_MAX,INT_MAX-1},{2},true);
 prepare_search(6);enhance_ival_cap=10;enhance_original_max_roll=1;
 enhance(&numeric_actor,&source_item,&material_item);check_search({11},{2},true);
 prepare_search(6);enhance_ival_cap=9;enhance_original_max_roll=1;entry.ival=10;
 enhance(&numeric_actor,&source_item,&material_item);check_search({10},{2},true);
 prepare_search(6);enhance_ival_cap=9;enhance_original_max_roll=0;
 enhance(&numeric_actor,&source_item,&material_item);check_search({},{},false);
 prepare_search(7);
 enhance_index_entry same,wrong_value,unreadable,found,later;
 entry.wear_flags=16;entry.next=&same;same.vnum=1;same.next=&wrong_value;
 wrong_value.ival=12;wrong_value.next=&unreadable;unreadable.vnum=3;unreadable.next=&found;
 found.vnum=4;found.next=&later;later.vnum=5;failed_vnums={3};
 enhance(&numeric_actor,&source_item,&material_item);check_search({11},{3,4},true);
 prepare_search(8);enhance_original_max_roll=1;
 enhance_index_entry step1,step2;entry.next=&step1;step1.ival=12;step1.vnum=3;step1.next=&step2;
 step2.ival=13;step2.vnum=4;failed_vnums={2,3};
 enhance(&numeric_actor,&source_item,&material_item);check_search({11,12,12,9,13},{2,3,3,4},true);
 prepare_search(9);failed_vnums={2};
 enhance(&numeric_actor,&source_item,&material_item);check_search({11},{2},false);
 prepare_search(10);enhance_original_max_roll=1;failed_vnums={2};
 enhance(&numeric_actor,&source_item,&material_item);check_search({11},{2},false);
 prepare_search(11);enhance_original_max_roll=-1;
 enhance(&numeric_actor,&source_item,&material_item);check_search({},{},false);
 prepare_search(11);enhance_ival_table[0]=nullptr;enhance_search_max_attempts=-1;
 enhance(&numeric_actor,&source_item,&material_item);check_search({11},{},false);
 prepare_search(12);reject_debit=true;
 enhance(&numeric_actor,&source_item,&material_item);
 assert((searched_values==std::vector<int>{11}) && (read_vnums==std::vector<int>{2}));
 assert(numeric_actor.money==10000 && attempts==1 && !debits && !published && !input_retired && output_retired==1);
 prepare_search(14);
 enhance_index_entry original_next,replacement;original_next.vnum=3;replacement.vnum=4;
 entry.next=&original_next;replacement_next=&replacement;failed_vnums={2};
 enhance(&numeric_actor,&source_item,&material_item);check_search({11},{2,4},true);
 prepare_search(15);
 enhance_index_entry now_same,after_same;now_same.vnum=3;after_same.vnum=4;
 entry.next=&now_same;now_same.next=&after_same;failed_vnums={2};
 enhance(&numeric_actor,&source_item,&material_item);check_search({11},{2,4},true);
 prepare_search(16);enhance_search_max_attempts=1;
 enhance_index_entry captured_budget;captured_budget.ival=12;captured_budget.vnum=3;
 entry.next=&captured_budget;failed_vnums={2};
 enhance(&numeric_actor,&source_item,&material_item);check_search({11,10,12},{2,3},true);
 prepare_search(17);
 enhance_index_entry captured_wear;captured_wear.vnum=3;entry.next=&captured_wear;failed_vnums={2};
 enhance(&numeric_actor,&source_item,&material_item);check_search({11},{2,3},true);
 prepare_search(18);source_item.value=1;enhance_ival_gain_normal=-2;
 enhance_original_max_roll=3;enhance_search_max_attempts=0;entry.ival=1;
 enhance(&numeric_actor,&source_item,&material_item);check_search({},{},false);
 prepare_search(18);source_item.value=1;enhance_ival_gain_normal=-2;
 enhance_original_max_roll=3;enhance_search_max_attempts=2;entry.ival=1;
 enhance(&numeric_actor,&source_item,&material_item);check_search({1},{2},true);
 search_mode=-1;

}
'''
with tempfile.TemporaryDirectory(prefix='duris-ordinary-enhance-payment-') as temporary:
    cpp=Path(temporary)/'payment.cpp'
    binary=Path(temporary)/'payment'
    cpp.write_text(HARNESS.replace('@POLICY@',extract_function('enhance.c','bool is_enhance_banned('))
                          .replace('@FUNCTION@',extract_function('enhance.c','void enhance(')))
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                    '-fno-pie','-no-pie','-Isrc',str(cpp),'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True,timeout=30)
print('Ordinary enhancement: wide material/value/cascade/search bounds, payment refusal cleanup and valid free/paid quotes passed')
