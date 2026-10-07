#!/usr/bin/env python3
"""Execute complete native valuation with controlled metadata and diagnostics.

Object types/masks are canonical. Circle/procedure values, racial/prototype rows
and diagnostic endpoints are fixtures. No native world metadata, item lifecycle,
admission, persistence or publication authority is qualified.
"""

from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/utility.h"
#include "economy/item_value_quote.h"
#include "item/objmisc.h"
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
int panic_corruption_int(const char*,const char*,...) { std::abort(); }
static index_data indices[2]{};
P_index obj_index = indices;
const struct stat_data stat_factor[LAST_RACE+1] = {
    {},{100,100,100,100,100,100,100,100,100,100},
    {75,75,75,75,75,75,75,75,75,75},
    {150,150,150,150,150,150,150,150,150,150}
};
static std::vector<std::string> trace;
static P_obj active;
static int mutation=0;
static int controlled_proc(P_obj,P_char,int,char*) { assert(false); return 0; }
int get_mincircle(int spell) {
    trace.push_back("circle:"+std::to_string(spell));
    if (mutation==1 && trace.size()==1) {
        active->value[5]=0;active->value[6]=80;active->value[7]=1;active->R_num=1;
    }
    return spell;
}
int get_ival_from_proc(obj_proc_type proc) {
    assert(proc==controlled_proc);trace.push_back("proc");
    if(mutation==1) {active->affected[0].location=APPLY_STR;active->affected[0].modifier=3;}
    return 7;
}
void debug(const char* format,...) {
    char message[1024]; va_list args; va_start(args,format);vsnprintf(message,sizeof(message),format,args);va_end(args);
    trace.push_back(message);
    if(mutation==2) {active->affected[0].location=APPLY_STR_MAX; active->affected[0].modifier=10;}
    if(mutation==3) {active->type=ITEM_LIGHT;}
}
static obj_data base() {
    obj_data obj{};obj.type=ITEM_LIGHT;obj.wear_flags=ITEM_TAKE;obj.R_num=0;
    obj.short_description=const_cast<char*>("native fixture");active=nullptr;trace.clear();mutation=0;
    indices[0]={};indices[1]={};indices[0].virtual_number=123;indices[1].virtual_number=456; indices[1].func.obj=controlled_proc;
    return obj;
}

'''

DRIVER = r'''

