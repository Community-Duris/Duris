#!/usr/bin/env python3
"""Exercise the production Craft/Forge quote and recipe gate at numeric bounds."""
from _paths import ROOT, SRC, extract_function
import os
from pathlib import Path
import subprocess
import tempfile

HARNESS = r'''
#include "economy/crafting.h"
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <limits>

int quoted_value=1, material_vnum=400045;
bool magical=false;
double crafting_material_quantity_multiplier=1.0;
int crafting_level_gate=3, crafting_recipe_max_player_level=50;
bool crafting_craft_enabled=true, crafting_forge_enabled=true;
int itemvalue(P_obj) { return quoted_value; }
int get_matstart(P_obj) { return material_vnum; }
bool has_affect(P_obj) { return magical; }
@PLANNER@
bool crafting_validate_recipe_target(P_obj item) {
 crafting_plan plan{}; return crafting_build_plan(item,&plan);
}
@GATE@
int main() {
 obj_data item{};
 crafting_plan plan{};
 quoted_value=INT_MAX;
 assert(crafting_build_plan(&item,&plan));
 assert(plan.item_value==INT_MAX && plan.high_material_count==429496730 && plan.low_material_count==1);
 for (int value : {1,2,5,10,150}) {
  quoted_value=value; magical=value==5;
  assert(crafting_build_plan(&item,&plan));
  assert(plan.high_material_count==(value+4)/5 && plan.low_material_count==(value+4)%5);
  assert(plan.low_material_vnum==400045 && plan.high_material_vnum==400049 && plan.magical==magical);
 }
 quoted_value=2; crafting_material_quantity_multiplier=1.5;
 assert(crafting_build_plan(&item,&plan));
 assert(plan.high_material_count==2 && plan.low_material_count==2);
 crafting_material_quantity_multiplier=static_cast<double>(INT_MAX);
 quoted_value=1;
 assert(crafting_build_plan(&item,&plan) && plan.high_material_count==INT_MAX && plan.low_material_count==0);
 const crafting_plan unchanged=plan;
 for (double multiplier : {std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::quiet_NaN(),1e300,-1.0,0.0}) {
  crafting_material_quantity_multiplier=multiplier;
  assert(!crafting_build_plan(&item,&plan));
  assert(plan.item_value==unchanged.item_value && plan.low_material_count==unchanged.low_material_count &&
         plan.high_material_count==unchanged.high_material_count && plan.low_material_vnum==unchanged.low_material_vnum &&
         plan.high_material_vnum==unchanged.high_material_vnum && plan.magical==unchanged.magical);
 }
 crafting_material_quantity_multiplier=1.0;
 for (int value : {-1,0}) { quoted_value=value; assert(!crafting_build_plan(&item,&plan)); }
 quoted_value=1; material_vnum=INT_MAX;
 assert(!crafting_build_plan(&item,&plan));
 material_vnum=400045;
 assert(!crafting_build_plan(nullptr,&plan) && !crafting_build_plan(&item,nullptr));
 // The production wrapper above calls the actual owned-state implementation.
 // Evaluate owned facts independently of global/live values, including failure
 // after the high quote succeeds but the low quote exceeds INT_MAX.
 crafting_plan owned{};
 assert(crafting_prepare_plan(2,400045,true,1.5,&owned));
 assert(owned.item_value==2 && owned.low_material_vnum==400045 &&
        owned.high_material_vnum==400049 && owned.high_material_count==2 &&
        owned.low_material_count==2 && owned.magical);
 const crafting_plan frozen=owned;
 assert(!crafting_prepare_plan(4,400045,false,static_cast<double>(INT_MAX),&owned));
 assert(owned.item_value==frozen.item_value && owned.low_material_vnum==frozen.low_material_vnum &&
        owned.high_material_vnum==frozen.high_material_vnum && owned.low_material_count==frozen.low_material_count &&
        owned.high_material_count==frozen.high_material_count && owned.magical==frozen.magical);
 assert(crafting_prepare_plan(2,400045,true,1.5,&owned));
 assert(owned.low_material_count==frozen.low_material_count && owned.high_material_count==frozen.high_material_count);
 assert(crafting_prepare_plan(1,INT_MAX-4,false,1.0,&owned) && owned.high_material_vnum==INT_MAX);
 assert(!crafting_prepare_plan(1,INT_MAX-3,false,1.0,&owned));
 assert(!crafting_prepare_plan(1,400045,false,1.0,nullptr));
 quoted_value=150; assert(crafting_recipe_target_is_available(&item));
 quoted_value=151; assert(!crafting_recipe_target_is_available(&item));
 assert(crafting_required_level(INT_MAX)==715827883);
 crafting_level_gate=INT_MAX; quoted_value=INT_MAX;
 assert(crafting_required_level(INT_MAX)==1);
 assert(crafting_recipe_target_is_available(&item));
 crafting_craft_enabled=crafting_forge_enabled=false;
 assert(!crafting_recipe_target_is_available(&item));
}
'''
source=(SRC/'crafting.c').read_text()
functions=[]
if 'static int crafting_required_level(' in source:
    functions.append(extract_function('crafting.c','static int crafting_required_level('))
if 'static bool crafting_scale_material_count(' in source:
    functions.append(extract_function('crafting.c','static bool crafting_scale_material_count('))
functions.append(extract_function('crafting.c','bool crafting_build_plan('))
harness=HARNESS.replace('@PLANNER@','\n'.join(functions)).replace(
    '@GATE@',extract_function('crafting.c','bool crafting_recipe_target_is_available('))
with tempfile.TemporaryDirectory(prefix='duris-craft-plan-bounds-') as temporary:
    cpp=Path(temporary)/'plan.cpp'; binary=Path(temporary)/'plan'
    cpp.write_text(harness)
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-O1','-g','-fsanitize=address,undefined,float-cast-overflow',
                    '-fno-sanitize-recover=all','-fno-pie','-no-pie','-Isrc',
                    '-I/usr/include/libxml2',str(cpp),'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True,timeout=30)
print('Craft/Forge planning: wide material quotes, finite scaling, refusal without partial plan, wide recipe gate passed')
