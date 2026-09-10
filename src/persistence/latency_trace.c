#include "persistence/latency_trace.h"
#include "core/clock_utils.h"

#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

static pthread_mutex_t latency_mutex = PTHREAD_MUTEX_INITIALIZER;
static latency_section latency_sections[LATENCY_MAX_SECTIONS];
static int latency_section_count = 0;

static latency_entry latency_window_top[LATENCY_TRACE_TOP_COUNT];
static int latency_window_top_count = 0;
static uint64_t latency_window_sample_count = 0;
static uint64_t latency_window_dropped_section_samples = 0;
static uint64_t latency_window_invalid_clock_samples = 0;
static uint64_t latency_window_dropped_contended_samples = 0;
static uint64_t latency_window_start_utc_us = 0;
static uint64_t latency_window_start_mono_us = 0;
static char latency_boot_id[LATENCY_TRACE_BOOT_ID_LENGTH] = "uninitialized";
static bool latency_initialized = false;
static uint64_t latency_current_tick = LATENCY_TRACE_TICK_UNAVAILABLE;
static uint64_t latency_pulse_start_mono_us = 0;

static uint64_t latency_clock_us(clockid_t clock_id)
{
	uint64_t result = 0;

	if (!clock_read_microseconds(clock_id, &result, clock_gettime))
		return 0;
	return result;
}

uint64_t latency_trace_monotonic_us(void)
{
	return latency_clock_us(CLOCK_MONOTONIC);
}

uint64_t latency_trace_elapsed_us(uint64_t started_us, uint64_t finished_us)
{
	return started_us && finished_us >= started_us ? finished_us - started_us :
							 LATENCY_TRACE_DURATION_INVALID;
}

static void latency_trace_initialize_locked(void)
{
	if (latency_initialized)
		return;

	const uint64_t utc_us = latency_clock_us(CLOCK_REALTIME);
	latency_window_start_utc_us = utc_us;
	latency_window_start_mono_us = latency_clock_us(CLOCK_MONOTONIC);
	if (utc_us)
		snprintf(latency_boot_id, sizeof latency_boot_id, "%" PRIu64 "-%ld", utc_us,
			 (long)getpid());
	else
		snprintf(latency_boot_id, sizeof latency_boot_id, "unknown-%ld", (long)getpid());
	latency_initialized = true;
}

void latency_trace_init(void)
{
	pthread_mutex_lock(&latency_mutex);
	latency_trace_initialize_locked();
	pthread_mutex_unlock(&latency_mutex);
}

const char *latency_trace_boot_id(void)
{
	latency_trace_init();
	return latency_boot_id;
}

void latency_trace_begin_pulse(uint64_t tick, uint64_t monotonic_us)
{
	pthread_mutex_lock(&latency_mutex);
	latency_current_tick = tick;
	latency_pulse_start_mono_us = monotonic_us;
	pthread_mutex_unlock(&latency_mutex);
}

uint64_t latency_trace_current_tick(void)
{
	pthread_mutex_lock(&latency_mutex);
	const uint64_t tick = latency_current_tick;
	pthread_mutex_unlock(&latency_mutex);
	return tick;
}

uint64_t latency_trace_pulse_start_monotonic_us(void)
{
	pthread_mutex_lock(&latency_mutex);
	const uint64_t monotonic_us = latency_pulse_start_mono_us;
	pthread_mutex_unlock(&latency_mutex);
	return monotonic_us;
}

static int latency_find_section(const char *name)
{
	for (int index = 0; index < latency_section_count; ++index)
		if (!strcmp(latency_sections[index].name, name))
			return index;
	return -1;
}

static bool latency_update_section(const char *name, uint64_t duration_us)
{
	int index = latency_find_section(name);
	if (index < 0 && latency_section_count < LATENCY_MAX_SECTIONS)
	{
		index = latency_section_count++;
		latency_sections[index] = { {}, duration_us, duration_us, duration_us, 1 };
		snprintf(latency_sections[index].name, sizeof latency_sections[index].name, "%s",
			 name);
		return true;
	}
	if (index < 0)
	{
		latency_window_dropped_section_samples++;
		return false;
	}

	latency_section *section = &latency_sections[index];
	if (duration_us < section->min_us)
		section->min_us = duration_us;
	if (duration_us > section->max_us)
		section->max_us = duration_us;
	section->total_us += duration_us;
	section->count++;
	return true;
}

