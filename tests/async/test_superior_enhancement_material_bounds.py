#!/usr/bin/env python3
"""Exercise production superior planning and its original native observation order."""
from _paths import ROOT, SRC, extract_function
import os
from pathlib import Path
import subprocess
import tempfile

HARNESS = r'''
#include "economy/enhancement_superior_plan.h"
@CAP_INCLUDE@
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
constexpr int APPLY_NONE=0, VIRTUAL=1;
#define MAX(a,b) ((a)>(b)?(a):(b))
struct object { struct { int location=0; signed char modifier=0; } affected[4]; int vnum=100; unsigned int wear_flags=1; };
#define OBJ_VNUM(item) ((item)->vnum)
using P_obj=object *;
struct enhance_index_entry { int vnum=1; int ival=1; int apply_loc[4]={1,0,0,0}, apply_mod[4]={11,0,0,0}; unsigned int wear_flags=1; enhance_index_entry *next=nullptr; } target;

double enhance_stat_material_quantity_multiplier=1.0;
#include <string>
#include <vector>
int mode=-1, observed_slot=-1;
std::vector<std::string> trace;
superior_enhancement_plan *observed_plan=nullptr;
P_obj observed_item=nullptr;
object prototype;
enhance_index_entry *enhance_stat_table[1]={&target}, *selected_target=nullptr;
unsigned int enhance_wear_skip_mask=0;
double enhance_stat_cap_multiplier=2.0;
std::vector<std::pair<int,int>> queries;
std::vector<int> cap_inputs;
static int baseline_enhance_base_modifier(P_obj,int);
static int baseline_enhance_stat_cap(int);
static enhance_index_entry *baseline_find_stat_enhance_target(P_obj,int,int);
static int baseline_superior_stat_remaining_steps(P_obj,int,int,int);
int enhance_stat_hash(int) { return 0; }
bool native_mode() { return mode>=10 && mode!=17; }
void event(const char *name) { if(mode>=0) trace.emplace_back(name); }
bool is_superior_stat_apply(int location) {
 if(mode>=0) { observed_slot=location-1; event("eligible"); }
 return !(mode==1 && location==3);
}
int enhance_base_modifier(P_obj item,int location) {
 if(mode>=0) { observed_slot=location-1; event("base"); }
 if(native_mode()) return baseline_enhance_base_modifier(item,location);
 if(mode==1 && location==4) return 0;
 if(mode==6) item->affected[observed_slot].modifier=20;
 return 10;
}
int enhance_stat_cap(int base) { event("cap"); if(native_mode()) { cap_inputs.push_back(base);return baseline_enhance_stat_cap(base); } return 20; }
enhance_index_entry *find_stat_enhance_target(P_obj item,int location,int modifier) {
 event("find");
 if(native_mode()) { queries.emplace_back(location,modifier); selected_target=baseline_find_stat_enhance_target(item,location,modifier); return selected_target; }
 if(mode==2 && observed_slot==1) target.ival=-1;
 if(mode==3) target.ival=observed_slot==0 ? 1 : 2;
 if(mode==7 && observed_slot==0) return nullptr;
 return &target;
}
object temporary;
P_obj read_object(int vnum,int) { if(native_mode() && vnum==100) { event("prototype-read");return mode==11 ? nullptr : &prototype; } event("read"); return mode==8 || mode==19 ? nullptr : &temporary; }
int get_matstart(P_obj) { event("material"); return 400045; }
void extract_obj(P_obj object) {
 if(native_mode() && object==&prototype) {
  event("prototype-cleanup");
  if(mode==12) enhance_stat_cap_multiplier=1.0;
  if(mode==13) observed_item->affected[0].modifier=20;
  if(mode==18) observed_item->affected[0].location=4;
  return;
 }
 event("cleanup");
 if(mode==14) {
  selected_target->ival=2;enhance_stat_material_quantity_multiplier=1.5;
  observed_item->affected[0].location=4;observed_item->affected[0].modifier=12;
 }
 if(mode==4) {
  observed_plan->material_count=7;
  for(int i=0;i<7;++i) observed_plan->materials[i]={100+i,1};
 }
 if(mode==5) {
  target.ival=2; enhance_stat_material_quantity_multiplier=1.5;
  observed_item->affected[0].location=4; observed_item->affected[0].modifier=12;
 }
}
int superior_stat_remaining_steps(P_obj item,int location,int current,int cap) {
 event("remaining");
 if(mode>=0) {
  assert(observed_plan->slot_count==observed_slot+1);
  assert(observed_plan->slots[observed_slot]==observed_slot);
 }
 if(native_mode()) return baseline_superior_stat_remaining_steps(item,location,current,cap);
 if(mode==5) assert(location==4 && current==12 && cap==20);
 if(mode==9 && observed_slot==0) item->affected[1].modifier=0;
 return mode==0 ? (observed_slot==1 ? 4 : observed_slot+1) : 1;
}
void assert_plan(std::initializer_list<int> slots, int remaining,
                 std::initializer_list<superior_material_requirement> materials) {
 superior_enhancement_plan expected{};
 for(int slot:slots) expected.slots[expected.slot_count++]=slot;
 expected.remaining_enhancements=remaining;
 for(auto material:materials) expected.materials[expected.material_count++]=material;
 assert(std::memcmp(observed_plan,&expected,sizeof(expected))==0);
}

@NATIVE_FUNCTIONS@
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
 mode=0; observed_item=&item; observed_plan=&plan; trace.clear();
 enhance_stat_material_quantity_multiplier=1.0; target.ival=2;
 for(int i=0;i<4;++i) item.affected[i]={i+1,10};
 std::memset(&plan,0xa5,sizeof(plan));
 assert(build_superior_enhancement_plan(&item,&plan));
 const std::vector<std::string> full_step={"eligible","base","cap","find","read","material","cleanup","remaining"};
 std::vector<std::string> expected;
 for(int i=0;i<4;++i) expected.insert(expected.end(),full_step.begin(),full_step.end());
 assert(trace==expected && plan.slot_count==4 && plan.material_count==2 && plan.remaining_enhancements==4);
 assert(plan.materials[0].count==4 && plan.materials[1].count==4);
 for(int i=0;i<4;++i) assert(plan.slots[i]==i);
 assert_plan({0,1,2,3},4,{{400045,4},{400049,4}});
 mode=1;trace.clear(); item.affected[0].location=0;item.affected[1].modifier=0;
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert((trace==std::vector<std::string>{"eligible","eligible","base","cap"}));
 superior_enhancement_plan empty{};assert(std::memcmp(&plan,&empty,sizeof(plan))==0);
 auto prepare_case=[&](int selected,int slots=1) {
  mode=selected;trace.clear(); target.ival=2;enhance_stat_material_quantity_multiplier=1.0;
  item={};for(int i=0;i<slots;++i)item.affected[i]={i+1,10};
  std::memset(&plan,0xa5,sizeof(plan));
 };
 prepare_case(2,3);
 assert(!build_superior_enhancement_plan(&item,&plan));
 expected=full_step; expected.insert(expected.end(),full_step.begin(),full_step.end()-1);
 assert(trace==expected && plan.slot_count==1 && plan.material_count==2 && plan.remaining_enhancements==1);
 assert(plan.materials[0].count==1 && plan.materials[1].count==1);
 assert_plan({0},1,{{400045,1},{400049,1}});
 prepare_case(3,3);enhance_stat_material_quantity_multiplier=static_cast<double>(INT_MAX)-1;
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert(trace==expected && plan.slot_count==1 && plan.material_count==2 && plan.remaining_enhancements==1);
 assert(plan.materials[0].vnum==400049 && plan.materials[0].count==INT_MAX-1);
 assert(plan.materials[1].vnum==400045 && plan.materials[1].count==INT_MAX-1);
 assert_plan({0},1,{{400049,INT_MAX-1},{400045,INT_MAX-1}});
 prepare_case(4,2);
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert((trace==std::vector<std::string>(full_step.begin(),full_step.end()-1)));
 assert(plan.slot_count==0 && plan.material_count==8 && plan.materials[7].vnum==400045);
 assert_plan({},0,{{100,1},{101,1},{102,1},{103,1},{104,1},{105,1},{106,1},{400045,1}});
 prepare_case(5);target.ival=1;
 assert(build_superior_enhancement_plan(&item,&plan));
 assert(trace==full_step && plan.material_count==2 && plan.materials[0].count==2 && plan.materials[1].count==2);
 assert_plan({0},1,{{400045,2},{400049,2}});
 prepare_case(6);
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert((trace==std::vector<std::string>{"eligible","base","cap"}) && plan.slot_count==0);
 assert_plan({},0,{});
 prepare_case(7);
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert((trace==std::vector<std::string>{"eligible","base","cap","find"}));
 assert_plan({},0,{});
 prepare_case(8);
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert((trace==std::vector<std::string>{"eligible","base","cap","find","read"}));
 assert_plan({},0,{});
 prepare_case(9,2);
 assert(build_superior_enhancement_plan(&item,&plan));
 assert(trace==full_step && plan.slot_count==1 && item.affected[1].modifier==0);
 assert_plan({0},1,{{400045,1},{400049,1}});
 mode=-1;

 auto prepare_native=[&](int selected) {
  prepare_case(selected); prototype={};prototype.affected[0]={1,10};prototype.affected[1]={1,99};
  target={};target.ival=2;enhance_stat_table[0]=&target; selected_target=nullptr;
  enhance_stat_cap_multiplier=2.0;enhance_wear_skip_mask=0;queries.clear();cap_inputs.clear();
 };
 const std::vector<std::string> native_step={"eligible","base","prototype-read","prototype-cleanup","cap","find","read","material","cleanup","remaining","find","find"};
 prepare_native(10);
 assert(build_superior_enhancement_plan(&item,&plan));
 assert(trace==native_step && plan.slot_count==1 && plan.remaining_enhancements==1);
 assert((cap_inputs==std::vector<int>{10}));
 assert((queries==std::vector<std::pair<int,int>>{{1,11},{1,11},{1,12}}));
 assert_plan({0},1,{{400045,1},{400049,1}});
 prepare_native(11);
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert((trace==std::vector<std::string>{"eligible","base","prototype-read","cap"}));
 assert((cap_inputs==std::vector<int>{0}));
 assert_plan({},0,{});
 prepare_native(12);
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert((trace==std::vector<std::string>{"eligible","base","prototype-read","prototype-cleanup","cap"}));
 assert(enhance_stat_cap_multiplier==1.0 && plan.slot_count==0);
 assert_plan({},0,{});
 prepare_native(13);
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert((trace==std::vector<std::string>{"eligible","base","prototype-read","prototype-cleanup","cap"}));
 assert(item.affected[0].modifier==20 && plan.slot_count==0);
 assert_plan({},0,{});
 prepare_native(14);target.ival=1;
 enhance_index_entry future13,future15;future13.apply_loc[0]=future15.apply_loc[0]=4;
 future13.apply_mod[0]=13;future15.apply_mod[0]=15;
 target.next=&future13;future13.next=&future15;
 assert(build_superior_enhancement_plan(&item,&plan));
 assert(trace==native_step && plan.remaining_enhancements==1 && plan.materials[0].count==2 && plan.materials[1].count==2);
 assert((queries==std::vector<std::pair<int,int>>{{1,11},{4,13},{4,14}}));
 assert_plan({0},1,{{400045,2},{400049,2}});
 prepare_native(15);
 enhance_index_entry first_tie,last_tie;
 target.vnum=5;target.ival=5;target.wear_flags=0;
 first_tie.vnum=last_tie.vnum=1;first_tie.ival=2;last_tie.ival=4;
 first_tie.wear_flags=last_tie.wear_flags=0;target.next=&first_tie;first_tie.next=&last_tie;
 item.wear_flags=8;enhance_wear_skip_mask=8;
 assert(build_superior_enhancement_plan(&item,&plan));
 assert(trace==native_step && plan.materials[0].count==1 && plan.materials[1].count==1);
 assert_plan({0},1,{{400045,1},{400049,1}});
 prepare_native(16);target.wear_flags=0;item.wear_flags=8;
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert((trace==std::vector<std::string>{"eligible","base","prototype-read","prototype-cleanup","cap","find"}));
 assert_plan({},0,{});
 prepare_case(17,3);target.ival=0;
 enhance_stat_material_quantity_multiplier=static_cast<double>(INT_MAX-4)/4.0;
 assert(!build_superior_enhancement_plan(&item,&plan));
 expected=full_step;expected.insert(expected.end(),full_step.begin(),full_step.end()-1);
 assert(trace==expected);
 assert_plan({0},1,{{400045,INT_MAX-4}});
 prepare_native(18);target.apply_loc[0]=4;
 assert(build_superior_enhancement_plan(&item,&plan));
 assert(trace==native_step);
 assert((queries==std::vector<std::pair<int,int>>{{4,11},{4,11},{4,12}}));
 assert_plan({0},1,{{400045,1},{400049,1}});
 prepare_native(19);
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert((trace==std::vector<std::string>{"eligible","base","prototype-read","prototype-cleanup","cap","find","read"}));
 assert_plan({},0,{});
 prepare_native(20);prototype={};
 assert(!build_superior_enhancement_plan(&item,&plan));
 assert((trace==std::vector<std::string>{"eligible","base","prototype-read","prototype-cleanup","cap"}));
 assert_plan({},0,{});
 mode=-1;

}
'''
source=(SRC/'enhance.c').read_text()
functions=[]
if 'static bool scale_superior_material_count(' in source:
    functions.append(extract_function('enhance.c','static bool scale_superior_material_count('))
