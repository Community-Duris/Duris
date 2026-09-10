#ifndef __COMMAND_LATENCY_H__
#define __COMMAND_LATENCY_H__

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#define COMMAND_LATENCY_SLOW_US 50000ULL
#define COMMAND_LATENCY_MAX_REPORTS 8
#define COMMAND_LATENCY_REPORT_INTERVAL_PULSES 4ULL
#define COMMAND_LATENCY_REPORT_BUFFER_SIZE 16384
#define COMMAND_LATENCY_LOG_PREFIX_SIZE 40
#define COMMAND_LATENCY_PLAYER_NAME_LENGTH 64
#define COMMAND_LATENCY_OPERATION_LENGTH 32

typedef enum
{
	COMMAND_LATENCY_PLAYING = 0,
	COMMAND_LATENCY_NANNY,
	COMMAND_LATENCY_PAGER,
	COMMAND_LATENCY_EDITOR,
	COMMAND_LATENCY_SSL,
	COMMAND_LATENCY_DESCRIPTOR,
	COMMAND_LATENCY_KIND_COUNT
} command_latency_kind;

typedef struct
{
	uint64_t count;
	uint64_t total_us;
	uint64_t max_us;
} command_latency_stats;

typedef struct
{
	command_latency_kind kind;
	int connection_state;
	long player_id;
	char player_name[COMMAND_LATENCY_PLAYER_NAME_LENGTH];
	char operation[COMMAND_LATENCY_OPERATION_LENGTH];
	uint64_t duration_us;
} command_latency_event;

typedef struct
{
	command_latency_stats kinds[COMMAND_LATENCY_KIND_COUNT];
	command_latency_event slowest[COMMAND_LATENCY_MAX_REPORTS];
	int retained_slow_count;
	uint64_t slow_count;
	uint64_t measured_us;
} command_latency_tracker;

typedef void (*command_latency_emit_fn)(const char *line, void *context);

typedef struct
{
	char text[COMMAND_LATENCY_REPORT_BUFFER_SIZE];
	size_t length;
	char continuation_prefix[COMMAND_LATENCY_LOG_PREFIX_SIZE];
} command_latency_log_buffer;

typedef struct
{
	bool has_reported;
	uint64_t last_report_tick;
	uint64_t pulses_since_report;
	uint64_t suppressed_reports;
	uint64_t suppressed_slow_operations;
	bool has_suppressed_worst;
	command_latency_event suppressed_worst;
	uint64_t suppressed_worst_tick;
	uint64_t suppressed_worst_pulse_start_mono_us;
} command_latency_report_state;

/* Playing labels must come from the command table, never raw input. */
void command_latency_event_prepare(command_latency_event *event, command_latency_kind kind,
				   int connection_state, long player_id, const char *player_name,
				   const char *canonical_command);
void command_latency_record(command_latency_tracker *tracker, const command_latency_event *event,
			    uint64_t duration_us);
void command_latency_report(const command_latency_tracker *tracker, uint64_t sweep_us,
			    const char *boot_id, uint64_t tick, uint64_t pulse_start_mono_us,
			    command_latency_emit_fn emit, void *context);
void command_latency_report_throttled(command_latency_report_state *state,
				      const command_latency_tracker *tracker, uint64_t sweep_us,
				      const char *boot_id, uint64_t tick,
				      uint64_t pulse_start_mono_us, command_latency_emit_fn emit,
				      void *context);
void command_latency_log_buffer_reset(command_latency_log_buffer *report,
				      const char *continuation_prefix);
void command_latency_log_buffer_collect(const char *line, void *context);
const char *command_latency_kind_name(command_latency_kind kind);

#endif
