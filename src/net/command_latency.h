#ifndef __COMMAND_LATENCY_H__
#define __COMMAND_LATENCY_H__

#include <inttypes.h>

#define COMMAND_LATENCY_SLOW_US 50000ULL
#define COMMAND_LATENCY_MAX_REPORTS 8
#define COMMAND_LATENCY_PLAYER_NAME_LENGTH 64
#define COMMAND_LATENCY_OPERATION_LENGTH 32

typedef enum
{
	COMMAND_LATENCY_PLAYING = 0,
	COMMAND_LATENCY_NANNY,
	COMMAND_LATENCY_PAGER,
	COMMAND_LATENCY_EDITOR,
	COMMAND_LATENCY_SSL,
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

void command_latency_event_prepare(command_latency_event *event, command_latency_kind kind,
				   int connection_state, long player_id, const char *player_name,
				   const char *playing_input);
void command_latency_record(command_latency_tracker *tracker, const command_latency_event *event,
			    uint64_t duration_us);
void command_latency_report(const command_latency_tracker *tracker, uint64_t sweep_us,
			    const char *boot_id, uint64_t tick, uint64_t pulse_start_mono_us,
			    command_latency_emit_fn emit, void *context);
uint64_t command_latency_elapsed_us(uint64_t started_us, uint64_t finished_us);
const char *command_latency_kind_name(command_latency_kind kind);

#endif
