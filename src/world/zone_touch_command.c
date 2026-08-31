#include "world/zone_touch_command.h"

#include <algorithm>

namespace
{
template <typename T> void append_le(std::vector<uint8_t> *output, T value)
{
	for (size_t byte = 0; byte < sizeof(T); ++byte)
		output->push_back(static_cast<uint8_t>(static_cast<uint64_t>(value) >> (byte * 8)));
}

template <typename T> bool read_le(const uint8_t **cursor, const uint8_t *end, T *value)
{
	if (!cursor || !*cursor || !value || static_cast<size_t>(end - *cursor) < sizeof(T))
		return false;
	uint64_t decoded = 0;
	for (size_t byte = 0; byte < sizeof(T); ++byte)
		decoded |= static_cast<uint64_t>((*cursor)[byte]) << (byte * 8);
	*cursor += sizeof(T);
	*value = static_cast<T>(decoded);
	return true;
}

bool valid(const zone_touch_payload &payload)
{
	if (!(payload.zone_number && payload.toucher_pid && payload.group_size &&
	      payload.group_size <= ZONE_TOUCH_MAX_PARTICIPANTS &&
	      payload.participant_pids[0] == payload.toucher_pid &&
	      std::all_of(payload.participant_pids.begin(),
			  payload.participant_pids.begin() + payload.group_size,
			  [](uint32_t pid) { return pid != 0; }) &&
	      std::all_of(payload.participant_pids.begin() + payload.group_size,
			  payload.participant_pids.end(), [](uint32_t pid) { return pid == 0; }) &&
	      payload.alignment_delta >= -1 && payload.alignment_delta <= 1 &&
	      payload.reset_requested <= 1))
		return false;
	for (size_t index = 0; index < payload.group_size; ++index)
		if (std::find(payload.participant_pids.begin(),
			      payload.participant_pids.begin() + index,
			      payload.participant_pids[index]) !=
		    payload.participant_pids.begin() + index)
			return false;
	return true;
}
} // namespace

bool zone_touch_command_build(critical_command *command, critical_operation_id operation_id,
			      const zone_touch_payload &payload)
{
	if (!command || !valid(payload))
		return false;
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::zone,
		     .payload_version = ZONE_TOUCH_PAYLOAD_VERSION,
		     .source_site = critical_source_site::zone_event,
		     .deadline_class = critical_deadline_class::interactive,
		     .accepted_at_usec = 0,
		     .keys = {},
		     .expected_revisions = {},
		     .payload = {} };
	for (size_t index = 0; index < payload.group_size; ++index)
		command->keys.push_back(
			{ critical_entity_type::player, payload.participant_pids[index] });
	command->keys.push_back({ critical_entity_type::zone, payload.zone_number });
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	append_le<uint32_t>(&command->payload, payload.zone_number);
	append_le<uint32_t>(&command->payload, payload.toucher_pid);
	append_le<int32_t>(&command->payload, payload.boot_time);
	append_le<int32_t>(&command->payload, payload.touched_at);
	append_le<uint16_t>(&command->payload, payload.group_size);
	for (size_t index = 0; index < payload.group_size; ++index)
		append_le<uint32_t>(&command->payload, payload.participant_pids[index]);
	append_le<int32_t>(&command->payload, payload.epic_value);
	append_le<int16_t>(&command->payload, payload.alignment_delta);
	append_le<uint8_t>(&command->payload, payload.reset_requested);
	return true;
}

bool zone_touch_command_decode_payload(const critical_command &command, zone_touch_payload *payload)
{
	if (!payload || command.type != critical_command_type::zone ||
	    command.payload_version != ZONE_TOUCH_PAYLOAD_VERSION)
		return false;
	*payload = {};
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	if (!read_le(&cursor, end, &payload->zone_number) ||
	    !read_le(&cursor, end, &payload->toucher_pid) ||
	    !read_le(&cursor, end, &payload->boot_time) ||
	    !read_le(&cursor, end, &payload->touched_at) ||
	    !read_le(&cursor, end, &payload->group_size) ||
	    payload->group_size > ZONE_TOUCH_MAX_PARTICIPANTS)
		return false;
	for (size_t index = 0; index < payload->group_size; ++index)
		if (!read_le(&cursor, end, &payload->participant_pids[index]))
			return false;
	if (!read_le(&cursor, end, &payload->epic_value) ||
	    !read_le(&cursor, end, &payload->alignment_delta) ||
	    !read_le(&cursor, end, &payload->reset_requested) || cursor != end)
		return false;
	critical_command expected = {};
	return valid(*payload) &&
	       zone_touch_command_build(&expected, command.operation_id, *payload) &&
	       expected.keys.size() == command.keys.size() &&
	       std::equal(expected.keys.begin(), expected.keys.end(), command.keys.begin(),
			  critical_entity_key_equal) &&
	       command.expected_revisions.empty();
}

bool zone_touch_command_encode_result(const zone_touch_result &result,
				      std::array<uint8_t, ZONE_TOUCH_RESULT_BYTES> *encoded)
{
	if (!encoded || !valid(result))
		return false;
	critical_command command = {};
	critical_operation_id operation = {};
	operation.bytes[0] = 1;
	if (!zone_touch_command_build(&command, operation, result) ||
	    command.payload.size() > encoded->size())
		return false;
	encoded->fill(0);
	std::copy(command.payload.begin(), command.payload.end(), encoded->begin());
	return true;
}

bool zone_touch_command_decode_result(const uint8_t *encoded, size_t size,
				      zone_touch_result *result)
{
	if (!encoded || !result || size != ZONE_TOUCH_RESULT_BYTES)
		return false;
	const uint8_t *cursor = encoded;
	const uint8_t *end = encoded + size;
	*result = {};
	if (!read_le(&cursor, end, &result->zone_number) ||
	    !read_le(&cursor, end, &result->toucher_pid) ||
	    !read_le(&cursor, end, &result->boot_time) ||
	    !read_le(&cursor, end, &result->touched_at) ||
	    !read_le(&cursor, end, &result->group_size))
		return false;
	if (result->group_size > ZONE_TOUCH_MAX_PARTICIPANTS)
		return false;
	for (size_t index = 0; index < result->group_size; ++index)
		if (!read_le(&cursor, end, &result->participant_pids[index]))
			return false;
	if (!read_le(&cursor, end, &result->epic_value) ||
	    !read_le(&cursor, end, &result->alignment_delta) ||
	    !read_le(&cursor, end, &result->reset_requested))
		return false;
	return std::all_of(cursor, end, [](uint8_t byte) { return byte == 0; }) && valid(*result);
}
