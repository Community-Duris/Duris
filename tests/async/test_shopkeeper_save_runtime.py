#!/usr/bin/env python3
"""Execute production SQL save/flush with deterministic world and transaction I/O.

No live DB: the double records begin/write/rollback/commit calls, not SQL semantics.
--baseline executes the original flush and proves its failure storm/dirty loss.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
baseline = "--baseline" in sys.argv
sql = (subprocess.check_output(["git", "show", "440248b17:src/sql/sql_player.c"], cwd=ROOT, text=True)
       if baseline else (ROOT / "src/sql/sql_player.c").read_text())
save_start = sql.rindex("bool sql_save_shopkeeper(P_char ch, int shop_nr)")
save = sql[save_start:sql.index("bool sql_delete_shopkeeper", save_start)]
flush_start = sql.index("void sql_save_dirty_shopkeepers(")
flush = sql[flush_start:sql.index("static P_obj sql_load_saved_item_contents", flush_start)]
guards = "" if baseline else sql[sql.rfind("namespace", 0, sql.index("enum class shopkeeper_save_reason")):sql.index("static bool sql_save_shopkeeper_item_affects")]
files = (ROOT / "src/core/files.c").read_text()
direct = files[files.index("int writeShopKeeper("):files.index("int deleteShopKeeper(")]
preamble = r'''
#include "economy/shopkeeper_save_policy.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
constexpr int NOWHERE = -1, LOG_DEBUG = 0, MAX_WEAR = 2;
struct Object { Object *next_content = nullptr; };
using P_obj = Object *;
struct Character {
    int rnum = 0, in_room = 0;
    bool npc = true;
    Character *next = nullptr, *next_in_room = nullptr, *master = nullptr;
    P_obj equipment[MAX_WEAR] = {}, carrying = nullptr;
};
using P_char = Character *;
struct Shop { int keeper=0, in_room=100, shop_is_roaming=0, dirty=1;
    shopkeeper_save_retry_state dirty_save_retry = {}; } shops[2];
auto *shop_index = shops;
int number_of_shops=1, top_of_world=1, top_of_mobt=1;
struct Room { int number; P_char people; } world[2] = {{100,nullptr},{101,nullptr}};
struct Index { int virtual_number; bool shopkeeper; } mob_index[2] = {{200,true},{201,true}};
P_char character_list=nullptr;
bool DB=true;
#define IS_NPC(ch) ((ch)->npc)
#define IS_PC(ch) (!(ch)->npc)
#define GET_RNUM(ch) ((ch)->rnum)
#define GET_MASTER(ch) ((ch)->master)
#define GET_NAME(ch) "fixture"
#define GET_PLYR(ch) (ch)
#define IS_SHOPKEEPER(ch) (IS_NPC(ch) && mob_index[GET_RNUM(ch)].shopkeeper)
time_t clock_now=1000;
time_t fake_time(time_t *) { return clock_now; }
#define time fake_time
int real_room(int n) { return n >=100 && n<=101 ? n-100 : NOWHERE; }
std::vector<std::string> logs;
void logit(int, const char *fmt, ...) {
    char buf[2048]; va_list args; va_start(args,fmt);
    vsnprintf(buf,sizeof(buf),fmt,args); va_end(args); logs.emplace_back(buf);
}
int begins=0, writes=0, commits=0, rollbacks=0;
bool begin_ok=true, write_ok=true, commit_ok=true, item_ok=true;
bool sql_begin_transaction() { ++begins; return begin_ok; }
bool sql_run_query(const char *) { ++writes; return write_ok; }
bool sql_commit() { ++commits; return commit_ok; }
void sql_rollback() { ++rollbacks; }
int mysql_insert_id(bool) { return 1; }
bool sql_save_shopkeeper_affects(int, P_char) { return true; }
bool sql_save_shopkeeper_item(int, P_obj, int, int) { return item_ok; }
bool shop_producing(P_obj, int) { return false; }
'''
main = r'''
int main() {
    Character keeper;
    character_list = world[0].people = &keeper;
    // Production incident signature: matching NPC but missing shop procedure.
    mob_index[0].shopkeeper=false;
    sql_save_dirty_shopkeepers(false);
    assert(shops[0].dirty && begins==0 && writes==0);
    assert(logs.back().find("reason=keeper_not_shopkeeper") != std::string::npos);
    auto first_logs=logs.size();
    for (int i=0; i<100; ++i) sql_save_dirty_shopkeepers(false);
    assert(logs.size()==first_logs && begins==0);
    clock_now += 60;
    sql_save_dirty_shopkeepers(false);
    assert(logs.size()==first_logs+1 && shops[0].dirty);
    // Restoring identity permits a forced terminal flush despite backoff.
    mob_index[0].shopkeeper=true;
    sql_save_dirty_shopkeepers(true);
    assert(!shops[0].dirty && begins==1 && commits==1);
    assert(shops[0].dirty_save_retry.failure_count==0);
    // Invalid configured keeper must not silently discard retry state.
    shops[0].dirty=1; shops[0].keeper=-1;
    sql_save_dirty_shopkeepers(false);
    assert(shops[0].dirty && begins==1);
    assert(logs.back().find("reason=invalid_keeper") != std::string::npos);
    shops[0].keeper=2;
    sql_save_dirty_shopkeepers(true);
    assert(shops[0].dirty && begins==1);
    shops[0].keeper=0;
    // Missing keeper: bounded retries and no writes to old stock.
    character_list=world[0].people=nullptr;
    sql_save_dirty_shopkeepers(true);
    assert(shops[0].dirty && begins==1);
    assert(logs.back().find("reason=keeper_not_found") != std::string::npos);
    // Duplicate inventories must not select an arbitrary first mob.
    Character duplicate; keeper.next=&duplicate;
    character_list=world[0].people=&keeper;
    sql_save_dirty_shopkeepers(true);
    assert(shops[0].dirty && begins==1);
    assert(logs.back().find("reason=keeper_ambiguous") != std::string::npos);
    keeper.next=nullptr;
    // Transaction failures never clear dirty; retries use the real save body.
    begin_ok=false; sql_save_dirty_shopkeepers(true);
    assert(shops[0].dirty && begins==2 && writes==2);
    begin_ok=true; write_ok=false; sql_save_dirty_shopkeepers(true);
    assert(shops[0].dirty && rollbacks==1);
    write_ok=true; commit_ok=false; sql_save_dirty_shopkeepers(true);
    assert(shops[0].dirty && rollbacks==2);
    commit_ok=true; Object stock; keeper.carrying=&stock; item_ok=false;
    sql_save_dirty_shopkeepers(true);
    assert(shops[0].dirty && rollbacks==3 && keeper.carrying==&stock);
    item_ok=true; sql_save_dirty_shopkeepers(true);
    assert(!shops[0].dirty && keeper.carrying==&stock);
    // Direct invalid identity is refused before BEGIN or array dereference.
    auto before=begins;
    assert(!sql_save_shopkeeper(&keeper,2));
    keeper.rnum=1; assert(!sql_save_shopkeeper(&keeper,0)); keeper.rnum=0;
    keeper.in_room=99; assert(!sql_save_shopkeeper(&keeper,0)); keeper.in_room=0;
    assert(begins==before);
    // Shared-template shops: direct saves must choose the current room, not slot 0.
    number_of_shops=2; shops[1].in_room=101; shops[1].dirty=1;
    keeper.in_room=1; shops[0].dirty=1;
    assert(writeShopKeeper(&keeper)==1 && !shops[1].dirty && shops[0].dirty);
    // Roaming lookup cannot consume the sole keeper belonging to its fixed sibling.
    shops[0].shop_is_roaming=1; before=begins;
    sql_save_dirty_shopkeepers(true);
    assert(shops[0].dirty && begins==before);
    // A unique roaming configuration can save away from home.
    number_of_shops=1;
    sql_save_dirty_shopkeepers(true);
    assert(!shops[0].dirty);
    // A player-controlled copy of a keeper template is not shop stock authority.
    keeper.master=&duplicate; shops[0].dirty=1; before=begins;
    sql_save_dirty_shopkeepers(true);
    assert(shops[0].dirty && begins==before); keeper.master=nullptr;
    // A failed direct buy-path checkpoint must leave a retry pending.
    keeper.in_room=0; commit_ok=false;
    assert(writeShopKeeper(&keeper)==0 && shops[0].dirty);
    commit_ok=true; assert(writeShopKeeper(&keeper)==1 && !shops[0].dirty);
    std::puts("production shopkeeper save/flush: guards, storm, retained dirty, duplicate, forced recovery, transaction failures PASS");
}
'''
old_main = r'''
int main() {
    Character keeper; character_list=world[0].people=&keeper;
    mob_index[0].shopkeeper=false;
    for (int i=0;i<100;++i) sql_save_dirty_shopkeepers();
    assert(logs.size()==100 && begins==0 && shops[0].dirty);
    shops[0].keeper=-1; sql_save_dirty_shopkeepers();
    assert(!shops[0].dirty);
    std::puts("BASELINE reproduced: 100 pretransaction failures/logs; invalid keeper silently clears dirty");
}
'''
build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="shop-save-runtime-", dir=build) as tmp:
    source=Path(tmp)/"test.cpp"
    binary=Path(tmp)/"test"
    source.write_text(preamble+guards+save+flush+("" if baseline else direct)+(old_main if baseline else main))
    subprocess.run(["g++","-std=c++20","-g","-Wall","-Wextra","-Werror",
                    "-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie",
                    "-I",str(ROOT/"src"),str(source),"-o",str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=1:halt_on_error=1"})
