#!/usr/bin/env python3
"""Exercise the real ACK/hold lock boundary without linking the journal implementation."""

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

# These declarations and pure predicates come from the actual current source.
# No fixture copy of the ACK body, command enum, or save-hold predicate is used.
native_context = extract_function("critical_command_coordinator.c", "struct native_operation_context") + ";"
zone_typed = extract_function("critical_command_coordinator.c", "bool zone_reset_typed_command(")
id_zero = extract_function("critical_command.c", "bool critical_operation_id_is_zero(")
key = extract_function("critical_command_coordinator.c", "std::string operation_key(")
zone_text = source("zone_reset_item_command.h").read_text(encoding="utf-8")
zone_constants = "\n".join(
    next(line for line in zone_text.splitlines() if line.startswith("constexpr uint16_t " + name + " ="))
    for name in ("ZONE_RESET_ITEM_PAYLOAD_VERSION", "ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION")
)

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

#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_journal.h"
#include "player/player_save_execution_guard.h"

__ZONE_CONSTANTS__
__NATIVE_CONTEXT__

// Test-owned projection: only members read by the exact extracted ACK.
// Real command/completion/native-context types are retained; omitted operation
// state and journal/health bindings are not claimed as coordinator integration.
struct operation_state {
    bool publication_checkpointing = false;
    critical_command command = {};
    std::unique_ptr<native_operation_context> native;
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

__ID_ZERO__
__KEY__
__ZONE_TYPED__
bool operation_is_publication_pending(const operation_state &) { return true; }
void remove_fences(const std::string &, const critical_command &) {}
void remember_completed(const std::string &, const critical_command &,
                        const critical_completion &) {}
void update_depth() {}

// The production ACK also reports its completed disk checkpoint. These small
// observation bindings preserve the isolated command/journal fixture types.
enum class persistence_trace_stage { publication_ack };
struct fixture_trace { uint64_t command; uint32_t outcome = 0; };
std::vector<fixture_trace> recorded_traces;
fixture_trace persistence_command_trace(const critical_command &command, persistence_trace_stage stage) {
    assert(stage == persistence_trace_stage::publication_ack);
    return {command.accepted_at_usec, 0};
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
    critical_operation_id id = {};
    id.bytes[0] = 1;
    assert(!critical_command_coordinator_acknowledge_publication(critical_operation_id{}));
    assert(!critical_command_coordinator_acknowledge_publication(id));
    operations.emplace(operation_key(id), std::make_unique<operation_state>());
    auto &state = *operations.at(operation_key(id));
    state.command.type = critical_command_type::test;
    state.command.accepted_at_usec = 41;
    state.command.operation_id = id;

    // Native and ROOM publication must refuse this generic ACK route before
    // checkpointing. Verify no journal entry, trace, completion or fence retirement.
    const auto refused_without_checkpoint = [&] {
        assert(!critical_command_coordinator_acknowledge_publication(id));
        assert(operations.size() == 1 && !state.publication_checkpointing);
        assert(publication_checkpoints_inflight == 0 && !journal_entered);
        assert(health.completed == 0 && recorded_traces.empty());
    };
    state.native = std::make_unique<native_operation_context>();
    refused_without_checkpoint();
    state.native.reset();
    state.command.type = critical_command_type::native_mobile_birth;
    refused_without_checkpoint();
    state.command.type = critical_command_type::zone_reset_item_birth;
    state.command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
    state.command.publication_required = true;
    for (const uint16_t version : {ZONE_RESET_ITEM_PAYLOAD_VERSION,
                                  ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION}) {
        state.command.payload_version = version;
        assert(zone_reset_typed_command(state.command));
        refused_without_checkpoint();
    }
    state.command.type = critical_command_type::test;

    // Isolated fixture setup of the real leaf's hold array, not an admission or
    // pipeline ownership claim. The actual production held predicate locks it.
    {
        std::lock_guard<std::mutex> lock(player_save_execution_guard::detail::mutex);
        player_save_execution_guard::detail::holds[0] = {7, id, 1};
    }
    assert(player_save_execution_guard::publication_operation_held(id));
    refused_without_checkpoint();
    {
        std::lock_guard<std::mutex> lock(player_save_execution_guard::detail::mutex);
        player_save_execution_guard::detail::holds[0] = {};
    }
    assert(!player_save_execution_guard::publication_operation_held(id));
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
    operations.at(operation_key(id))->command.type = critical_command_type::test;
    operations.at(operation_key(id))->command.accepted_at_usec = 71;
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
'''.replace("__ACK__", ack).replace("__NATIVE_CONTEXT__", native_context).replace(
    "__ZONE_CONSTANTS__", zone_constants
).replace("__ID_ZERO__", id_zero).replace(
    "__KEY__", key
).replace("__ZONE_TYPED__", zone_typed)

with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    program = root / "publication_ack.cpp"
    binary = root / "publication_ack.exe"
    program.write_text(harness, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-pthread", "-I" + str(source("critical_command.h").parents[1]), str(program), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("publication ACK checkpoint boundary passed")
