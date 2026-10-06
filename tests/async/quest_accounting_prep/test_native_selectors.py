#!/usr/bin/env python3
"""Execute the actual durable selector with production Q terms and isolated seams.

This proves selection/refusal only. Submission, credit capture and reward capture
are stubs; it grants no native SQL, accounting, birth or publication proof.
"""

import argparse
import json
from pathlib import Path
import subprocess
import tempfile

from case_data import CASES, ROOT, blocks, digest
from _paths import extract_function

PRELUDE = r'''
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
struct object { int vnum; uint64_t obj_uid; object *next_content=nullptr; bool durable=true; };
struct character { int pid=42, vnum=0, in_room=0; object *carrying=nullptr; };
using P_char=character*; using P_obj=object*;
struct goal_data { int goal_type; int number; goal_data *next=nullptr; };
struct quest_complete_data { goal_data *give=nullptr, *receive=nullptr; quest_complete_data *next=nullptr; };
struct quest_data { quest_complete_data *quest_complete=nullptr; };
quest_data quest_index[1];
constexpr int QUEST_GOAL_ITEM=1, QUEST_GOAL_COINS=3, QUEST_GOAL_SKILL=4, QUEST_GOAL_EXP=5;
constexpr size_t QUEST_DURABLE_MAX_OFFERINGS=14;
struct quest_durable_context {
 int quester_id=0,completion_index=0,room=0; uint32_t count=0;
 uint64_t roots[14]={},completed_at=0;
};
struct item_transfer_continuation {};
enum class item_owner_type { player, destruction };
struct item_owner_identity { item_owner_type type; uint64_t id,context_id; };
enum class item_transfer_reason { quest_turnin };
enum class item_movement_reject { none };
enum class economic_source_kind { intentional_destruction };
namespace economic_gameplay_authority { bool active() { return true; } }
#define OBJ_VNUM(obj) ((obj)->vnum)
#define GET_VNUM(ch) ((ch)->vnum)
#define GET_PID(ch) ((ch)->pid)
constexpr int LOG_DEBUG=1;
bool capture_allowed=true, submission_allowed=true;
bool item_command_uses_durable_ownership(P_obj obj) { return obj && obj->durable; }
bool capture_quest_credit_context(P_char,quest_durable_context*) { return capture_allowed; }
bool capture_quest_offering_continuation(P_char,P_char,int,int,const quest_complete_data*,
                                       quest_durable_context&,item_transfer_continuation*) { return capture_allowed; }
void complete_quest_offering() {}
void publish_quest_offering() {}
int submissions=0; std::vector<uint64_t> selected;
quest_durable_context captured;
bool item_movement_transaction_submit_batch(P_char,P_obj *roots,size_t count,void*,
 const item_owner_identity&,const item_owner_identity&,item_transfer_reason,int,
 void(*)(),const void *context,size_t size,void*,item_movement_reject*,void(*)(),
 economic_source_kind,int,const item_transfer_continuation&) {
 assert(size==sizeof(captured));
 if (!submission_allowed) return false;
 memcpy(&captured,context,size); ++submissions;
 selected.clear(); for(size_t i=0;i<count;++i) selected.push_back(roots[i]->obj_uid);
 return true;
}
std::string last_message;
void send_to_char(const char *message,P_char) { last_message=message; }
void logit(int,const char*,...) {}
const char *item_movement_reject_name(item_movement_reject) { return "none"; }
'''

POSTLUDE = r'''
struct recipe {
 std::vector<goal_data> give,receive; quest_complete_data completion;
 recipe(std::vector<goal_data> g,std::vector<goal_data> r):give(g),receive(r) {
  for(size_t i=0;i<give.size();++i) give[i].next=i+1<give.size()?&give[i+1]:nullptr;
  for(size_t i=0;i<receive.size();++i) receive[i].next=i+1<receive.size()?&receive[i+1]:nullptr;
  completion.give=give.empty()?nullptr:&give[0]; completion.receive=receive.empty()?nullptr:&receive[0];
 }
};
character actor,mob;
void set_inventory(std::vector<object>& inventory) {
 for(size_t i=0;i<inventory.size();++i) inventory[i].next_content=i+1<inventory.size()?&inventory[i+1]:nullptr;
 actor.carrying=inventory.empty()?nullptr:&inventory[0];
 submissions=0; selected.clear(); last_message.clear(); capture_allowed=submission_allowed=true;
}
'''


def goals(values):
    types = {"I": "QUEST_GOAL_ITEM", "C": "QUEST_GOAL_COINS", "E": "QUEST_GOAL_EXP", "S": "QUEST_GOAL_SKILL"}
    return "{" + ",".join("{" + types[kind] + f",{number},nullptr" + "}" for kind, number in reversed(values)) + "}"


def inventory(vnums):
    return "{" + ",".join("{" + f"{vnum},{100 + index},nullptr,true" + "}" for index, vnum in enumerate(vnums)) + "}"


