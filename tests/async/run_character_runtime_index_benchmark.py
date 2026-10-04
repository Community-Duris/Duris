#!/usr/bin/env python3
"""Compare production runtime-ID lookup with the former list scan (no timing gate)."""

import json
import os
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function


HARNESS = r'''
#include "account/character_identity.c"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

P_char character_list = nullptr;
static std::thread::id nevent_game_thread;
static bool nevent_game_thread_bound = false;
void panic_corruption(const char *, const char *, ...) { std::abort(); }
void logit(const char *, const char *, ...) {}
// INSERT_PRODUCTION_THREAD_GUARDS

// The pre-index algorithm, retained only here as the measurement baseline.
static P_char linear_lookup(uint64_t runtime_id)
{
    if (!runtime_id) return nullptr;
    for (P_char ch = character_list; ch; ch = ch->next)
        if (ch->runtime_id == runtime_id) return ch;
    return nullptr;
}

static uint64_t checksum;
static double measure(P_char (*lookup)(uint64_t), const std::vector<uint64_t> &queries,
                      int repetitions)
{
    auto start = std::chrono::steady_clock::now();
    uint64_t result = 0;
    for (int pass = 0; pass < repetitions; ++pass)
        for (uint64_t id : queries)
            result += reinterpret_cast<uintptr_t>(lookup(id));
    checksum ^= result;
    const auto elapsed = std::chrono::steady_clock::now() - start;
    return std::chrono::duration<double, std::nano>(elapsed).count() /
           (queries.size() * repetitions);
}

int main()
{
    nevent_bind_game_thread();
    for (size_t count : {256U, 4096U, 16384U, 32768U}) {
        std::vector<char_data> characters(count);
        for (size_t i = 0; i < count; ++i) {
            characters[i].runtime_id = allocate_character_runtime_id();
            characters[i].next = i + 1 < count ? &characters[i + 1] : nullptr;
            register_character_runtime_id(&characters[i]);
        }
        character_list = characters.data();
        if (!character_runtime_index_is_consistent()) return 1;
        std::vector<uint64_t> hits, misses;
        uint64_t random = 42;
        for (int i = 0; i < 2048; ++i) {
            random = random * UINT64_C(6364136223846793005) + 1;
            hits.push_back(characters[(random >> 32) % count].runtime_id);
            misses.push_back(UINT64_MAX - i);
        }
        for (const auto &queries : {hits, misses}) {
            const bool missing = queries.front() > next_runtime_id;
            for (uint64_t id : queries)
                if (linear_lookup(id) != find_character_by_runtime_id(id)) return 2;
            std::vector<double> indexed, linear;
            for (int trial = 0; trial < 3; ++trial) {
                indexed.push_back(measure(find_character_by_runtime_id, queries, 100));
                linear.push_back(measure(linear_lookup, queries, 1));
            }
            std::sort(indexed.begin(), indexed.end());
            std::sort(linear.begin(), linear.end());
            std::printf("{\"characters\":%zu,\"missing\":%s,\"queries\":%zu,"
                        "\"indexed_ns\":%.2f,\"linear_ns\":%.2f,\"speedup\":%.2f}\n",
                        count, missing ? "true" : "false", queries.size(),
                        indexed[1], linear[1], linear[1] / indexed[1]);
        }
        for (auto &ch : characters) unregister_character_runtime_id(&ch);
        character_list = nullptr;
        if (!character_runtime_index_is_consistent()) return 3;
    }
    std::fprintf(stderr, "lookup checksum: %llu; char_data bytes: %zu\n",
                 static_cast<unsigned long long>(checksum), sizeof(char_data));
}
'''

HARNESS = HARNESS.replace(
    "// INSERT_PRODUCTION_THREAD_GUARDS",
    "\n".join(extract_function("new_events.c", signature) for signature in (
        "void nevent_bind_game_thread()", "bool nevent_is_game_thread()",
        "bool nevent_require_game_thread(const char *operation)")),
)

scratch = ROOT / "bin/tests"
scratch.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="character-index-benchmark-", dir=scratch) as temporary:
    work = Path(temporary)
    source, binary = work / "benchmark.cpp", work / "benchmark"
    source.write_text(HARNESS, encoding="ascii")
    subprocess.run(["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-pthread", f"-I{ROOT / 'src'}", str(source), "-o", str(binary)], check=True)
    output = subprocess.check_output([str(binary)], text=True, timeout=60)
    print(json.dumps({"compiler": "g++ -O2, no sanitizers", "trials": 3,
                      "indexed_repetitions": 100, "linear_repetitions": 1,
                      "cpu": os.uname().machine,
                      "cases": [json.loads(row) for row in output.splitlines()]}, indent=2))
