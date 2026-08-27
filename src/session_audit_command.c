#include "session_audit_command.h"

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

bool valid(const session_audit_payload &payload)
{
	return payload.pid && payload.occurred_at > 0 &&
	       (payload.event == session_audit_event::login ||
		payload.event == session_audit_event::logout);
}
} // namespace

bool session_audit_command_build(critical_command *command, critical_operation_id operation_id,
				 const session_audit_payload &payload)
{
	if (!command || !valid(payload))
		return false;
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::session_audit,
		     .payload_version = SESSION_AUDIT_PAYLOAD_VERSION,
		     .source_site = critical_source_site::login,
		     .deadline_class = critical_deadline_class::background,
		     .accepted_at_usec = 0,
		     .keys = { { critical_entity_type::player, payload.pid } },
		     .expected_revisions = {},
		     .payload = {} };
	append_le<uint32_t>(&command->payload, payload.pid);
	append_le<uint8_t>(&command->payload, static_cast<uint8_t>(payload.event));
	append_le<int64_t>(&command->payload, payload.occurred_at);
	return true;
}

bool session_audit_command_decode_payload(const critical_command &command,
					  session_audit_payload *payload)
{
	if (!payload || command.type != critical_command_type::session_audit ||
	    command.payload_version != SESSION_AUDIT_PAYLOAD_VERSION)
		return false;
	*payload = {};
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	uint8_t event = 0;
	if (!read_le(&cursor, end, &payload->pid) || !read_le(&cursor, end, &event) ||
	    !read_le(&cursor, end, &payload->occurred_at) || cursor != end)
		return false;
	payload->event = static_cast<session_audit_event>(event);
	critical_command expected = {};
	return valid(*payload) &&
	       session_audit_command_build(&expected, command.operation_id, *payload) &&
	       expected.keys.size() == command.keys.size() &&
	       std::equal(expected.keys.begin(), expected.keys.end(), command.keys.begin(),
			  critical_entity_key_equal) &&
	       command.expected_revisions.empty();
}

bool session_audit_command_encode_result(const session_audit_result &result,
					 std::array<uint8_t, SESSION_AUDIT_RESULT_BYTES> *encoded)
{
	if (!encoded || !valid(result))
		return false;
	critical_command command = {};
	critical_operation_id operation = {};
	operation.bytes[0] = 1;
	if (!session_audit_command_build(&command, operation, result) ||
	    command.payload.size() > encoded->size())
		return false;
	encoded->fill(0);
	std::copy(command.payload.begin(), command.payload.end(), encoded->begin());
	return true;
}

bool session_audit_command_decode_result(const uint8_t *encoded, size_t size,
					 session_audit_result *result)
{
	if (!encoded || !result || size != SESSION_AUDIT_RESULT_BYTES)
		return false;
	*result = {};
	const uint8_t *cursor = encoded;
	const uint8_t *end = encoded + size;
	uint8_t event = 0;
	if (!read_le(&cursor, end, &result->pid) || !read_le(&cursor, end, &event) ||
	    !read_le(&cursor, end, &result->occurred_at))
		return false;
	result->event = static_cast<session_audit_event>(event);
	return std::all_of(cursor, end, [](uint8_t byte) { return byte == 0; }) && valid(*result);
}
