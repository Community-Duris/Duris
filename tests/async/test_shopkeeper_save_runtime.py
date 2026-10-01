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
flush_marker = "void sql_save_dirty_shopkeepers(" if baseline else "bool sql_save_dirty_shopkeepers("
flush_start = sql.index(flush_marker) if baseline else sql.rindex(flush_marker)
flush = sql[flush_start:sql.index("static P_obj sql_load_saved_item_contents", flush_start)]
guards = "" if baseline else sql[sql.rfind("namespace", 0, sql.index("enum class shopkeeper_save_reason")):sql.index("static bool sql_save_shopkeeper_item_affects")]
files = (subprocess.check_output(["git", "show", "440248b17:src/core/files.c"], cwd=ROOT, text=True)
         if baseline else (ROOT / "src/core/files.c").read_text())
direct_start = files.index("int writeShopKeeper(P_char ch)") if baseline else files.index(
    "int writeShopKeeper(P_char ch, int shop_nr)"
)
direct = files[direct_start:files.index("int deleteShopKeeper(", direct_start)]
preamble = r'''
#include "economy/shopkeeper_save_policy.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
constexpr int NOWHERE = -1, LOG_DEBUG = 0, MAX_WEAR = 2;
struct Character;
using P_char = Character *;
struct Object { Object *next_content = nullptr; };
using P_obj = Object *;
using mob_proc = int (*)(P_char, P_char, int, char *);
struct npc_data { int shopkeeper_shop_id = -1; };
struct Character {
    int rnum = 0, in_room = 0, birthplace = 0;
    bool npc = true;
    Character *next = nullptr, *next_in_room = nullptr, *master = nullptr;
    npc_data npc_storage = {};
    struct { npc_data *npc; } only;
    P_obj equipment[MAX_WEAR] = {}, carrying = nullptr;
    Character() : only{&npc_storage} {}
};
int shop_keeper(P_char, P_char, int, char *) { return 0; }
int world_quest_proc(P_char, P_char, int, char *) { return 0; }
int trainer_proc(P_char, P_char, int, char *) { return 0; }
struct Shop { int keeper=0, in_room=100, shop_is_roaming=0, dirty=1;
    mob_proc func=nullptr;
    shopkeeper_save_retry_state dirty_save_retry = {}; } shops[2];
auto *shop_index = shops;
int number_of_shops=1, top_of_world=1, top_of_mobt=1;
struct Room { int number; P_char people; } world[2] = {{100,nullptr},{101,nullptr}};
struct Index { int virtual_number=0; struct { mob_proc mob=nullptr; } func; mob_proc qst_func=nullptr; } mob_index[2] = {};
P_char character_list=nullptr;
bool DB=true;
#define IS_NPC(ch) ((ch)->npc)
#define IS_PC(ch) (!(ch)->npc)
#define GET_RNUM(ch) ((ch)->rnum)
#define GET_MASTER(ch) ((ch)->master)
#define GET_NAME(ch) "fixture"
#define GET_PLYR(ch) (ch)
#define GET_BIRTHPLACE(ch) ((ch)->birthplace)
int singleton_shop_id(P_char keeper) {
    if (!keeper || !IS_NPC(keeper) || GET_MASTER(keeper)) return -1;
    const int bound = keeper->only.npc ? keeper->only.npc->shopkeeper_shop_id : -1;
    if (bound >= 0)
        return bound < number_of_shops && shop_index[bound].keeper == GET_RNUM(keeper) ? bound : -1;
    const int room = keeper->in_room >= 0 && keeper->in_room <= top_of_world ? world[keeper->in_room].number : -1;
    int room_match = -1, home = -1, roaming = -1;
    for (int shop = 0; shop < number_of_shops; ++shop) {
        if (shop_index[shop].keeper != GET_RNUM(keeper)) continue;
        if (!shop_index[shop].shop_is_roaming && shop_index[shop].in_room == room) {
            if (room_match >= 0) return -1;
            room_match = shop;
        }
        if (!shop_index[shop].shop_is_roaming && shop_index[shop].in_room == GET_BIRTHPLACE(keeper)) home = shop;
        if (shop_index[shop].shop_is_roaming) roaming = roaming == -1 ? shop : -2;
    }
    return room_match >= 0 ? room_match : (home >= 0 ? home : (roaming >= 0 ? roaming : -1));
}
void bind_shopkeeper(P_char keeper, int shop_nr) {
    if (keeper && IS_NPC(keeper) && !GET_MASTER(keeper) && keeper->only.npc &&
        shop_nr >= 0 && shop_nr < number_of_shops && shop_index[shop_nr].keeper == GET_RNUM(keeper))
        keeper->only.npc->shopkeeper_shop_id = shop_nr;
}
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
    Character duplicate;
    mob_index[0].virtual_number = 200;
    mob_index[1].virtual_number = 201;
    character_list = world[0].people = &keeper;
    shops[0].keeper = 0;
    // A configured non-shop procedure is irrelevant to persistence identity;
    // start with an invalid configured shop to exercise a retained red retry.
    mob_index[0].qst_func = trainer_proc;
    mob_index[0].func.mob = trainer_proc;
    shops[0].keeper = -1;
    assert(!sql_save_dirty_shopkeepers(false));
    assert(shops[0].dirty && begins==0 && writes==0);
    assert(logs.back().find("reason=invalid_keeper") != std::string::npos);
    auto first_logs=logs.size();
    for (int i=0; i<100; ++i) assert(!sql_save_dirty_shopkeepers(false));
    assert(logs.size()==first_logs && begins==0);
    clock_now += 60;
    assert(!sql_save_dirty_shopkeepers(false));
    assert(logs.size()==first_logs+1 && shops[0].dirty);

    // Restore the configured keeper role; world_quest/trainer procedures remain installed.
    shops[0].keeper = 0;
    mob_index[0].qst_func = world_quest_proc;
    mob_index[0].func.mob = trainer_proc;
    shops[0].dirty = 1;
    assert(sql_save_dirty_shopkeepers(true));
    assert(mob_index[0].qst_func == world_quest_proc && mob_index[0].func.mob == trainer_proc);
    assert(!shops[0].dirty && begins==1 && commits==1);
    assert(shops[0].dirty_save_retry.failure_count==0);
    shops[0].dirty = 1;
    mob_index[0].qst_func = trainer_proc;
    assert(sql_save_dirty_shopkeepers(true));
    assert(mob_index[0].qst_func == trainer_proc && mob_index[0].func.mob == trainer_proc);
    assert(!shops[0].dirty && commits==2);

    // Invalid configured keeper must not silently discard retry state.
    shops[0].dirty=1; shops[0].keeper=-1;
    assert(!sql_save_dirty_shopkeepers(false));
    assert(shops[0].dirty && logs.back().find("reason=invalid_keeper") != std::string::npos);
    shops[0].keeper=2;
    assert(!sql_save_dirty_shopkeepers(true) && shops[0].dirty);
    shops[0].keeper=0;
    // Missing keeper: bounded retries and no writes to old stock.
    character_list=world[0].people=nullptr;
    assert(!sql_save_dirty_shopkeepers(true) && shops[0].dirty);
    assert(logs.back().find("reason=keeper_not_found") != std::string::npos);
    // Duplicate inventories must not select an arbitrary first mob.
    keeper.next=&duplicate;
    character_list=world[0].people=&keeper;
    assert(!sql_save_dirty_shopkeepers(true) && shops[0].dirty);
    assert(logs.back().find("reason=keeper_ambiguous") != std::string::npos);
    keeper.next=nullptr;
    // Transaction failures never clear dirty; forced flush reports failure.
    begin_ok=false;
    assert(!sql_save_dirty_shopkeepers(true) && shops[0].dirty && begins==3);
    begin_ok=true; write_ok=false;
    assert(!sql_save_dirty_shopkeepers(true) && shops[0].dirty && rollbacks==1);
    write_ok=true; commit_ok=false;
    assert(!sql_save_dirty_shopkeepers(true) && shops[0].dirty && rollbacks==2);
    commit_ok=true; Object stock; keeper.carrying=&stock; item_ok=false;
    assert(!sql_save_dirty_shopkeepers(true) && shops[0].dirty && rollbacks==3 && keeper.carrying==&stock);
    item_ok=true;
    assert(sql_save_dirty_shopkeepers(true) && !shops[0].dirty && keeper.carrying==&stock);
    // Direct invalid identity is refused before BEGIN or array dereference.
    auto before=begins;
    assert(!sql_save_shopkeeper(&keeper,2));
    keeper.rnum=1; assert(!sql_save_shopkeeper(&keeper,0)); keeper.rnum=0;
    keeper.in_room=99; assert(!sql_save_shopkeeper(&keeper,0)); keeper.in_room=0;
    assert(begins==before);
    // Explicit shop_nr selects the bound shared-template shop; no slot guessing.
    number_of_shops=2; shops[0].keeper=shops[1].keeper=0;
    shops[0].in_room=100; shops[1].in_room=101;
    shops[0].shop_is_roaming=shops[1].shop_is_roaming=0;
    shops[0].dirty=shops[1].dirty=1;
    keeper.only.npc->shopkeeper_shop_id=-1;
    keeper.in_room=1;
    assert(writeShopKeeper(&keeper,1)==1 && !shops[1].dirty && shops[0].dirty);
    // A valid bound identity is accepted away from the fixed shop home.
    shops[1].dirty=1; keeper.only.npc->shopkeeper_shop_id=1; keeper.in_room=0;
    assert(writeShopKeeper(&keeper,1)==1 && !shops[1].dirty);
    // Without that binding, the same fixed-shop away-room candidate is rejected.
    shops[1].dirty=1; keeper.only.npc->shopkeeper_shop_id=-1;
    assert(writeShopKeeper(&keeper,1)==0 && shops[1].dirty);
    keeper.in_room=1;
    assert(writeShopKeeper(&keeper,0)==0 && shops[0].dirty);
    // A roaming configuration with room 0 is valid, but shared roaming IDs are not guessed.
    shops[0].shop_is_roaming=shops[1].shop_is_roaming=1;
    shops[0].in_room=shops[1].in_room=0;
    shops[0].dirty=shops[1].dirty=1;
    keeper.in_room=1;
    keeper.only.npc->shopkeeper_shop_id=1;
    assert(!sql_save_dirty_shopkeepers(true) && shops[0].dirty && !shops[1].dirty);
    number_of_shops=1;
    shops[0].dirty=1; keeper.only.npc->shopkeeper_shop_id=-1;
    assert(sql_save_dirty_shopkeepers(true) && !shops[0].dirty);
    // A player-controlled copy of a keeper template is not shop stock authority.
    keeper.master=&duplicate; shops[0].dirty=1; before=begins;
    assert(!sql_save_dirty_shopkeepers(true) && shops[0].dirty && begins==before); keeper.master=nullptr;
    // Database outage is a terminal failed flush, not permission to clear dirty state.
    DB=false; before=begins;
    assert(!sql_save_dirty_shopkeepers(true) && shops[0].dirty && begins==before);
    DB=true;
    // A failed direct buy-path checkpoint must leave a retry pending.
    keeper.in_room=0; keeper.only.npc->shopkeeper_shop_id=-1; commit_ok=false;
    assert(writeShopKeeper(&keeper,0)==0 && shops[0].dirty);
    commit_ok=true; assert(writeShopKeeper(&keeper,0)==1 && !shops[0].dirty);
    std::puts("production shopkeeper save/flush: explicit identity, non-shop procs, roaming room0, retained dirty, controlled exclusion, terminal failures PASS");
}
'''
old_main = r'''
int main() {
    Character keeper; character_list=world[0].people=&keeper;
    mob_index[0].func.mob=trainer_proc;
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
    legacy_guard = ("#define IS_SHOPKEEPER(ch) (IS_NPC(ch) && ((mob_index[GET_RNUM(ch)].qst_func == shop_keeper) || "
                     "(mob_index[GET_RNUM(ch)].func.mob == shop_keeper)))\n") if baseline else ""
    source.write_text(preamble+legacy_guard+guards+save+flush+("" if baseline else direct)+(old_main if baseline else main))
    subprocess.run(["g++","-std=c++20","-g","-Wall","-Wextra","-Werror",
                    "-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie",
                    "-I",str(ROOT/"src"),str(source),"-o",str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=1:halt_on_error=1"})
