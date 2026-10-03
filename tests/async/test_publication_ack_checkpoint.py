#!/usr/bin/env python3
"""Exercise the publication ACK lock boundary without POSIX journal dependencies."""

from _paths import extract_function, source
import subprocess
import tempfile
from pathlib import Path


ack = extract_function(
    "critical_command_coordinator.c",
    "bool critical_command_coordinator_acknowledge_publication(",
)
coordinator = source("critical_command_coordinator.c").read_text(encoding="utf-8")
shutdown = coordinator.split("bool critical_command_coordinator_shutdown(void)", 1)[1].split(
    "critical_submit_result critical_command_coordinator_submit_internal", 1
)[0]
assert shutdown.index("publication_checkpoint_finished.wait(") < shutdown.index(
    "operations.clear();"
) < shutdown.index("critical_command_journal_shutdown();")

harness = r'''
#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <cstdint>
#include <vector>

struct critical_operation_id { int value; };
struct critical_command { int value; };
struct critical_completion { int value; };
enum class critical_command_journal_result { ok, io_failure };
struct operation_state {
    bool publication_checkpointing = false;
    critical_command command = {};
    critical_completion publication_completion = {};
};
struct health_state { int completed = 0; } health;

std::mutex coordinator_mutex;
std::condition_variable publication_checkpoint_finished;
size_t publication_checkpoints_inflight = 0;
std::unordered_map<std::string, std::unique_ptr<operation_state>> operations;
std::condition_variable work_available;
std::mutex journal_mutex;
std::condition_variable journal_changed;
bool journal_entered = false;
bool release_journal = false;
bool fail_checkpoint = false;

bool critical_operation_id_is_zero(const critical_operation_id &id) { return id.value == 0; }
std::string operation_key(const critical_operation_id &id) { return std::to_string(id.value); }
bool operation_is_publication_pending(const operation_state &) { return true; }
void remove_fences(const std::string &, const critical_command &) {}
void remember_completed(const std::string &, const critical_command &,
                        const critical_completion &) {}
void update_depth() {}

// The production ACK also reports its completed disk checkpoint. These small
// observation bindings preserve the isolated command/journal fixture types.
enum class persistence_trace_stage { publication_ack };
struct fixture_trace { int command; uint32_t outcome = 0; };
std::vector<fixture_trace> recorded_traces;
fixture_trace persistence_command_trace(const critical_command &command, persistence_trace_stage stage) {
    assert(stage == persistence_trace_stage::publication_ack);
    return {command.value, 0};
}
void persistence_trace_record(const fixture_trace &trace) { recorded_traces.push_back(trace); }

critical_command_journal_result critical_command_journal_checkpoint(
    const critical_operation_id &)
{
    std::unique_lock<std::mutex> lock(journal_mutex);
    journal_entered = true;
    journal_changed.notify_all();
    journal_changed.wait(lock, [] { return release_journal; });
    return fail_checkpoint ? critical_command_journal_result::io_failure
                           : critical_command_journal_result::ok;
}

__ACK__

int main()
{
    const critical_operation_id id = {1};
    operations.emplace(operation_key(id), std::make_unique<operation_state>());
    operations.at(operation_key(id))->command.value = 41;
    std::thread ack_thread([&] {
        assert(critical_command_coordinator_acknowledge_publication(id));
    });
    {
        std::unique_lock<std::mutex> lock(journal_mutex);
        journal_changed.wait(lock, [] { return journal_entered; });
    }
    // A blocked disk checkpoint must leave coordinator reads and other ACKs responsive.
    assert(coordinator_mutex.try_lock());
    assert(publication_checkpoints_inflight == 1);
    coordinator_mutex.unlock();
    assert(!critical_command_coordinator_acknowledge_publication(id));
    {
        std::lock_guard<std::mutex> lock(journal_mutex);
        release_journal = true;
    }
    journal_changed.notify_all();
    ack_thread.join();
    assert(publication_checkpoints_inflight == 0);
    assert(operations.empty());
    assert(health.completed == 1);
    assert(recorded_traces.size() == 1 && recorded_traces[0].command == 41 &&
           recorded_traces[0].outcome == static_cast<uint32_t>(critical_command_journal_result::ok));

    operations.emplace(operation_key(id), std::make_unique<operation_state>());
    operations.at(operation_key(id))->command.value = 71;
    fail_checkpoint = true;
    assert(!critical_command_coordinator_acknowledge_publication(id));
    assert(operations.at(operation_key(id))->publication_checkpointing == false);
    assert(publication_checkpoints_inflight == 0);
    assert(recorded_traces.size() == 2 && recorded_traces[1].command == 71 &&
           recorded_traces[1].outcome == static_cast<uint32_t>(critical_command_journal_result::io_failure));
    fail_checkpoint = false;
    assert(critical_command_coordinator_acknowledge_publication(id));
    assert(operations.empty());
    assert(health.completed == 2 && recorded_traces.size() == 3 && recorded_traces[2].command == 71 &&
           recorded_traces[2].outcome == static_cast<uint32_t>(critical_command_journal_result::ok));
}
'''.replace("__ACK__", ack)

with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    program = root / "publication_ack.cpp"
    binary = root / "publication_ack.exe"
    program.write_text(harness, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-pthread", str(program), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("publication ACK checkpoint boundary passed")
