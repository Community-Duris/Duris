#!/usr/bin/env python3
"""Runtime and source regression for the legacy profiling clock and units."""

from pathlib import Path
import subprocess
import tempfile

from _paths import SRC
from contract_text import contains


HARNESS = r'''
#include <stddef.h>
#include <stdint.h>
#include <time.h>

struct fake_sample
{
	int result;
	time_t seconds;
	long nanoseconds;
};

static fake_sample samples[16];
static size_t sample_count = 0;
static size_t sample_index = 0;

static int fake_clock_gettime(clockid_t clock_id, struct timespec *result)
{
	if (clock_id != CLOCK_MONOTONIC || sample_index >= sample_count)
		return -1;
	const fake_sample sample = samples[sample_index++];
	if (sample.result)
		return sample.result;
	result->tv_sec = sample.seconds;
	result->tv_nsec = sample.nanoseconds;
	return 0;
}

#define PROFILE_CLOCK_GETTIME fake_clock_gettime
#include "core/profile.h"

#include <assert.h>
#include <stdio.h>

bool do_profile = true;
PROFILE_DEFINE(probe);

static void set_samples(const fake_sample *values, size_t count)
{
	assert(count <= 16);
	for (size_t index = 0; index < count; ++index)
		samples[index] = values[index];
	sample_count = count;
	sample_index = 0;
}

int main()
{
	const fake_sample normal[] = {
		{ 0, 1, 0 },
		{ 0, 1, 50000000 },
		{ 0, 1, 70000000 },
		{ 0, 1, 100000000 },
		{ 0, 1, 160000000 },
	};
	set_samples(normal, sizeof normal / sizeof normal[0]);
	PROFILE_RESET(probe);
	PROFILE_START(probe);
	PROFILE_END(probe);
	PROFILE_START(probe);
	PROFILE_END(probe);
	assert(probe_profile.calls == 2);
	assert(probe_profile.total_inside_us == 80000);
	assert(probe_profile.total_outside_us == 80000);
	assert(PROFILE_LAST_US(probe) == 60000);

	const fake_sample after_disabled_interval[] = {
		{ 0, 50, 0 },
		{ 0, 50, 10000000 },
		{ 0, 50, 20000000 },
	};
	set_samples(after_disabled_interval,
		    sizeof after_disabled_interval / sizeof after_disabled_interval[0]);
	PROFILE_REBASE(probe);
	PROFILE_START(probe);
	PROFILE_END(probe);
	assert(probe_profile.calls == 3);
	assert(probe_profile.total_inside_us == 90000);
	assert(probe_profile.total_outside_us == 90000);
	assert(PROFILE_LAST_US(probe) == 10000);

	const fake_sample rebase_during_outer_command[] = {
		{ 0, 55, 0 },
 { 0, 55, 50000000 },
	};
	set_samples(rebase_during_outer_command,
		    sizeof rebase_during_outer_command / sizeof rebase_during_outer_command[0]);
	PROFILE_REBASE(probe);
	PROFILE_END(probe);
	assert(sample_index == 2);
 assert(probe_profile.ended_us == 55050000);
	assert(probe_profile.calls == 3);
	assert(probe_profile.total_inside_us == 90000);
	assert(PROFILE_LAST_US(probe) == 0);

	const fake_sample failed_start[] = {
		{ -1, 0, 0 },
		{ 0, 60, 0 },
	};
	set_samples(failed_start, sizeof failed_start / sizeof failed_start[0]);
	PROFILE_START(probe);
	PROFILE_END(probe);
	assert(probe_profile.calls == 3);
	assert(probe_profile.total_inside_us == 90000);
	assert(PROFILE_LAST_US(probe) == 0);

	const fake_sample invalid_nanoseconds[] = {
		{ 0, 65, 1000000000L },
		{ 0, 66, 0 },
	};
	set_samples(invalid_nanoseconds,
		    sizeof invalid_nanoseconds / sizeof invalid_nanoseconds[0]);
	PROFILE_START(probe);
	PROFILE_END(probe);
	assert(probe_profile.calls == 3);
	assert(probe_profile.total_inside_us == 90000);
	assert(PROFILE_LAST_US(probe) == 0);

	const fake_sample failed_end[] = { { 0, 67, 0 }, { -1, 0, 0 } };
 set_samples(failed_end, 2);
 PROFILE_START(probe);
 PROFILE_END(probe);
 assert(probe_profile.calls == 3 && PROFILE_LAST_US(probe) == 0);

 const fake_sample backwards[] = {
		{ 0, 70, 0 },
		{ 0, 69, 0 },
	};
	set_samples(backwards, sizeof backwards / sizeof backwards[0]);
	PROFILE_START(probe);
	PROFILE_END(probe);
	assert(probe_profile.calls == 3);
	assert(probe_profile.total_inside_us == 90000);
	assert(PROFILE_LAST_US(probe) == 0);

	do_profile = false;
	set_samples(normal, sizeof normal / sizeof normal[0]);
	PROFILE_START(probe);
	PROFILE_END(probe);
	assert(sample_index == 0);
	assert(probe_profile.calls == 3);

	puts("monotonic profile timer runtime checks passed");
	return 0;
}
'''

