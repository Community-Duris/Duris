#!/usr/bin/env python3
"""Observe actual creation callback/reset with an injected settlement context.

This proves callback behavior only. Debit, creation producer, history and SQL
boundaries are stubbed. It does not prove a reachable native stale-payment race.
See CREATION_WATERMARK_REVIEW.md for command/configuration reachability.
"""

import json
from pathlib import Path
import subprocess
import tempfile

from case_data import ROOT, digest
from _paths import extract_function
from test_bartender_settlement import PRELUDE


def run():
    source = (ROOT / "src/specs/specs.world_quest.c").read_text()
    start = source.index("enum class world_quest_payment_action")
    end = source.index("static_assert(sizeof(world_quest_payment_context)", start)
    dispatch = extract_function("specs/specs.world_quest.c", "int world_quest(")
    request = dispatch[dispatch.index("world_quest_payment_action::quest,"):]
    assert "pl->only.pc->quest_started" in request.split("};", 1)[0]
    # Execute the production reset, retaining the shared boundary stubs for all
    # economic effects and the creation producer. No persisted state is seeded.
    prelude = PRELUDE.replace("int quest_map_bought=0,", "int quest_zone_number=0,quest_giver=0,"
        "quest_level=0,quest_receiver=0,quest_shares_left=0,quest_kill_original=0,"
        "quest_map_room=0;\n int quest_map_bought=0,")
    reset_start = prelude.index("void resetQuest(P_char pl)")
    reset_end = prelude.index("void quest_buy_map", reset_start)
    prelude = prelude[:reset_start] + extract_function(
        "world/world_quest.c", "void resetQuest(") + "\n" + prelude[reset_end:]
    functions = "\n".join([
        extract_function("core/utility.c", "void ADD_MONEY("),
        extract_function("specs/specs.world_quest.c", "static void world_quest_report_creation_failure("),
        extract_function("specs/specs.world_quest.c", "static void world_quest_refund_payment("),
        extract_function("specs/specs.world_quest.c", "static void world_quest_payment_committed("),
    ])
    main = r'''
int main() {
 pc_data state; character actor; actor.only.pc=&state;
 creation_success=true;
 world_quest_payment_context payment={world_quest_payment_action::quest,220,1709,100};
 auto settle=[&](bool committed) {
  world_quest_payment_committed(&actor,committed,{},0,
   reinterpret_cast<const uint8_t*>(&payment),sizeof(payment));
 };
 static_assert(sizeof(payment)<=CURRENCY_PENDING_CONTEXT_MAX_BYTES);
 // Original inactive request remains valid.
 settle(true); assert(create_calls==1 && credit_requests.empty());
 // Inject the review premise: B advanced the persisted watermark, then reset.
 // The real reset retains B's watermark; the actual callback still calls creation.
 state.quest_active=1; state.quest_accomplished=1; state.quest_started=101;
 resetQuest(&actor);
 assert(state.quest_started==101 && !state.quest_active && !state.quest_accomplished);
 settle(true); assert(create_calls==2 && state.quest_started==101 && credit_requests.empty());
 // Reset alone does not advance identity and preserves legitimate creation.
 state.quest_started=100; state.quest_active=1; resetQuest(&actor);
 settle(true); assert(create_calls==3 && credit_requests.empty());
 // A new player may legitimately have no previous attempt watermark.
 state.quest_started=0; payment.quest_started=0; settle(true); assert(create_calls==4);
 payment.quest_started=100; state.quest_started=101;
 // Existing active/completed/quota guards still prevent producer dispatch.
 for(int guard=0;guard<3;++guard) {
  state.quest_active=guard==0;state.quest_accomplished=guard==1;quota=guard==2?0:1;
  settle(true); assert(create_calls==4 && credit_requests.back()==220);
 }
 state.quest_active=state.quest_accomplished=0;quota=1;
 const auto credits=credit_requests.size();
 settle(false);
 world_quest_payment_committed(&actor,true,{},0,nullptr,0);
 world_quest_payment_committed(nullptr,true,{},0,
  reinterpret_cast<const uint8_t*>(&payment),sizeof(payment));
 world_quest_payment_committed(&actor,true,{},0,
  reinterpret_cast<const uint8_t*>(&payment),sizeof(payment)-1);
 payment.quest_started=-1;settle(true);payment.quest_started=100;
 payment.fee=0;settle(true);payment.fee=220;
 payment.giver_vnum=0;settle(true);payment.giver_vnum=1709;
 payment.action=static_cast<world_quest_payment_action>(255);settle(true);
 assert(create_calls==4 && credit_requests.size()==credits);
}
'''
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="quest-prep-creation-", dir=build_root) as directory:
        cpp, executable = Path(directory) / "creation.cpp", Path(directory) / "creation"
        cpp.write_text(prelude + source[start:end] + functions + main)
        subprocess.run(["g++", "-std=c++20", "-O0", "-Wall", "-Wextra", "-Werror",
                        str(cpp), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    return dict(result="PASS: actual callback dispatches creation after injected advanced/reset state",
        authority="component observation; hypothetical debit; producer/history/SQL stubbed",
        native_reachability="unqualified; default sharing disabled; configured cross-actor path needs proof",
        source_hashes={path: digest(ROOT / path) for path in
            ("src/specs/specs.world_quest.c", "src/world/world_quest.c", "src/core/utility.c",
             "src/cmd/interp.c", "src/net/comm.c", "lib/duris.properties")})


if __name__ == "__main__":
    print(json.dumps(run(), indent=2))
