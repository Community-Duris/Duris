#!/usr/bin/env python3
"""Exercise the real pool's deadlines, borrower shutdown, capacity recovery and exclusion gates."""
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
#include <cerrno>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <mutex>
#include <new>
#include <pthread.h>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
std::atomic<unsigned long> next_id{1}, opened{0}, closed{0};
std::atomic<bool> fail_open{false};
std::atomic<bool> throw_open{false};
std::atomic<unsigned> acquire_wait_calls{0};
thread_local int last_acquire_wait_result = ETIMEDOUT;
std::mutex factory_mutex;
std::condition_variable factory_changed;
bool pause_open = false, entered = false, resume_open = false;
struct ProbeReply {
    int query_error = 0;
    std::string value = "1";
    unsigned int fields = 1;
    my_ulonglong rows = 1;
    bool null_value = false;
    bool additional_result = false;
};
std::mutex probe_mutex;
std::deque<ProbeReply> probe_replies;
std::vector<std::string> observed_probe_queries;
thread_local ProbeReply current_probe_reply;
thread_local std::string current_probe_value;
thread_local char *current_probe_row[1];
thread_local unsigned long current_probe_lengths[1];
thread_local int fake_result;
thread_local bool current_probe_query_failed = false;
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
    const int leased_before = sql_pool_in_use();
    sql_pool_discard_connection(connection);
    sql_pool_discard_connection(connection);
    assert(closed.load() == before); // Still owned by the borrower.
    assert(sql_pool_replace_connection(connection) == nullptr);
    // Replacement consumes only this lease, retaining other borrowers.
    assert(sql_pool_in_use() == leased_before - 1);
    assert(closed.load() == before + 1);
}
void queue_probe(ProbeReply reply = {}) {
    std::lock_guard<std::mutex> lock(probe_mutex);
    probe_replies.push_back(std::move(reply));
}
void configure_guard(MYSQL *owner) {
    auto &guard = duris_sql_exclusion_guard_state_ref();
    guard.connection = owner;
    guard.connection_id = 700;
    guard.process_id = getpid();
    guard.economic_connection_id = 701;
    guard.lost = false;
}
void clear_guard() {
    auto &guard = duris_sql_exclusion_guard_state_ref();
    guard.connection = nullptr;
    guard.connection_id = 0;
    guard.process_id = 0;
    guard.economic_connection_id = 0;
    guard.lost = false;
}
void test_guard_probe_recovery() {
    MYSQL owner{};
    configure_guard(&owner);
    ProbeReply query_error; query_error.query_error = 1;
    MYSQL synchronous_probe{};
    queue_probe(query_error);
    assert(!duris_sql_exclusion_guard_allows(&synchronous_probe));
    assert(!duris_sql_exclusion_guard_state_ref().lost);

    // A query error on a pooled worker is inconclusive. Retire that worker,
    // validate a fresh connection against both original lock owner IDs, and
    // return it without latching loss.
    queue_probe();
    assert(sql_pool_init(1) == 0);
    queue_probe();
    MYSQL *stale = sql_pool_acquire(); assert(stale);
    const auto stale_id = stale->thread_id;
    sql_pool_release(stale);
    queue_probe(query_error);
    queue_probe();
    queue_probe();
    MYSQL *recovered = sql_pool_acquire(); assert(recovered);
    assert(recovered->thread_id != stale_id && !duris_sql_exclusion_guard_state_ref().lost);
    {
        std::lock_guard<std::mutex> lock(probe_mutex);
        assert(probe_replies.empty());
        bool checked_original_owners = false;
        for (const auto &query : observed_probe_queries)
            checked_original_owners |= query.find(")=700 AND") != std::string::npos &&
                                       query.find("),0)=701,1,0") != std::string::npos;
        assert(checked_original_owners);
    }
    sql_pool_release(recovered);
    sql_pool_shutdown();
    assert(opened == closed);
    clear_guard();

    // Unknown scalar values and malformed result shapes are inconclusive too.
    configure_guard(&owner);
    queue_probe();
    assert(sql_pool_init(1) == 0);
    queue_probe();
    stale = sql_pool_acquire(); assert(stale);
    sql_pool_release(stale);
    ProbeReply malformed_value; malformed_value.value = "2";
    queue_probe(malformed_value);
    queue_probe();
    queue_probe();
    recovered = sql_pool_acquire(); assert(recovered);
    assert(!duris_sql_exclusion_guard_state_ref().lost);
    sql_pool_release(recovered);
    sql_pool_shutdown();
    assert(opened == closed);
    clear_guard();

    configure_guard(&owner);
    queue_probe();
    assert(sql_pool_init(1) == 0);
    queue_probe();
    stale = sql_pool_acquire(); assert(stale);
    sql_pool_release(stale);
    ProbeReply malformed_shape; malformed_shape.fields = 2;
    queue_probe(malformed_shape);
    queue_probe();
    queue_probe();
    recovered = sql_pool_acquire(); assert(recovered);
    assert(!duris_sql_exclusion_guard_state_ref().lost);
    sql_pool_release(recovered);
    sql_pool_shutdown();
    assert(opened == closed);
    clear_guard();

    // A failed configured replacement leaves a recoverable empty slot.
    configure_guard(&owner);
    queue_probe();
    assert(sql_pool_init(1) == 0);
    queue_probe();
    stale = sql_pool_acquire(); assert(stale);
    sql_pool_release(stale);
    queue_probe(query_error);
    queue_probe(query_error);
    int active = 0;
    assert(sql_pool_acquire_with_status(&active) == nullptr && active == 1);
    assert(sql_pool_in_use() == 0 && sql_pool_available() == 0);
    assert(!duris_sql_exclusion_guard_state_ref().lost);
    queue_probe();
    queue_probe();
    recovered = sql_pool_acquire(); assert(recovered);
    sql_pool_release(recovered);
    sql_pool_shutdown();
    assert(opened == closed);
    clear_guard();

    // Only a valid "0" proves owner loss; a positive later answer cannot
    // replace either original lock owner.
    configure_guard(&owner);
    queue_probe();
    assert(sql_pool_init(1) == 0);
    queue_probe();
    stale = sql_pool_acquire(); assert(stale);
    sql_pool_release(stale);
    ProbeReply owner_lost; owner_lost.value = "0";
    queue_probe(owner_lost);
    assert(sql_pool_acquire() == nullptr);
    assert(duris_sql_exclusion_guard_state_ref().lost);
    queue_probe();
    assert(sql_pool_acquire() == nullptr);
    assert(!probe_replies.empty());
    sql_pool_shutdown();
    assert(opened == closed);
    clear_guard();
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
    if (duris_sql_exclusion_guard_check(connection) !=
        duris_sql_exclusion_guard_status::allowed) {
        mysql_close(connection);
        return nullptr;
    }
    return connection;
}
void logit(const char *, const char *, ...) {}
extern "C" int STDCALL mysql_real_query(MYSQL *, const char *query, unsigned long length) {
    std::lock_guard<std::mutex> lock(probe_mutex);
    observed_probe_queries.emplace_back(query, length);
    assert(!probe_replies.empty());
    current_probe_reply = std::move(probe_replies.front());
    probe_replies.pop_front();
    current_probe_value = current_probe_reply.value;
    current_probe_query_failed = current_probe_reply.query_error != 0;
    return current_probe_reply.query_error;
}
extern "C" MYSQL_RES *STDCALL mysql_store_result(MYSQL *) {
    return current_probe_query_failed ? nullptr : reinterpret_cast<MYSQL_RES *>(&fake_result);
}
extern "C" unsigned int STDCALL mysql_num_fields(MYSQL_RES *) {
    return current_probe_reply.fields;
}
extern "C" my_ulonglong STDCALL mysql_num_rows(MYSQL_RES *) {
    return current_probe_reply.rows;
}
extern "C" MYSQL_ROW STDCALL mysql_fetch_row(MYSQL_RES *) {
    current_probe_row[0] = current_probe_reply.null_value ? nullptr :
                           const_cast<char *>(current_probe_value.c_str());
    return current_probe_reply.rows ? current_probe_row : nullptr;
}
extern "C" unsigned long *STDCALL mysql_fetch_lengths(MYSQL_RES *) {
    current_probe_lengths[0] = static_cast<unsigned long>(current_probe_value.size());
    return current_probe_lengths;
}
extern "C" void STDCALL mysql_free_result(MYSQL_RES *) {}
extern "C" int STDCALL mysql_next_result(MYSQL *) {
    return current_probe_reply.additional_result ? 0 : -1;
}
extern "C" void mysql_close(MYSQL *connection) {
    assert(connection);
    ++closed;
    std::free(connection);
}
// Observe entry into the real wait without replacing its synchronization or
// timeout. It holds pool_mutex until the wait atomically releases it, so seeing
// this counter guarantees shutdown will encounter an already-waiting borrower.
extern "C" int __real_pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *, const timespec *);
extern "C" int __wrap_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const timespec *deadline) {
    ++acquire_wait_calls;
    last_acquire_wait_result = __real_pthread_cond_timedwait(cond, mutex, deadline);
    return last_acquire_wait_result;
}
int main() {
    // Exhaustion is an active-pool failure, never permission to fall back to a
    // shared legacy connection. Releasing the lease restores usable capacity.
    int inactive = 1;
    assert(!sql_pool_acquire_with_status(&inactive) && inactive == 0);
    assert(sql_pool_init(1) == 0);
    MYSQL *borrowed = sql_pool_acquire(); assert(borrowed);
    int exhausted = 0;
    const auto wait_started = std::chrono::steady_clock::now();
    assert(!sql_pool_acquire_with_status(&exhausted) && exhausted == 1);
    const auto waited = std::chrono::steady_clock::now() - wait_started;
    assert(waited >= std::chrono::milliseconds(SQL_POOL_ACQUIRE_TIMEOUT_MS / 2));
    assert(waited < std::chrono::seconds(5));
    assert(sql_pool_in_use() == 1 && sql_pool_available() == 0);

    // Wake a waiting borrower on shutdown without closing the outstanding
    // lease. The old substring checks could not prove either ordering.
    const auto closed_before_shutdown = closed.load();
    MYSQL *waiting_result = borrowed;
    int waiting_wake_result = ETIMEDOUT;
    const auto waits_before_shutdown = acquire_wait_calls.load();
    std::thread waiting([&] {
        waiting_result = sql_pool_acquire();
        waiting_wake_result = last_acquire_wait_result;
    });
    const auto wait_entry_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (acquire_wait_calls == waits_before_shutdown && std::chrono::steady_clock::now() < wait_entry_deadline)
        std::this_thread::yield();
    assert(acquire_wait_calls > waits_before_shutdown);
    std::atomic<bool> borrowed_shutdown_done{false};
    std::thread borrowed_closer([&] { sql_pool_shutdown(); borrowed_shutdown_done = true; });
    bool borrowed_closing_seen = false;
    const auto closing_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < closing_deadline) {
        int still_active = 1;
        assert(!sql_pool_acquire_with_status(&still_active));
        if (!still_active) { borrowed_closing_seen = true; break; }
    }
    assert(borrowed_closing_seen);
    waiting.join();
    assert(waiting_wake_result == 0); // A broadcast, rather than deadline expiry, woke the waiter.
    assert(!waiting_result && !borrowed_shutdown_done);
    assert(closed.load() == closed_before_shutdown);
    assert(!sql_pool_replace_connection(borrowed));
    assert(sql_pool_in_use() == 0);
    borrowed_closer.join();
    assert(borrowed_shutdown_done && opened == closed && sql_pool_total() == 0);

    // A failed replacement consumes the old lease; no caller may touch its
    // pointer afterward, even when the connection factory throws.
    for (int fault = 0; fault < 2; ++fault) {
        assert(sql_pool_init(1) == 0);
        MYSQL *original = sql_pool_acquire(); assert(original);
        const auto prior_closed = closed.load();
        fail_open = fault == 0; throw_open = fault == 1;
        assert(sql_pool_replace_connection(original) == nullptr);
        fail_open = false; throw_open = false;
        assert(sql_pool_in_use() == 0 && sql_pool_available() == 0);
        assert(closed.load() == prior_closed + 1);
        MYSQL *fresh = sql_pool_acquire(); assert(fresh);
        sql_pool_release(fresh); sql_pool_shutdown();
        assert(opened == closed);
    }

    // A slow replacement cannot hold the pool mutex or block an unrelated
    // healthy lease; successful replacement returns one clean borrowed handle.
    assert(sql_pool_init(2) == 0);
    MYSQL *slow = sql_pool_acquire(), *other = sql_pool_acquire();
    assert(slow && other);
    const auto slow_id = slow->thread_id;
    pause_factory();
    MYSQL *fresh_replacement = nullptr;
    std::thread slow_replacer([&] { fresh_replacement = sql_pool_replace_connection(slow); });
    await_factory();
    sql_pool_release(other);
    assert(sql_pool_acquire() == other);
    sql_pool_release(other);
    resume_factory(); slow_replacer.join();
    assert(fresh_replacement && fresh_replacement != other && fresh_replacement->thread_id != slow_id);
    assert(sql_pool_in_use() == 1 && sql_pool_available() == 1);
    sql_pool_release(fresh_replacement); sql_pool_shutdown();
    assert(opened == closed);

    // Closing while the replacement factory is outside the mutex consumes
    // both the original and unpublished fresh handle, and releases shutdown.
    assert(sql_pool_init(1) == 0);
    MYSQL *original = sql_pool_acquire(); assert(original);
    pause_factory();
    MYSQL *replacing_result = original;
    std::thread replacing([&] { replacing_result = sql_pool_replace_connection(original); });
    await_factory();
    std::atomic<bool> replacement_shutdown_done{false};
    std::thread replacement_closer([&] { sql_pool_shutdown(); replacement_shutdown_done = true; });
    bool replacement_closing_seen = false;
    const auto replacement_closing_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < replacement_closing_deadline) {
        int still_active = 1;
        assert(!sql_pool_acquire_with_status(&still_active));
        if (!still_active) { replacement_closing_seen = true; break; }
    }
    assert(replacement_closing_seen && !replacement_shutdown_done);
    resume_factory(); replacing.join();
    assert(!replacing_result && sql_pool_in_use() == 0);
    replacement_closer.join();
    assert(replacement_shutdown_done && opened == closed);

    // An unborrowed handle is not a lease and must never be replaced/closed.
    assert(sql_pool_init(1) == 0);
    MYSQL *unborrowed = sql_pool_acquire(); assert(unborrowed);
    sql_pool_release(unborrowed);
    const auto prior_closed = closed.load();
    assert(!sql_pool_replace_connection(unborrowed));
    assert(closed.load() == prior_closed && sql_pool_available() == 1);
    sql_pool_shutdown(); assert(opened == closed);

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
    test_guard_probe_recovery();
    std::cout << "PASS: pool replenishment preserves capacity, lock ownership, borrower ordering, and shutdown fencing\n";
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
                    "-pthread", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-fno-pie", "-no-pie", "-Isrc", *cflags,
                    str(source), "src/sql/sql_pool.c", "-Wl,--wrap=pthread_cond_timedwait", *libs,
                    "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=15)
