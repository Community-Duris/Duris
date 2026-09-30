#!/usr/bin/env python3
"""Exercise real dispatcher/worker/journal/load lifecycle with controlled DB refusal.

SQL application and pool acquisition are controlled boundaries, not a database
simulation. The real loader must not acquire a connection while replay is closed;
otherwise a pre-replay snapshot could be cached and used after readiness opens.
"""
from _paths import SRC, rel
import ast
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
# Reuse the codec-valid death and materializer fixtures without executing the
# sibling test or copying its large character/item/pet construction block.
tree = ast.parse((Path(__file__).with_name("test_death_journal_load_fence.py")).read_text())
fixture = next(ast.literal_eval(node.value) for node in tree.body
               if isinstance(node, ast.Assign)
               and any(isinstance(target, ast.Name) and target.id == "HARNESS"
                       for target in node.targets))
fixture = fixture[:fixture.index("int main(int argc")]
for retired in (
    "player_save_pipeline_replay_gate replay_gate;\n",
    "bool player_save_pipeline_loads_allowed(void)\n{\n    return replay_gate.loads_allowed();\n}\n",
    "bool player_revision_hydrate(int, player_revision_t) { return true; }\n",
):
    assert fixture.count(retired) == 1, retired
    fixture = fixture.replace(retired, "")

