#include "item_transfer_command.h"

#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <utility>

namespace
{
constexpr size_t FROM_OFFSET = 0;
constexpr size_t TO_OFFSET = 17;
constexpr size_t REASON_OFFSET = 34;
constexpr size_t COUNT_OFFSET = 36;
constexpr size_t REASON_ID_OFFSET = 40;
constexpr size_t FROM_REVISION_OFFSET = 48;
constexpr size_t TO_REVISION_OFFSET = 56;
constexpr size_t SELECTED_ITEM_OFFSET = 64;
constexpr size_t TARGET_ROOT_OFFSET = 72;
constexpr size_t TARGET_PARENT_OFFSET = 80;
constexpr size_t TARGET_PARENT_REVISION_OFFSET = 88;

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

void encode_owner(uint8_t *output, const item_owner_identity &owner)
{
	output[0] = static_cast<uint8_t>(owner.type);
	put_u64(output + 1, owner.id);
	put_u64(output + 9, owner.context_id);
}

item_owner_identity decode_owner(const uint8_t *input)
{
	return { static_cast<item_owner_type>(input[0]), get_u64(input + 1), get_u64(input + 9) };
}

critical_entity_type entity_type_for_owner(item_owner_type type)
{
	switch (type)
	{
	case item_owner_type::player:
		return critical_entity_type::player;
	case item_owner_type::container:
		return critical_entity_type::item;
	case item_owner_type::corpse:
		return critical_entity_type::corpse;
	case item_owner_type::locker:
		return critical_entity_type::locker;
	case item_owner_type::auction:
		return critical_entity_type::auction;
	case item_owner_type::room:
		return critical_entity_type::room;
	case item_owner_type::shopkeeper:
		return critical_entity_type::shopkeeper;
	default:
		return critical_entity_type::system;
	}
}

bool valid_reason(item_transfer_reason reason)
{
	return reason > item_transfer_reason::unknown && reason <= item_transfer_reason::shop_sell;
}

bool validate_payload(const item_transfer_payload &payload)
{
	if (!item_owner_identity_valid(payload.from_owner) ||
	    !item_owner_identity_valid(payload.to_owner) || !valid_reason(payload.reason) ||
	    !payload.item_count || payload.item_count > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	const bool creation = payload.from_owner.type == item_owner_type::system;
	const bool destruction = payload.to_owner.type == item_owner_type::destruction;
	if (payload.to_owner.type == item_owner_type::system ||
	    payload.from_owner.type == item_owner_type::destruction ||
	    (payload.reason == item_transfer_reason::creation) != creation ||
	    (payload.reason == item_transfer_reason::destruction) != destruction ||
	    ((creation || destruction) &&
	     item_owner_identity_equal(payload.from_owner, payload.to_owner)))
		return false;
	const uint64_t source_root = payload.items[0].root_item_uid;
	const uint64_t selected = payload.selected_item_uid ? payload.selected_item_uid :
							      source_root;
	const uint64_t target_root = payload.target_root_item_uid ? payload.target_root_item_uid :
								    selected;
	if (!source_root || !selected || !target_root ||
	    (!payload.target_parent_item_uid && target_root != selected))
		return false;
	bool found_selected = false;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const item_transfer_entry &entry = payload.items[index];
		if (!entry.item_uid || entry.root_item_uid != source_root || entry.vnum <= 0 ||
		    entry.expected_state !=
			    (creation ? item_custody_state::absent : item_custody_state::active) ||
		    (creation && entry.expected_item_revision != ITEM_TRANSFER_ABSENT_REVISION))
			return false;
		if (index && payload.items[index - 1].item_uid >= entry.item_uid)
			return false;
		if (entry.item_uid == payload.target_parent_item_uid)
			return false;
		if (entry.item_uid == selected)
		{
			if (found_selected)
				return false;
			found_selected = true;
		}
		else if (!entry.parent_item_uid)
			return false;
	}
	if (!found_selected || (creation && selected != source_root))
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		if (payload.items[index].item_uid == selected)
			continue;
		uint64_t ancestor_uid = payload.items[index].parent_item_uid;
		bool reaches_root = false;
		for (size_t depth = 0; depth < payload.item_count; ++depth)
		{
			if (ancestor_uid == selected)
			{
				reaches_root = true;
				break;
			}
			auto parent = std::find_if(payload.items.begin(),
						   payload.items.begin() + payload.item_count,
						   [&](const item_transfer_entry &candidate)
						   { return candidate.item_uid == ancestor_uid; });
			if (parent == payload.items.begin() + payload.item_count)
				break;
			ancestor_uid = parent->parent_item_uid;
		}
		if (!reaches_root)
			return false;
	}
	return true;
}
} // namespace

bool item_owner_identity_valid(const item_owner_identity &owner)
{
	if (owner.type <= item_owner_type::unknown || owner.type > item_owner_type::shopkeeper)
		return false;
	if (owner.type == item_owner_type::system || owner.type == item_owner_type::destruction)
		return owner.id == 0 && owner.context_id == 0;
	return owner.id != 0;
}