def main_body(case_id, acceptance):
    selected = blocks(case_id)
    lines = ["int main() {", f"mob.vnum={CASES[case_id]['giver']};"]
    for index, block in enumerate(selected):
        lines.append(f"recipe r{index}({goals(block['give'])},{goals(block['receive'])});")
        lines.append(f"r{index}.completion.next=" + (f"&r{index-1}.completion;" if index else "nullptr;"))
    lines.append(f"quest_index[0].quest_complete=&r{len(selected)-1}.completion;")
    if case_id == "QP02":
        lines += [f"std::vector<object> items={inventory([19006] * 3)}; set_inventory(items);",
                  "assert(submit_durable_quest_offering(&mob,&actor,0,&items[0]));"]
        if acceptance:
            lines += ["if(submissions!=1 || captured.completion_index!=1 || selected.size()!=3) return 30;"]
        else:
            lines += ["assert(submissions==0 && selected.empty());",
                      'assert(last_message.find("cannot accept")!=std::string::npos);']
        lines += ["for(size_t i=0;i<items.size();++i) assert(items[i].obj_uid==100+i);",
                  "for(int count:{1,2,4}) {",
                  "std::vector<object> paid; for(int i=0;i<count;++i) paid.push_back({19006,static_cast<uint64_t>(200+i),nullptr,true});",
                  "set_inventory(paid); assert(submit_durable_quest_offering(&mob,&actor,0,&paid[0]));",
                  "assert(submissions==0 && selected.empty()); }" ]
    else:
        # Test one selected contract at a time, keeping native goal order. QP05
        # has both the ordinary triple skin and the supplied two-huge-skin contract.
        for index, block in enumerate(selected):
            inputs = [number for kind, number in block["give"] if kind == "I"]
            lines += [f"quest_index[0].quest_complete=&r{index}.completion; r{index}.completion.next=nullptr;",
                      "{", f"std::vector<object> items={inventory(inputs + [inputs[0]])}; set_inventory(items);",
                      "assert(submit_durable_quest_offering(&mob,&actor,0,&items[0]));",
                      f"assert(submissions==1 && selected.size()=={len(inputs)} && captured.count=={len(inputs)});",
                      "for(size_t i=0;i<selected.size();++i) { assert(captured.roots[i]==selected[i]);",
                      "for(size_t j=0;j<i;++j) assert(selected[i]!=selected[j]); }",
                      f"for(uint64_t uid:selected) assert(uid!={100+len(inputs)});",
                      "assert(items.back().obj_uid==" + str(100+len(inputs)) + ");",
                      "capture_allowed=false; submissions=0;",
                      "assert(submit_durable_quest_offering(&mob,&actor,0,&items[0])); assert(submissions==0);",
                      "capture_allowed=true; submission_allowed=false;",
                      "assert(submit_durable_quest_offering(&mob,&actor,0,&items[0])); assert(submissions==0);",
                      "}", "{", f"std::vector<object> short_items={inventory(inputs[:-1])}; set_inventory(short_items);",
                      "assert(submit_durable_quest_offering(&mob,&actor,0,&short_items[0])); assert(submissions==0);", "}"]
            if case_id == "QP01":
                lines += ["{", f"std::vector<object> wrong={inventory([43703,43703,43703,44164])}; set_inventory(wrong);",
                          "assert(submit_durable_quest_offering(&mob,&actor,0,&wrong[0])); assert(submissions==0);", "}"]
            lines += ["{", f"std::vector<object> held={inventory(inputs)}; held.back().durable=false; set_inventory(held);",
                      "assert(submit_durable_quest_offering(&mob,&actor,0,&held[0])); assert(submissions==0);", "}"]
    return "\n".join(lines + ["}"])


def run(case_id, acceptance=False):
    function = extract_function("world/quest.c", "static bool submit_durable_quest_offering(")
    program = PRELUDE + "\n" + function + "\n" + POSTLUDE + "\n" + main_body(case_id, acceptance)
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="quest-prep-selector-", dir=build_root) as directory:
        cpp, executable = Path(directory) / "selector.cpp", Path(directory) / "selector"
        cpp.write_text(program)
        subprocess.run(["g++", "-std=c++20", "-O0", "-Wall", "-Wextra", "-Werror",
                        str(cpp), "-o", str(executable)], check=True)
        result = subprocess.run([str(executable)], check=False)
        if result.returncode:
            raise AssertionError(f"{case_id}: selector failed code {result.returncode}; "
                                 "QP02 code30 means supported backpack still unreachable")
    return dict(case=case_id, mode="acceptance" if acceptance else "pinned behavior",
                result="component passed; submission/reward/custody authority stubbed",
                source_sha256=digest(ROOT / "src/world/quest.c"))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=("QP01", "QP02", "QP05", "QP06"))
    parser.add_argument("--acceptance", action="store_true", help="QP02 requires reachable supported backpack; RED on prep pin")
    args = parser.parse_args()
    print(json.dumps([run(k, args.acceptance) for k in
                      ([args.case] if args.case else ("QP01", "QP02", "QP05", "QP06"))], indent=2))
