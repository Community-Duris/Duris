#include "net/command_latency.h"
#include "persistence/latency_trace.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void command_latency_sanitize(char *destination, size_t destination_size, const char *source,
				     bool first_word_only)
{
	if (!destination || !destination_size)
		return;
	size_t written = 0;
	if (source)
	{
		while (*source && isspace((unsigned char)*source))
			++source;
		while (*source && written + 1 < destination_size)
		{
			const unsigned char character = (unsigned char)*source++;
			if (first_word_only && isspace(character))
				break;
			if (isalnum(character) || character == '_' || character == '-' ||
			    character == '\'' || character == '?')
				destination[written++] = (char)character;
			else
				destination[written++] = '?';
		}
	}
	if (!written)
	{
		const char *fallback = "-";
		while (*fallback && written + 1 < destination_size)
			destination[written++] = *fallback++;
	}
	destination[written] = '\0';
}

const char *command_latency_kind_name(command_latency_kind kind)
{
	switch (kind)
	{
	case COMMAND_LATENCY_PLAYING:
		return "playing";
	case COMMAND_LATENCY_NANNY:
		return "nanny";
	case COMMAND_LATENCY_PAGER:
		return "pager";
	case COMMAND_LATENCY_EDITOR:
		return "editor";
	case COMMAND_LATENCY_SSL:
		return "ssl";
	default:
		return "unknown";
	}
}

uint64_t command_latency_elapsed_us(uint64_t started_us, uint64_t finished_us)
{
	return started_us && finished_us >= started_us ? finished_us - started_us : 0;
}

void command_latency_event_prepare(command_latency_event *event, command_latency_kind kind,
				   int connection_state, long player_id, const char *player_name,
				   const char *playing_input)
{
	if (!event)
		return;
	memset(event, 0, sizeof *event);
	event->kind = kind;
	event->connection_state = connection_state;
	event->player_id = player_id;
	command_latency_sanitize(event->player_name, sizeof event->player_name, player_name, false);
	if (kind == COMMAND_LATENCY_PLAYING)
		command_latency_sanitize(event->operation, sizeof event->operation, playing_input,
					 true);
	else
		snprintf(event->operation, sizeof event->operation, "%s",
			 command_latency_kind_name(kind));
}

static void command_latency_retain_slow(command_latency_tracker *tracker,
					const command_latency_event *event)
{
	int index = tracker->retained_slow_count;
	if (index < COMMAND_LATENCY_MAX_REPORTS)
		tracker->retained_slow_count++;
	else
	{
		index = COMMAND_LATENCY_MAX_REPORTS - 1;
		if (event->duration_us <= tracker->slowest[index].duration_us)
			return;
	}
	tracker->slowest[index] = *event;
	while (index > 0 &&
	       tracker->slowest[index].duration_us > tracker->slowest[index - 1].duration_us)
	{
		command_latency_event swap = tracker->slowest[index - 1];
		tracker->slowest[index - 1] = tracker->slowest[index];
		tracker->slowest[index] = swap;
		--index;
	}
}

void command_latency_record(command_latency_tracker *tracker, const command_latency_event *event,
			    uint64_t duration_us)
{
	if (!tracker || !event || event->kind < 0 || event->kind >= COMMAND_LATENCY_KIND_COUNT)
		return;
	command_latency_event completed = *event;
	completed.duration_us = duration_us;
	command_latency_stats *stats = &tracker->kinds[event->kind];
	stats->count++;
	stats->total_us += duration_us;
	if (duration_us > stats->max_us)
		stats->max_us = duration_us;
	tracker->measured_us += duration_us;
	if (duration_us >= COMMAND_LATENCY_SLOW_US)
	{
		tracker->slow_count++;
		command_latency_retain_slow(tracker, &completed);
	}
}

static void command_latency_emit(const char *line, command_latency_emit_fn emit, void *context)
{
	if (emit)
		emit(line, context);
}

void command_latency_report(const command_latency_tracker *tracker, uint64_t sweep_us,
			    const char *boot_id, uint64_t tick, uint64_t pulse_start_mono_us,
			    command_latency_emit_fn emit, void *context)
{
	if (!tracker || !emit || (sweep_us < COMMAND_LATENCY_SLOW_US && tracker->slow_count == 0))
		return;
	char line[1024];
	char tick_buffer[LATENCY_TRACE_TICK_STRING_LENGTH];
	const char *safe_boot_id = boot_id && *boot_id ? boot_id : "-";
	const char *formatted_tick = latency_trace_format_tick(tick, tick_buffer);
	for (int index = 0; index < tracker->retained_slow_count; ++index)
	{
		const command_latency_event *event = &tracker->slowest[index];
		snprintf(
			line, sizeof line,
			"COMMAND OP SLOW: boot=%s tick=%s"
			" pulse_start_mono_us=%" PRIu64
			" kind=%s state=%d player_id=%ld player=%s operation=%s duration_us=%" PRIu64,
			safe_boot_id, formatted_tick, pulse_start_mono_us,
			command_latency_kind_name(event->kind), event->connection_state,
			event->player_id, event->player_name, event->operation, event->duration_us);
		command_latency_emit(line, emit, context);
	}

	const uint64_t residual_us =
		sweep_us >= tracker->measured_us ? sweep_us - tracker->measured_us : 0;
	const uint64_t suppressed = tracker->slow_count - (uint64_t)tracker->retained_slow_count;
	snprintf(line, sizeof line,
		 "%s: boot=%s tick=%s pulse_start_mono_us=%" PRIu64 " total_us=%" PRIu64
		 " measured_operation_us=%" PRIu64 " maintenance_residual_us=%" PRIu64
		 " slow=%" PRIu64 " reported=%d"
		 " suppressed=%" PRIu64,
		 sweep_us >= COMMAND_LATENCY_SLOW_US ? "COMMAND SWEEP SLOW" :
						       "COMMAND SLOW SUMMARY",
		 safe_boot_id, formatted_tick, pulse_start_mono_us, sweep_us, tracker->measured_us,
		 residual_us, tracker->slow_count, tracker->retained_slow_count, suppressed);
	command_latency_emit(line, emit, context);

	for (int kind = 0; kind < COMMAND_LATENCY_KIND_COUNT; ++kind)
	{
		const command_latency_stats *stats = &tracker->kinds[kind];
		if (!stats->count)
			continue;
		snprintf(line, sizeof line,
			 "COMMAND SWEEP KIND: boot=%s tick=%s pulse_start_mono_us=%" PRIu64
			 " kind=%s count=%" PRIu64 " total_us=%" PRIu64 " max_us=%" PRIu64,
			 safe_boot_id, formatted_tick, pulse_start_mono_us,
			 command_latency_kind_name((command_latency_kind)kind), stats->count,
			 stats->total_us, stats->max_us);
		command_latency_emit(line, emit, context);
	}
}
