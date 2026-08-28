#include "corpse_lifecycle_command.h"

#include "item_transfer_command.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>

namespace
{
constexpr size_t fixed_payload_bytes = 80;
constexpr size_t owner_pid_offset = 8;
constexpr size_t save_id_offset = 12;
constexpr size_t expected_revision_offset = 16;
constexpr size_t room_offset = 24;
constexpr size_t weight_offset = 28;
constexpr size_t values_offset = 32;
constexpr size_t money_offset = 64;

template <typename T> void put_number(uint8_t *output, T value)
{
	using U = std::make_unsigned_t<T>;
	U bits = static_cast<U>(value);
	for (size_t index = 0; index < sizeof(T); ++index)
	{
		output[index] = static_cast<uint8_t>(bits & 0xff);
		bits >>= 8;
	}
}

template <typename T> T get_number(const uint8_t *input)
{
	using U = std::make_unsigned_t<T>;
	U bits = 0;
	for (size_t index = 0; index < sizeof(T); ++index)
		bits |= static_cast<U>(input[index]) << (index * 8);
	return static_cast<T>(bits);
}

bool valid_text(const std::string &value, size_t maximum, bool required)
{
	if ((required && value.empty()) || value.size() > maximum)
		return false;
	return std::all_of(value.begin(), value.end(), [](unsigned char character)
			   { return character >= 0x20 && character != 0x7f; });
}

bool append_text(std::vector<uint8_t> *output, const std::string &value)
{
	if (!output || value.size() > UINT32_MAX ||
	    output->size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - sizeof(uint32_t) ||
	    value.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - output->size() - sizeof(uint32_t))
		return false;
	try
	{
		const size_t offset = output->size();
		output->resize(offset + sizeof(uint32_t));
		put_number<uint32_t>(output->data() + offset, static_cast<uint32_t>(value.size()));
		output->insert(output->end(), value.begin(), value.end());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool read_text(const uint8_t *input, size_t size, size_t *offset, size_t maximum,
	       std::string *value)
{
	if (!input || !offset || !value || *offset > size || size - *offset < sizeof(uint32_t))
		return false;
	const uint32_t length = get_number<uint32_t>(input + *offset);
	*offset += sizeof(uint32_t);
	if (length > maximum || *offset > size || size - *offset < length)
		return false;
	try
	{
		value->assign(reinterpret_cast<const char *>(input + *offset), length);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*offset += length;
	return true;
}

bool valid_payload(const corpse_lifecycle_payload &payload)
{
	if ((payload.action != corpse_lifecycle_action::upsert &&
	     payload.action != corpse_lifecycle_action::remove) ||
	    !payload.owner_pid || payload.owner_pid > INT32_MAX || !payload.save_id ||
	    payload.save_id > INT32_MAX ||
	    !valid_text(payload.owner_name, CORPSE_LIFECYCLE_OWNER_NAME_MAX_BYTES, true))
		return false;
	if (payload.action == corpse_lifecycle_action::remove)
		return payload.expected_corpse_revision && !payload.room_vnum && !payload.weight &&
		       std::all_of(payload.values.begin(), payload.values.end(),
				   [](int32_t value) { return value == 0; }) &&
		       std::all_of(payload.money.begin(), payload.money.end(),
				   [](int32_t value) { return value == 0; }) &&
		       payload.short_description.empty() && payload.description.empty() &&
		       payload.keywords.empty();
	return payload.room_vnum >= 0 &&
	       payload.values[3] == static_cast<int32_t>(payload.owner_pid) &&
	       payload.values[5] >= 0 && payload.values[5] <= 4 &&
	       payload.values[6] == static_cast<int32_t>(payload.save_id) &&
	       std::all_of(payload.money.begin(), payload.money.end(),
			   [](int32_t value) { return value >= 0; }) &&
	       valid_text(payload.short_description, CORPSE_LIFECYCLE_SHORT_DESCRIPTION_MAX_BYTES,
			  false) &&
	       valid_text(payload.description, CORPSE_LIFECYCLE_DESCRIPTION_MAX_BYTES, false) &&
	       valid_text(payload.keywords, CORPSE_LIFECYCLE_KEYWORDS_MAX_BYTES, false);
}

bool valid_result(const corpse_lifecycle_result &result)
{
	return result.owner_pid && result.save_id && result.catalog_revision &&
	       (result.action == corpse_lifecycle_action::upsert ?
			result.corpse_revision != 0 :
			result.action == corpse_lifecycle_action::remove &&
				!result.corpse_revision);
}
} // namespace

bool corpse_lifecycle_command_encode_payload(const corpse_lifecycle_payload &payload,
					     std::vector<uint8_t> *encoded)
{
	if (!encoded || !valid_payload(payload))
		return false;
	try
	{
		encoded->assign(fixed_payload_bytes, 0);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	(*encoded)[0] = static_cast<uint8_t>(payload.action);
	put_number<uint32_t>(encoded->data() + owner_pid_offset, payload.owner_pid);
	put_number<uint32_t>(encoded->data() + save_id_offset, payload.save_id);
	put_number<uint64_t>(encoded->data() + expected_revision_offset,
			     payload.expected_corpse_revision);
	put_number<int32_t>(encoded->data() + room_offset, payload.room_vnum);
	put_number<int32_t>(encoded->data() + weight_offset, payload.weight);
	for (size_t index = 0; index < payload.values.size(); ++index)
		put_number<int32_t>(encoded->data() + values_offset + index * sizeof(int32_t),
				    payload.values[index]);
	for (size_t index = 0; index < payload.money.size(); ++index)
		put_number<int32_t>(encoded->data() + money_offset + index * sizeof(int32_t),
				    payload.money[index]);
	return append_text(encoded, payload.owner_name) &&
	       append_text(encoded, payload.short_description) &&
	       append_text(encoded, payload.description) && append_text(encoded, payload.keywords);
}

bool corpse_lifecycle_command_decode_payload(const critical_command &command,
					     corpse_lifecycle_payload *payload)
{
	if (!payload || command.type != critical_command_type::corpse_lifecycle ||
	    command.payload_version != CORPSE_LIFECYCLE_PAYLOAD_VERSION ||
	    command.payload.size() < fixed_payload_bytes + sizeof(uint32_t) * 4)
		return false;
	const uint8_t *input = command.payload.data();
	for (size_t index = 1; index < owner_pid_offset; ++index)
		if (input[index])
			return false;
	*payload = {};
	payload->action = static_cast<corpse_lifecycle_action>(input[0]);
	payload->owner_pid = get_number<uint32_t>(input + owner_pid_offset);
	payload->save_id = get_number<uint32_t>(input + save_id_offset);
	payload->expected_corpse_revision = get_number<uint64_t>(input + expected_revision_offset);
	payload->room_vnum = get_number<int32_t>(input + room_offset);
	payload->weight = get_number<int32_t>(input + weight_offset);
	for (size_t index = 0; index < payload->values.size(); ++index)
		payload->values[index] =
			get_number<int32_t>(input + values_offset + index * sizeof(int32_t));
	for (size_t index = 0; index < payload->money.size(); ++index)
		payload->money[index] =
			get_number<int32_t>(input + money_offset + index * sizeof(int32_t));
	size_t offset = fixed_payload_bytes;
	if (!read_text(input, command.payload.size(), &offset,
		       CORPSE_LIFECYCLE_OWNER_NAME_MAX_BYTES, &payload->owner_name) ||
	    !read_text(input, command.payload.size(), &offset,
		       CORPSE_LIFECYCLE_SHORT_DESCRIPTION_MAX_BYTES, &payload->short_description) ||
	    !read_text(input, command.payload.size(), &offset,
		       CORPSE_LIFECYCLE_DESCRIPTION_MAX_BYTES, &payload->description) ||
	    !read_text(input, command.payload.size(), &offset, CORPSE_LIFECYCLE_KEYWORDS_MAX_BYTES,
		       &payload->keywords) ||
	    offset != command.payload.size() || !valid_payload(*payload))
		return false;
	const critical_entity_key expected_key = { critical_entity_type::corpse,
						   item_corpse_owner_id(payload->owner_pid,
									payload->save_id) };
	return command.keys.size() == 1 && command.expected_revisions.size() == 1 &&
	       critical_entity_key_equal(command.keys[0], expected_key) &&
	       critical_entity_key_equal(command.expected_revisions[0].key, expected_key) &&
	       command.expected_revisions[0].revision == payload->expected_corpse_revision;
}

bool corpse_lifecycle_command_encode_result(
	const corpse_lifecycle_result &result,
	std::array<uint8_t, CORPSE_LIFECYCLE_RESULT_BYTES> *encoded)
{
	if (!encoded || !valid_result(result))
		return false;
	encoded->fill(0);
	put_number<uint32_t>(encoded->data(), result.owner_pid);
	put_number<uint32_t>(encoded->data() + 4, result.save_id);
	(*encoded)[8] = static_cast<uint8_t>(result.action);
	put_number<uint64_t>(encoded->data() + 16, result.corpse_revision);
	put_number<uint64_t>(encoded->data() + 24, result.catalog_revision);
	return true;
}

bool corpse_lifecycle_command_decode_result(const uint8_t *encoded, size_t encoded_size,
					    corpse_lifecycle_result *result)
{
	if (!encoded || !result || encoded_size != CORPSE_LIFECYCLE_RESULT_BYTES)
		return false;
	for (size_t index = 9; index < 16; ++index)
		if (encoded[index])
			return false;
	*result = { get_number<uint32_t>(encoded), get_number<uint32_t>(encoded + 4),
		    static_cast<corpse_lifecycle_action>(encoded[8]),
		    get_number<uint64_t>(encoded + 16), get_number<uint64_t>(encoded + 24) };
	return valid_result(*result);
}

bool corpse_lifecycle_command_build(critical_command *command, critical_operation_id operation_id,
				    const corpse_lifecycle_payload &payload,
				    critical_source_site source_site,
				    critical_deadline_class deadline_class)
{
	if (!command || critical_operation_id_is_zero(operation_id))
		return false;
	std::vector<uint8_t> encoded;
	if (!corpse_lifecycle_command_encode_payload(payload, &encoded))
		return false;
	const critical_entity_key key = { critical_entity_type::corpse,
					  item_corpse_owner_id(payload.owner_pid,
							       payload.save_id) };
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::corpse_lifecycle,
		     .payload_version = CORPSE_LIFECYCLE_PAYLOAD_VERSION,
		     .source_site = source_site,
		     .deadline_class = deadline_class,
		     .accepted_at_usec = 0,
		     .keys = { key },
		     .expected_revisions = { { key, payload.expected_corpse_revision } },
		     .payload = std::move(encoded) };
	return true;
}
