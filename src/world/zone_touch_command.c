#include "world/zone_touch_command.h"

#include <algorithm>
#include "world/epic_command.h"

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
	if (!((payload.zone_number || (payload.stone_uid && !payload.record_zone)) &&
	      payload.toucher_pid && payload.group_size &&
	      payload.group_size <= ZONE_TOUCH_MAX_PARTICIPANTS &&
	      payload.participant_pids[0] == payload.toucher_pid &&
	      std::all_of(payload.participant_pids.begin(),
			  payload.participant_pids.begin() + payload.group_size,
			  [](uint32_t pid) { return pid != 0; }) &&
	      std::all_of(payload.participant_pids.begin() + payload.group_size,
			  payload.participant_pids.end(), [](uint32_t pid) { return pid == 0; }) &&
	      payload.alignment_delta >= -1 && payload.alignment_delta <= 1 &&
	      payload.reset_requested <= 1 && payload.record_zone <= 1))
		return false;
	if (payload.stone_uid)
	{
		for (size_t index = 0; index < payload.awards.size(); ++index)
		{
			const auto &award = payload.awards[index];
			if (index < payload.group_size ?
				    (award.amount <= 0 || award.flags > 3) :
				    (award.amount || award.errand || award.flags))
				return false;
		}
	}
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
		     .payload_version = static_cast<uint16_t>(
			     payload.stone_uid ? ZONE_TOUCH_PAYLOAD_VERSION : 1),
		     .source_site = critical_source_site::zone_event,
		     .deadline_class = critical_deadline_class::interactive,
		     .accepted_at_usec = 0,
		     .keys = {},
		     .expected_revisions = {},
		     .payload = {} };
	for (size_t index = 0; index < payload.group_size; ++index)
		command->keys.push_back(
			{ critical_entity_type::player, payload.participant_pids[index] });
	command->keys.push_back(
		payload.record_zone ?
			critical_entity_key{ critical_entity_type::zone, payload.zone_number } :
			critical_entity_key{ critical_entity_type::item, payload.stone_uid });
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
	if (payload.stone_uid)
	{
		append_le<uint64_t>(&command->payload, payload.stone_uid);
		append_le<int32_t>(&command->payload, payload.stone_level);
		append_le<uint8_t>(&command->payload, payload.record_zone);
		for (size_t i = 0; i < payload.group_size; ++i)
		{
			append_le<int32_t>(&command->payload, payload.awards[i].amount);
			append_le<int32_t>(&command->payload, payload.awards[i].errand);
			append_le<uint8_t>(&command->payload, payload.awards[i].flags);
		}
	}
	return true;
}

bool zone_touch_command_decode_payload(const critical_command &command, zone_touch_payload *payload)
{
	if (!payload || command.type != critical_command_type::zone ||
	    (command.payload_version != 1 && command.payload_version != ZONE_TOUCH_PAYLOAD_VERSION))
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
	    !read_le(&cursor, end, &payload->reset_requested))
		return false;
	if (command.payload_version == ZONE_TOUCH_PAYLOAD_VERSION)
	{
		if (!read_le(&cursor, end, &payload->stone_uid) || !payload->stone_uid ||
		    !read_le(&cursor, end, &payload->stone_level) ||
		    !read_le(&cursor, end, &payload->record_zone))
			return false;
		for (size_t i = 0; i < payload->group_size; ++i)
			if (!read_le(&cursor, end, &payload->awards[i].amount) ||
			    !read_le(&cursor, end, &payload->awards[i].errand) ||
			    !read_le(&cursor, end, &payload->awards[i].flags))
				return false;
	}
	if (cursor != end)
		return false;
	critical_command expected = {};
	return valid(*payload) &&
	       zone_touch_command_build(&expected, command.operation_id, *payload) &&
	       expected.keys.size() == command.keys.size() &&
	       std::equal(expected.keys.begin(), expected.keys.end(), command.keys.begin(),
			  critical_entity_key_equal) &&
	       command.expected_revisions.empty();
}

bool zone_touch_award_command(const critical_command &parent, size_t index, critical_command *award)
{
	zone_touch_payload payload = {};
	critical_operation_id id = {};
	const bool built =
		award && zone_touch_command_decode_payload(parent, &payload) && payload.stone_uid &&
		index < payload.group_size &&
		critical_operation_id_derive(parent.operation_id, 0x5a544132,
					     payload.participant_pids[index], &id) &&
		epic_command_build(award, id,
				   { payload.participant_pids[index], payload.awards[index].amount,
				     epic_reason_type::zone_award, 0, payload.zone_number },
				   UINT64_MAX, critical_source_site::zone_event,
				   critical_deadline_class::interactive);
	if (built)
		award->accepted_at_usec = parent.accepted_at_usec;
	return built;
}

