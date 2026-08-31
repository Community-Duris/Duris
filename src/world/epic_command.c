#include "world/epic_command.h"

#include <array>
#include <limits>
#include <utility>

namespace
{
void put_u16(uint8_t *output, uint16_t value)
{
	output[0] = static_cast<uint8_t>(value);
	output[1] = static_cast<uint8_t>(value >> 8);
}

void put_u32(uint8_t *output, uint32_t value)
{
	for (unsigned int byte = 0; byte < 4; ++byte)
		output[byte] = static_cast<uint8_t>(value >> (byte * 8));
}

void put_u64(uint8_t *output, uint64_t value)
{
	for (unsigned int byte = 0; byte < 8; ++byte)
		output[byte] = static_cast<uint8_t>(value >> (byte * 8));
}

uint16_t get_u16(const uint8_t *input)
{
	return static_cast<uint16_t>(input[0]) |
	       static_cast<uint16_t>(static_cast<uint16_t>(input[1]) << 8);
}

uint32_t get_u32(const uint8_t *input)
{
	uint32_t value = 0;
	for (unsigned int byte = 0; byte < 4; ++byte)
		value |= static_cast<uint32_t>(input[byte]) << (byte * 8);
	return value;
}

uint64_t get_u64(const uint8_t *input)
{
	uint64_t value = 0;
	for (unsigned int byte = 0; byte < 8; ++byte)
		value |= static_cast<uint64_t>(input[byte]) << (byte * 8);
	return value;
}

bool valid_reason(epic_reason_type reason)
{
	return reason > epic_reason_type::unknown && reason <= epic_reason_type::admin_adjustment;
}
} // namespace

bool epic_command_encode_payload(const epic_command_payload &payload, std::vector<uint8_t> *encoded)
{
	if (!encoded || !payload.pid || !payload.delta ||
	    payload.delta == std::numeric_limits<int64_t>::min() || !valid_reason(payload.reason) ||
	    (payload.flags & ~EPIC_COMMAND_REQUIRE_FUNDS) ||
	    (payload.delta > 0 && (payload.flags & EPIC_COMMAND_REQUIRE_FUNDS)))
		return false;
	encoded->assign(EPIC_COMMAND_PAYLOAD_BYTES, 0);
	put_u32(encoded->data(), payload.pid);
	put_u64(encoded->data() + 4, static_cast<uint64_t>(payload.delta));
	put_u16(encoded->data() + 12, static_cast<uint16_t>(payload.reason));
	put_u16(encoded->data() + 14, payload.flags);
	put_u64(encoded->data() + 16, static_cast<uint64_t>(payload.reason_id));
	return true;
}

bool epic_command_decode_payload(const critical_command &command, epic_command_payload *payload)
{
	if (!payload || command.type != critical_command_type::epic ||
	    command.payload_version != EPIC_COMMAND_PAYLOAD_VERSION ||
	    command.payload.size() != EPIC_COMMAND_PAYLOAD_BYTES)
		return false;
	for (size_t index = 24; index < command.payload.size(); ++index)
		if (command.payload[index])
			return false;
	*payload = {
		.pid = get_u32(command.payload.data()),
		.delta = static_cast<int64_t>(get_u64(command.payload.data() + 4)),
		.reason = static_cast<epic_reason_type>(get_u16(command.payload.data() + 12)),
		.flags = get_u16(command.payload.data() + 14),
		.reason_id = static_cast<int64_t>(get_u64(command.payload.data() + 16)),
	};
	if (!payload->pid || !payload->delta ||
	    payload->delta == std::numeric_limits<int64_t>::min() ||
	    !valid_reason(payload->reason) || (payload->flags & ~EPIC_COMMAND_REQUIRE_FUNDS) ||
	    (payload->delta > 0 && (payload->flags & EPIC_COMMAND_REQUIRE_FUNDS)))
		return false;
	return command.keys.size() == 1 && command.keys[0].type == critical_entity_type::player &&
	       command.keys[0].id == payload->pid && command.expected_revisions.size() == 1 &&
	       critical_entity_key_equal(command.expected_revisions[0].key, command.keys[0]);
}

bool epic_command_encode_result(const epic_command_result &result,
				std::array<uint8_t, EPIC_RESULT_PAYLOAD_BYTES> *encoded)
{
	if (!encoded || !result.delta)
		return false;
	put_u64(encoded->data(), static_cast<uint64_t>(result.balance));
	put_u64(encoded->data() + 8, result.revision);
	put_u64(encoded->data() + 16, static_cast<uint64_t>(result.delta));
	return true;
}

bool epic_command_decode_result(const uint8_t *encoded, size_t size, epic_command_result *result)
{
	if (!encoded || size != EPIC_RESULT_PAYLOAD_BYTES || !result)
		return false;
	*result = { .balance = static_cast<int64_t>(get_u64(encoded)),
		    .revision = get_u64(encoded + 8),
		    .delta = static_cast<int64_t>(get_u64(encoded + 16)) };
	return result->delta && result->delta != std::numeric_limits<int64_t>::min();
}

bool epic_command_build(critical_command *command, critical_operation_id operation_id,
			const epic_command_payload &payload, uint64_t expected_revision,
			critical_source_site source_site, critical_deadline_class deadline_class)
{
	if (!command || critical_operation_id_is_zero(operation_id))
		return false;
	std::vector<uint8_t> encoded;
	if (!epic_command_encode_payload(payload, &encoded))
		return false;
	const critical_entity_key key = { critical_entity_type::player, payload.pid };
	*command = {
		.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		.operation_id = operation_id,
		.type = critical_command_type::epic,
		.payload_version = EPIC_COMMAND_PAYLOAD_VERSION,
		.source_site = source_site,
		.deadline_class = deadline_class,
		.accepted_at_usec = 0,
		.keys = { key },
		.expected_revisions = { { key, expected_revision } },
		.payload = std::move(encoded),
	};
	return true;
}
