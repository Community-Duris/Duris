#!/usr/bin/env python3
"""Final notifications survive a full pulse buffer, including retry exhaustion.

Run the production coordinator, journal, and workers. The apply adapter controls
outcomes; observing the private queue makes ordering deterministic without sleeps
that assume a worker has finished. Journals use disposable local directories.
"""

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "persistence/critical_command_coordinator.c"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <map>
#include <set>

static std::atomic<unsigned int> attempts{0};
static std::atomic<bool> release_last{false};
static critical_apply_outcome exhausted_outcome;

template <typename F> void wait_for(F condition) {
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!condition()) {
        assert(std::chrono::steady_clock::now() < until);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
static size_t result_depth() {
    std::lock_guard<std::mutex> lock(coordinator_mutex);
    return raw_results.size();
}
static critical_command command(unsigned int tag) {
    critical_command result = {};
    result.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
    assert(critical_operation_id_generate(&result.operation_id));
    result.type = critical_command_type::test;
    result.payload_version = 1;
    result.source_site = critical_source_site::command;
    result.deadline_class = critical_deadline_class::interactive;
    result.keys = {{critical_entity_type::player, tag}};
    result.payload = {uint8_t(tag)};
    return result;
}
static critical_apply_result apply(const critical_command &cmd, void *) {
    if (cmd.payload[0] == 2) {
        if (++attempts == CRITICAL_COORDINATOR_MAX_RETRIES + 1)
            wait_for([] { return release_last.load(); });
        return {exhausted_outcome, 73, 123};
    }
    if (cmd.payload[0] % 2)
        return {critical_apply_outcome::terminal_failure, 0, 45};
    return {critical_apply_outcome::applied, 1, 0};
}
static void capacity_case(const std::string &directory, size_t capacity,
                          critical_apply_outcome outcome) {
    attempts = 0;
    release_last = false;
    exhausted_outcome = outcome;
    assert(critical_command_coordinator_init(directory.c_str(), apply, nullptr, 2));
    const auto failed = command(2);
    assert(critical_command_coordinator_submit(failed) == critical_submit_result::accepted);
    // A retry that can still run needs no output slot, even with a null buffer.
    for (unsigned int n = 0; n < CRITICAL_COORDINATOR_MAX_RETRIES; ++n) {
        wait_for([] { return result_depth() == 1; });
        assert(critical_command_coordinator_pulse(nullptr, 0) == 0);
    }
    wait_for([] { return attempts.load() == CRITICAL_COORDINATOR_MAX_RETRIES + 1; });
    std::map<std::string, critical_apply_outcome> expected;
    expected.emplace(operation_key(failed.operation_id), outcome);
    // Fill the output with mixed successful and terminal results, then finish
    // the exhausted operation. With capacity zero it is the first queued result.
    for (size_t i = 0; i < capacity; ++i) {
        const auto other = command(10 + i);
        expected.emplace(operation_key(other.operation_id), i % 2 ?
                         critical_apply_outcome::terminal_failure : critical_apply_outcome::applied);
        assert(critical_command_coordinator_submit(other) == critical_submit_result::accepted);
    }
    wait_for([&] { return result_depth() == capacity; });
    release_last = true;
    wait_for([&] { return result_depth() == capacity + 1; });
    critical_completion delivered[64] = {};
    std::set<std::string> seen;
    const auto consume = [&](size_t count) {
        for (size_t i = 0; i < count; ++i) {
            const auto id = operation_key(delivered[i].operation_id);
            assert(expected.at(id) == delivered[i].outcome);
            assert(seen.insert(id).second);
        }
    };
    assert(critical_command_coordinator_pulse(capacity ? delivered : nullptr, capacity) == capacity);
    consume(capacity);
    assert(result_depth() == 1);
    assert(critical_command_coordinator_is_fenced(failed.keys[0], nullptr));
    // Repeated empty pulses cannot consume it or trigger another retry.
    for (int i = 0; i < 3; ++i)
        assert(critical_command_coordinator_pulse(nullptr, 0) == 0);
    assert(result_depth() == 1);
    assert(critical_command_coordinator_pulse(delivered, 1) == 1);
    consume(1);
    assert(critical_operation_id_equal(delivered[0].operation_id, failed.operation_id));
    assert(delivered[0].attempt == CRITICAL_COORDINATOR_MAX_RETRIES + 1);
    assert(delivered[0].error_code == 123 && delivered[0].durable_revision == 73);
    assert(attempts == CRITICAL_COORDINATOR_MAX_RETRIES + 1);
    assert(seen.size() == expected.size());
    assert(critical_command_coordinator_pulse(delivered, 64) == 0);
    const auto health = critical_command_coordinator_health_copy();
    assert(health.blocked == 1 && health.retries == CRITICAL_COORDINATOR_MAX_RETRIES);
    assert(health.terminal_failures == capacity / 2 + 1);
    assert(health.completed == capacity);
    assert(health.ambiguous == (outcome == critical_apply_outcome::ambiguous_commit ? 9 : 0));
    assert(critical_command_coordinator_is_fenced(failed.keys[0], nullptr));
    critical_completion cached = {};
    assert(!critical_command_coordinator_get_completed(failed.operation_id, &cached));
    assert(critical_command_coordinator_submit(failed) == critical_submit_result::attached);
    printf("capacity=%zu outcome=%u: all %zu identities delivered once; exhausted operation fenced\n",
           capacity, unsigned(outcome), seen.size());
    critical_command_coordinator_shutdown();
}
int main(int argc, char **argv) {
    assert(argc == 2);
    for (auto outcome : {critical_apply_outcome::retryable_failure, critical_apply_outcome::ambiguous_commit})
        for (size_t capacity : {0, 1, 64})
            capacity_case(std::string(argv[1]) + "/" + std::to_string(unsigned(outcome)) +
                          "-" + std::to_string(capacity), capacity, outcome);
}
'''

with tempfile.TemporaryDirectory(prefix="duris-completion-capacity-") as directory:
    temporary = Path(directory)
    source = temporary / "capacity.cpp"
    binary = temporary / "capacity"
    source.write_text(HARNESS)
    (temporary / "journals").mkdir()
    subprocess.run([
        "g++", "-std=c++20", "-g", "-Og", "-Wall", "-Wextra", "-Werror", "-pthread",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-Isrc", str(source), "src/persistence/critical_command.c",
        "src/persistence/critical_command_journal.c", "-lz", "-lcrypto", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary), str(temporary / "journals")], check=True, timeout=60)

print("critical completion capacity and retry exhaustion regression passed")
