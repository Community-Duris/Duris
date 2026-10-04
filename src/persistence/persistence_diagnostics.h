#pragma once

#include "persistence/critical_command.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>

// Diagnostic observations, never authority or replay input. No strings, object
// pointers, payloads, I/O, allocation, or blocking admission enter this recorder.
struct persistence_custody_witness
{
	uint64_t item_uid = 0;
	uint64_t expected_root = 0, expected_parent = 0;
	uint64_t observed_root = 0, observed_parent = 0;
	uint64_t observed_item_revision = 0;
	uint32_t source_line = 0;
	int32_t expected_vnum = 0, observed_vnum = 0;
	uint16_t expected_slot = 0, observed_slot = 0;
	bool expected_present = false, observed_present = false;
};

enum class persistence_trace_stage : uint8_t
{
	save_capture = 1,
	save_journal,
	save_checkpoint,
	save_apply,
	save_result,
	save_ack,
	save_fence,
	save_timeout,
	load_result,
	command_admitted,
	command_journal,
	command_apply,
	command_result,
	publication_ack,
	save_replay_apply,
	save_replay_result,
	save_replay_fence,
	command_checkpoint,
};

inline const char *persistence_trace_stage_name(persistence_trace_stage stage)
{
	switch (stage)
	{
	case persistence_trace_stage::save_capture:
		return "save_capture";
	case persistence_trace_stage::save_journal:
		return "save_journal";
	case persistence_trace_stage::save_checkpoint:
		return "save_checkpoint";
	case persistence_trace_stage::save_apply:
		return "save_apply";
	case persistence_trace_stage::save_result:
		return "save_result";
	case persistence_trace_stage::save_ack:
		return "save_ack";
	case persistence_trace_stage::save_fence:
		return "save_fence";
	case persistence_trace_stage::save_timeout:
		return "save_timeout";
	case persistence_trace_stage::load_result:
		return "load_result";
	case persistence_trace_stage::command_admitted:
		return "command_admitted";
	case persistence_trace_stage::command_journal:
		return "command_journal";
	case persistence_trace_stage::command_apply:
		return "command_apply";
	case persistence_trace_stage::command_result:
		return "command_result";
	case persistence_trace_stage::publication_ack:
		return "publication_ack";
	case persistence_trace_stage::save_replay_apply:
		return "save_replay_apply";
	case persistence_trace_stage::save_replay_result:
		return "save_replay_result";
	case persistence_trace_stage::save_replay_fence:
		return "save_replay_fence";
	case persistence_trace_stage::command_checkpoint:
		return "command_checkpoint";
	}
	return "unknown";
}

constexpr size_t PERSISTENCE_TRACE_CAPACITY = 4096;
constexpr size_t PERSISTENCE_INCIDENT_CAPACITY = 64;
constexpr size_t PERSISTENCE_TRACE_KEYS = 8;
constexpr size_t PERSISTENCE_TRACE_REPORT_EVENTS = 64;

struct persistence_trace_event
{
	uint64_t sequence = 0, mono_usec = 0;
	persistence_trace_stage stage = persistence_trace_stage::save_capture;
	int32_t pid = 0;
	uint64_t revision = 0, durable_revision = 0, components = 0, request_id = 0;
	critical_operation_id operation = {};
	uint16_t command_type = 0, source_site = 0;
	uint32_t outcome = 0, error = 0, diagnosis = 0, attempt = 0;
	persistence_custody_witness witness = {};
	std::array<critical_entity_key, PERSISTENCE_TRACE_KEYS> keys = {};
	size_t key_count = 0;
	bool keys_truncated = false, incident = false;
};

struct persistence_trace_filter
{
	critical_entity_type type = critical_entity_type::player;
	uint64_t id = 0;
	critical_operation_id operation = {};
	bool by_operation = false;
};

struct persistence_trace_snapshot
{
	uint64_t run = 0, latest_sequence = 0, overwritten = 0, dropped = 0;
	uint64_t incidents_evicted = 0, matching_events = 0;
	size_t retained = 0, incident_count = 0, count = 0;
	bool available = false;
	std::array<persistence_trace_event, PERSISTENCE_TRACE_REPORT_EVENTS> events = {};
	std::array<persistence_trace_event, PERSISTENCE_INCIDENT_CAPACITY> incidents = {};
};

namespace persistence_diagnostics_detail
{
inline std::mutex diagnostic_mutex;
inline std::array<persistence_trace_event, PERSISTENCE_TRACE_CAPACITY> diagnostic_events = {};
inline std::array<persistence_trace_event, PERSISTENCE_INCIDENT_CAPACITY> diagnostic_incidents = {};
inline size_t diagnostic_head = 0, diagnostic_count = 0, diagnostic_incident_head = 0,
	      diagnostic_incident_count = 0;
inline uint64_t diagnostic_sequence = 0, diagnostic_overwritten = 0,
		diagnostic_incidents_evicted = 0;
inline std::atomic<uint64_t> diagnostic_dropped{ 0 };
inline const uint64_t diagnostic_run =
	static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
				      std::chrono::system_clock::now().time_since_epoch())
				      .count());