HARNESS = r'''
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include "player/player_load_pipeline.h"
#include "player/player_snapshot_repository.h"
#include "persistence/persistence_observability.h"
#include "sql/sql_pool.h"
'''+fixture+r'''
namespace lifecycle {
std::mutex mutex;
std::condition_variable changed;
bool entered = false, released = false;
player_save_apply_outcome outcome = player_save_apply_outcome::retryable_failure;
bool wrong_revision = false;
std::vector<uint8_t> expected;
std::atomic<unsigned> pool_calls{0};
}

// This callback is deliberately BEFORE any SQL/retention write. It verifies
// actual decoded journal bytes, then lets the controller choose a replay result.
player_save_apply_result player_snapshot_repository_apply_from_pool(
    const player_snapshot &snapshot, void *)
{
    std::vector<uint8_t> encoded;
    assert(player_snapshot_encode(snapshot, &encoded) == player_snapshot_codec_result::ok);
    std::unique_lock<std::mutex> lock(lifecycle::mutex);
    assert(encoded == lifecycle::expected);
    lifecycle::entered = true;
    lifecycle::changed.notify_all();
    assert(lifecycle::changed.wait_for(lock, std::chrono::seconds(5),
                                      [] { return lifecycle::released; }));
    return {lifecycle::outcome, snapshot.revision + (lifecycle::wrong_revision ? 1 : 0),
            lifecycle::outcome == player_save_apply_outcome::applied ? 0U : 1205U};
}

// No connection is available in this test. Counter assertions distinguish a
// real read-admission refusal from merely failing later at pool acquisition.
MYSQL *sql_pool_acquire() { ++lifecycle::pool_calls; return nullptr; }
void sql_pool_release(MYSQL *) { std::abort(); }
void sql_pool_discard_connection(MYSQL *) { std::abort(); }
player_load_result player_load_repository_execute(MYSQL *, const player_load_request &)
{
    std::abort();
}
bool player_load_request_valid(const player_load_request &request, uint64_t now)
{
    return request.request_id && request.pid == 80 && request.deadline_usec > now;
}

template<class Predicate> void await(Predicate predicate)
{
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!predicate()) {
        assert(std::chrono::steady_clock::now() < limit);
        std::this_thread::yield();
    }
}
player_load_request load_request(bool recovery = false)
{
    player_load_request request = {};
    request.request_id = player_load_pipeline_next_request_id();
    request.pid = 80;
    request.player_name = "Probe";
    request.account_name = "owner";
    request.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
    request.include_items = !recovery;
    request.include_pets = !recovery;
    if (recovery)
        request.death_recovery_query.kind = player_death_recovery_query_kind::list;
    return request;
}
void assert_closed_reads()
{
    const unsigned before = lifecycle::pool_calls;
    auto request = load_request();
    player_load_result result = {};
    if (player_save_journal_pid_quarantined(request.pid)) {
        assert(!player_load_pipeline_execute_sync(request, &result));
        assert(player_load_pipeline_submit(request) == player_load_submit_outcome::unavailable);
        request = load_request(true);
        assert(!player_load_pipeline_execute_sync(request, &result));
        assert(player_load_pipeline_submit(request) == player_load_submit_outcome::unavailable);
        assert(lifecycle::pool_calls == before);
        return;
    }
    assert(player_load_pipeline_execute_sync(request, &result));
    if (lifecycle::pool_calls != before) {
        std::cerr << "FAIL: normal SQL read reached pool before replay readiness\n";
        std::abort();
    }
    assert(result.request_id == request.request_id);
    assert(result.outcome == player_load_outcome::component_failure);
    assert(result.recovery_gate == player_load_recovery_gate::unavailable);
    assert(result.failed_component && std::strcmp(result.failed_component, "save_journal_replay") == 0);
    assert(result.snapshot.status_integers.empty() && result.snapshot.items.empty() &&
           result.snapshot.pets.empty());
    request = load_request();
    assert(player_load_pipeline_wait(request, &result, 1000));
    assert(result.request_id == request.request_id);
    assert(result.outcome == player_load_outcome::component_failure);
    assert(result.recovery_gate == player_load_recovery_gate::unavailable);
    assert(lifecycle::pool_calls == before);
    // Read-only recovery uses its own authenticated SQL contract and is not
    // stopped by this fence. With no pool here it fails at the pool boundary.
    request = load_request(true);
    assert(player_load_pipeline_execute_sync(request, &result));
    assert(lifecycle::pool_calls == before + 1);
    assert(!result.failed_component || std::strcmp(result.failed_component, "save_journal_replay") != 0);
}
void begin_replay(const std::string &directory, player_save_apply_outcome outcome,
                  bool wrong_revision = false)
{
    {
        std::lock_guard<std::mutex> lock(lifecycle::mutex);
        lifecycle::entered = lifecycle::released = false;
        lifecycle::outcome = outcome;
        lifecycle::wrong_revision = wrong_revision;
    }
    assert(player_save_pipeline_init(directory.c_str()));
    std::unique_lock<std::mutex> lock(lifecycle::mutex);
    assert(lifecycle::changed.wait_for(lock, std::chrono::seconds(5),
                                      [] { return lifecycle::entered; }));
    assert(!player_save_pipeline_loads_allowed());
}
void release_replay()
{
    std::lock_guard<std::mutex> lock(lifecycle::mutex);
    lifecycle::released = true;
    lifecycle::changed.notify_all();
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    const std::string directory = argv[1];
    assert(player_load_pipeline_init());
    assert(!player_save_pipeline_loads_allowed());
    assert(player_save_journal_init(directory.c_str()));
    const auto death = terminal_death_snapshot();
    assert(player_snapshot_encode(death, &lifecycle::expected) == player_snapshot_codec_result::ok);
    assert(player_save_journal_append(death) == player_save_journal_result::ok);
    player_save_journal_shutdown();
    char_data untouched = {};
    pc_only_data untouched_pc = {};
    untouched.only.pc = &untouched_pc;
    untouched.player.level = 9;
    untouched_pc.pid = 900;
    const auto stale = stale_cold_load();
    const player_save_apply_outcome blocked[] = {
        player_save_apply_outcome::retryable_failure,
        player_save_apply_outcome::terminal_failure,
        player_save_apply_outcome::ambiguous_commit,
        player_save_apply_outcome::stale_revision,
        player_save_apply_outcome::applied,
    };
    unsigned case_number = 0;
    for (const auto outcome : blocked) {
        const std::string isolated = directory + "-outcome-" + std::to_string(++case_number);
        assert(player_save_journal_init(isolated.c_str()));
        assert(player_save_journal_append(death) == player_save_journal_result::ok);
        player_save_journal_shutdown();
        begin_replay(isolated, outcome, outcome == player_save_apply_outcome::applied);
        assert_closed_reads();
        assert(!player_load_materialize(&untouched, stale));
        release_replay();
        const bool retry = outcome == player_save_apply_outcome::retryable_failure ||
                           outcome == player_save_apply_outcome::ambiguous_commit;
        if (retry) {
            await([] { return player_save_pipeline_health_copy().replay_blocked; });
            assert(!player_save_pipeline_loads_allowed());
            assert(player_save_journal_health_copy().records == 1);
            assert(!player_save_journal_pid_quarantined(80));
        } else {
            await([] { return player_save_pipeline_health_copy().replay_complete; });
            assert(player_save_pipeline_loads_allowed());
            assert(player_save_journal_health_copy().records == 0);
            assert(player_save_journal_pid_quarantined(80));
        }
        assert_closed_reads();
        assert(!player_load_materialize(&untouched, stale));
        assert(untouched.player.level == 9 && untouched_pc.pid == 900);
        assert(reset_count == 0 && item_materialize_count == 0 && pet_stage_count == 0);
        player_save_pipeline_shutdown();
        assert(!player_save_pipeline_loads_allowed());
        assert(player_save_journal_init(isolated.c_str()));
        assert(player_save_journal_pid_quarantined(80) == !retry);
        assert(player_save_journal_health_copy().records == (retry ? 1 : 0));
        player_save_journal_shutdown();
    }
    // The same durable death (not an unrelated empty/ordinary journal) is now
    // exactly acknowledged through the real dispatcher and checkpoint code.
    begin_replay(directory, player_save_apply_outcome::applied);
    assert_closed_reads();
    release_replay();
    await([] { return player_save_pipeline_health_copy().replay_complete; });
    assert(player_save_pipeline_loads_allowed());
    assert(player_save_journal_health_copy().records == 0);
    char_data healthy = {};
    pc_only_data healthy_pc = {};
    healthy.only.pc = &healthy_pc;
    assert(player_load_materialize(&healthy, healthy_load()));
    assert(healthy.player.level == 61 && healthy_pc.pid == 81);
    assert(reset_count == 1 && item_materialize_count > 0 && pet_stage_count > 0);
    const auto calls = lifecycle::pool_calls.load();
    player_load_result result = {};
    assert(player_load_pipeline_execute_sync(load_request(), &result));
    assert(lifecycle::pool_calls == calls + 1); // healthy reads reach their backend
    player_save_pipeline_shutdown();
    assert(!player_save_pipeline_loads_allowed());
    assert_closed_reads();
    assert(!player_save_pipeline_init("not-an-absolute-journal-path"));
    assert(!player_save_pipeline_loads_allowed());
    assert_closed_reads();
    player_load_pipeline_shutdown();
    std::cout << "PASS: real SQL-build lifecycle; startup/transient/ambiguous replay deny reads and hydration; terminal/unproven PID groups stay fenced across restart; exact death replay opens; stop/init failure close\n";
}
'''

with tempfile.TemporaryDirectory(prefix="death-journal-pipeline-") as tmp:
    temporary = Path(tmp)
    source = temporary / "lifecycle.cpp"
    binary = temporary / "lifecycle"
    source.write_text(HARNESS)
    cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    libraries = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-ffunction-sections", "-fdata-sections", "-pthread", "-Isrc", *cflags, str(source),
        rel("player_snapshot_codec.c"), rel("player_save_journal.c"),
        rel("player_save_pipeline.c"), rel("player_save_worker.c"),
        rel("player_revision_state.c"), rel("persistence_observability.c"),
        rel("player_load_pipeline.c"), rel("player_load_materialize.c"),
        "-Wl,--gc-sections", *libraries, "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary), str(temporary / "journal")], check=True, timeout=45)