REAL_HARNESS = r'''
#include "core/profile.h"

#include <assert.h>
#include <atomic>
#include <chrono>
#include <stdio.h>
#include <thread>
#include <vector>

bool do_profile = true;
PROFILE_DEFINE(probe);

int main()
{
	std::atomic<bool> running{ true };
	std::vector<std::thread> workers;
	for (int index = 0; index < 8; ++index)
		workers.emplace_back([&running]() {
			volatile uint64_t value = 1;
			while (running.load(std::memory_order_relaxed))
				value = value * 1664525U + 1013904223U;
		});

	PROFILE_RESET(probe);
	PROFILE_START(probe);
	std::this_thread::sleep_for(std::chrono::milliseconds(80));
	PROFILE_END(probe);
	running.store(false, std::memory_order_relaxed);
	for (std::thread &worker : workers)
		worker.join();

	assert(probe_profile.calls == 1);
	assert(PROFILE_LAST_US(probe) >= 60000);
	assert(PROFILE_LAST_US(probe) < 500000);
	puts("profile sleeping interval passed with concurrent worker CPU");
	return 0;
}
'''


profile = (SRC / "core" / "profile.h").read_text()
clock_utils = (SRC / "core" / "clock_utils.h").read_text()
events = (SRC / "world" / "new_events.c").read_text()
debug = (SRC / "core" / "debug.c").read_text()

assert contains(
    profile,
    "clock_read_microseconds(CLOCK_MONOTONIC, result, PROFILE_CLOCK_GETTIME)",
)
assert contains(clock_utils, "seconds > (UINT64_MAX - fractional_us) / 1000000ULL")
assert not contains(profile, "clock()")
assert not contains(events, "CLOCKS_PER_SEC")
assert contains(events, "duration_us >= 50000")
assert contains(events, '" duration_us=%" PRIu64')
assert contains(events, '" total_calls=%" PRIu64 " average_us=%.0f')
assert contains(debug, "PROFILES(REBASE);")
assert contains(profile, "skip_next_end")

with tempfile.TemporaryDirectory(prefix="duris-profile-") as directory:
    temporary = Path(directory)
    harness = temporary / "profile_test.c"
    binary = temporary / "profile_test"
    real_harness = temporary / "profile_real_test.c"
    real_binary = temporary / "profile_real_test"
    harness.write_text(HARNESS)
    real_harness.write_text(REAL_HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-g",
            "-O1",
            "-fsanitize=address,undefined",
            "-I",
            str(SRC),
            str(harness),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-g",
            "-O1",
            "-fsanitize=address,undefined",
            "-I",
            str(SRC),
            str(real_harness),
            "-pthread",
            "-o",
            str(real_binary),
        ],
        check=True,
    )
    subprocess.run([str(real_binary)], check=True)

print("legacy profiler monotonic clock and unit checks passed")