if 'static bool superior_plan_add_material(' in source:
    functions.append(extract_function('enhance.c','static bool superior_plan_add_material('))
else:
    functions.append('#define superior_plan_add_material enhancement_superior_plan_add_material')
functions.append(extract_function('enhance.c','static bool build_superior_enhancement_plan('))
native_functions=[]
for signature, renamed in [
    ('static int enhance_entry_modifier(',None),
    ('static int enhance_base_modifier(','baseline_enhance_base_modifier'),
    ('static int enhance_stat_cap(','baseline_enhance_stat_cap'),
    ('static struct enhance_index_entry *find_stat_enhance_target(','baseline_find_stat_enhance_target'),
    ('static int superior_stat_remaining_steps(','baseline_superior_stat_remaining_steps'),
]:
    function=extract_function('enhance.c',signature)
    if renamed:
        original=signature.split()[-1][:-1].lstrip('*')
        function=function.replace(original+'(',renamed+'(',1)
    native_functions.append(function)
cap_include=('#include "economy/enhancement_stat_rules.h"'
             if 'return enhancement_stat_cap(' in source else '')
with tempfile.TemporaryDirectory(prefix='duris-superior-material-') as temporary:
    cpp=Path(temporary)/'plan.cpp'; binary=Path(temporary)/'plan'
    cpp.write_text(HARNESS.replace('@FUNCTIONS@','\n'.join(functions))
                  .replace('@NATIVE_FUNCTIONS@','\n'.join(native_functions))
                  .replace('@CAP_INCLUDE@',cap_include))
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-O1','-g','-fsanitize=address,undefined,float-cast-overflow',
                    '-fno-sanitize-recover=all','-fno-pie','-no-pie','-Isrc',str(cpp),'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True,timeout=30)
print('superior material planning: original bounds, exact all-stat plans and native ordering passed')
