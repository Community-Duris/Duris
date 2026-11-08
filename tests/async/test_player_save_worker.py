#!/usr/bin/env python3
"""The one persistence writer applies every save in capture order."""

from _paths import SRC, rel
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
WORKER = (SRC / "player_save_worker.c").read_text()
WORKER_HEADER = (SRC / "player_save_worker.h").read_text()
REPOSITORY = (SRC / "player_snapshot_repository.c").read_text()
DIAGNOSTICS = (SRC / "actinf.c").read_text()
LOCKER = (SRC / "locker_async.c").read_text()

assert "std::unordered_set<std::string> description_keys" in REPOSITORY
assert "if (!description_keys.insert(std::move(description_key)).second)" in REPOSITORY


HARNESS = r'''
#include "player/player_save_worker.h"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <new>
#include <cerrno>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Fault injection is thread-local: admission and post-ACK delivery can fail
// independently without disturbing the other thread or test bookkeeping.
static thread_local int allocation_fail_after = -1;
void *operator new(size_t bytes)
{
    if (allocation_fail_after >= 0 && allocation_fail_after-- == 0)
        throw std::bad_alloc();
    if (void *memory = std::malloc(bytes ? bytes : 1)) return memory;
    throw std::bad_alloc();
}
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, size_t) noexcept { std::free(memory); }

struct apply_state
{
    std::mutex mutex;
    std::condition_variable changed;
    std::vector<std::string> order;
    std::map<int, unsigned int> attempts;
    bool first_started = false;
    bool release_first = false;
    // Pid 12 blocks as a query on a locked table does, until shutdown interrupts it.
    bool stuck_started = false;
    unsigned int interrupts = 0;
    // Pid 13 blocks as opening a new connection does: the interrupt cannot end it.
    bool connecting = false;
    bool connected = false;
};

apply_state state;

std::string label(const char *kind, uint64_t owner, uint64_t revision)
{
    return std::string(kind) + std::to_string(owner) + ":" + std::to_string(revision);
}

player_save_apply_result apply_snapshot(const player_snapshot &snapshot, void *)
{
    std::unique_lock<std::mutex> lock(state.mutex);
    ++state.attempts[snapshot.pid];
    if (snapshot.pid == 20)
    {
        assert(snapshot.schema_version == PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION);
        assert(snapshot.death && snapshot.death->corpse_room_vnum == 3001);
        assert(snapshot.output_preferences == "death disposition must survive admission refusal");
    }
    if (snapshot.pid == 1 && snapshot.revision == 1)
    {
        state.first_started = true;
        state.changed.notify_all();
        state.changed.wait(lock, [] { return state.release_first; });
    }
    if (snapshot.pid == 12)
    {
        state.stuck_started = true;
        state.changed.notify_all();
        state.changed.wait(lock, [] { return state.interrupts > 0; });
        return {player_save_apply_outcome::retryable_failure, 0, 2013};
    }
    if (snapshot.pid == 13)
    {
        state.connecting = true;
        state.changed.notify_all();
        state.changed.wait(lock, [] { return state.connected; });
        return {player_save_apply_outcome::retryable_failure, 0, 2013};
    }
    // Pid 5 loses its connection twice, pid 9 never gets it back.
    if ((snapshot.pid == 5 && state.attempts[5] <= 2) || snapshot.pid == 9)
        return {player_save_apply_outcome::retryable_failure, 0, 2013};
    state.order.push_back(label("p", snapshot.pid, snapshot.revision));
    state.changed.notify_all();
    if (snapshot.pid == 8)
        return {player_save_apply_outcome::terminal_failure, 0, 1452};
    return {player_save_apply_outcome::applied, snapshot.revision, 0};
}

player_snapshot snapshot_for(int pid, player_revision_t revision)
{
    player_snapshot snapshot = {};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = pid;
    snapshot.revision = revision;
    snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
    snapshot.encoded_size_bound = 256;
    return snapshot;
}

persistence_job_write_fn locker_job(uint64_t locker)
{
    return [locker] {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.order.push_back(label("l", locker, 0));
        return player_save_apply_result{player_save_apply_outcome::applied, 0, 0};
    };
}

template <typename Predicate> void wait_until(Predicate predicate)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!predicate())
    {
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// What sql_pool_interrupt_borrowed() does to a blocked query: it returns as a lost
// connection.
void interrupt_writer()
{
    std::lock_guard<std::mutex> lock(state.mutex);
    ++state.interrupts;
    state.changed.notify_all();
}

std::vector<player_save_completion> drain()
{
    std::vector<player_save_completion> all;
    player_save_completion completions[16] = {};
    for (size_t count; (count = player_save_worker_pulse(completions, 16)) > 0;)
        all.insert(all.end(), completions, completions + count);
    return all;
}

static void test_admission_allocation_failure()
{
    player_save_worker_reset_for_tests();
    assert(player_save_worker_init(apply_snapshot, nullptr));
    assert(player_save_worker_submit(snapshot_for(1, 1)) == player_save_submit_result::accepted);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait(lock, [] { return state.first_started; });
    }
    unsigned int failures = 0;
    bool errors_reported = true;
    bool admitted = false;
    for (int index = 0; index < 8; ++index)
    {
        auto retained = snapshot_for(14, 1);
        retained.output_preferences = "retain this complete payload after either allocation fails";
        allocation_fail_after = index;
        const auto result = player_save_worker_submit_retained(&retained);
        allocation_fail_after = -1;
        if (result == player_save_submit_result::accepted)
        {
            admitted = true;
            break;
        }
        ++failures;
        errors_reported = errors_reported && errno == ENOMEM;
        assert(result == player_save_submit_result::capacity_exceeded);
        assert(retained.pid == 14 && retained.revision == 1);
        assert(retained.output_preferences == "retain this complete payload after either allocation fails");
        assert(!player_save_worker_pid_pending(14));
        assert(persistence_writer_pending_owners().size() == 1);
        const auto health = player_save_worker_health_copy();
        assert(health.queued_jobs == 0 && health.queued_bytes == 0 && health.submitted == 1);
    }
    // Both the list-node and owner-index admission failures leave no partial job.
    assert(admitted && failures >= 2 && errors_reported);
    auto replacement = snapshot_for(14, 2);
    replacement.encoded_size_bound = 1024;
    allocation_fail_after = 0;
    const auto replaced = player_save_worker_submit(std::move(replacement));
    const bool allocated = allocation_fail_after != 0;
    allocation_fail_after = -1;
    assert(replaced == player_save_submit_result::replaced && !allocated);
    assert(player_save_worker_pid_pending(14));
    assert(persistence_writer_pending_owners().size() == 2);
    const auto health = player_save_worker_health_copy();
    assert(health.queued_jobs == 1 && health.queued_bytes == 1024);
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.release_first = true;
        state.changed.notify_all();
    }
    assert(persistence_writer_wait_idle(5000));
    assert((state.order == std::vector<std::string>{"p1:1", "p14:2"}));
    assert(drain().size() == 2);
    player_save_worker_reset_for_tests();
    state.order.clear();
    state.attempts.clear();
    state.first_started = state.release_first = false;
}

static unsigned int journal_acks = 0;
static unsigned int journal_appends = 0;
static bool append_snapshot(const player_snapshot &, void *) { ++journal_appends; return true; }
static bool acknowledge_snapshot(int pid, player_revision_t revision, void *context)
{
    assert(pid == 20 && revision == 2 && context == &journal_acks);
    return ++journal_acks > 1; // A failed checkpoint must be retried before completion.
}

static void test_journal_and_death_admission()
{
    player_save_worker_reset_for_tests();
    auto death = snapshot_for(20, 2);
    death.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
    death.death.emplace();
    death.death->corpse_room_vnum = 3001;
    death.output_preferences = "death disposition must survive admission refusal";
    assert(player_save_worker_submit_retained(&death) == player_save_submit_result::worker_unavailable);
    assert(death.death && death.death->corpse_room_vnum == 3001);
    assert(death.output_preferences == "death disposition must survive admission refusal");
    assert(player_save_worker_init(apply_snapshot, nullptr));
    assert(player_save_worker_set_journal_hooks(append_snapshot, acknowledge_snapshot, &journal_acks));
    std::mutex gate_mutex;
    std::condition_variable gate_changed;
    bool started = false, release = false;
    assert(persistence_writer_submit(persistence_job_kind::log, 99,
        PLAYER_SAVE_WORKER_MAX_PENDING_BYTES, [&] {
            std::unique_lock<std::mutex> lock(gate_mutex);
            started = true;
            gate_changed.notify_all();
            gate_changed.wait(lock, [&] { return release; });
            return player_save_apply_result{player_save_apply_outcome::applied, 0, 0};
        }) == player_save_submit_result::accepted);
    {
        std::unique_lock<std::mutex> lock(gate_mutex);
        gate_changed.wait(lock, [&] { return started; });
    }
    assert(player_save_worker_submit_retained(&death) == player_save_submit_result::capacity_exceeded);
    assert(death.death && death.death->corpse_room_vnum == 3001);
    assert(death.output_preferences == "death disposition must survive admission refusal");
    {
        std::lock_guard<std::mutex> lock(gate_mutex);
        release = true;
        gate_changed.notify_all();
    }
    assert(persistence_writer_wait_idle(5000));
    drain();
    // Schema and optional death payload must agree; neither malformed combination is admitted.
    auto malformed = death;
    malformed.death.reset();
    assert(player_save_worker_submit(malformed) == player_save_submit_result::invalid);
    malformed = death;
    malformed.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    assert(player_save_worker_submit(malformed) == player_save_submit_result::invalid);
    assert(player_save_worker_submit_retained(&death) == player_save_submit_result::accepted);
    assert(persistence_writer_wait_idle(5000));
    const auto completed = drain();
    assert(completed.size() == 1 && completed[0].pid == 20 && completed[0].revision == 2);
    assert(completed[0].outcome == player_save_apply_outcome::applied);
    assert(journal_acks == 2 && journal_appends == 2);
    assert(state.attempts[20] == 2);
    player_save_worker_reset_for_tests();
    state.order.clear();
    state.attempts.clear();
}

static unsigned int delivery_acks = 0;
static unsigned int delivery_writes = 0;
static bool delivery_allocation_attempted = false;
static player_save_apply_result apply_without_allocation(const player_snapshot &snapshot, void *)
{
    // The previous ACK armed failure. Moving the next queued job and delivering
    // its predecessor must not allocate, even when the completion ring wraps.
    if (delivery_writes && allocation_fail_after != 0)
        delivery_allocation_attempted = true;
    allocation_fail_after = -1;
    ++delivery_writes;
    return {player_save_apply_outcome::applied, snapshot.revision, 0};
}
static bool acknowledge_without_allocation(int, player_revision_t, void *)
{
    ++delivery_acks;
    allocation_fail_after = 0;
    return true;
}
static void test_delivery_after_ack()
{
    player_save_worker_reset_for_tests();
    assert(player_save_worker_init(apply_without_allocation, nullptr));
    assert(player_save_worker_set_journal_hooks(nullptr, acknowledge_without_allocation, nullptr));
    // Fill every reserved completion slot, check admission backpressure, consume
    // half, then refill to exercise wraparound with the writer allocation denied.
    const size_t limit = PLAYER_SAVE_WORKER_MAX_PENDING_JOBS;
    for (size_t i = 0; i < limit; ++i)
        assert(player_save_worker_submit(snapshot_for(1000 + i, 1)) == player_save_submit_result::accepted);
    assert(persistence_writer_wait_idle(10000));
    assert(player_save_worker_health_copy().completion_jobs == limit);
    assert(player_save_worker_submit(snapshot_for(999, 1)) == player_save_submit_result::unavailable);
    std::vector<player_save_completion> completed(limit);
    assert(player_save_worker_pulse(completed.data(), limit / 2) == limit / 2);
    for (size_t i = 0; i < limit / 2; ++i) {
        assert(completed[i].pid == static_cast<int>(1000 + i));
        assert(completed[i].outcome == player_save_apply_outcome::applied);
        assert(!player_save_worker_pid_pending(1000 + i));
        assert(player_save_worker_submit(snapshot_for(1000 + limit + i, 1)) == player_save_submit_result::accepted);
    }
    assert(persistence_writer_wait_idle(10000));
    assert(player_save_worker_pulse(completed.data(), limit) == limit);
    for (size_t i = 0; i < limit; ++i) {
        assert(completed[i].pid == static_cast<int>(1000 + limit / 2 + i));
        assert(completed[i].revision == 1 && completed[i].durable_revision == 1);
        assert(completed[i].outcome == player_save_apply_outcome::applied);
    }
    assert(delivery_acks == limit + limit / 2 && delivery_writes == delivery_acks);
    assert(!delivery_allocation_attempted);
    assert(player_save_worker_health_copy().completion_jobs == 0);
    player_save_worker_reset_for_tests();
}

int main()
{
    test_delivery_after_ack();
    test_journal_and_death_admission();
    test_admission_allocation_failure();
    player_save_worker_reset_for_tests();
    assert(player_save_worker_init(apply_snapshot, nullptr));
    assert(!player_save_worker_init(apply_snapshot, nullptr));

    // Pid 1 revision 1 is being written while the rest queue behind it.
    assert(player_save_worker_submit(snapshot_for(1, 1)) == player_save_submit_result::accepted);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait(lock, [] { return state.first_started; });
    }
    assert(player_save_worker_pid_pending(1));
    assert(player_save_worker_submit(snapshot_for(2, 1)) == player_save_submit_result::accepted);
    assert(persistence_writer_submit(persistence_job_kind::locker, 7, 64, locker_job(7)) ==
           player_save_submit_result::accepted);
    // A newer save of an owner being written queues behind it; it replaces nothing.
    assert(player_save_worker_submit(snapshot_for(1, 2)) == player_save_submit_result::accepted);
    assert(player_save_worker_submit(snapshot_for(3, 1)) == player_save_submit_result::accepted);
    // A newer save of a queued owner does not overtake what was queued after that
    // owner's save: it queues behind. Only the last job queued is replaced.
    assert(player_save_worker_submit(snapshot_for(2, 2)) == player_save_submit_result::accepted);
    assert(player_save_worker_submit(snapshot_for(2, 3)) == player_save_submit_result::replaced);
    assert(persistence_writer_pending(persistence_job_kind::locker, 7));
    assert(!persistence_writer_pending(persistence_job_kind::corpse, 7));
    assert(persistence_writer_submit(persistence_job_kind::player, 4, 1, locker_job(4)) ==
           player_save_submit_result::invalid);
    assert(player_save_worker_submit(snapshot_for(0, 1)) == player_save_submit_result::invalid);

    const auto owners = persistence_writer_pending_owners();
    assert(owners.size() == 6);
    assert(owners[0] == persistence_job_owner(persistence_job_kind::player, 1));
    assert(owners[1] == persistence_job_owner(persistence_job_kind::player, 2));
    assert(owners[5] == persistence_job_owner(persistence_job_kind::player, 2));
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.release_first = true;
        state.changed.notify_all();
    }
    assert(persistence_writer_wait_idle(5000));
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        const std::vector<std::string> expected = {"p1:1", "p2:1", "l7:0", "p1:2",
                                                   "p3:1", "p2:3"};
        assert(state.order == expected);
    }
    assert(player_save_worker_pid_pending(1)); // completion still belongs to the old session
    auto completions = drain();
    assert(!player_save_worker_pid_pending(1));
    assert(completions.size() == 6);
    assert(completions[1].pid == 2 && completions[1].revision == 1);
    assert(completions[2].kind == persistence_job_kind::locker && completions[2].owner == 7);
    assert(completions[5].pid == 2 && completions[5].revision == 3);
    player_save_worker_health health = player_save_worker_health_copy();
    assert(health.submitted == 6 && health.replaced == 1 && health.applied == 6);
    assert(health.queued_jobs == 0 && health.inflight_jobs == 0 && health.queued_bytes == 0);
    assert(health.high_water_jobs >= 5);

    // A lost connection is retried at the head: pid 6 waits for pid 5.
    assert(player_save_worker_submit(snapshot_for(5, 1)) == player_save_submit_result::accepted);
    assert(player_save_worker_submit(snapshot_for(6, 1)) == player_save_submit_result::accepted);
    // Any other failure drops the job and reports it.
    assert(player_save_worker_submit(snapshot_for(8, 1)) == player_save_submit_result::accepted);
    assert(persistence_writer_wait_idle(5000));
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        const std::vector<std::string> tail(state.order.end() - 3, state.order.end());
        const std::vector<std::string> expected = {"p5:1", "p6:1", "p8:1"};
        assert(tail == expected);
    }
    completions = drain();
    assert(completions.size() == 3);
    assert(completions[0].pid == 5 && completions[0].retry_count == 2 &&
           completions[0].outcome == player_save_apply_outcome::applied);
    assert(completions[2].pid == 8 &&
           completions[2].outcome == player_save_apply_outcome::terminal_failure &&
           completions[2].error_code == 1452);
    health = player_save_worker_health_copy();
    assert(health.connection_retries == 2 && health.failures == 1);

    // Shutdown stops a job whose connection never comes back and names what it
    // could not write.
    assert(player_save_worker_submit(snapshot_for(9, 1)) == player_save_submit_result::accepted);
    assert(player_save_worker_submit(snapshot_for(10, 1)) == player_save_submit_result::accepted);
    wait_until([] { return player_save_worker_health_copy().connection_retries >= 3; });
    assert(!persistence_writer_wait_idle(50));
    const auto started = std::chrono::steady_clock::now();
    player_save_worker_shutdown();
    assert(std::chrono::steady_clock::now() - started < std::chrono::seconds(2));
    const auto left = persistence_writer_pending_owners();
    assert(left.size() == 2);
    assert(left[0] == persistence_job_owner(persistence_job_kind::player, 9));
    assert(left[1] == persistence_job_owner(persistence_job_kind::player, 10));
    assert(!player_save_worker_health_copy().running);
    assert(player_save_worker_submit(snapshot_for(11, 1)) ==
           player_save_submit_result::unavailable);
    player_save_worker_reset_for_tests();
    assert(persistence_writer_pending_owners().empty());

    // Shutdown does not wait on a job stuck in a database call: it interrupts it, the
    // job stays pending and is named with what was queued behind it. A writer with
    // nothing in flight is not interrupted.
    assert(player_save_worker_init(apply_snapshot, nullptr));
    player_save_worker_shutdown(interrupt_writer);
    assert(state.interrupts == 0);
    player_save_worker_reset_for_tests();
    assert(player_save_worker_init(apply_snapshot, nullptr));
    assert(player_save_worker_submit(snapshot_for(12, 1)) == player_save_submit_result::accepted);
    assert(persistence_writer_submit(persistence_job_kind::log, 1, 64, locker_job(1)) ==
           player_save_submit_result::accepted);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait(lock, [] { return state.stuck_started; });
    }
    const auto stopping = std::chrono::steady_clock::now();
    player_save_worker_shutdown(interrupt_writer);
    assert(std::chrono::steady_clock::now() - stopping < std::chrono::seconds(2));
    assert(state.interrupts == 1);
    const auto unwritten = persistence_writer_pending_owners();
    assert(unwritten.size() == 2);
    assert(unwritten[0] == persistence_job_owner(persistence_job_kind::player, 12));
    assert(unwritten[1] == persistence_job_owner(persistence_job_kind::log, 1));
    assert(std::string(persistence_job_kind_name(persistence_job_kind::log)) == "log");
    player_save_worker_reset_for_tests();

    // A writer the interrupt cannot stop is left behind after the grace: shutdown
    // still returns and names its job. No new writer starts until it has exited.
    assert(player_save_worker_init(apply_snapshot, nullptr));
    assert(player_save_worker_submit(snapshot_for(13, 1)) == player_save_submit_result::accepted);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait(lock, [] { return state.connecting; });
    }
    const auto abandoning = std::chrono::steady_clock::now();
    player_save_worker_shutdown(interrupt_writer);
    const auto abandoned = std::chrono::steady_clock::now() - abandoning;
    assert(abandoned >= std::chrono::milliseconds(PLAYER_SAVE_WORKER_STOP_GRACE_MSEC));
    assert(abandoned < std::chrono::milliseconds(PLAYER_SAVE_WORKER_STOP_GRACE_MSEC + 1000));
    const auto stuck = persistence_writer_pending_owners();
    assert(stuck.size() == 1);
    assert(stuck[0] == persistence_job_owner(persistence_job_kind::player, 13));
    assert(!player_save_worker_health_copy().running);
    assert(!player_save_worker_init(apply_snapshot, nullptr));
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.connected = true;
        state.changed.notify_all();
    }
    player_save_worker_reset_for_tests();
    assert(player_save_worker_init(apply_snapshot, nullptr));
    player_save_worker_reset_for_tests();
    return 0;
}
'''