int main() {
    assert(itemvalue(nullptr)==0);
    { auto obj=base();assert(itemvalue(&obj)==1 && trace.empty());}
    { auto obj=base();obj.bitvector=AFF_STONE_SKIN;assert(itemvalue(&obj)==125);}
    { auto obj=base();obj.bitvector3=AFF3_GR_SPIRIT_WARD;assert(itemvalue(&obj)==55);}
    { auto obj=base();obj.bitvector5=~0UL;assert(itemvalue(&obj)==1);}
    struct apply_case {int location,modifier,type,expected;};
    const apply_case cases[]={
        {APPLY_STR,3,ITEM_LIGHT,6},{APPLY_STR,4,ITEM_LIGHT,9},
        {APPLY_DAMROLL,3,ITEM_WEAPON,7},{APPLY_DAMROLL,3,ITEM_LIGHT,37},
        {APPLY_HITROLL,2,ITEM_LIGHT,6},{APPLY_POW,6,ITEM_LIGHT,12},{APPLY_POW,7,ITEM_LIGHT,15},
        {APPLY_HIT,4,ITEM_LIGHT,8},{APPLY_HIT,5,ITEM_LIGHT,11},
        {APPLY_MOVE,25,ITEM_LIGHT,25},{APPLY_MANA,26,ITEM_LIGHT,29},
        {APPLY_HIT_REG,3,ITEM_LIGHT,3},{APPLY_MANA_REG,4,ITEM_LIGHT,5},
        {APPLY_AC,-50,ITEM_LIGHT,82},{APPLY_AC,50,ITEM_LIGHT,82},
        {APPLY_SAVING_PARA,-3,ITEM_LIGHT,18},{APPLY_SAVING_SPELL,3,ITEM_LIGHT,1},
        {APPLY_COMBAT_PULSE,-1,ITEM_LIGHT,150},{APPLY_COMBAT_PULSE,1,ITEM_LIGHT,1},
        {APPLY_STR_MAX,1,ITEM_LIGHT,3},{APPLY_STR_MAX,-2,ITEM_LIGHT,1},
        {APPLY_AGI_RACE,1,ITEM_LIGHT,50},{APPLY_STR_RACE,2,ITEM_LIGHT,1},{APPLY_CHA_RACE,3,ITEM_LIGHT,150},
        {APPLY_INT,127,ITEM_LIGHT,15876},{APPLY_DEX,-128,ITEM_LIGHT,1}
    };
    for(const auto& c:cases) {auto obj=base();obj.type=static_cast<::byte>(c.type);obj.affected[0].location=static_cast<::byte>(c.location);obj.affected[0].modifier=static_cast<sbyte>(c.modifier);const int value=itemvalue(&obj);if(value!=c.expected)fprintf(stderr,"apply %d mod%d expected%d observed%d\n",c.location,c.modifier,c.expected,value);assert(value==c.expected);}
    { auto obj=base();obj.type=ITEM_WEAPON;obj.value[0]=WEAPON_DAGGER;obj.value[1]=1;obj.value[2]=8;assert(itemvalue(&obj)==114);}
    { auto obj=base();obj.type=ITEM_ARMOR;obj.value[0]=-50;assert(itemvalue(&obj)==82);}
    { auto obj=base();obj.wear_flags|=ITEM_WIELD;obj.value[5]=1001;obj.value[6]=10;obj.value[7]=30;assert(itemvalue(&obj)==2);assert((trace==std::vector<std::string>{"circle:1","circle:1","circle:0"}));}
    { auto obj=base();obj.wear_flags|=ITEM_WIELD;obj.value[5]=1000001001;obj.value[6]=10;obj.value[7]=30;assert(itemvalue(&obj)==1);assert((trace==std::vector<std::string>{"circle:1","circle:1","circle:0"}));}
    { auto obj=base();active=&obj;mutation=1;obj.wear_flags|=ITEM_WIELD;obj.value[5]=1000001001;obj.value[6]=10;obj.value[7]=30;assert(itemvalue(&obj)==15);assert((trace==std::vector<std::string>{"circle:1","circle:1","circle:0","proc"}));assert(obj.R_num==1 && obj.value[5]==0);}
    { auto obj=base();active=&obj;mutation=2;obj.affected[0].location=APPLY_AGI_RACE;obj.affected[0].modifier=0;const int value=itemvalue(&obj);fprintf(stderr,"original diagnostic mutation value:%d\n",value);assert(value==114);assert(trace.size()==1 && trace[0].find("native fixture' 123")!=std::string::npos);}
    { auto obj=base();active=&obj;mutation=3;obj.type=ITEM_KEY;obj.R_num=1;assert(itemvalue(&obj)==1 && obj.type==ITEM_LIGHT);assert(trace.size()==2 && trace[0]=="proc" && trace[1].find("456 has stats giving ival 7.000")!=std::string::npos);}
    { auto obj=base();obj.type=ITEM_KEY;assert(itemvalue(&obj)==1 && trace.empty());}
    { auto obj=base();obj.type=ITEM_TELEPORT;obj.wear_flags=0;obj.bitvector=AFF_STONE_SKIN;assert(itemvalue(&obj)==1 && trace.size()==1);}
    { auto obj=base();obj.type=ITEM_TELEPORT;obj.bitvector=AFF_STONE_SKIN;assert(itemvalue(&obj)==125 && trace.empty());}
    { auto obj=base();obj.bitvector=AFF_STONE_SKIN;obj.wear_flags|=ITEM_WEAR_EYES|ITEM_WEAR_EARRING;obj.extra_flags=ITEM_TWOHANDS;assert(itemvalue(&obj)==156);}
    { auto obj=base();obj.wear_flags|=ITEM_WIELD;obj.value[5]=1000002001;obj.value[6]=25;obj.value[7]=30;assert(itemvalue(&obj)==2);assert((trace==std::vector<std::string>{"circle:1","circle:2","circle:0"}));}
    { auto obj=base();obj.wear_flags|=ITEM_WIELD;obj.value[5]=2001;obj.value[6]=25;obj.value[7]=30;assert(itemvalue(&obj)==7);assert((trace==std::vector<std::string>{"circle:1","circle:2","circle:0"}));}
    { auto obj=base();obj.bitvector4=AFF4_WILDMAGIC;assert(itemvalue(&obj)==240);}
    puts("original complete item valuation feasibility passed");
}
'''


def main():
    harness = "\n".join([PRELUDE, extract_function("economy/tradeskill.c", "int itemvalue("), DRIVER])
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="item-value-", dir=build_root) as directory:
        cpp = Path(directory) / "item_value.cpp"
        cpp.write_text(harness, encoding="utf-8")
        for optimization in ([], ["-Og"]):
            binary = Path(directory) / ("item_value_og" if optimization else "item_value")
            subprocess.run(["g++", *optimization, "-std=c++20", "-Wall", "-Wextra", "-Werror",
                            "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                            "-ffunction-sections", "-fdata-sections", "-Isrc", "-I/usr/include/libxml2",
                            "-I/usr/include/mysql", str(cpp), "-Wl,--gc-sections", "-o", str(binary)],
                           cwd=ROOT, check=True, timeout=120)
            subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)


if __name__ == "__main__":
    main()
