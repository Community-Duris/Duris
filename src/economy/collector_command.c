#include "economy/collector_command.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

namespace
{
constexpr size_t COLLECTOR_RESULT_RECORD_OFFSET = 112;
constexpr size_t COLLECTOR_RESULT_MATERIALIZED_ITEM_OFFSET =
	COLLECTOR_RESULT_RECORD_OFFSET + collector::encoded_record_bytes;

template <typename T> void append_le(std::vector<uint8_t> *output, T value)
{
	using unsigned_type = std::make_unsigned_t<T>;
	const unsigned_type encoded = static_cast<unsigned_type>(value);
	for (size_t byte = 0; byte < sizeof(T); ++byte)
		output->push_back(static_cast<uint8_t>(encoded >> (byte * 8)));
}

template <typename T> bool read_le(const uint8_t **cursor, const uint8_t *end, T *value)
{
	if (!cursor || !*cursor || !value || static_cast<size_t>(end - *cursor) < sizeof(T))
		return false;
	using unsigned_type = std::make_unsigned_t<T>;
	unsigned_type decoded = 0;
	for (size_t byte = 0; byte < sizeof(T); ++byte)
		decoded |= static_cast<unsigned_type>((*cursor)[byte]) << (byte * 8);
	*cursor += sizeof(T);
	*value = static_cast<T>(decoded);
	return true;
}

void put_u64(uint8_t *output, uint64_t value)
{
	for (size_t byte = 0; byte < sizeof(value); ++byte)
		output[byte] = static_cast<uint8_t>(value >> (byte * 8));
}

void put_u32(uint8_t *output, uint32_t value)
{
	for (size_t byte = 0; byte < sizeof(value); ++byte)
		output[byte] = static_cast<uint8_t>(value >> (byte * 8));
}

uint64_t get_u64(const uint8_t *input)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < sizeof(value); ++byte)
		value |= static_cast<uint64_t>(input[byte]) << (byte * 8);
	return value;
}

uint32_t get_u32(const uint8_t *input)
{
	uint32_t value = 0;
	for (size_t byte = 0; byte < sizeof(value); ++byte)
		value |= static_cast<uint32_t>(input[byte]) << (byte * 8);
	return value;
}

bool valid_action(collector_action action)
{
	return action > collector_action::unknown && action <= collector_action::hint_ack;
}

bool valid_cancel_reason(collector::reason why)
{
	switch (why)
	{
	case collector::reason::claimed:
	case collector::reason::destroyed:
	case collector::reason::quarantined:
	case collector::reason::excluded:
	case collector::reason::character_deleted:
	case collector::reason::season_reset:
		return true;
	case collector::reason::none:
	case collector::reason::holding_elapsed:
		return false;
	}
	return false;
}

bool empty_owner(const item_owner_identity &owner)
{
	return owner.type == item_owner_type::unknown && !owner.id && !owner.context_id;
}

bool collector_owner(const item_owner_identity &owner, uint64_t listing)
{
	return owner.type == item_owner_type::collector &&
	       owner.id == item_collector_owner_id(listing) && !owner.context_id;
}

bool destruction_owner(const item_owner_identity &owner)
{
	return owner.type == item_owner_type::destruction && !owner.id && !owner.context_id;
}

bool quarantine_owner(const item_owner_identity &owner)
{
	return owner.type == item_owner_type::system && !owner.id && !owner.context_id;
}

bool collection_source_owner(const item_owner_identity &owner)
{
	if (owner.type == item_owner_type::room)
		return owner.id && !owner.context_id;
	if (owner.type != item_owner_type::corpse || owner.context_id)
		return false;
	const uint64_t owner_pid = owner.id >> 32;
	const uint64_t save_id = static_cast<uint32_t>(owner.id);
	return owner_pid && owner_pid <= INT32_MAX && save_id && save_id <= INT32_MAX;
}

bool valid_name(const std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> &name)
{
	const size_t length = strnlen(name.data(), name.size());
	if (!length || length >= name.size())
		return false;
	for (size_t index = 0; index < length; ++index)
		if (static_cast<unsigned char>(name[index]) < 0x20)
			return false;
	return true;
}

bool empty_actor(const collector_command_payload &payload)
{
	return !payload.actor_pid && !payload.racewar && !payload.account_name[0] &&
	       !payload.expected_wallet_revision && !payload.expected_bank_revision &&
	       !payload.capacity_admitted;
}

