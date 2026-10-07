#!/usr/bin/env python3
"""Execute whole native shop customer access and a complete value-command consumer.

Visibility, feedback and service endpoints are controlled. Frozen original
fixtures retain their historical output labels; actual current native bodies
are compiled under both sanitizer profiles, without a database or running game.
"""

from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function, source

PRELUDE_DIRECT = r'''
#include "core/prototypes.h"
#include "net/comm.h"
#include "core/utils.h"
#include "economy/shop.h"
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
static shop_data shop{};
shop_data *shop_index=&shop;
time_info_data time_info{};
static char_data actor{},keeper{};
static bool visible=true;
static int visibility_mutation=0,feedback_mutation=0;
static std::vector<std::string> trace;
bool ac_can_see(P_char observer,P_char customer,bool coord){
 assert(observer==&keeper && customer==&actor && coord==TRUE);
 trace.push_back("see");
 if(visibility_mutation==1)actor.player.level=MAXLVLMORTAL+1;
 if(visibility_mutation==2)actor.player.level=MAXLVLMORTAL;
 if(visibility_mutation==3)actor.specials.act|=PLR_MORTAL;
 return visible;
}
void mobsay(P_char observer,const char *text){assert(observer==&keeper);trace.push_back(std::string("say:")+text);}
void act(const char *text,int hidden,P_char customer,P_obj object,void *victim,int target){
 assert(hidden==FALSE && customer==&actor && !object && !victim);
 trace.push_back(std::string(target==TO_ROOM?"room:":"char:")+text);
 if(feedback_mutation && target==TO_ROOM){
  keeper.player.short_descr=const_cast<char*>("changed keeper");
  actor.player.name=const_cast<char*>("changed actor");
  shop.racist_message=const_cast<char*>("changed message");
  actor.player.race=static_cast<ubyte>(shop.shopkeeper_race);
  actor.player.level=MAXLVLMORTAL+1;
 }
}
static void reset(){
 shop={};actor={};keeper={};time_info={};trace.clear();visible=true;visibility_mutation=feedback_mutation=0;
 actor.player.name=const_cast<char*>("Alice");actor.player.short_descr=const_cast<char*>("an actor");actor.player.race=1;actor.player.level=MAXLVLMORTAL;
 keeper.player.name=const_cast<char*>("Keeper");keeper.player.short_descr=const_cast<char*>("a keeper");keeper.specials.act=ACT_ISNPC;
 shop.open1=8;shop.close1=12;shop.open2=14;shop.close2=18;shop.racist_message=const_cast<char*>("go away");shop.shopkeeper_race=1;time_info.hour=10;
}

'''

