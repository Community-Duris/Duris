#pragma once

#include "persistence/persistence_diagnostics.h"

#include <charconv>
#include <sstream>
#include <string>
#include <string_view>

inline bool persistence_trace_parse_target(std::string_view kind, std::string_view value,
					   persistence_trace_filter *filter)
{
	if (!filter || value.empty())
		return false;
	*filter = {};
	if (kind == "operation")
	{
		if (value.size() != 32)
			return false;
		filter->by_operation = true;
		for (size_t i = 0; i < 16; ++i)
		{
			unsigned int byte = 0;
			const auto parsed = std::from_chars(value.data() + i * 2,
							    value.data() + i * 2 + 2, byte, 16);
			if (parsed.ec != std::errc{} || parsed.ptr != value.data() + i * 2 + 2)
				return false;
			filter->operation.bytes[i] = static_cast<uint8_t>(byte);
		}
		return filter->operation.bytes != std::array<uint8_t, 16>{};
	}
	if (kind != "player" && kind != "item")
		return false;
	const auto parsed = std::from_chars(value.data(), value.data() + value.size(), filter->id);
	if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || !filter->id ||
	    (kind == "player" && filter->id > INT32_MAX))
		return false;
	filter->type = kind == "player" ? critical_entity_type::player : critical_entity_type::item;
	return true;
}

inline std::string persistence_trace_operation_hex(const critical_operation_id &operation)
{
	constexpr char digits[] = "0123456789abcdef";
	std::string hex(32, '0');
	for (size_t i = 0; i < operation.bytes.size(); ++i)
	{
		hex[i * 2] = digits[operation.bytes[i] >> 4];
		hex[i * 2 + 1] = digits[operation.bytes[i] & 15];
	}
	return hex;
}

inline void persistence_trace_event_json(std::ostream &out, const persistence_trace_event &event)
{
	const auto &w = event.witness;
	out << "{\"sequence\":" << event.sequence << ",\"mono_usec\":" << event.mono_usec
	    << ",\"stage\":\"" << persistence_trace_stage_name(event.stage)
	    << "\",\"pid\":" << event.pid << ",\"revision\":" << event.revision
	    << ",\"durable_revision\":" << event.durable_revision
	    << ",\"request_id\":" << event.request_id << ",\"components\":" << event.components
	    << ",\"operation\":\"" << persistence_trace_operation_hex(event.operation)
	    << "\",\"command_type\":" << event.command_type
	    << ",\"source_site\":" << event.source_site << ",\"outcome\":" << event.outcome
	    << ",\"error\":" << event.error << ",\"diagnosis\":" << event.diagnosis
	    << ",\"attempt\":" << event.attempt << ",\"keys_truncated\":" << event.keys_truncated
	    << ",\"witness\":{\"item_uid\":" << w.item_uid
	    << ",\"expected_present\":" << w.expected_present
	    << ",\"observed_present\":" << w.observed_present
	    << ",\"expected_root\":" << w.expected_root
	    << ",\"expected_parent\":" << w.expected_parent
	    << ",\"observed_root\":" << w.observed_root
	    << ",\"observed_parent\":" << w.observed_parent
	    << ",\"observed_item_revision\":" << w.observed_item_revision
	    << ",\"source_line\":" << w.source_line << ",\"expected_vnum\":" << w.expected_vnum
	    << ",\"observed_vnum\":" << w.observed_vnum << ",\"expected_slot\":" << w.expected_slot
	    << ",\"observed_slot\":" << w.observed_slot << "},\"keys\":[";
	for (size_t i = 0; i < event.key_count; ++i)
	{
		if (i)
			out << ',';
		out << "{\"type\":" << static_cast<unsigned>(event.keys[i].type)
		    << ",\"id\":" << event.keys[i].id << '}';
	}
	out << "]}";
}

inline void persistence_trace_snapshot_json(std::ostream &out,
					    const persistence_trace_snapshot &trace)
{
	out << "\"history\":{\"run\":" << trace.run << ",\"available\":" << trace.available
	    << ",\"latest_sequence\":" << trace.latest_sequence
	    << ",\"retained\":" << trace.retained
	    << ",\"matching_events\":" << trace.matching_events
	    << ",\"overwritten\":" << trace.overwritten << ",\"dropped\":" << trace.dropped
	    << ",\"incidents_evicted\":" << trace.incidents_evicted << ",\"events\":[";
	for (size_t i = 0; i < trace.count; ++i)
	{
		if (i)
			out << ',';
		persistence_trace_event_json(out, trace.events[i]);
	}
	out << "],\"incidents\":[";
	for (size_t i = 0; i < trace.incident_count; ++i)
	{
		if (i)
			out << ',';
		persistence_trace_event_json(out, trace.incidents[i]);
	}
	out << "]}";
}