const item_transfer_entry *find_item(const collector_command_payload &payload, uint64_t uid)
{
	auto found = std::lower_bound(payload.items.begin(),
				      payload.items.begin() + payload.item_count, uid,
				      [](const item_transfer_entry &entry, uint64_t sought)
				      { return entry.item_uid < sought; });
	return found != payload.items.begin() + payload.item_count && found->item_uid == uid ?
		       &*found :
		       nullptr;
}

bool valid_item(const item_transfer_entry &item)
{
	return item.item_uid && item.root_item_uid && item.expected_item_revision &&
	       item.expected_item_revision != std::numeric_limits<uint64_t>::max() &&
	       item.vnum > 0 && item.expected_state == item_custody_state::active;
}

bool valid_source_tree(const collector_command_payload &payload)
{
	if (!payload.item_count || payload.item_count > payload.items.size() ||
	    !payload.selected_item_uid)
		return false;
	const uint64_t root = payload.items[0].root_item_uid;
	bool selected_found = false;
	bool root_found = false;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &item = payload.items[index];
		if (!valid_item(item) || item.root_item_uid != root ||
		    (index && payload.items[index - 1].item_uid >= item.item_uid))
			return false;
		selected_found = selected_found || item.item_uid == payload.selected_item_uid;
		if (!item.parent_item_uid)
		{
			if (root_found || item.item_uid != root)
				return false;
			root_found = true;
		}
		else if (item.item_uid == root || !find_item(payload, item.parent_item_uid))
			return false;
	}
	if (!selected_found || !root_found)
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const item_transfer_entry *item = &payload.items[index];
		for (size_t depth = 0; depth < payload.item_count; ++depth)
		{
			if (item->item_uid == root)
				break;
			item = find_item(payload, item->parent_item_uid);
			if (!item)
				return false;
		}
		if (!item || item->item_uid != root)
			return false;
	}
	return true;
}

bool valid_held_item(const collector_command_payload &payload)
{
	return payload.item_count == 1 && valid_item(payload.items[0]) &&
	       payload.items[0].item_uid == payload.selected_item_uid &&
	       payload.items[0].root_item_uid == payload.selected_item_uid &&
	       !payload.items[0].parent_item_uid;
}

bool metadata_only(const collector_command_payload &payload)
{
	return empty_actor(payload) && empty_owner(payload.from_owner) &&
	       empty_owner(payload.to_owner) && !payload.expected_from_owner_revision &&
	       !payload.expected_to_owner_revision && !payload.selected_item_uid &&
	       !payload.item_count && !payload.item_blob_size &&
	       payload.target_state == item_custody_state::absent;
}

bool valid_payload(const collector_command_payload &payload)
{
	if (!valid_action(payload.action) || !payload.listing ||
	    !payload.expected_listing_revision ||
	    payload.expected_listing_revision == std::numeric_limits<uint64_t>::max() ||
	    !payload.observed_at || payload.racewar > 4 ||
	    payload.item_count > payload.items.size() ||
	    payload.item_blob_size > payload.item_blob.size())
		return false;
	if (payload.action == collector_action::activate ||
	    payload.action == collector_action::pause ||
	    payload.action == collector_action::resume ||
	    payload.action == collector_action::hint ||
	    payload.action == collector_action::hint_ack)
		return payload.cancel_reason == collector::reason::none && metadata_only(payload);
	if (payload.action == collector_action::cancel && !payload.item_count)
		return valid_cancel_reason(payload.cancel_reason) && metadata_only(payload);
	if (!payload.selected_item_uid || !payload.item_count || !payload.item_blob_size ||
	    !item_owner_identity_valid(payload.from_owner) ||
	    !item_owner_identity_valid(payload.to_owner) ||
	    item_owner_identity_equal(payload.from_owner, payload.to_owner) ||
	    payload.expected_from_owner_revision == std::numeric_limits<uint64_t>::max() ||
	    payload.expected_to_owner_revision == std::numeric_limits<uint64_t>::max())
		return false;
	if (payload.action == collector_action::collect)
		return payload.cancel_reason == collector::reason::none && empty_actor(payload) &&
		       collection_source_owner(payload.from_owner) &&
		       collector_owner(payload.to_owner, payload.listing) &&
		       payload.target_state == item_custody_state::active &&
		       valid_source_tree(payload);
	if (payload.action == collector_action::purchase)
		return payload.cancel_reason == collector::reason::none && payload.actor_pid &&
		       payload.capacity_admitted && valid_name(payload.account_name) &&
		       collector_owner(payload.from_owner, payload.listing) &&
		       payload.to_owner.type == item_owner_type::player &&
		       payload.to_owner.id == payload.actor_pid && !payload.to_owner.context_id &&
		       payload.target_state == item_custody_state::active &&
		       valid_held_item(payload);
	if (payload.action == collector_action::expire)
		return payload.cancel_reason == collector::reason::none && empty_actor(payload) &&
		       collector_owner(payload.from_owner, payload.listing) &&
		       destruction_owner(payload.to_owner) &&
		       payload.target_state == item_custody_state::destroyed &&
		       valid_held_item(payload);
	if (payload.action != collector_action::cancel ||
	    !valid_cancel_reason(payload.cancel_reason) ||
	    payload.cancel_reason == collector::reason::claimed || !empty_actor(payload) ||
	    !collector_owner(payload.from_owner, payload.listing) || !valid_held_item(payload))
		return false;
	if (payload.cancel_reason == collector::reason::quarantined)
		return quarantine_owner(payload.to_owner) &&
		       payload.target_state == item_custody_state::quarantined;
	return destruction_owner(payload.to_owner) &&
	       payload.target_state == item_custody_state::destroyed;
}

