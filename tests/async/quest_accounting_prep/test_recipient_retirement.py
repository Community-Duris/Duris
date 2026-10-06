#!/usr/bin/env python3
"""Observe original recipient lookup and D cleanup; native retirement is stubbed."""

import argparse
import json
from pathlib import Path
import subprocess
import tempfile

from case_data import ROOT, digest
from _paths import extract_function

PRELUDE = r'''
#include <cassert>
#include <cstdint>
#include <ctime>
#include <string>
#include <string_view>
#include <vector>
struct object { uint64_t uid; int vnum; object *next_content=nullptr; };
struct character {
 bool npc=true; int pid=0,rnum=16006,in_room=0;
 object *carrying=nullptr; object *equipment[4]={}; character *next=nullptr;
};
using P_char=character*; using P_obj=object*;
struct quest_durable_context { int quester_id=0,room=0; };
struct quest_data { int quester=16006; };
struct quest_complete_data {
 bool disappear=true,echoAll=false; const char *disappear_message="gone";
};
int number_of_quests=1; quest_data quest_index[1];
struct room { int number=16077; };
room rooms[1]; room *world=rooms;
P_char character_list=nullptr,cleanup_actor=nullptr;
#define IS_NPC(ch) ((ch)->npc)
#define GET_RNUM(ch) ((ch)->rnum)
constexpr int MAX_WEAR=4,TRUE=1,FALSE=0,TO_ROOM=1,TO_VICT=2,LOG_DEBUG=1;
std::vector<uint64_t> retired; std::vector<std::string> effects;
namespace zone_story_quest_runtime {
 bool record_legacy_completion(P_char,const quest_complete_data*,int room_number,int64_t,std::string*,std::string_view) {
  assert(room_number==16077); effects.push_back("tracking"); return true;
 }
}
void logit(int,const char*,...) {}
void give_reward(quest_complete_data*,P_char,P_char,uint64_t uid) {
 assert(uid==99); effects.push_back("reward");
}
void act(const char*,int,P_char,void*,P_char,int) { effects.push_back("disappear"); }
P_obj unequip_char(P_char mob,int slot) { auto *obj=mob->equipment[slot]; mob->equipment[slot]=nullptr; return obj; }
void extract_obj(P_obj obj,int) {
 retired.push_back(obj->uid); effects.push_back("stock");
 if(cleanup_actor && cleanup_actor->carrying==obj) cleanup_actor->carrying=obj->next_content;
}
void extract_char(P_char mob) { assert(mob==cleanup_actor); effects.push_back("actor"); }
'''


def run(acceptance=False):
    functions = "\n".join([
        extract_function("world/quest.c", "static P_char quest_mobile_for("),
        extract_function("world/quest.c", "static void finish_quest_reward("),
    ])
    body = r'''
int main() {
 quest_durable_context context; character original,player,replacement;
 player.npc=false; player.pid=42;
 character_list=&original; assert(quest_mobile_for(context)==&original);
 original.in_room=1; assert(quest_mobile_for(context)==nullptr);
 original.in_room=0; character_list=nullptr; assert(quest_mobile_for(context)==nullptr);
 character_list=&replacement; object new_lance{201,16016}; replacement.carrying=&new_lance;
'''
    body += ("if(quest_mobile_for(context)!=nullptr) return 32;\n" if acceptance else
             "assert(quest_mobile_for(context)==&replacement);\n")
    body += r'''
 // D cleanup extracts residual stock even if a stock VNUM is also a reward.
 object lance{101,16016},scale{102,16015}; lance.next_content=&scale;
 original.carrying=&lance; cleanup_actor=&original;
 quest_complete_data completion;
 finish_quest_reward(&completion,&original,&player,99,100,16077);
 assert((retired==std::vector<uint64_t>{101,102}));
 assert((effects==std::vector<std::string>{"tracking","reward","disappear","stock","stock","actor"}));
 assert(original.carrying==nullptr);
 // Replacement's independent stock survives this original actor's cleanup.
 assert(replacement.carrying->uid==201);
}
'''
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="quest-prep-retirement-", dir=build_root) as directory:
        cpp, executable = Path(directory) / "recipient.cpp", Path(directory) / "recipient"
        cpp.write_text(PRELUDE + functions + body)
        subprocess.run(["g++", "-std=c++20", "-O0", "-Wall", "-Wextra", "-Werror",
                        str(cpp), "-o", str(executable)], check=True)
        result = subprocess.run([str(executable)], check=False)
        if result.returncode:
            raise AssertionError(f"QP03: component failed code {result.returncode}; "
                                 "32=prototype/room lookup adopts replacement incarnation")
    return dict(case="QP03", mode="acceptance" if acceptance else "pinned behavior",
                result="component passed; rewards/tracking/extraction/native events stubbed",
                source_sha256=digest(ROOT / "src/world/quest.c"))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--acceptance", action="store_true", help="original incarnation required; RED on prep pin")
    args = parser.parse_args()
    print(json.dumps(run(args.acceptance), indent=2))
