#!/usr/bin/env python3
"""Runtime identity, journal, ordering, fence, retry, replay, and bound contracts."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
COMMAND = (ROOT / "src/critical_command.c").read_text()
JOURNAL = (ROOT / "src/critical_command_journal.c").read_text()
COORDINATOR = (ROOT / "src/critical_command_coordinator.c").read_text()
HEADER = (ROOT / "src/critical_command_coordinator.h").read_text()


HARNESS = r'''
#include "critical_command_coordinator.h"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <fcntl.h>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

struct apply_state
{
    std::mutex mutex;
    std::condition_variable changed;
    std::map<unsigned int, unsigned int> attempts;
    bool a_running = false;
    bool c_started = false;
    bool b_started_while_a = false;
    bool release_a = false;
    bool gate_running = false;
    bool release_gate = false;
    bool late_started = false;
    bool hold_all = false;
    bool release_all = false;
};

critical_command make_command(unsigned int tag, std::vector<critical_entity_key> keys)
{
    critical_command command = {};
    command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
    assert(critical_operation_id_generate(&command.operation_id));
    command.type = critical_command_type::test;
    command.payload_version = 1;
    command.source_site = critical_source_site::command;
    command.deadline_class = critical_deadline_class::interactive;
    command.keys = std::move(keys);
    command.payload = {static_cast<uint8_t>(tag)};
    return command;
}

critical_apply_result apply(const critical_command &command, void *raw)
{
    auto &state = *static_cast<apply_state *>(raw);
    const unsigned int tag = command.payload[0];
    std::unique_lock<std::mutex> lock(state.mutex);
    const unsigned int attempt = ++state.attempts[tag];
    if (state.hold_all)
        state.changed.wait(lock, [&] { return state.release_all; });
    if (tag == 1)
    {
        state.a_running = true;
        state.changed.notify_all();
        state.changed.wait(lock, [&] { return state.release_a; });
        state.a_running = false;
        state.changed.notify_all();
    }
    else if (tag == 2)
    {
        state.b_started_while_a = state.a_running;
        state.changed.notify_all();
    }
    else if (tag == 3)
    {
        state.c_started = true;
        state.changed.notify_all();
    }
    else if (tag == 5)
    {
        state.gate_running = true;
        state.changed.notify_all();
        state.changed.wait(lock, [&] { return state.release_gate; });
        state.gate_running = false;
        state.changed.notify_all();
    }
    else if (tag == 7)
    {
        state.late_started = true;
        state.changed.notify_all();
    }
    if (tag == 4 && attempt == 1)
        return {critical_apply_outcome::ambiguous_commit, 0, 2013};
    return {critical_apply_outcome::applied, 1, 0};
}

struct replay_state
{
    std::vector<critical_command> commands;
};

bool collect_replay(critical_command command, void *raw)
{
    static_cast<replay_state *>(raw)->commands.push_back(std::move(command));
    return true;
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

int main(int argc, char **argv)
{
    assert(argc == 5);

    std::set<std::string> identities;
    for (unsigned int index = 0; index < 512; ++index)
    {
        critical_operation_id identity = {};
        assert(critical_operation_id_generate(&identity));
        char hex[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
        assert(critical_operation_id_to_hex(identity, hex, sizeof(hex)));
        assert(identities.insert(hex).second);
        critical_operation_id decoded = {};
        assert(critical_operation_id_from_hex(hex, &decoded));
        assert(critical_operation_id_equal(identity, decoded));
    }

    critical_command codec = make_command(
        7, {{critical_entity_type::item, 9}, {critical_entity_type::player, 2}});
    codec.accepted_at_usec = 1700000000000000ULL;
    codec.expected_revisions = {{{critical_entity_type::item, 9}, 3}};
    assert(critical_command_normalize(&codec));
    assert(codec.keys[0].type == critical_entity_type::player);
    std::vector<uint8_t> encoded;
    assert(critical_command_encode(codec, &encoded) == critical_command_codec_result::ok);
    critical_command decoded = {};
    assert(critical_command_decode(encoded.data(), encoded.size(), &decoded) ==
           critical_command_codec_result::ok);
    assert(critical_command_equal(codec, decoded));
    auto truncated = encoded;
    truncated.pop_back();
    assert(critical_command_decode(truncated.data(), truncated.size(), &decoded) ==
           critical_command_codec_result::truncated);
    auto malformed = encoded;
    malformed[53] = 1;
    assert(critical_command_decode(malformed.data(), malformed.size(), &decoded) ==
           critical_command_codec_result::invalid);

    assert(critical_command_journal_init(argv[1]));
    critical_command first = make_command(8, {{critical_entity_type::player, 8}});
    first.accepted_at_usec = 1700000000000001ULL;
    assert(critical_command_normalize(&first));
    critical_command second = make_command(9, {{critical_entity_type::account, 9}});
    second.accepted_at_usec = 1700000000000002ULL;
    assert(critical_command_normalize(&second));
    assert(critical_command_journal_append(first) == critical_command_journal_result::ok);
    assert(critical_command_journal_append(second) == critical_command_journal_result::ok);
    assert(critical_command_journal_append(first) == critical_command_journal_result::ok);
    replay_state replay;
    assert(critical_command_journal_replay(collect_replay, &replay) ==
           critical_command_journal_result::ok);
    assert(replay.commands.size() == 2);
    assert(critical_command_journal_health_copy().duplicates == 1);
    assert(critical_command_journal_checkpoint(first.operation_id) ==
           critical_command_journal_result::ok);
    replay.commands.clear();
    assert(critical_command_journal_replay(collect_replay, &replay) ==
           critical_command_journal_result::ok);
    assert(replay.commands.size() == 1);
    assert(critical_operation_id_equal(replay.commands[0].operation_id, second.operation_id));
    critical_command_journal_shutdown();

    const std::string journal_path = std::string(argv[1]) + "/critical-command.journal";
    int fd = open(journal_path.c_str(), O_RDWR);
    assert(fd >= 0);
    unsigned char byte = 0;
    assert(pread(fd, &byte, 1, 50) == 1);
    byte ^= 0x5a;
    assert(pwrite(fd, &byte, 1, 50) == 1);
    assert(fsync(fd) == 0);
    close(fd);
    assert(!critical_command_journal_init(argv[1]));
    critical_command_journal_reset_for_tests();

    apply_state state;
    assert(critical_command_coordinator_init(argv[2], apply, &state, 2));
    critical_command a = make_command(
        1, {{critical_entity_type::item, 10}, {critical_entity_type::player, 1}});
    critical_command b = make_command(
        2, {{critical_entity_type::player, 2}, {critical_entity_type::item, 10}});
    critical_command c = make_command(3, {{critical_entity_type::player, 3}});
    assert(critical_command_coordinator_submit(a) == critical_submit_result::accepted);
    assert(critical_command_coordinator_submit(a) == critical_submit_result::attached);
    critical_command mismatch = a;
    mismatch.payload = {99};
    assert(critical_command_coordinator_submit(mismatch) ==
           critical_submit_result::identity_conflict);
    assert(critical_command_coordinator_submit(b) == critical_submit_result::accepted);
    assert(critical_command_coordinator_submit(c) == critical_submit_result::accepted);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait(lock, [&] { return state.a_running && state.c_started; });
        assert(!state.b_started_while_a);
    }
    critical_operation_id fenced_by = {};
    assert(critical_command_coordinator_is_fenced(
        {critical_entity_type::item, 10}, &fenced_by));
    assert(critical_operation_id_equal(fenced_by, a.operation_id));
    critical_completion stale = {};
    stale.operation_id = a.operation_id;
    stale.outcome = critical_apply_outcome::applied;
    stale.durable_revision = 1;
    stale.attempt = 99;
    assert(critical_command_coordinator_inject_completion_for_tests(stale));
    critical_completion completions[16] = {};
    assert(critical_command_coordinator_pulse(completions, 16) == 0);
    assert(critical_command_coordinator_health_copy().stale_completions == 1);
    assert(critical_command_coordinator_is_fenced({critical_entity_type::item, 10}, nullptr));
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.release_a = true;
        state.changed.notify_all();
    }
    wait_until([&] {
        critical_command_coordinator_pulse(completions, 16);
        return critical_command_coordinator_health_copy().completed == 3;
    });
    assert(!state.b_started_while_a);
    assert(!critical_command_coordinator_is_fenced({critical_entity_type::item, 10}, nullptr));

    critical_command gate = make_command(5, {{critical_entity_type::account, 50}});
    critical_command multi = make_command(
        6, {{critical_entity_type::account, 50}, {critical_entity_type::item, 60}});
    critical_command late = make_command(7, {{critical_entity_type::item, 60}});
    assert(critical_command_coordinator_submit(gate) == critical_submit_result::accepted);
    assert(critical_command_coordinator_submit(multi) == critical_submit_result::accepted);
    assert(critical_command_coordinator_submit(late) == critical_submit_result::accepted);
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait(lock, [&] { return state.gate_running; });
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        lock.lock();
        assert(!state.late_started);
        state.release_gate = true;
        state.changed.notify_all();
    }
    wait_until([&] {
        critical_command_coordinator_pulse(completions, 16);
        return critical_command_coordinator_health_copy().completed == 6;
    });
    assert(state.late_started);

    critical_command one = make_command(8, {{critical_entity_type::player, 80}});
    critical_command two = make_command(9, {{critical_entity_type::player, 90}});
    assert(critical_command_coordinator_submit(one) == critical_submit_result::accepted);
    assert(critical_command_coordinator_submit(two) == critical_submit_result::accepted);
    wait_until([&] {
        std::lock_guard<std::mutex> lock(state.mutex);
        return state.attempts[8] == 1 && state.attempts[9] == 1;
    });
    wait_until([&] { return critical_command_coordinator_pulse(completions, 1) == 1; });
    assert(critical_command_coordinator_health_copy().completed == 7);
    wait_until([&] { return critical_command_coordinator_pulse(completions, 1) == 1; });
    assert(critical_command_coordinator_health_copy().completed == 8);

    critical_command d = make_command(4, {{critical_entity_type::guild, 4}});
    assert(critical_command_coordinator_submit(d) == critical_submit_result::accepted);
    wait_until([&] {
        critical_command_coordinator_pulse(completions, 16);
        return critical_command_coordinator_health_copy().completed == 9;
    });
    assert(state.attempts[4] == 2);
    auto health = critical_command_coordinator_health_copy();
    assert(health.ambiguous == 1 && health.retries == 1 && health.fenced_keys == 0);
    assert(critical_command_coordinator_submit(d) == critical_submit_result::attached);
    critical_command_coordinator_quiesce();
    critical_command rejected = make_command(6, {{critical_entity_type::player, 6}});
    assert(critical_command_coordinator_submit(rejected) == critical_submit_result::unavailable);
    critical_command_coordinator_resume();
    assert(critical_command_coordinator_drain(3000));
    critical_command_coordinator_shutdown();

    assert(critical_command_journal_init(argv[3]));
    critical_command recovery = make_command(10, {{critical_entity_type::corpse, 10}});
    recovery.accepted_at_usec = 1700000000000010ULL;
    assert(critical_command_normalize(&recovery));
    assert(critical_command_journal_append(recovery) == critical_command_journal_result::ok);
    critical_command_journal_shutdown();
    apply_state recovery_state;
    assert(critical_command_coordinator_init(argv[3], apply, &recovery_state, 1));
    wait_until([&] {
        critical_command_coordinator_pulse(completions, 16);
        return critical_command_coordinator_health_copy().completed == 1;
    });
    assert(recovery_state.attempts[10] == 1);
    assert(critical_command_journal_health_copy().records == 0);
    critical_command_coordinator_shutdown();

    apply_state capacity;
    capacity.hold_all = true;
    assert(critical_command_coordinator_init(argv[4], apply, &capacity, 2));
    for (size_t index = 0; index < CRITICAL_COORDINATOR_MAX_OPERATIONS; ++index)
    {
        critical_command command = make_command(
            11, {{critical_entity_type::player, 10000 + index}});
        assert(critical_command_coordinator_submit(command) == critical_submit_result::accepted);
    }
    critical_command overflow = make_command(11, {{critical_entity_type::player, 999999}});
    assert(critical_command_coordinator_submit(overflow) == critical_submit_result::overloaded);
    health = critical_command_coordinator_health_copy();
    assert(health.queued + health.inflight == CRITICAL_COORDINATOR_MAX_OPERATIONS);
    assert(health.high_water_operations == CRITICAL_COORDINATOR_MAX_OPERATIONS);
    assert(health.overloads == 1);
    {
        std::lock_guard<std::mutex> lock(capacity.mutex);
        capacity.release_all = true;
        capacity.changed.notify_all();
    }
    critical_command_coordinator_shutdown();
    return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-critical-command-") as temporary:
    temp = Path(temporary)
    source = temp / "critical_command_test.cpp"
    binary = temp / "critical_command_test"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-pthread", "-Isrc", str(source), "src/critical_command.c",
            "src/critical_command_journal.c", "src/critical_command_coordinator.c",
            "-lz", "-lcrypto", "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    directories = [temp / name for name in ("journal", "coordinator", "replay", "capacity")]
    subprocess.run([str(binary), *(str(path) for path in directories)], check=True, timeout=30)

for contract in (
    "CRITICAL_COORDINATOR_MAX_OPERATIONS = 1024",
    "CRITICAL_COORDINATOR_MAX_BYTES = 64 * 1024 * 1024",
    "CRITICAL_COORDINATOR_MAX_RETRIES = 8",
    "CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES = 8 * 1024 * 1024",
):
    assert contract in HEADER
for forbidden in ("P_char", "P_obj", "MYSQL", "redis", "sql_"):
    assert forbidden not in COORDINATOR
assert "getrandom(" in COMMAND and "rand(" not in COMMAND
assert "fsync(fd)" in JOURNAL and "crc32(" in JOURNAL and "O_NOFOLLOW" in JOURNAL
assert COORDINATOR.index("critical_command_journal_append(command)") < COORDINATOR.index(
    "work_available.notify_all();\n\treturn critical_submit_result::accepted"
)

MAKEFILE = (ROOT / "src/Makefile").read_text()
COMM = (ROOT / "src/comm.c").read_text()
COPYOVER = (ROOT / "src/copyover.c").read_text()
ACTINF = (ROOT / "src/actinf.c").read_text()
for object_name in (
    "critical_command.o",
    "critical_command_journal.o",
    "critical_command_coordinator.o",
):
    assert object_name in MAKEFILE
assert "critical_command_coordinator_pulse(critical_completions, 64)" in COMM
assert "critical_command_coordinator_quiesce()" in COMM
assert "critical_command_coordinator_drain(3000)" in COMM
assert "critical_command_coordinator_quiesce()" in COPYOVER
assert "critical_command_coordinator_drain(3000)" in COPYOVER
assert COPYOVER.index("critical_command_coordinator_drain(3000)") < COPYOVER.index(
    "player_save_pipeline_quiesce()"
)
assert (
    "critical_command_coordinator_resume();\n"
    "\tcritical_outbox_resume();\n"
    "\tplayer_save_pipeline_resume();"
) in COPYOVER
assert '\"critical_commands state=%s' in ACTINF
assert "command.payload" not in ACTINF and "operation_id" not in ACTINF
assert "critical_command_equal" in COORDINATOR and "identity_conflict" in COORDINATOR
assert "keys_available" in COORDINATOR and "acquire_keys" in COORDINATOR
assert "found->second->attempt != completion.attempt" in COORDINATOR

print("critical command identity, journal, ordering, replay, fence, and bound contracts passed")