void append_owner(std::vector<uint8_t> *encoded, const item_owner_identity &owner)
{
	append_le<uint8_t>(encoded, static_cast<uint8_t>(owner.type));
	append_le<uint64_t>(encoded, owner.id);
	append_le<uint64_t>(encoded, owner.context_id);
}

bool read_owner(const uint8_t **cursor, const uint8_t *end, item_owner_identity *owner)
{
	uint8_t type = 0;
	if (!read_le(cursor, end, &type) || !read_le(cursor, end, &owner->id) ||
	    !read_le(cursor, end, &owner->context_id))
		return false;
	owner->type = static_cast<item_owner_type>(type);
	return true;
}

bool append_name(std::vector<uint8_t> *encoded,
		 const std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> &name)
{
	const size_t length = strnlen(name.data(), name.size());
	if (length >= name.size())
		return false;
	append_le<uint16_t>(encoded, static_cast<uint16_t>(length));
	encoded->insert(encoded->end(), name.begin(), name.begin() + length);
	return true;
}

bool read_name(const uint8_t **cursor, const uint8_t *end,
	       std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> *name)
{
	uint16_t length = 0;
	if (!read_le(cursor, end, &length) || length >= name->size() ||
	    static_cast<size_t>(end - *cursor) < length)
		return false;
	name->fill(0);
	memcpy(name->data(), *cursor, length);
	*cursor += length;
	return true;
}

bool matching_fences(const critical_command &left, const critical_command &right)
{
	if (left.keys.size() != right.keys.size() ||
	    left.expected_revisions.size() != right.expected_revisions.size())
		return false;
	for (size_t index = 0; index < left.keys.size(); ++index)
		if (!critical_entity_key_equal(left.keys[index], right.keys[index]))
			return false;
	for (size_t index = 0; index < left.expected_revisions.size(); ++index)
		if (!critical_entity_key_equal(left.expected_revisions[index].key,
					       right.expected_revisions[index].key) ||
		    left.expected_revisions[index].revision !=
			    right.expected_revisions[index].revision)
			return false;
	return true;
}

bool result_state_matches(const collector_command_result &result)
{
	if (!result.record_present)
		return true;
	switch (result.action)
	{
	case collector_action::collect:
		return result.entry.status == collector::state::collected;
	case collector_action::activate:
		return result.entry.status == collector::state::available &&
		       !result.entry.holding_paused;
	case collector_action::purchase:
		return result.entry.status == collector::state::purchased;
	case collector_action::expire:
		return result.entry.status == collector::state::expired;
	case collector_action::cancel:
		return result.entry.status == collector::state::cancelled;
	case collector_action::pause:
		return result.entry.status == collector::state::available &&
		       result.entry.holding_paused;
	case collector_action::resume:
		return result.entry.status == collector::state::available &&
		       !result.entry.holding_paused;
	case collector_action::hint:
		return result.entry.status == collector::state::available &&
		       !result.entry.holding_paused;
	case collector_action::hint_ack:
		// A delivery acknowledgement may race with purchase or expiry. It only
		// publishes the current listing image; the hint state is the mutation.
		return collector::valid_record(result.entry);
	case collector_action::unknown:
		return false;
	}
	return false;
}

