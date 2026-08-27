#!/usr/bin/env python3
"""Runtime and source contracts for the keyed revision-guarded player save worker."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
WORKER = (ROOT / "src/player_save_worker.c").read_text()
WORKER_HEADER = (ROOT / "src/player_save_worker.h").read_text()
REPOSITORY = (ROOT / "src/player_snapshot_repository.c").read_text()
DIAGNOSTICS = (ROOT / "src/actinf.c").read_text()


HARNESS = r'''
#include "player_save_worker.h"
#include "player_revision_state.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

struct apply_state
{
    std::mutex mutex;
    std::condition_variable changed;
    std::map<int, std::vector<player_revision_t>> revisions;
    std::map<int, unsigned int> attempts;
    unsigned int active = 0;
    unsigned int max_active = 0;
    bool first_started = false;
    bool release_first = false;
};

struct capacity_state
{
    std::mutex mutex;
    std::condition_variable changed;
    bool started = false;
    bool release = false;
};

struct journal_hook_state
{
    unsigned int appends = 0;
    unsigned int acknowledgements = 0;
};

bool journal_append(const player_snapshot &, void *raw)
{
    ++static_cast<journal_hook_state *>(raw)->appends;
    return true;
}

bool journal_ack(int, player_revision_t, void *raw)
{
    ++static_cast<journal_hook_state *>(raw)->acknowledgements;
    return true;
}

player_save_apply_result apply_snapshot(const player_snapshot &snapshot, void *raw)
{
    auto &state = *static_cast<apply_state *>(raw);
    std::unique_lock<std::mutex> lock(state.mutex);
    ++state.active;
    state.max_active = std::max(state.max_active, state.active);
    ++state.attempts[snapshot.pid];
    if (snapshot.pid == 1 && snapshot.revision == 1)
    {
        state.first_started = true;
        state.changed.notify_all();
        state.changed.wait(lock, [&] { return state.release_first; });
    }
    state.revisions[snapshot.pid].push_back(snapshot.revision);
    const bool retry = snapshot.pid == 3 && state.attempts[snapshot.pid] == 1;
    --state.active;
    state.changed.notify_all();
    return {retry ? player_save_apply_outcome::retryable_failure
                  : player_save_apply_outcome::applied,
            retry ? snapshot.revision - 1 : snapshot.revision,
            retry ? 1213U : 0U};
}

player_save_apply_result hold_snapshot(const player_snapshot &snapshot, void *raw)
{
    auto &state = *static_cast<capacity_state *>(raw);
    std::unique_lock<std::mutex> lock(state.mutex);
    state.started = true;
    state.changed.notify_all();
    state.changed.wait(lock, [&] { return state.release; });
    return {player_save_apply_outcome::applied, snapshot.revision, 0};
}

player_snapshot next_snapshot(int pid, player_component_mask_t newly_dirty)
{
    player_revision_t revision = 0;
    player_component_mask_t components = 0;
    assert(player_revision_mark(pid, newly_dirty, &revision));
    assert(player_revision_queue(pid, &revision, &components));
    player_snapshot snapshot = {};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = pid;
    snapshot.revision = revision;
    snapshot.components = components;
    snapshot.encoded_size_bound = 256;
    return snapshot;
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
    player_revision_reset_for_tests();
    player_save_worker_reset_for_tests();
    apply_state state;
    assert(player_save_worker_init(apply_snapshot, &state, 2));

    assert(player_revision_hydrate(1, 0));
    assert(player_revision_hydrate(2, 0));
    assert(player_revision_hydrate(3, 0));
    assert(player_save_worker_submit(next_snapshot(1, PLAYER_COMPONENT_STATUS)) ==
           player_save_submit_result::accepted);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait(lock, [&] { return state.first_started; });
    }

    assert(player_save_worker_submit(next_snapshot(1, PLAYER_COMPONENT_SKILLS)) ==
           player_save_submit_result::coalesced);
    assert(player_save_worker_submit(next_snapshot(2, PLAYER_COMPONENT_AFFECTS)) ==
           player_save_submit_result::accepted);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait(lock, [&] { return !state.revisions[2].empty(); });
        assert(state.max_active == 2);
        state.release_first = true;
        state.changed.notify_all();
    }

    player_save_completion completions[8] = {};
    wait_until([&] {
        player_save_worker_pulse(completions, 8);
        return player_save_worker_health_copy().applied == 3;
    });
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        assert((state.revisions[1] == std::vector<player_revision_t>{1, 2}));
        assert((state.revisions[2] == std::vector<player_revision_t>{1}));
    }

    player_revision_snapshot revision = {};
    assert(player_revision_snapshot_copy(1, &revision));
    assert(revision.acknowledged_revision == 2);
    assert(revision.unacknowledged_components == 0);
    assert(revision.inflight_components == 0);

    assert(player_save_worker_submit(next_snapshot(3, PLAYER_COMPONENT_TROPHIES)) ==
           player_save_submit_result::accepted);
    wait_until([&] {
        player_save_worker_pulse(completions, 8);
        return player_save_worker_health_copy().applied == 4;
    });
    const player_save_worker_health health = player_save_worker_health_copy();
    assert(health.retryable_failures == 1);
    assert(health.queued_pids == 0);
    assert(health.inflight_pids == 0);
    assert(health.queued_bytes == 0);
    assert(health.high_water_pids >= 3);
    assert(health.max_revision_gap == 1);

    player_save_worker_shutdown();
    assert(!player_save_worker_health_copy().running);
    player_save_worker_reset_for_tests();
    player_revision_reset_for_tests();

    apply_state durable_apply;
    journal_hook_state journal_hooks;
    assert(player_save_worker_set_journal_hooks(journal_append, journal_ack, &journal_hooks));
    assert(player_save_worker_init(apply_snapshot, &durable_apply, 1));
    assert(player_revision_hydrate(4, 0));
    assert(player_save_worker_submit(next_snapshot(4, PLAYER_COMPONENT_STATUS)) ==
           player_save_submit_result::accepted);
    wait_until([&] {
        player_save_worker_pulse(completions, 8);
        return player_save_worker_health_copy().applied == 1;
    });
    assert(journal_hooks.appends == 1 && journal_hooks.acknowledgements == 1);
    player_save_worker_shutdown();
    assert(player_revision_hydrate(5, 0));
    assert(player_save_worker_submit(next_snapshot(5, PLAYER_COMPONENT_STATUS)) ==
           player_save_submit_result::durably_spilled);
    assert(journal_hooks.appends == 2 && journal_hooks.acknowledgements == 1);
    player_save_worker_reset_for_tests();
    player_revision_reset_for_tests();

    capacity_state capacity;
    assert(player_save_worker_init(hold_snapshot, &capacity, 1));
    for (int pid = 1000; pid < 1000 + static_cast<int>(PLAYER_SAVE_WORKER_MAX_PIDS); ++pid)
    {
        assert(player_revision_hydrate(pid, 0));
        assert(player_save_worker_submit(next_snapshot(pid, PLAYER_COMPONENT_STATUS)) ==
               player_save_submit_result::accepted);
    }
    {
        std::unique_lock<std::mutex> lock(capacity.mutex);
        capacity.changed.wait(lock, [&] { return capacity.started; });
    }
    assert(player_revision_hydrate(9999, 0));
    assert(player_save_worker_submit(next_snapshot(9999, PLAYER_COMPONENT_STATUS)) ==
           player_save_submit_result::capacity_exceeded);
    assert(player_save_worker_health_copy().high_water_pids == PLAYER_SAVE_WORKER_MAX_PIDS);
    {
        std::lock_guard<std::mutex> lock(capacity.mutex);
        capacity.release = true;
        capacity.changed.notify_all();
    }
    player_save_worker_shutdown();
    player_save_worker_reset_for_tests();
    player_revision_reset_for_tests();
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-player-save-worker-") as temp_dir:
    source = Path(temp_dir) / "worker_test.cpp"
    binary = Path(temp_dir) / "worker_test"
    source.write_text(HARNESS)
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
            str(source),
            "src/player_save_worker.c",
            "src/player_revision_state.c",
            "src/persistence_observability.c",
            "-lmysqlclient",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True, timeout=10)

for contract in (
    "PLAYER_SAVE_WORKER_MAX_PIDS = 128",
    "PLAYER_SAVE_WORKER_MAX_RESULTS = 256",
    "PLAYER_SAVE_WORKER_MAX_BYTES = 32 * 1024 * 1024",
    "PLAYER_SAVE_WORKER_MAX_AGE_MSEC",
    "PLAYER_SAVE_WORKER_MAX_RETRIES",
):
    assert contract in WORKER_HEADER
for contract in (
    "std::unordered_map<int, pid_slot> slots",
    "std::unique_ptr<queued_snapshot> active",
    "std::unique_ptr<queued_snapshot> pending",
    "player_revision_begin_inflight",
    "player_revision_acknowledge",
    "player_revision_fail_inflight",
):
    assert contract in WORKER
assert WORKER.index("apply_callback(job->snapshot") < WORKER.index("results.push_back")
assert "apply_callback(job->snapshot, apply_context)" in WORKER
print("[PASS] keyed queue orders one PID, coalesces cumulative work, and permits cross-PID parallelism")

for contract in (
    'execute(connection, "START TRANSACTION")',
    '"SELECT save_revision FROM player_data WHERE pid="',
    '" FOR UPDATE"',
    "durable >= snapshot.revision",
    'execute(connection, "COMMIT")',
    "mysql_affected_rows(connection) != 1",
    "ambiguous_commit",
    "read_durable_revision",
    "sql_pool_replace_connection",
):
    assert contract in REPOSITORY
assert REPOSITORY.index("durable >= snapshot.revision") < REPOSITORY.index(
    "apply_components(connection, snapshot)"
)
print("[PASS] repository locks revision before components and reconciles ambiguous commits")

for component in (
    "PLAYER_COMPONENT_STATUS",
    "PLAYER_COMPONENT_LANGUAGES",
    "PLAYER_COMPONENT_INTRODUCTIONS",
    "PLAYER_COMPONENT_TIMERS",
    "PLAYER_COMPONENT_UNDEAD_SLOTS",
    "PLAYER_COMPONENT_FORGED_ITEMS",
    "PLAYER_COMPONENT_GRANTED_COMMANDS",
    "PLAYER_COMPONENT_SKILLS",
    "PLAYER_COMPONENT_AFFECTS",
    "PLAYER_COMPONENT_EQUIPMENT",
    "PLAYER_COMPONENT_INVENTORY",
    "PLAYER_COMPONENT_PETS",
    "PLAYER_COMPONENT_SHAPECHANGES",
    "PLAYER_COMPONENT_TROPHIES",
):
    assert component in REPOSITORY
for table in (
    "player_data",
    "player_languages",
    "player_intros",
    "player_timers",
    "player_undead_slots",
    "player_forged_items",
    "player_granted_cmds",
    "player_skills",
    "player_affects",
    "player_items",
    "player_item_affects",
    "player_item_extra_descr",
    "player_pets",
    "player_pet_items",
    "player_shapechanges",
    "zone_trophy",
):
    assert table in REPOSITORY
assert "P_char" not in REPOSITORY and "P_obj" not in REPOSITORY
print("[PASS] all checkpoint components route through pointer-free typed repositories")

for metric in (
    "queued_pids",
    "inflight_pids",
    "queued_bytes",
    "oldest_age_msec",
    "age_limit_exceeded",
    "retryable_failures",
    "max_capture_to_apply_usec",
    "max_apply_usec",
    "max_ack_latency_usec",
    "max_revision_gap",
):
    assert metric in WORKER_HEADER and metric in DIAGNOSTICS
assert "player_save state=" in DIAGNOSTICS
print("[PASS] bounded redacted worker health is exposed through persistence diagnostics")

print("keyed revision-guarded player save worker contracts passed")