inline bool matches(const persistence_trace_event &event, const persistence_trace_filter &filter)
{
	if (filter.by_operation)
		return event.operation.bytes == filter.operation.bytes;
	if (filter.type == critical_entity_type::player && event.pid > 0 &&
	    static_cast<uint64_t>(event.pid) == filter.id)
		return true;
	if (filter.type == critical_entity_type::item && event.witness.item_uid == filter.id)
		return true;
	for (size_t i = 0; i < event.key_count; ++i)
		if (event.keys[i].type == filter.type && event.keys[i].id == filter.id)
			return true;
	return false;
}
} // namespace persistence_diagnostics_detail

inline uint64_t persistence_trace_now()
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
					     std::chrono::steady_clock::now().time_since_epoch())
					     .count());
}

inline void persistence_trace_record(persistence_trace_event event) noexcept
{
	using namespace persistence_diagnostics_detail;
	std::unique_lock<std::mutex> lock(diagnostic_mutex, std::try_to_lock);
	if (!lock.owns_lock() || diagnostic_sequence == UINT64_MAX)
	{
		diagnostic_dropped.fetch_add(1, std::memory_order_relaxed);
		return;
	}
	event.sequence = ++diagnostic_sequence;
	event.mono_usec = persistence_trace_now();
	if (event.key_count > event.keys.size())
	{
		event.key_count = event.keys.size();
		event.keys_truncated = true;
	}
	diagnostic_events[diagnostic_head] = event;
	diagnostic_head = (diagnostic_head + 1) % diagnostic_events.size();
	if (diagnostic_count < diagnostic_events.size())
		++diagnostic_count;
	else
		++diagnostic_overwritten;
	if (!event.incident)
		return;
	// Preserve the first witness for a failed save/operation even after the rolling
	// history wraps. A timeout is a separate observation and must not suppress a
	// later definitive failure. Repeated attempts preserve each first observation.
	for (size_t i = 0; i < diagnostic_incident_count; ++i)
		if (diagnostic_incidents[i].pid == event.pid &&
		    diagnostic_incidents[i].revision == event.revision &&
		    diagnostic_incidents[i].operation.bytes == event.operation.bytes &&
		    diagnostic_incidents[i].request_id == event.request_id &&
		    (diagnostic_incidents[i].stage == persistence_trace_stage::save_timeout) ==
			    (event.stage == persistence_trace_stage::save_timeout))
			return;
	diagnostic_incidents[diagnostic_incident_head] = event;
	diagnostic_incident_head = (diagnostic_incident_head + 1) % diagnostic_incidents.size();
	if (diagnostic_incident_count < diagnostic_incidents.size())
		++diagnostic_incident_count;
	else
		++diagnostic_incidents_evicted;
}

inline persistence_trace_event persistence_command_trace(const critical_command &command,
							 persistence_trace_stage stage)
{
	persistence_trace_event event;
	event.stage = stage;
	event.operation = command.operation_id;
	event.command_type = static_cast<uint16_t>(command.type);
	event.source_site = static_cast<uint16_t>(command.source_site);
	event.key_count = command.keys.size();
	event.keys_truncated = event.key_count > event.keys.size();
	event.key_count = std::min(event.key_count, event.keys.size());
	for (size_t i = 0; i < event.key_count; ++i)
		event.keys[i] = command.keys[i];
	return event;
}

inline persistence_trace_snapshot persistence_trace_copy(const persistence_trace_filter &filter)
{
	using namespace persistence_diagnostics_detail;
	persistence_trace_snapshot result;
	result.run = diagnostic_run;
	result.dropped = diagnostic_dropped.load(std::memory_order_relaxed);
	std::unique_lock<std::mutex> lock(diagnostic_mutex, std::try_to_lock);
	if (!lock.owns_lock())
		return result;
	result.available = true;
	result.latest_sequence = diagnostic_sequence;
	result.overwritten = diagnostic_overwritten;
	result.incidents_evicted = diagnostic_incidents_evicted;
	result.retained = diagnostic_count;
	// Scan newest first, then reverse the bounded selection into chronology.
	for (size_t i = 0; i < diagnostic_count; ++i)
	{
		const auto &event =
			diagnostic_events[(diagnostic_head + diagnostic_events.size() - 1 - i) %
					  diagnostic_events.size()];
		if (!matches(event, filter))
			continue;
		++result.matching_events;
		if (result.count < result.events.size())
			result.events[result.count++] = event;
	}
	std::reverse(result.events.begin(), result.events.begin() + result.count);
	for (size_t i = 0; i < diagnostic_incident_count; ++i)
	{
		const auto &event =
			diagnostic_incidents[(diagnostic_incident_head + diagnostic_incidents.size() -
					      diagnostic_incident_count + i) %
					     diagnostic_incidents.size()];
		if (matches(event, filter))
			result.incidents[result.incident_count++] = event;
	}
	return result;
}

static_assert(sizeof(persistence_diagnostics_detail::diagnostic_events) +
		      sizeof(persistence_diagnostics_detail::diagnostic_incidents) <
	      2 * 1024 * 1024);
