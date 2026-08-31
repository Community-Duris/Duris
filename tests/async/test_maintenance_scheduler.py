#!/usr/bin/env python3
"""Runtime and source contracts for the bounded maintenance scheduler."""

from _paths import SRC, rel
import subprocess
import tempfile
import shlex
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (SRC / "maintenance_scheduler.c").read_text()
COMM = (SRC / "comm.c").read_text()
COPYOVER = (SRC / "copyover.c").read_text()
SNAPSHOT = (SRC / "maintenance_snapshot.c").read_text()
REPOSITORY = (SRC / "maintenance_repository.c").read_text()
NEW_EVENTS = (SRC / "new_events.c").read_text()

HARNESS = r'''
#include "maintenance_scheduler.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

struct state_type
{
    std::mutex mutex;
    std::condition_variable changed;
    std::vector<maintenance_request> requests;
    bool hold = true;
    bool started = false;
    bool restart = false;
    size_t finished = 0;
};

maintenance_result execute(const maintenance_request &request, void *raw)
{
    auto &state = *static_cast<state_type *>(raw);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.requests.push_back(request);
        if (!state.restart && request.job_id == maintenance_job_id::auction_due_scan &&
            state.requests.size() == 1)
        {
            state.started = true;
            state.changed.notify_all();
            state.changed.wait(lock, [&] { return !state.hold; });
        }
    }
    maintenance_result result = {request.work_id, request.job_id,
                                 maintenance_outcome::complete, 0, 1, 0, 0, 0, {}};
    if (!state.restart && request.job_id == maintenance_job_id::auction_due_scan)
    {
        size_t auction_calls = 0;
        std::lock_guard<std::mutex> lock(state.mutex);
        for (const auto &seen : state.requests)
            if (seen.job_id == maintenance_job_id::auction_due_scan)
                ++auction_calls;
        if (auction_calls == 1)
            result.outcome = maintenance_outcome::retryable_failure;
        else if (auction_calls == 2)
        {
            result.outcome = maintenance_outcome::more;
            result.next_cursor = 77;
        }
    }
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        ++state.finished;
        state.changed.notify_all();
    }
    return result;
}

// Keep the harness deterministic: only the auction job may be submitted, so no
// other job can leave durable request or completion state behind across the
// simulated restarts below.
bool auction_only(maintenance_request &request, void *)
{
    return request.job_id == maintenance_job_id::auction_due_scan;
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

int main(int argc, char **argv)
{
    assert(argc == 2);
    size_t count = 0;
    const auto *registry = maintenance_registry(&count);
    assert(registry && count == MAINTENANCE_JOB_COUNT);
    const size_t lifecycle = static_cast<size_t>(maintenance_job_id::lifecycle_archive);
    assert(lifecycle < count && !registry[lifecycle].enabled);
    for (size_t index = 0; index < count; ++index)
    {
        assert(registry[index].row_budget <= MAINTENANCE_ROW_BUDGET_MAX);
        assert(registry[index].time_budget_usec <= MAINTENANCE_TIME_BUDGET_USEC_MAX);
        assert(maintenance_job_offset(registry[index].id, registry[index].cadence_ticks, 9) <
               registry[index].cadence_ticks);
        assert(maintenance_job_offset(registry[index].id, registry[index].cadence_ticks, 9) ==
               maintenance_job_offset(registry[index].id, registry[index].cadence_ticks, 9));
    }
    size_t distinct_offsets = 0;
    for (size_t index = 1; index < count; ++index)
        if (maintenance_job_offset(registry[index].id, registry[index].cadence_ticks, 9) !=
            maintenance_job_offset(registry[0].id, registry[0].cadence_ticks, 9))
            ++distinct_offsets;
    assert(distinct_offsets > 0);
    assert(!maintenance_activity_due(0, 0, 1));

    state_type state;
    assert(maintenance_scheduler_set_state_path(argv[1]));
    assert(maintenance_scheduler_init(9, execute, &state, auction_only));
    auto health = maintenance_scheduler_health_copy(0);
    assert(!health.jobs[lifecycle].enabled);
    const size_t auction = static_cast<size_t>(maintenance_job_id::auction_due_scan);
    const uint64_t first_tick = health.jobs[auction].next_due_tick;
    maintenance_result results[MAINTENANCE_COMPLETION_MAX] = {};
    maintenance_scheduler_pulse(first_tick, results, MAINTENANCE_COMPLETION_MAX);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait(lock, [&] { return state.started; });
    }
    maintenance_scheduler_pulse(first_tick + registry[auction].cadence_ticks, results,
                                MAINTENANCE_COMPLETION_MAX);
    health = maintenance_scheduler_health_copy(first_tick + registry[auction].cadence_ticks);
    assert(health.jobs[auction].overlap_suppressed == 1);
    maintenance_scheduler_quiesce();
    assert(!maintenance_scheduler_drain(1));
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.hold = false;
        state.changed.notify_all();
    }
	assert(maintenance_scheduler_drain(5000));
	maintenance_scheduler_resume();

    uint64_t tick = first_tick + registry[auction].cadence_ticks;
    // Stop pulsing as soon as the retry has been resubmitted. Any later pulse
    // could acknowledge its completion, which this phase must leave durable.
    wait_until([&] {
        if (maintenance_scheduler_health_copy(tick).jobs[auction].submitted >= 2)
            return true;
        maintenance_scheduler_pulse(++tick, results, MAINTENANCE_COMPLETION_MAX);
        return false;
    });
	{
		std::unique_lock<std::mutex> lock(state.mutex);
		state.changed.wait(lock, [&] { return state.finished >= 2; });
	}

    std::vector<maintenance_request> auction_requests;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        for (const auto &request : state.requests)
            if (request.job_id == maintenance_job_id::auction_due_scan)
                auction_requests.push_back(request);
    }
    assert(auction_requests.size() == 2);
    assert(auction_requests[0].work_id == auction_requests[1].work_id);
    assert(auction_requests[0].cursor == 0 && auction_requests[1].cursor == 0);
    const uint64_t persisted_work_id = auction_requests[0].work_id;

    // The worker persisted the exact continuation before publishing its completion.
    // Shutdown here simulates a restart between those two operations.
    maintenance_scheduler_shutdown();
    assert(!maintenance_scheduler_health_copy(tick).running);

	state_type restarted;
	restarted.hold = false;
	restarted.restart = true;
	assert(maintenance_scheduler_init(9, execute, &restarted, auction_only));
	assert(maintenance_scheduler_health_copy(0).completions == 1);
	maintenance_scheduler_pulse(0, results, MAINTENANCE_COMPLETION_MAX);
	maintenance_scheduler_pulse(1, results, MAINTENANCE_COMPLETION_MAX);
	wait_until([&] {
		std::lock_guard<std::mutex> lock(restarted.mutex);
		return restarted.finished >= 1;
	});
	maintenance_request resumed = {};
	{
		std::lock_guard<std::mutex> lock(restarted.mutex);
		assert(restarted.requests.size() == 1);
		resumed = restarted.requests[0];
	}
	assert(resumed.job_id == maintenance_job_id::auction_due_scan);
	assert(resumed.work_id == persisted_work_id);
	assert(resumed.cursor == 77);
	wait_until([&] { return maintenance_scheduler_health_copy(2).completions == 1; });
	maintenance_scheduler_pulse(2, results, MAINTENANCE_COMPLETION_MAX);
	health = maintenance_scheduler_health_copy(2);
	assert(!health.jobs[auction].active);
	assert(health.jobs[auction].cursor == 0);
    maintenance_scheduler_reset_for_tests();
	state_type acknowledged;
	acknowledged.hold = false;
	acknowledged.restart = true;
	assert(maintenance_scheduler_set_state_path(argv[1]));
	assert(maintenance_scheduler_init(9, execute, &acknowledged, auction_only));
	assert(maintenance_scheduler_health_copy(0).completions == 0);
	maintenance_scheduler_shutdown();
	maintenance_scheduler_reset_for_tests();
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-maintenance-scheduler-") as temp_dir:
    source = Path(temp_dir) / "scheduler_test.cpp"
    binary = Path(temp_dir) / "scheduler_test"
    source.write_text(HARNESS)
    mysql_cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    mysql_libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run(
        ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
         "-pthread", "-Isrc", *mysql_cflags, str(source), rel("maintenance_scheduler.c"),
         rel("persistence_observability.c"), *mysql_libs, "-o", str(binary)],
        cwd=ROOT, check=True,
    )
    state_file = Path(temp_dir) / "scheduler.state"
    subprocess.run([str(binary), str(state_file)], check=True, timeout=10)
    current = state_file.read_bytes()
    job_size = (len(current) - 24) // 12
    assert 16 + job_size * 12 + 8 == len(current)
    legacy = bytearray(b"DMSMNT2\0" + struct.pack("=II", 2, 11))
    legacy.extend(current[16:16 + job_size * 11])
    checksum = 1469598103934665603
    for byte in legacy:
        checksum ^= byte
        checksum = (checksum * 1099511628211) & ((1 << 64) - 1)
    legacy.extend(struct.pack("=Q", checksum))
    state_file.write_bytes(legacy)
    subprocess.run([str(binary), str(state_file)], check=True, timeout=10)

for contract in (
    "MAINTENANCE_QUEUE_MAX", "maintenance_job_offset", "overlap_suppressed",
    "MAINTENANCE_RETRY_MAX_TICKS", "next_cursor", "time_budget_usec",
    "maintenance_scheduler_shutdown", "maintenance_scheduler_quiesce",
    "maintenance_scheduler_drain", "persist_state",
):
    assert contract in SOURCE or contract in (SRC / "maintenance_scheduler.h").read_text()

assert "J_NAME" not in SOURCE and "account_name" not in SOURCE

for aligned_callback in (
    "auction_houses_activity();", "timers_activity();", "web_info();",
    "epic_zone_balance();", "update_epic_zone_mods();", "boon_maintenance();",
):
    assert aligned_callback not in COMM
assert "add_event(0, 0, 0, event_write_statistic" not in NEW_EVENTS
for forbidden_io in ("mysql_", "qry(", "redis_", "open(", "write(", "rename(", "fsync("):
    assert forbidden_io not in SNAPSHOT
for copyover_contract in (
    "maintenance_scheduler_quiesce();", "maintenance_scheduler_drain(3000)",
    "maintenance_scheduler_resume();",
):
    assert copyover_contract in COPYOVER
for repository_contract in (
    "ORDER BY id LIMIT ", "ORDER BY pid,zone_number LIMIT ", "before_deadline(request)",
    "maintenance_level_cap", "maintenance_cargo", "maintenance_boon",
    "level_cap_changed", "mysql_affected_rows(connection) == 1",
    "atomic_replace", "sql_pool_acquire()",
):
    assert repository_contract in REPOSITORY
assert "result.outcome == maintenance_outcome::complete && result.rows > 0" in COMM
print("bounded maintenance scheduler contracts passed")
