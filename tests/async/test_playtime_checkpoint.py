#!/usr/bin/env python3
"""Issue #259: a quiet player must be status-dirty before periodic capture."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "core/utils.h"
#include "core/files.h"
#include "persistence/persistence_checkpoint.h"
#include "player/player_save_pipeline.h"
#include "player/player_save_worker.h"
#include <cassert>
#include <vector>
#include <iostream>

int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
int _pwipe = 0;
extern const int top_of_world = 0;
room_data rooms[1] = {};
P_room world = rooms;
P_char character_list = nullptr;
char_data characters[10] = {};
pc_only_data pcs[10] = {};
player_component_mask_t dirty[11] = {};
int captured = 0, continuations = 0;
bool player_save_pipeline_mark(int pid, player_component_mask_t mask) {
    dirty[pid] |= mask;
    return true;
}
player_save_pipeline_result player_save_pipeline_checkpoint_dirty(P_char ch, int intent, int room) {
    assert(intent == RENT_CRASH && room == 22800);
    assert(dirty[GET_PID(ch)] == PLAYER_COMPONENT_STATUS);
    dirty[GET_PID(ch)] = 0;
    ++captured;
    return player_save_pipeline_result::queued;
}
P_char find_character_by_runtime_id(uint64_t id) {
    return id >= 1 && id <= 10 ? &characters[id-1] : nullptr;
}
void nevent_periodic_retry_after(unsigned long long, const char *) { std::abort(); }
void nevent_periodic_continue_after(unsigned long long) { ++continuations; }
bool redis_floor_runtime_enabled() { return false; }
void redis_flush_floor_drops() { std::abort(); }

int main() {
    rooms[0].number = 22800;
    for (int i=0; i<10; ++i) {
        characters[i].only.pc = &pcs[i];
        pcs[i].pid = i+1;
        characters[i].runtime_id = i+1;
        characters[i].player.time.logon = 100;
        characters[i].next = i<9 ? &characters[i+1] : nullptr;
    }
    character_list = characters;
    event_flush_dirty_players(nullptr,nullptr,nullptr,nullptr);
    assert(captured == 8 && continuations == 1);
    event_flush_dirty_players(nullptr,nullptr,nullptr,nullptr);
    assert(captured == 10 && continuations == 1);
    // The next cycle still checkpoints quiet players after the prior ACK.
    event_flush_dirty_players(nullptr,nullptr,nullptr,nullptr);
    event_flush_dirty_players(nullptr,nullptr,nullptr,nullptr);
    assert(captured == 20 && continuations == 2);
    flush_dirty_players();
    assert(captured == 30);
    std::cout << "[PASS] clean players checkpoint status without unrelated mutations; eight-player slicing preserved\n";
}
'''
with tempfile.TemporaryDirectory(prefix="duris-playtime-checkpoint-") as temporary:
    source = Path(temporary) / "checkpoint.cpp"
    binary = Path(temporary) / "checkpoint"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-ffunction-sections", "-fdata-sections", "-Isrc",
                    str(source), "src/persistence/persistence_checkpoint.c",
                    "-Wl,--gc-sections", "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