bool valid_result(const collector_command_result &result)
{
	if (!valid_action(result.action) ||
	    (result.record_present &&
	     (!result.catalog_revision || !collector::valid_record(result.entry) ||
	      !result_state_matches(result))) ||
	    (result.materialized_item_id &&
	     (!result.record_present || result.action != collector_action::purchase)))
		return false;
	for (int64_t amount : result.wallet.amount)
		if (amount < 0)
			return false;
	for (int64_t amount : result.bank.amount)
		if (amount < 0)
			return false;
	return true;
}
} // namespace

bool collector_command_encode_payload(const collector_command_payload &payload,
				      std::vector<uint8_t> *encoded)
{
	if (!encoded || !valid_payload(payload))
		return false;
	try
	{
		encoded->clear();
		encoded->reserve(128 + payload.item_count * 40 + payload.item_blob_size);
		append_le<uint8_t>(encoded, static_cast<uint8_t>(payload.action));
		append_le<uint8_t>(encoded, static_cast<uint8_t>(payload.cancel_reason));
		append_le<uint8_t>(encoded, static_cast<uint8_t>(payload.target_state));
		append_le<uint8_t>(encoded, payload.capacity_admitted ? 1 : 0);
		append_le<uint64_t>(encoded, payload.listing);
		append_le<uint64_t>(encoded, payload.expected_listing_revision);
		append_le<uint64_t>(encoded, payload.observed_at);
		append_le<uint32_t>(encoded, payload.actor_pid);
		append_le<uint8_t>(encoded, payload.racewar);
		append_le<uint64_t>(encoded, payload.expected_wallet_revision);
		append_le<uint64_t>(encoded, payload.expected_bank_revision);
		append_owner(encoded, payload.from_owner);
		append_owner(encoded, payload.to_owner);
		append_le<uint64_t>(encoded, payload.expected_from_owner_revision);
		append_le<uint64_t>(encoded, payload.expected_to_owner_revision);
		append_le<uint64_t>(encoded, payload.selected_item_uid);
		append_le<uint16_t>(encoded, payload.item_count);
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto &item = payload.items[index];
			append_le<uint64_t>(encoded, item.item_uid);
			append_le<uint64_t>(encoded, item.root_item_uid);
			append_le<uint64_t>(encoded, item.parent_item_uid);
			append_le<uint64_t>(encoded, item.expected_item_revision);
			append_le<int32_t>(encoded, item.vnum);
			append_le<uint8_t>(encoded, static_cast<uint8_t>(item.expected_state));
			append_le<uint8_t>(encoded, 0);
			append_le<uint8_t>(encoded, 0);
			append_le<uint8_t>(encoded, 0);
		}
		if (!append_name(encoded, payload.account_name))
			return false;
		append_le<uint32_t>(encoded, payload.item_blob_size);
		encoded->insert(encoded->end(), payload.item_blob.begin(),
				payload.item_blob.begin() + payload.item_blob_size);
	}
	catch (const std::bad_alloc &)
	{
		encoded->clear();
		return false;
	}
	return encoded->size() <= CRITICAL_COMMAND_MAX_PAYLOAD_BYTES;
}

