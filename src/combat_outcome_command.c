#include "combat_outcome_command.h"

#include <algorithm>
#include <cstring>
#include <limits>

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

template <size_t N> bool bounded(const std::array<char, N> &value)
{
	return memchr(value.data(), '\0', value.size()) != nullptr;
}

template <size_t N>
void append_string(std::vector<uint8_t> *output, const std::array<char, N> &value)
{
	const size_t size = strnlen(value.data(), value.size());
	append_le<uint16_t>(output, static_cast<uint16_t>(size));
	output->insert(output->end(), value.data(), value.data() + size);
}

template <size_t N>
bool read_string(const uint8_t **cursor, const uint8_t *end, std::array<char, N> *value)
{
	uint16_t size = 0;
	if (!read_le(cursor, end, &size) || size >= N || static_cast<size_t>(end - *cursor) < size)
		return false;
	value->fill('\0');
	memcpy(value->data(), *cursor, size);
	*cursor += size;
	return true;
}

bool valid(const combat_outcome_payload &payload)
{
	if (!payload.victim_pid || !payload.participant_count ||
	    payload.participant_count > payload.participants.size() || !bounded(payload.room_name))
		return false;
	for (size_t index = 0; index < payload.participant_count; ++index)
	{
		const auto &entry = payload.participants[index];
		if (!entry.pid || entry.role == combat_participant_role::unknown ||
		    entry.role > combat_participant_role::victim_group ||
		    !bounded(entry.account_name) || !bounded(entry.description) ||
		    ((entry.wallet_delta_copper != 0) && !entry.account_name[0]))
			return false;
		for (size_t other = 0; other < index; ++other)
			if (payload.participants[other].pid == entry.pid)
				return false;
	}
	return std::any_of(payload.participants.begin(),
			   payload.participants.begin() + payload.participant_count,
			   [&](const auto &entry) { return entry.pid == payload.victim_pid; });
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
} // namespace

bool combat_outcome_command_encode_payload(const combat_outcome_payload &payload,
					   std::vector<uint8_t> *encoded)
{
	if (!encoded || !valid(payload))
		return false;
	encoded->clear();
	append_le<uint32_t>(encoded, payload.victim_pid);
	append_le<int32_t>(encoded, payload.room_vnum);
	append_string(encoded, payload.room_name);
	append_le<uint16_t>(encoded, payload.participant_count);
	for (size_t index = 0; index < payload.participant_count; ++index)
	{
		const auto &entry = payload.participants[index];
		append_le<uint32_t>(encoded, entry.pid);
		append_le<uint8_t>(encoded, static_cast<uint8_t>(entry.role));
		append_le<uint8_t>(encoded, entry.flags);
		append_le<uint16_t>(encoded, entry.level);
		append_le<uint8_t>(encoded, entry.racewar);
		append_le<int64_t>(encoded, entry.frag_delta);
		append_le<int64_t>(encoded, entry.epic_delta);
		append_le<int64_t>(encoded, entry.wallet_delta_copper);
		append_le<uint64_t>(encoded, entry.expected_frag_revision);
		append_le<uint64_t>(encoded, entry.expected_epic_revision);
		append_le<uint64_t>(encoded, entry.expected_wallet_revision);
		append_le<uint64_t>(encoded, entry.expected_bank_revision);
		append_string(encoded, entry.account_name);
		append_string(encoded, entry.description);
	}
	return encoded->size() <= CRITICAL_COMMAND_MAX_PAYLOAD_BYTES;
}

bool combat_outcome_command_decode_payload(const critical_command &command,
					   combat_outcome_payload *payload)
{
	if (!payload || command.type != critical_command_type::combat_outcome ||
	    command.payload_version != COMBAT_OUTCOME_PAYLOAD_VERSION)
		return false;
	*payload = {};
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	if (!read_le(&cursor, end, &payload->victim_pid) ||
	    !read_le(&cursor, end, &payload->room_vnum) ||
	    !read_string(&cursor, end, &payload->room_name) ||
	    !read_le(&cursor, end, &payload->participant_count) ||
	    payload->participant_count > payload->participants.size())
		return false;
	for (size_t index = 0; index < payload->participant_count; ++index)
	{
		auto &entry = payload->participants[index];
		uint8_t role = 0;
		if (!read_le(&cursor, end, &entry.pid) || !read_le(&cursor, end, &role) ||
		    !read_le(&cursor, end, &entry.flags) || !read_le(&cursor, end, &entry.level) ||
		    !read_le(&cursor, end, &entry.racewar) ||
		    !read_le(&cursor, end, &entry.frag_delta) ||
		    !read_le(&cursor, end, &entry.epic_delta) ||
		    !read_le(&cursor, end, &entry.wallet_delta_copper) ||
		    !read_le(&cursor, end, &entry.expected_frag_revision) ||
		    !read_le(&cursor, end, &entry.expected_epic_revision) ||
		    !read_le(&cursor, end, &entry.expected_wallet_revision) ||
		    !read_le(&cursor, end, &entry.expected_bank_revision) ||
		    !read_string(&cursor, end, &entry.account_name) ||
		    !read_string(&cursor, end, &entry.description))
			return false;
		entry.role = static_cast<combat_participant_role>(role);
	}
	if (cursor != end || !valid(*payload))
		return false;
	critical_command expected = {};
	if (!combat_outcome_command_build(&expected, command.operation_id, *payload))
		return false;
	return expected.keys.size() == command.keys.size() &&
	       std::equal(expected.keys.begin(), expected.keys.end(), command.keys.begin(),
			  critical_entity_key_equal) &&
	       expected.expected_revisions.size() == command.expected_revisions.size() &&
	       std::equal(expected.expected_revisions.begin(), expected.expected_revisions.end(),
			  command.expected_revisions.begin(),
			  [](const auto &left, const auto &right) {
				  return critical_entity_key_equal(left.key, right.key) &&
					 left.revision == right.revision;
			  });
}

