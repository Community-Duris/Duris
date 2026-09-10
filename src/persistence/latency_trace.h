/* ===================================================================
 * latency_trace.h - Bounded, process-global latency tracing.
 * =================================================================== */

#ifndef __LATENCY_TRACE_H__
#define __LATENCY_TRACE_H__

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef LATENCY_TRACE_ENABLED
#define LATENCY_TRACE_ENABLED 1
#endif

#define LATENCY_TRACE_NAME_LENGTH 64
/* A zero clock endpoint means failure; elapsed intervals propagate this sentinel. */
#define LATENCY_TRACE_DURATION_INVALID UINT64_MAX

#define LATENCY_TRACE_TOP_COUNT 10
#define LATENCY_TRACE_BOOT_ID_LENGTH 64
#define LATENCY_TRACE_TICK_UNAVAILABLE UINT64_MAX
#define LATENCY_TRACE_TICK_STRING_LENGTH 21

typedef struct
{
	char name[LATENCY_TRACE_NAME_LENGTH];
	uint64_t duration_us;
	uint64_t tick;
} latency_entry;

typedef struct
{
	char name[LATENCY_TRACE_NAME_LENGTH];
	uint64_t min_us;
	uint64_t max_us;
	uint64_t total_us;
	uint64_t count;
} latency_section;

#define LATENCY_MAX_SECTIONS 32

typedef struct
{
	latency_section sections[LATENCY_MAX_SECTIONS];
	int section_count;
	latency_entry top[LATENCY_TRACE_TOP_COUNT];
	int top_count;
	uint64_t sample_count;
	uint64_t dropped_section_samples;
	uint64_t invalid_clock_samples;
	uint64_t dropped_contended_samples;
	uint64_t window_start_utc_us;
	uint64_t window_end_utc_us;
	uint64_t window_start_mono_us;
	uint64_t window_end_mono_us;
	char boot_id[LATENCY_TRACE_BOOT_ID_LENGTH];
} latency_trace_snapshot;

void latency_trace_init(void);
void latency_trace_record(const char *name, uint64_t duration_us, uint64_t tick);
bool latency_trace_record_nonblocking(const char *name, uint64_t duration_us, uint64_t tick);
void latency_trace_reset(void);
void latency_trace_snapshot_take_and_reset(latency_trace_snapshot *snapshot);
void latency_trace_snapshot_dump(FILE *output, const latency_trace_snapshot *snapshot);
uint64_t latency_trace_monotonic_us(void);
uint64_t latency_trace_elapsed_us(uint64_t started_us, uint64_t finished_us);
/* Returned storage is initialized once and immutable for process lifetime. */
const char *latency_trace_boot_id(void);
void latency_trace_begin_pulse(uint64_t tick, uint64_t monotonic_us);
uint64_t latency_trace_current_tick(void);
uint64_t latency_trace_pulse_start_monotonic_us(void);

static inline const char *latency_trace_format_tick(uint64_t tick,
						    char buffer[LATENCY_TRACE_TICK_STRING_LENGTH])
{
	if (tick == LATENCY_TRACE_TICK_UNAVAILABLE || !buffer)
		return "-";
	snprintf(buffer, LATENCY_TRACE_TICK_STRING_LENGTH, "%" PRIu64, tick);
	return buffer;
}

/* Both unavailable correlation ticks and invalid durations render as '-'. */
static inline const char *
latency_trace_format_duration(uint64_t duration_us, char buffer[LATENCY_TRACE_TICK_STRING_LENGTH])
{
	return latency_trace_format_tick(duration_us, buffer);
}

#endif /* __LATENCY_TRACE_H__ */
