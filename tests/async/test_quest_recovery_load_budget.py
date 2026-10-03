#!/usr/bin/env python3
"""Exercise the actual SQL load recovery blocks with instrumented read outcomes."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
repository = (ROOT / "src/player/player_load_repository.c").read_text()
begin = repository.index("\t{\n\t\tstd::vector<quest_reward_obligation_record> obligations;")
end = repository.index("\tif (!request.pending_spell_effect_operations.empty())", begin)
blocks = repository[begin:end]
harness = r'''
#include "player/player_load_repository.h"
#include <cassert>
#include <cerrno>
#include <iostream>
#include <new>

bool refuse = false;
void mark_degraded(player_load_result *result, uint32_t component, const char *stage) {
    result->outcome = player_load_outcome::degraded;
    result->degraded_components |= component;
    result->failed_component = stage;
}
quest_reward_obligation_result quest_reward_obligation_repository_pending(
    MYSQL *, uint32_t, std::vector<quest_reward_obligation_record> *records,
    unsigned *error, quest_reward_read_metrics *metrics) {
    *metrics = {3, 4, 512};
    if (refuse) { *error = EIO; return quest_reward_obligation_result::database_error; }
    quest_reward_obligation_record record;
    record.continuation = {1, 2, 3};
    record.economic_applied_mask = 1;
    records->push_back(record);
    return quest_reward_obligation_result::ok;
}
quest_reward_obligation_result quest_reward_xp_entitlement_repository_pending(
    MYSQL *, uint32_t, std::vector<quest_reward_xp_entitlement_record> *records,
    unsigned *error, quest_reward_read_metrics *metrics) {
    *metrics = {1, 2, 256};
    if (refuse) { *error = EIO; return quest_reward_obligation_result::database_error; }
    quest_reward_xp_entitlement_record record;
    record.continuation = {4, 5, 6};
    record.amount = 100;
    records->push_back(record);
    return quest_reward_obligation_result::ok;
}
void recover(player_load_result &result) {
    MYSQL *connection = nullptr;
''' + blocks + r'''
}
int main() {
    player_load_result result;
    result.pid = 7;
    result.metrics = {10, 20, 1000, 0};
    recover(result);
    assert(result.metrics.query_count == 14 && result.metrics.row_count == 26 &&
           result.metrics.byte_count == 1768);
    assert(result.pending_quest_rewards.size() == 1 && result.pending_quest_xp_entitlements.size() == 1);
    assert(result.pending_quest_rewards[0].economic_applied_mask == 1 &&
           result.pending_quest_xp_entitlements[0].amount == 100);
    refuse = true;
    recover(result);
    assert(result.metrics.query_count == 18 && result.metrics.row_count == 32 &&
           result.metrics.byte_count == 2536);
    assert(result.pending_quest_rewards.empty() && result.pending_quest_xp_entitlements.empty());
    assert(result.degraded_components & PLAYER_LOAD_DEGRADED_RECOVERY);
    assert(result.error_code == EIO);
    std::cout << "quest recovery load aggregates actual read budgets on success and refusal: ok\n";
}
'''

with tempfile.TemporaryDirectory(prefix="quest-load-budget-") as directory:
    source = Path(directory) / "probe.cpp"
    binary = Path(directory) / "probe"
    source.write_text(harness)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
        "-I/usr/include/mysql", str(source), "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
