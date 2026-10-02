#!/usr/bin/env python3
"""Run ordinary enhancement with failed wallet admission and configured fees."""
from _paths import ROOT, extract_function
import os
from pathlib import Path
import subprocess
import tempfile

HARNESS = r'''
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <initializer_list>
constexpr int MAX_STRING_LENGTH=4096, NOWHERE=-1, VIRTUAL=0, FALSE=0, TO_CHAR=0;
constexpr int ITEM_TAKE=1, ITEM_HOLD=2, ITEM_ATTACH_BELT=4, ITEM_WEAR_BACK=8,
 ITEM_GUILD_INSIGNIA=16, ITEM_SECRET=1, ITEM_NODROP=2, ITEM_INVISIBLE=4, ITEM_NOREPAIR=8;
struct character { struct { int level=50; } player; int in_room=NOWHERE; int64_t money=10000; };
struct object { int wear_flags=32, extra_flags=0, R_num=0, value=10; const char* short_description="fixture"; };
using P_char=character*; using P_obj=object*;
struct enhance_index_entry { int ival=11, wear_flags=32, vnum=2; enhance_index_entry* next=nullptr; };
enhance_index_entry entry; enhance_index_entry* enhance_ival_table[1]={&entry};
struct room { int number; }; room world[1];
struct fixture_index { int virtual_number; }; fixture_index obj_index[1];
int enhance_material_ival_delta=0, enhance_search_max_attempts=1,
 enhance_guild_insignia_ival_bonus=0, enhance_cost_low_ival_threshold=20,
 enhance_cost_low_amount=500, enhance_cost_high_amount=500,
 enhance_luck_extreme_range=1, enhance_luck_very_range=1, enhance_luck_lucky_range=1,
 enhance_ival_gain_extreme=1, enhance_ival_gain_very=1, enhance_ival_gain_lucky=1,
 enhance_ival_gain_normal=1, enhance_original_max_roll=0, enhance_original_cascade_down_first=1,
 enhance_ival_cap=100;
int attempts, debits, published, input_retired, output_retired, reads;
bool reject_debit, pouch_mode;
object output, source_item, material_item;
#define GET_C_LUK(ch) 0
#define GET_LEVEL(ch) ((ch)->player.level)
#define GET_MONEY(ch) ((ch)->money)
#define GET_NAME(ch) "fixture"
#define OBJ_VNUM(obj) 1
#define IS_SET(flags, bit) ((flags)&(bit))
#define REMOVE_BIT(flags, bit) ((flags)&=~(bit))
#define SET_BIT(flags, bit) ((flags)|=(bit))
int checked_snprintf(char* buf,size_t size,const char* format,...) {
 va_list args; va_start(args,format); int result=vsnprintf(buf,size,format,args); va_end(args); return result;
}
bool is_enhance_banned(P_obj) { return false; }
bool chaos_material_pouch_is_active(P_obj o) { return pouch_mode && o==&material_item; }
P_obj chaos_material_pouch_find(P_char) { return &material_item; }
int itemvalue(P_obj o) { return o->value; }
int number(int,int) { return INT_MAX; }
int enhance_hash(int) { return 0; }
int enhance_maximum_item_value(int) { return 150; }
P_obj read_object(int,int) { ++reads; return &output; }
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
int main() {
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
}
'''
with tempfile.TemporaryDirectory(prefix='duris-ordinary-enhance-payment-') as temporary:
    cpp=Path(temporary)/'payment.cpp'
    binary=Path(temporary)/'payment'
    cpp.write_text(HARNESS.replace('@FUNCTION@',extract_function('enhance.c','void enhance(')))
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                    '-fno-pie','-no-pie',str(cpp),'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True,timeout=30)
print('Ordinary enhancement: debit refusal preserves inputs, releases output, and accepts valid free/paid quotes')