bool item_owner_identity_equal(const item_owner_identity &left, const item_owner_identity &right)
{
	return left.type == right.type && left.id == right.id &&
	       left.context_id == right.context_id;
}

uint64_t item_corpse_owner_id(uint32_t player_pid, uint32_t corpse_save_id)
{
	if (!player_pid || !corpse_save_id)
		return 0;
	return (static_cast<uint64_t>(player_pid) << 32) | corpse_save_id;
}

uint64_t item_shopkeeper_owner_id(uint32_t shop_id)
{
	return static_cast<uint64_t>(shop_id) + 1;
}

bool item_owner_key(const item_owner_identity &owner, critical_entity_key *key)
{
	if (!key || !item_owner_identity_valid(owner))
		return false;
	if (owner.id && !owner.context_id)
	{
		*key = { entity_type_for_owner(owner.type), owner.id };
		return true;
	}
	std::array<uint8_t, 17> encoded = {};
	encode_owner(encoded.data(), owner);
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(encoded.data(), encoded.size(), digest.data());
	uint64_t identity = get_u64(digest.data());
	if (!identity)
		identity = 1;
	*key = { entity_type_for_owner(owner.type), identity };
	return true;
}

bool item_transfer_command_encode_payload(const item_transfer_payload &payload,
					  std::vector<uint8_t> *encoded)
{
	if (!encoded || !validate_payload(payload))
		return false;
	encoded->assign(ITEM_TRANSFER_PAYLOAD_BYTES, 0);
	encode_owner(encoded->data() + FROM_OFFSET, payload.from_owner);
	encode_owner(encoded->data() + TO_OFFSET, payload.to_owner);
	put_u16(encoded->data() + REASON_OFFSET, static_cast<uint16_t>(payload.reason));
	put_u16(encoded->data() + COUNT_OFFSET, payload.item_count);
	put_u64(encoded->data() + REASON_ID_OFFSET, static_cast<uint64_t>(payload.reason_id));
	put_u64(encoded->data() + FROM_REVISION_OFFSET, payload.expected_from_revision);
	put_u64(encoded->data() + TO_REVISION_OFFSET, payload.expected_to_revision);
	const uint64_t selected = payload.selected_item_uid ? payload.selected_item_uid :
							      payload.items[0].root_item_uid;
	put_u64(encoded->data() + SELECTED_ITEM_OFFSET, selected);
	put_u64(encoded->data() + TARGET_ROOT_OFFSET,
		payload.target_root_item_uid ? payload.target_root_item_uid : selected);
	put_u64(encoded->data() + TARGET_PARENT_OFFSET, payload.target_parent_item_uid);
	put_u64(encoded->data() + TARGET_PARENT_REVISION_OFFSET,
		payload.expected_target_parent_revision);
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const item_transfer_entry &entry = payload.items[index];
		uint8_t *output = encoded->data() + ITEM_TRANSFER_HEADER_BYTES +
				  index * ITEM_TRANSFER_ENTRY_BYTES;
		put_u64(output, entry.item_uid);
		put_u64(output + 8, entry.root_item_uid);
		put_u64(output + 16, entry.parent_item_uid);
		put_u64(output + 24, entry.expected_item_revision);
		put_u32(output + 32, static_cast<uint32_t>(entry.vnum));
		output[36] = static_cast<uint8_t>(entry.expected_state);
	}
	return true;
}

