#!/usr/bin/env python3
"""Exercise the bounded asynchronous critical-command journal admission lane."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, rel


HARNESS = r'''
#include "persistence/critical_command_coordinator.c"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <thread>
#include <vector>

static std::atomic<unsigned int> slow_fsyncs{0};
static std::atomic<unsigned int> write_failures{0};
static std::atomic<unsigned int> applied{0};
static std::thread::id submitter;

extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd)
{
    if (std::this_thread::get_id() != submitter)
    {
        unsigned int expected = slow_fsyncs.load();
        while (expected &&
               !slow_fsyncs.compare_exchange_weak(expected, expected - 1))
        {
        }
        if (expected)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return __real_fsync(fd);
}

extern "C" ssize_t __real_write(int, const void *, size_t);
extern "C" ssize_t __wrap_write(int fd, const void *data, size_t size)
{
    const int flags = fcntl(fd, F_GETFL);
    if (flags >= 0 && (flags & O_APPEND) && write_failures.exchange(0))
    {
        errno = ENOSPC;
        return -1;
    }
    return __real_write(fd, data, size);
}

static critical_command command(unsigned int tag)
{
    critical_command result = {};
    result.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
    assert(critical_operation_id_generate(&result.operation_id));
    result.type = critical_command_type::test;
    result.payload_version = 1;
    result.source_site = critical_source_site::command;
    result.deadline_class = critical_deadline_class::interactive;
    result.keys = {{critical_entity_type::player, 1000 + tag}};
    result.payload = {static_cast<uint8_t>(tag)};
    return result;
}

static critical_apply_result apply(const critical_command &, void *)
{
    ++applied;
    return {critical_apply_outcome::applied, 1, 0};
}

template <typename F> static void wait_for(F condition)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!condition())
    {
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

static unsigned long long percentile(std::vector<unsigned long long> values,
                                     unsigned int percentage)
{
    std::sort(values.begin(), values.end());
    const size_t index = (values.size() - 1) * percentage / 100;
    return values[index];
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    submitter = std::this_thread::get_id();
    std::filesystem::remove_all(argv[1]);

    assert(critical_command_coordinator_init(argv[1], apply, nullptr, 1));
    const critical_command slow = command(1);
    slow_fsyncs = 1;
    const auto begin = std::chrono::steady_clock::now();
    assert(critical_command_coordinator_submit(slow) ==
           critical_submit_result::awaiting_durability);
    const auto submit_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - begin)
                               .count();
    auto health = critical_command_coordinator_health_copy();
    assert(submit_ms < 40);
    assert(health.awaiting_durability >= 1 && health.admission_queue_bytes > 0);
    assert(critical_command_coordinator_durability(slow.operation_id) ==
           critical_command_durability::awaiting_durability);

    bool saw_awaiting_without_apply = false;
    unsigned int simulation_pulses = 0;
    wait_for([&] {
        ++simulation_pulses;
        critical_command_coordinator_pulse(nullptr, 0);
        const auto durability =
            critical_command_coordinator_durability(slow.operation_id);
        if (durability == critical_command_durability::awaiting_durability)
        {
            saw_awaiting_without_apply = true;
            assert(applied == 0);
        }
        return durability == critical_command_durability::durable;
    });
    assert(saw_awaiting_without_apply);
    assert(simulation_pulses > 10);
    assert(critical_command_coordinator_drain(5000));
    assert(applied == 1);
    printf("slow_append: submit_ms=%lld simulation_pulses_while_awaiting=%u\n",
           static_cast<long long>(submit_ms), simulation_pulses);
    critical_command_coordinator_shutdown();

    std::filesystem::remove_all(argv[1]);
    applied = 0;
    assert(critical_command_coordinator_init(argv[1], apply, nullptr, 2));
    slow_fsyncs = 32;
    std::vector<unsigned long long> submit_latencies;
    for (unsigned int tag = 2; tag < 34; ++tag)
    {
        const auto request = command(tag);
        const auto started = std::chrono::steady_clock::now();
        assert(critical_command_coordinator_submit(request) ==
               critical_submit_result::awaiting_durability);
        submit_latencies.push_back(static_cast<unsigned long long>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - started)
                .count()));
    }
    health = critical_command_coordinator_health_copy();
    assert(health.awaiting_durability > 0 && health.admission_queue_bytes > 0);
    printf("slow_append_distribution: n=%zu p50_us=%llu p95_us=%llu p99_us=%llu awaiting=%llu queue_bytes=%llu\n",
           submit_latencies.size(), percentile(submit_latencies, 50),
           percentile(submit_latencies, 95), percentile(submit_latencies, 99),
           static_cast<unsigned long long>(health.awaiting_durability),
           static_cast<unsigned long long>(health.admission_queue_bytes));
    assert(percentile(submit_latencies, 95) < 40000);
    assert(critical_command_coordinator_drain(15000));
    assert(applied == 32);
    critical_command_coordinator_shutdown();

    std::filesystem::remove_all(argv[1]);
    applied = 0;
    assert(critical_command_coordinator_init(argv[1], apply, nullptr, 1));
    const critical_command failed = command(99);
    write_failures = 1;
    assert(critical_command_coordinator_submit(failed) ==
           critical_submit_result::awaiting_durability);
    wait_for([&] {
        return critical_command_coordinator_durability(failed.operation_id) ==
               critical_command_durability::failed;
    });
    critical_completion completion = {};
    assert(critical_command_coordinator_pulse(&completion, 1) == 1);
    assert(critical_operation_id_equal(completion.operation_id, failed.operation_id));
    assert(completion.outcome == critical_apply_outcome::terminal_failure);
    assert(completion.error_code == EIO);
    assert(!critical_command_coordinator_is_fenced(failed.keys[0], nullptr));
    assert(applied == 0);
    assert(critical_command_coordinator_drain(5000));
    printf("append_failure: terminal_error=%u retained_until_delivery=1\n",
           completion.error_code);
    critical_command_coordinator_shutdown();
}
'''


with tempfile.TemporaryDirectory(prefix="duris-critical-admission-") as directory:
    temporary = Path(directory)
    source = temporary / "admission.cpp"
    binary = temporary / "admission"
    source.write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        [
            "g++", "-std=c++20", "-g", "-Og", "-Wall", "-Wextra", "-Wpedantic",
            "-Werror", "-pthread", "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-Isrc", str(source),
            rel("critical_command.c"), rel("critical_command_journal.c"),
            "-lz", "-lcrypto", "-Wl,--wrap=fsync", "-Wl,--wrap=write",
            "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(temporary / "journal")], check=True, timeout=60)

print("asynchronous critical-command journal admission regression passed")
