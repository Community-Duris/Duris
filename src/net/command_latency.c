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
	case COMMAND_LATENCY_DESCRIPTOR:
		return "descriptor";
	default:
		return "unknown";
	}
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
	if (!tracker || !event ||
	    (unsigned int)event->kind >= (unsigned int)COMMAND_LATENCY_KIND_COUNT)
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

	const uint64_t unattributed_us =
		sweep_us >= tracker->measured_us ? sweep_us - tracker->measured_us : 0;
	const uint64_t suppressed = tracker->slow_count - (uint64_t)tracker->retained_slow_count;
	snprintf(line, sizeof line,
		 "%s: boot=%s tick=%s pulse_start_mono_us=%" PRIu64 " total_us=%" PRIu64
		 " measured_operation_us=%" PRIu64 " unattributed_sweep_us=%" PRIu64
		 " slow=%" PRIu64 " reported=%d"
		 " suppressed=%" PRIu64,
		 sweep_us >= COMMAND_LATENCY_SLOW_US ? "COMMAND SWEEP SLOW" :
						       "COMMAND SLOW SUMMARY",
		 safe_boot_id, formatted_tick, pulse_start_mono_us, sweep_us, tracker->measured_us,
		 unattributed_us, tracker->slow_count, tracker->retained_slow_count, suppressed);
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

static uint64_t command_latency_saturating_add(uint64_t left, uint64_t right)
{
	return UINT64_MAX - left < right ? UINT64_MAX : left + right;
}

static bool command_latency_report_due(const command_latency_report_state *state, uint64_t tick)
{
	if (!state->has_reported || tick == LATENCY_TRACE_TICK_UNAVAILABLE ||
	    state->last_report_tick == LATENCY_TRACE_TICK_UNAVAILABLE ||
	    tick < state->last_report_tick)
		return true;
	return tick - state->last_report_tick >= COMMAND_LATENCY_REPORT_INTERVAL_PULSES;
}

static void command_latency_retain_suppressed(command_latency_report_state *state,
					      const command_latency_tracker *tracker, uint64_t tick,
					      uint64_t pulse_start_mono_us)
{
	state->suppressed_reports = command_latency_saturating_add(state->suppressed_reports, 1);
	state->suppressed_slow_operations = command_latency_saturating_add(
		state->suppressed_slow_operations, tracker->slow_count);
	if (tracker->retained_slow_count > 0 &&
	    (!state->has_suppressed_worst ||
	     tracker->slowest[0].duration_us > state->suppressed_worst.duration_us))
	{
		state->suppressed_worst = tracker->slowest[0];
		state->suppressed_worst_tick = tick;
		state->suppressed_worst_pulse_start_mono_us = pulse_start_mono_us;
		state->has_suppressed_worst = true;
	}
}

static void command_latency_emit_throttled_pulse(const command_latency_tracker *tracker,
						 uint64_t sweep_us, const char *boot_id,
						 uint64_t tick, uint64_t pulse_start_mono_us,
						 command_latency_emit_fn emit, void *context)
{
	char line[1024];
	char tick_buffer[LATENCY_TRACE_TICK_STRING_LENGTH];
	const char *safe_boot_id = boot_id && *boot_id ? boot_id : "-";
	const command_latency_event *worst =
		tracker->retained_slow_count > 0 ? &tracker->slowest[0] : NULL;
	const uint64_t unattributed_us =
		sweep_us >= tracker->measured_us ? sweep_us - tracker->measured_us : 0;
	snprintf(line, sizeof line,
		 "COMMAND REPORT THROTTLED: boot=%s tick=%s pulse_start_mono_us=%" PRIu64
		 " total_us=%" PRIu64 " measured_operation_us=%" PRIu64
		 " unattributed_sweep_us=%" PRIu64 " slow_operations=%" PRIu64
		 " worst_kind=%s worst_state=%d worst_player_id=%ld worst_player=%s"
		 " worst_operation=%s worst_duration_us=%" PRIu64,
		 safe_boot_id, latency_trace_format_tick(tick, tick_buffer), pulse_start_mono_us,
		 sweep_us, tracker->measured_us, unattributed_us, tracker->slow_count,
		 worst ? command_latency_kind_name(worst->kind) : "-",
		 worst ? worst->connection_state : -1, worst ? worst->player_id : -1L,
		 worst ? worst->player_name : "-", worst ? worst->operation : "-",
		 worst ? worst->duration_us : 0);
	command_latency_emit(line, emit, context);
}