static void latency_update_top(const char *name, uint64_t duration_us, uint64_t tick)
{
	int index = latency_window_top_count;
	if (index < LATENCY_TRACE_TOP_COUNT)
		latency_window_top_count++;
	else
	{
		index = LATENCY_TRACE_TOP_COUNT - 1;
		if (duration_us <= latency_window_top[index].duration_us)
			return;
	}

	latency_window_top[index] = { {}, duration_us, tick };
	snprintf(latency_window_top[index].name, sizeof latency_window_top[index].name, "%s", name);
	while (index > 0 &&
	       latency_window_top[index].duration_us > latency_window_top[index - 1].duration_us)
	{
		latency_entry swap = latency_window_top[index - 1];
		latency_window_top[index - 1] = latency_window_top[index];
		latency_window_top[index] = swap;
		--index;
	}
}

static void latency_trace_record_locked(const char *name, uint64_t duration_us, uint64_t tick)
{
	latency_trace_initialize_locked();
	if (duration_us == LATENCY_TRACE_DURATION_INVALID)
	{
		latency_window_invalid_clock_samples++;
		return;
	}
	latency_window_sample_count++;
	if (strnlen(name, LATENCY_TRACE_NAME_LENGTH) >= LATENCY_TRACE_NAME_LENGTH)
	{
		latency_window_dropped_section_samples++;
		return;
	}
	if (latency_update_section(name, duration_us))
		latency_update_top(name, duration_us, tick);
}

void latency_trace_record(const char *name, uint64_t duration_us, uint64_t tick)
{
#if LATENCY_TRACE_ENABLED
	if (!name)
		return;
	pthread_mutex_lock(&latency_mutex);
	latency_trace_record_locked(name, duration_us, tick);
	pthread_mutex_unlock(&latency_mutex);
#else
	(void)name;
	(void)duration_us;
	(void)tick;
#endif
}

bool latency_trace_record_nonblocking(const char *name, uint64_t duration_us, uint64_t tick)
{
#if LATENCY_TRACE_ENABLED
	if (!name)
		return false;
	if (pthread_mutex_trylock(&latency_mutex) != 0)
	{
		__atomic_fetch_add(&latency_window_dropped_contended_samples, 1, __ATOMIC_RELAXED);
		return false;
	}
	latency_trace_record_locked(name, duration_us, tick);
	pthread_mutex_unlock(&latency_mutex);
	return true;
#else
	(void)name;
	(void)duration_us;
	(void)tick;
	return false;
#endif
}

static void latency_reset_locked(uint64_t utc_us, uint64_t mono_us)
{
	latency_section_count = 0;
	latency_window_top_count = 0;
	latency_window_sample_count = 0;
	latency_window_dropped_section_samples = 0;
	latency_window_invalid_clock_samples = 0;
	latency_window_start_utc_us = utc_us;
	latency_window_start_mono_us = mono_us;
}

void latency_trace_reset(void)
{
#if LATENCY_TRACE_ENABLED
	pthread_mutex_lock(&latency_mutex);
	latency_trace_initialize_locked();
	const uint64_t utc_us = latency_clock_us(CLOCK_REALTIME);
	const uint64_t mono_us = latency_clock_us(CLOCK_MONOTONIC);
	latency_reset_locked(utc_us, mono_us);
	__atomic_exchange_n(&latency_window_dropped_contended_samples, 0, __ATOMIC_RELAXED);
	pthread_mutex_unlock(&latency_mutex);
#endif
}