bool collector_command_decode_payload(const critical_command &command,
				      collector_command_payload *payload)
{
	if (!payload || command.type != critical_command_type::collector ||
	    command.payload_version != COLLECTOR_COMMAND_PAYLOAD_VERSION)
		return false;
	*payload = {};
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	uint8_t action = 0, reason = 0, target_state = 0, capacity = 0;
	if (!read_le(&cursor, end, &action) || !read_le(&cursor, end, &reason) ||
	    !read_le(&cursor, end, &target_state) || !read_le(&cursor, end, &capacity) ||
	    capacity > 1 || !read_le(&cursor, end, &payload->listing) ||
	    !read_le(&cursor, end, &payload->expected_listing_revision) ||
	    !read_le(&cursor, end, &payload->observed_at) ||
	    !read_le(&cursor, end, &payload->actor_pid) ||
	    !read_le(&cursor, end, &payload->racewar) ||
	    !read_le(&cursor, end, &payload->expected_wallet_revision) ||
	    !read_le(&cursor, end, &payload->expected_bank_revision) ||
	    !read_owner(&cursor, end, &payload->from_owner) ||
	    !read_owner(&cursor, end, &payload->to_owner) ||
	    !read_le(&cursor, end, &payload->expected_from_owner_revision) ||
	    !read_le(&cursor, end, &payload->expected_to_owner_revision) ||
	    !read_le(&cursor, end, &payload->selected_item_uid) ||
	    !read_le(&cursor, end, &payload->item_count) ||
	    payload->item_count > payload->items.size())
		return false;
	payload->action = static_cast<collector_action>(action);
	payload->cancel_reason = static_cast<collector::reason>(reason);
	payload->target_state = static_cast<item_custody_state>(target_state);
	payload->capacity_admitted = capacity != 0;
	for (size_t index = 0; index < payload->item_count; ++index)
	{
		uint8_t state = 0, reserved[3] = {};
		auto &item = payload->items[index];
		if (!read_le(&cursor, end, &item.item_uid) ||
		    !read_le(&cursor, end, &item.root_item_uid) ||
		    !read_le(&cursor, end, &item.parent_item_uid) ||
		    !read_le(&cursor, end, &item.expected_item_revision) ||
		    !read_le(&cursor, end, &item.vnum) || !read_le(&cursor, end, &state) ||
		    !read_le(&cursor, end, &reserved[0]) || !read_le(&cursor, end, &reserved[1]) ||
		    !read_le(&cursor, end, &reserved[2]) || reserved[0] || reserved[1] ||
		    reserved[2])
			return false;
		item.expected_state = static_cast<item_custody_state>(state);
	}
	if (!read_name(&cursor, end, &payload->account_name) ||
	    !read_le(&cursor, end, &payload->item_blob_size) ||
	    payload->item_blob_size > payload->item_blob.size() ||
	    static_cast<size_t>(end - cursor) != payload->item_blob_size)
		return false;
	memcpy(payload->item_blob.data(), cursor, payload->item_blob_size);
	if (!valid_payload(*payload))
		return false;
	critical_command expected = {};
	if (!collector_command_build(&expected, command.operation_id, *payload, command.source_site,
				     command.deadline_class) ||
	    !matching_fences(expected, command))
		return false;
	return true;
}

bool collector_command_encode_result(const collector_command_result &result,
				     std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> *encoded)
{
	if (!encoded || !valid_result(result))
		return false;
	encoded->fill(0);
	(*encoded)[0] = static_cast<uint8_t>(result.action);
	(*encoded)[1] = COLLECTOR_COMMAND_RESULT_VERSION;
	(*encoded)[2] = result.record_present ? 1 : 0;
	put_u64(encoded->data() + 8, result.catalog_revision);
	put_u64(encoded->data() + 16, result.from_owner_revision);
	put_u64(encoded->data() + 24, result.to_owner_revision);
	put_u64(encoded->data() + 32, result.wallet_revision);
	put_u64(encoded->data() + 40, result.bank_revision);
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
	{
		put_u64(encoded->data() + 48 + index * 8,
			static_cast<uint64_t>(result.wallet.amount[index]));
		put_u64(encoded->data() + 80 + index * 8,
			static_cast<uint64_t>(result.bank.amount[index]));
	}
	if (result.record_present)
	{
		std::array<uint8_t, collector::encoded_record_bytes> record = {};
		if (collector::record_encode(result.entry, &record) != collector::codec_result::ok)
			return false;
		std::copy(record.begin(), record.end(),
			  encoded->begin() + COLLECTOR_RESULT_RECORD_OFFSET);
	}
	put_u32(encoded->data() + COLLECTOR_RESULT_MATERIALIZED_ITEM_OFFSET,
		result.materialized_item_id);
	return true;
}