DRIVER_DIRECT = r'''

int main(){
 int controls=0;
 const auto check=[&](int result,const std::vector<std::string>& expected){assert(is_ok(&keeper,&actor,0)==result);assert(trace==expected);++controls;};
 const char *early="say:Come back later!",*gap="say:Sorry, we have closed, but come back later.",*closed="say:Sorry, come back tomorrow.",*unseen="say:I don't trade with someone I can't see!";
 for(int hour: {-1,0,7,8,9,12,13,14,15,18,19,23,24}){
  reset();time_info.hour=hour;
  if(hour<8)check(FALSE,{early});else if(hour>12 && hour<14)check(FALSE,{gap});else if(hour>18)check(FALSE,{closed});else check(TRUE,{"see"});
 }
 for(int hour: {7,13,19}){reset();actor.player.level=MAXLVLMORTAL+1;time_info.hour=hour;check(FALSE,{hour==7?early:hour==13?gap:closed});}
 for(int racial: {-1,0,2,INT_MAX}){reset();shop.racist=racial;actor.player.race=2;check(TRUE,{"see"});}
 reset();shop.racist=1;check(TRUE,{"see"});
 reset();shop.racist=1;actor.player.race=2;check(FALSE,{"room:a keeper says to Alice, 'go away'","char:a keeper says to you, 'go away'"});
 reset();shop.racist=1;actor.player.race=2;actor.specials.act=ACT_ISNPC;actor.player.level=MAXLVLMORTAL+1;check(FALSE,{"room:a keeper says to an actor, 'go away'","char:a keeper says to you, 'go away'"});
 reset();shop.racist=1;actor.player.race=2;actor.player.level=MAXLVLMORTAL+1;check(TRUE,{"see"});
 reset();shop.racist=1;actor.player.race=2;actor.player.level=MAXLVLMORTAL+1;actor.specials.act=PLR_MORTAL;check(FALSE,{"room:a keeper says to Alice, 'go away'","char:a keeper says to you, 'go away'"});
 reset();shop.racist=1;actor.player.race=2;feedback_mutation=1;check(FALSE,{"room:a keeper says to Alice, 'go away'","char:changed keeper says to you, 'changed message'"});
 reset();visible=false;check(FALSE,{"see",unseen});
 reset();visible=false;actor.player.level=MAXLVLMORTAL+1;check(TRUE,{"see"});
 reset();visible=false;actor.player.level=MAXLVLMORTAL+1;actor.specials.act=PLR_MORTAL;check(FALSE,{"see",unseen});
 reset();visible=false;actor.player.level=MAXLVLMORTAL+1;actor.specials.act=ACT_ISNPC;check(FALSE,{"see",unseen});
 reset();visible=false;visibility_mutation=1;check(TRUE,{"see"});
 reset();visible=false;actor.player.level=MAXLVLMORTAL+1;visibility_mutation=2;check(FALSE,{"see",unseen});
 reset();visible=false;actor.player.level=MAXLVLMORTAL+1;visibility_mutation=3;check(FALSE,{"see",unseen});
 reset();visible=true;visibility_mutation=3;actor.player.level=MAXLVLMORTAL+1;check(TRUE,{"see"});
 for(int who: {-1,0,1,2,INT_MAX}){reset();shop.with_who=who;check(TRUE,{"see"});}
 reset();shop.open1=12;shop.close1=8;time_info.hour=10;check(FALSE,{early});
 reset();shop.open1=8;shop.close1=8;shop.open2=18;shop.close2=14;time_info.hour=15;check(FALSE,{gap});
 reset();shop.open1=8;shop.close1=8;shop.open2=18;shop.close2=14;time_info.hour=18;check(FALSE,{closed});
 reset();shop.open2=5;shop.close2=20;time_info.hour=13;check(TRUE,{"see"});
 reset();shop.open2=5;shop.close2=20;time_info.hour=7;check(FALSE,{early});
 reset();shop.open2=5;shop.close2=20;time_info.hour=20;check(TRUE,{"see"});
 reset();shop.open2=5;shop.close2=20;time_info.hour=21;check(FALSE,{closed});
 reset();shop.open1=12;shop.close1=8;shop.open2=5;shop.close2=20;time_info.hour=12;check(TRUE,{"see"});
 std::cout<<"original shop customer access controls:"<<controls<<'\n';
}
'''

PRELUDE_COMMAND = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "net/comm.h"
#include "economy/shop.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/shop_trade_runtime.h"
#include "economy/shop_trade_publication.h"
#include "persistence/persistence_mode.h"
#include "sql/sql.h"
#include "classes/salchemist.h"
#include <cassert>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
static index_data item_indices[2]{},mob_indices[1]{};
P_index obj_index=item_indices,mob_index=mob_indices;
static room_data room{};P_room world=&room;
static shop_data shop{};shop_data* shop_index=&shop;int number_of_shops=1;
cha_app_type cha_app[256]{};
static std::unordered_map<uint32_t,int> produced_purchase_sequences;
static persistence_mode mode;
static bool accounted=false,busy=false,found=true,barter=false,refuse=false;
static int roll=0,trophies=0,mutation=0,paid=-1,built=-1,min_calls=0;
static float trophy_mod=.05f,min_pct=.10f;
static P_obj selected=nullptr;static P_char actor=nullptr;
static std::vector<std::string> trace,fixture_messages;
bool economic_gameplay_authority::active(){return accounted;}
bool shop_trade_preparation_owner::production_available() noexcept{return false;}
persistence_mode persistence_mode_get(){return mode;}
bool shop_trade_transaction_player_busy(P_char){trace.push_back("busy");return busy;}

