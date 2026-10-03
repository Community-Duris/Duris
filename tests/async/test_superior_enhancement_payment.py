#!/usr/bin/env python3
"""Run the production payment function with overflowing and ordinary quotes."""
from _paths import ROOT, extract_function
import os
from pathlib import Path
import subprocess
import tempfile


HARNESS = r'''
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
constexpr int MAX_STRING_LENGTH = 4096, MAX_SUPERIOR_MATERIALS = 32;
constexpr bool TRUE = true, FALSE = false;
constexpr int NOWHERE = -1;
struct character { struct { int level = 50; } player; int in_room = NOWHERE; int64_t money; };
struct object { struct { int modifier = 10; } affected[1]; const char *short_description = "fixture"; };
using P_char = character *;
using P_obj = object *;
struct room { int number; };
room world[1] = {};
struct superior_enhancement_plan {
 int slots[1] = {0}; int slot_count = 1;
 struct { int vnum = 400049; int count = 3; } materials[1]; int material_count = 1;
};
struct chaos_material_pouch_usage { int vnum; uint64_t count; };
int enhance_stat_platinum_base, enhance_stat_platinum_per_ival, item_value;
int debits, debit_attempts, consumed, marked, generated;
bool reject_debit = false;
int itemvalue(P_obj) { return item_value; }
#define GET_MONEY(ch) ((ch)->money)
#define GET_NAME(ch) "fixture"
int SUB_MONEY(P_char ch, int cost, int) {
 ++debit_attempts;
 if (reject_debit || cost <= 0 || ch->money < cost) return -1;
 ch->money -= cost; ++debits; return 0;
}
bool superior_plan_has_materials(P_char, P_obj, const superior_enhancement_plan *) { return true; }
void vnum_from_inv(P_char, int vnum, int count) { assert(vnum == 400049); consumed += count; }
void mark_item_superior(P_obj) { ++marked; }
bool chaos_material_pouch_can_record_generated(P_char, const chaos_material_pouch_usage *, int) { return true; }
bool chaos_material_pouch_record_generated(P_char, const chaos_material_pouch_usage *usage, int count) {
 assert(count == 1 && usage[0].vnum == 400049); generated += usage[0].count; return true;
}
void chaos_material_pouch_report_generated_failure(P_char, const char *) { assert(false); }
void send_to_char(const char *, P_char) {}
void statuslog(int, const char *, ...) {}
@FUNCTION@
int main() {
 superior_enhancement_plan plan;
 struct quote { int base, per_value, value; };
 for (bool pouch_mode : {false, true}) {
  enhance_stat_platinum_base=5000; enhance_stat_platinum_per_ival=0;
  character actor; actor.money=10000; object item, pouch;
  debits=debit_attempts=consumed=marked=generated=0; reject_debit=true;
  assert(!perform_superior_enhancement(&actor,&item,pouch_mode ? &pouch : nullptr,&plan));
  assert(actor.money==10000 && debit_attempts==1 && !debits && !consumed && !marked && !generated);
  assert(item.affected[0].modifier==10);
  reject_debit=false;
 }

 for (bool pouch_mode : {false, true}) {
  for (const auto quote : {quote{1, INT_MAX, 2}, quote{INT_MAX, 1, 1},
                          quote{0, -1, 1}, quote{INT_MIN, INT_MIN, INT_MIN}}) {
   enhance_stat_platinum_base=quote.base; enhance_stat_platinum_per_ival=quote.per_value; item_value=quote.value;
   character actor; actor.money=INT_MAX; object item, pouch;
   debits=debit_attempts=consumed=marked=generated=0;
   assert(!perform_superior_enhancement(&actor,&item,pouch_mode ? &pouch : nullptr,&plan));
   assert(actor.money==INT_MAX && !debits && !consumed && !marked && !generated);
   assert(item.affected[0].modifier==10);
  }
  for (const auto quote : {quote{1000,100,5}, quote{INT_MAX,0,0}, quote{0,0,5}}) {
   enhance_stat_platinum_base=quote.base; enhance_stat_platinum_per_ival=quote.per_value; item_value=quote.value;
   const int64_t expected=static_cast<int64_t>(quote.base)+static_cast<int64_t>(quote.per_value)*quote.value;
   character actor; actor.money=INT_MAX; object item, pouch;
   debits=debit_attempts=consumed=marked=generated=0;
   assert(perform_superior_enhancement(&actor,&item,pouch_mode ? &pouch : nullptr,&plan));
   assert(actor.money==INT_MAX-expected && debits==(expected > 0 ? 1 : 0) && debit_attempts==(expected > 0 ? 1 : 0) && marked==1);
   assert(consumed==(pouch_mode ? 0 : 3) && generated==(pouch_mode ? 3 : 0));
   assert(item.affected[0].modifier==11);
  }
 }
 enhance_stat_platinum_base=5000; enhance_stat_platinum_per_ival=0;
 character actor; actor.money=4000; object item; debits=debit_attempts=consumed=marked=generated=0;
 assert(!perform_superior_enhancement(&actor,&item,nullptr,&plan));
 assert(actor.money==4000 && !debits && !consumed && !marked && !generated);
}
'''

with tempfile.TemporaryDirectory(prefix="duris-enhancement-payment-") as temporary:
    cpp = Path(temporary) / "payment.cpp"
    binary = Path(temporary) / "payment"
    cpp.write_text(HARNESS.replace("@FUNCTION@", extract_function(
        "enhance.c", "static bool perform_superior_enhancement(")))
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra",
                    "-Werror", "-O1", "-g", "-fsanitize=address,undefined",
                    "-fno-sanitize-recover=undefined", "-fno-pie", "-no-pie",
                    str(cpp), "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
print("superior enhancement payment: overflow/negative refusal, exact debit, pouch and physical materials passed")
