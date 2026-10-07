#!/usr/bin/env python3
"""Execute native item acceptance/selection and the complete keyword parser."""

from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function, source

PRELUDE_CONTROLLED = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "net/comm.h"
#include "economy/shop.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
static shop_data shop{};shop_data*shop_index=&shop;
static obj_data*selected=nullptr;static int expression_result=1,mutation=0,queries=0,lookups=0;
static bool found=true;static std::vector<std::string> fixture_messages,expressions,substitutions;
int evaluate_expression(P_obj object,char*expr){assert(object==selected);++queries;expressions.emplace_back(expr?expr:"<null>");if(mutation==1){object->cost=0;object->type=ITEM_STAFF;object->value[2]=0;shop.type[0].type=0;}return expression_result;}
P_obj get_obj_in_list_vis(P_char,const char*,P_obj,bool){++lookups;if(mutation==2)selected->extra_flags|=ITEM_NOSELL;return found?selected:nullptr;}
void mobsay(P_char,const char*message){fixture_messages.emplace_back(message);}
void logit(const char*,const char*,...){assert(false);}
[[noreturn]] int panic_corruption_int(const char*,const char*,...){abort();}
int checked_substitute_strings(char*buffer,size_t size,const char*format,const char*const*,size_t){substitutions.emplace_back(format);snprintf(buffer,size,"%s",format);return 0;}
'''

DRIVER_CONTROLLED = r'''
struct fixture {
 obj_data item{};char_data actor{},keeper{};shop_buy_data rows[4]{};
 fixture(){item.cost=100;item.type=ITEM_WEAPON;item.value[2]=3;item.name=const_cast<char*>("sword");actor.player.name=const_cast<char*>("Seller");actor.carrying=&item;selected=&item;shop={};shop.type=rows;rows[0].type=ITEM_WEAPON;rows[0].keywords=const_cast<char*>("weapon");shop.no_such_item2=const_cast<char*>("missing");shop.do_not_buy=const_cast<char*>("declined");expression_result=1;mutation=queries=lookups=0;found=true;fixture_messages.clear();expressions.clear();substitutions.clear();}
 int classify(char repairing=0){return trade_with(&item,0,repairing);}
 P_obj select(int msg=1,char repairing=0){char name[]="sword";return get_selling_obj(&actor,name,&keeper,0,msg,repairing);}
};
int main(){int controls=0;
 for(int cost:{-1,0,1,100}){fixture f;f.item.cost=cost;assert(f.classify()==(cost<1?OBJECT_NOTOK:OBJECT_OK));assert(queries==(cost<1?0:1));++controls;}
 for(char repairing:{char(-1),char(0),char(1),char(127)}){fixture f;f.item.extra_flags=ITEM_NOSELL;assert(f.classify(repairing)==(!repairing?OBJECT_NOTOK:OBJECT_OK));assert(queries==(!repairing?0:1));++controls;}
 for(char repairing:{char(0),char(1)}){fixture f;f.item.extra_flags=ITEM_NOSELL|ITEM_TRANSIENT;assert(f.classify(repairing)==OBJECT_NOTOK&&queries==0);++controls;}
 for(int type:{ITEM_WAND,ITEM_STAFF})for(int charges:{-1,0,1}){fixture f;f.item.type=type;f.rows[0].type=type;f.item.value[2]=charges;assert(f.classify()==(charges==0?OBJECT_DEAD:OBJECT_OK));assert(queries==(charges==0?0:1));++controls;}
 for(int result:{-1,0,1,2}){fixture f;expression_result=result;assert(f.classify()==OBJECT_OK&&queries==1&&expressions==std::vector<std::string>({"weapon"}));++controls;}
 {fixture f;f.rows[0].keywords=nullptr;assert(f.classify()==OBJECT_OK&&expressions==std::vector<std::string>({"<null>"}));++controls;}
 {fixture f;f.rows[0].type=ITEM_ARMOR;f.item.type=ITEM_WORN;assert(f.classify()==OBJECT_OK&&queries==0);++controls;}
 {fixture f;f.rows[0].type=ITEM_WORN;f.item.type=ITEM_ARMOR;assert(f.classify()==OBJECT_NOTOK&&queries==0);++controls;}
 {fixture f;f.rows[0].type=ITEM_FOOD;f.rows[1].type=ITEM_WEAPON;f.rows[1].keywords=const_cast<char*>("later");assert(f.classify()==OBJECT_OK&&expressions==std::vector<std::string>({"later"}));++controls;}
 {fixture f;f.rows[0].type=0;f.rows[1].type=ITEM_WEAPON;assert(f.classify()==OBJECT_NOTOK&&queries==0);++controls;}
 {fixture f;f.rows[0].type=ITEM_WEAPON;f.rows[1].type=ITEM_WEAPON;f.rows[1].keywords=const_cast<char*>("unused");assert(f.classify()==OBJECT_OK&&expressions==std::vector<std::string>({"weapon"}));++controls;}
 {fixture f;mutation=1;expression_result=0;assert(f.classify()==OBJECT_OK&&queries==1&&f.item.cost==0&&f.item.type==ITEM_STAFF&&f.rows[0].type==0);++controls;}
 for(int msg:{0,1,-1}){fixture f;assert(f.select(msg)==&f.item&&lookups==1&&queries==1&&fixture_messages.empty());++controls;}
 for(int msg:{0,1}){fixture f;found=false;assert(f.select(msg)==nullptr&&lookups==1&&queries==0);assert(fixture_messages==(msg?std::vector<std::string>({"missing"}):std::vector<std::string>{}));assert(substitutions.size()==size_t(msg));++controls;}
 for(int msg:{0,1}){fixture f;f.item.cost=0;assert(f.select(msg)==nullptr&&queries==0);assert(substitutions==std::vector<std::string>({"declined"}));assert(fixture_messages==(msg?std::vector<std::string>({"declined"}):std::vector<std::string>{}));++controls;}
 for(int type:{ITEM_WAND,ITEM_STAFF}){fixture f;f.item.type=type;f.rows[0].type=type;f.item.value[2]=0;assert(f.select()==nullptr&&queries==0&&fixture_messages==std::vector<std::string>({"Seller I don't buy used up wands or staves!"}));++controls;}
 {fixture f;f.item.extra_flags=ITEM_NOSELL;assert(f.select(1,1)==&f.item&&queries==1);++controls;}
 {fixture f;mutation=2;assert(f.select()==nullptr&&lookups==1&&queries==0&&fixture_messages==std::vector<std::string>({"declined"}));++controls;}
 printf("original shop eligibility controls: %d\n",controls);
}
'''

PRELUDE_REAL = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "net/comm.h"
#include "economy/shop.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
static shop_data shop{};shop_data*shop_index=&shop;
static P_obj selected=nullptr;static int mutation=0,lookups=0;
static bool found=true;static std::vector<std::string> fixture_messages,observations;
bool isname(const char*name,const char*list){assert(list);observations.emplace_back(std::string("name:")+name);const bool match=!strcasecmp(name,list);if(mutation==1){selected->cost=0;selected->type=ITEM_STAFF;selected->value[2]=0;shop.type[0].type=0;}return match;}
P_obj get_obj_in_list_vis(P_char,const char*,P_obj,bool){++lookups;observations.emplace_back("lookup");return found?selected:nullptr;}
void mobsay(P_char,const char*message){fixture_messages.emplace_back(message);observations.emplace_back("mobsay");}
void logit(const char*,const char*format,...){char buffer[256];va_list args;va_start(args,format);vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);observations.emplace_back(std::string("log:")+buffer);}
[[noreturn]] int panic_corruption_int(const char*,const char*,...){abort();}
int checked_substitute_strings(char*buffer,size_t size,const char*format,const char*const*,size_t){snprintf(buffer,size,"%s",format);return 0;}
'''

