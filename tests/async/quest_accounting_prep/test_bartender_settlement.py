#!/usr/bin/env python3
"""Execute actual bartender settlement and ADD_MONEY with isolated boundaries.

Callbacks are injected after a hypothetical committed debit. No SQL debit,
activation, refund commit or native quest persistence is claimed by this test.
"""

import argparse
import json
from pathlib import Path
import subprocess
import tempfile

from case_data import ACCOUNTING_PIN, ROOT, digest
from _paths import extract_function

PRELUDE = r'''
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
struct pc_data {
 int quest_active=0,quest_accomplished=0,quest_type=0,quest_kill_how_many=0;
 int quest_map_bought=0,quest_started=100,quest_mob_vnum=50;
};
struct character {
 struct { pc_data *pc=nullptr; } only;
 int pid=42,level=11; bool npc=false;
 int copper=0,silver=0,gold=0,platinum=0;
};
using P_char=character*;
#define GET_PID(ch) ((ch)->pid)
#define GET_LEVEL(ch) ((ch)->level)
#define IS_PC(ch) (!(ch)->npc)
#define GET_PLATINUM(ch) ((ch)->platinum)
#define GET_GOLD(ch) ((ch)->gold)
#define GET_SILVER(ch) ((ch)->silver)
#define GET_COPPER(ch) ((ch)->copper)
constexpr int MAXLVLMORTAL=50, LOG_WIZ=1,LOG_DEBUG=2,LOG_EXIT=3,FIND_AND_KILL=1,PLAYER_COMPONENT_STATUS=1;
constexpr size_t CURRENCY_PENDING_CONTEXT_MAX_BYTES=128;
struct currency_command_result {};
enum class currency_reason_type { wallet_reward };
enum class critical_source_site { command };
enum class critical_deadline_class { interactive };
using currency_completion_fn=void(*)(P_char,bool,const currency_command_result&,unsigned int,const uint8_t*,size_t);
enum quest_creation_failure { QUEST_CREATION_NO_FAILURE,QUEST_CREATION_NO_ELIGIBLE_ZONE,
                             QUEST_CREATION_NO_ELIGIBLE_TARGET };
bool active=false,creation_success=false;
namespace economic_gameplay_authority { bool active() { return ::active; } }
quest_creation_failure creation_failure=QUEST_CREATION_NO_ELIGIBLE_TARGET;
int create_calls=0,map_calls=0,reset_calls=0,finished_calls=0,gmcp_calls=0,quota=1;
int observed_start=0,observed_target=0;
std::vector<int64_t> credit_requests;
std::string messages;
void send_to_char(const char *message,P_char) { messages+=message; }
void logit(int,const char*,...) {}
void gmcp_char_vitals(P_char) {}
void mark_player_dirty_components(int,int) {}
void currency_adjustment_committed(P_char,bool,const currency_command_result&,unsigned int,const uint8_t*,size_t) {}
bool currency_transaction_submit_wallet_value(P_char,int64_t amount,currency_reason_type,int,
 critical_source_site,critical_deadline_class,currency_completion_fn,const void*,size_t) {
 credit_requests.push_back(amount); return true;
}
bool insert_money_pickup(int,int) { return false; }
int sql_world_quest_can_do_another(P_char) { return quota; }
void gmcp_quest_status(P_char) { ++gmcp_calls; }
void sql_world_quest_finished(P_char,void*) { ++finished_calls; }
void resetQuest(P_char pl) {
 ++reset_calls; observed_start=pl->only.pc->quest_started; observed_target=pl->only.pc->quest_mob_vnum;
 pl->only.pc->quest_active=0;
}
void quest_buy_map(P_char pl) {
 ++map_calls; observed_start=pl->only.pc->quest_started; observed_target=pl->only.pc->quest_mob_vnum;
 pl->only.pc->quest_map_bought=1;
}
bool createQuestForGiverVnum(P_char,int giver,quest_creation_failure *failure) {
 assert(giver==1709); ++create_calls; *failure=creation_failure; return creation_success;
}
char *writable_arg(const char *text) { return const_cast<char*>(text); }
void do_quest(P_char,char*,int) {}
void ADD_MONEY(P_char,int,const char *committed_message=nullptr);
'''