bool zone_touch_command_encode_result(const zone_touch_result &result,
				      std::array<uint8_t, ZONE_TOUCH_RESULT_BYTES> *encoded)
{
	critical_command command = {};
	critical_operation_id operation = {};
	operation.bytes[0] = 1;
	if (!encoded || !zone_touch_command_build(&command, operation, result))
		return false;
	std::vector<uint8_t> bytes;
	append_le<uint16_t>(&bytes, command.payload_version);
	append_le<uint16_t>(&bytes, static_cast<uint16_t>(command.payload.size()));
	bytes.insert(bytes.end(), command.payload.begin(), command.payload.end());
	for (size_t i = 0; i < result.group_size; ++i)
	{
		append_le<int64_t>(&bytes, result.balances[i]);
		append_le<uint64_t>(&bytes, result.revisions[i]);
	}
	append_le<uint8_t>(&bytes, result.recovered_claim ? 1 : 0);
	if (bytes.size() > encoded->size())
		return false;
	encoded->fill(0);
	std::copy(bytes.begin(), bytes.end(), encoded->begin());
	return true;
}

bool zone_touch_command_decode_result(const uint8_t *encoded, size_t size,
				      zone_touch_result *result)
{
	if (!encoded || !result ||
	    (size != ZONE_TOUCH_RESULT_BYTES && size != ZONE_TOUCH_LEGACY_RESULT_BYTES))
		return false;
	*result = {};
	const uint8_t *cursor = encoded;
	const uint8_t *end = encoded + size;
	uint16_t version = 1, payload_size = 0;
	if (size == ZONE_TOUCH_RESULT_BYTES)
	{
		if (!read_le(&cursor, end, &version) || !read_le(&cursor, end, &payload_size) ||
		    payload_size > static_cast<size_t>(end - cursor))
			return false;
	}
	else
	{
		// Legacy v1 used a compact payload padded to 88 bytes.
		uint16_t group = static_cast<uint16_t>(encoded[16] | (encoded[17] << 8));
		if (group > ZONE_TOUCH_MAX_PARTICIPANTS)
			return false;
		payload_size = static_cast<uint16_t>(25 + 4 * group);
	}
	critical_command command = {};
	command.type = critical_command_type::zone;
	command.payload_version = version;
	command.payload.assign(cursor, cursor + payload_size);
	cursor += payload_size;
	// Recover canonical keys from the prefix before validating the full command.
	const uint8_t *key_cursor = command.payload.data();
	const uint8_t *key_end = key_cursor + command.payload.size();
	uint32_t zone = 0, toucher = 0, stamp = 0;
	uint16_t group = 0;
	if (!read_le(&key_cursor, key_end, &zone) || !read_le(&key_cursor, key_end, &toucher) ||
	    !read_le(&key_cursor, key_end, &stamp) || !read_le(&key_cursor, key_end, &stamp) ||
	    !read_le(&key_cursor, key_end, &group) || group > ZONE_TOUCH_MAX_PARTICIPANTS)
		return false;
	for (size_t i = 0; i < group; ++i)
	{
		uint32_t pid = 0;
		if (!read_le(&key_cursor, key_end, &pid))
			return false;
		command.keys.push_back({ critical_entity_type::player, pid });
	}
	uint64_t uid = 0;
	uint32_t value = 0;
	uint16_t delta = 0;
	uint8_t reset = 0, record = 1;
	if (!read_le(&key_cursor, key_end, &value) || !read_le(&key_cursor, key_end, &delta) ||
	    !read_le(&key_cursor, key_end, &reset))
		return false;
	if (version == ZONE_TOUCH_PAYLOAD_VERSION &&
	    (!read_le(&key_cursor, key_end, &uid) || !read_le(&key_cursor, key_end, &value) ||
	     !read_le(&key_cursor, key_end, &record)))
		return false;
	command.keys.push_back(record ? critical_entity_key{ critical_entity_type::zone, zone } :
					critical_entity_key{ critical_entity_type::item, uid });
	std::sort(command.keys.begin(), command.keys.end(), critical_entity_key_less);
	if (!zone_touch_command_decode_payload(command, result))
		return false;
	if (size == ZONE_TOUCH_RESULT_BYTES)
	{
		for (size_t i = 0; i < result->group_size; ++i)
			if (!read_le(&cursor, end, &result->balances[i]) ||
			    !read_le(&cursor, end, &result->revisions[i]))
				return false;
		uint8_t recovered = 0;
		if (!read_le(&cursor, end, &recovered) || recovered > 1)
			return false;
		result->recovered_claim = recovered;
	}
	return std::all_of(cursor, end, [](uint8_t byte) { return byte == 0; });
}