P_obj get_selling_obj(P_char,char*,P_char,int,int,char){trace.push_back("select");return found?selected:nullptr;}
int STAT_INDEX(int value){trace.push_back("stat:"+std::to_string(value));if(mutation==5){shop.buy_percent=.25f;selected->condition=50;}return value;}
bool has_innate(P_char,int id){assert(id==INNATE_BARTER);trace.push_back("innate");return barter;}
int number(int low,int high){assert(low==0&&high==125);trace.push_back("rng");if(mutation==1){selected->cost=120;selected->condition=50;shop.buy_percent=.99f;actor->curr_stats.Cha=0;}return roll;}
int sql_shop_trophy(P_obj object){assert(object==selected);trace.push_back("trophy");if(mutation==2){object->cost=9000;object->condition=1;}return trophies;}
float get_property(const char* key,double fallback){
 if(!strcmp(key,"shops.sellTrophyMod")){assert(fallback==.05);trace.push_back("mod");if(mutation==3)min_pct=.2f;return trophy_mod;}
 assert(!strcmp(key,"shops.sellMinPct")&&fallback==.10);trace.push_back("min");++min_calls;return mutation==4&&min_calls>1?.2f:min_pct;
}
int get_property(const char* key,int fallback){assert(!strcmp(key,"stats.sell.log")&&fallback==500000);return 500000;}
void send_to_char(const char* text,P_char){fixture_messages.emplace_back(text);}

void do_tell(P_char,char* text,int){fixture_messages.emplace_back(text);}
void logit(const char*,const char*,...){}
void wizlog(int,const char*,...){}void statuslog(int,const char*,...){}
[[noreturn]] int panic_corruption_int(const char*,const char*,...){abort();}
int checked_substitute_strings(char* buffer,size_t size,const char*,const char*const*,size_t){if(size)*buffer=0;return 0;}
char* coin_stringv(int value,int){static char text[64];snprintf(text,sizeof(text),"%d copper",value);return text;}
bool isname(const char* name,const char* list){return list&&!strcmp(name,list);}
int fill_word(char*){return false;}

int sql_shop_sell(P_char,P_obj,int sale){trace.push_back("sql:"+std::to_string(sale));paid=sale;return 0;}
void ADD_MONEY(P_char ch,int value,const char*){GET_COPPER(ch)+=value;}
int SUB_MONEY(P_char ch,int value,int){GET_COPPER(ch)-=value;return 0;}
P_obj get_obj_in_list(char*,P_obj){return nullptr;}
void extract_obj(P_obj,int){assert(false);}
void obj_from_char(P_obj object){object->loc_p=LOC_NOWHERE;}
void obj_to_char(P_obj object,P_char ch){object->loc_p=LOC_CARRIED;object->loc.carrying=ch;}
static bool shop_trade_start_accounted(P_char,P_char,P_obj,P_obj,P_obj,uint32_t,shop_trade_action,int64_t){assert(false);return false;}
static bool shop_trade_publish_physical(P_char,const shop_trade_result&,const shop_trade_payload&,uint32_t&){assert(false);return false;}
static void shop_trade_completion(P_char,bool,const shop_trade_result&,unsigned int,const shop_trade_payload&){assert(false);}
shop_trade_payload_build_result shop_trade_runtime_build_payload(P_char,P_char,P_obj,P_obj,P_obj,uint32_t,shop_trade_action,int64_t price,shop_trade_payload* payload){trace.push_back("build:"+std::to_string(price));built=price;*payload={};return shop_trade_payload_build_result::ok;}
bool shop_trade_transaction_submit_with_publication(P_char,const shop_trade_payload&,shop_trade_physical_publication_fn,shop_trade_completion_fn){trace.push_back("submit");return !refuse;}