void latency_trace_snapshot_take_and_reset(latency_trace_snapshot *snapshot)
{
#if LATENCY_TRACE_ENABLED
	if (!snapshot)
		return;
	pthread_mutex_lock(&latency_mutex);
	latency_trace_initialize_locked();
	const uint64_t utc_us = latency_clock_us(CLOCK_REALTIME);
	const uint64_t mono_us = latency_clock_us(CLOCK_MONOTONIC);
	memset(snapshot, 0, sizeof *snapshot);
	memcpy(snapshot->sections, latency_sections,
	       (size_t)latency_section_count * sizeof snapshot->sections[0]);
	snapshot->section_count = latency_section_count;
	memcpy(snapshot->top, latency_window_top,
	       (size_t)latency_window_top_count * sizeof snapshot->top[0]);
	snapshot->top_count = latency_window_top_count;
	snapshot->sample_count = latency_window_sample_count;
	snapshot->dropped_section_samples = latency_window_dropped_section_samples;
	snapshot->invalid_clock_samples = latency_window_invalid_clock_samples;
	snapshot->dropped_contended_samples =
		__atomic_exchange_n(&latency_window_dropped_contended_samples, 0, __ATOMIC_RELAXED);
	snapshot->window_start_utc_us = latency_window_start_utc_us;
	snapshot->window_end_utc_us = utc_us;
	snapshot->window_start_mono_us = latency_window_start_mono_us;
	snapshot->window_end_mono_us = mono_us;
	snprintf(snapshot->boot_id, sizeof snapshot->boot_id, "%s", latency_boot_id);
	latency_reset_locked(utc_us, mono_us);
	pthread_mutex_unlock(&latency_mutex);
#else
	if (snapshot)
		memset(snapshot, 0, sizeof *snapshot);
#endif
}

static void latency_sort_sections(latency_section *sections, int count)
{
	for (int index = 0; index < count; ++index)
		for (int candidate = index + 1; candidate < count; ++candidate)
			if (sections[candidate].total_us > sections[index].total_us)
			{
				latency_section swap = sections[index];
				sections[index] = sections[candidate];
				sections[candidate] = swap;
			}
}

void latency_trace_snapshot_dump(FILE *output, const latency_trace_snapshot *snapshot)
{
#if LATENCY_TRACE_ENABLED
	if (!output || !snapshot)
		return;
	latency_section sections[LATENCY_MAX_SECTIONS] = {};
	memcpy(sections, snapshot->sections, (size_t)snapshot->section_count * sizeof sections[0]);
	latency_sort_sections(sections, snapshot->section_count);

	fprintf(output, "\n===== LATENCY TRACE SUMMARY =====\n");
	fprintf(output,
		"boot=%s window_start_utc_us=%" PRIu64 " window_end_utc_us=%" PRIu64
		" window_start_mono_us=%" PRIu64 " window_end_mono_us=%" PRIu64 " samples=%" PRIu64
		" dropped_section_samples=%" PRIu64 " dropped_contended_samples=%" PRIu64
		" invalid_clock_samples=%" PRIu64 "\n",
		snapshot->boot_id, snapshot->window_start_utc_us, snapshot->window_end_utc_us,
		snapshot->window_start_mono_us, snapshot->window_end_mono_us,
		snapshot->sample_count, snapshot->dropped_section_samples,
		snapshot->dropped_contended_samples, snapshot->invalid_clock_samples);
	fprintf(output, "%-30s %12s %12s %12s %12s\n", "Section", "min(us)", "max(us)", "avg(us)",
		"samples");
	for (int index = 0; index < snapshot->section_count; ++index)
	{
		const latency_section *section = &sections[index];
		const uint64_t average = section->count ? section->total_us / section->count : 0;
		fprintf(output, "%-30s %12" PRIu64 " %12" PRIu64 " %12" PRIu64 " %12" PRIu64 "\n",
			section->name, section->min_us, section->max_us, average, section->count);
	}

	fprintf(output, "\n--- Top-10 worst individual samples ---\n");
	fprintf(output, "%-30s %12s %20s\n", "Section", "us", "tick");
	for (int index = 0; index < snapshot->top_count; ++index)
	{
		const latency_entry *entry = &snapshot->top[index];
		char tick_buffer[LATENCY_TRACE_TICK_STRING_LENGTH];
		fprintf(output, "%-30s %12" PRIu64 " %20s\n", entry->name, entry->duration_us,
			latency_trace_format_tick(entry->tick, tick_buffer));
	}
	fprintf(output, "===== END LATENCY TRACE =====\n\n");
#else
	(void)output;
	(void)snapshot;
#endif
}
