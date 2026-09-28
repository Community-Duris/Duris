#!/usr/bin/env python3
"""Exercise real pool capacity recovery, failed reopen, and shutdown ordering."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "sql/sql_pool.h"
#include "sql/sql_exclusion_guard.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <new>
#include <thread>

namespace {
std::atomic<unsigned long> next_id{1}, opened{0}, closed{0};
std::atomic<bool> fail_open{false};
std::atomic<bool> throw_open{false};
std::mutex factory_mutex;
std::condition_variable factory_changed;
bool pause_open = false, entered = false, resume_open = false;
void pause_factory() {
    std::lock_guard<std::mutex> lock(factory_mutex);
    pause_open = true; entered = false; resume_open = false;
}
void await_factory() {
    std::unique_lock<std::mutex> lock(factory_mutex);
    assert(factory_changed.wait_for(lock, std::chrono::seconds(3), [] { return entered; }));
}
void resume_factory() {
    std::lock_guard<std::mutex> lock(factory_mutex);
    pause_open = false; resume_open = true; factory_changed.notify_all();
}
void retire(MYSQL *connection) {
    connection->server_status = SERVER_STATUS_IN_TRANS;
    const auto before = closed.load();
    sql_pool_discard_connection(connection);
    sql_pool_discard_connection(connection);
    assert(closed.load() == before); // Still owned by the borrower.
    assert(sql_pool_replace_connection(connection) == nullptr);
    sql_pool_release(connection);
    assert(closed.load() == before + 1);
}
}
MYSQL *sql_open_configured_connection(unsigned long) {
    {
        std::unique_lock<std::mutex> lock(factory_mutex);
        if (pause_open) {
            entered = true; factory_changed.notify_all();
            assert(factory_changed.wait_for(lock, std::chrono::seconds(3), [] { return resume_open; }));
        }
    }
    if (throw_open) throw std::bad_alloc();
    if (fail_open) return nullptr;
    auto *connection = static_cast<MYSQL *>(std::calloc(1, sizeof(MYSQL)));
    assert(connection);
    connection->server_status = SERVER_STATUS_AUTOCOMMIT;
    connection->thread_id = next_id.fetch_add(1);
    ++opened;
    return connection;
}
void logit(const char *, const char *, ...) {}
extern "C" void mysql_close(MYSQL *connection) {
    assert(connection);
    ++closed;
    std::free(connection);
}
int main() {
    // Repeated faults must not permanently exhaust even a one-slot pool.
    assert(sql_pool_init(1) == 0);
    for (int attempt = 0; attempt < 4; ++attempt) {
        MYSQL *bad = sql_pool_acquire(); assert(bad);
        const auto old_id = bad->thread_id;
        retire(bad);
        int active = 0;
        MYSQL *replacement = sql_pool_acquire_with_status(&active);
        if (!replacement) {
            std::cerr << "FAIL: discarded pool capacity never replenished\n";
            return 1;
        }
        assert(active == 1 && replacement->thread_id != old_id);
        assert(replacement->server_status == SERVER_STATUS_AUTOCOMMIT);
        sql_pool_release(replacement);
    }
    // A failed reopen returns failure, not a dirty handle or legacy fallback;
    // the same empty slot can be recovered by a later borrower.
    retire(sql_pool_acquire());
    fail_open = true;
    int active = 0;
    assert(sql_pool_acquire_with_status(&active) == nullptr && active == 1);
    assert(sql_pool_in_use() == 0);
    fail_open = false;
    throw_open = true;
    assert(sql_pool_acquire_with_status(&active) == nullptr && active == 1);
    assert(sql_pool_in_use() == 0);
    throw_open = false;
    MYSQL *recovered = sql_pool_acquire(); assert(recovered);
    sql_pool_release(recovered);
    sql_pool_shutdown();
    assert(opened == closed);

    // Reopening an empty slot must not hold the global mutex and block a
    // healthy connection's release/acquisition by another worker.
    assert(sql_pool_init(2) == 0);
    MYSQL *bad = sql_pool_acquire(), *healthy = sql_pool_acquire();
    retire(bad);
    pause_factory();
    MYSQL *replacement = nullptr;
    std::thread refiller([&] { replacement = sql_pool_acquire(); });
    await_factory();
    sql_pool_release(healthy);
    assert(sql_pool_acquire() == healthy);
    sql_pool_release(healthy);
    resume_factory(); refiller.join();
    assert(replacement && replacement != healthy);
    sql_pool_release(replacement);
    sql_pool_shutdown();
    assert(opened == closed);

    // Shutdown must wait for the reserved/reopening slot, close the new
    // handle rather than publish it, and never free its slot prematurely.
    assert(sql_pool_init(1) == 0);
    retire(sql_pool_acquire());
    pause_factory();
    replacement = nullptr;
    std::thread during_shutdown([&] { replacement = sql_pool_acquire(); });
    await_factory();
    std::atomic<bool> shutdown_done{false};
    std::thread closer([&] { sql_pool_shutdown(); shutdown_done = true; });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    bool closing_seen = false;
    while (std::chrono::steady_clock::now() < deadline) {
        active = 1;
        assert(sql_pool_acquire_with_status(&active) == nullptr);
        if (!active) { closing_seen = true; break; }
    }
    assert(closing_seen && !shutdown_done);
    resume_factory(); during_shutdown.join(); closer.join();
    assert(!replacement && shutdown_done && sql_pool_total() == 0);
    assert(opened == closed);

    // A new handle is subject to the same runtime exclusion gate as any
    // existing lease; replenishment must not create a bypass.
    assert(sql_pool_init(1) == 0);
    retire(sql_pool_acquire());
    auto &guard = duris_sql_exclusion_guard_state_ref();
    guard.lost = true;
    assert(sql_pool_acquire() == nullptr);
    assert(sql_pool_in_use() == 0);
    guard.lost = false;
    sql_pool_shutdown();
    assert(opened == closed);
    std::cout << "PASS: real pool replenishes retired leases without dirty reuse, blocking healthy leases, or racing shutdown\n";
}
'''

with tempfile.TemporaryDirectory(prefix="sql-pool-discard-recovery-") as directory:
    directory = Path(directory)
    source = directory / "harness.cpp"
    source.write_text(HARNESS)
    binary = directory / "harness"
    cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-pthread", "-Isrc", *cflags,
                    str(source), "src/sql/sql_pool.c", *libs, "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=15)