def run(case_id, acceptance=False):
    source = (ROOT / "src/specs/specs.world_quest.c").read_text()
    start = source.index("enum class world_quest_payment_action")
    end = source.index("static_assert(sizeof(world_quest_payment_context)", start)
    context = source[start:end]
    functions = "\n".join([
        extract_function("core/utility.c", "void ADD_MONEY("),
        extract_function("specs/specs.world_quest.c", "static void world_quest_report_creation_failure("),
        extract_function("specs/specs.world_quest.c", "static void world_quest_refund_payment("),
        extract_function("specs/specs.world_quest.c", "static void world_quest_payment_committed("),
    ])
    common = r'''
int main() {
 pc_data state; character actor; actor.only.pc=&state;
 world_quest_payment_context payment={world_quest_payment_action::quest,220,1709};
 auto settle=[&](bool committed) {
  world_quest_payment_committed(&actor,committed,{},0,
   reinterpret_cast<const uint8_t*>(&payment),sizeof(payment));
 };
 settle(false); assert(create_calls==0 && credit_requests.empty() && map_calls==0 && reset_calls==0);
 world_quest_payment_committed(&actor,true,{},0,nullptr,0);
 assert(create_calls==0 && credit_requests.empty());
'''
    if case_id == "QP04":
        body = r'''
 // Inactive supported seam: a failed creation requests exactly the charged fee.
 for(auto failure:{QUEST_CREATION_NO_ELIGIBLE_ZONE,QUEST_CREATION_NO_ELIGIBLE_TARGET}) {
  creation_failure=failure; settle(true);
  assert(credit_requests.back()==220 && state.quest_active==0 && reset_calls==0 && finished_calls==0);
 }
 assert(create_calls==2 && credit_requests.size()==2);
 state.quest_active=1; settle(true); assert(create_calls==2 && credit_requests.back()==220);
 state.quest_active=0; active=true; messages.clear();
 const auto credits_before=credit_requests.size(); settle(true);
 assert(create_calls==3 && state.quest_active==0 && finished_calls==0);
'''
        body += ("if(credit_requests.size()!=credits_before+1) return 30;\n" if acceptance else r'''
 assert(credit_requests.size()==credits_before);
 assert(messages.find("You get your money back")!=std::string::npos);
 assert(messages.find("coin credit could not be processed")!=std::string::npos);
''')
    else:
        body = r'''
 // Admit conceptually for A(start100,target50), settle after B replaces it.
 // Current context contains no A fields, so identical boolean readiness passes.
 state.quest_active=1; state.quest_started=200; state.quest_mob_vnum=60;
 payment={world_quest_payment_action::map,110,1709}; settle(true);
'''
        body += ("if(map_calls!=0 || state.quest_map_bought!=0) return 31;\n" if acceptance else r'''
 assert(map_calls==1 && state.quest_map_bought==1 && observed_start==200 && observed_target==60);
 // Repeat of a settled map is refused by map_bought, but that flag does not
 // distinguish a replacement task before the first completion.
 settle(true); assert(map_calls==1 && credit_requests.back()==110);
 payment={world_quest_payment_action::abandon,1331,1709};
 state.quest_map_bought=0; state.quest_active=1;
 state.quest_type=FIND_AND_KILL; state.quest_kill_how_many=1; settle(true);
 assert(reset_calls==1 && finished_calls==1 && state.quest_active==0);
 assert(observed_start==200 && observed_target==60);
''')
    program = PRELUDE + context + functions + common + body + "}\n"
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="quest-prep-bartender-", dir=build_root) as directory:
        cpp, executable = Path(directory) / "bartender.cpp", Path(directory) / "bartender"
        cpp.write_text(program)
        subprocess.run(["g++", "-std=c++20", "-O0", "-Wall", "-Wextra", "-Werror",
                        str(cpp), "-o", str(executable)], check=True)
        result = subprocess.run([str(executable)], check=False)
        if result.returncode:
            raise AssertionError(f"{case_id}: component failed code {result.returncode}; "
                                 "30=active refund not submitted; 31=replacement task received stale map")
    return dict(case=case_id, accounting_pin=ACCOUNTING_PIN,
                mode="acceptance" if acceptance else "current observation",
                owner="world_quest_payment_committed -> world_quest_refund_payment/ADD_MONEY",
                result="component passed; debit/SQL/quest persistence boundaries stubbed",
                source_hashes={str(p.relative_to(ROOT)): digest(p) for p in
                               (ROOT / "src/specs/specs.world_quest.c", ROOT / "src/core/utility.c")})


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=("QP04", "QP07"))
    parser.add_argument("--acceptance", action="store_true", help="local acceptance assertions are RED on prep pin; native proof still required")
    args = parser.parse_args()
    print(json.dumps([run(k, args.acceptance) for k in ([args.case] if args.case else ("QP04", "QP07"))], indent=2))
