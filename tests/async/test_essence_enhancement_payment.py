#!/usr/bin/env python3
"""Exercise production essence enhancement before payment and byte-width mutation."""
from _paths import ROOT, extract_function
import os
from pathlib import Path
import subprocess
import tempfile

HARNESS=r'''
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <initializer_list>
constexpr int MAX_STRING_LENGTH=4096,FALSE=0,TO_CHAR=0,VIRTUAL=0;
constexpr int APPLY_HIT=2,APPLY_HIT_REG=3,APPLY_MOVE_REG=4,ITEM2_ENHANCED=1;
struct character { int64_t money; };
struct object { struct { unsigned char location=0; signed char modifier=0; } affected[3];
 int value=10,vnum=400238,extra2_flags=0;
 const char* name="fixture"; const char* short_description="fixture"; };
using P_char=character*;using P_obj=object*;using sbyte=signed char;
int enhance_mod_max_steps=3,debits,attempts,consumed,descriptions,reads,templates_retired,chosen_loc=1;
bool reject_debit=false;
object prototype;
const char* modenhance_names[5]={nullptr,"test","test","test","test"};
#define GET_MONEY(ch) ((ch)->money)
#define OBJ_VNUM(obj) ((obj)->vnum)
#define SET_BIT(flags,bit) ((flags)|=(bit))
#define IS_ENCRUSTED(obj) false
int itemvalue(P_obj o){return o->value;}
bool is_enhance_banned(P_obj){return false;}
int essence_loc(int){return chosen_loc;}
int SUB_MONEY(P_char ch,int amount,int){
 ++attempts; if(reject_debit || amount<=0 || ch->money<amount)return -1;
 ch->money-=amount;++debits;return 0;
}
void send_to_char(const char*,P_char){}
void act(const char*,int,P_char,P_obj,void*,int){}
void obj_from_char(P_obj){}
void extract_obj(P_obj o){if(o==&prototype)++templates_retired;else ++consumed;}
P_obj read_object(int,int){++reads;return &prototype;}
void describe_encrusted_enhanced(P_obj){}
void set_keywords(P_obj,const char*){++descriptions;}
void set_short_description(P_obj,const char*){++descriptions;}
int checked_snprintf(char* buf,size_t size,const char* format,...){
 va_list args;va_start(args,format);int rc=vsnprintf(buf,size,format,args);va_end(args);return rc;
}
@FUNCTION@
void reset(){debits=attempts=consumed=descriptions=reads=templates_retired=0;enhance_mod_max_steps=3;}
void assert_unchanged(const character& actor,const object& item,int location,int modifier){
 assert(actor.money==200000 && item.affected[2].location==location && item.affected[2].modifier==modifier);
 assert(!item.extra2_flags && !debits && !consumed && !descriptions && !reads && !templates_retired);
}
int main(){
 for(int value:{10,21,31}) for(bool same:{false,true}){
  reset();chosen_loc=1;reject_debit=true;
  character actor{200000};object item,material;material.value=value;
  item.affected[2].location=same ? 1 : 4;item.affected[2].modifier=1;
  modenhance(&actor,&item,&material);
  assert_unchanged(actor,item,same ? 1 : 4,1);assert(attempts==1);
 }
 reject_debit=false;
 for(int loc:{1,APPLY_HIT,APPLY_HIT_REG,APPLY_MOVE_REG}) for(int value:{10,21,31}){
  const int step=loc==1 ? 1 : 3,cost=value<=20 ? 1000 : value<=30 ? 20000 : 100000;
  for(bool same:{false,true}){
   reset();chosen_loc=loc;character actor{200000};object item,material;material.value=value;
   item.affected[2].location=same ? loc : 0;item.affected[2].modifier=same ? step : 100;
   modenhance(&actor,&item,&material);
   assert(actor.money==200000-cost && attempts==1 && debits==1 && consumed==1);
   assert(item.affected[2].location==loc && item.affected[2].modifier==(same ? 2*step : step));
   assert(item.extra2_flags==ITEM2_ENHANCED && descriptions==2 && reads==1 && templates_retired==1);
  }
  reset();chosen_loc=loc;character actor{200000};object item,material;
  item.affected[2].location=loc;item.affected[2].modifier=3*step;
  modenhance(&actor,&item,&material);assert_unchanged(actor,item,loc,3*step);assert(!attempts);
  reset();enhance_mod_max_steps=INT_MAX;
  item.affected[2].modifier=127;
  modenhance(&actor,&item,&material);assert_unchanged(actor,item,loc,127);assert(!attempts);
  reset();enhance_mod_max_steps=INT_MAX;
  item.affected[2].modifier=127-step;
  modenhance(&actor,&item,&material);
  assert(item.affected[2].modifier==127 && attempts==1 && debits==1 && consumed==1);
 }
}
'''
with tempfile.TemporaryDirectory(prefix='duris-essence-payment-') as temporary:
    cpp=Path(temporary)/'essence.cpp';binary=Path(temporary)/'essence'
    cpp.write_text(HARNESS.replace('@FUNCTION@',extract_function('enhance.c','void modenhance(')))
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                    '-fno-pie','-no-pie',str(cpp),'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True,timeout=30)
print('Essence enhancement: rejected payment/capped modifiers preserve inputs; all tiers and exact signed-byte boundary pass')
