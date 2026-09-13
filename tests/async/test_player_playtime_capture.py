#!/usr/bin/env python3
"""Issue #259: exercise real status capture with deterministic session clocks."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "core/utils.h"
#include "core/files.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include <cassert>
#include <climits>
#include <iostream>

index_data indexes[1] = {};
P_index obj_index = indexes;
P_index mob_index = indexes;
room_data rooms[1] = {};
P_room world = rooms;
int top_of_objt = 0;
int top_of_mobt = 0;
extern const int top_of_world = 0;
Skill skills[MAX_SKILLS] = {};
bool has_innate(P_char, int) { return false; }
void logit(const char *, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
P_char get_linked_char(P_char, ush_int) { return nullptr; }
static time_t now = 10000;
extern "C" time_t __wrap_time(time_t *out) {
    if (out) *out = now;
    return now;
}

int main() {
    char_data ch = {};
    pc_only_data pc = {};
    ch.only.pc = &pc;
    pc.pid = 42;
    ch.player.time.played = 3600;
    ch.player.time.logon = 9940;
    ch.player.time.saved = 9000;
    player_revision_t revision = 0;
    auto capture = [&](int intent = RENT_CRASH) {
        const auto before = ch.player.time;
        player_snapshot snapshot;
        assert(player_snapshot_capture(&ch, ++revision, PLAYER_COMPONENT_STATUS,
                                       intent, 1, &snapshot) == player_snapshot_capture_result::ok);
        assert(ch.player.time.played == before.played);
        assert(ch.player.time.logon == before.logon);
        assert(ch.player.time.saved == before.saved);
        std::vector<uint8_t> bytes;
        assert(player_snapshot_encode(snapshot, &bytes) == player_snapshot_codec_result::ok);
        player_snapshot decoded;
        assert(player_snapshot_decode(bytes.data(), bytes.size(), &decoded) == player_snapshot_codec_result::ok);
        for (const auto &row : decoded.status_integers)
            if (row.field == player_status_field::played_time)
                return row.is_unsigned ? row.unsigned_value : static_cast<uint64_t>(row.signed_value);
        std::abort();
    };
    assert(capture() == 3660); // The current-session minute must be durable.
    assert(capture() == 3660); // Repeated capture cannot compound elapsed time.
    now += 15;
    assert(capture() == 3675);
    assert(capture(RENT_DEATH) == 3675); // Terminal/death uses the same clock contract.
    ch.player.time.played = capture(); // Simulate loading the acknowledged total.
    now += 86400; // Offline time is not played time.
    ch.player.time.logon = now;
    assert(capture() == 3675);
    now += 10;
    assert(capture() == 3685);
    ch.player.time.logon = now + 10; // Backward clock jump must not underflow.
    assert(capture() == 3675);
    ch.player.time.logon = 0; // Uninitialized creation/offline fixture.
    assert(capture() == 3675);
    ch.player.time.logon = -1;
    assert(capture() == 3675);
    ch.player.time.logon = 10;
    now = -1; // time() failure must not grant time or underflow.
    assert(capture() == 3675);
    ch.player.time.played = INT_MAX - 5;
    ch.player.time.logon = 10;
    now = 100;
    assert(capture() == INT_MAX); // Durable SQL column is signed INT.
    std::cout << "[PASS] active time, repeat saves, terminal capture, reload, clock guards, SQL range; live fields unchanged\n";
}
'''

with tempfile.TemporaryDirectory(prefix="duris-playtime-") as temporary:
    source = Path(temporary) / "playtime.cpp"
    binary = Path(temporary) / "playtime"
    source.write_text(HARNESS)
    subprocess.run([
        "g++", "-std=c++20", "-g", "-O1", "-ffunction-sections", "-fdata-sections",
        "-Isrc", str(source), "src/player/player_snapshot_capture.c",
        "src/player/player_snapshot_codec.c", "src/player/pet_restore_state.c",
        "src/player/pet_restore_runtime.c", "src/item/item_ownership_runtime.c",
        "src/item/item_transfer_command.c", "src/persistence/critical_command.c",
        "-Wl,--gc-sections", "-Wl,--wrap=time", "-lcrypto", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
