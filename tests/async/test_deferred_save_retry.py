#!/usr/bin/env python3
"""Runtime policy and source contracts for retryable deferred player saves."""

from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
actoth = (root / "src/actoth.c").read_text()
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
#include "deferred_save_policy.h"
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
        ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I", str(root / "src"),
         str(source), str(root / "src/deferred_save_policy.c"), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

checks = {
    "fixed slot capacity": "#define PERSISTENCE_DEFERRED_SAVE_SLOTS 512" in actoth,
    "one scheduling owner": "static void schedule_deferred_save_event" in actoth,
    "fresh slot delegates scheduling": "slot->scheduled = 1;" not in fresh_slot and
                                       "schedule_deferred_save_event(slot, ch, delay);" in fresh_slot,
    "callback clears event marker": "slot->scheduled = 0;" in actoth,
    "failure advances delay": "slot->retry_delay = deferred_save_next_retry_delay" in actoth,
    "failure rearms event": actoth.count("schedule_deferred_save_event(slot, ch, slot->retry_delay);") >= 3,
    "coalesced request repairs scheduling": "if (!slot->scheduled)" in actoth,
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