time_info_data time_info{};
static P_char current_keeper=nullptr;
static bool visible=true;
static int visibility_mutation=0,feedback_mutation=0;
bool ac_can_see(P_char observer,P_char customer,bool coord){
 assert(observer==current_keeper && customer==actor && coord==TRUE);trace.push_back("see");
 if(visibility_mutation==1)actor->player.level=MAXLVLMORTAL+1;
 if(visibility_mutation==2)actor->player.level=MAXLVLMORTAL;
 if(visibility_mutation==3)actor->specials.act|=PLR_MORTAL;
 return visible;
}
void mobsay(P_char observer,const char* text){assert(observer==current_keeper);trace.push_back(std::string("say:")+text);fixture_messages.emplace_back(text);}
void act(const char *text,int hidden,P_char customer,P_obj object,void *victim,int target){
 assert(hidden==FALSE && customer==actor && !object && !victim);
 trace.push_back(std::string(target==TO_ROOM?"room:":"char:")+text);
 if(feedback_mutation && target==TO_ROOM){
  current_keeper->player.short_descr=const_cast<char*>("changed keeper");
  shop.racist_message=const_cast<char*>("changed message");actor->player.race=static_cast<ubyte>(shop.shopkeeper_race);actor->player.level=MAXLVLMORTAL+1;
 }
}

'''

DRIVER_COMMAND = r'''

struct fixture {
 char_data player{},keeper{};pc_only_data pc{};npc_only_data npc{};obj_data item{};
 fixture(){
  shop={};shop.buy_percent=.5f;shop.message_sell=const_cast<char*>("%s %s");shop.missing_cash1=const_cast<char*>("%s cash");
  player.only.pc=&pc;pc.pid=42;player.player.name=const_cast<char*>("Seller");player.curr_stats.Cha=100;GET_COPPER(&player)=1000;
  keeper.specials.act=ACT_ISNPC;keeper.only.npc=&npc;keeper.player.name=const_cast<char*>("Keeper");GET_COPPER(&keeper)=1000;mob_indices[0].virtual_number=100;
  item.R_num=0;item_indices[0].virtual_number=200;item.cost=100;item.condition=100;item.type=ITEM_WEAPON;item.name=const_cast<char*>("sword");item.short_description=const_cast<char*>("a sword");
  item.loc_p=LOC_CARRIED;item.loc.carrying=&player;player.carrying=&item;selected=&item;actor=&player;
  mode=PERSISTENCE_MODE_MARIADB_PRIMARY;accounted=busy=barter=refuse=false;found=true;roll=trophies=mutation=min_calls=0;paid=built=-1;trophy_mod=.05f;min_pct=.10f;
  time_info={};time_info.hour=10;shop.open1=8;shop.close1=12;shop.open2=14;shop.close2=18;shop.shopkeeper_race=0;shop.racist_message=const_cast<char*>("go away");
  player.player.level=MAXLVLMORTAL;player.player.short_descr=const_cast<char*>("an actor");keeper.player.short_descr=const_cast<char*>("a keeper");current_keeper=&keeper;visible=true;visibility_mutation=feedback_mutation=0;
  trace.clear();fixture_messages.clear();produced_purchase_sequences.clear();for(auto& row:cha_app)row.modifier=0;
 }
 void call(bool value,const char* arg="sword"){char text[128];strcpy(text,arg);if(value)shopping_value(text,&player,&keeper,0);else shopping_sell(text,&player,&keeper,0);}
 int quote(bool value) const{if(!value)return paid;assert(!fixture_messages.empty());int price=-1;assert(sscanf(fixture_messages.back().c_str(),"The shopkeeper says 'I'll give you %d copper",&price)==1);return price;}
};


