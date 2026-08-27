#include "boon_reward_command.h"

#include <bit>

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

void put_u64(uint8_t *output, uint64_t value)
{
	for (size_t byte = 0; byte < 8; ++byte)
		output[byte] = static_cast<uint8_t>(value >> (byte * 8));
}

uint64_t get_u64(const uint8_t *input)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < 8; ++byte)
		value |= static_cast<uint64_t>(input[byte]) << (byte * 8);
	return value;
}

bool valid(const boon_reward_payload &payload)
{
	return payload.pid && payload.option < 14 && payload.level <= 255 &&
	       !(payload.victim_flags & ~uint8_t{ 3 });
}
} // namespace

bool boon_reward_command_build(critical_command *command, critical_operation_id operation_id,
			       const boon_reward_payload &payload)
{
	if (!command || !valid(payload))
		return false;
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::boon_reward,
		     .payload_version = BOON_REWARD_PAYLOAD_VERSION,
		     .source_site = critical_source_site::combat,
		     .deadline_class = critical_deadline_class::background,
		     .accepted_at_usec = 0,
		     .keys = { { critical_entity_type::player, payload.pid } },
		     .expected_revisions = {},
		     .payload = {} };
	append_le<uint32_t>(&command->payload, payload.pid);
	append_le<uint8_t>(&command->payload, payload.racewar);
	append_le<uint16_t>(&command->payload, payload.level);
	append_le<int32_t>(&command->payload, payload.zone_number);
	append_le<uint8_t>(&command->payload, payload.option);
	append_le<uint64_t>(&command->payload, std::bit_cast<uint64_t>(payload.data));
	append_le<int32_t>(&command->payload, payload.victim_vnum);
	append_le<int16_t>(&command->payload, payload.victim_race);
	append_le<uint8_t>(&command->payload, payload.victim_flags);
	return true;
}

bool boon_reward_command_decode_payload(const critical_command &command,
					boon_reward_payload *payload)
{
	if (!payload || command.type != critical_command_type::boon_reward ||
	    command.payload_version != BOON_REWARD_PAYLOAD_VERSION)
		return false;
	*payload = {};
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	uint64_t data = 0;
	if (!read_le(&cursor, end, &payload->pid) || !read_le(&cursor, end, &payload->racewar) ||
	    !read_le(&cursor, end, &payload->level) ||
	    !read_le(&cursor, end, &payload->zone_number) ||
	    !read_le(&cursor, end, &payload->option) || !read_le(&cursor, end, &data) ||
	    !read_le(&cursor, end, &payload->victim_vnum) ||
	    !read_le(&cursor, end, &payload->victim_race) ||
	    !read_le(&cursor, end, &payload->victim_flags) || cursor != end)
		return false;
	payload->data = std::bit_cast<double>(data);
	critical_command expected = {};
	return valid(*payload) &&
	       boon_reward_command_build(&expected, command.operation_id, *payload) &&
	       expected.keys.size() == command.keys.size() &&
	       std::equal(expected.keys.begin(), expected.keys.end(), command.keys.begin(),
			  critical_entity_key_equal) &&
	       command.expected_revisions.empty();
}

bool boon_reward_command_encode_result(const boon_reward_result &result,
				       std::array<uint8_t, BOON_REWARD_RESULT_BYTES> *encoded)
{
	constexpr size_t HEADER = 8;
	constexpr size_t ENTRY = 56;
	if (!encoded || result.entry_count > result.entries.size() ||
	    HEADER + result.entry_count * ENTRY > encoded->size())
		return false;
	encoded->fill(0);
	for (size_t byte = 0; byte < 4; ++byte)
		(*encoded)[byte] = static_cast<uint8_t>(result.pid >> (byte * 8));
	(*encoded)[4] = static_cast<uint8_t>(result.entry_count);
	(*encoded)[5] = static_cast<uint8_t>(result.entry_count >> 8);
	for (size_t index = 0; index < result.entry_count; ++index)
	{
		uint8_t *row = encoded->data() + HEADER + index * ENTRY;
		const auto &entry = result.entries[index];
		for (size_t byte = 0; byte < 4; ++byte)
			row[byte] = static_cast<uint8_t>(entry.boon_id >> (byte * 8));
		row[4] = entry.type;
		row[5] = entry.option;
		row[6] = entry.flags;
		row[7] = entry.repeat;
		put_u64(row + 8, std::bit_cast<uint64_t>(entry.criteria));
		put_u64(row + 16, std::bit_cast<uint64_t>(entry.criteria2));
		put_u64(row + 24, std::bit_cast<uint64_t>(entry.bonus));
		put_u64(row + 32, std::bit_cast<uint64_t>(entry.bonus2));
		put_u64(row + 40, std::bit_cast<uint64_t>(entry.counter));
	}
	return true;
}

bool boon_reward_command_decode_result(const uint8_t *encoded, size_t size,
				       boon_reward_result *result)
{
	constexpr size_t HEADER = 8;
	constexpr size_t ENTRY = 56;
	if (!encoded || !result || size != BOON_REWARD_RESULT_BYTES)
		return false;
	*result = {};
	for (size_t byte = 0; byte < 4; ++byte)
		result->pid |= static_cast<uint32_t>(encoded[byte]) << (byte * 8);
	result->entry_count = encoded[4] | static_cast<uint16_t>(encoded[5]) << 8;
	if (!result->pid || result->entry_count > result->entries.size())
		return false;
	for (size_t index = 0; index < result->entry_count; ++index)
	{
		const uint8_t *row = encoded + HEADER + index * ENTRY;
		auto &entry = result->entries[index];
		for (size_t byte = 0; byte < 4; ++byte)
			entry.boon_id |= static_cast<uint32_t>(row[byte]) << (byte * 8);
		entry.type = row[4];
		entry.option = row[5];
		entry.flags = row[6];
		entry.repeat = row[7];
		entry.criteria = std::bit_cast<double>(get_u64(row + 8));
		entry.criteria2 = std::bit_cast<double>(get_u64(row + 16));
		entry.bonus = std::bit_cast<double>(get_u64(row + 24));
		entry.bonus2 = std::bit_cast<double>(get_u64(row + 32));
		entry.counter = std::bit_cast<double>(get_u64(row + 40));
	}
	return true;
}
