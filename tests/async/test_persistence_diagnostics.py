#!/usr/bin/env python3
"""Exercise bounded native incident retention, target isolation and JSON reports."""

import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "persistence/persistence_diagnostic_report.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    persistence_trace_filter filter;
    assert(persistence_trace_parse_target("player", "9001", &filter));
    for (const char *value : {"0", "-1", "+1", "12x", "2147483648", "18446744073709551616"})
        assert(!persistence_trace_parse_target("player", value, &filter));
    assert(!persistence_trace_parse_target("operation", "00000000000000000000000000000000", &filter));
    assert(!persistence_trace_parse_target("operation", "gg000000000000000000000000000000", &filter));
    assert(!persistence_trace_parse_target("unknown", "1", &filter));
    assert(persistence_trace_parse_target("player", "9001", &filter));
    persistence_trace_event first;
    first.stage = persistence_trace_stage::save_result;
    first.pid = 9001; first.revision = 42; first.error = 10001; first.diagnosis = 6;
    first.incident = true; first.witness.item_uid = 7001;
    first.witness.observed_present = true; first.witness.observed_item_revision = 3;
    persistence_trace_record(first);
    persistence_trace_record(first); // repeats retain the first sequence
    for (size_t i = 0; i < PERSISTENCE_TRACE_CAPACITY + 10; ++i) {
        persistence_trace_event event;
        event.pid = i % 2 ? 9001 : 9002;
        event.revision = i + 100;
        persistence_trace_record(event);
    }
    auto snapshot = persistence_trace_copy(filter);
    assert(snapshot.available && snapshot.retained == PERSISTENCE_TRACE_CAPACITY);
    assert(snapshot.overwritten == 12 && snapshot.incident_count == 1);
    assert(snapshot.incidents[0].sequence == 1 && snapshot.incidents[0].witness.item_uid == 7001);
    assert(snapshot.count == PERSISTENCE_TRACE_REPORT_EVENTS);
    for (size_t i = 0; i < snapshot.count; ++i) {
        assert(snapshot.events[i].pid == 9001);
        if (i) assert(snapshot.events[i].sequence > snapshot.events[i - 1].sequence);
    }
    {
        std::lock_guard<std::mutex> held(persistence_diagnostics_detail::diagnostic_mutex);
        std::thread writer([&] { persistence_trace_record(first); });
        writer.join(); // bounded admission returns despite this held lock
        assert(!persistence_trace_copy(filter).available);
    }
    assert(persistence_trace_copy(filter).dropped == 1);
    critical_command command{};
    command.operation_id.bytes[0] = 5;
    command.type = critical_command_type::item_transfer;
    command.source_site = critical_source_site::command;
    for (size_t i = 0; i < PERSISTENCE_TRACE_KEYS + 1; ++i)
        command.keys.push_back({critical_entity_type::item, i + 7001});
    persistence_trace_record(persistence_command_trace(command, persistence_trace_stage::command_admitted));
    assert(persistence_trace_parse_target("operation", "05000000000000000000000000000000", &filter));
    snapshot = persistence_trace_copy(filter);
    assert(snapshot.count == 1 && snapshot.events[0].keys_truncated);
    assert(persistence_trace_parse_target("item", "7001", &filter));
    snapshot = persistence_trace_copy(filter);
    assert(snapshot.count == 1 && snapshot.incident_count == 1);
    for (size_t i = 0; i < PERSISTENCE_INCIDENT_CAPACITY + 1; ++i) {
        auto event = first; event.pid = 9100 + i; event.revision = 50 + i;
        persistence_trace_record(event);
    }
    snapshot = persistence_trace_copy(filter);
    assert(snapshot.incidents_evicted == 2);
    const auto before = snapshot.latest_sequence;
    const auto lost_before = snapshot.dropped;
    std::vector<std::thread> producers;
    for (int i = 0; i < 4; ++i) producers.emplace_back([] {
        for (int j = 0; j < 1000; ++j) persistence_trace_record({});
    });
    for (auto &producer : producers) producer.join();
    snapshot = persistence_trace_copy(filter);
    assert(snapshot.latest_sequence - before + snapshot.dropped - lost_before == 4000);
    std::cout << '{'; persistence_trace_snapshot_json(std::cout, snapshot); std::cout << '}';
}
'''

with tempfile.TemporaryDirectory(prefix="duris-persistence-diagnostics-") as directory:
    source, binary = Path(directory) / "probe.cpp", Path(directory) / "probe"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pthread",
                    "-Isrc", str(source), "-o", str(binary)], cwd=ROOT, check=True)
    report = json.loads(subprocess.check_output([str(binary)], timeout=15))
    assert report["history"]["available"] == 1
    assert report["history"]["overwritten"] > 0

command = (ROOT / "src/cmd/actinf.c").read_text()
start = command.index("static void show_world_persistence_diagnosis(")
body = command[start:command.index("static void show_world_persistence(P_char", start)]
assert body.index("GET_LEVEL(ch) < AVATAR") < body.index("persistence_trace_copy(filter)")
for forbidden in ("mysql_", "player_save_journal_init(", "player_save_journal_replay(", "fopen("):
    assert forbidden not in body
for forbidden in ("player_save_journal_health_copy(", "player_save_journal_pid_quarantined(",
                  "player_load_pipeline_login_admit(", "player_save_pipeline_save_admitted(",
                  "player_save_pipeline_target_save_pending("):
    assert forbidden not in body  # these legacy helpers wait on the journal I/O lock
for forbidden in ("player_save_journal_prepare_recovery(",
                  "player_save_journal_resolve_recovery(",
                  "player_save_journal_clear"):
    assert forbidden not in body

# Exercise the actual staff command body with controlled resident metadata. This
# catches invalid JSON, privilege bypass, and parser regressions in both modes.
COMMAND_PREAMBLE = r'''
#include "persistence/persistence_diagnostic_report.h"
#include "persistence/critical_command_coordinator.h"
#include "player/player_save_pipeline.h"
#include "player/player_load_pipeline.h"
#include "player/player_save_journal.h"
#include "item/item_ownership_runtime.h"
#include <cassert>
#include <iostream>
#include <thread>
struct char_data {
    int level, pid; uint64_t runtime_flags; bool npc; void *desc; char_data *next;
};
#define IS_NPC(ch) ((ch)->npc)
#define IS_PC(ch) (!IS_NPC(ch))
#define GET_LEVEL(ch) ((ch)->level)
#define GET_PID(ch) ((ch)->pid)
#define IS_SET(value, bit) ((value) & (bit))
#define CHAR_RFLAG_LOAD_DEGRADED 1
#define AVATAR 100
#define LOG_NONE 0
P_char character_list = nullptr;
std::string output;
int metadata_calls = 0;
const char *selected_backend = "flatfile-primary";
const char *persistence_mode_name() { return selected_backend; }
void send_to_char(const char *value, P_char) { output = value; }
void send_to_char(const char *value, P_char, int log) {
    assert(log == LOG_NONE && std::string(value).size() <= 16384); output += value;
}
player_save_journal_health player_save_journal_health_copy() {
    ++metadata_calls; player_save_journal_health result{}; result.initialized = true; return result;
}
player_save_pipeline_health player_save_pipeline_health_copy() {
    ++metadata_calls; player_save_pipeline_health result{}; result.accepting = true; return result;
}
player_save_worker_health player_save_worker_health_copy() {
    ++metadata_calls; player_save_worker_health result{}; result.running = true; return result;
}
bool player_revision_snapshot_copy(int pid, player_revision_snapshot *result) {
    ++metadata_calls; *result = {}; result->pid = pid; result->current_revision = 42;
    result->acknowledged_revision = 41; return true;
}
player_save_journal_diagnostic player_save_journal_diagnostic_copy(int) {
    ++metadata_calls; player_save_journal_diagnostic result{};
    result.health.initialized = true;
    result.available = true; result.pid_fence = true; result.archived_frames = 2; return result;
}
std::mutex pipeline_mutex;
player_save_pipeline_health health{};
bool stop_requested = false, accepting = true, retained = false, target_fence = false;
int append_inflight_pid = -1;
struct terminal_fence { bool death_pinned = false; };
terminal_fence terminal;
bool has_terminal = false;
terminal_fence *find_terminal_fence_locked(int) { return has_terminal ? &terminal : nullptr; }
bool find_target_save_login_fence_locked(int) { return target_fence; }
bool any_snapshot_is_retained_locked(int) { return retained; }
bool player_save_worker_pid_pending(int) { ++metadata_calls; return true; }
bool player_save_journal_pid_quarantined(int) { ++metadata_calls; return true; }
bool player_load_pipeline_login_admit(int) { ++metadata_calls; return false; }
bool player_save_pipeline_save_admitted(int) { ++metadata_calls; return false; }
bool player_save_pipeline_target_save_pending(int) { ++metadata_calls; return true; }
bool item_ownership_runtime_lookup(uint64_t uid, item_ownership_runtime_entry *result) {
    ++metadata_calls; *result = {}; result->item_uid = uid; result->root_item_uid = uid;
    result->owner = {item_owner_type::player, 9001, 0}; result->item_revision = 3; return true;
}
bool critical_command_coordinator_get_completed(const critical_operation_id &, critical_completion *result) {
    ++metadata_calls; *result = {}; result->durable_revision = 7; return true;
}
'''
COMMAND_MAIN = r'''
int main() {
    health.initialized = true; health.accepting = true;
    auto metadata = player_save_pipeline_diagnostic_copy(9001);
    assert(metadata.available && metadata.pid_admission_open && !metadata.retained_save);
    has_terminal = true; terminal.death_pinned = true;
    metadata = player_save_pipeline_diagnostic_copy(9001);
    assert(metadata.available && !metadata.pid_admission_open && metadata.retained_save);
    has_terminal = false;
    {
        std::lock_guard<std::mutex> held(pipeline_mutex);
        std::thread reader([] { assert(!player_save_pipeline_diagnostic_copy(9001).available); });
        reader.join(); // resident diagnostic copy never waits behind the held lock
    }
    char_data staff{100, 9001, 1, false, nullptr, nullptr};
    char_data visitor{99, 9002, 0, false, nullptr, nullptr};
    character_list = &staff;
    show_world_persistence_diagnosis(&visitor, "diagnose player 9001");
    assert(metadata_calls == 0 && output.find("avatar access") != std::string::npos);
    for (const char *invalid : {"diagnose player 9001 extra", "diagnose player -1", "player 9001"}) {
        show_world_persistence_diagnosis(&staff, invalid);
        assert(metadata_calls == 0 && output.find("Usage:") != std::string::npos);
    }
    persistence_trace_event event;
    event.pid = 9001; event.revision = 42; event.stage = persistence_trace_stage::save_result;
    event.error = 10001; event.diagnosis = 6; event.incident = true; event.witness.item_uid = 7001;
    persistence_trace_record(event);
    for (const char *backend : {"flatfile-primary", "mariadb-primary"}) {
        selected_backend = backend;
        output.clear();
        show_world_persistence_diagnosis(&staff, "diagnose player 9001");
        std::cout << output;
    }
    output.clear();
    show_world_persistence_diagnosis(&staff, "diagnose item 7001");
    std::cout << output;
    output.clear();
    show_world_persistence_diagnosis(&staff, "diagnose operation abababababababababababababababab");
    std::cout << output;
    for (size_t i = 0; i < PERSISTENCE_INCIDENT_CAPACITY; ++i) {
        event.revision = i + 100; event.witness.item_uid = 7001;
        event.key_count = PERSISTENCE_TRACE_KEYS;
        for (size_t key = 0; key < event.key_count; ++key)
            event.keys[key] = {critical_entity_type::item, UINT64_MAX - key};
        persistence_trace_record(event);
    }
    output.clear();
    show_world_persistence_diagnosis(&staff, "diagnose player 9001");
    assert(output.size() > 65536);
    std::cout << output;
    {
        std::lock_guard<std::mutex> held(pipeline_mutex);
        output.clear();
        show_world_persistence_diagnosis(&staff, "diagnose player 9001");
        std::cout << output;
    }
}
'''
pipeline = (ROOT / "src/player/player_save_pipeline.c").read_text()
metadata_start = pipeline.index("player_save_pipeline_diagnostic player_save_pipeline_diagnostic_copy(")
metadata_body = pipeline[metadata_start:pipeline.index("bool player_save_pipeline_loads_allowed(", metadata_start)]
with tempfile.TemporaryDirectory(prefix="duris-staff-diagnosis-") as directory:
    source, binary = Path(directory) / "command.cpp", Path(directory) / "command"
    source.write_text(COMMAND_PREAMBLE + metadata_body + body + COMMAND_MAIN)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pthread",
                    "-Isrc", str(source), "-o", str(binary)], cwd=ROOT, check=True)
    reports = [json.loads(line) for line in subprocess.check_output([str(binary)], timeout=15).splitlines()]
    assert [report["backend"] for report in reports[:2]] == ["flatfile-primary", "mariadb-primary"]
    for report in reports[:2]:
        assert report["complete"] is False
        assert report["state"]["quarantined"] == 1 and report["state"]["login_admitted"] == 0
        assert report["state"]["archived_frames"] == 2
        assert report["history"]["incidents"][0]["witness"]["item_uid"] == 7001
    assert reports[2]["state"]["item_revision"] == 3
    assert reports[3]["state"]["operation_revision"] == 7
    assert len(reports[4]["history"]["events"]) == 64
    assert len(reports[4]["history"]["incidents"]) == 64
    assert reports[5]["state"]["pipeline_available"] == 0
    assert reports[5]["state"]["save_admitted"] is None
    assert reports[5]["state"]["save_pending"] is None
print("[PASS] bounded history, first witness, nonblocking admission, isolation, concurrency, JSON and native staff command")
