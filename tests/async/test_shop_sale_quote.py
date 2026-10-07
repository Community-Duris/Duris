#!/usr/bin/env python3
"""Execute complete native sale/value callers with controlled service endpoints."""

from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function, source

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "net/comm.h"
#include "economy/shop.h"
#include "economy/shop_sale_quote.h"
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
static bool accounted=false,busy=false,okay=true,found=true,barter=false,refuse=false;
static int roll=0,trophies=0,mutation=0,paid=-1,built=-1,min_calls=0;
static float trophy_mod=.05f,min_pct=.10f;
static P_obj selected=nullptr;static P_char actor=nullptr;
static std::vector<std::string> trace,fixture_messages;
bool economic_gameplay_authority::active(){return accounted;}
bool shop_trade_preparation_owner::production_available() noexcept{return false;}
persistence_mode persistence_mode_get(){return mode;}
bool shop_trade_transaction_player_busy(P_char){trace.push_back("busy");return busy;}
int is_ok(P_char,P_char,int){trace.push_back("okay");return okay;}
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
void mobsay(P_char,const char* text){fixture_messages.emplace_back(text);}
void do_tell(P_char,char* text,int){fixture_messages.emplace_back(text);}
void logit(const char*,const char*,...){}
void wizlog(int,const char*,...){}void statuslog(int,const char*,...){}
[[noreturn]] int panic_corruption_int(const char*,const char*,...){abort();}
int checked_substitute_strings(char* buffer,size_t size,const char*,const char*const*,size_t){if(size)*buffer=0;return 0;}
char* coin_stringv(int value,int){static char text[64];snprintf(text,sizeof(text),"%d copper",value);return text;}
bool isname(const char* name,const char* list){return list&&!strcmp(name,list);}
int fill_word(char*){return false;}
void act(const char*,int,P_char,P_obj,void*,int){}
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
'''

DRIVER = r'''
struct fixture {
 char_data player{},keeper{};pc_only_data pc{};npc_only_data npc{};obj_data item{};
 fixture(){
  shop={};shop.buy_percent=.5f;shop.message_sell=const_cast<char*>("%s %s");shop.missing_cash1=const_cast<char*>("%s cash");
  player.only.pc=&pc;pc.pid=42;player.player.name=const_cast<char*>("Seller");player.curr_stats.Cha=100;GET_COPPER(&player)=1000;
  keeper.specials.act=ACT_ISNPC;keeper.only.npc=&npc;keeper.player.name=const_cast<char*>("Keeper");GET_COPPER(&keeper)=1000;mob_indices[0].virtual_number=100;
  item.R_num=0;item_indices[0].virtual_number=200;item.cost=100;item.condition=100;item.type=ITEM_WEAPON;item.name=const_cast<char*>("sword");item.short_description=const_cast<char*>("a sword");
  item.loc_p=LOC_CARRIED;item.loc.carrying=&player;player.carrying=&item;selected=&item;actor=&player;
  mode=PERSISTENCE_MODE_MARIADB_PRIMARY;accounted=busy=barter=refuse=false;okay=found=true;roll=trophies=mutation=min_calls=0;paid=built=-1;trophy_mod=.05f;min_pct=.10f;
  trace.clear();fixture_messages.clear();produced_purchase_sequences.clear();for(auto& row:cha_app)row.modifier=0;
 }
 void call(bool value,const char* arg="sword"){char text[128];strcpy(text,arg);if(value)shopping_value(text,&player,&keeper,0);else shopping_sell(text,&player,&keeper,0);}
 int quote(bool value) const{if(!value)return paid;assert(!fixture_messages.empty());int price=-1;assert(sscanf(fixture_messages.back().c_str(),"The shopkeeper says 'I'll give you %d copper",&price)==1);return price;}
};
int main(){
 struct row{int modifier,cha,race,cost,condition;float percent;int expected;};
 const row rows[]={
  {0,100,0,101,100,.5f,50},{25,100,0,100,100,.5f,49},{-25,100,0,100,100,.5f,37},
  {-25,100,1,100,100,.5f,43},{25,100,1,100,100,.5f,49},{0,80,0,101,100,.5f,50},
  {0,140,0,101,100,.5f,50},{-128,100,0,100,100,.5f,1},{127,100,0,100,100,.5f,49},
  {0,100,0,101,50,.5f,25},{0,100,0,101,125,.5f,50},{0,100,0,100,-10,.5f,1},
  {0,100,0,100,0,.5f,1},{0,100,0,0,100,.5f,1},{0,100,0,-100,100,.5f,1},
  {0,100,0,101,100,.333f,33},{0,100,0,100,100,0.f,1},{0,100,0,100,100,-.5f,1}};
 int controls=0;
 for(bool value:{false,true})for(const auto& r:rows){fixture f;f.player.curr_stats.Cha=r.cha;GET_RACE(&f.player)=r.race;cha_app[MAX(100,r.cha)].modifier=static_cast<::byte>(r.modifier);f.item.cost=r.cost;f.item.condition=r.condition;shop.buy_percent=r.percent;f.call(value);int price=f.quote(value);printf("matrix %d %d %d\n",value,controls,price);assert(price==r.expected);assert(!value?trace==std::vector<std::string>({"okay","select","stat:"+std::to_string(MAX(100,r.cha)),"trophy","sql:"+std::to_string(price)}):trace==std::vector<std::string>({"okay","select","stat:"+std::to_string(MAX(100,r.cha)),"innate","trophy"}));++controls;}
 for(bool value:{false,true})for(int count:{0,1,2,20}){fixture f;trophies=count;f.call(value);int expected=count<=1?50:count==2?45:5;assert(f.quote(value)==expected);assert(min_calls==(count>1?count==20?2:1:0));++controls;}
 for(bool value:{false,true}){fixture f;trophies=20;mutation=3;trophy_mod=.1f;f.call(value);assert(f.quote(value)==10&&min_calls==2);++controls;}
 for(bool value:{false,true}){fixture f;trophies=20;mutation=4;f.call(value);assert(f.quote(value)==10&&min_calls==2);++controls;}
 for(bool value:{false,true}){fixture f;mutation=2;trophies=2;f.call(value);assert(f.quote(value)==45&&f.item.cost==9000);++controls;}
 for(int choice:{0,99,100,101}){fixture f;barter=true;roll=choice;f.call(true);assert(f.quote(true)==(choice<100?25:60));assert(trace==std::vector<std::string>({"okay","select","stat:100","innate","rng","trophy"}));++controls;}
 {fixture f;barter=true;f.call(false);assert(f.quote(false)==50);assert(trace==std::vector<std::string>({"okay","select","stat:100","trophy","sql:50"}));++controls;}
 {fixture f;barter=true;roll=0;mutation=1;f.call(true);printf("rng mutation %d\n",f.quote(true));assert(f.quote(true)==15);assert(f.player.curr_stats.Cha==0&&f.item.cost==120&&f.item.condition==50);++controls;}
 for(bool value:{false,true})for(int reason:{0,1,2,3}){fixture f;if(reason==0)okay=false;if(reason==1)found=false;if(reason==2)f.item.extra_flags|=ITEM_ARTIFACT;if(reason==3)f.item.name=const_cast<char*>("encrust");f.call(value);assert(paid==-1&&built==-1);assert(std::find(trace.begin(),trace.end(),"trophy")==trace.end());assert(std::find(trace.begin(),trace.end(),"stat:100")==trace.end());++controls;}
 for(bool value:{false,true}){fixture f;f.call(value,"");assert(trace==std::vector<std::string>({"okay"}));assert(fixture_messages.size()==1);++controls;}
 {fixture f;f.item.extra_flags|=ITEM_NODROP;f.call(false);assert(trace==std::vector<std::string>({"okay","select","stat:100"}));assert(paid==-1);++controls;}
 {fixture f;f.item.extra_flags|=ITEM_NODROP;f.call(true);assert(trace==std::vector<std::string>({"okay","select"}));++controls;}
 {fixture f;shop.shop_is_roaming=1;GET_COPPER(&f.keeper)=49;f.item.extra_flags|=ITEM_NODROP;f.call(false);assert(trace==std::vector<std::string>({"okay","select","stat:100"}));assert(paid==-1);++controls;}
 {fixture f;shop.shop_is_roaming=1;GET_COPPER(&f.keeper)=50;trophies=2;f.call(false);assert(paid==45&&GET_MONEY(&f.player)==1045&&GET_MONEY(&f.keeper)==5);++controls;}
 {fixture f;shop.shop_is_roaming=1;GET_COPPER(&f.keeper)=0;mob_indices[0].virtual_number=11005;f.call(false);assert(paid==50);++controls;}
 {fixture f;mode=PERSISTENCE_MODE_FLATFILE_PRIMARY;trophies=2;f.call(false);assert(built==45&&paid==-1&&GET_MONEY(&f.player)==1000);assert(trace==std::vector<std::string>({"okay","busy","select","stat:100","trophy","mod","min","build:45","submit"}));++controls;}
 {fixture f;mode=PERSISTENCE_MODE_FLATFILE_PRIMARY;busy=true;f.call(false);assert(trace==std::vector<std::string>({"okay","busy"}));++controls;}
 {fixture f;accounted=true;f.call(false);assert(trace.empty()&&fixture_messages.size()==1);++controls;}
 {fixture f;accounted=true;f.call(true);assert(f.quote(true)==50);++controls;}

 for(bool value:{false,true}){fixture f;mutation=5;f.call(value);assert(f.quote(value)==12&&shop.buy_percent==.25f&&f.item.condition==50);++controls;}
 for(bool value:{false,true}){fixture f;trophies=2;trophy_mod=.015f;f.call(value);assert(f.quote(value)==49);++controls;}
 for(bool value:{false,true}){fixture f;trophies=20;f.item.cost=2;trophy_mod=1;min_pct=0;f.call(value);assert(f.quote(value)==1);++controls;}
 {fixture f;mode=PERSISTENCE_MODE_FLATFILE_PRIMARY;refuse=true;trophies=2;f.call(false);assert(built==45&&paid==-1&&GET_MONEY(&f.player)==1000&&f.item.loc.carrying==&f.player);assert(fixture_messages.back()=="The shop transaction service is busy. Please try again.\r\n");++controls;}
 {fixture f;item_indices[0].virtual_number=FIRST_POTION_VIRTUAL;f.call(false);assert(trace==std::vector<std::string>({"okay","select"}));++controls;}
 {fixture f;obj_data contained{};f.item.type=ITEM_CONTAINER;f.item.contains=&contained;f.call(false);assert(trace==std::vector<std::string>({"okay","select"}));++controls;}
 for(bool value:{false,true}){fixture f;mutation=3;trophies=1;f.call(value);assert(f.quote(value)==50&&min_calls==0&&min_pct==.1f);++controls;}
 printf("original sale/value controls: %d\n",controls);
}
'''

def main():
    shop = source("economy/shop.c").read_text(encoding="utf-8")
    provider = shop[shop.index("struct shop_sale_quote_observations"):shop.index("void shopping_sell(")]
    harness = "\n".join([PRELUDE, provider,
        extract_function("cmd/interp.c", "char *one_argument("),
        extract_function("cmd/interp.c", "char *lohrr_chop("),
        extract_function("economy/shop.c", "static bool refuse_unported_shop_mutation("),
        extract_function("economy/shop.c", "void shopping_sell("),
        extract_function("economy/shop.c", "void shopping_value("), DRIVER])
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="shop-sale-", dir=build_root) as directory:
        cpp = Path(directory) / "shop_sale.cpp"
        binary = Path(directory) / "shop_sale"
        cpp.write_text(harness, encoding="utf-8")
        for flags in ([], ["-Og"]):
            subprocess.run(["g++", *flags, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                "-ffunction-sections", "-fdata-sections", "-Isrc", "-I/usr/include/libxml2",
                "-I/usr/include/mysql", str(cpp), "-Wl,--gc-sections", "-o", str(binary)],
                cwd=ROOT, check=True, timeout=120)
            subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)

if __name__ == "__main__":
    main()