DRIVER_REAL = r'''
struct fixture {
 obj_data item{};char_data actor{},keeper{};shop_buy_data rows[3]{};
 fixture(){item.cost=100;item.type=ITEM_WEAPON;item.value[2]=3;item.name=const_cast<char*>("sword");actor.player.name=const_cast<char*>("Seller");actor.carrying=&item;selected=&item;shop={};shop.type=rows;rows[0].type=ITEM_WEAPON;rows[0].keywords=const_cast<char*>("sword");shop.no_such_item2=const_cast<char*>("missing");shop.do_not_buy=const_cast<char*>("declined");mutation=lookups=0;found=true;fixture_messages.clear();observations.clear();}
 int evaluate(const char*expression){return evaluate_expression(&item,const_cast<char*>(expression));}
 int classify(const char*expression){rows[0].keywords=const_cast<char*>(expression);return trade_with(&item,0,0);}
 P_obj select(const char*expression){rows[0].keywords=const_cast<char*>(expression);char name[]="sword";return get_selling_obj(&actor,name,&keeper,0,1,0);}
};
int main(){int controls=0;
 struct row{const char*expression;int expected;std::vector<std::string> trace;};
 const std::string illegal="log:Illegal expression in shop keyword list";
 const std::string extra="log:Extra operands left on shop keyword expression stack";
 const row rows[]={
  {nullptr,TRUE,{}},{"sword",TRUE,{"name:sword"}},{"bread",FALSE,{"name:bread"}},
  {"",FALSE,{illegal}},{"sword bread",FALSE,{"name:sword","name:bread",extra}},
  {")",FALSE,{illegal,illegal}},{"sword)",TRUE,{"name:sword",illegal}},
  {"^sword",FALSE,{"name:sword"}},{"^bread",TRUE,{"name:bread"}},
  {"sword&bread",FALSE,{"name:sword","name:bread",extra}},
  {"bread&sword",FALSE,{"name:bread","name:sword"}},
  {"sword|bread",TRUE,{"name:sword","name:bread"}},
  {"bread|sword",TRUE,{"name:bread","name:sword"}},
  {"(sword",FALSE,{"name:(sword"}}
 };
 for(const auto&r:rows){fixture f;const int result=f.evaluate(r.expression);printf("real parser %d result %d\n",controls,result);assert(result==r.expected&&observations==r.trace);++controls;}
 for(const auto&r:rows){fixture f;assert(f.classify(r.expression)==OBJECT_OK&&observations==r.trace);++controls;}
 for(const auto&r:rows){fixture f;assert(f.select(r.expression)==&f.item&&fixture_messages.empty()&&lookups==1);std::vector<std::string> expected={"lookup"};expected.insert(expected.end(),r.trace.begin(),r.trace.end());assert(observations==expected);++controls;}
 for(int flag:{0,1,2}){fixture f;f.item.extra_flags=flag;const char*expr=flag==2?"noshow":"glow";assert(f.evaluate(expr)==flag&&observations.empty());assert(f.classify(expr)==OBJECT_OK&&observations.empty());controls+=2;}
 for(int type:{ITEM_WAND,ITEM_STAFF}){fixture f;f.item.type=type;f.rows[0].type=type;f.item.value[2]=0;assert(f.classify("sword)")==OBJECT_DEAD&&observations.empty());++controls;}
 {fixture f;f.item.type=ITEM_WORN;f.rows[0].type=ITEM_ARMOR;assert(f.classify(")")==OBJECT_OK&&observations.empty());++controls;}
 {fixture f;mutation=1;assert(f.classify("bread")==OBJECT_OK&&observations==std::vector<std::string>({"name:bread"}));assert(f.item.cost==0&&f.item.type==ITEM_STAFF&&f.rows[0].type==0);++controls;}
 {fixture f;found=false;assert(f.select(")")==nullptr&&observations==std::vector<std::string>({"lookup","mobsay"}));++controls;}
 {fixture f;f.item.cost=0;assert(f.select(")")==nullptr&&observations==std::vector<std::string>({"lookup","mobsay"}));++controls;}
 printf("real keyword original controls: %d\n",controls);
}
'''

def main():
    shop = source("economy/shop.c").read_text(encoding="utf-8")
    common = source("core/common.c").read_text(encoding="utf-8")
    start = shop.index("const char *operator_str[] =")
    operators = shop[start:shop.index(";", start) + 1]
    start = common.index("flagDef extra_bits[] =")
    flags = common[start:common.index("};", start) + 2]
    classifier = extract_function("economy/shop.c", "int trade_with(")
    selector = extract_function("economy/shop.c", "P_obj get_selling_obj(")
    helpers = [extract_function("economy/shop.c", signature) for signature in
        ["void push(", "int topp(", "int pop(", "void evaluate_operation(",
         "int find_oper_num(", "int evaluate_expression("]]
    include = '#include "economy/shop_item_acceptance.h"'
    components = {
        "controlled": "\n".join([PRELUDE_CONTROLLED, include, classifier, selector, DRIVER_CONTROLLED]),
        "real": "\n".join([PRELUDE_REAL, include, operators, flags, *helpers, classifier, selector, DRIVER_REAL]),
    }
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="shop-acceptance-", dir=build_root) as directory:
        for name, harness in components.items():
            cpp = Path(directory) / ("shop_item_" + name + ".cpp")
            binary = Path(directory) / ("shop_item_" + name)
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