test_outputs = ROOT / "bin/tests"
test_outputs.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="duris-player-save-worker-", dir=test_outputs) as temp_dir:
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
            rel("player_save_worker.c"),
            rel("persistence_observability.c"),
            "-lmysqlclient",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True, timeout=30)
print("[PASS] allocation failures admit neither partial queue jobs nor owner entries; tail replacement allocates nothing")
print("[PASS] one writer applies saves in capture order; a newer save replaces its queued one")
print("[PASS] a lost connection is retried at the head; other failures are reported and dropped")
print("[PASS] shutdown stops a stuck retry and names the owners it could not write")
print("[PASS] shutdown interrupts a job stuck in a database call and names it as unwritten")
print("[PASS] shutdown leaves a writer the interrupt cannot stop after the grace, and names its job")

assert "PLAYER_SAVE_WORKER_DEFAULT_THREADS = 1" in WORKER_HEADER
assert "std::thread writer;" in WORKER
assert "std::vector<std::thread>" not in WORKER
assert "player_revision_" not in WORKER
assert "journal_ack_callback" in WORKER
for kind in ("player,", "corpse,", "locker,", "saved_item,"):
    assert kind in WORKER_HEADER
print("[PASS] the writer is one thread with player, corpse, locker and saved-item jobs")

# Locker saves share the writer when ported.
if "persistence_writer_submit" in LOCKER:
    assert "persistence_writer_submit(\n\t\tpersistence_job_kind::locker" in LOCKER
    assert "pthread_create" not in LOCKER
    print("[PASS] locker saves go through the one writer")

for metric in (
    "queued_jobs",
    "inflight_jobs",
    "queued_bytes",
    "oldest_age_msec",
    "age_limit_exceeded",
    "connection_retries",
    "max_capture_to_apply_usec",
    "max_apply_usec",
):
    assert metric in WORKER_HEADER and metric in DIAGNOSTICS
assert "writer state=" in DIAGNOSTICS
print("[PASS] bounded redacted writer health is exposed through persistence diagnostics")

print("persistence writer contracts passed")
