#!/usr/bin/env python3
"""Run the real bulk continuation with real source policy after custody changes."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

function = extract_function('cmd/actobj.c', 'static void continue_bulk_get(P_char actor, uint32_t actor_pid)\n{')
prelude = r'''
#include "core/utils.h"
#include "item/item_get_policy.h"
#include "item/item_ownership_runtime.h"
#include <cassert>
#include <cstdlib>
#include <map>
#include <vector>
#include <cstdio>
room_data rooms[1]{};
P_room world = rooms;
extern const int top_of_world = 0;
int top_of_objt = 4;
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
void send_to_char(const char *, P_char) {}
std::map<uint64_t, P_obj> live;
std::map<uint64_t, item_ownership_runtime_entry> custody;
bool item_ownership_runtime_lookup(uint64_t uid, item_ownership_runtime_entry *out) {
    auto it = custody.find(uid);
    if (it == custody.end()) return false;
    *out = it->second; return true;
}
P_obj find_live_item_uid(uint64_t uid) { return live.at(uid); }
bool get_item_source_owner(P_char a, P_obj o, P_obj c, item_owner_identity *s) {
    return item_get_source_owner(a, o, c, s);
}
struct bulk_get_state {
    uint64_t container_uid{};
    std::vector<uint64_t> durable_items;
    item_owner_identity source{};
    int reason{};
    bool corpse{};
};
struct bulk_movement_context { uint32_t actor_pid; };
enum class item_movement_reject { none, owner_mismatch };
std::map<uint32_t, bulk_get_state> bulk_gets;
int rejected = 0, adopted = 0, submitted = 0;
bool bulk_get_source_available(P_char, const bulk_get_state &, P_obj) { return true; }
bool bulk_get_source_matches(const bulk_get_state &, P_obj, P_obj) { return true; }
void fail_bulk_get(P_char, uint32_t, const char *) { ++rejected; }
void reject_bulk_get_admission(P_char, uint32_t, item_movement_reject) { ++rejected; }
void announce_corpse_bulk_get(P_char, const bulk_get_state &, P_obj) {}
void bulk_get_adoption_completion() {}
void bulk_get_completion() {}
template<class... Args> bool item_movement_transaction_submit(Args...) { ++adopted; return true; }
template<class... Args> bool item_movement_transaction_submit_batch(Args...) { ++submitted; return true; }
'''
driver = r'''
int main() {
    pc_only_data pc{}; pc.pid = 42;
    char_data actor{}; actor.only.pc = &pc; actor.in_room = 0;
    rooms[0].number = 100;
    obj_data root{}, container{}, other{};
    root.obj_uid = 1; container.obj_uid = 2; other.obj_uid = 3;
    root.loc_p = LOC_ROOM; root.loc.room = 0;
    container.loc_p = LOC_ROOM; container.loc.room = 0;
    other.loc_p = LOC_ROOM; other.loc.room = 0;
    live = {{1, &root}, {2, &container}, {3, &other}};
    item_owner_identity owner{item_owner_type::room, 100, 0};
    item_ownership_runtime_entry active{};
    active.owner = owner; active.state = item_custody_state::active;
    bulk_gets[42].source = owner; bulk_gets[42].durable_items = {1};
    custody[1] = active;
    continue_bulk_get(&actor, 42);
    assert(submitted == 1 && !rejected && !adopted);
    for (auto state : {item_custody_state::quarantined, item_custody_state::destroyed}) {
        custody[1].state = state;
        rejected = submitted = adopted = 0;
        continue_bulk_get(&actor, 42);
        assert(rejected == 1 && !submitted && !adopted);
    }
    custody[1] = active;
    root.loc_p = LOC_INSIDE; root.loc.inside = &container;
    bulk_gets[42].container_uid = 2;
    custody[2] = active;
    rejected = submitted = adopted = 0;
    continue_bulk_get(&actor, 42);
    assert(submitted == 1 && !rejected);
    custody[2].state = item_custody_state::quarantined;
    rejected = submitted = adopted = 0;
    continue_bulk_get(&actor, 42);
    assert(rejected == 1 && !submitted && !adopted);
    // Validate the whole forest before even adopting the first stock root.
    root.loc_p = LOC_ROOM; root.loc.room = 0;
    bulk_gets[42].container_uid = 0;
    bulk_gets[42].durable_items = {1, 3};
    custody.erase(1); custody[3] = active;
    custody[3].state = item_custody_state::destroyed;
    rejected = submitted = adopted = 0;
    continue_bulk_get(&actor, 42);
    assert(rejected == 1 && !submitted && !adopted);
    custody[3] = active;
    rejected = submitted = adopted = 0;
    continue_bulk_get(&actor, 42);
    assert(adopted == 1 && !submitted && !rejected);
    puts("bulk continuation rejects changed root/container custody before adoption or transfer");
}
'''
with tempfile.TemporaryDirectory(prefix='bulk-custody-') as directory:
    path = Path(directory)
    cpp = path / 'test.cpp'
    binary = path / 'test'
    cpp.write_text(prelude + function + driver)
    subprocess.run(['g++', '-std=c++20', '-Isrc', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
                    '-ffunction-sections', '-fdata-sections', str(cpp),
                    'src/item/item_get_policy.c', 'src/item/item_transfer_command.c',
                    '-Wl,--gc-sections', '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