bool combat_outcome_command_encode_result(const combat_outcome_result &result,
					  std::array<uint8_t, COMBAT_OUTCOME_RESULT_BYTES> *encoded)
{
	constexpr size_t ENTRY = 92;
	if (!encoded || result.participant_count > result.participants.size() ||
	    10 + result.participant_count * ENTRY > encoded->size())
		return false;
	encoded->fill(0);
	put_u64(encoded->data(), result.event_id);
	(*encoded)[8] = static_cast<uint8_t>(result.participant_count);
	(*encoded)[9] = static_cast<uint8_t>(result.participant_count >> 8);
	for (size_t index = 0; index < result.participant_count; ++index)
	{
		uint8_t *row = encoded->data() + 10 + index * ENTRY;
		const auto &entry = result.participants[index];
		for (size_t byte = 0; byte < 4; ++byte)
			row[byte] = static_cast<uint8_t>(entry.pid >> (byte * 8));
		put_u64(row + 4, static_cast<uint64_t>(entry.frags));
		put_u64(row + 12, static_cast<uint64_t>(entry.epics));
		put_u64(row + 20, static_cast<uint64_t>(entry.wallet_value));
		for (size_t denomination = 0; denomination < entry.bank.amount.size();
		     ++denomination)
			put_u64(row + 28 + denomination * 8,
				static_cast<uint64_t>(entry.bank.amount[denomination]));
		put_u64(row + 60, entry.frag_revision);
		put_u64(row + 68, entry.epic_revision);
		put_u64(row + 76, entry.wallet_revision);
		put_u64(row + 84, entry.bank_revision);
	}
	return true;
}

bool combat_outcome_command_decode_result(const uint8_t *encoded, size_t size,
					  combat_outcome_result *result)
{
	constexpr size_t ENTRY = 92;
	if (!encoded || !result || size != COMBAT_OUTCOME_RESULT_BYTES)
		return false;
	*result = {};
	result->event_id = get_u64(encoded);
	result->participant_count = encoded[8] | static_cast<uint16_t>(encoded[9]) << 8;
	if (result->participant_count > result->participants.size())
		return false;
	for (size_t index = 0; index < result->participant_count; ++index)
	{
		const uint8_t *row = encoded + 10 + index * ENTRY;
		auto &entry = result->participants[index];
		for (size_t byte = 0; byte < 4; ++byte)
			entry.pid |= static_cast<uint32_t>(row[byte]) << (byte * 8);
		entry.frags = static_cast<int64_t>(get_u64(row + 4));
		entry.epics = static_cast<int64_t>(get_u64(row + 12));
		entry.wallet_value = static_cast<int64_t>(get_u64(row + 20));
		for (size_t denomination = 0; denomination < entry.bank.amount.size();
		     ++denomination)
			entry.bank.amount[denomination] =
				static_cast<int64_t>(get_u64(row + 28 + denomination * 8));
		entry.frag_revision = get_u64(row + 60);
		entry.epic_revision = get_u64(row + 68);
		entry.wallet_revision = get_u64(row + 76);
		entry.bank_revision = get_u64(row + 84);
	}
	return true;
}

bool combat_outcome_command_build(critical_command *command, critical_operation_id operation_id,
				  const combat_outcome_payload &payload)
{
	if (!command || !valid(payload))
		return false;
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::combat_outcome,
		     .payload_version = COMBAT_OUTCOME_PAYLOAD_VERSION,
		     .source_site = critical_source_site::combat,
		     .deadline_class = critical_deadline_class::terminal,
		     .accepted_at_usec = 0,
		     .keys = {},
		     .expected_revisions = {},
		     .payload = {} };
	for (size_t index = 0; index < payload.participant_count; ++index)
	{
		const auto &entry = payload.participants[index];
		const critical_entity_key player = { critical_entity_type::player, entry.pid };
		command->keys.push_back(player);
		command->expected_revisions.push_back({ player, entry.expected_frag_revision });
		if (entry.wallet_delta_copper)
		{
			critical_entity_key account = {};
			if (!currency_account_key(entry.account_name.data(), entry.racewar,
						  &account))
				return false;
			const auto existing = std::find_if(
				command->keys.begin(), command->keys.end(), [&](const auto &key)
				{ return critical_entity_key_equal(key, account); });
			if (existing == command->keys.end())
			{
				command->keys.push_back(account);
				command->expected_revisions.push_back(
					{ account, entry.expected_bank_revision });
			}
		}
	}
	if (!combat_outcome_command_encode_payload(payload, &command->payload))
		return false;
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	std::sort(command->expected_revisions.begin(), command->expected_revisions.end(),
		  [](const auto &left, const auto &right)
		  { return critical_entity_key_less(left.key, right.key); });
	return command->keys.size() <= CRITICAL_COMMAND_MAX_KEYS;
}
