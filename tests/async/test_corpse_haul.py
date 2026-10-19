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
#define CURRENCY_DENOMINATION_COUNT 4
#define ITEM_CORPSE 24
#define ITEM_MONEY 1
#define PC_CORPSE 1
#define CORPSE_FLAGS 1
#define CORPSE_PID 2
#define CORPSE_SAVEID 3
#define AVATAR 60
#define LOG_FILE 0
#define LOG_CORPSE 1
#define PULSE_VIOLENCE 1
#define GET_NAME(ch) "tester"
#define LOWEST_MAT_VNUM 1000
#define HIGHEST_MAT_VNUM 2000
#define ITEM2_ACCOUNT_BOUND 1
#define ITEM2_NOLOOT 2
#define GET_PID(ch) (assert(!(ch)->npc && (ch)->only.pc), (ch)->only.pc->pid)
#define IS_ALIVE(ch) ((ch)->alive)
#define IS_AWAKE(ch) (!(ch)->sleeping)
#define IS_IMMOBILE(ch) ((ch)->sleeping)
#define GET_ITEM_TYPE(obj) ((obj)->type)
#define IS_PC(ch) (!(ch)->npc)
#define IS_SET(a,b) ((a)&(b))
#define OBJ_ROOM(o) ((o)->location == 1)
#define OBJ_INSIDE(o) ((o)->location == 2)
#define OBJ_CARRIED_BY(o,ch) ((o)->location == 3 && (o)->carrier == (ch))
#define OBJ_WORN_BY(o,ch) false
#define OBJ_VNUM(obj) 0
#define CAN_SEE_OBJ(ch,obj) ((obj)->visible)
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
struct pc_data { int pid=42; };
struct char_data {
 struct { pc_data *pc=new pc_data; } only;
 uint64_t runtime_id=1; bool npc=false, alive=true, sleeping=false;
 void *desc=this; int in_room=1, count_limit=3, weight_limit=10;
 ~char_data() { delete only.pc; }
};
using P_char = char_data *;
struct obj_data {
 uint64_t obj_uid=0; int type=0, location=0; int value[8]={};
 struct { P_obj_unused_placeholder; } unused;
 const char *short_description="a dagger", *name="dagger", *action_description="Source";
 int weight=1, condition=1, flags=0; bool visible=true;
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
static P_char live_actor=nullptr;
static uint64_t trapped_uid=0;
static int trap_effect=0, trap_calls=0, checked_after_trap=0, corpse_writes=0;
static P_char find_character_by_runtime_id(uint64_t id) {
 return live_actor && live_actor->runtime_id==id ? live_actor : nullptr;
}
static void extract_actor(P_char ch) {
 delete ch->only.pc; ch->only.pc=nullptr; ch->desc=nullptr;
 ch->in_room=-1; ch->alive=false; live_actor=nullptr;
}
static bool die_on_delivery=false, throw_on_delivery=false, durable_items=true;
static bool item_get_ack_publication=false, item_get_deferred=false, item_get_rejected=false;
static P_obj find_live_item_uid(uint64_t uid) { auto i=objects.find(uid); return i==objects.end()?nullptr:i->second; }
static void send_to_char(const char *s,P_char) { output+=s; }
static void act(const char *s,int,P_char ch,P_obj,void *,int target) {
 if(target==TO_ROOM) rooms[ch->in_room]+=s; else output+=s;
}
static void obj_from_obj(P_obj o) { o->location=0; }
enum class obj_to_char_result { placed, deferred, rejected, destroyed };
static obj_to_char_result obj_to_char_checked(P_obj o, P_char ch) {
 if(fail_delivery) { objects.erase(o->obj_uid); return obj_to_char_result::destroyed; }
 o->location=3; o->carrier=ch;
 if(die_on_delivery) extract_actor(ch);
 if(throw_on_delivery) throw std::bad_alloc();
 return obj_to_char_result::placed;
}
static void obj_to_char(P_obj o, P_char ch) {
 (void)obj_to_char_checked(o, ch);
}
static bool item_owner_identity_equal(item_owner_identity a,item_owner_identity b) { return a.type==b.type && a.id==b.id; }
static bool item_owner_identity_valid(item_owner_identity a) { return a.id != 0; }
static bool item_ownership_runtime_lookup(uint64_t,item_ownership_runtime_entry *r) { r->owner={}; return owned; }
static bool item_movement_transaction_player_busy(P_char) { return false; }
static bool corpse_lifecycle_transaction_busy(uint32_t,uint32_t) { return false; }
static bool corpse_lifecycle_transaction_note_item_transfer(uint32_t,uint32_t,int) { return true; }
static void persistence_alert(int,const char *,const char *,const char *,const char *,const char *,...) { ++alerts; }
static int loot_audits=0, loot_waits=0;
static std::string loot_audit;
static void logit(int type,const char *format,...) {
 if(type!=LOG_CORPSE) return;
 char buffer[256]; va_list args; va_start(args,format);
 vsnprintf(buffer,sizeof(buffer),format,args); va_end(args);
 ++loot_audits; loot_audit=buffer;
}
static void CharWait(P_char actor,int) { assert(actor->alive); ++loot_waits; }
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
static bool item_command_object_is_takeable(P_char,P_obj o) { return o->weight>=0; }
static bool checkgetput(P_char ch,P_obj obj) {
 if(trap_calls) ++checked_after_trap;
 if(obj->obj_uid!=trapped_uid) return false;
 ++trap_calls;
 if(trap_effect==1) extract_actor(ch);
 if(trap_effect==2) ch->in_room=2;
 if(trap_effect==3) ch->sleeping=true;
 if(trap_effect==4) objects.erase(obj->loc.inside->obj_uid);
 if(trap_effect==5) obj->loc.inside->loc.room=2;
 if(trap_effect==6) {
  P_obj removed=obj->next_content;
  obj->next_content=nullptr; objects.erase(removed->obj_uid); delete removed;
 }
 if(trap_effect==7) { obj->next_content->location=1; obj->next_content->loc.room=2; }
 return true;
}
static bool item_command_uses_durable_ownership(P_obj o) { return durable_items && o->type!=ITEM_MONEY; }
static int64_t total_carried_weight(P_char) { return 0; }
static bool item_get_source_owner(P_char,P_obj,P_obj,item_owner_identity *o) { *o={}; return true; }
static void checked_snprintf(char *b,size_t n,const char *f,...) { va_list a; va_start(a,f); vsnprintf(b,n,f,a); va_end(a); }
static int scrap_attempts=0;
static void MakeScrap(P_char,P_obj) { ++scrap_attempts; }
static void mark_player_dirty_components(int,int) {}
static void writeCorpse(P_obj corpse) { assert(corpse); ++corpse_writes; }
static bool publish_coin_pile(const coin_transfer_endpoint &,const item_transfer_result &,uint64_t) { return pile_ok; }
static const char *coins_to_string(int p,int g,int s,int c,const char *) {
 static char b[100]; snprintf(b,sizeof(b),"%dp %dg %ds %dc",p,g,s,c); return b;
}
'''.replace(' struct { P_obj_unused_placeholder; } unused;\n','')

finalizers = r'''
static bool coin_options_seen=false;
static coin_get_submission_options last_coin_options = {};
static void do_get_finalize_container_success(P_char ch,P_char,P_obj container,P_obj object,
 int &total,bool &found,bool,const char *,const coin_get_submission_options *options=nullptr) {
 item_get_deferred=false; item_get_rejected=false;
 if(object->type==ITEM_MONEY) {
  ++coin_attempts; coin_options_seen=options!=nullptr;
  if(options) last_coin_options=*options;
  item_get_deferred=admitted; item_get_rejected=!admitted; return;
 }
 publish_container_get(ch,object,container,TRUE,false); ++total; found=true;
}
static void do_get_finalize_room_item(P_char ch,P_obj o,bool &found,int &total) {
 obj_to_char(o,ch); ++total; found=true;
}
'''

driver = r'''
static void reset() {
 loot_audits=loot_waits=0; loot_audit.clear();
 trapped_uid=0; trap_effect=trap_calls=checked_after_trap=corpse_writes=0; live_actor=nullptr;
 die_on_delivery=throw_on_delivery=false; durable_items=true;
 bulk_gets.clear(); objects.clear(); rooms.clear(); output.clear();
 submissions=alerts=coin_attempts=scrap_attempts=0; coin_options_seen=false; last_coin_options={};
 admitted=owned=pile_ok=true; fail_delivery=false;
 held=nullptr; held_context.clear();
}
static void acknowledge(P_char actor,bool ok=true) {
 auto cb=held; auto context=held_context; assert(cb);
 held=nullptr; cb(actor,ok,{},0,context.data(),context.size());
}
static void setup(P_char ch,P_obj corpse,P_obj dagger,P_obj coins) {
 live_actor=ch; ch->in_room=1; corpse->obj_uid=50; corpse->type=ITEM_CORPSE;
 corpse->location=1; corpse->loc.room=1; corpse->short_description="the corpse of a frost giant";
 corpse->value[CORPSE_FLAGS]=PC_CORPSE; corpse->contains=dagger; objects[50]=corpse;
 dagger->obj_uid=51; dagger->location=2; dagger->loc.inside=corpse; dagger->next_content=coins;
 objects[51]=dagger;
 if(coins) { coins->obj_uid=52; coins->type=ITEM_MONEY; coins->value[2]=4;
  coins->location=2; coins->loc.inside=corpse; objects[52]=coins; }
}
int main() {
 // Real extraction releases PC data but retains the outer allocation temporarily.
 // Queue ordinary loot before a lethal selection trap; later siblings must not run.
 for(bool floor : {false,true}) for(bool filtered : {false,true})
 for(int count : {1,3}) for(int position=0; position<count; ++position)
 for(int effect : {0,1,2,3,4,5}) {
  if(floor && effect>=4) continue;
  reset(); durable_items=false; char_data actor; obj_data corpse,items[3];
  setup(&actor,&corpse,&items[0],nullptr);
  for(int i=0; i<count; ++i) {
   items[i].obj_uid=51+i; items[i].location=floor?1:2;
   items[i].loc.room=1; items[i].loc.inside=&corpse;
   items[i].next_content=i+1<count?&items[i+1]:nullptr;
   objects[51+i]=&items[i];
  }
  world[1].contents=&items[0];
  trapped_uid=51+position; trap_effect=effect;
  start_bulk_get(&actor,floor?nullptr:&corpse,filtered?"dagger":nullptr,true);
  assert(trap_calls==1 && checked_after_trap==(effect?0:count-position-1));
  assert(corpse_writes==((floor || effect==4)?0:1));
  for(int i=0; i<count; ++i)
   assert(OBJ_CARRIED_BY(&items[i],&actor)==(!effect && i!=position));
  if(effect) assert(output.empty());
  assert(bulk_gets.empty() && !item_get_ack_publication);
 }
 // A trap can invalidate the saved sibling without invalidating its actor.
 for(int effect : {6,7}) {
  reset(); durable_items=false; char_data actor; obj_data corpse,dagger,trapped;
  setup(&actor,&corpse,&dagger,nullptr);
  trapped.obj_uid=53; trapped.location=2; trapped.loc.inside=&corpse;
  P_obj sibling=new obj_data; sibling->obj_uid=54;
  sibling->location=2; sibling->loc.inside=&corpse;
  dagger.next_content=&trapped; trapped.next_content=sibling;
  objects[53]=&trapped; objects[54]=sibling;
  trapped_uid=53; trap_effect=effect;
  start_bulk_get(&actor,&corpse,nullptr,true);
  assert(trap_calls==1 && checked_after_trap==0 && OBJ_CARRIED_BY(&dagger,&actor));
  assert(bulk_gets.empty() && !item_get_ack_publication);
  if(effect==7) { assert(sibling->location==1 && sibling->loc.room==2); delete sibling; }
 }
 // Cancellation and allocation failure after insertion clear publication and batch state.
 for(bool allocation_failure : {false,true}) {
  reset(); durable_items=false; char_data actor; obj_data corpse,dagger,coins;
  setup(&actor,&corpse,&dagger,&coins);
  die_on_delivery=!allocation_failure; throw_on_delivery=allocation_failure;
  start_bulk_get(&actor,&corpse,nullptr,true);
  assert(bulk_gets.empty() && !item_get_ack_publication && coin_attempts==0);
  assert(output.find("You finish")==std::string::npos);
  // A fresh runtime identity for the same PID can start another operation.
  char_data relogged; relogged.runtime_id=2; obj_data fresh,loot;
  die_on_delivery=throw_on_delivery=false;
  setup(&relogged,&fresh,&loot,nullptr);
  start_bulk_get(&relogged,&fresh,nullptr,true);
  assert(OBJ_CARRIED_BY(&loot,&relogged) && bulk_gets.empty());
 }


  for(bool player_corpse : {false,true}) {
   reset(); char_data actor; obj_data corpse,dagger,coins; setup(&actor,&corpse,&dagger,&coins);
   start_bulk_get(&actor,&corpse,nullptr,player_corpse);
  assert(submissions==1 && output.find("You begin pulling") == 0);
  assert(output.find("Haul:")==std::string::npos && !rooms[1].empty());
   const auto original=output; start_bulk_get(&actor,&corpse,nullptr,player_corpse);
  assert(submissions==1 && output.find("already moving")!=std::string::npos);
  output=original;
  // Held durable transfer commits after departure. A corpse haul keeps the
  // selected coin boundary and resumes; the ordinary-container case below stops.
   actor.in_room=2; corpse.short_description="a replacement description";
   acknowledge(&actor);
   assert(OBJ_CARRIED_BY(&dagger,&actor) && alerts==0);
   assert(coin_attempts==1 && coin_options_seen);
   assert(last_coin_options.allow_source_move && last_coin_options.has_amount_limit);
   assert(last_coin_options.amount_limit[2]==4);
   coin_pickup_context context={50,42,TRUE,true};
   coin_transfer_payload payload; payload.source.before={0,0,4,0};
   assert(coin_get_completion(&actor,true,payload,{},0,
       (const uint8_t*)&context,sizeof(context)));
   assert(output.find("You finish sorting your haul from the corpse of a frost giant.")!=std::string::npos);
   assert(output.find("Haul:\r\n  a dagger\r\n")!=std::string::npos);
   assert(output.find("  0p 4g 0s 0c")!=std::string::npos);
   assert(output.find("Some contents were not acquired.")==std::string::npos);
   assert(rooms[2].empty() && bulk_gets.empty());
   auto terminal=output; finish_bulk_get(&actor,GET_PID(&actor)); assert(output==terminal);
  }
  // A moved ordinary container never receives the corpse continuation
  // exception, even though its selected coin admission is otherwise valid.
  {
   reset(); char_data actor; obj_data container,dagger,coins; setup(&actor,&container,&dagger,&coins);
   container.type=0; start_bulk_get(&actor,&container,nullptr,false); actor.in_room=2;
   acknowledge(&actor); assert(coin_attempts==0 && OBJ_CARRIED_BY(&dagger,&actor));
   assert(bulk_gets.empty());
  }
  // A stable corpse exception applies only to selected coins. A non-money
  // synchronous action such as scrap still requires the actor at the source.
  {
   reset(); char_data actor; obj_data corpse,scrap,coins,durable; setup(&actor,&corpse,&scrap,&coins);
   scrap.condition=0; durable.obj_uid=53; durable.location=2; durable.loc.inside=&corpse;
   durable.next_content=&scrap; corpse.contains=&durable; objects[53]=&durable;
   start_bulk_get(&actor,&corpse,nullptr,false); actor.in_room=2; acknowledge(&actor);
   assert(scrap_attempts==0 && coin_attempts==1);
   coin_pickup_context context={50,42,TRUE,true}; coin_transfer_payload payload;
   payload.source.before={0,0,4,0};
   assert(coin_get_completion(&actor,true,payload,{},0,
       (const uint8_t*)&context,sizeof(context)));
   assert(bulk_gets.empty());
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
 // Several rejected roots still produce one count-limit notice. The scan
 // must reach a later coin pile for either NPC or player corpses.
 for(bool player_corpse : {false,true}) {
  reset(); char_data full_actor; full_actor.count_limit=0;
  obj_data full_corpse, first_item, second_item, later_coins;
  setup(&full_actor,&full_corpse,&first_item,&later_coins);
  second_item.obj_uid=53; second_item.location=2; second_item.loc.inside=&full_corpse;
  first_item.next_content=&second_item; second_item.next_content=&later_coins;
  objects[53]=&second_item;
  start_bulk_get(&full_actor,&full_corpse,nullptr,player_corpse);
  assert(coin_attempts==1 && output.find("carry any more")==std::string::npos);
  coin_pickup_context full_context={50,42,TRUE,true};
  coin_transfer_payload full_payload; full_payload.source.before={0,0,4,0};
  assert(coin_get_completion(&full_actor,true,full_payload,{},0,
      (const uint8_t*)&full_context,sizeof(full_context)));
  assert(output.find("  0p 4g 0s 0c")!=std::string::npos);
  assert(output.find("You can't carry any more.\r\n")!=std::string::npos);
  assert(output.find("You can't carry any more.\r\n") ==
         output.rfind("You can't carry any more.\r\n"));
  assert(OBJ_INSIDE(&first_item) && OBJ_INSIDE(&second_item));
  assert(bulk_gets.empty());
 }
 // A bounded malformed sibling cycle does not multiply the typed notice;
 // neither a hidden direct child nor a nested descendant is disclosed.
 reset(); char_data cycle_actor; cycle_actor.count_limit=0;
 obj_data cycle_corpse, visible_root, hidden_root, nested_secret;
 setup(&cycle_actor,&cycle_corpse,&visible_root,nullptr);
 hidden_root.obj_uid=53; hidden_root.location=2; hidden_root.loc.inside=&cycle_corpse;
 hidden_root.visible=false; hidden_root.short_description="a hidden gem";
 nested_secret.short_description="a nested pearl";
 visible_root.contains=&nested_secret;
 visible_root.next_content=&hidden_root; hidden_root.next_content=&visible_root;
 objects[53]=&hidden_root;
 start_bulk_get(&cycle_actor,&cycle_corpse,nullptr,false);
 assert(output=="You can't carry any more.\r\n" && bulk_gets.empty());
 assert(OBJ_INSIDE(&visible_root) && OBJ_INSIDE(&hidden_root));
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
  assert(loot_audits==0 && loot_waits==0);
  actor.in_room=2;
  coin_pickup_context context={50,42,TRUE,true};
  coin_transfer_payload payload; payload.source.before={0,0,450,0}; payload.source.after={0,0,50,0};
  assert(coin_get_completion(&actor,true,payload,{},0,(const uint8_t*)&context,sizeof(context)));
  assert(loot_audits==1 && loot_waits==1);
  assert(loot_audit.find("0p, 400g, 0s, 0c")!=std::string::npos);
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
 assert(loot_audits==1 && loot_waits==1);
 assert(coin_attempts==2 && output.find("Haul:")==std::string::npos);
 assert(coin_get_completion(&actor,false,{}, {},0,(const uint8_t*)&context,sizeof(context)));
 assert(loot_audits==1 && loot_waits==1);
 assert(output.find("  a dagger")!=std::string::npos && output.find("  0p 7g 0s 0c")!=std::string::npos);
 assert(output.find("sorting")<output.find("did not commit") && bulk_gets.empty());
 // The helper is PC-only. The strict PID macro above aborts for a real NPC,
 // so this also protects every container/coin/artifact publication caller.
 reset(); char_data scavenger; scavenger.npc=true; obj_data npc_bag,npc_loot;
 setup(&scavenger,&npc_bag,&npc_loot,nullptr);
 assert(corpse_bulk_get(nullptr,npc_bag.obj_uid)==nullptr);
 assert(corpse_bulk_get(&scavenger,npc_bag.obj_uid)==nullptr);
 item_get_ack_publication=true;
 publish_container_get(&scavenger,&npc_loot,&npc_bag,TRUE,false);
 item_get_ack_publication=false;
 assert(OBJ_CARRIED_BY(&npc_loot,&scavenger) && bulk_gets.empty());
 puts("corpse haul: held transfer/adoption/coins, movement, rejection, stale source, disconnect and strict NPC publication passed");
}
'''

parts = [prelude, take('struct synchronous_get_item')+';', take('struct bulk_get_state')+';',
         take('struct coin_get_submission_options')+';',
         take('struct bulk_movement_context')+';',
         'static std::unordered_map<uint32_t,bulk_get_state> bulk_gets;',
         take('static bulk_get_state *corpse_bulk_get('),
         take('static void announce_corpse_bulk_get('), take('static bool get_can_continue('), take('static void publish_container_get('), finalizers]
for name in ['static bool bulk_get_source_matches(', 'static bool bulk_get_source_for_roots(',
             'static bool bulk_get_source_available(',
             'static bool bulk_get_corpse_source_available(',
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
