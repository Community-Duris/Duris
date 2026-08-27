#!/usr/bin/env python3
"""Final Phase 01 ordering, ownership, fault, and bounded-load gate."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REDIS = (ROOT / "src/redis.c").read_text()
WORKER = (ROOT / "src/player_save_worker.c").read_text()
PIPELINE = (ROOT / "src/player_save_pipeline.c").read_text()
WORLD = (ROOT / "src/world_recovery_pipeline.c").read_text()
COMM = (ROOT / "src/comm.c").read_text()
COPYOVER = (ROOT / "src/copyover.c").read_text()
WIZREDIS = (ROOT / "src/wizredis.c").read_text()


HARNESS = r'''
#include "player_revision_state.h"
#include "player_save_worker.h"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <map>
#include <mutex>
#include <thread>

struct load_state
{
    std::mutex mutex;
    std::condition_variable changed;
    std::map<int, unsigned int> attempts;
    bool release = false;
};

player_save_apply_result apply(const player_snapshot &snapshot, void *raw)
{
    auto &state = *static_cast<load_state *>(raw);
    std::unique_lock<std::mutex> lock(state.mutex);
    state.changed.wait(lock, [&] { return state.release; });
    const unsigned int attempt = ++state.attempts[snapshot.pid];
    if (snapshot.pid % 17 == 0 && attempt == 1)
        return {player_save_apply_outcome::ambiguous_commit, snapshot.revision - 1, 2013};
    if (snapshot.pid % 23 == 0)
        return {player_save_apply_outcome::already_applied, snapshot.revision, 0};
    return {player_save_apply_outcome::applied, snapshot.revision, 0};
}

player_snapshot snapshot_for(int pid)
{
    assert(player_revision_hydrate(pid, 0));
    player_revision_t revision = 0;
    player_component_mask_t components = 0;
    assert(player_revision_mark(pid, PLAYER_COMPONENT_STATUS, &revision));
    assert(player_revision_queue(pid, &revision, &components));
    player_snapshot snapshot = {};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = pid;
    snapshot.revision = revision;
    snapshot.components = components;
    snapshot.encoded_size_bound = 4096;
    return snapshot;
}

void run_wave(int clients)
{
    player_revision_reset_for_tests();
    player_save_worker_reset_for_tests();
    load_state state;
    assert(player_save_worker_init(apply, &state, 4));
    const auto started = std::chrono::steady_clock::now();
    for (int index = 0; index < clients; ++index)
        assert(player_save_worker_submit(snapshot_for(10000 + index)) ==
               player_save_submit_result::accepted);
    auto queued = player_save_worker_health_copy();
    assert(queued.queued_pids + queued.inflight_pids == static_cast<uint64_t>(clients));
    assert(queued.queued_bytes <= PLAYER_SAVE_WORKER_MAX_BYTES);
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.release = true;
        state.changed.notify_all();
    }
    player_save_completion completions[256] = {};
    for (;;)
    {
        player_save_worker_pulse(completions, 256);
        const auto health = player_save_worker_health_copy();
        if (health.applied == static_cast<uint64_t>(clients))
            break;
        assert(std::chrono::steady_clock::now() - started < std::chrono::seconds(5));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const auto health = player_save_worker_health_copy();
    assert(health.queued_pids == 0 && health.inflight_pids == 0 && health.queued_bytes == 0);
    assert(health.high_water_pids == static_cast<uint64_t>(clients));
    assert(health.oldest_age_msec < PLAYER_SAVE_WORKER_MAX_AGE_MSEC);
    uint64_t expected_retries = 0;
    for (int index = 0; index < clients; ++index)
        if ((10000 + index) % 17 == 0)
            ++expected_retries;
    assert(health.retryable_failures == expected_retries);
    for (int index = 0; index < clients; ++index)
    {
        player_revision_snapshot revision = {};
        assert(player_revision_snapshot_copy(10000 + index, &revision));
        assert(revision.current_revision == 1 && revision.acknowledged_revision == 1);
        assert(revision.unacknowledged_components == 0);
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - started).count();
    std::printf("clients=%d high_water_pids=%llu high_water_bytes=%llu retries=%llu elapsed_us=%lld\n",
                clients, static_cast<unsigned long long>(health.high_water_pids),
                static_cast<unsigned long long>(health.high_water_bytes),
                static_cast<unsigned long long>(health.retryable_failures),
                static_cast<long long>(elapsed));
    player_save_worker_shutdown();
    player_save_worker_reset_for_tests();
    player_revision_reset_for_tests();
}

int main()
{
    for (const int clients : {25, 50, 100, 200})
        run_wave(clients);
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-phase01-gate-") as temp_dir:
    source = Path(temp_dir) / "phase01_gate.cpp"
    binary = Path(temp_dir) / "phase01_gate"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-pthread", "-Isrc", str(source), "src/player_save_worker.c",
            "src/player_revision_state.c", "src/persistence_observability.c",
            "-lmysqlclient", "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=20)


for retired in (
    "redis_poll_child", "redis_terminate_child", "world_state_save_pid",
    "redis_save_world_state_json", "redis_save_world_state_sync",
    "redis_load_world_state_json", "mud:dirty_players",
):
    assert retired not in REDIS
assert "fork(" not in REDIS and "waitpid(" not in REDIS
assert "player_queue" in WIZREDIS and "redis clear dirty" not in WIZREDIS

for contract in (
    "player_save_pipeline_request", "player_save_pipeline_checkpoint_dirty",
    "player_save_pipeline_terminal", "player_save_pipeline_drain",
):
    assert contract in PIPELINE
assert "player_snapshot_capture" in PIPELINE
assert "apply_callback(job->snapshot" in WORKER
assert "P_char" not in WORKER and "P_obj" not in WORKER
assert "publish_callback(blob.data(), blob.size(), &header" in WORLD
publisher = WORLD[WORLD.index("void publisher_main()"): WORLD.index("bool capture_one_record()")]
assert "P_char" not in publisher and "P_obj" not in publisher
assert "redis_world_recovery_drain(3000)" in COMM
assert "redis_world_recovery_drain(3000)" in COPYOVER

for existing_gate in (
    "test_player_save_journal.py", "test_player_save_worker.py",
    "test_terminal_save_safety.py", "test_world_recovery_pipeline.py",
):
    assert (ROOT / "tests/async" / existing_gate).is_file()

print("phase 01 recovery and bounded-load gate passed")