bool item_transfer_command_decode_payload(const critical_command &command,
					  item_transfer_payload *payload)
{
	if (!payload || command.type != critical_command_type::item_transfer ||
	    (command.payload_version != ITEM_TRANSFER_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_PREVIOUS_PAYLOAD_VERSION) ||
	    command.payload.size() != ITEM_TRANSFER_PAYLOAD_BYTES)
		return false;
	*payload = {};
	payload->from_owner = decode_owner(command.payload.data() + FROM_OFFSET);
	payload->to_owner = decode_owner(command.payload.data() + TO_OFFSET);
	payload->reason =
		static_cast<item_transfer_reason>(get_u16(command.payload.data() + REASON_OFFSET));
	payload->item_count = get_u16(command.payload.data() + COUNT_OFFSET);
	payload->reason_id =
		static_cast<int64_t>(get_u64(command.payload.data() + REASON_ID_OFFSET));
	payload->expected_from_revision = get_u64(command.payload.data() + FROM_REVISION_OFFSET);
	payload->expected_to_revision = get_u64(command.payload.data() + TO_REVISION_OFFSET);
	payload->selected_item_uid = get_u64(command.payload.data() + SELECTED_ITEM_OFFSET);
	payload->target_root_item_uid = get_u64(command.payload.data() + TARGET_ROOT_OFFSET);
	payload->target_parent_item_uid = get_u64(command.payload.data() + TARGET_PARENT_OFFSET);
	payload->expected_target_parent_revision =
		get_u64(command.payload.data() + TARGET_PARENT_REVISION_OFFSET);
	if (!payload->item_count || payload->item_count > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	for (size_t index = 0; index < ITEM_TRANSFER_MAX_ITEMS; ++index)
	{
		const uint8_t *input = command.payload.data() + ITEM_TRANSFER_HEADER_BYTES +
				       index * ITEM_TRANSFER_ENTRY_BYTES;
		if (index < payload->item_count)
			payload->items[index] = { get_u64(input),
						  get_u64(input + 8),
						  get_u64(input + 16),
						  get_u64(input + 24),
						  static_cast<int32_t>(get_u32(input + 32)),
						  static_cast<item_custody_state>(input[36]) };
		else
			for (size_t byte = 0; byte < ITEM_TRANSFER_ENTRY_BYTES; ++byte)
				if (input[byte])
					return false;
		if (input[37] || input[38] || input[39])
			return false;
	}
	if (!validate_payload(*payload) ||
	    (command.payload_version == ITEM_TRANSFER_PREVIOUS_PAYLOAD_VERSION &&
	     payload->reason > item_transfer_reason::auction_claim) ||
	    command.expected_revisions.size() != command.keys.size())
		return false;
	critical_command expected = {};
	if (!item_transfer_command_build(&expected, command.operation_id, *payload,
					 command.source_site, command.deadline_class))
		return false;
	return std::equal(command.keys.begin(), command.keys.end(), expected.keys.begin(),
			  [](const critical_entity_key &left, const critical_entity_key &right)
			  { return critical_entity_key_equal(left, right); }) &&
	       std::equal(command.expected_revisions.begin(), command.expected_revisions.end(),
			  expected.expected_revisions.begin(),
			  [](const critical_expected_revision &left,
			     const critical_expected_revision &right) {
				  return critical_entity_key_equal(left.key, right.key) &&
					 left.revision == right.revision;
			  });
}

bool item_transfer_command_encode_result(const item_transfer_result &result,
					 std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> *encoded)
{
	if (!encoded || !result.root_item_uid || !result.item_count ||
	    result.item_count > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	encoded->fill(0);
	put_u64(encoded->data(), result.root_item_uid);
	put_u16(encoded->data() + 8, result.item_count);
	put_u64(encoded->data() + 16, result.from_owner_revision);
	put_u64(encoded->data() + 24, result.to_owner_revision);
	put_u64(encoded->data() + 32, result.max_item_revision);
	return true;
}

bool item_transfer_command_decode_result(const uint8_t *encoded, size_t size,
					 item_transfer_result *result)
{
	if (!encoded || size != ITEM_TRANSFER_RESULT_BYTES || !result || encoded[10] ||
	    encoded[11] || encoded[12] || encoded[13] || encoded[14] || encoded[15])
		return false;
	*result = { get_u64(encoded), get_u16(encoded + 8), get_u64(encoded + 16),
		    get_u64(encoded + 24), get_u64(encoded + 32) };
	return result->root_item_uid && result->item_count &&
	       result->item_count <= ITEM_TRANSFER_MAX_ITEMS;
}

bool item_transfer_command_build(critical_command *command, critical_operation_id operation_id,
				 const item_transfer_payload &payload,
				 critical_source_site source_site,
				 critical_deadline_class deadline_class)
{
	if (!command || critical_operation_id_is_zero(operation_id))
		return false;
	std::vector<uint8_t> encoded;
	critical_entity_key from_key = {}, to_key = {};
	if (!item_transfer_command_encode_payload(payload, &encoded) ||
	    !item_owner_key(payload.from_owner, &from_key) ||
	    !item_owner_key(payload.to_owner, &to_key))
		return false;
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::item_transfer,
		     .payload_version = ITEM_TRANSFER_PAYLOAD_VERSION,
		     .source_site = source_site,
		     .deadline_class = deadline_class,
		     .accepted_at_usec = 0,
		     .keys = { from_key, to_key },
		     .expected_revisions = { { from_key, payload.expected_from_revision },
					     { to_key, payload.expected_to_revision } },
		     .payload = std::move(encoded) };
	if (item_owner_identity_equal(payload.from_owner, payload.to_owner))
	{
		if (payload.expected_from_revision != payload.expected_to_revision)
			return false;
		command->keys.pop_back();
		command->expected_revisions.pop_back();
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		critical_entity_key item_key = { critical_entity_type::item,
						 payload.items[index].item_uid };
		command->keys.push_back(item_key);
		command->expected_revisions.push_back(
			{ item_key, payload.items[index].expected_item_revision });
	}
	if (payload.target_parent_item_uid)
	{
		critical_entity_key parent_key = { critical_entity_type::item,
						   payload.target_parent_item_uid };
		command->keys.push_back(parent_key);
		command->expected_revisions.push_back(
			{ parent_key, payload.expected_target_parent_revision });
	}
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	if (std::adjacent_find(command->keys.begin(), command->keys.end(),
			       critical_entity_key_equal) != command->keys.end())
		return false;
	std::sort(command->expected_revisions.begin(), command->expected_revisions.end(),
		  [](const critical_expected_revision &left,
		     const critical_expected_revision &right)
		  { return critical_entity_key_less(left.key, right.key); });
	return true;
}
