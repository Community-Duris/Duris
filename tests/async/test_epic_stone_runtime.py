#!/usr/bin/env python3
"""Exercise the real zone transaction orchestrator with fault-controlled adapters."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from _paths import extract_function

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
#include <vector>

static char_data characters[2] = {};
static pc_only_data pcs[2] = {};
static bool online[2] = {true,true};
static bool mysql_available = true;
static bool magic = true;
static int effects = 0, zone_publications = 0;
static critical_submit_result admission = critical_submit_result::accepted;
static critical_command submitted;
static std::string captured_messages;
struct artifact_publication
{
 uint32_t pid;
 critical_operation_id operation;
 int amount, type;
};
static std::vector<artifact_publication> artifacts;

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
bool artifact_guild_transaction_submit(P_char ch, const critical_operation_id &operation,
                                       int amount, int type)
{ artifacts.push_back({static_cast<uint32_t>(ch->only.pc->pid),operation,amount,type}); return true; }
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
  zone_touch_payload decoded;
  assert(zone_touch_command_decode_payload(submitted,&decoded));
  zone_touch_result r(decoded); r.balances[0]=110; r.balances[1]=120;
  r.revisions[0]=r.revisions[1]=1;
  std::array<uint8_t,ZONE_TOUCH_RESULT_BYTES> bytes;
  assert(zone_touch_command_encode_result(r,&bytes));
  std::copy(bytes.begin(),bytes.end(),c.result_payload.begin()); c.result_size=bytes.size();
 }
 return c;
}
static void assert_artifact(size_t index, size_t participant)
{
 critical_command child;
 assert(zone_touch_award_command(submitted,participant,&child));
 assert(artifacts[index].pid==participant+1);
 assert(artifacts[index].operation.bytes==child.operation_id.bytes);
 assert(artifacts[index].operation.bytes!=submitted.operation_id.bytes);
 assert(artifacts[index].amount==(participant==0 ? 10 : 20));
 assert(artifacts[index].type==EPIC_ZONE);
}
static void test_pending_admission_excludes_committed_offline_receipts()
{
 zone_touch_transaction_reset_for_tests();
 artifacts.clear();
 online[0]=online[1]=false;
 // More than the entire admission budget may be waiting only for reconnect.
 for(size_t i=0;i<ZONE_TOUCH_PENDING_MAX+6;++i)
 {
  auto p=payload(); p.stone_uid=1000+i;
  assert(zone_touch_transaction_submit(p));
  auto committed=complete(critical_apply_outcome::applied);
  zone_touch_transaction_handle_completions(&committed,1);
  assert(!zone_touch_transaction_busy(p.stone_uid,p.zone_number));
 }
 assert(artifacts.empty());
 critical_completion first_pending_failure;
 // Those committed receipts must leave all 64 actual transaction slots free.
 for(size_t i=0;i<ZONE_TOUCH_PENDING_MAX;++i)
 {
  auto p=payload(); p.stone_uid=2000+i; p.zone_number=2000+i;
  assert(zone_touch_transaction_submit(p));
  assert(zone_touch_transaction_busy(p.stone_uid,p.zone_number));
  if(i==0) first_pending_failure=complete(critical_apply_outcome::terminal_failure,false);
 }
 auto extra=payload(); extra.stone_uid=3000; extra.zone_number=3000;
 const auto prior_operation=submitted.operation_id;
 assert(!zone_touch_transaction_submit(extra));
 assert(submitted.operation_id.bytes==prior_operation.bytes);
 zone_touch_transaction_handle_completions(&first_pending_failure,1);
 assert(zone_touch_transaction_submit(extra));
 zone_touch_transaction_reset_for_tests();
 online[0]=online[1]=true;
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
 assert(magic && effects==0 && pcs[0].epics==100 && artifacts.empty());
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
 assert(artifacts.size()==1); assert_artifact(0,0);
 zone_touch_transaction_handle_completions(&success,1);
 assert(effects==1 && zone_publications==1 && artifacts.size()==1);
 online[1]=true;
 // Reconnect hydration must not be overwritten by an older award receipt.
 pcs[1].epics=200; pcs[1].epic_revision=3;
 zone_touch_transaction_player_ready(&characters[1]);
 assert(effects==2 && pcs[1].epics==200 && pcs[1].epic_revision==3);
 assert(artifacts.size()==2); assert_artifact(1,1);
 zone_touch_transaction_player_ready(&characters[1]);
 assert(effects==2 && artifacts.size()==2);
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
 assert(!magic && effects==2 && zone_publications==1 && artifacts.size()==2);
 zone_touch_transaction_player_ready(&characters[0]);
 zone_touch_transaction_player_ready(&characters[1]);
 assert(artifacts.size()==2);
 test_pending_admission_excludes_committed_offline_receipts();
}
'''


# Compile the real scalar helper, object wrapper, and award publisher together.
# Only their game-state/services are adapters; no object exists at publication.
LEVEL_ADAPTERS = r'''
#include "world/zone_touch_command.h"
#include "world/epic_command.h"
#include <cassert>
#include <cstdio>
#include <cstring>
struct test_pc { int epics = 100; };
struct test_character
{
 struct { test_pc *pc; } only;
 int level = 50;
 long experience = 1000;
 bool alive = true, npc = false, nolevel = false, multiclass = false;
};
using P_char = test_character *;
struct test_object { int value[4] = {}; };
using P_obj = test_object *;
struct affected_type { int modifier; };
#define IS_ALIVE(ch) ((ch)->alive)
#define IS_NPC(ch) ((ch)->npc)
#define GET_RACEWAR(ch) 0
#define GET_LEVEL(ch) ((ch)->level)
#define GET_EXP(ch) ((ch)->experience)
#define PLR3_FLAGGED(ch,flag) ((ch)->nolevel)
#define IS_MULTICLASS_PC(ch) ((ch)->multiclass)
#define MAXLVLMORTAL 56
#define EPIC_ZONE 1
#define BOPT_ZONE 1
#define LOG_DEBUG "debug"
struct epic_level_context { int expected_level; long experience_cost; int epic_cost; };
struct epic_award_context { int type, data, amount; bool blessing, task_penalty; };
static long new_exp_table[60] = {};
static int spends = 0, payouts = 0, object_lookups = 0;
static int64_t spend_delta = 0;
static epic_level_context spend_context = {};
int sql_level_cap(int) { return 56; }
int get_property(const char *name, int fallback)
{
 if (std::strncmp(name,"epic.forLevel.",14)==0) return 100;
 return fallback;
}
void send_to_char(const char *,P_char) {}
void debug(const char *,...) {}
void logit(const char *,const char *,...) {}
void epiclog(int,const char *,...) {}
void epic_stone_set_affect(P_char) {}
affected_type *get_epic_task(P_char) { return nullptr; }
void epic_complete_errand(P_char,int) {}
void affect_remove(P_char,affected_type *) {}
void check_boon_completion(P_char,void *,uint32_t,int) {}
P_obj find_epic_stone(uint64_t) { ++object_lookups; return nullptr; }
void epic_level_committed(P_char,bool,const epic_command_result &,unsigned int,const uint8_t *,size_t) {}
void epic_award_committed(P_char,bool,const epic_command_result &,unsigned int,const uint8_t *,size_t)
{ ++payouts; }
bool epic_transaction_submit(P_char ch,int64_t delta,epic_reason_type reason,int64_t reason_id,
 uint16_t flags,critical_source_site source,critical_deadline_class deadline,
 void (*)(P_char,bool,const epic_command_result &,unsigned int,const uint8_t *,size_t),
 const void *context,size_t size)
{
 assert(reason==epic_reason_type::level_purchase && reason_id==ch->level+1);
 assert(flags==EPIC_COMMAND_REQUIRE_FUNDS && source==critical_source_site::zone_event);
 assert(deadline==critical_deadline_class::interactive && size==sizeof(spend_context));
 std::memcpy(&spend_context,context,size);
 ++spends; spend_delta=delta;
 return true;
}
'''
LEVEL_CHECKS = r'''
int main()
{
 test_pc pc;
 test_character character;
 character.only.pc=&pc;
 new_exp_table[51]=1000;
 zone_touch_result result;
 result.group_size=1;
 result.stone_uid=12345; // absent: extraction, reset, or reconnect before publication
 result.stone_level=51;
 result.awards[0].amount=10;
 epic_publish_stone_award(&character,result,0);
 assert(payouts==1 && spends==1 && spend_delta==-100 && object_lookups==0);
 assert(spend_context.expected_level==50 && spend_context.experience_cost==1000 &&
        spend_context.epic_cost==100);
 // Captured stone level, not an unrelated present object, determines eligibility.
 result.stone_level=50;
 epic_publish_stone_award(&character,result,0);
 assert(payouts==2 && spends==1 && object_lookups==0);
 pc.epics=200;
 epic_publish_stone_award(&character,result,0);
 assert(spends==2 && spend_delta==-100); // preserve the existing any-stone threshold
 pc.epics=100;
 result.stone_level=51;
 character.experience=999;
 epic_publish_stone_award(&character,result,0);
 assert(spends==2);
 character.experience=1000;
 character.nolevel=true;
 epic_publish_stone_award(&character,result,0);
 assert(spends==2);
 character.nolevel=false;
 result.recovered_claim=true;
 const int prior_payouts=payouts;
 epic_publish_stone_award(&character,result,0);
 assert(spends==2 && payouts==prior_payouts);
 result.recovered_claim=false;
 character.level=48; new_exp_table[49]=1000; result.stone_level=49;
 epic_publish_stone_award(&character,result,0);
 assert(spends==2); // free-level publication must not also buy a stone level
 character.level=50;
 test_object stone; stone.value[3]=51;
 epic_stone_level_char(&stone,&character);
 assert(spends==3); // existing direct object callers still use the same policy
 epic_stone_level_char(nullptr,&character);
 assert(spends==3);
}
'''

class StoneRuntimeTests(unittest.TestCase):
    def test_admission_failure_pending_ambiguity_success_disconnect_and_recovery(self):
        self.assertIsNotNone(shutil.which("g++"), "Linux g++ is required for the native runtime regression")
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'stone.cpp'
            binary = Path(directory) / 'stone'
            source.write_text(HARNESS)
            subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Isrc',
                str(source), 'src/world/zone_touch_transaction.c', 'src/world/zone_touch_command.c',
                'src/world/epic_command.c', 'src/persistence/critical_command.c',
                '-lcrypto', '-o', str(binary)], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)

    def test_captured_stone_level_survives_object_removal(self):
        self.assertIsNotNone(shutil.which("g++"), "Linux g++ is required for the native gameplay regression")
        code = LEVEL_ADAPTERS + "\n".join(
            extract_function("epic.c", signature) for signature in (
                "static void epic_stone_level_char_from_level(int stone_level, P_char ch)",
                "void epic_stone_level_char(P_obj obj, P_char ch)",
                "void epic_publish_stone_award(P_char ch, const zone_touch_result &result, size_t index)",
            )
        ) + LEVEL_CHECKS
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "stone_level.cpp"
            binary = Path(directory) / "stone_level"
            source.write_text(code)
            subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
                            str(source), "-o", str(binary)], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)

if __name__ == '__main__':
    unittest.main()
