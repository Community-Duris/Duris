#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "actoth.c").read_text()

manual_save = source[source.index("void do_save(") : source.index("void do_not_here(")]
status_event = source[
    source.index("static void check_manual_character_save_status("):
    source.index("static struct deferred_save_slot *find_deferred_save_slot")
]

assert '"Save already queued for %s.\\r\\n"' in manual_save
assert '"Save queued for %s.\\r\\n"' in manual_save
assert manual_save.index("find_manual_save_status") < manual_save.index("persistence_schedule_character_save")
assert "revision.acknowledged_revision >= status->revision" in status_event
assert '"Save complete for %s.\\r\\n"' in status_event
assert '"Save failed for %s; please try again.\\r\\n"' in status_event

print("[PASS] manual saves report queued, duplicate, completion, and failure states")

# Execute the real save slot code with world callbacks deliberately withheld.
# Only the clock, character lookup, snapshot submission, and ACK are controlled.
import subprocess

save_slots = source[
    source.index("#define PERSISTENCE_DEFERRED_SAVE_SLOTS"):
    source.index("bool persistence_save_character_terminal")
]
preamble = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include "player/player_revision_state.h"
#include "persistence/deferred_save_policy.h"
struct character { int pid; uint64_t runtime_id; character *next; };
using P_char = character *;
using P_obj = void *;
character player{1, 100, nullptr};
P_char character_list = &player;
#define IS_NPC(ch) false
#define IS_PC(ch) true
#define GET_PID(ch) ((ch)->pid)
#define GET_NAME(ch) "Synthetic"
#define WAIT_SEC 4
#define AVATAR 0
#define LOG_DEBUG 0
uint64_t now_usec = 1000000;
int saves = 0;
bool fail_save = false;
std::string messages;
uint64_t persistence_observability_now_usec() { return now_usec; }
P_char find_character_by_runtime_id(uint64_t id) {
    return player.runtime_id == id ? &player : nullptr;
}
void persistence_counter_saturating_add(uint64_t *value, uint64_t amount) { *value += amount; }
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *, ...) {}
void logit(int, const char *, ...) {}
void sql_update_level(P_char) {}
void send_to_char_f(P_char, const char *format, ...) { messages += format; }
void add_event(void (*)(P_char, P_char, P_obj, void *), int, P_char, int, int, int, void *, size_t) {}
bool do_save_silent(P_char ch, int) {
    ++saves;
    if (fail_save) return false;
    return player_revision_mark(ch->pid, PLAYER_CHECKPOINT_COMPONENT_ALL, nullptr);
}
'''
main = r'''
void manual_request() {
    auto *status = find_empty_manual_save_status();
    assert(status);
    status->pid = player.pid;
    status->started_usec = now_usec;
#ifdef PULSE_SAVE_TEST
    status->runtime_id = player.runtime_id;
#endif
    persistence_schedule_character_save(&player, 1, 2, "manual_save");
}
int main() {
    assert(player_revision_hydrate(1, 0));
    manual_request();
    persistence_pulse_character_saves();
    assert(saves == 0); // Preserve the requested half-second delay.
    now_usec += 500000;
    persistence_pulse_character_saves();
    assert(saves == 1); // No world callback ran, but the save was submitted.
    assert(messages.find("Save complete") == std::string::npos);
    player_revision_t revision = 0;
    player_component_mask_t components = 0;
    assert(player_revision_queue(1, &revision, &components));
    assert(player_revision_begin_inflight(1, revision, components));
    assert(player_revision_acknowledge(1, revision, components));
    persistence_pulse_character_saves();
    assert(messages.find("Save complete") != std::string::npos);
    assert(!find_manual_save_status(1));

    messages.clear();
    fail_save = true;
    manual_request();
    now_usec += 500000;
    persistence_pulse_character_saves();
    assert(saves == 2 && find_deferred_save_slot(1));
    assert(messages.find("retrying") != std::string::npos);
    fail_save = false;
    now_usec += 999999;
    persistence_pulse_character_saves();
    assert(saves == 2);
    ++now_usec;
    persistence_pulse_character_saves();
    assert(saves == 3 && !find_deferred_save_slot(1));
    // Without an ACK the status must expire, never report success.
    messages.clear();
    now_usec += 30000000;
    persistence_pulse_character_saves();
    assert(messages.find("Save failed") != std::string::npos);
    assert(messages.find("Save complete") == std::string::npos);

    manual_request();
    ++player.runtime_id; // Simulate extraction and storage reuse.
    now_usec += 500000;
    persistence_pulse_character_saves();
    assert(saves == 3 && !find_deferred_save_slot(1));
    assert(!find_manual_save_status(1));
}
'''
# The pre-fix main loop has no save-slot pulse; this fallback lets the same
# withheld-callback regression demonstrate a runtime assertion on that source.
if "void persistence_pulse_character_saves(void)" in save_slots:
    preamble += "\n#define PULSE_SAVE_TEST 1\n"
else:
    preamble += "\nvoid persistence_pulse_character_saves() {}\n"
build_dir = ROOT / "bin/tests/manual-save-feedback"
build_dir.mkdir(parents=True, exist_ok=True)
harness_path = build_dir / "regression.cpp"
binary = build_dir / "regression"
harness_path.write_text(preamble + save_slots + main)
subprocess.run([
    "g++", "-std=c++20", "-Isrc", str(harness_path),
    "src/player/player_revision_state.c", "src/persistence/deferred_save_policy.c",
    "-o", str(binary),
], cwd=ROOT, check=True)
subprocess.run([str(binary)], check=True)
comm = (SRC / "comm.c").read_text()
assert "persistence_pulse_character_saves();" in comm
print("[PASS] saves, ACK feedback, deadlines, retries, and lifetime checks work without world callbacks")
