/* ===================================================================
 * latency_trace.h - Bounded, process-global latency tracing.
 * =================================================================== */

#ifndef __LATENCY_TRACE_H__
#define __LATENCY_TRACE_H__

#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifndef LATENCY_TRACE_MAX_SAMPLES
#define LATENCY_TRACE_MAX_SAMPLES 4096
#endif

#ifndef LATENCY_TRACE_ENABLED
#define LATENCY_TRACE_ENABLED 1
#endif

#define LATENCY_TRACE_TOP_COUNT 10
#define LATENCY_TRACE_BOOT_ID_LENGTH 64
#define LATENCY_TRACE_TICK_UNAVAILABLE UINT64_MAX

typedef struct
{
	const char *name; /* section name (pointer to string literal) */
	uint64_t duration_us;
	uint64_t tick;
} latency_entry;

typedef struct
{
	const char *name;
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
	uint64_t window_start_utc_us;
	uint64_t window_end_utc_us;
	uint64_t window_start_mono_us;
	uint64_t window_end_mono_us;
	char boot_id[LATENCY_TRACE_BOOT_ID_LENGTH];
} latency_trace_snapshot;

extern latency_entry _latency_buf[LATENCY_TRACE_MAX_SAMPLES];
extern int _latency_head;
extern int _latency_count;
extern pthread_mutex_t _latency_mutex;
extern latency_section _latency_sections[LATENCY_MAX_SECTIONS];
extern int _latency_nsections;

void latency_trace_init(void);
void latency_trace_record(const char *name, uint64_t duration_us, uint64_t tick);
void latency_trace_reset(void);
void latency_trace_snapshot_capture(latency_trace_snapshot *snapshot);
void latency_trace_snapshot_dump(FILE *output, const latency_trace_snapshot *snapshot);
uint64_t latency_trace_monotonic_us(void);
const char *latency_trace_boot_id(void);
void latency_trace_begin_pulse(uint64_t tick, uint64_t monotonic_us);
uint64_t latency_trace_current_tick(void);
uint64_t latency_trace_pulse_start_monotonic_us(void);

/*
 * Convenience form for game-thread scopes. Call sites outside the game loop
 * should call latency_trace_record with LATENCY_TRACE_TICK_UNAVAILABLE instead
 * of borrowing mutable loop state from another thread.
 */
#define LATENCY_TRACE(name)                                                                      \
	for (struct timespec _lt_start, _lt_end = { 0 };                                         \
	     !_lt_end.tv_sec && (clock_gettime(CLOCK_MONOTONIC, &_lt_start), 1);                 \
	     clock_gettime(CLOCK_MONOTONIC, &_lt_end), ({                                        \
		     uint64_t _us = (uint64_t)(_lt_end.tv_sec - _lt_start.tv_sec) * 1000000ULL + \
				    (uint64_t)(_lt_end.tv_nsec - _lt_start.tv_nsec) / 1000ULL;   \
		     extern unsigned long long ne_event_tick;                                    \
		     latency_trace_record(name, _us, (uint64_t)ne_event_tick);                   \
	     }))

#endif /* __LATENCY_TRACE_H__ */
