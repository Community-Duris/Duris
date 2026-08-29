#include "boon_shop_command.h"

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

template <typename T> void put_le(uint8_t *output, T value)
{
	for (size_t byte = 0; byte < sizeof(T); ++byte)
		output[byte] = static_cast<uint8_t>(static_cast<uint64_t>(value) >> (byte * 8));
}

template <typename T> T get_le(const uint8_t *input)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < sizeof(T); ++byte)
		value |= static_cast<uint64_t>(input[byte]) << (byte * 8);
	return static_cast<T>(value);
}

bool valid(const boon_shop_payload &payload)
{
	return payload.pid && payload.stat_index < BOON_SHOP_BASE_STAT_COUNT;
}
} // namespace

bool boon_shop_command_build(critical_command *command, critical_operation_id operation_id,
			     const boon_shop_payload &payload)
{
	if (!command || !valid(payload))
		return false;
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::boon_shop,
		     .payload_version = BOON_SHOP_PAYLOAD_VERSION,
		     .source_site = critical_source_site::command,
		     .deadline_class = critical_deadline_class::interactive,
		     .accepted_at_usec = 0,
		     .keys = { { critical_entity_type::player, payload.pid } },
		     .expected_revisions = {},
		     .payload = {} };
	append_le(&command->payload, payload.pid);
	append_le(&command->payload, payload.stat_index);
	return true;
}

bool boon_shop_command_decode_payload(const critical_command &command, boon_shop_payload *payload)
{
	if (!payload || command.type != critical_command_type::boon_shop ||
	    command.payload_version != BOON_SHOP_PAYLOAD_VERSION)
		return false;
	*payload = {};
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	critical_command expected = {};
	return read_le(&cursor, end, &payload->pid) &&
	       read_le(&cursor, end, &payload->stat_index) && cursor == end && valid(*payload) &&
	       boon_shop_command_build(&expected, command.operation_id, *payload) &&
	       expected.keys.size() == command.keys.size() &&
	       std::equal(expected.keys.begin(), expected.keys.end(), command.keys.begin(),
			  critical_entity_key_equal) &&
	       command.expected_revisions.empty();
}

bool boon_shop_command_encode_result(const boon_shop_result &result,
				     std::array<uint8_t, BOON_SHOP_RESULT_BYTES> *encoded)
{
	if (!encoded || !result.pid || result.stat_index >= BOON_SHOP_BASE_STAT_COUNT ||
	    result.stat_value < 0 || result.stat_value > 100 || result.remaining_stat_points < 0)
		return false;
	encoded->fill(0);
	put_le(encoded->data(), result.pid);
	(*encoded)[4] = result.stat_index;
	put_le(encoded->data() + 8, result.stat_value);
	put_le(encoded->data() + 16, result.remaining_stat_points);
	put_le(encoded->data() + 24, result.stat_revision);
	return true;
}

bool boon_shop_command_decode_result(const uint8_t *encoded, size_t size, boon_shop_result *result)
{
	if (!encoded || !result || size != BOON_SHOP_RESULT_BYTES)
		return false;
	*result = { .pid = get_le<uint32_t>(encoded),
		    .stat_index = encoded[4],
		    .stat_value = get_le<int16_t>(encoded + 8),
		    .remaining_stat_points = get_le<int64_t>(encoded + 16),
		    .stat_revision = get_le<uint64_t>(encoded + 24) };
	std::array<uint8_t, BOON_SHOP_RESULT_BYTES> canonical = {};
	return boon_shop_command_encode_result(*result, &canonical) &&
	       std::equal(canonical.begin(), canonical.end(), encoded);
}
