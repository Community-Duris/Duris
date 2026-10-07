#!/usr/bin/env python3
"""Exercise production superior material planning at numeric boundaries."""
from _paths import ROOT, SRC, extract_function
import os
from pathlib import Path
import subprocess
import tempfile

HARNESS = r'''
#include "economy/enhancement_material_quote.h"
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
constexpr bool TRUE=true, FALSE=false;
constexpr int MAX_OBJ_AFFECT=4, MAX_SUPERIOR_MATERIALS=8, APPLY_NONE=0, VIRTUAL=1;
#define MAX(a,b) ((a)>(b)?(a):(b))
struct object { struct { int location=0; signed char modifier=0; } affected[4]; };
using P_obj=object *;
struct enhance_index_entry { int vnum=1; int ival=1; } target;
struct superior_material_requirement { int vnum; int count; };
struct superior_enhancement_plan {
 int slots[4], slot_count, remaining_enhancements;
 superior_material_requirement materials[8]; int material_count;
};
double enhance_stat_material_quantity_multiplier=1.0;
bool is_superior_stat_apply(int) { return true; }
int enhance_base_modifier(P_obj,int) { return 10; }
int enhance_stat_cap(int) { return 20; }
enhance_index_entry *find_stat_enhance_target(P_obj,int,int) { return &target; }
object temporary;
P_obj read_object(int,int) { return &temporary; }
int get_matstart(P_obj) { return 400045; }
void extract_obj(P_obj) {}
int superior_stat_remaining_steps(P_obj,int,int,int) { return 1; }
@FUNCTIONS@
int main() {
 enhancement_material_quote owned{7,8};
 assert(enhancement_prepare_material_quote(0,1.0,&owned) && owned.low_count==4 && owned.high_count==0);
 assert(enhancement_prepare_material_quote(INT_MAX,1.0,&owned) && owned.low_count==1 && owned.high_count==429496730);
 const enhancement_material_quote sentinel=owned;
 for (int value : {3,6}) {
  assert(!enhancement_prepare_material_quote(value,static_cast<double>(INT_MAX),&owned));
  assert(owned.low_count==sentinel.low_count && owned.high_count==sentinel.high_count);
 }
 assert(!enhancement_prepare_material_quote(-1,1.0,&owned));
 assert(!enhancement_prepare_material_quote(INT_MIN,1.0,&owned));
 assert(!enhancement_prepare_material_quote(1,1.0,nullptr));
 for (double multiplier : {static_cast<double>(INFINITY),static_cast<double>(NAN),1e300,-1.0,0.0}) {
  assert(!enhancement_prepare_material_quote(1,multiplier,&owned));
  assert(owned.low_count==sentinel.low_count && owned.high_count==sentinel.high_count);
 }
 assert(enhancement_prepare_material_quote(1,0.0000005,&owned) && owned.low_count==0 && owned.high_count==0);
 enhancement_material_quote repeated;
 assert(enhancement_prepare_material_quote(1,0.0000005,&repeated));
 assert(repeated.low_count==owned.low_count && repeated.high_count==owned.high_count);
 int scaled=42;
 assert(!enhancement_scale_material_count(-1,1.0,&scaled) && scaled==42);
 assert(!enhancement_scale_material_count(1,1.0,nullptr));
 assert(enhancement_scale_material_count(0,1.0,&scaled) && scaled==0);

 object item; item.affected[0].location=1; item.affected[0].modifier=10;
 superior_enhancement_plan plan;
 target.ival=0;
 assert(build_superior_enhancement_plan(&item,&plan));
 assert(plan.material_count==1 && plan.materials[0].vnum==400045 && plan.materials[0].count==4);
 target.ival=INT_MAX;
 assert(build_superior_enhancement_plan(&item,&plan));
 assert(plan.material_count==2 && plan.materials[0].count==1 && plan.materials[1].count==429496730);
 target.ival=1;
 for(double multiplier : {static_cast<double>(INFINITY), static_cast<double>(NAN), 1e300, -1.0, 0.0}) {
  enhance_stat_material_quantity_multiplier=multiplier;
  assert(!build_superior_enhancement_plan(&item,&plan));
  assert(item.affected[0].modifier==10);
 }
 enhance_stat_material_quantity_multiplier=1.5;
 target.ival=2;
 assert(build_superior_enhancement_plan(&item,&plan));
 assert(plan.materials[0].count==2 && plan.materials[1].count==2);
 enhance_stat_material_quantity_multiplier=0.0000005; target.ival=1;
 assert(build_superior_enhancement_plan(&item,&plan));
 assert(plan.material_count==0 && plan.slot_count==1 && item.affected[0].modifier==10);
 target.ival=-1;
 assert(!build_superior_enhancement_plan(&item,&plan));
 enhance_stat_material_quantity_multiplier=1.0;
 std::memset(&plan,0,sizeof(plan));
 assert(superior_plan_add_material(&plan,400045,INT_MAX));
 assert(!superior_plan_add_material(&plan,400045,1));
 assert(plan.materials[0].count==INT_MAX);
 assert(superior_plan_add_material(&plan,400049,1));
}
'''
source=(SRC/'enhance.c').read_text()
functions=[]
if 'static bool scale_superior_material_count(' in source:
    functions.append(extract_function('enhance.c','static bool scale_superior_material_count('))
functions += [extract_function('enhance.c', 'static bool superior_plan_add_material('),
              extract_function('enhance.c', 'static bool build_superior_enhancement_plan(')]
with tempfile.TemporaryDirectory(prefix='duris-superior-material-') as temporary:
    cpp=Path(temporary)/'plan.cpp'; binary=Path(temporary)/'plan'
    cpp.write_text(HARNESS.replace('@FUNCTIONS@','\n'.join(functions)))
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-O1','-g','-fsanitize=address,undefined,float-cast-overflow',
                    '-fno-sanitize-recover=all','-fno-pie','-no-pie','-Isrc',str(cpp),'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True,timeout=30)
print('superior material planning: wide quotes, invalid scaling refusal, aggregation bounds passed')
