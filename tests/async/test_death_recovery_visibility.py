#!/usr/bin/env python3
"""Execute batch refusal diagnostics and conservative terminal classifications."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import player_death_recovery_visibility as recovery


def function(text, signature):
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 0
    for i in range(opening, len(text)):
        depth += (text[i] == "{") - (text[i] == "}")
        if not depth:
            return text[start:i + 1]
    raise AssertionError(signature)


(ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
fight = (ROOT / "src/combat/fight.c").read_text()
harness = r'''
#include "persistence/death_recovery_visibility.h"
#include "item/item_transfer_command.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
struct obj_data { int value[8] = {}; };
using P_obj = obj_data *;
struct pc_data { uint64_t death_recovery_owner = 0, death_recovery_since_usec = 0,
    death_recovery_reports = 0, death_recovery_last_alert_usec = 0;
    bool death_recovery_failure_reported = false; };
struct char_data { bool disputed = false; P_obj carrying = nullptr; pc_data pc;
    struct { pc_data *pc; } only{&pc}; };
using P_char = char_data *;
enum class persistence_severity { info, ok, alert };
std::string report;
uint64_t clock_usec = 1000000;
unsigned reports_emitted = 0;
uint64_t persistence_observability_now_usec() { return clock_usec; }
#define IS_PC(ch) true
#define AVATAR 0
void persistence_report(persistence_severity, int, const char*, const char*, const char*, const char*, const char*, const char *format, ...) {
    ++reports_emitted;
    char text[1024]; va_list arguments; va_start(arguments, format);
    vsnprintf(text, sizeof(text), format, arguments); va_end(arguments); report = text;
}
P_obj corpse_live_item(uint64_t) { return nullptr; }
void note_corpse_transfer_dispute(P_char ch) { ch->disputed = true; }
void persistence_alert(int, const char*, const char*, const char*, const char*, const char*, const char*, ...) {}
bool corpse_lifecycle_transaction_note_item_transfer(uint32_t,uint32_t,uint64_t) { return true; }
void obj_from_char(P_obj) {} void obj_to_obj(P_obj,P_obj) {}
void mark_player_dirty_components(int,int) {} void writeCorpse(P_obj) {}
bool collector_catalog_cache_refresh() { return true; }
void collector_death_enrollment_end(P_obj) {} void wake_death_extract_retry(P_char) {}
bool submit_next_corpse_item(P_char,P_obj) { return false; }
#define AVATAR 0
#define GET_PID(ch) 42
#define OBJ_CARRIED_BY(item,ch) true
#define CORPSE_SAVEID 6
#define CORPSE_PID 3
#define PLAYER_COMPONENT_STATUS 1
#define PLAYER_COMPONENT_EQUIPMENT 2
#define PLAYER_COMPONENT_INVENTORY 4
struct corpse_transfer_context { uint64_t corpse_uid, item_uid, corpse_save_id; uint16_t item_count, root_count; };
''' + function(fight, "static void death_recovery_report(") + function(fight, "void corpse_item_completion(") + r'''
int main() {
    char_data actor;
    corpse_transfer_context context{123,0,9001,15,15};
    item_transfer_result result{};
    corpse_item_completion(&actor,false,result,ITEM_TRANSFER_TOPOLOGY_CARDINALITY,
        reinterpret_cast<const uint8_t*>(&context),sizeof(context));
    assert(actor.disputed);
    assert(report.find("item_uid=0 scope=batch captured_items=15 roots=15") != std::string::npos);
    assert(report.find("refusal=durable_topology_cardinality_mismatch") != std::string::npos);
    assert(report.find("custody=unresolved recovery_owner=death_disposition") != std::string::npos);
    assert(std::string(death_recovery_refusal_name(90)) == "legacy_transfer_size_mismatch");
    uint64_t last = 0, emitted = 0;
    for (uint64_t count = 1; count <= 4096; ++count)
        emitted += death_recovery_alert_due(count, count * 1000000, &last);
    assert(emitted < 14 && emitted > 1);
    actor.pc = {}; reports_emitted = 0;
    for (uint64_t count = 1; count <= 4096; ++count) {
        clock_usec = count * 1000000;
        death_recovery_report(&actor, count == 1 ? persistence_severity::info : persistence_severity::alert,
            "retry", "custody=unresolved");
        if (count == 2) assert(reports_emitted == 2);
    }
    assert(actor.pc.death_recovery_reports == 4096 && reports_emitted < 16);
    clock_usec = 4097000000;
    death_recovery_report(&actor, persistence_severity::ok, "completed", "custody=durable");
    assert(report.find("count=4097 elapsed_sec=4096") != std::string::npos);
    assert(report.find("custody=durable") != std::string::npos);
    char correlation[33], other[33];
    death_recovery_correlation((uint64_t{42}<<32)|9001, correlation);
    death_recovery_correlation((uint64_t{42}<<32)|9002, other);
    assert(strcmp(correlation, other));
    puts(correlation);
}
'''
with tempfile.TemporaryDirectory(dir=ROOT / "bin/tests", prefix="recovery-visibility-") as temp:
    source, binary = Path(temp) / "test.cpp", Path(temp) / "test"
    source.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-Isrc", str(source), "-lcrypto", "-o", str(binary)], cwd=ROOT, check=True)
    output = subprocess.check_output([str(binary)], text=True).strip()
    assert output == recovery.correlation(42, 9001)

decoded = {"pid": 42, "revision": 2, "death": {
    "operation_id_hex": "a5" * 16, "corpse": [
        {"object_uid": 200, "values": [0, 0, 0, 42, 0, 0, 9001, 0]}, {"object_uid": 100}],
    "custody": [{"item_uid": 100}, {"item_uid": 101}],
    "wallet_before": [0] * 4, "unresolved_operations": []}}
owners = [{"item_uid": 100, "state": 3, "owner_type": 1},
          {"item_uid": 101, "state": 3, "owner_type": 1}]
case = recovery.summarize(decoded, owners)
assert case["terminal_custody"] == "quarantine" and case["recovery_required"]
assert case["captured_count"] == 1 and case["authority_count"] == 2 and case["unmatched_count"] == 1
assert case["scope"] == "batch" and case["item_uid"] == 0
assert case["recovery_owner"] == "reviewed_restitution"
owners[0].update(state=1, materialized=True, delivery_matches=True)
case = recovery.summarize(decoded, owners)
assert case["recovery_required"] and case["counts"]["restored"] == 1
owners[1].update(state=2, owner_type=8)
case = recovery.summarize(decoded, owners)
assert case["recovery_required"] and case["verification_requires_review"]
assert case["items"][0]["recovery_owner"] == "restitution_verification"
owners[0]["delivery_verified"] = True
case = recovery.summarize(decoded, owners)
assert not case["recovery_required"] and case["terminal_custody"] == "restored"
assert case["counts"]["safely_retired"] == 1
owners[0]["materialized"] = False
assert recovery.summarize(decoded, owners)["terminal_custody"] == "unresolved"
owners[0].update(materialized=True, delivery_matches=False)
assert recovery.summarize(decoded, owners)["terminal_custody"] == "durable"
owners[0].update(state=2, owner_type=8)
assert recovery.summarize(decoded, owners)["terminal_custody"] == "safely_retired"
assert recovery.summarize(decoded, [])["recovery_required"]
print("[PASS] executable batch UID, named refusals, bounded alerts, correlation and terminal custody states")
