#!/usr/bin/env python3
"""Quest reward grants claim a stable, distinct source for each item goal."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function, source
from contract_text import contains


quest = source("world/quest.c").read_text(encoding="utf-8")
reward = extract_function("world/quest.c", "void give_reward(struct quest_complete_data *qcp")
assert "quest_item_reward_source_id(" in reward
assert contains(reward, "pl, obj, pl, NULL, source, source_id")
assert contains(reward, "pl, obj, pl->in_room, source, nullptr, source_id")
assert "economic_source_kind::quest_completion" in reward
helper = extract_function("item/quest_reward_continuation.h", "inline uint64_t quest_item_reward_source_id(uint64_t")

program = r'''
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <openssl/sha.h>
''' + helper + r'''
int main() {
    assert(quest_item_reward_source_id(0, 29286, 0) == 0);
    assert(quest_item_reward_source_id(100, 0, 0) == 0);
    const auto first = quest_item_reward_source_id(100, 29286, 0);
    assert(first > 0 && first <= INT64_MAX);
    assert(first == quest_item_reward_source_id(100, 29286, 0));
    assert(first != quest_item_reward_source_id(101, 29286, 0));
    assert(first != quest_item_reward_source_id(100, 29286, 1));
    assert(first != quest_item_reward_source_id(100, 29287, 0));
}
'''

with tempfile.TemporaryDirectory(prefix="duris-quest-reward-source-") as directory:
    cpp = Path(directory) / "reward.cpp"
    binary = Path(directory) / "reward"
    cpp.write_text(program, encoding="utf-8")
    subprocess.run(["g++", "-std=c++20", str(cpp), "-lcrypto", "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

print("static quest item reward source identity passed")
