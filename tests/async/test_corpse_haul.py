#!/usr/bin/env python3
"""Run production corpse bulk callbacks with held acknowledgements and player movement.

The item/currency services are controlled boundaries, not a substitute for a
server/SQL journey. Real selection, admission continuation, publication and
terminal reporting functions execute under ASan and UBSan.
"""
from pathlib import Path
import subprocess
import tempfile
from _paths import SRC, extract_function

source = (SRC / 'actobj.c').read_text()
def take(signature):
    return extract_function('actobj.c', signature)

prelude = r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cctype>
#include <cerrno>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#define TRUE 1
#define FALSE 0
#define MAX_STRING_LENGTH 65536
#define TO_CHAR 1
#define TO_ROOM 2
#define USE_SPACE 0
#define ITEM_CORPSE 24
#define ITEM_MONEY 1
#define PC_CORPSE 1
#define CORPSE_FLAGS 1
#define CORPSE_PID 2
#define CORPSE_SAVEID 3
#define AVATAR 60
#define LOG_FILE 0
#define LOWEST_MAT_VNUM 1000
#define HIGHEST_MAT_VNUM 2000
#define ITEM2_ACCOUNT_BOUND 1
#define ITEM2_NOLOOT 2
#define GET_PID(ch) ((ch)->pid)
#define GET_ITEM_TYPE(obj) ((obj)->type)
#define IS_PC(ch) true
#define IS_SET(a,b) ((a)&(b))
#define OBJ_ROOM(o) ((o)->location == 1)
#define OBJ_INSIDE(o) ((o)->location == 2)
#define OBJ_CARRIED_BY(o,ch) ((o)->location == 3 && (o)->carrier == (ch))
#define OBJ_WORN_BY(o,ch) false
#define OBJ_VNUM(obj) 0
#define CAN_SEE_OBJ(ch,obj) true
#define CAN_CARRY_N(ch) ((ch)->count_limit)
#define CAN_CARRY_W(ch) ((ch)->weight_limit)
#define IS_CARRYING_N(ch) 0
#define GET_OBJ_WEIGHT(obj) ((obj)->weight)
#define IS_OBJ_STAT2(obj,flag) ((obj)->flags & (flag))
#define IS_TRUSTED(ch) false
#define OBJS(obj,ch) ((obj)->short_description)
#define PERS(victim,ch,hide) "a horse"
#define J_NAME(ch) "tester"
#define CAP(text) ((text)[0] = toupper((text)[0]))
#define PLAYER_COMPONENT_STATUS 1
#define PLAYER_COMPONENT_INVENTORY 2
struct char_data { int pid=42; int in_room=1; int count_limit=3; int weight_limit=10; };
using P_char = char_data *;
struct obj_data {
 uint64_t obj_uid=0; int type=0, location=0; int value[8]={};
 struct { P_obj_unused_placeholder; } unused;
 const char *short_description="a dagger", *name="dagger";
 int weight=1, condition=1, flags=0;
 P_char carrier=nullptr, hitched_to=nullptr;
 obj_data *next_content=nullptr, *contains=nullptr;
 struct { int room=0; obj_data *inside=nullptr; } loc;
};
using P_obj = obj_data *;
enum class item_owner_type { player, room, locker };
struct item_owner_identity { item_owner_type type=item_owner_type::room; uint64_t id=1, extra=0; };
enum class item_transfer_reason { unknown, locker_withdraw, corpse_loot, player_get };
enum class item_movement_reject { none, owner_mismatch, busy };
struct item_ownership_runtime_entry { item_owner_identity owner; };
struct item_transfer_result { int corpse_revision=0; };
struct coin_transfer_endpoint { std::array<int32_t,4> before={}, after={}; };
struct coin_transfer_payload { coin_transfer_endpoint source, destination; };
struct coin_transfer_result { std::array<item_transfer_result,2> piles={}; };
static struct { P_obj contents=nullptr; } world[8];
static int top_of_objt=50;
static std::unordered_map<uint64_t, P_obj> objects;
static std::unordered_map<int, std::string> rooms;
static std::string output;
static int submissions=0, alerts=0, coin_attempts=0;
static bool admitted=true, owned=true, fail_delivery=false, pile_ok=true;
static bool item_get_ack_publication=false, item_get_deferred=false, item_get_rejected=false;
static P_obj find_live_item_uid(uint64_t uid) { auto i=objects.find(uid); return i==objects.end()?nullptr:i->second; }
static void send_to_char(const char *s,P_char) { output+=s; }
static void act(const char *s,int,P_char ch,P_obj,void *,int target) {
 if(target==TO_ROOM) rooms[ch->in_room]+=s; else output+=s;
}
static void obj_from_obj(P_obj o) { o->location=0; }
static void obj_to_char(P_obj o,P_char ch) {
 if(fail_delivery) { objects.erase(o->obj_uid); return; }
 o->location=3; o->carrier=ch;
}
static bool item_owner_identity_equal(item_owner_identity a,item_owner_identity b) { return a.type==b.type && a.id==b.id; }
static bool item_ownership_runtime_lookup(uint64_t,item_ownership_runtime_entry *r) { r->owner={}; return owned; }
static bool item_movement_transaction_player_busy(P_char) { return false; }
static bool corpse_lifecycle_transaction_busy(uint32_t,uint32_t) { return false; }
static bool corpse_lifecycle_transaction_note_item_transfer(uint32_t,uint32_t,int) { return true; }
static void persistence_alert(int,const char *,const char *,const char *,const char *,const char *,...) { ++alerts; }
static void logit(int,const char *,...) {}
static bool item_movement_reject_is_transient(item_movement_reject r) { return r==item_movement_reject::busy; }
static const char *item_movement_reject_name(item_movement_reject) { return "refused"; }
static void report_batch_movement_reject(P_char ch,item_movement_reject,const char *,const char *s) { send_to_char(s,ch); }
using callback = void(*)(P_char,bool,const item_transfer_result &,unsigned,const uint8_t *,size_t);
static callback held=nullptr;
static std::vector<uint8_t> held_context;
template<typename T> static bool hold(T cb,const void *data,size_t size) {
 if(!admitted) return false;
 ++submissions; held=cb; held_context.assign((const uint8_t*)data,(const uint8_t*)data+size); return true;
}
static bool item_movement_transaction_submit(P_char,P_obj,P_obj,item_owner_identity,item_owner_identity,
 item_transfer_reason,int64_t,callback cb,const void *data,size_t size,P_obj,item_movement_reject *r) {
 *r=item_movement_reject::busy; return hold(cb,data,size);
}
static bool item_movement_transaction_submit_batch(P_char,P_obj *,size_t,P_obj,item_owner_identity,item_owner_identity,
 item_transfer_reason,int64_t,callback cb,const void *data,size_t size,P_obj,item_movement_reject *r) {
 *r=item_movement_reject::busy; return hold(cb,data,size);
}
static bool isname(const char *a,const char *b) { return !strcmp(a,b); }
static bool account_bound_reward_owner(P_char,P_obj) { return false; }
static bool do_get_obj_is_takeable(P_char,P_obj o) { return o->weight>=0; }
static bool checkgetput(P_char,P_obj) { return false; }
static bool uses_generic_item_ownership(P_obj o) { return o->type!=ITEM_MONEY; }
static int64_t total_carried_weight(P_char) { return 0; }
static bool get_item_source_owner(P_char,P_obj,P_obj,item_owner_identity *o) { *o={}; return true; }
static void checked_snprintf(char *b,size_t n,const char *f,...) { va_list a; va_start(a,f); vsnprintf(b,n,f,a); va_end(a); }
static void MakeScrap(P_char,P_obj) {}
static void mark_player_dirty_components(int,int) {}
static void writeCorpse(P_obj) {}
static bool publish_coin_pile(const coin_transfer_endpoint &,const item_transfer_result &,uint64_t) { return pile_ok; }
static const char *coins_to_string(int p,int g,int s,int c,const char *) {
 static char b[100]; snprintf(b,sizeof(b),"%dp %dg %ds %dc",p,g,s,c); return b;
}
'''.replace(' struct { P_obj_unused_placeholder; } unused;\n','')

finalizers = r'''
static void do_get_finalize_container_success(P_char ch,P_char,P_obj container,P_obj object,
 int &total,bool &found,bool,const char *) {
 item_get_deferred=false; item_get_rejected=false;
 if(object->type==ITEM_MONEY) { ++coin_attempts; item_get_deferred=admitted; item_get_rejected=!admitted; return; }
 publish_container_get(ch,object,container,TRUE,false); ++total; found=true;
}
static void do_get_finalize_room_item(P_char ch,P_obj o,bool &found,int &total) {
 obj_to_char(o,ch); ++total; found=true;
}
'''

driver = r'''
static void reset() {
 bulk_gets.clear(); objects.clear(); rooms.clear(); output.clear();
 submissions=alerts=coin_attempts=0; admitted=owned=pile_ok=true; fail_delivery=false;
 held=nullptr; held_context.clear();
}
static void acknowledge(P_char actor,bool ok=true) {
 auto cb=held; auto context=held_context; assert(cb);
 held=nullptr; cb(actor,ok,{},0,context.data(),context.size());
}
static void setup(P_char ch,P_obj corpse,P_obj dagger,P_obj coins) {
 ch->in_room=1; corpse->obj_uid=50; corpse->type=ITEM_CORPSE;
 corpse->location=1; corpse->loc.room=1; corpse->short_description="the corpse of a frost giant";
 corpse->contains=dagger; objects[50]=corpse;
 dagger->obj_uid=51; dagger->location=2; dagger->loc.inside=corpse; dagger->next_content=coins;
 objects[51]=dagger;
 if(coins) { coins->obj_uid=52; coins->type=ITEM_MONEY; coins->location=2; coins->loc.inside=corpse; objects[52]=coins; }
}
int main() {
 for(bool pc : {false,true}) {
  reset(); char_data actor; obj_data corpse,dagger,coins; setup(&actor,&corpse,&dagger,&coins);
  start_bulk_get(&actor,&corpse,nullptr,pc);
  assert(submissions==1 && output.find("You begin pulling") == 0);
  assert(output.find("Haul:")==std::string::npos && !rooms[1].empty());
  const auto original=output; start_bulk_get(&actor,&corpse,nullptr,pc);
  assert(submissions==1 && output.find("already moving")!=std::string::npos);
  output=original;
  // Held durable transfer commits after departure. Publish accepted equipment,
  // but never start the unsubmitted coin transfer remotely.
  actor.in_room=2; corpse.short_description="a replacement description";
  acknowledge(&actor);
  assert(OBJ_CARRIED_BY(&dagger,&actor) && alerts==0 && coin_attempts==0);
  assert(output.find("You finish sorting your haul from the corpse of a frost giant.")!=std::string::npos);
  assert(output.find("Haul:\r\n  a dagger\r\n")!=std::string::npos);
  assert(output.find("remaining contents")>output.find("a dagger"));
  assert(rooms[2].empty() && bulk_gets.empty());
  auto terminal=output; finish_bulk_get(&actor,actor.pid); assert(output==terminal);
 }
 // Adoption preserves source custody only; leaving before the transfer stops it.
 reset(); char_data actor; obj_data corpse,dagger,coins; setup(&actor,&corpse,&dagger,&coins);
 owned=false; start_bulk_get(&actor,&corpse,nullptr,false); actor.in_room=2;
 owned=true; acknowledge(&actor);
 assert(submissions==1 && OBJ_INSIDE(&dagger) && coin_attempts==0);
 assert(output.find("Nothing acquired.")!=std::string::npos && output.find("no longer available")!=std::string::npos);
 // No accepted work means no misleading start/finish.
 reset(); setup(&actor,&corpse,&dagger,nullptr); admitted=false;
 start_bulk_get(&actor,&corpse,nullptr,false);
 assert(output.find("You begin")==std::string::npos && output.find("sorting")==std::string::npos);
 assert(output.find("busy")!=std::string::npos && bulk_gets.empty());
 reset(); setup(&actor,&corpse,&dagger,nullptr); actor.count_limit=0;
 start_bulk_get(&actor,&corpse,nullptr,false);
 assert(output=="You can't carry any more.\r\n"); actor.count_limit=3;
 // Missing/moved source never falsely reports selected items as delivered.
 for(int missing=0;missing<3;++missing) {
  reset(); setup(&actor,&corpse,&dagger,nullptr); start_bulk_get(&actor,&corpse,nullptr,false);
  if(missing==0) objects.erase(50);
  if(missing==1) corpse.loc.room=3;
  if(missing==2) dagger.loc.inside=nullptr;
  acknowledge(&actor); assert(alerts==1 && !OBJ_CARRIED_BY(&dagger,&actor));
  assert(output.find("Nothing acquired.")!=std::string::npos && output.find("  a dagger")==std::string::npos);
 }
 // Failure is terminal and buffered after the completion, without fake loot.
 reset(); setup(&actor,&corpse,&dagger,nullptr); start_bulk_get(&actor,&corpse,nullptr,false);
 acknowledge(&actor,false); assert(output.find("sorting")<output.find("did not commit"));
 // A disconnected actor releases only transient reporting state.
 reset(); setup(&actor,&corpse,&dagger,nullptr); start_bulk_get(&actor,&corpse,nullptr,false);
 acknowledge(nullptr); assert(bulk_gets.empty());
 // A failed live delivery is never listed in the haul.
 reset(); setup(&actor,&corpse,&dagger,nullptr); start_bulk_get(&actor,&corpse,nullptr,false);
 fail_delivery=true; acknowledge(&actor); assert(output.find("  a dagger")==std::string::npos);
 // Coin-only and mixed operations hold output across acknowledgements. The
 // actual committed denominations are copied before shared formatter reuse.
 for(bool mixed : {false,true}) {
  reset(); setup(&actor,&corpse,&dagger,&coins);
  if(!mixed) corpse.contains=&coins;
  start_bulk_get(&actor,&corpse,nullptr,false);
  if(mixed) acknowledge(&actor);
  assert(coin_attempts==1 && output.find("Haul:")==std::string::npos);
  actor.in_room=2;
  coin_pickup_context context={50,42,TRUE,true};
  coin_transfer_payload payload; payload.source.before={0,0,450,0}; payload.source.after={0,0,50,0};
  assert(coin_get_completion(&actor,true,payload,{},0,(const uint8_t*)&context,sizeof(context)));
  assert(output.find("  0p 400g 0s 0c")!=std::string::npos);
  assert(output.find("couldn't carry")>output.find("Haul:"));
  assert(rooms[2].empty() && bulk_gets.empty());
 }
 // A later coin failure keeps the already delivered equipment and first pile,
 // reports completion once, and never lists the rejected second pile.
 reset(); setup(&actor,&corpse,&dagger,&coins); obj_data later;
 later.obj_uid=53; later.type=ITEM_MONEY; later.location=2; later.loc.inside=&corpse;
 coins.next_content=&later; objects[53]=&later;
 start_bulk_get(&actor,&corpse,nullptr,false); acknowledge(&actor);
 coin_pickup_context context={50,42,TRUE,true}; coin_transfer_payload first;
 first.source.before={0,0,7,0};
 assert(coin_get_completion(&actor,true,first,{},0,(const uint8_t*)&context,sizeof(context)));
 assert(coin_attempts==2 && output.find("Haul:")==std::string::npos);
 assert(coin_get_completion(&actor,false,{}, {},0,(const uint8_t*)&context,sizeof(context)));
 assert(output.find("  a dagger")!=std::string::npos && output.find("  0p 7g 0s 0c")!=std::string::npos);
 assert(output.find("sorting")<output.find("did not commit") && bulk_gets.empty());
 puts("corpse haul: held transfer/adoption/coins, movement, rejection, stale source and disconnect passed");
}
'''

parts = [prelude, take('struct synchronous_get_item')+';', take('struct bulk_get_state')+';',
         take('struct bulk_movement_context')+';',
         'static std::unordered_map<uint32_t,bulk_get_state> bulk_gets;',
         take('static bulk_get_state *corpse_bulk_get('),
         take('static void announce_corpse_bulk_get('), take('static void publish_container_get('), finalizers]
for name in ['static bool bulk_get_source_matches(', 'static bool bulk_get_source_available(',
             'static void report_bulk_get(', 'static void finish_bulk_get(', 'static void fail_bulk_get(',
             'static void reject_bulk_get_admission(', 'static P_obj resolve_synchronous_get_item(',
             'static bool finish_bulk_get_after_commit(', 'static void bulk_get_completion(']:
    parts.append(take(name))
parts += ['static void continue_bulk_get(P_char actor,uint32_t actor_pid);',
          take('static void bulk_get_adoption_completion(')]
# The forward declaration precedes the definition; select the latter explicitly.
pos = source.index('/** Adopt missing stock roots')
start = source.index('static void continue_bulk_get(', pos)
end = source.index('/** Snapshot rejection text', start)
parts.append(source[start:end])
for name in ['static void reject_bulk_get_object(', 'static bool select_bulk_get_item(', 'static void start_bulk_get(']:
    parts.append(take(name))
parts += [take('struct coin_pickup_context')+';', take('static bool coin_get_completion('), driver]
with tempfile.TemporaryDirectory(prefix='corpse-haul-') as directory:
    cpp=Path(directory)/'test.cpp'; binary=Path(directory)/'test'
    cpp.write_text('\n'.join(parts))
    subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror','-Wno-unused-parameter',
                    '-Wno-missing-field-initializers','-fsanitize=address,undefined','-g',str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
