#!/usr/bin/env python3
"""ASan/UBSan: real pickup finalizers after trap extraction and rejected delivery."""
import os
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, SRC, HARNESS_STUBS, extract_function

source = (SRC / 'actobj.c').read_text()
def take(signature):
    return extract_function('actobj.c', signature)

prelude = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "net/comm.h"
#include "net/gmcp.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "persistence/persistence_checkpoint.h"
#include "redis/redis_floor_runtime.h"
#include "economy/currency_transaction.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdarg>
#include <string>
#include <unordered_map>
#include <vector>
#define USE_SPACE 0
#define CORPSE_FLAGS 1
#define IN_WELL_ROOM(ch) false
#define TRAP_EFF_OBJECT 2
#define GETDBG_LOG(...) debug(__VA_ARGS__)
P_obj object_list=nullptr;
P_char character_list=nullptr;
static room_data rooms[3]{};
P_room world=rooms;
static index_data indexes[2]{};
P_index obj_index=indexes;
P_index mob_index=indexes;
int top_of_objt=2;
static int trap_effect=1, trap_calls=0, pickups=0, reports=0, waits=0, corpse_writes=0;
static bool item_get_ack_publication=false, item_get_rejected=false;
static bool crumble=false, refuse_coins=false;
static std::string output, audit;
void send_to_char(const char *text, P_char ch) {
 assert(IS_ALIVE(ch) && ch->only.pc); output+=text;
}
void act(const char *text,int,P_char ch,P_obj,void *,int) {
 assert(IS_ALIVE(ch) && ch->only.pc); output+=text;
}
void debug(const char *format,...) {
 char buf[8192]; va_list ap; va_start(ap,format); vsnprintf(buf,sizeof(buf),format,ap); va_end(ap);
}
void logit(const char *,const char *format,...) {
 char buf[8192]; va_list ap; va_start(ap,format); vsnprintf(buf,sizeof(buf),format,ap); va_end(ap);
 ++reports; audit+=buf;
}
bool account_bound_reward_owner(P_char,P_obj) { return false; }
static bool is_in_locker(P_char, P_obj) { return false; }
int total_carried_weight(P_char) { return 0; }
int number(int low,int) { return low; }
int STAT_INDEX(int) { return 0; }
int GET_CHAR_SKILL_P(P_char,int) { return 0; }
int BOUNDED(int low,int value,int high) { return std::clamp(value,low,high); }
int checked_snprintf_at(const char *,int,char *buf,size_t size,const char *format,...) {
 va_list ap; va_start(ap,format); int n=vsnprintf(buf,size,format,ap); va_end(ap); return n;
}
int real_mobile(int) { return -1; }
void CharWait(P_char ch,int) { assert(IS_ALIVE(ch)); ++waits; }
void MakeScrap(P_char,P_obj) {}
void writeCorpse(P_obj corpse) { assert(corpse); ++corpse_writes; }
void redis_remove_floor_drop(unsigned long) {}
void mark_player_dirty_components(int,player_component_mask_t) {}
void gmcp_char_vitals(P_char) {}
int char_light(P_char ch) { assert(IS_ALIVE(ch)); return 0; }
int room_light(int room,int) { assert(room>=0); return 0; }
void sql_update_bind_data(int,int *,int *) {}
void artifact_feed_to_min_sql(P_obj,int) {}
void obj_from_obj(P_obj obj) { obj->loc_p=LOC_NOWHERE; }
void obj_from_room(P_obj obj) { obj->loc_p=LOC_NOWHERE; }
void obj_to_char(P_obj obj,P_char ch) {
 assert(IS_ALIVE(ch)); ++pickups;
 if(crumble) { object_list=obj->next; delete obj; return; }
 obj->loc_p=LOC_CARRIED; obj->loc.carrying=ch;
}
void extract_obj(P_obj obj,int) { object_list=obj->next; delete obj; }
void add_coins(P_obj,int,int,int,int) {}
char *coins_to_string(int,int,int,int,const char *) { static char coins[]="coins"; return coins; }
static bool credit_coins(P_char,int64_t) { return !refuse_coins; }
void trapdamage(P_char ch,P_obj obj) {
 ++trap_calls; --obj->trap_charge;
 if(trap_effect==1) {
  delete ch->only.pc; ch->only.pc=nullptr; ch->desc=nullptr;
  ch->in_room=NOWHERE; ch->specials.position=STAT_DEAD; character_list=nullptr;
 } else if(trap_effect==2) ch->in_room=2;
 else if(trap_effect==3) ch->specials.position=STAT_SLEEPING;
}
'''

parts=[prelude,
       extract_function('character_identity.c','P_char find_character_by_runtime_id('),
       take('P_obj find_live_item_uid('), take('static bool get_can_continue('),
       take('struct synchronous_get_item')+';',take('struct bulk_get_state')+';',
       take('struct coin_get_submission_options')+';',
       'static std::unordered_map<uint32_t,bulk_get_state> bulk_gets;',
       take('static bulk_get_state *corpse_bulk_get('),
       take('static void publish_container_get('),
       take('static void report_coin_get_rejection('),
       extract_function('trap.c','bool checkgetput(')]
start=source.index('static bool take_coins(',source.index('// Take the selected coins'))
parts.append(source[start:source.index('P_obj find_live_item_uid(',start)])
parts += [take('void get('), take('static bool do_get_commit_pickup_core('),
          take('static bool do_get_finalize_pickup_core('),
          take('static bool do_get_finalize_container_item(')]
# Skip the forward declaration when extracting the artifact logger.
start=source.index('static void do_get_log_container_artifact_pickup(',source.index('static void do_get_log_room_artifact_pickup('))
end=source.index('\nstatic P_obj do_get_obj_in_equipment_vis(',start)
parts += [source[start:end],'static void\n'+take('do_get_finalize_container_success('),
          take('static void do_get_log_room_artifact_pickup('),
          take('static void do_get_finalize_room_item(')]
parts.append(r'''
int main() {
 // Modes: surviving/lethal/teleport/sleep traps, delivery, crumble, whole/partial/
 // capped coins, bound/no-loot/hitched refusal, and wallet refusal.
 for(bool npc : {false,true}) for(bool container : {false,true}) for(int mode=0; mode<13; ++mode) {
  if(npc && mode!=4) continue;
  if((mode==7 || mode==8) && !container) continue;
  trap_effect=mode; crumble=mode==5; refuse_coins=mode==12;
  trap_calls=pickups=reports=waits=corpse_writes=0; output.clear(); audit.clear();
  pc_only_data *pc=new pc_only_data{}; char_data actor{}; descriptor_data descriptor{};
  actor.only.pc=pc; actor.runtime_id=1; actor.in_room=1; actor.desc=&descriptor;
  actor.specials.position=POS_STANDING|STAT_NORMAL; character_list=&actor;
  pc->pid=42; actor.player.name=const_cast<char *>("Looter");
  actor.player.level=1;
  npc_only_data npc_data{};
  if(npc) { actor.specials.act=ACT_ISNPC; actor.only.npc=&npc_data; }
  obj_data corpse{}; corpse.type=ITEM_CORPSE; corpse.obj_uid=50;
  corpse.value[CORPSE_FLAGS]=PC_CORPSE;
  corpse.loc_p=LOC_ROOM; corpse.loc.room=1;
  corpse.name=const_cast<char *>("corpse"); corpse.action_description=const_cast<char *>("Source");
  corpse.short_description=const_cast<char *>("the corpse of Source");
  P_obj item=new obj_data{}; item->obj_uid=51; item->condition=1;
  item->R_num=0; item->name=const_cast<char *>("stone");
  item->short_description=const_cast<char *>("an ioun stone");
  item->wear_flags=ITEM_TAKE|ITEM_WEAR_IOUN;
  item->loc_p=container?LOC_INSIDE:LOC_ROOM;
  if(container) item->loc.inside=&corpse; else item->loc.room=1;
  item->trap_eff=TRAP_EFF_OBJECT; item->trap_charge=mode<4?1:0;
  item->next=&corpse; object_list=item;
  coin_get_submission_options options{};
  if((mode>=6 && mode<=8) || mode==12) {
   item->type=ITEM_MONEY;
   item->value[0]=4; item->value[1]=3; item->value[2]=5; item->value[3]=1;
   options.has_amount_limit=true; options.amount_limit={mode==8?9:1,2,3,0};
  }
  if(mode==9) item->extra2_flags=ITEM2_ACCOUNT_BOUND;
  if(mode==10) item->extra2_flags=ITEM2_NOLOOT;
  if(mode==11) item->hitched_to=&actor;
  bool found=false; int total=0;
  if(container) do_get_finalize_container_success(&actor,&actor,&corpse,item,total,found,true,"post",(mode==7 || mode==8)?&options:nullptr);
  else do_get_finalize_room_item(&actor,item,found,total);
  if(mode<4 || mode>=9) {
   assert(!found && total==0 && pickups==0 && reports==0 && waits==0);
   assert(item_get_rejected && trap_calls==(mode<4?1:0) && item->trap_charge==0);
   if(mode<4) assert(corpse_writes==(container?1:0));
   if(mode==1) assert(!actor.only.pc && !actor.desc && !character_list && actor.in_room==NOWHERE);
  } else if(mode==5) {
   assert(item_get_rejected && !found && total==0 && pickups==1 && reports==0 && waits==0);
  } else {
   assert(!item_get_rejected);
   if(mode<7) assert(found && total==1);
   if(mode==4) assert(pickups==1 && OBJ_CARRIED_BY(item,&actor));
   if(container) {
    assert(reports==1 && waits==1);
    if(mode==6) assert(audit.find("1p, 5g, 3s, 4c")!=std::string::npos);
    if(mode==7 || mode==8) {
     assert(audit.find(mode==8?"0p, 3g, 2s, 4c":"0p, 3g, 2s, 1c")!=std::string::npos);
     assert(item->value[0]==(mode==8?0:3) && item->value[1]==1 && item->value[2]==2 && item->value[3]==1);
    }
   }
  }
  if(mode!=5 && mode!=6) delete item;
  if(npc) delete pc; else delete actor.only.pc;
 }
 puts("pickup finalizers: death, survival, teleport, sleep, ioun, crumble, coins, refusals and NPC pickup passed");
}
''')
build=ROOT/'bin/tests';build.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='get-trap-lifecycle-',dir=build) as directory:
 cpp=Path(directory)/'test.cpp';binary=Path(directory)/'test'
 cpp.write_text('\n'.join(parts))
 subprocess.run(['g++','-std=c++20','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                 '-I/usr/include/mysql','-fno-pie','-no-pie','-I'+str(SRC),str(cpp),str(HARNESS_STUBS),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1'})
