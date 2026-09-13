#!/usr/bin/env python3
"""Real fixed-point model, async worker and durable flat-file CAS under sanitizers."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "item/artifact_mana_runtime.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <mutex>
#include <thread>
#include <sys/stat.h>

const artifact_mana_profile profile{7, 1, 100000, 1, 0};
struct memory_store final : artifact_mana_backend {
    std::mutex mutex;
    std::condition_variable wake;
    std::map<uint64_t, artifact_mana_record> data;
    bool blocked = false, fail = false, lose_ack = false;
    std::atomic<int> writes{0};
    artifact_mana_read read(uint64_t uid, artifact_mana_record &record) override {
        std::lock_guard lock(mutex);
        if (!data.count(uid)) return artifact_mana_read::missing;
        record = data.at(uid);
        return artifact_mana_read::found;
    }
    bool write(uint64_t expected, const artifact_mana_record &record) override {
        std::unique_lock lock(mutex);
        ++writes;
        wake.wait(lock, [&] { return !blocked; });
        if (fail) return false;
        auto found = data.find(record.uid);
        if (found != data.end() && found->second == record) return true;
        if ((!expected && found != data.end()) ||
            (expected && (found == data.end() || found->second.version != expected))) return false;
        data[record.uid] = record;
        if (lose_ack) { lose_ack = false; return false; }
        return true;
    }
    void block(bool value) {
        std::lock_guard lock(mutex); blocked = value; wake.notify_all();
    }
};
void worker_pause() { std::this_thread::sleep_for(std::chrono::milliseconds(2)); }
void drain(artifact_mana_runtime &runtime, uint64_t mono) {
    const auto start = std::chrono::steady_clock::now();
    for (uint64_t tries = 0; std::chrono::steady_clock::now() - start < std::chrono::seconds(30); ++tries) {
        // An old failed write can arrive after the backend recovers. Its retry
        // deadline is based on the pulse that observes it; advance the fake
        // clock so that retry is reachable even when CI schedules it late.
        // Spending-window assertions below still use their exact fixed times.
        runtime.pulse(mono + tries * 10);
        const auto h = runtime.health();
        if (!h.dirty && !h.outstanding) return;
        worker_pause();
    }
    const auto h = runtime.health();
    std::fprintf(stderr, "drain timeout: cached=%zu dirty=%zu outstanding=%zu failures=%llu\n",
                 h.cached, h.dirty, h.outstanding, static_cast<unsigned long long>(h.write_failures));
    assert(false && "worker failed to drain");
}
bool ready(artifact_mana_runtime &runtime, uint64_t uid, uint64_t wall, uint64_t mono) {
    artifact_mana_record record;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (std::chrono::steady_clock::now() < deadline) {
        runtime.pulse(mono);
        if (runtime.inspect(uid, profile, wall, mono, record)) return true;
        worker_pause();
    }
    return false;
}
void model() {
    auto record = artifact_mana_empty(42, profile, 100);
    assert(record.reserve == 0 && record.version == 1);
    auto old = record;
    assert(!artifact_mana_spend(record, profile, 100, 1, false) && record == old);
    assert(artifact_mana_spend(record, profile, 101, 1, false)); // exact 0.001
    assert(record.reserve == 0 && record.version == 2);
    assert(!artifact_mana_spend(record, profile, 101, 1, false));
    assert(artifact_mana_project(record, profile, 90)); // backwards clock
    assert(record.reserve == 0 && record.settled_at == 101);
    assert(artifact_mana_project(record, profile, UINT64_MAX));
    assert(record.reserve == ARTIFACT_MANA_MAX_OFFLINE_SECONDS);
    auto changed = profile;
    changed.revision = 2; changed.capacity = 50000;
    assert(artifact_mana_project(record, changed, UINT64_MAX));
    assert(record.reserve == 50000);
    changed.revision = 3; changed.capacity = 200000;
    assert(artifact_mana_project(record, changed, UINT64_MAX));
    assert(record.reserve == 50000); // larger capacity never fills
    assert(!artifact_mana_project(record, profile, UINT64_MAX));
    changed.passive_floor = 10000;
    assert(!artifact_mana_spend(record, changed, UINT64_MAX, 40001, true));
    assert(artifact_mana_spend(record, changed, UINT64_MAX, 40000, true));
    assert(artifact_mana_spend(record, changed, UINT64_MAX, 10000, false));
    old = record;
    assert(!artifact_mana_spend(record, changed, UINT64_MAX, 0, false) && record == old);
    assert(!artifact_mana_spend(record, changed, UINT64_MAX, UINT64_MAX, false));
    record.version = UINT64_MAX;
    assert(!artifact_mana_spend(record, changed, UINT64_MAX, 1, false));
    changed.capacity = ARTIFACT_MANA_MAXIMUM + 1;
    assert(!artifact_mana_valid(changed));
    changed = profile; changed.regeneration = UINT64_MAX;
    assert(!artifact_mana_valid(changed));
    changed = profile; changed.passive_floor = profile.capacity + 1;
    assert(!artifact_mana_valid(changed));
    changed = profile; changed.id = 8;
    assert(!artifact_mana_project(old, changed, 100000));
    changed = profile; changed.capacity += 1;
    old = artifact_mana_empty(42, profile, 100);
    assert(!artifact_mana_project(old, changed, 100)); // revision collision
}
void worker() {
    auto owned = std::make_unique<memory_store>();
    auto *store = owned.get();
    artifact_mana_runtime runtime(std::move(owned));
    artifact_mana_record record;
    assert(!runtime.debit(42, profile, 100, 1000, 1, false, 1)); // no cold-load credit
    assert(ready(runtime, 42, 100, 1000));
    assert(runtime.spending_ready(42, 1000));
    assert(runtime.inspect(42, profile, 100, 1000, record) && record.reserve == 0);
    assert(runtime.debit(42, profile, 200, 1001, 1, false, 1));
    assert(!runtime.debit(42, profile, 200, 1002, 1, false, 1)); // same token
    drain(runtime, 1100);
    assert(runtime.inspect(42, profile, 200, 1100, record) && record.reserve == 99);
    store->block(true);
    assert(runtime.debit(42, profile, 200, 2000, 1, false, 2));
    assert(runtime.debit(42, profile, 200, 3999, 1, false, 3));
    assert(!runtime.debit(42, profile, 200, 4000, 1, false, 4)); // exactly two seconds
    assert(!runtime.spending_ready(42, 4000));
    assert(!runtime.debit(42, profile, 200, 1999, 1, false, 4)); // monotonic rollback
    store->block(false);
    drain(runtime, 5000);
    assert(runtime.spending_ready(42, 5000));
    assert(runtime.debit(42, profile, 200, 5001, 1, false, 4));
    assert(!runtime.debit(42, profile, 200, 5002, 1, false, 2)); // old token after new
    drain(runtime, 5100);
    {
        std::lock_guard lock(store->mutex);
        store->lose_ack = true;
        store->blocked = true;
    }
    assert(runtime.debit(42, profile, 200, 6000, 1, false, 5));
    assert(runtime.debit(42, profile, 200, 6001, 1, false, 6));
    store->block(false);
    // Lost acknowledgement must retry the exact older snapshot before the
    // coalesced newer debit; otherwise CAS would wedge or refund this pool.
    for (int i = 0; i < 1000 && !runtime.health().write_failures; ++i) {
        runtime.pulse(6100); worker_pause();
    }
    assert(runtime.health().write_failures == 1);
    drain(runtime, 8000);
    assert(runtime.inspect(42, profile, 200, 8000, record) && record.reserve == 94);
    {
        std::lock_guard lock(store->mutex);
        assert(store->data.at(42) == record);
        store->fail = true;
    }
    assert(runtime.debit(42, profile, 200, 9000, 1, false, 7));
    for (int i = 0; i < 100; ++i) { runtime.pulse(9500); worker_pause(); }
    assert(!runtime.debit(42, profile, 200, 11000, 1, false, 8));
    assert(runtime.inspect(42, profile, 200, 11000, record) && record.reserve == 93);
    {
        std::lock_guard lock(store->mutex);
        store->fail = false;
    }
    drain(runtime, 13000);
    // Completed reads still occupy the bounded admission queue until the main
    // thread polls them. Queue pressure may suppress, never grant cold credit.
    for (uint64_t uid = 1000; uid < 6000; ++uid)
        assert(!runtime.inspect(uid, profile, 200, 13000, record));
    assert(runtime.health().outstanding == 4096);
    assert(!runtime.debit(7000, profile, 200, 13000, 1, false, 8));
    drain(runtime, 14000);
    runtime.stop();
    assert(!runtime.debit(42, profile, 200, 14000, 1, false, 8));
}
void files(const std::string &root) {
    std::filesystem::create_directories(root + "/domains");
    chmod(root.c_str(), 0700); chmod((root + "/domains").c_str(), 0700);
    artifact_mana_record record;
    assert(artifact_mana_store_read(false, root, 81, record) == artifact_mana_read::missing);
    record = artifact_mana_empty(81, profile, 100);
    assert(artifact_mana_store_write(false, root, 0, record));
    auto previous = record;
    assert(artifact_mana_spend(record, profile, 1000, 333, false));
    assert(artifact_mana_store_write(false, root, 1, record));
    assert(artifact_mana_store_write(false, root, 1, record)); // exact retry
    assert(!artifact_mana_store_write(false, root, 0, previous)); // stale owner snapshot
    auto stale = record; stale.reserve += 1;
    assert(!artifact_mana_store_write(false, root, 1, stale));
    {
        artifact_mana_runtime restored(artifact_mana_persistent_backend(false, root));
        assert(ready(restored, 81, 1000, 2000));
        artifact_mana_record recovered;
        assert(restored.inspect(81, profile, 1000, 2000, recovered));
        assert(recovered == record); // restart/transfer reload by UID
        assert(ready(restored, 82, 1000, 2000)); // cloned new UID starts empty
        assert(restored.inspect(82, profile, 1000, 2000, recovered) && recovered.reserve == 0);
    }
    std::fstream damaged(root + "/domains/artifact-mana-81", std::ios::binary|std::ios::in|std::ios::out);
    damaged.seekp(65); damaged.put(char(17)); damaged.close();
    assert(artifact_mana_store_read(false, root, 81, record) == artifact_mana_read::error);
    assert(!artifact_mana_store_write(false, root, 0, previous));
}
int main(int argc, char **argv) {
    assert(argc == 2); model(); worker(); files(argv[1]);
}
'''

with tempfile.TemporaryDirectory(prefix="duris-artifact-mana-") as directory:
    work = Path(directory)
    harness = work / "harness.cpp"
    harness.write_text(HARNESS)
    binary = work / "test"
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
        "-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie",
        "-D__NO_MYSQL__", "-I", str(ROOT / "src"), str(harness),
        *[str(ROOT / "src/item" / f"artifact_mana_{part}.c") for part in ("model", "runtime", "store")],
        str(ROOT / "src/flatfile/flatfile_store.c"), "-lcrypto", "-pthread", "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary), str(work / "authority")], check=True, timeout=30)
print("artifact mana model, async admission/recovery and flat-file CAS passed")
