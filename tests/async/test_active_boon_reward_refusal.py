#!/usr/bin/env python3
"""Execute the boon admission path with and without an active accounting epoch."""

from __future__ import annotations

import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


def submit_function() -> str:
    source = (ROOT / "src/economy/boon_reward_transaction.c").read_text(encoding="utf-8")
    start = source.index("bool boon_reward_transaction_submit(")
    opening = source.index("{", start)
    depth = 0
    for end in range(opening, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if depth == 0:
            return source[start : end + 1]
    raise AssertionError("unterminated boon reward submission function")


PRELUDE = r"""
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <new>
#include <string>
#include <utility>

struct Character { int pid = 7, racewar = 1, level = 20, in_room = 3, vnum = 0, race = 0;
                   bool npc = false, pc_pet = false; };
using P_char = Character *;
#define IS_NPC(ch) ((ch)->npc)
#define IS_PC(ch) (!(ch)->npc)
#define IS_PC_PET(ch) ((ch)->pc_pet)
#define GET_PID(ch) ((ch)->pid)
#define GET_RACEWAR(ch) ((ch)->racewar)
#define GET_LEVEL(ch) ((ch)->level)
#define GET_VNUM(ch) ((ch)->vnum)
#define GET_RACE(ch) ((ch)->race)
#define ROOM_ZONE_NUMBER(room) (room)
#define LNK_PET 1
#define TAG_CONJURED_PET 2
#define MAX_BOPT 8
#define BOON_REWARD_PENDING_MAX 64
P_char get_linked_char(P_char, int) { return nullptr; }
bool affected_by_spell(P_char, int) { return false; }

struct boon_reward_payload {
    uint32_t pid = 0;
    uint8_t racewar = 0;
    uint16_t level = 0;
    int zone_number = 0;
    uint8_t option = 0;
    double data = 0;
    int victim_vnum = 0;
    int16_t victim_race = 0;
    uint8_t victim_flags = 0;
};
struct pending_reward { uint32_t pid; boon_reward_payload payload; uint64_t submitted_at_msec; };
std::map<std::string, pending_reward> pending;
struct { int submission_failures = 0, submitted = 0; size_t pending = 0; } health;
struct critical_operation_id { std::array<uint8_t, 16> bytes = {}; };
struct critical_command {};
enum class critical_submit_result { queued };
int generated = 0, built = 0, submissions = 0;
bool active_epoch = false;
namespace economic_gameplay_authority { bool active() { return active_epoch; } }
bool critical_operation_id_generate(critical_operation_id *id) {
    id->bytes[0] = ++generated;
    return true;
}
bool boon_reward_command_build(critical_command *, critical_operation_id,
                               const boon_reward_payload &) { ++built; return true; }
std::string operation_key(const critical_operation_id &id) {
    return std::string(reinterpret_cast<const char *>(id.bytes.data()), id.bytes.size());
}
uint64_t monotonic_msec() { return 12; }
critical_submit_result critical_command_coordinator_submit(critical_command) {
    ++submissions;
    return critical_submit_result::queued;
}
bool critical_submit_result_keeps_operation(critical_submit_result) { return true; }
"""

MAIN = r"""
int main() {
    Character character;
    active_epoch = true;
    assert(!boon_reward_transaction_submit(&character, nullptr, 1.0, 0));
    assert(generated == 0 && built == 0 && submissions == 0 && pending.empty());
    active_epoch = false;
    assert(boon_reward_transaction_submit(&character, nullptr, 1.0, 0));
    assert(generated == 1 && built == 1 && submissions == 1 && pending.size() == 1);
    assert(health.submitted == 1 && health.pending == 1);
}
"""


class ActiveBoonRewardRefusal(unittest.TestCase):
    def test_refuses_before_completion_command_admission(self) -> None:
        compiler = shlex.split(os.environ.get("CXX", "g++"))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest("C++ compiler unavailable")
        source = PRELUDE + submit_function() + MAIN
        with tempfile.TemporaryDirectory(prefix="active-boon-refusal-") as directory:
            program = Path(directory) / "boon.cpp"
            binary = Path(directory) / "boon"
            program.write_text(source, encoding="utf-8")
            subprocess.run([*compiler, "-std=c++20", "-O0", str(program), "-o", str(binary)],
                           check=True, timeout=60)
            subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
