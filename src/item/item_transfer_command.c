#include "item/item_transfer_command.h"

#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>
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
constexpr uint32_t CORPSE_CONTEXT_VERSION = 1;
constexpr size_t CORPSE_PID_VALUE_INDEX = 3;
constexpr size_t CORPSE_RACEWAR_VALUE_INDEX = 5;
constexpr size_t CORPSE_SAVE_ID_VALUE_INDEX = 6;

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

bool append_u32(std::vector<uint8_t> *output, uint32_t value)
{
	if (!output)
		return false;
	try
	{
		const size_t offset = output->size();
		output->resize(offset + sizeof(value));
		put_u32(output->data() + offset, value);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool append_text(std::vector<uint8_t> *output, const std::string &value)
{
	if (!output || value.size() > UINT32_MAX ||
	    !append_u32(output, static_cast<uint32_t>(value.size())))
		return false;
	try
	{
		output->insert(output->end(), value.begin(), value.end());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool read_u32(const uint8_t *input, size_t size, size_t *offset, uint32_t *value)
{
	if (!input || !offset || !value || *offset > size || size - *offset < sizeof(*value))
		return false;
	*value = get_u32(input + *offset);
	*offset += sizeof(*value);
	return true;
}

bool read_text(const uint8_t *input, size_t size, size_t *offset, size_t maximum,
	       std::string *value)
{
	uint32_t length = 0;
	if (!value || !read_u32(input, size, offset, &length) || length > maximum ||
	    *offset > size || size - *offset < length)
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

bool valid_text(const std::string &value, size_t maximum, bool required)
{
	if ((required && value.empty()) || value.size() > maximum)
		return false;
	return std::all_of(value.begin(), value.end(), [](unsigned char character)
			   { return character >= 0x20 && character != 0x7f; });
}

bool encode_corpse_context(const item_corpse_metadata &corpse, std::vector<uint8_t> *encoded)
{
	if (!encoded)
		return false;
	encoded->clear();
	if (!corpse.present)
		return true;
	if (!append_u32(encoded, CORPSE_CONTEXT_VERSION) ||
	    !append_u32(encoded, static_cast<uint32_t>(corpse.room_vnum)) ||
	    !append_u32(encoded, static_cast<uint32_t>(corpse.weight)) ||
	    !append_u32(encoded, corpse.actor_racewar))
		return false;
	for (int32_t value : corpse.values)
		if (!append_u32(encoded, static_cast<uint32_t>(value)))
			return false;
	return append_text(encoded, corpse.owner_name) &&
	       append_text(encoded, corpse.short_description) &&
	       append_text(encoded, corpse.description) && append_text(encoded, corpse.keywords);
}

bool decode_corpse_context(const uint8_t *encoded, size_t size, item_corpse_metadata *corpse)
{
	if (!corpse || (!encoded && size))
		return false;
	*corpse = {};
	if (!size)
		return true;
	size_t offset = 0;
	uint32_t version = 0, room_vnum = 0, weight = 0, actor_racewar = 0;
	if (!read_u32(encoded, size, &offset, &version) || version != CORPSE_CONTEXT_VERSION ||
	    !read_u32(encoded, size, &offset, &room_vnum) ||
	    !read_u32(encoded, size, &offset, &weight) ||
	    !read_u32(encoded, size, &offset, &actor_racewar) || actor_racewar > UINT8_MAX)
		return false;
	corpse->present = true;
	corpse->room_vnum = static_cast<int32_t>(room_vnum);
	corpse->weight = static_cast<int32_t>(weight);
	corpse->actor_racewar = static_cast<uint8_t>(actor_racewar);
	for (int32_t &value : corpse->values)
	{
		uint32_t decoded = 0;
		if (!read_u32(encoded, size, &offset, &decoded))
			return false;
		value = static_cast<int32_t>(decoded);
	}
	return read_text(encoded, size, &offset, ITEM_TRANSFER_CORPSE_NAME_MAX_BYTES,
			 &corpse->owner_name) &&
	       read_text(encoded, size, &offset, ITEM_TRANSFER_CORPSE_SHORT_DESCRIPTION_MAX_BYTES,
			 &corpse->short_description) &&
	       read_text(encoded, size, &offset, ITEM_TRANSFER_CORPSE_DESCRIPTION_MAX_BYTES,
			 &corpse->description) &&
	       read_text(encoded, size, &offset, ITEM_TRANSFER_CORPSE_KEYWORDS_MAX_BYTES,
			 &corpse->keywords) &&
	       offset == size;
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

const item_transfer_entry *find_payload_item(const item_transfer_payload &payload,
					     uint64_t item_uid)
{
	auto found = std::lower_bound(payload.items.begin(),
				      payload.items.begin() + payload.item_count, item_uid,
				      [](const item_transfer_entry &entry, uint64_t uid)
				      { return entry.item_uid < uid; });
	return found != payload.items.begin() + payload.item_count && found->item_uid == item_uid ?
		       &*found :
		       nullptr;
}

uint64_t selected_root_for(const item_transfer_payload &payload, uint64_t item_uid)
{
	if (!payload.multi_root)
		return payload.selected_item_uid ? payload.selected_item_uid :
						   payload.items[0].root_item_uid;
	const item_transfer_entry *entry = find_payload_item(payload, item_uid);
	for (size_t depth = 0; entry && depth <= payload.item_count; ++depth)
	{
		const item_transfer_entry *parent =
			find_payload_item(payload, entry->parent_item_uid);
		if (!parent)
			return entry->item_uid;
		entry = parent;
	}
	return 0;
}

bool target_topology_for(const item_transfer_payload &payload, uint64_t item_uid,
			 uint64_t *root_item_uid, uint64_t *parent_item_uid)
{
	const item_transfer_entry *entry = find_payload_item(payload, item_uid);
	const uint64_t selected_root = selected_root_for(payload, item_uid);
	if (!entry || !selected_root || !root_item_uid || !parent_item_uid)
		return false;
	*root_item_uid = payload.target_parent_item_uid ? payload.target_root_item_uid :
							  selected_root;
	*parent_item_uid = item_uid == selected_root ? payload.target_parent_item_uid :
						       entry->parent_item_uid;
	return *root_item_uid != 0;
}

bool validate_payload(const item_transfer_payload &payload, uint16_t payload_version)
{
	if (!item_owner_identity_valid(payload.from_owner) ||
	    !item_owner_identity_valid(payload.to_owner) || !valid_reason(payload.reason) ||
	    !payload.item_count || payload.item_count > ITEM_TRANSFER_MAX_ITEMS ||
	    (payload_version < ITEM_TRANSFER_PAYLOAD_VERSION &&
	     payload.item_count > ITEM_TRANSFER_LEGACY_MAX_ITEMS) ||
	    payload.item_blob_size > payload.item_blob.size())
		return false;
	const bool corpse_create = payload.reason == item_transfer_reason::corpse_create;
	const bool corpse_loot = payload.reason == item_transfer_reason::corpse_loot;
	const bool corpse_context_required = payload_version >=
						     ITEM_TRANSFER_CORPSE_PAYLOAD_VERSION &&
					     (corpse_create || corpse_loot);
	if (corpse_context_required != payload.corpse.present ||
	    (corpse_create && (payload.from_owner.type != item_owner_type::player ||
			       payload.to_owner.type != item_owner_type::corpse)) ||
	    (corpse_loot && (payload.from_owner.type != item_owner_type::corpse ||
			     payload.to_owner.type != item_owner_type::player)))
		return false;
	if (payload.corpse.present)
	{
		const item_owner_identity &owner = corpse_create ? payload.to_owner :
								   payload.from_owner;
		const uint32_t owner_pid = static_cast<uint32_t>(owner.id >> 32);
		const uint32_t save_id = static_cast<uint32_t>(owner.id);
		if (!owner_pid || owner_pid > INT32_MAX || !save_id || save_id > INT32_MAX ||
		    owner.context_id || payload.corpse.room_vnum < 0 ||
		    payload.corpse.actor_racewar > 4 ||
		    payload.corpse.values[CORPSE_PID_VALUE_INDEX] !=
			    static_cast<int32_t>(owner_pid) ||
		    payload.corpse.values[CORPSE_SAVE_ID_VALUE_INDEX] !=
			    static_cast<int32_t>(save_id) ||
		    payload.corpse.values[CORPSE_RACEWAR_VALUE_INDEX] < 0 ||
		    payload.corpse.values[CORPSE_RACEWAR_VALUE_INDEX] > 4 ||
		    !valid_text(payload.corpse.owner_name, ITEM_TRANSFER_CORPSE_NAME_MAX_BYTES,
				true) ||
		    !valid_text(payload.corpse.short_description,
				ITEM_TRANSFER_CORPSE_SHORT_DESCRIPTION_MAX_BYTES, false) ||
		    !valid_text(payload.corpse.description,
				ITEM_TRANSFER_CORPSE_DESCRIPTION_MAX_BYTES, false) ||
		    !valid_text(payload.corpse.keywords, ITEM_TRANSFER_CORPSE_KEYWORDS_MAX_BYTES,
				false))
			return false;
	}
	const bool creation = payload.from_owner.type == item_owner_type::system;
	const bool destruction = payload.to_owner.type == item_owner_type::destruction;
	if (payload.to_owner.type == item_owner_type::system ||
	    payload.from_owner.type == item_owner_type::destruction ||
	    (payload.reason == item_transfer_reason::creation) != creation ||
	    (payload.reason == item_transfer_reason::destruction) != destruction ||
	    ((creation || destruction) &&
	     item_owner_identity_equal(payload.from_owner, payload.to_owner)))
		return false;
	if (payload.multi_root)
	{
		const bool batch_reason = payload.reason == item_transfer_reason::player_get ||
					  payload.reason == item_transfer_reason::player_drop ||
					  payload.reason == item_transfer_reason::player_put ||
					  payload.reason == item_transfer_reason::locker_deposit ||
					  payload.reason == item_transfer_reason::locker_withdraw ||
					  payload.reason == item_transfer_reason::corpse_loot;
		if (payload_version != ITEM_TRANSFER_PAYLOAD_VERSION || payload.selected_item_uid ||
		    creation || destruction || !batch_reason ||
		    (payload.target_parent_item_uid ? !payload.target_root_item_uid :
						      payload.target_root_item_uid != 0))
			return false;
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const item_transfer_entry &entry = payload.items[index];
			if (!entry.item_uid || !entry.root_item_uid || entry.vnum <= 0 ||
			    entry.expected_state != item_custody_state::active ||
			    (index && payload.items[index - 1].item_uid >= entry.item_uid) ||
			    entry.item_uid == payload.target_parent_item_uid)
				return false;
			const uint64_t selected_root = selected_root_for(payload, entry.item_uid);
			const item_transfer_entry *selected =
				find_payload_item(payload, selected_root);
			if (!selected || selected->root_item_uid != entry.root_item_uid)
				return false;
			uint64_t target_root = 0, target_parent = 0;
			if (!target_topology_for(payload, entry.item_uid, &target_root,
						 &target_parent))
				return false;
		}
		return true;
	}
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

uint64_t item_transfer_selected_root(const item_transfer_payload &payload, uint64_t item_uid)
{
	return selected_root_for(payload, item_uid);
}

uint64_t item_transfer_result_root(const item_transfer_payload &payload)
{
	return payload.item_count ? selected_root_for(payload, payload.items[0].item_uid) : 0;
}

bool item_transfer_target_topology(const item_transfer_payload &payload, uint64_t item_uid,
				   uint64_t *root_item_uid, uint64_t *parent_item_uid)
{
	return target_topology_for(payload, item_uid, root_item_uid, parent_item_uid);
}

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

namespace
{
bool populate_command_entities(critical_command *command, const item_transfer_payload &payload)
{
	critical_entity_key from_key = {}, to_key = {};
	if (!command || !item_owner_key(payload.from_owner, &from_key) ||
	    !item_owner_key(payload.to_owner, &to_key))
		return false;
	command->keys = { from_key, to_key };
	command->expected_revisions = { { from_key, payload.expected_from_revision },
					{ to_key, payload.expected_to_revision } };
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
} // namespace

bool item_transfer_command_encode_payload(const item_transfer_payload &payload,
					  std::vector<uint8_t> *encoded)
{
	std::vector<uint8_t> corpse_context;
	if (!encoded || !validate_payload(payload, ITEM_TRANSFER_PAYLOAD_VERSION) ||
	    !encode_corpse_context(payload.corpse, &corpse_context))
		return false;
	const size_t item_section_size =
		ITEM_TRANSFER_HEADER_BYTES + payload.item_count * ITEM_TRANSFER_ENTRY_BYTES;
	const size_t payload_size = item_section_size + sizeof(uint32_t) + payload.item_blob_size +
				    sizeof(uint32_t) + corpse_context.size();
	if (payload_size > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	encoded->assign(payload_size, 0);
	encode_owner(encoded->data() + FROM_OFFSET, payload.from_owner);
	encode_owner(encoded->data() + TO_OFFSET, payload.to_owner);
	put_u16(encoded->data() + REASON_OFFSET, static_cast<uint16_t>(payload.reason));
	put_u16(encoded->data() + COUNT_OFFSET, payload.item_count);
	put_u64(encoded->data() + REASON_ID_OFFSET, static_cast<uint64_t>(payload.reason_id));
	put_u64(encoded->data() + FROM_REVISION_OFFSET, payload.expected_from_revision);
	put_u64(encoded->data() + TO_REVISION_OFFSET, payload.expected_to_revision);
	const uint64_t selected = payload.multi_root ? 0 :
						       (payload.selected_item_uid ?
								payload.selected_item_uid :
								payload.items[0].root_item_uid);
	put_u64(encoded->data() + SELECTED_ITEM_OFFSET, selected);
	put_u64(encoded->data() + TARGET_ROOT_OFFSET,
		payload.multi_root ?
			payload.target_root_item_uid :
			(payload.target_root_item_uid ? payload.target_root_item_uid : selected));
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
	put_u32(encoded->data() + item_section_size, payload.item_blob_size);
	std::copy_n(payload.item_blob.begin(), payload.item_blob_size,
		    encoded->begin() + item_section_size + sizeof(uint32_t));
	const size_t corpse_size_offset =
		item_section_size + sizeof(uint32_t) + payload.item_blob_size;
	put_u32(encoded->data() + corpse_size_offset, static_cast<uint32_t>(corpse_context.size()));
	std::copy(corpse_context.begin(), corpse_context.end(),
		  encoded->begin() + corpse_size_offset + sizeof(uint32_t));
	return true;
}

bool item_transfer_command_decode_payload(const critical_command &command,
					  item_transfer_payload *payload)
{
	if (!payload || command.type != critical_command_type::item_transfer ||
	    (command.payload_version != ITEM_TRANSFER_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_CORPSE_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_EXACT_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_PREVIOUS_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_LEGACY_PAYLOAD_VERSION) ||
	    (command.payload_version >= ITEM_TRANSFER_EXACT_PAYLOAD_VERSION ?
		     command.payload.size() < ITEM_TRANSFER_HEADER_BYTES +
						      ITEM_TRANSFER_ENTRY_BYTES + sizeof(uint32_t) :
		     command.payload.size() != ITEM_TRANSFER_PAYLOAD_BYTES))
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
	payload->multi_root = command.payload_version == ITEM_TRANSFER_PAYLOAD_VERSION &&
			      payload->selected_item_uid == 0;
	if (!payload->item_count || payload->item_count > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	const bool variable_items = command.payload_version == ITEM_TRANSFER_PAYLOAD_VERSION;
	if (!variable_items && payload->item_count > ITEM_TRANSFER_LEGACY_MAX_ITEMS)
		return false;
	const size_t encoded_item_count = variable_items ? payload->item_count :
							   ITEM_TRANSFER_LEGACY_MAX_ITEMS;
	const size_t item_section_size =
		ITEM_TRANSFER_HEADER_BYTES + encoded_item_count * ITEM_TRANSFER_ENTRY_BYTES;
	if (command.payload.size() < item_section_size)
		return false;
	for (size_t index = 0; index < encoded_item_count; ++index)
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
	if (command.payload_version >= ITEM_TRANSFER_EXACT_PAYLOAD_VERSION)
	{
		payload->item_blob_size = get_u32(command.payload.data() + item_section_size);
		const size_t item_end =
			item_section_size + sizeof(uint32_t) + payload->item_blob_size;
		if (payload->item_blob_size > payload->item_blob.size() ||
		    item_end > command.payload.size())
			return false;
		std::copy_n(command.payload.begin() + item_section_size + sizeof(uint32_t),
			    payload->item_blob_size, payload->item_blob.begin());
		if (command.payload_version == ITEM_TRANSFER_EXACT_PAYLOAD_VERSION)
		{
			if (command.payload.size() != item_end)
				return false;
		}
		else
		{
			if (command.payload.size() < item_end + sizeof(uint32_t))
				return false;
			const uint32_t corpse_size = get_u32(command.payload.data() + item_end);
			if (corpse_size > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
			    command.payload.size() != item_end + sizeof(uint32_t) + corpse_size ||
			    !decode_corpse_context(command.payload.data() + item_end +
							   sizeof(uint32_t),
						   corpse_size, &payload->corpse))
				return false;
		}
	}
	if (!validate_payload(*payload, command.payload_version) ||
	    (command.payload_version == ITEM_TRANSFER_LEGACY_PAYLOAD_VERSION &&
	     payload->reason > item_transfer_reason::auction_claim) ||
	    command.expected_revisions.size() != command.keys.size())
		return false;
	critical_command expected = {};
	if (!populate_command_entities(&expected, *payload))
		return false;
	return command.keys.size() == expected.keys.size() &&
	       command.expected_revisions.size() == expected.expected_revisions.size() &&
	       std::equal(command.keys.begin(), command.keys.end(), expected.keys.begin(),
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
	put_u64(encoded->data() + 40, result.corpse_revision);
	return true;
}

bool item_transfer_command_decode_result(const uint8_t *encoded, size_t size,
					 item_transfer_result *result)
{
	if (!encoded ||
	    (size != ITEM_TRANSFER_RESULT_BYTES && size != ITEM_TRANSFER_LEGACY_RESULT_BYTES) ||
	    !result || encoded[10] || encoded[11] || encoded[12] || encoded[13] || encoded[14] ||
	    encoded[15])
		return false;
	*result = { get_u64(encoded),
		    get_u16(encoded + 8),
		    get_u64(encoded + 16),
		    get_u64(encoded + 24),
		    get_u64(encoded + 32),
		    size == ITEM_TRANSFER_RESULT_BYTES ? get_u64(encoded + 40) : 0 };
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
	if (!item_transfer_command_encode_payload(payload, &encoded))
		return false;
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::item_transfer,
		     .payload_version = ITEM_TRANSFER_PAYLOAD_VERSION,
		     .source_site = source_site,
		     .deadline_class = deadline_class,
		     .accepted_at_usec = 0,
		     .keys = {},
		     .expected_revisions = {},
		     .payload = std::move(encoded) };
	return populate_command_entities(command, payload);
}
