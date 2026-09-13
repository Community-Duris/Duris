#!/usr/bin/env python3
"""Production game bridge: physical identity, ownership inspection and flag/reload behavior."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "core/prototypes.h"
#include <cassert>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <thread>
#include <time.h>
#include <sys/stat.h>

static uint64_t test_wall = 100, test_mono = 1000;
static float master = 1, mana_enabled = 1;
static bool game_thread = true;
static std::string root, output;
int fake_clock(clockid_t, timespec *result) {
    result->tv_sec = test_mono / 1000; result->tv_nsec = (test_mono % 1000) * 1000000; return 0;
}
time_t fake_time(time_t *) { return test_wall; }
#define clock_gettime fake_clock
#define time fake_time
#include "item/artifact_mana.c"
#undef time
#undef clock_gettime

P_obj object_list = nullptr;
index_data indexes[2] = {};
P_index obj_index = indexes;
bool nevent_is_game_thread() { return game_thread; }
bool persistence_mode_requires_mysql() { return false; }
const char *persistence_mode_flatfile_root() { return root.c_str(); }
float get_property(const char *key, double fallback) {
    if (!std::strcmp(key, "itemActions.enabled")) return master;
    if (!std::strcmp(key, "itemActions.mana.enabled")) return mana_enabled;
    return fallback;
}
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
void logit(const char *, const char *, ...) {}
void send_to_char(const char *text, P_char) { output += text; }
char *one_argument(const char *text, char *name) { std::strcpy(name, text); return const_cast<char *>(text + std::strlen(text)); }
P_obj get_obj_in_list_vis(P_char, const char *name, P_obj list, bool) { return std::strcmp(name, "blade") ? nullptr : list; }
P_obj get_object_in_equip_vis(P_char actor, char *name, int *slot) {
    *slot = 0; return std::strcmp(name, "blade") ? nullptr : actor->equipment[0];
}
int main(int argc, char **argv) {
    assert(argc == 2); root = argv[1];
    std::filesystem::create_directories(root + "/domains");
    chmod(root.c_str(), 0700); chmod((root + "/domains").c_str(), 0700);
    indexes[0].virtual_number = 900;
    indexes[1].virtual_number = 901;
    obj_data source = {}, duplicate = {};
    source.obj_uid = 81; source.R_num = 0; object_list = &source;
    char_data owner = {}, stranger = {};
    owner.carrying = &source;
    artifact_mana_profile profile{ 7, 1, 1000, 1, 0 };
    assert(artifact_mana_publish(900, profile));
    assert(!artifact_mana_debit(&source, 1, false, 1));
    artifact_mana_record record;
    for (int i = 0; i < 1000; ++i) {
        artifact_mana_pulse();
        if (artifact_mana_inspect(&source, record)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    assert(artifact_mana_inspect(&source, record) && record.reserve == 0);
    test_wall = 200;
    assert(artifact_mana_debit(&source, 30, true, 1));
    assert(artifact_mana_inspect(&source, record) && record.reserve == 70);
    char blade[] = "blade";
    do_itemmana(&stranger, blade, 0);
    assert(output.find("0.070") == std::string::npos); // another holder cannot inspect
    output.clear(); do_itemmana(&owner, blade, 0);
    assert(output.find("0.070 / 1.000") != std::string::npos);
    // Moving custody and slot does not touch the authoritative pool.
    owner.carrying = nullptr; stranger.equipment[0] = &source;
    output.clear(); do_itemmana(&stranger, blade, 0);
    assert(output.find("0.070 / 1.000") != std::string::npos);
    duplicate.obj_uid = source.obj_uid; duplicate.R_num = 0;
    source.next = &duplicate;
    assert(!artifact_mana_debit(&source, 1, false, 2));
    assert(!artifact_mana_inspect(&duplicate, record));
    source.next = nullptr;
    for (float invalid : {0.0f, 1.5f, std::numeric_limits<float>::quiet_NaN()}) {
        mana_enabled = invalid;
        assert(!artifact_mana_debit(&source, 1, false, 2));
    }
    mana_enabled = 1; master = 0;
    assert(!artifact_mana_debit(&source, 1, false, 2));
    master = 1;
    assert(artifact_mana_inspect(&source, record) && record.reserve == 70); // no toggle refill
    auto invalid = profile; invalid.capacity = 2000;
    assert(!artifact_mana_publish(900, invalid)); // unchanged revision
    assert(artifact_mana_inspect(&source, record) && record.reserve == 70);
    profile.revision = 2; profile.capacity = 50;
    assert(artifact_mana_publish(900, profile));
    assert(artifact_mana_inspect(&source, record) && record.reserve == 50);
    assert(artifact_mana_publish(901, profile));
    source.R_num = 1; // form changes preserve a shared profile
    assert(artifact_mana_debit(&source, 50, false, 2));
    assert(artifact_mana_inspect(&source, record) && record.reserve == 0);
    profile.revision = 3; profile.capacity = 1000;
    assert(artifact_mana_publish(900, profile));
    assert(artifact_mana_inspect(&source, record) && record.reserve == 0);
    invalid = profile; invalid.id = 8;
    assert(!artifact_mana_publish(900, invalid));
    game_thread = false;
    assert(!artifact_mana_debit(&source, 1, false, 3));
    assert(!artifact_mana_publish(900, profile));
    game_thread = true;
    artifact_mana_shutdown();
}
'''
with tempfile.TemporaryDirectory(prefix="duris-mana-game-") as directory:
    work = Path(directory)
    (work / "test.cpp").write_text(HARNESS)
    binary = work / "test"
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie", "-D__NO_MYSQL__",
        "-I", str(ROOT / "src"), "-I", str(ROOT / "src/no_mysql"), str(work / "test.cpp"),
        *[str(ROOT / "src/item" / f"artifact_mana_{part}.c") for part in ("model", "runtime", "store")],
        str(ROOT / "src/flatfile/flatfile_store.c"), "-lcrypto", "-pthread", "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary), str(work / "authority")], check=True, timeout=30)
print("mana bridge UID, owner inspection, conservation, reload and disable/enable checks passed")
