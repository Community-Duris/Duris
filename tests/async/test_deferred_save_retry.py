#!/usr/bin/env python3
"""Runtime policy and source contracts for retryable deferred player saves."""

from _paths import SRC
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
actoth = (SRC / "actoth.c").read_text()
schedule_checkpoint = actoth[
    actoth.index("static void persistence_schedule_checkpoint"):
    actoth.index("void persistence_schedule_character_save")
]
fresh_slot = schedule_checkpoint[schedule_checkpoint.index("slot->pid = GET_PID(ch);"):]
terminal_helper = actoth[
    actoth.index("bool persistence_save_character_terminal"):
    actoth.index("bool persistence_save_all_characters_terminal")
]

harness = r'''
#include "persistence/deferred_save_policy.h"
#include <cassert>

int main()
{
    assert(deferred_save_next_retry_delay(-1) == 4);
    assert(deferred_save_next_retry_delay(0) == 4);
    assert(deferred_save_next_retry_delay(4) == 8);
    assert(deferred_save_next_retry_delay(8) == 16);
    assert(deferred_save_next_retry_delay(64) == 128);
    assert(deferred_save_next_retry_delay(120) == 240);
    assert(deferred_save_next_retry_delay(240) == 240);
    assert(!persistence_should_extract_terminal_inventory(false, true));
    assert(!persistence_should_extract_terminal_inventory(false, false));
    assert(!persistence_should_extract_terminal_inventory(true, false));
    assert(persistence_should_extract_terminal_inventory(true, true));
    return 0;
}
'''

with tempfile.TemporaryDirectory() as tmp:
    source = Path(tmp) / "retry_test.cpp"
    binary = Path(tmp) / "retry_test"
    source.write_text(harness)
    subprocess.run(
        ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I", str(SRC),
         str(source), str(SRC / "deferred_save_policy.c"), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

checks = {
    "fixed slot capacity": "#define PERSISTENCE_DEFERRED_SAVE_SLOTS 512" in actoth,
    "one scheduling owner": "static void schedule_deferred_save" in actoth,
    "fresh slot delegates scheduling": "slot->due_usec = 1;" not in fresh_slot and
                                       "schedule_deferred_save(slot, ch, delay);" in fresh_slot,
    "pulse clears due time": "slot->due_usec = 0;" in actoth,
    "failure advances delay": "slot->retry_delay = deferred_save_next_retry_delay" in actoth,
    "failure rearms deadline": len(re.findall(
        r"schedule_deferred_save\(\s*slot\s*,\s*ch\s*,\s*slot->retry_delay\s*\)\s*;", actoth
    )) >= 3,
    "coalesced request repairs scheduling": "if (!slot->due_usec)" in actoth,
    "latest type retained": "slot->type = type ? type : slot->type;" in actoth,
    "level intent coalesced": "slot->level_dirty = slot->level_dirty || level_dirty;" in actoth,
    "direct flush is truthful": "bool persistence_flush_character_saves(P_char ch)" in actoth,
    "global flush is truthful": "bool persistence_flush_all_character_saves(void)" in actoth,
    "terminal helper consumes slot only after durability": "if (saved && slot)" in terminal_helper and
                                                            "memset(slot, 0" in terminal_helper,
    "failed terminal retains safe retry": "if (!saved)" in terminal_helper and
                                           "persistence_schedule_character_save" in terminal_helper and
                                           "RENT_CRASH" in terminal_helper,
    "terminal failure queues safe retry": '"terminal-save-retry"' in actoth,
    "dead pc retry remains eligible": "if (!ch || IS_NPC(ch) || !GET_NAME(ch))" in schedule_checkpoint,
}

for name, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {name}")
assert all(checks.values())
print("deferred save retry checks passed")

# Exercise the production scheduler with a controlled monotonic clock.
def section(start, end):
    """Extract production source between two unique markers for the scheduler harness."""
    return actoth[actoth.index(start):actoth.index(end, actoth.index(start))]

slot_definition = section("struct deferred_save_slot\n", "static struct deferred_save_slot deferred_saves")
scheduling_harness = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#define WAIT_SEC 4
#define PERSISTENCE_DEFERRED_SAVE_SLOTS 1
#define AVATAR 0
struct char_data { int pid; uint64_t runtime_id; };
using P_char = char_data *;
#define IS_NPC(ch) false
#define GET_NAME(ch) "Tester"
#define GET_PID(ch) ((ch)->pid)
uint64_t now_usec = 1000000;
uint64_t persistence_observability_now_usec() { return now_usec; }
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *, const char *, ...) {}
void sql_update_level(P_char) {}
bool do_save_silent(P_char, int) { return true; }
#define LOG_DEBUG 0
void logit(int, const char *) {}
''' + slot_definition + r'''
deferred_save_slot deferred_saves[1] = {};
''' + section("static struct deferred_save_slot *find_deferred_save_slot(", "static void process_deferred_character_save(") + schedule_checkpoint + r'''
int main() {
    char_data player{1, 11};
    persistence_schedule_checkpoint(&player, 1, 4, "initial", 0);
    auto &slot = deferred_saves[0];
    assert(slot.runtime_id == 11 && slot.due_usec == 2000000);
    now_usec = 1500000;
    player.runtime_id = 22;
    persistence_schedule_checkpoint(&player, 2, 4, "reconnect", 1);
    assert(slot.runtime_id == 22 && slot.due_usec == 2000000);
    assert(slot.type == 2 && slot.level_dirty);
    schedule_deferred_save(&slot, &player, 8);
    assert(slot.due_usec == 3500000);
    now_usec = 2000000;
    schedule_deferred_save(&slot, &player, 16);
    assert(slot.due_usec == 6000000);
    persistence_schedule_checkpoint(&player, 0, 4, "coalesce", 0);
    assert(slot.due_usec == 6000000 && slot.type == 2 && slot.level_dirty);
    slot.due_usec = 0;
    slot.retry_delay = 8;
    persistence_schedule_checkpoint(&player, 0, 4, "repair", 0);
    assert(slot.due_usec == 4000000);
}
'''
build = root / "bin/tests/deferred-save-scheduling"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(dir=build) as temp_dir:
    source = Path(temp_dir) / "regression.cpp"
    binary = Path(temp_dir) / "regression"
    source.write_text(scheduling_harness)
    subprocess.run(["g++", "-std=c++20", str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("[PASS] reconnect refreshes identity, coalescing preserves deadlines, and retries replace deadlines")
