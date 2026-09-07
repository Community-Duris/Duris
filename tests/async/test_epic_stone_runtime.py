#!/usr/bin/env python3
"""Exercise the real zone transaction orchestrator with fault-controlled adapters."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "core/structs.h"
#include "core/utils.h"
#include "core/prototypes.h"
#include "world/zone_touch_transaction.h"
#include "world/epic.h"
#include "world/epic_transaction.h"
#include "guild/artifact_guild_transaction.h"
#include "persistence/persistence_mode.h"
#include <cassert>
#include <string>

static char_data characters[2] = {};
static pc_only_data pcs[2] = {};
static bool online[2] = {true,true};
static bool mysql_available = true;
static bool magic = true;
static int effects = 0, zone_publications = 0;
static critical_submit_result admission = critical_submit_result::accepted;
static critical_command submitted;
static std::string captured_messages;

bool persistence_mode_requires_mysql() { return mysql_available; }
critical_submit_result critical_command_coordinator_submit(critical_command command)
{ submitted = command; return admission; }
P_char find_player_by_pid(int pid)
{ return pid >= 1 && pid <= 2 && online[pid-1] ? &characters[pid-1] : nullptr; }
void send_to_char(const char *text, P_char) { captured_messages += text; }
void logit(const char *, const char *, ...) {}
bool redis_invalidate_epic_zones() { return true; }
bool epic_transaction_publish_balance(P_char ch, int64_t balance, uint64_t revision)
{ ch->only.pc->epics=balance; ch->only.pc->epic_revision=revision; return true; }
bool artifact_guild_transaction_submit(P_char, const critical_operation_id &, int, int)
{ return true; }
void epic_publish_zone_touch(const zone_touch_result &) { ++zone_publications; }
void epic_finish_stone_touch(const zone_touch_result &r)
{ magic=false; if(r.record_zone && !r.recovered_claim) ++zone_publications; }
void epic_publish_stone_award(P_char, const zone_touch_result &, size_t) { ++effects; }

static zone_touch_payload payload()
{
 zone_touch_payload p = {};
 p.zone_number=77; p.toucher_pid=1; p.group_size=2;
 p.participant_pids[0]=1; p.participant_pids[1]=2;
 p.stone_uid=99; p.awards[0]={10,77,0}; p.awards[1]={20,0,0};
 return p;
}
static critical_completion complete(critical_apply_outcome outcome, bool receipt=true)
{
 critical_completion c = {};
 c.operation_id=submitted.operation_id; c.outcome=outcome;
 if(receipt)
 {
  zone_touch_result r(payload()); r.balances[0]=110; r.balances[1]=120;
  r.revisions[0]=r.revisions[1]=1;
  std::array<uint8_t,ZONE_TOUCH_RESULT_BYTES> bytes;
  assert(zone_touch_command_encode_result(r,&bytes));
  std::copy(bytes.begin(),bytes.end(),c.result_payload.begin()); c.result_size=bytes.size();
 }
 return c;
}
int main()
{
 for(int i=0;i<2;++i) { characters[i].only.pc=&pcs[i]; pcs[i].pid=i+1; pcs[i].epics=100; }
 auto p=payload();
 mysql_available=false;
 assert(!zone_touch_transaction_submit(p));
 mysql_available=true;
 for(auto rejection : {critical_submit_result::overloaded, critical_submit_result::journal_failure})
 {
  admission=rejection; assert(!zone_touch_transaction_submit(p));
  assert(!zone_touch_transaction_busy(99,77) && magic && effects==0);
 }
 admission=critical_submit_result::accepted;
 assert(zone_touch_transaction_submit(p));
 assert(zone_touch_transaction_busy(99,77));
 assert(!zone_touch_transaction_submit(p));
 assert(magic && effects==0 && pcs[0].epics==100);
 auto failed=complete(critical_apply_outcome::terminal_failure,false);
 zone_touch_transaction_handle_completions(&failed,1);
 assert(!zone_touch_transaction_busy(99,77) && magic && effects==0);
 assert(zone_touch_transaction_submit(p));
 auto uncertain=complete(critical_apply_outcome::ambiguous_commit,false);
 zone_touch_transaction_handle_completions(&uncertain,1);
 assert(zone_touch_transaction_busy(99,77) && !zone_touch_transaction_submit(p));
 auto malformed=complete(critical_apply_outcome::applied,false);
 zone_touch_transaction_handle_completions(&malformed,1);
 assert(zone_touch_transaction_busy(99,77) && magic);
 online[1]=false;
 auto success=complete(critical_apply_outcome::applied);
 zone_touch_transaction_handle_completions(&success,1);
 assert(!magic && effects==1 && pcs[0].epics==110 && pcs[1].epics==100);
 assert(zone_publications==1 && !zone_touch_transaction_busy(99,77));
 zone_touch_transaction_handle_completions(&success,1);
 assert(effects==1 && zone_publications==1);
 online[1]=true;
 // Reconnect hydration must not be overwritten by an older award receipt.
 pcs[1].epics=200; pcs[1].epic_revision=3;
 zone_touch_transaction_player_ready(&characters[1]);
 assert(effects==2 && pcs[1].epics==200 && pcs[1].epic_revision==3);
 zone_touch_transaction_player_ready(&characters[1]);
 assert(effects==2);
 zone_touch_transaction_reset_for_tests();
 magic=true;
 assert(zone_touch_transaction_submit(p));
 auto recovered=complete(critical_apply_outcome::applied);
 zone_touch_result receipt;
 assert(zone_touch_command_decode_result(recovered.result_payload.data(),recovered.result_size,&receipt));
 receipt.recovered_claim=true;
 std::array<uint8_t,ZONE_TOUCH_RESULT_BYTES> bytes;
 assert(zone_touch_command_encode_result(receipt,&bytes));
 std::copy(bytes.begin(),bytes.end(),recovered.result_payload.begin());
 zone_touch_transaction_handle_completions(&recovered,1);
 assert(!magic && effects==2 && zone_publications==1);
}
'''

class StoneRuntimeTests(unittest.TestCase):
    def test_admission_failure_pending_ambiguity_success_disconnect_and_recovery(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'stone.cpp'
            binary = Path(directory) / 'stone'
            source.write_text(HARNESS)
            subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Isrc',
                str(source), 'src/world/zone_touch_transaction.c', 'src/world/zone_touch_command.c',
                'src/world/epic_command.c', 'src/persistence/critical_command.c',
                '-lcrypto', '-o', str(binary)], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)

if __name__ == '__main__':
    unittest.main()