static void command_latency_emit_suppressed_summary(command_latency_report_state *state,
						    const char *boot_id, uint64_t tick,
						    uint64_t pulse_start_mono_us,
						    command_latency_emit_fn emit, void *context)
{
	if (!state->suppressed_reports)
		return;
	char line[1024];
	char tick_buffer[LATENCY_TRACE_TICK_STRING_LENGTH];
	char worst_tick_buffer[LATENCY_TRACE_TICK_STRING_LENGTH];
	const char *safe_boot_id = boot_id && *boot_id ? boot_id : "-";
	const command_latency_event *worst =
		state->has_suppressed_worst ? &state->suppressed_worst : NULL;
	snprintf(line, sizeof line,
		 "COMMAND REPORT THROTTLE: boot=%s tick=%s pulse_start_mono_us=%" PRIu64
		 " suppressed_reports=%" PRIu64 " suppressed_slow_operations=%" PRIu64
		 " suppressed_worst_tick=%s suppressed_worst_pulse_start_mono_us=%" PRIu64
		 " suppressed_worst_kind=%s suppressed_worst_state=%d"
		 " suppressed_worst_player_id=%ld suppressed_worst_player=%s"
		 " suppressed_worst_operation=%s suppressed_worst_duration_us=%" PRIu64,
		 safe_boot_id, latency_trace_format_tick(tick, tick_buffer), pulse_start_mono_us,
		 state->suppressed_reports, state->suppressed_slow_operations,
		 worst ? latency_trace_format_tick(state->suppressed_worst_tick,
						   worst_tick_buffer) :
			 "-",
		 worst ? state->suppressed_worst_pulse_start_mono_us : 0,
		 worst ? command_latency_kind_name(worst->kind) : "-",
		 worst ? worst->connection_state : -1, worst ? worst->player_id : -1L,
		 worst ? worst->player_name : "-", worst ? worst->operation : "-",
		 worst ? worst->duration_us : 0);
	command_latency_emit(line, emit, context);
	state->suppressed_reports = 0;
	state->suppressed_slow_operations = 0;
	state->has_suppressed_worst = false;
	memset(&state->suppressed_worst, 0, sizeof state->suppressed_worst);
	state->suppressed_worst_tick = 0;
	state->suppressed_worst_pulse_start_mono_us = 0;
}

void command_latency_report_throttled(command_latency_report_state *state,
				      const command_latency_tracker *tracker, uint64_t sweep_us,
				      const char *boot_id, uint64_t tick,
				      uint64_t pulse_start_mono_us, command_latency_emit_fn emit,
				      void *context)
{
	if (!state || !tracker || !emit ||
	    (sweep_us < COMMAND_LATENCY_SLOW_US && tracker->slow_count == 0))
		return;
	if (!command_latency_report_due(state, tick))
	{
		command_latency_retain_suppressed(state, tracker, tick, pulse_start_mono_us);
		command_latency_emit_throttled_pulse(tracker, sweep_us, boot_id, tick,
						     pulse_start_mono_us, emit, context);
		return;
	}

	state->has_reported = true;
	state->last_report_tick = tick;
	command_latency_emit_suppressed_summary(state, boot_id, tick, pulse_start_mono_us, emit,
						context);
	command_latency_report(tracker, sweep_us, boot_id, tick, pulse_start_mono_us, emit,
			       context);
}

void command_latency_log_buffer_reset(command_latency_log_buffer *report,
				      const char *continuation_prefix)
{
	if (!report)
		return;
	report->length = 0;
	report->text[0] = '\0';
	snprintf(report->continuation_prefix, sizeof report->continuation_prefix, "%s",
		 continuation_prefix && *continuation_prefix ? continuation_prefix :
							       "timestamp-unavailable::");
}

static bool command_latency_log_buffer_append(command_latency_log_buffer *report, const char *text)
{
	if (!report || !text || report->length >= sizeof report->text - 1)
		return false;
	const size_t available = sizeof report->text - report->length;
	const int written = snprintf(report->text + report->length, available, "%s", text);
	if (written < 0)
		return false;
	report->length += (size_t)written < available ? (size_t)written : available - 1;
	return (size_t)written < available;
}

void command_latency_log_buffer_collect(const char *line, void *context)
{
	if (!line || !context)
		return;
	command_latency_log_buffer *report = (command_latency_log_buffer *)context;
	const size_t previous_length = report->length;
	if (report->length &&
	    (!command_latency_log_buffer_append(report, "\n") ||
	     !command_latency_log_buffer_append(report, report->continuation_prefix)))
	{
		report->text[previous_length] = '\0';
		report->length = previous_length;
		return;
	}
	command_latency_log_buffer_append(report, line);
}
