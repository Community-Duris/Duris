#!/usr/bin/env python3
"""Runtime and source contracts for the bounded consistent player-load pipeline."""

import subprocess
import tempfile
import shlex
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (ROOT / "src/player_load_pipeline.c").read_text()
PIPELINE_HEADER = (ROOT / "src/player_load_pipeline.h").read_text()
REPOSITORY = (ROOT / "src/player_load_repository.c").read_text()
ACCOUNT = (ROOT / "src/account.c").read_text()
COPYOVER = (ROOT / "src/copyover.c").read_text()
COMM = (ROOT / "src/comm.c").read_text()
NANNY = (ROOT / "src/nanny.c").read_text()
MATERIALIZE = (ROOT / "src/player_load_materialize.c").read_text()
DIAGNOSTICS = (ROOT / "src/actinf.c").read_text()


HARNESS = r'''
#include "player_load_pipeline.h"
#include "persistence_observability.h"
#include "sql_pool.h"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

extern "C" MYSQL *sql_pool_acquire(void) { return nullptr; }
extern "C" void sql_pool_release(MYSQL *) {}

bool player_load_request_valid(const player_load_request &request, uint64_t now)
{
    return request.schema_version == PLAYER_LOAD_SCHEMA_VERSION && request.request_id > 0 &&
           request.pid > 0 && !request.account_name.empty() &&
           request.deadline_usec > now &&
           request.deadline_usec - now <= PLAYER_LOAD_TIMEOUT_USEC;
}

player_load_result player_load_repository_execute(MYSQL *, const player_load_request &request)
{
    player_load_result result = {};
    result.request_id = request.request_id;
    result.pid = request.pid;
    result.outcome = player_load_outcome::applied;
    return result;
}

struct callback_state
{
    std::mutex mutex;
    std::condition_variable changed;
    bool hold_started = false;
    bool release = false;
};

player_load_result execute(const player_load_request &request, void *raw)
{
    auto &state = *static_cast<callback_state *>(raw);
    if (request.request_id >= 1000)
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.hold_started = true;
        state.changed.notify_all();
        state.changed.wait(lock, [&] { return state.release; });
    }
    if (request.request_id == 4)
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    player_load_result result = {};
    result.request_id = request.request_id;
    result.pid = request.pid;
    result.outcome = player_load_outcome::applied;
    result.metrics.query_count = 7;
    result.metrics.row_count = 11;
    result.metrics.byte_count = 1234;
    result.metrics.transaction_usec = 456;
    if (request.request_id == 5)
        result.outcome = player_load_outcome::component_failure;
    if (request.request_id == 2)
    {
        result.request_id = 999;
        result.pid = request.pid + 1;
    }
    return result;
}

player_load_request request(uint64_t id, int pid)
{
    player_load_request value = {};
    value.request_id = id;
    value.pid = pid;
    value.account_name = "test-account";
    value.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
    return value;
}

template <typename Predicate> void wait_until(Predicate predicate)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!predicate())
    {
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main()
{
    callback_state state;
    player_load_pipeline_reset_for_tests();
    assert(player_load_pipeline_init(execute, &state));

    assert(player_load_pipeline_submit(request(1, 10)) ==
           player_load_submit_outcome::accepted);
    assert(player_load_pipeline_submit(request(1, 10)) ==
           player_load_submit_outcome::duplicate);
    assert(player_load_pipeline_submit(request(2, 20)) ==
           player_load_submit_outcome::accepted);

    player_load_result results[8] = {};
    size_t received = 0;
    wait_until([&] {
        received += player_load_pipeline_pulse(results + received, 8 - received);
        return received == 2;
    });
    assert(results[0].request_id == 1 && results[0].pid == 10);
    assert(results[0].outcome == player_load_outcome::applied);
    assert(results[1].request_id == 2);
    assert(results[1].outcome == player_load_outcome::stale);

    assert(player_load_pipeline_submit(request(3, 30)) ==
           player_load_submit_outcome::accepted);
    assert(player_load_pipeline_cancel(3));
    assert(player_load_pipeline_cancel(3));
    wait_until([&] { return player_load_pipeline_pulse(results, 8) == 1; });
    assert(results[0].request_id == 3);
    assert(results[0].outcome == player_load_outcome::cancelled);

    player_load_result timed = {};
    assert(!player_load_pipeline_wait(request(4, 40), &timed, 1));
    wait_until([&] { return player_load_pipeline_pulse(results, 8) == 1; });
    assert(results[0].request_id == 4);
    assert(results[0].outcome == player_load_outcome::cancelled);

    assert(player_load_pipeline_submit(request(5, 50)) ==
           player_load_submit_outcome::accepted);
    wait_until([&] { return player_load_pipeline_pulse(results, 8) == 1; });
    assert(results[0].request_id == 5);
    assert(results[0].outcome == player_load_outcome::component_failure);

    assert(player_load_pipeline_submit(request(1000, 1000)) ==
           player_load_submit_outcome::accepted);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait(lock, [&] { return state.hold_started; });
    }
    for (size_t index = 1; index < PLAYER_LOAD_MAX_PENDING; ++index)
        assert(player_load_pipeline_submit(request(1000 + index, 1000 + index)) ==
               player_load_submit_outcome::accepted);
    assert(player_load_pipeline_submit(request(9000, 9000)) ==
           player_load_submit_outcome::capacity_exceeded);
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.release = true;
        state.changed.notify_all();
    }
    player_load_pipeline_shutdown();

    const player_load_pipeline_health health = player_load_pipeline_health_copy();
    assert(!health.running);
    assert(health.applied >= 1);
    assert(health.stale >= 1);
    assert(health.cancelled == 2);
    assert(health.component_failures == 1);
    assert(health.high_water == PLAYER_LOAD_MAX_PENDING);
    assert(health.last_query_count == 7);
    assert(health.last_row_count == 11);
    assert(health.last_snapshot_bytes == 1234);
    assert(health.max_completion_latency_usec > 0);
    player_load_pipeline_reset_for_tests();
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-player-load-pipeline-") as temp_dir:
    source = Path(temp_dir) / "pipeline_test.cpp"
    binary = Path(temp_dir) / "pipeline_test"
    source.write_text(HARNESS)
    mysql_cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    mysql_libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-pthread",
            "-Isrc",
            *mysql_cflags,
            str(source),
            "src/player_load_pipeline.c",
            "src/persistence_observability.c",
            *mysql_libs,
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=10)

for contract in (
    "PLAYER_LOAD_MAX_PENDING = 256",
    "PLAYER_LOAD_MAX_COMPLETIONS = 256",
    "player_load_pipeline_cancel",
    "player_load_pipeline_note_stale",
    "player_load_pipeline_next_request_id",
    "last_completion_latency_usec",
):
    assert contract in PIPELINE_HEADER
for contract in (
    "std::unordered_set<uint64_t> active_ids",
    "std::unordered_set<uint64_t> cancelled_ids",
    "player_load_outcome::stale",
    "sql_pool_acquire()",
    "sql_worker_thread_init()",
    "pool_connection_guard guard",
    "mysql_rollback(connection)",
    "selected_execute_callback()",
    "flatfile_player_load_repository_execute_selected",
):
    assert contract in PIPELINE

for contract in (
    "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ",
    "START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY",
    'execute(connection, "COMMIT"',
    'execute(connection, "ROLLBACK"',
    "PLAYER_LOAD_QUERY_MAX",
    "PLAYER_SNAPSHOT_MAX_ROWS",
    "PLAYER_SNAPSHOT_MAX_BYTES",
    "PERSISTENCE_QUERY_CONTEXT_PLAYER_LOAD_WORKER",
):
    assert contract in REPOSITORY
assert REPOSITORY.index("load_status(connection") < REPOSITORY.index("load_components(connection")
assert REPOSITORY.index("load_components(connection") < REPOSITORY.index("load_bank(connection")

assert "restoreCharOnly(player" not in ACCOUNT
assert "player_load_pipeline_submit(request)" in ACCOUNT
assert "STATE(d) = CON_PLAYER_LOAD" in ACCOUNT
assert "player_load_materialize(player, loaded)" in ACCOUNT
completion = ACCOUNT[ACCOUNT.index("void account_player_load_complete") :]
recheck = completion.index('account_confirm_char(d, writable_arg("Y"))')
discard = completion.index("ready_player_loads.erase(completed_request_id)")
assert recheck < discard
assert "d->player_load_request_id = 0" in completion[discard:]
blocking = ACCOUNT[ACCOUNT.index("P_char load_char_into_game") : ACCOUNT.index("void account_player_load_complete")]
assert "ready_player_loads.emplace(request.request_id" not in blocking
assert "restoreCharOnly(player" not in COPYOVER
assert "player_load_pipeline_wait(request" in COPYOVER
assert "player_load_materialize(player, result)" in COPYOVER
assert "player_load_pipeline_cancel(d->player_load_request_id)" in COMM
assert "player_load_pipeline_shutdown()" in COMM
bank_load = NANNY.index("sql_load_account_bank(acct")
assert NANNY.rindex("d->player_load_mode == PLAYER_LOAD_MODE_NONE", 0, bank_load) < bank_load
assert "restoreCharOnly(d->character" not in NANNY
assert "d->player_load_mode = PLAYER_LOAD_MODE_LEGACY" in NANNY
assert "nanny_player_load_complete" in NANNY
assert "valid_snapshot(result)" in MATERIALIZE
assert "player_revision_hydrate" in MATERIALIZE
assert "ZONE_TROPHY(ch) = zone_trophies.release()" in MATERIALIZE
assert "player_load state=" in DIAGNOSTICS
assert "account_name" not in DIAGNOSTICS[DIAGNOSTICS.index("player_load state=") :][:1000]

print("bounded consistent player-load pipeline contracts passed")
