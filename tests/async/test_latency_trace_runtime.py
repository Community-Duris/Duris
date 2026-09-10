#!/usr/bin/env python3
"""Runtime regression for correlated, windowed latency tracing."""

from pathlib import Path
import subprocess
import tempfile

from _paths import SRC


HARNESS = r'''
#include "persistence/latency_trace.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static constexpr uint64_t CONCURRENT_RECORDS = 50000;
static bool producer_done = false;
static bool nonblocking_producer_done = false;

static void *record_concurrently(void *)
{
	for (uint64_t index = 0; index < CONCURRENT_RECORDS; ++index)
		latency_trace_record("concurrent", index, index);
	__atomic_store_n(&producer_done, true, __ATOMIC_RELEASE);
	return nullptr;
}

static void *record_nonblocking_concurrently(void *)
{
	for (uint64_t index = 0; index < CONCURRENT_RECORDS; ++index)
		latency_trace_record_nonblocking("nonblocking-concurrent", index, index);
	__atomic_store_n(&nonblocking_producer_done, true, __ATOMIC_RELEASE);
	return nullptr;
}

static uint64_t max_for(const latency_trace_snapshot *snapshot, const char *name)
{
	for (int index = 0; index < snapshot->section_count; ++index)
		if (!strcmp(snapshot->sections[index].name, name))
			return snapshot->sections[index].max_us;
	return UINT64_MAX;
}

static uint64_t count_for(const latency_trace_snapshot *snapshot, const char *name)
{
	for (int index = 0; index < snapshot->section_count; ++index)
		if (!strcmp(snapshot->sections[index].name, name))
			return snapshot->sections[index].count;
	return 0;
}

static char *dump_to_string(const latency_trace_snapshot *snapshot)
{
	char *text = nullptr;
	size_t length = 0;
	FILE *stream = open_memstream(&text, &length);
	assert(stream);
	latency_trace_snapshot_dump(stream, snapshot);
	assert(!fclose(stream));
	assert(text && length);
	return text;
}

int main(int argc, char **argv)
{
	latency_trace_init();
	if (argc == 2 && !strcmp(argv[1], "--boot"))
	{
		puts(latency_trace_boot_id());
		return 0;
	}
	assert(latency_trace_elapsed_us(100, 149) == 49);
	assert(latency_trace_elapsed_us(100, 99) == LATENCY_TRACE_DURATION_INVALID);
	assert(latency_trace_elapsed_us(0, 149) == LATENCY_TRACE_DURATION_INVALID);
	char tick_buffer[LATENCY_TRACE_TICK_STRING_LENGTH];
	assert(!strcmp(latency_trace_format_tick(LATENCY_TRACE_TICK_UNAVAILABLE, tick_buffer), "-"));
 assert(!strcmp(latency_trace_format_duration(LATENCY_TRACE_DURATION_INVALID, tick_buffer), "-"));
 assert(!strcmp(latency_trace_format_duration(0, tick_buffer), "0"));
	assert(!strcmp(latency_trace_format_tick(UINT64_C(1) << 40, tick_buffer),
		       "1099511627776"));

	latency_trace_reset();
	latency_trace_record("spike", 900, 7);
	latency_trace_snapshot first = {};
	latency_trace_snapshot_take_and_reset(&first);
	assert(first.sample_count == 1);
	assert(first.dropped_section_samples == 0);
	assert(first.dropped_contended_samples == 0);
	assert(max_for(&first, "spike") == 900);

	latency_trace_reset();
	assert(latency_trace_record_nonblocking("nonblocking", 25, 8));
	latency_trace_snapshot nonblocking = {};
	latency_trace_snapshot_take_and_reset(&nonblocking);
	assert(nonblocking.sample_count == 1);
	assert(nonblocking.dropped_contended_samples == 0);
	assert(max_for(&nonblocking, "nonblocking") == 25);

	latency_trace_record("spike", 100, UINT64_C(1) << 40);
	latency_trace_record("worker", 50, LATENCY_TRACE_TICK_UNAVAILABLE);
	latency_trace_snapshot second = {};
	latency_trace_snapshot_take_and_reset(&second);
	assert(second.sample_count == 2);
	assert(max_for(&second, "spike") == 100);
	assert(second.top[0].tick == (UINT64_C(1) << 40));

	char *first_dump = dump_to_string(&second);
	char *second_dump = dump_to_string(&second);
	assert(!strcmp(first_dump, second_dump));
	assert(strstr(first_dump, "1099511627776"));
	assert(strstr(first_dump, "worker"));
	assert(strstr(first_dump, "                    -"));
	assert(strstr(first_dump, "window_start_utc_us="));
	assert(strstr(first_dump, "window_start_mono_us="));
	free(first_dump);
	free(second_dump);

	latency_trace_reset();
	char duplicate_a[] = "duplicate";
	char duplicate_b[] = "duplicate";
	latency_trace_record(duplicate_a, 10, 1);
	latency_trace_record(duplicate_b, 20, 2);
	memset(duplicate_a, 'x', sizeof duplicate_a);
 memset(duplicate_b, 'y', sizeof duplicate_b);
 latency_trace_snapshot duplicate = {};
	latency_trace_snapshot_take_and_reset(&duplicate);
	assert(duplicate.section_count == 1);
	assert(count_for(&duplicate, "duplicate") == 2);

	static constexpr uint64_t OVERFLOW_RECORDS = 5000;
	latency_trace_reset();
	for (uint64_t index = 0; index < OVERFLOW_RECORDS; ++index)
		latency_trace_record("overflow", index, index);
	latency_trace_snapshot overflow = {};
	latency_trace_snapshot_take_and_reset(&overflow);
	assert(overflow.sample_count == OVERFLOW_RECORDS);
	assert(overflow.top_count == LATENCY_TRACE_TOP_COUNT);
	assert(overflow.top[0].duration_us == OVERFLOW_RECORDS - 1);

	latency_trace_reset();
	char section_names[LATENCY_MAX_SECTIONS + 1][32];
	for (int index = 0; index < LATENCY_MAX_SECTIONS + 1; ++index)
	{
		snprintf(section_names[index], sizeof section_names[index], "section-%d", index);
		latency_trace_record(section_names[index], (uint64_t)index, (uint64_t)index);
	}
	latency_trace_record(section_names[LATENCY_MAX_SECTIONS], 100, 100);
	latency_trace_snapshot saturated = {};
	latency_trace_snapshot_take_and_reset(&saturated);
	assert(saturated.section_count == LATENCY_MAX_SECTIONS);
	assert(saturated.sample_count == LATENCY_MAX_SECTIONS + 2);
	assert(saturated.dropped_section_samples == 2);
 for (int i = 0; i < saturated.top_count; ++i)
  assert(count_for(&saturated, saturated.top[i].name) > 0);
	char *saturated_dump = dump_to_string(&saturated);
	assert(strstr(saturated_dump, "dropped_section_samples=2"));
	free(saturated_dump);

	latency_trace_snapshot empty = {};
	latency_trace_snapshot_take_and_reset(&empty);
	assert(empty.sample_count == 0);
	assert(empty.section_count == 0);
	assert(empty.top_count == 0);
	assert(empty.dropped_section_samples == 0);
	assert(empty.dropped_contended_samples == 0);

	latency_trace_reset();
	producer_done = false;
	pthread_t producer;
	assert(!pthread_create(&producer, nullptr, record_concurrently, nullptr));
	uint64_t captured = 0;
	while (!__atomic_load_n(&producer_done, __ATOMIC_ACQUIRE))
	{
		latency_trace_snapshot concurrent = {};
		latency_trace_snapshot_take_and_reset(&concurrent);
		captured += concurrent.sample_count;
	}
	assert(!pthread_join(producer, nullptr));
	latency_trace_snapshot remainder = {};
	latency_trace_snapshot_take_and_reset(&remainder);
	captured += remainder.sample_count;
	assert(captured == CONCURRENT_RECORDS);

	latency_trace_reset();
	nonblocking_producer_done = false;
	assert(!pthread_create(&producer, nullptr, record_nonblocking_concurrently, nullptr));
	uint64_t accounted = 0;
	while (!__atomic_load_n(&nonblocking_producer_done, __ATOMIC_ACQUIRE))
	{
		latency_trace_snapshot concurrent = {};
		latency_trace_snapshot_take_and_reset(&concurrent);
		accounted += concurrent.sample_count + concurrent.dropped_contended_samples;
	}
	assert(!pthread_join(producer, nullptr));
	latency_trace_snapshot nonblocking_remainder = {};
	latency_trace_snapshot_take_and_reset(&nonblocking_remainder);
	accounted += nonblocking_remainder.sample_count +
		     nonblocking_remainder.dropped_contended_samples;
	assert(accounted == CONCURRENT_RECORDS);

	latency_trace_begin_pulse(UINT64_C(1) << 42, 123456789);
	assert(latency_trace_current_tick() == (UINT64_C(1) << 42));
	assert(latency_trace_pulse_start_monotonic_us() == 123456789);

	FILE *missing = fopen("/definitely/missing/duris/latency.log", "a");
	assert(!missing);
	char *fallback = dump_to_string(&empty);
	assert(strstr(fallback, "samples=0"));
	free(fallback);


 latency_trace_reset();
 char *temporary_name = strdup("owned-name");
 latency_trace_record(temporary_name, 15, 1);
 free(temporary_name);
 latency_trace_record("owned-name", latency_trace_elapsed_us(0, 100), 2);
 latency_trace_record("owned-name", latency_trace_elapsed_us(100, 0), 3);
 latency_trace_record_nonblocking("owned-name", latency_trace_elapsed_us(100, 99), 4);
 latency_trace_record("owned-name", 0, 5); // Valid sub-microsecond sample survives.
 latency_trace_snapshot owned = {};
 latency_trace_snapshot_take_and_reset(&owned);
 assert(owned.sample_count == 2 && owned.invalid_clock_samples == 3);
 assert(count_for(&owned, "owned-name") == 2);
 latency_trace_record("replacement", 999, 6);
 assert(!strcmp(owned.top[0].name, "owned-name"));
 char *owned_dump = dump_to_string(&owned);
 assert(strstr(owned_dump, "invalid_clock_samples=3"));
 free(owned_dump);
 latency_trace_snapshot next = {};
 latency_trace_snapshot_take_and_reset(&next);
 assert(next.invalid_clock_samples == 0);
 char overlong[LATENCY_TRACE_NAME_LENGTH + 1];
 memset(overlong, 'x', sizeof overlong); overlong[sizeof overlong - 1] = 0;
 latency_trace_record(overlong, 10, 1);
 latency_trace_snapshot rejected = {};
 latency_trace_snapshot_take_and_reset(&rejected);
 assert(rejected.dropped_section_samples == 1 && rejected.top_count == 0);
 puts("latency trace runtime checks passed");
	return 0;
}
'''

header = (SRC / "latency_trace.h").read_text()
assert "_lt_end.tv_nsec - _lt_start.tv_nsec" not in header
assert "#define LATENCY_TRACE(" not in header
assert "extern unsigned long long ne_event_tick" not in header


with tempfile.TemporaryDirectory(prefix="duris-latency-trace-") as directory:
    temporary = Path(directory)
    harness = temporary / "latency_trace_test.c"
    binary = temporary / "latency_trace_test"
    harness.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-g",
            "-O1",
            "-D_GNU_SOURCE",
            "-fsanitize=address,undefined",
            "-I",
            str(SRC),
            str(harness),
            str(SRC / "persistence" / "latency_trace.c"),
            "-pthread",
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)
    first_boot = subprocess.run(
        [str(binary), "--boot"], check=True, capture_output=True, text=True
    ).stdout.strip()
    second_boot = subprocess.run(
        [str(binary), "--boot"], check=True, capture_output=True, text=True
    ).stdout.strip()
    assert first_boot
    assert second_boot
    assert first_boot != second_boot

print("latency trace process identity checks passed")