bool collector_command_decode_result(const uint8_t *encoded, size_t encoded_size,
				     collector_command_result *result)
{
	if (!encoded || encoded_size != COLLECTOR_COMMAND_RESULT_BYTES || !result ||
	    (encoded[1] != COLLECTOR_COMMAND_RESULT_VERSION &&
	     encoded[1] != COLLECTOR_COMMAND_PREVIOUS_RESULT_VERSION) ||
	    encoded[2] > 1 || encoded[3] || encoded[4] || encoded[5] || encoded[6] || encoded[7])
		return false;
	*result = {};
	result->action = static_cast<collector_action>(encoded[0]);
	result->record_present = encoded[2] != 0;
	result->catalog_revision = get_u64(encoded + 8);
	result->from_owner_revision = get_u64(encoded + 16);
	result->to_owner_revision = get_u64(encoded + 24);
	result->wallet_revision = get_u64(encoded + 32);
	result->bank_revision = get_u64(encoded + 40);
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
	{
		result->wallet.amount[index] =
			static_cast<int64_t>(get_u64(encoded + 48 + index * 8));
		result->bank.amount[index] =
			static_cast<int64_t>(get_u64(encoded + 80 + index * 8));
	}
	if (result->record_present)
	{
		if (collector::record_decode(encoded + COLLECTOR_RESULT_RECORD_OFFSET,
					     collector::encoded_record_bytes,
					     &result->entry) != collector::codec_result::ok)
			return false;
	}
	else
		for (size_t offset = COLLECTOR_RESULT_RECORD_OFFSET;
		     offset < COLLECTOR_RESULT_RECORD_OFFSET + collector::encoded_record_bytes;
		     ++offset)
			if (encoded[offset])
				return false;
	if (encoded[1] == COLLECTOR_COMMAND_RESULT_VERSION)
		result->materialized_item_id =
			get_u32(encoded + COLLECTOR_RESULT_MATERIALIZED_ITEM_OFFSET);
	for (size_t offset =
		     COLLECTOR_RESULT_MATERIALIZED_ITEM_OFFSET +
		     (encoded[1] == COLLECTOR_COMMAND_RESULT_VERSION ? sizeof(uint32_t) : 0);
	     offset < encoded_size; ++offset)
		if (encoded[offset])
			return false;
	return valid_result(*result);
}

bool collector_command_build(critical_command *command, critical_operation_id operation_id,
			     const collector_command_payload &payload,
			     critical_source_site source_site,
			     critical_deadline_class deadline_class)
{
	if (!command || critical_operation_id_is_zero(operation_id) || !valid_payload(payload))
		return false;
	std::vector<uint8_t> encoded;
	if (!collector_command_encode_payload(payload, &encoded))
		return false;
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::collector,
		     .payload_version = COLLECTOR_COMMAND_PAYLOAD_VERSION,
		     .source_site = source_site,
		     .deadline_class = deadline_class,
		     .accepted_at_usec = 0,
		     .keys = {},
		     .expected_revisions = {},
		     .payload = std::move(encoded) };
	auto add_key = [&](critical_entity_key key)
	{
		if (std::find_if(command->keys.begin(), command->keys.end(),
				 [&](const critical_entity_key &candidate) {
					 return critical_entity_key_equal(candidate, key);
				 }) == command->keys.end())
			command->keys.push_back(key);
	};
	auto add_fence = [&](critical_entity_key key, uint64_t revision)
	{
		add_key(key);
		if (std::find_if(command->expected_revisions.begin(),
				 command->expected_revisions.end(),
				 [&](const critical_expected_revision &candidate) {
					 return critical_entity_key_equal(candidate.key, key);
				 }) == command->expected_revisions.end())
			command->expected_revisions.push_back({ key, revision });
	};
	try
	{
		const critical_entity_key listing = { critical_entity_type::collector,
						      payload.listing };
		add_fence(listing, payload.expected_listing_revision);
		if (payload.action == collector_action::purchase)
		{
			const critical_entity_key player = { critical_entity_type::player,
							     payload.actor_pid };
			critical_entity_key account = {};
			if (!currency_account_key(payload.account_name.data(), payload.racewar,
						  &account))
				return false;
			add_fence(player, payload.expected_wallet_revision);
			add_fence(account, payload.expected_bank_revision);
		}
		if (payload.item_count)
		{
			critical_entity_key from = {}, to = {};
			if (!item_owner_key(payload.from_owner, &from) ||
			    !item_owner_key(payload.to_owner, &to))
				return false;
			add_fence(from, payload.expected_from_owner_revision);
			add_fence(to, payload.expected_to_owner_revision);
			for (size_t index = 0; index < payload.item_count; ++index)
				add_fence({ critical_entity_type::item,
					    payload.items[index].item_uid },
					  payload.items[index].expected_item_revision);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	std::sort(command->expected_revisions.begin(), command->expected_revisions.end(),
		  [](const critical_expected_revision &left,
		     const critical_expected_revision &right)
		  { return critical_entity_key_less(left.key, right.key); });
	return command->keys.size() <= CRITICAL_COMMAND_MAX_KEYS &&
	       command->expected_revisions.size() <= CRITICAL_COMMAND_MAX_KEYS;
}
