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

static void *record_concurrently(void *)
{
	for (uint64_t index = 0; index < CONCURRENT_RECORDS; ++index)
		latency_trace_record("concurrent", index, index);
	__atomic_store_n(&producer_done, true, __ATOMIC_RELEASE);
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
	assert(latency_trace_elapsed_us(100, 99) == 0);
	assert(latency_trace_elapsed_us(0, 149) == 0);
	char tick_buffer[LATENCY_TRACE_TICK_STRING_LENGTH];
	assert(!strcmp(latency_trace_format_tick(LATENCY_TRACE_TICK_UNAVAILABLE, tick_buffer), "-"));
	assert(!strcmp(latency_trace_format_tick(UINT64_C(1) << 40, tick_buffer),
		       "1099511627776"));

	latency_trace_reset();
	latency_trace_record("spike", 900, 7);
	latency_trace_snapshot first = {};
	latency_trace_snapshot_capture(&first);
	assert(first.sample_count == 1);
	assert(first.dropped_section_samples == 0);
	assert(max_for(&first, "spike") == 900);

	latency_trace_record("spike", 100, UINT64_C(1) << 40);
	latency_trace_record("worker", 50, LATENCY_TRACE_TICK_UNAVAILABLE);
	latency_trace_snapshot second = {};
	latency_trace_snapshot_capture(&second);
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
	latency_trace_snapshot duplicate = {};
	latency_trace_snapshot_capture(&duplicate);
	assert(duplicate.section_count == 1);
	assert(count_for(&duplicate, "duplicate") == 2);

	static constexpr uint64_t OVERFLOW_RECORDS = 5000;
	latency_trace_reset();
	for (uint64_t index = 0; index < OVERFLOW_RECORDS; ++index)
		latency_trace_record("overflow", index, index);
	latency_trace_snapshot overflow = {};
	latency_trace_snapshot_capture(&overflow);
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
	latency_trace_snapshot_capture(&saturated);
	assert(saturated.section_count == LATENCY_MAX_SECTIONS);
	assert(saturated.sample_count == LATENCY_MAX_SECTIONS + 2);
	assert(saturated.dropped_section_samples == 2);
	char *saturated_dump = dump_to_string(&saturated);
	assert(strstr(saturated_dump, "dropped_section_samples=2"));
	free(saturated_dump);

	latency_trace_snapshot empty = {};
	latency_trace_snapshot_capture(&empty);
	assert(empty.sample_count == 0);
	assert(empty.section_count == 0);
	assert(empty.top_count == 0);
	assert(empty.dropped_section_samples == 0);

	latency_trace_reset();
	producer_done = false;
	pthread_t producer;
	assert(!pthread_create(&producer, nullptr, record_concurrently, nullptr));
	uint64_t captured = 0;
	while (!__atomic_load_n(&producer_done, __ATOMIC_ACQUIRE))
	{
		latency_trace_snapshot concurrent = {};
		latency_trace_snapshot_capture(&concurrent);
		captured += concurrent.sample_count;
	}
	assert(!pthread_join(producer, nullptr));
	latency_trace_snapshot remainder = {};
	latency_trace_snapshot_capture(&remainder);
	captured += remainder.sample_count;
	assert(captured == CONCURRENT_RECORDS);

	latency_trace_begin_pulse(UINT64_C(1) << 42, 123456789);
	assert(latency_trace_current_tick() == (UINT64_C(1) << 42));
	assert(latency_trace_pulse_start_monotonic_us() == 123456789);
	latency_trace_reset();
	LATENCY_TRACE("macro-scope")
	{
		uint64_t value = 0;
		value++;
		assert(value == 1);
	}
	latency_trace_snapshot macro = {};
	latency_trace_snapshot_capture(&macro);
	assert(macro.sample_count == 1);
	assert(macro.top[0].tick == (UINT64_C(1) << 42));

	FILE *missing = fopen("/definitely/missing/duris/latency.log", "a");
	assert(!missing);
	char *fallback = dump_to_string(&empty);
	assert(strstr(fallback, "samples=0"));
	free(fallback);

	puts("latency trace runtime checks passed");
	return 0;
}
'''

header = (SRC / "latency_trace.h").read_text()
assert "_lt_end.tv_nsec - _lt_start.tv_nsec" not in header
assert "latency_trace_current_tick()" in header
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