int main(){
 int controls=0;
 const auto refusal=[&](fixture& f,const std::vector<std::string>& expected){f.call(true);assert(trace==expected);assert(paid==-1&&built==-1&&GET_MONEY(&f.player)==1000&&f.item.loc.carrying==&f.player);assert(std::find(trace.begin(),trace.end(),"select")==trace.end());++controls;};
 const auto accepted=[&](fixture& f){f.call(true);assert(f.quote(true)==50);assert(trace==std::vector<std::string>({"see","select","stat:100","innate","trophy"}));assert(paid==-1&&built==-1&&GET_MONEY(&f.player)==1000&&f.item.loc.carrying==&f.player);++controls;};
 const char *early="say:Come back later!",*gap="say:Sorry, we have closed, but come back later.",*closed="say:Sorry, come back tomorrow.",*unseen="say:I don't trade with someone I can't see!";
 for(int hour:{7,8,12,13,14,18,19}){fixture f;time_info.hour=hour;if(hour==7)refusal(f,{early});else if(hour==13)refusal(f,{gap});else if(hour==19)refusal(f,{closed});else accepted(f);}
 {fixture f;shop.racist=1;f.player.player.race=1;refusal(f,{"room:a keeper says to Seller, 'go away'","char:a keeper says to you, 'go away'"});}
 {fixture f;shop.racist=1;f.player.player.race=1;f.player.specials.act=ACT_ISNPC;f.player.player.level=MAXLVLMORTAL+1;refusal(f,{"room:a keeper says to an actor, 'go away'","char:a keeper says to you, 'go away'"});}
 {fixture f;shop.racist=1;f.player.player.race=1;f.player.player.level=MAXLVLMORTAL+1;accepted(f);}
 {fixture f;shop.racist=1;f.player.player.race=1;f.player.player.level=MAXLVLMORTAL+1;f.player.specials.act=PLR_MORTAL;refusal(f,{"room:a keeper says to Seller, 'go away'","char:a keeper says to you, 'go away'"});}
 {fixture f;shop.racist=1;f.player.player.race=1;feedback_mutation=1;refusal(f,{"room:a keeper says to Seller, 'go away'","char:changed keeper says to you, 'changed message'"});}
 {fixture f;visible=false;refusal(f,{"see",unseen});}
 {fixture f;visible=false;f.player.player.level=MAXLVLMORTAL+1;accepted(f);}
 {fixture f;visible=false;visibility_mutation=1;accepted(f);}
 {fixture f;visible=false;f.player.player.level=MAXLVLMORTAL+1;visibility_mutation=2;refusal(f,{"see",unseen});}
 {fixture f;visible=false;f.player.player.level=MAXLVLMORTAL+1;visibility_mutation=3;refusal(f,{"see",unseen});}
 for(int who:{-1,0,1,2}){fixture f;shop.with_who=who;accepted(f);}
 {fixture f;shop.open2=5;shop.close2=20;time_info.hour=13;accepted(f);}
 {fixture f;shop.open2=5;shop.close2=20;time_info.hour=7;refusal(f,{early});}
 {fixture f;shop.open1=12;shop.close1=8;shop.open2=5;shop.close2=20;time_info.hour=12;accepted(f);}
 printf("original real customer/value controls:%d\n",controls);
}
'''

def main():
    shop = source("economy/shop.c").read_text(encoding="utf-8")
    include = '#include "economy/shop_customer_access.h"'
    direct = "\n".join([PRELUDE_DIRECT, include,
        extract_function("economy/shop.c", "int is_ok("), DRIVER_DIRECT])
    quote_include = ""
    quote_provider = ""
    if "struct shop_sale_quote_observations" in shop:
        quote_include = '#include "economy/shop_sale_quote.h"'
        quote_provider = shop[shop.index("struct shop_sale_quote_observations"):
            shop.index("void shopping_sell(")]
    command = "\n".join([PRELUDE_COMMAND, include, quote_include, quote_provider,
        extract_function("cmd/interp.c", "char *one_argument("),
        extract_function("cmd/interp.c", "char *lohrr_chop("),
        extract_function("economy/shop.c", "static bool refuse_unported_shop_mutation("),
        extract_function("economy/shop.c", "int is_ok("),
        extract_function("economy/shop.c", "void shopping_sell("),
        extract_function("economy/shop.c", "void shopping_value("), DRIVER_COMMAND])
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="shop-customer-", dir=build_root) as directory:
        for name, harness in (("direct", direct), ("command", command)):
            cpp = Path(directory) / ("shop_customer_" + name + ".cpp")
            binary = Path(directory) / ("shop_customer_" + name)
            cpp.write_text(harness, encoding="utf-8")
            for flags in ([], ["-Og"]):
                subprocess.run(["g++", *flags, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-ffunction-sections", "-fdata-sections", "-Isrc",
                    "-I/usr/include/libxml2", "-I/usr/include/mysql", str(cpp),
                    "-Wl,--gc-sections", "-o", str(binary)], cwd=ROOT, check=True, timeout=120)
                subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)


if __name__ == "__main__":
    main()
