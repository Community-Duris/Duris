#include "item/item_transfer_command.h"
#include "item/quest_reward_continuation.h"
#include "item/craft_pouch_mutation.h"
#include "item/craft_recipe_continuation.h"

#include "player/player_snapshot_codec.h"

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
constexpr uint32_t COLLECTOR_CONTEXT_VERSION = 1;
constexpr uint64_t COLLECTOR_CATALOG_KEY = UINT64_MAX;
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

bool encode_collector_context(const item_collector_death_enrollment &collector,
			      std::vector<uint8_t> *encoded)
{
	if (!encoded)
		return false;
	encoded->clear();
	if (!collector.present)
		return true;
	constexpr size_t fixed_size = sizeof(uint32_t) + CRITICAL_COMMAND_ID_BYTES +
				      sizeof(uint32_t) + sizeof(uint64_t) * 6 + sizeof(uint32_t);
	if (collector.eligible_item_uids.size() > UINT32_MAX ||
	    collector.eligible_item_uids.size() >
		    (CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - fixed_size) / sizeof(uint64_t))
		return false;
	try
	{
		encoded->assign(fixed_size + collector.eligible_item_uids.size() * sizeof(uint64_t),
				0);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	size_t offset = 0;
	put_u32(encoded->data() + offset, COLLECTOR_CONTEXT_VERSION);
	offset += sizeof(uint32_t);
	std::copy(collector.death_operation.bytes.begin(), collector.death_operation.bytes.end(),
		  encoded->begin() + offset);
	offset += CRITICAL_COMMAND_ID_BYTES;
	put_u32(encoded->data() + offset, collector.beneficiary_pid);
	offset += sizeof(uint32_t);
	put_u64(encoded->data() + offset, collector.death_time);
	offset += sizeof(uint64_t);
	put_u64(encoded->data() + offset, collector.policy.collection_delay);
	offset += sizeof(uint64_t);
	put_u64(encoded->data() + offset, collector.policy.sale_delay);
	offset += sizeof(uint64_t);
	put_u64(encoded->data() + offset, collector.policy.holding_duration);
	offset += sizeof(uint64_t);
	put_u64(encoded->data() + offset, collector.policy.price_percent);
	offset += sizeof(uint64_t);
	put_u64(encoded->data() + offset, collector.policy.minimum_value);
	offset += sizeof(uint64_t);
	put_u32(encoded->data() + offset,
		static_cast<uint32_t>(collector.eligible_item_uids.size()));
	offset += sizeof(uint32_t);
	for (uint64_t uid : collector.eligible_item_uids)
	{
		put_u64(encoded->data() + offset, uid);
		offset += sizeof(uint64_t);
	}
	return offset == encoded->size();
}

bool decode_collector_context(const uint8_t *encoded, size_t size,
			      item_collector_death_enrollment *collector)
{
	if (!collector || (!encoded && size))
		return false;
	*collector = {};
	if (!size)
		return true;
	constexpr size_t fixed_size = sizeof(uint32_t) + CRITICAL_COMMAND_ID_BYTES +
				      sizeof(uint32_t) + sizeof(uint64_t) * 6 + sizeof(uint32_t);
	if (size < fixed_size || get_u32(encoded) != COLLECTOR_CONTEXT_VERSION)
		return false;
	size_t offset = sizeof(uint32_t);
	std::copy_n(encoded + offset, collector->death_operation.bytes.size(),
		    collector->death_operation.bytes.begin());
	offset += collector->death_operation.bytes.size();
	collector->beneficiary_pid = get_u32(encoded + offset);
	offset += sizeof(uint32_t);
	collector->death_time = get_u64(encoded + offset);
	offset += sizeof(uint64_t);
	collector->policy.collection_delay = get_u64(encoded + offset);
	offset += sizeof(uint64_t);
	collector->policy.sale_delay = get_u64(encoded + offset);
	offset += sizeof(uint64_t);
	collector->policy.holding_duration = get_u64(encoded + offset);
	offset += sizeof(uint64_t);
	collector->policy.price_percent = get_u64(encoded + offset);
	offset += sizeof(uint64_t);
	collector->policy.minimum_value = get_u64(encoded + offset);
	offset += sizeof(uint64_t);
	const uint32_t count = get_u32(encoded + offset);
	offset += sizeof(uint32_t);
	if (count > ITEM_TRANSFER_MAX_ITEMS ||
	    size - offset != static_cast<size_t>(count) * sizeof(uint64_t))
		return false;
	try
	{
		collector->eligible_item_uids.reserve(count);
		for (uint32_t index = 0; index < count; ++index)
		{
			collector->eligible_item_uids.push_back(get_u64(encoded + offset));
			offset += sizeof(uint64_t);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	collector->present = true;
	return offset == size;
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
	case item_owner_type::collector:
		return critical_entity_type::collector;
	case item_owner_type::pet:
		return critical_entity_type::pet;
	case item_owner_type::native_mobile:
		return critical_entity_type::native_mobile;
	default:
		return critical_entity_type::system;
	}
}

bool valid_reason(item_transfer_reason reason)
{
	switch (reason)
	{
	case item_transfer_reason::synthetic:
	case item_transfer_reason::creation:
	case item_transfer_reason::destruction:
	case item_transfer_reason::operator_repair:
	case item_transfer_reason::player_get:
	case item_transfer_reason::player_drop:
	case item_transfer_reason::player_put:
	case item_transfer_reason::player_give:
	case item_transfer_reason::corpse_create:
	case item_transfer_reason::corpse_restore:
	case item_transfer_reason::corpse_loot:
	case item_transfer_reason::locker_deposit:
	case item_transfer_reason::locker_withdraw:
	case item_transfer_reason::auction_list:
	case item_transfer_reason::auction_claim:
	case item_transfer_reason::shop_buy:
	case item_transfer_reason::shop_sell:
	case item_transfer_reason::mobile_claim:
	case item_transfer_reason::death_restitution:
	case item_transfer_reason::corpse_raise_pet:
	case item_transfer_reason::pet_give:
	case item_transfer_reason::pet_return:
	case item_transfer_reason::trusted_steal:
	case item_transfer_reason::craft:
	case item_transfer_reason::soulbind:
	case item_transfer_reason::slip:
	case item_transfer_reason::player_wear:
	case item_transfer_reason::player_remove:
	case item_transfer_reason::combat_fumble:
	case item_transfer_reason::critical_disarm:
	case item_transfer_reason::quest_turnin:
	case item_transfer_reason::quest_offering:
		return true;
	case item_transfer_reason::collector_collect:
	case item_transfer_reason::collector_buyback:
	case item_transfer_reason::collector_expire:
	case item_transfer_reason::unknown:
		return false;
	}
	return false;
}

bool decode_craft_outputs(const item_transfer_payload &payload,
			  std::vector<player_item_snapshot> *outputs)
{
	if (!outputs || payload.reason != item_transfer_reason::craft ||
	    payload.item_blob_size > payload.item_blob.size())
		return false;
	outputs->clear();
	if (!payload.item_blob_size)
		return true;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     outputs) != player_snapshot_codec_result::ok ||
	    outputs->empty() || outputs->size() > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	std::vector<uint64_t> uids;
	try
	{
		uids.reserve(outputs->size());
		for (size_t index = 0; index < outputs->size(); ++index)
		{
			const player_item_snapshot &output = (*outputs)[index];
			if (!output.object_uid || output.vnum <= 0 ||
			    output.parent_index >= static_cast<int32_t>(index) ||
			    output.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    (index == 0 && output.object_uid != payload.selected_item_uid) ||
			    (index && output.parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
			     output.object_uid == payload.selected_item_uid))
				return false;
			uids.push_back(output.object_uid);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	std::sort(uids.begin(), uids.end());
	return std::adjacent_find(uids.begin(), uids.end()) == uids.end();
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
	if (payload.reason == item_transfer_reason::craft)
	{
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

bool valid_collector_context(const item_transfer_payload &payload, uint16_t payload_version)
{
	const item_collector_death_enrollment &collector = payload.collector;
	if (!collector.present)
		return collector.eligible_item_uids.empty();
	if (payload_version < ITEM_TRANSFER_COLLECTOR_PAYLOAD_VERSION ||
	    payload.reason != item_transfer_reason::corpse_create || !payload.corpse.present ||
	    critical_operation_id_is_zero(collector.death_operation) ||
	    !collector.beneficiary_pid || !collector.death_time ||
	    collector.beneficiary_pid != static_cast<uint32_t>(payload.to_owner.id >> 32) ||
	    collector.death_time != static_cast<uint32_t>(payload.to_owner.id) ||
	    !collector.policy.collection_delay ||
	    collector.policy.sale_delay < collector.policy.collection_delay ||
	    !collector.policy.holding_duration || !collector.policy.price_percent ||
	    !collector.policy.minimum_value ||
	    collector.eligible_item_uids.size() > payload.item_count ||
	    !std::is_sorted(collector.eligible_item_uids.begin(),
			    collector.eligible_item_uids.end()) ||
	    std::adjacent_find(collector.eligible_item_uids.begin(),
			       collector.eligible_item_uids.end()) !=
		    collector.eligible_item_uids.end())
		return false;
	return std::all_of(collector.eligible_item_uids.begin(), collector.eligible_item_uids.end(),
			   [&](uint64_t uid)
			   { return find_payload_item(payload, uid) != nullptr; });
}

bool target_topology_for(const item_transfer_payload &payload, uint64_t item_uid,
			 uint64_t *root_item_uid, uint64_t *parent_item_uid)
{
	const item_transfer_entry *entry = find_payload_item(payload, item_uid);
	const uint64_t selected_root = selected_root_for(payload, item_uid);
	if (!entry || !selected_root || !root_item_uid || !parent_item_uid)
		return false;
	if (payload.reason == item_transfer_reason::craft)
	{
		*root_item_uid = entry->root_item_uid;
		*parent_item_uid = entry->parent_item_uid;
		return true;
	}
	*root_item_uid = payload.target_parent_item_uid ? payload.target_root_item_uid :
							  selected_root;
	*parent_item_uid = item_uid == selected_root ? payload.target_parent_item_uid :
						       entry->parent_item_uid;
	return *root_item_uid != 0;
}

bool valid_quest_offering_continuation(const item_transfer_payload &payload)
{
	const std::vector<uint8_t> &data = payload.continuation.data;
	quest_reward_continuation continuation;
	if (!quest_reward_continuation_decode(data.data(), data.size(), &continuation) ||
	    continuation.player_pid != (payload.native_mobile.present ?
						payload.native_mobile.final_giver_pid :
						payload.from_owner.id) ||
	    continuation.mobile_vnum != static_cast<uint64_t>(payload.reason_id))
		return false;
	if (continuation.root_count > payload.item_count)
		return false;
	size_t selected_roots = 0;
	for (size_t index = 0; index < payload.item_count; ++index)
		selected_roots += payload.items[index].parent_item_uid == 0;
	if (selected_roots != continuation.root_count)
		return false;
	for (size_t index = 0; index < continuation.root_count; ++index)
	{
		const uint64_t uid = continuation.roots[index];
		const item_transfer_entry *root = find_payload_item(payload, uid);
		if (!root || root->parent_item_uid || root->root_item_uid != uid)
			return false;
	}
	return true;
}

bool valid_soulbind_continuation(const item_transfer_payload &payload)
{
	return payload.continuation.data.size() == 1 && payload.continuation.data[0] <= 1 &&
	       payload.reason == item_transfer_reason::soulbind && !payload.multi_root &&
	       payload.from_owner.type == item_owner_type::player &&
	       payload.to_owner.type == item_owner_type::player && payload.selected_item_uid &&
	       payload.item_count > 0 &&
	       find_payload_item(payload, payload.selected_item_uid) != nullptr;
}

bool valid_spell_component_continuation(const item_transfer_payload &payload,
					uint16_t payload_version)
{
	const auto &data = payload.continuation.data;
	if (payload_version < ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION ||
	    payload.reason != item_transfer_reason::destruction || !payload.multi_root ||
	    payload.from_owner.type != item_owner_type::player ||
	    payload.to_owner.type != item_owner_type::destruction)
		return false;
	// Accept the initial unversioned scaffold written before the explicit context
	// envelope was added. These commands remain non-replayable until their effect
	// receipt is available, but the journal must still decode them after upgrade.
	const bool legacy = data.size() >= 4 && data.size() <= 4 + 48 && data[1] == 0 &&
			    data[2] == 0 && data[3] == 0;
	if (!legacy &&
	    (data.size() < 6 || data.size() > 6 + 48 || data[0] != 1 || data[5] != data.size() - 6))
		return false;
	const size_t effect_offset = legacy ? 0 : 1;
	const uint32_t effect = static_cast<uint32_t>(data[effect_offset]) |
				(static_cast<uint32_t>(data[effect_offset + 1]) << 8) |
				(static_cast<uint32_t>(data[effect_offset + 2]) << 16) |
				(static_cast<uint32_t>(data[effect_offset + 3]) << 24);
	return effect >= static_cast<uint32_t>(item_spell_component_effect::faerie_sight) &&
	       effect <= static_cast<uint32_t>(item_spell_component_effect::vines);
}

bool valid_account_reward_retirement_continuation(const item_transfer_payload &payload,
						  uint16_t payload_version)
{
	const auto &data = payload.continuation.data;
	const uint32_t version = data.size() >= 4 ? get_u32(data.data()) : 0;
	const bool nested_uid = version == 2 && data.size() == 28;
	if (payload_version < ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION ||
	    payload.reason != item_transfer_reason::destruction || payload.multi_root ||
	    payload.item_count != 1 || payload.from_owner.type != item_owner_type::player ||
	    !payload.from_owner.id || payload.to_owner.type != item_owner_type::destruction ||
	    payload.to_owner.id || payload.to_owner.context_id ||
	    (!nested_uid && (data.size() != 20 || version != 1)) ||
	    (get_u32(data.data() + 4) & ~UINT32_C(3)) || !get_u64(data.data() + 8) ||
	    !get_u32(data.data() + 16) || get_u32(data.data() + 16) > INT32_MAX)
		return false;
	const item_transfer_entry &item = payload.items[0];
	if (nested_uid)
		return get_u64(data.data() + 20) == item.item_uid &&
		       item.item_uid == payload.selected_item_uid && item.root_item_uid &&
		       item.vnum == static_cast<int32_t>(get_u32(data.data() + 16));
	return item.parent_item_uid == 0 && item.root_item_uid == item.item_uid &&
	       item.item_uid == payload.selected_item_uid &&
	       item.vnum == static_cast<int32_t>(get_u32(data.data() + 16));
}

bool valid_account_reward_duplicate_promotion_continuation(const item_transfer_payload &payload,
							   uint16_t payload_version)
{
	const auto &data = payload.continuation.data;
	if (payload_version < ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION || !payload.multi_root ||
	    (payload.reason != item_transfer_reason::player_get &&
	     payload.reason != item_transfer_reason::player_put) ||
	    payload.from_owner.type != item_owner_type::player || !payload.from_owner.id ||
	    payload.from_owner.context_id || payload.from_owner.id != payload.to_owner.id ||
	    payload.to_owner.type != item_owner_type::player || payload.to_owner.context_id ||
	    payload.selected_item_uid || data.size() < 48 || get_u32(data.data()) != 1 ||
	    get_u32(data.data() + 4) > 1 || !get_u64(data.data() + 8) ||
	    !get_u32(data.data() + 16) || get_u32(data.data() + 16) > INT32_MAX ||
	    !get_u64(data.data() + 20) ||
	    get_u64(data.data() + 28) != payload.target_parent_item_uid)
		return false;
	const uint64_t duplicate_uid = get_u64(data.data() + 20);
	const uint64_t target_parent_uid = get_u64(data.data() + 28);
	const uint32_t direct_child_count = get_u32(data.data() + 36);
	if (!direct_child_count || direct_child_count > 90 ||
	    data.size() != 40 + static_cast<size_t>(direct_child_count) * sizeof(uint64_t) ||
	    duplicate_uid == target_parent_uid ||
	    (target_parent_uid ?
		     payload.reason != item_transfer_reason::player_put ||
			     payload.reason_id != static_cast<int64_t>(target_parent_uid) :
		     payload.reason != item_transfer_reason::player_get ||
			     payload.reason_id != static_cast<int64_t>(get_u64(data.data() + 40))))
		return false;
	std::vector<uint64_t> child_uids;
	try
	{
		child_uids.reserve(direct_child_count);
		for (size_t index = 0; index < direct_child_count; ++index)
		{
			const uint64_t uid = get_u64(data.data() + 40 + index * sizeof(uint64_t));
			const item_transfer_entry *child = find_payload_item(payload, uid);
			if (!uid || !child || child->parent_item_uid != duplicate_uid ||
			    std::find(child_uids.begin(), child_uids.end(), uid) !=
				    child_uids.end())
				return false;
			child_uids.push_back(uid);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	size_t payload_direct_children = 0;
	for (size_t index = 0; index < payload.item_count; ++index)
		if (payload.items[index].parent_item_uid == duplicate_uid)
		{
			++payload_direct_children;
			if (std::find(child_uids.begin(), child_uids.end(),
				      payload.items[index].item_uid) == child_uids.end())
				return false;
		}
	return payload_direct_children == direct_child_count;
}

bool native_mobile_version(uint16_t version) noexcept
{
	return version == ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION ||
	       version == ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION;
}

bool native_publication_terms_valid(const item_native_quest_publication_terms &terms)
{
	return terms.message.size() < ITEM_TRANSFER_NATIVE_MOBILE_MESSAGE_MAX_BYTES &&
	       terms.disappear_message.size() < ITEM_TRANSFER_NATIVE_MOBILE_MESSAGE_MAX_BYTES &&
	       terms.message.find('\0') == std::string::npos &&
	       terms.disappear_message.find('\0') == std::string::npos;
}
bool native_publication_terms_empty(const item_native_quest_publication_terms &terms)
{
	return terms.message.empty() && terms.disappear_message.empty() && !terms.echo_all &&
	       !terms.disappear;
}

bool valid_native_recovery(const item_transfer_payload &payload,
			   const item_native_mobile_recovery_context &recovery, uint16_t version)
{
	const auto before_role = shop_trade_recovery_forest_role::player_before;
	const auto after_role = shop_trade_recovery_forest_role::player_after;
	if (!native_publication_terms_valid(recovery.publication_terms) ||
	    !shop_trade_recovery_forest_shape_valid(recovery.player_before, before_role) ||
	    !shop_trade_recovery_forest_shape_valid(recovery.player_after, after_role))
		return false;
	if (version != ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION)
		return !recovery.present && !recovery.player_pid &&
		       !recovery.acknowledged_save_revision && !recovery.player_before.present &&
		       !recovery.player_after.present && recovery.consumed_root_order.empty() &&
		       native_publication_terms_empty(recovery.publication_terms);
	if (!recovery.present || !recovery.player_pid || recovery.player_pid > INT32_MAX ||
	    recovery.player_pid != payload.native_mobile.final_giver_pid ||
	    !recovery.acknowledged_save_revision)
		return false;
	if (payload.native_mobile.action == item_native_mobile_action::consumption)
	{
		if (recovery.player_before.present || recovery.player_after.present ||
		    recovery.consumed_root_order.empty() ||
		    recovery.consumed_root_order.size() > payload.item_count ||
		    recovery.consumed_root_order.size() >
			    ITEM_TRANSFER_NATIVE_MOBILE_MAX_CONSUMED_ROOTS)
			return false;
		std::vector<player_item_snapshot> selected;
		if (player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size, &selected) !=
			    player_snapshot_codec_result::ok ||
		    selected.size() != payload.item_count)
			return false;
		const auto root_count =
			std::count_if(selected.begin(), selected.end(), [](const auto &item)
				      { return item.parent_index == PLAYER_SNAPSHOT_NO_PARENT; });
		if (static_cast<size_t>(root_count) != recovery.consumed_root_order.size())
			return false;
		for (size_t i = 0; i < recovery.consumed_root_order.size(); ++i)
		{
			const auto uid = recovery.consumed_root_order[i];
			if (!uid || uid == UINT64_MAX ||
			    std::find(recovery.consumed_root_order.begin(),
				      recovery.consumed_root_order.begin() + i,
				      uid) != recovery.consumed_root_order.begin() + i)
				return false;
			auto root = std::find_if(selected.begin(), selected.end(),
						 [uid](const auto &item)
						 { return item.object_uid == uid; });
			if (root == selected.end() ||
			    root->parent_index != PLAYER_SNAPSHOT_NO_PARENT || root->equipment_slot)
				return false;
		}
		if (payload.continuation.kind == item_transfer_continuation_kind::none)
			return payload.continuation.data.empty() &&
			       !recovery.publication_terms.disappear &&
			       recovery.publication_terms.disappear_message.empty();
		quest_reward_continuation terms;
		return payload.continuation.kind ==
			       item_transfer_continuation_kind::quest_offering &&
		       quest_reward_continuation_decode(payload.continuation.data.data(),
							payload.continuation.data.size(), &terms) &&
		       terms.version == 5 &&
		       terms.root_count == recovery.consumed_root_order.size() &&
		       std::equal(recovery.consumed_root_order.begin(),
				  recovery.consumed_root_order.end(), terms.roots.begin());
	}
	if (!recovery.consumed_root_order.empty() ||
	    !native_publication_terms_empty(recovery.publication_terms))
		return false;
	if (payload.native_mobile.action != item_native_mobile_action::acceptance ||
	    payload.from_owner.id != recovery.player_pid || !recovery.player_before.present ||
	    !recovery.player_after.present ||
	    recovery.player_before.canonical_bytes <= recovery.player_after.canonical_bytes)
		return false;
	std::vector<player_item_snapshot> selected;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &selected) != player_snapshot_codec_result::ok ||
	    selected.size() != payload.item_count)
		return false;
	const auto &before = recovery.player_before.ordered_item_uids;
	const auto &after = recovery.player_after.ordered_item_uids;
	if (before.size() != after.size() + selected.size())
		return false;
	// The frozen selected DFS is one contiguous subtree in the complete DFS.
	auto first = std::find(before.begin(), before.end(), selected.front().object_uid);
	if (first == before.end() || static_cast<size_t>(before.end() - first) < selected.size())
		return false;
	for (size_t index = 0; index < selected.size(); ++index)
		if (first[index] != selected[index].object_uid)
			return false;
	const size_t prefix = static_cast<size_t>(first - before.begin());
	return std::equal(before.begin(), first, after.begin()) &&
	       std::equal(first + selected.size(), before.end(), after.begin() + prefix);
}

bool encode_native_recovery(const item_native_mobile_recovery_context &recovery,
			    std::vector<uint8_t> *out)
{
	std::vector<uint8_t> before, after;
	if (!shop_trade_recovery_forest_encode(recovery.player_before,
					       shop_trade_recovery_forest_role::player_before,
					       &before) ||
	    !shop_trade_recovery_forest_encode(
		    recovery.player_after, shop_trade_recovery_forest_role::player_after, &after))
		return false;
	std::vector<uint8_t> candidate(ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_HEADER_BYTES, 0);
	put_u16(candidate.data(), ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_VERSION);
	put_u32(candidate.data() + 4,
		static_cast<uint32_t>(candidate.size() + before.size() + after.size() +
				      recovery.consumed_root_order.size() * sizeof(uint64_t) +
				      ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES +
				      recovery.publication_terms.message.size() +
				      recovery.publication_terms.disappear_message.size()));
	put_u32(candidate.data() + 8, recovery.player_pid);
	put_u32(candidate.data() + 12, static_cast<uint32_t>(recovery.consumed_root_order.size()));
	put_u64(candidate.data() + 16, recovery.acknowledged_save_revision);
	candidate.insert(candidate.end(), before.begin(), before.end());
	candidate.insert(candidate.end(), after.begin(), after.end());
	for (uint64_t root : recovery.consumed_root_order)
	{
		const size_t offset = candidate.size();
		candidate.resize(offset + sizeof(uint64_t));
		put_u64(candidate.data() + offset, root);
	}
	const size_t publication_start = candidate.size();
	candidate.resize(publication_start + ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES);
	put_u32(candidate.data() + publication_start,
		(recovery.publication_terms.echo_all ? 1U : 0U) |
			(recovery.publication_terms.disappear ? 2U : 0U));
	put_u32(candidate.data() + publication_start + 4,
		recovery.publication_terms.message.size());
	put_u32(candidate.data() + publication_start + 8,
		recovery.publication_terms.disappear_message.size());
	candidate.insert(candidate.end(), recovery.publication_terms.message.begin(),
			 recovery.publication_terms.message.end());
	candidate.insert(candidate.end(), recovery.publication_terms.disappear_message.begin(),
			 recovery.publication_terms.disappear_message.end());
	*out = std::move(candidate);
	return true;
}

bool decode_native_recovery(std::span<const uint8_t> bytes,
			    item_native_mobile_recovery_context *out)
{
	if (bytes.size() < ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_MIN_BYTES ||
	    bytes.size() > ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_MAX_BYTES ||
	    get_u16(bytes.data()) != ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_VERSION ||
	    get_u16(bytes.data() + 2) || get_u32(bytes.data() + 4) != bytes.size() ||
	    get_u32(bytes.data() + 12) > ITEM_TRANSFER_NATIVE_MOBILE_MAX_CONSUMED_ROOTS)
		return false;
	const size_t first = ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_HEADER_BYTES;
	const size_t before_bytes = SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES +
				    get_u16(bytes.data() + first + 2) * sizeof(uint64_t);
	if (before_bytes > bytes.size() - first - SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES)
		return false;
	const size_t after_start = first + before_bytes;
	const size_t after_bytes = SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES +
				   get_u16(bytes.data() + after_start + 2) * sizeof(uint64_t);
	if (after_bytes > bytes.size() - after_start)
		return false;
	const size_t roots_start = after_start + after_bytes;
	const size_t root_count = get_u32(bytes.data() + 12);
	if (root_count * sizeof(uint64_t) > bytes.size() - roots_start ||
	    ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES >
		    bytes.size() - roots_start - root_count * sizeof(uint64_t))
		return false;
	item_native_mobile_recovery_context candidate;
	candidate.present = true;
	candidate.player_pid = get_u32(bytes.data() + 8);
	candidate.acknowledged_save_revision = get_u64(bytes.data() + 16);
	if (!shop_trade_recovery_forest_decode(bytes.subspan(first, before_bytes),
					       shop_trade_recovery_forest_role::player_before,
					       &candidate.player_before) ||
	    !shop_trade_recovery_forest_decode(bytes.subspan(after_start, after_bytes),
					       shop_trade_recovery_forest_role::player_after,
					       &candidate.player_after))
		return false;
	candidate.consumed_root_order.reserve(root_count);
	for (size_t i = 0; i < root_count; ++i)
		candidate.consumed_root_order.push_back(
			get_u64(bytes.data() + roots_start + i * sizeof(uint64_t)));
	const size_t publication_start = roots_start + root_count * sizeof(uint64_t);
	const uint32_t flags = get_u32(bytes.data() + publication_start);
	const size_t message_bytes = get_u32(bytes.data() + publication_start + 4);
	const size_t disappear_bytes = get_u32(bytes.data() + publication_start + 8);
	const size_t text_start =
		publication_start + ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES;
	if ((flags & ~3U) || message_bytes >= ITEM_TRANSFER_NATIVE_MOBILE_MESSAGE_MAX_BYTES ||
	    disappear_bytes >= ITEM_TRANSFER_NATIVE_MOBILE_MESSAGE_MAX_BYTES ||
	    message_bytes > bytes.size() - text_start ||
	    disappear_bytes != bytes.size() - text_start - message_bytes)
		return false;
	candidate.publication_terms.echo_all = (flags & 1U) != 0;
	candidate.publication_terms.disappear = (flags & 2U) != 0;
	candidate.publication_terms.message.assign(
		reinterpret_cast<const char *>(bytes.data() + text_start), message_bytes);
	candidate.publication_terms.disappear_message.assign(
		reinterpret_cast<const char *>(bytes.data() + text_start + message_bytes),
		disappear_bytes);
	if (!native_publication_terms_valid(candidate.publication_terms))
		return false;
	*out = std::move(candidate);
	return true;
}

bool valid_native_mobile_context(const item_transfer_payload &payload, uint16_t version)
{
	const bool from_native = payload.from_owner.type == item_owner_type::native_mobile;
	const bool to_native = payload.to_owner.type == item_owner_type::native_mobile;
	if (!native_mobile_version(version))
		return !from_native && !to_native && !payload.native_mobile.present &&
		       payload.reason != item_transfer_reason::quest_offering;
	const auto &context = payload.native_mobile;
	if (!context.present || from_native == to_native || !context.final_giver_pid ||
	    payload.corpse.present || payload.collector.present || payload.logical_source_id ||
	    !payload.item_count || payload.item_count > ITEM_TRANSFER_MAX_ITEMS ||
	    !payload.item_blob_size || payload.item_blob_size > payload.item_blob.size() ||
	    payload.target_parent_item_uid || payload.expected_target_parent_revision)
		return false;
	const auto &owner = from_native ? payload.from_owner : payload.to_owner;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference = {};
	if (!item_owner_identity_valid(owner) || owner.id != context.reference.mobile_instance_id ||
	    quest_mobile_native_reference_encode(context.reference, &reference) !=
		    player_snapshot_codec_result::ok ||
	    payload.reason_id != context.reference.mobile_vnum)
		return false;
	const bool acceptance = context.action == item_native_mobile_action::acceptance;
	const bool consumption = context.action == item_native_mobile_action::consumption;
	if ((!acceptance && !consumption) ||
	    (acceptance &&
	     (from_native || payload.from_owner.type != item_owner_type::player ||
	      payload.from_owner.context_id || payload.from_owner.id > UINT32_MAX ||
	      context.final_giver_pid != payload.from_owner.id ||
	      payload.reason != item_transfer_reason::quest_offering || payload.multi_root ||
	      payload.continuation.kind != item_transfer_continuation_kind::none)) ||
	    (consumption &&
	     (!from_native || payload.to_owner.type != item_owner_type::destruction ||
	      payload.reason != item_transfer_reason::quest_turnin || !payload.multi_root ||
	      payload.selected_item_uid || payload.target_root_item_uid ||
	      (payload.continuation.kind != item_transfer_continuation_kind::none &&
	       payload.continuation.kind != item_transfer_continuation_kind::quest_offering))))
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &entry = payload.items[index];
		if (!entry.item_uid || entry.item_uid == UINT64_MAX || !entry.root_item_uid ||
		    entry.root_item_uid == UINT64_MAX || entry.parent_item_uid == UINT64_MAX ||
		    !entry.expected_item_revision || entry.expected_item_revision == UINT64_MAX ||
		    entry.expected_state != item_custody_state::active ||
		    (index && payload.items[index - 1].item_uid >= entry.item_uid))
			return false;
	}
	std::vector<player_item_snapshot> items;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &items) != player_snapshot_codec_result::ok ||
	    items.size() != payload.item_count)
		return false;
	std::vector<uint8_t> canonical;
	if (player_item_snapshot_list_encode(items, &canonical) !=
		    player_snapshot_codec_result::ok ||
	    canonical.size() != payload.item_blob_size ||
	    !std::equal(canonical.begin(), canonical.end(), payload.item_blob.begin()))
		return false;
	std::vector<uint64_t> roots(items.size()), uids;
	std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH> path = {};
	size_t depth = 0;
	uids.reserve(items.size());
	size_t root_count = 0;
	uint64_t current_root = 0;
	for (size_t index = 0; index < items.size(); ++index)
	{
		const auto &item = items[index];
		if (!item.object_uid || item.object_uid == UINT64_MAX ||
		    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
		    item.parent_index >= static_cast<int32_t>(index) || item.string_mask != 15 ||
		    item.equipment_slot < 0 ||
		    item.equipment_slot > ITEM_TRANSFER_MAX_EQUIPMENT_SLOT)
			return false;
		const bool root = item.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
		const uint64_t parent = root ? 0 : items[item.parent_index].object_uid;
		roots[index] = root ? item.object_uid : roots[item.parent_index];
		if (root)
		{
			++root_count;
			current_root = item.object_uid;
			depth = 1;
			path[0] = static_cast<int32_t>(index);
		}
		else
		{
			if (roots[index] != current_root || item.equipment_slot)
				return false;
			while (depth && path[depth - 1] != item.parent_index)
				--depth;
			if (!depth || depth == path.size())
				return false;
			path[depth++] = static_cast<int32_t>(index);
		}
		const auto *entry = find_payload_item(payload, item.object_uid);
		if (!entry || entry->vnum != item.vnum || entry->parent_item_uid != parent ||
		    entry->root_item_uid != roots[index])
			return false;
		uids.push_back(item.object_uid);
	}
	std::sort(uids.begin(), uids.end());
	if (std::adjacent_find(uids.begin(), uids.end()) != uids.end())
		return false;
	if (acceptance &&
	    (root_count != 1 ||
	     (payload.selected_item_uid && payload.selected_item_uid != items[0].object_uid) ||
	     (payload.target_root_item_uid && payload.target_root_item_uid != items[0].object_uid)))
		return false;
	return true;
}

bool validate_payload(const item_transfer_payload &payload, uint16_t payload_version)
{
	if (!valid_native_mobile_context(payload, payload_version) ||
	    !valid_native_recovery(payload, payload.native_recovery, payload_version))
		return false;
	const bool native_consumption = native_mobile_version(payload_version) &&
					payload.native_mobile.action ==
						item_native_mobile_action::consumption;
	if (!item_owner_identity_valid(payload.from_owner) ||
	    !item_owner_identity_valid(payload.to_owner) || !valid_reason(payload.reason) ||
	    !payload.item_count || payload.item_count > ITEM_TRANSFER_MAX_ITEMS ||
	    (payload_version < ITEM_TRANSFER_BATCH_PAYLOAD_VERSION &&
	     payload.item_count > ITEM_TRANSFER_LEGACY_MAX_ITEMS) ||
	    payload.item_blob_size > payload.item_blob.size() ||
	    payload.from_owner.type == item_owner_type::collector ||
	    payload.to_owner.type == item_owner_type::collector ||
	    !valid_collector_context(payload, payload_version) ||
	    (payload.logical_source_id && (payload_version < ITEM_TRANSFER_SOURCE_PAYLOAD_VERSION ||
					   payload.reason != item_transfer_reason::creation)) ||
	    payload.continuation.data.size() >
		    item_transfer_continuation_limit(payload.continuation.kind) ||
	    (payload.continuation.kind == item_transfer_continuation_kind::none &&
	     !payload.continuation.data.empty()) ||
	    (payload.continuation.kind == item_transfer_continuation_kind::quest_offering &&
	     (payload_version < ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION ||
	      payload.reason != item_transfer_reason::quest_turnin ||
	      (!native_consumption && payload.from_owner.type != item_owner_type::player) ||
	      payload.to_owner.type != item_owner_type::destruction || !payload.multi_root ||
	      (!native_consumption && payload.reason_id <= 0) ||
	      !valid_quest_offering_continuation(payload))) ||
	    (payload.continuation.kind == item_transfer_continuation_kind::soulbind_transfer &&
	     (payload_version < ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION ||
	      !valid_soulbind_continuation(payload))) ||
	    (payload.continuation.kind ==
		     item_transfer_continuation_kind::spell_component_retirement &&
	     !valid_spell_component_continuation(payload, payload_version)) ||
	    (payload.continuation.kind ==
		     item_transfer_continuation_kind::account_reward_retirement &&
	     !valid_account_reward_retirement_continuation(payload, payload_version)) ||
	    (payload.continuation.kind ==
		     item_transfer_continuation_kind::account_reward_duplicate_promotion &&
	     !valid_account_reward_duplicate_promotion_continuation(payload, payload_version)) ||
	    (payload.continuation.kind == item_transfer_continuation_kind::craft_pouch_usage &&
	     payload_version < ITEM_TRANSFER_PAYLOAD_VERSION) ||
	    (payload.continuation.kind == item_transfer_continuation_kind::craft_recipe &&
	     (payload_version < ITEM_TRANSFER_PAYLOAD_VERSION ||
	      payload.reason != item_transfer_reason::craft)) ||
	    (payload.continuation.kind != item_transfer_continuation_kind::none &&
	     payload.continuation.kind != item_transfer_continuation_kind::quest_offering &&
	     payload.continuation.kind != item_transfer_continuation_kind::soulbind_transfer &&
	     payload.continuation.kind !=
		     item_transfer_continuation_kind::spell_component_retirement &&
	     payload.continuation.kind !=
		     item_transfer_continuation_kind::account_reward_retirement &&
	     payload.continuation.kind !=
		     item_transfer_continuation_kind::account_reward_duplicate_promotion &&
	     payload.continuation.kind != item_transfer_continuation_kind::craft_pouch_usage &&
	     payload.continuation.kind != item_transfer_continuation_kind::craft_recipe))
		return false;
	const bool corpse_create = payload.reason == item_transfer_reason::corpse_create;
	const bool corpse_loot = payload.reason == item_transfer_reason::corpse_loot;
	const bool corpse_raise_pet = payload.reason == item_transfer_reason::corpse_raise_pet;
	const bool world_corpse_raise_pet =
		corpse_raise_pet && payload.from_owner.type == item_owner_type::room &&
		payload.from_owner.id && payload.from_owner.id <= INT32_MAX &&
		!payload.from_owner.context_id && payload.to_owner.type == item_owner_type::pet &&
		payload.to_owner.id && payload.to_owner.context_id &&
		payload.to_owner.context_id <= INT32_MAX && payload.multi_root &&
		!payload.target_root_item_uid && !payload.target_parent_item_uid &&
		static_cast<uint64_t>(payload.reason_id) == payload.to_owner.id;
	const bool pet_give = payload.reason == item_transfer_reason::pet_give;
	const bool pet_return = payload.reason == item_transfer_reason::pet_return;
	const bool trusted_steal = payload.reason == item_transfer_reason::trusted_steal;
	const bool craft = payload.reason == item_transfer_reason::craft;
	const bool soulbind = payload.reason == item_transfer_reason::soulbind;
	const bool slip = payload.reason == item_transfer_reason::slip;
	const bool equipment = payload.reason == item_transfer_reason::player_wear ||
			       payload.reason == item_transfer_reason::player_remove;
	const bool forced_drop = item_transfer_forced_weapon_drop(payload.reason);
	const bool player_transfer = trusted_steal || soulbind || slip;
	if (forced_drop &&
	    (payload_version < ITEM_TRANSFER_COLLECTOR_PAYLOAD_VERSION ||
	     payload.from_owner.type != item_owner_type::player ||
	     payload.to_owner.type != item_owner_type::room || payload.from_owner.context_id ||
	     payload.to_owner.context_id || payload.multi_root || !payload.selected_item_uid ||
	     payload.target_root_item_uid != payload.selected_item_uid ||
	     payload.target_parent_item_uid || !payload.item_blob_size || payload.reason_id <= 0 ||
	     payload.reason_id > ITEM_TRANSFER_MAX_EQUIPMENT_SLOT ||
	     !find_payload_item(payload, payload.selected_item_uid) ||
	     find_payload_item(payload, payload.selected_item_uid)->parent_item_uid))
		return false;
	if (equipment &&
	    (payload_version < ITEM_TRANSFER_COLLECTOR_PAYLOAD_VERSION ||
	     payload.from_owner.type != item_owner_type::player ||
	     !item_owner_identity_equal(payload.from_owner, payload.to_owner) ||
	     payload.from_owner.context_id || payload.multi_root || !payload.selected_item_uid ||
	     payload.selected_item_uid != payload.items[0].root_item_uid ||
	     payload.target_root_item_uid != payload.selected_item_uid ||
	     payload.target_parent_item_uid || !payload.item_blob_size || payload.reason_id <= 0 ||
	     payload.reason_id > ITEM_TRANSFER_MAX_EQUIPMENT_SLOT ||
	     !find_payload_item(payload, payload.selected_item_uid) ||
	     find_payload_item(payload, payload.selected_item_uid)->parent_item_uid))
		return false;
	if (player_transfer &&
	    (payload_version < ITEM_TRANSFER_COLLECTOR_PAYLOAD_VERSION ||
	     payload.from_owner.type != item_owner_type::player ||
	     payload.to_owner.type != item_owner_type::player || !payload.from_owner.id ||
	     !payload.to_owner.id || payload.from_owner.id > INT32_MAX ||
	     payload.to_owner.id > INT32_MAX || payload.from_owner.context_id ||
	     payload.to_owner.context_id || payload.from_owner.id == payload.to_owner.id ||
	     payload.multi_root || payload.target_parent_item_uid ||
	     payload.reason_id != static_cast<int64_t>(payload.from_owner.id)))
		return false;
	if (craft)
	{
		std::vector<player_item_snapshot> outputs;
		craft_pouch_mutation pouch;
		if (payload_version < ITEM_TRANSFER_BATCH_PAYLOAD_VERSION ||
		    payload.from_owner.type != item_owner_type::player ||
		    payload.to_owner.type != item_owner_type::player ||
		    !item_owner_identity_equal(payload.from_owner, payload.to_owner) ||
		    !payload.from_owner.id || payload.from_owner.context_id ||
		    payload.to_owner.context_id || !payload.multi_root ||
		    !payload.selected_item_uid || payload.target_root_item_uid ||
		    payload.target_parent_item_uid || payload.expected_target_parent_revision ||
		    !decode_craft_outputs(payload, &outputs) ||
		    !craft_pouch_mutation_from_payload(payload, &pouch) ||
		    (outputs.empty() && !find_payload_item(payload, payload.selected_item_uid)) ||
		    payload.item_count + outputs.size() > CRITICAL_COMMAND_MAX_KEYS - 2)
			return false;
		if (payload.continuation.kind == item_transfer_continuation_kind::craft_recipe)
		{
			craft_recipe_continuation recipe;
			if (!craft_recipe_continuation_decode(payload.continuation.data, &recipe) ||
			    !craft_recipe_continuation_matches(recipe, payload) ||
			    (craft_recipe_is_alchemy(recipe.discipline) ?
				     static_cast<uint32_t>(
					     std::count_if(outputs.begin(), outputs.end(),
							   [](const auto &output) {
								   return output.parent_index ==
									  PLAYER_SNAPSHOT_NO_PARENT;
							   })) != recipe.output_count ||
					     (!outputs.empty() &&
					      outputs[0].object_uid != recipe.output_uid) :
				     outputs.size() != 1 ||
					     outputs[0].object_uid != recipe.output_uid ||
					     outputs[0].vnum !=
						     static_cast<int32_t>(recipe.recipe_vnum)))
				return false;
		}
		for (const player_item_snapshot &output : outputs)
			for (size_t index = 0; index < payload.item_count; ++index)
				if (output.object_uid == payload.items[index].item_uid)
					return false;
	}
	// Other commands do not update the pet's physical item projection.
	if ((payload.from_owner.type == item_owner_type::pet ||
	     payload.to_owner.type == item_owner_type::pet) &&
	    !pet_give && !pet_return && !corpse_raise_pet)
		return false;
	if ((pet_give && (payload.from_owner.type != item_owner_type::player ||
			  payload.to_owner.type != item_owner_type::pet ||
			  payload.from_owner.id != payload.to_owner.context_id ||
			  payload.reason_id != static_cast<int64_t>(payload.to_owner.id))) ||
	    (pet_return && (payload.from_owner.type != item_owner_type::pet ||
			    payload.to_owner.type != item_owner_type::player ||
			    payload.from_owner.context_id != payload.to_owner.id ||
			    payload.reason_id != static_cast<int64_t>(payload.from_owner.id))) ||
	    ((pet_give || pet_return) && (payload.multi_root || payload.target_parent_item_uid)))
		return false;
	const bool corpse_context_required =
		payload_version >= ITEM_TRANSFER_CORPSE_PAYLOAD_VERSION &&
		(corpse_create || corpse_loot || (corpse_raise_pet && !world_corpse_raise_pet));
	if (corpse_context_required != payload.corpse.present ||
	    (corpse_create && (payload.from_owner.type != item_owner_type::player ||
			       payload.to_owner.type != item_owner_type::corpse)) ||
	    (corpse_loot && (payload.from_owner.type != item_owner_type::corpse ||
			     payload.to_owner.type != item_owner_type::player)) ||
	    (corpse_raise_pet && !world_corpse_raise_pet &&
	     (payload.from_owner.type != item_owner_type::corpse ||
	      payload.to_owner.type != item_owner_type::pet)))
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
	const bool quest_turnin = payload.reason == item_transfer_reason::quest_turnin;
	if (payload.to_owner.type == item_owner_type::system ||
	    payload.from_owner.type == item_owner_type::destruction ||
	    (payload.reason == item_transfer_reason::creation) != creation ||
	    ((payload.reason == item_transfer_reason::destruction || quest_turnin) !=
	     destruction) ||
	    (quest_turnin && !native_consumption &&
	     (payload.from_owner.type != item_owner_type::player || !payload.multi_root ||
	      payload.reason_id <= 0 ||
	      payload.continuation.kind != item_transfer_continuation_kind::quest_offering)) ||
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
					  payload.reason == item_transfer_reason::corpse_loot ||
					  payload.reason ==
						  item_transfer_reason::corpse_raise_pet ||
					  payload.reason == item_transfer_reason::corpse_create ||
					  payload.reason == item_transfer_reason::destruction ||
					  quest_turnin || craft;
		const bool creation_batch = creation &&
					    payload.reason == item_transfer_reason::creation;
		if (payload_version < ITEM_TRANSFER_BATCH_PAYLOAD_VERSION ||
		    (!craft && payload.selected_item_uid) || (!creation_batch && !batch_reason) ||
		    (craft && (!payload.selected_item_uid || !payload.multi_root)) ||
		    (creation_batch &&
		     (payload.to_owner.type != item_owner_type::player ||
		      payload.target_root_item_uid || payload.target_parent_item_uid)) ||
		    (payload.target_parent_item_uid ? !payload.target_root_item_uid :
						      payload.target_root_item_uid != 0))
			return false;
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const item_transfer_entry &entry = payload.items[index];
			if (!entry.item_uid || !entry.root_item_uid || entry.vnum <= 0 ||
			    (creation_batch ? (entry.expected_item_revision !=
						       ITEM_TRANSFER_ABSENT_REVISION ||
					       entry.expected_state != item_custody_state::absent) :
					      entry.expected_state != item_custody_state::active) ||
			    (index && payload.items[index - 1].item_uid >= entry.item_uid) ||
			    entry.item_uid == payload.target_parent_item_uid)
				return false;
			if (creation_batch)
			{
				const uint64_t selected_root =
					selected_root_for(payload, entry.item_uid);
				const item_transfer_entry *root =
					find_payload_item(payload, selected_root);
				uint64_t target_root = 0, target_parent = 0;
				if (!root || selected_root != entry.root_item_uid ||
				    root->root_item_uid != root->item_uid ||
				    root->parent_item_uid ||
				    !target_topology_for(payload, entry.item_uid, &target_root,
							 &target_parent) ||
				    target_root != entry.root_item_uid ||
				    target_parent != entry.parent_item_uid)
					return false;
				continue;
			}
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

bool item_transfer_selected_roots(const item_transfer_payload &payload,
				  std::vector<uint64_t> *roots)
{
	if (!roots)
		return false;
	try
	{
		roots->clear();
		roots->reserve(payload.item_count);
		for (size_t index = 0; index < payload.item_count; ++index)
			if (selected_root_for(payload, payload.items[index].item_uid) ==
			    payload.items[index].item_uid)
				roots->push_back(payload.items[index].item_uid);
		std::sort(roots->begin(), roots->end());
		roots->erase(std::unique(roots->begin(), roots->end()), roots->end());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return !roots->empty();
}

uint64_t item_transfer_result_root(const item_transfer_payload &payload)
{
	if (payload.reason == item_transfer_reason::craft)
		return payload.selected_item_uid;
	uint64_t result = 0;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const uint64_t selected_root =
			selected_root_for(payload, payload.items[index].item_uid);
		if (!selected_root)
			return 0;
		if (!result || selected_root < result)
			result = selected_root;
	}
	return result;
}

bool item_transfer_target_topology(const item_transfer_payload &payload, uint64_t item_uid,
				   uint64_t *root_item_uid, uint64_t *parent_item_uid)
{
	return target_topology_for(payload, item_uid, root_item_uid, parent_item_uid);
}

bool item_owner_identity_valid(const item_owner_identity &owner)
{
	if (owner.type <= item_owner_type::unknown || owner.type > item_owner_type::native_mobile)
		return false;
	if (owner.type == item_owner_type::system || owner.type == item_owner_type::destruction)
		return owner.id == 0 && owner.context_id == 0;
	if (owner.type == item_owner_type::collector)
		return owner.id != 0 && owner.context_id == 0;
	if (owner.type == item_owner_type::native_mobile)
		return owner.id && owner.id != UINT64_MAX && !owner.context_id;
	if (owner.type == item_owner_type::pet)
		return owner.id != 0 && owner.context_id != 0 && owner.context_id <= INT32_MAX;
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

uint64_t item_collector_owner_id(uint64_t listing_id)
{
	return listing_id;
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
	if (payload.reason == item_transfer_reason::craft)
	{
		std::vector<player_item_snapshot> outputs;
		if (!decode_craft_outputs(payload, &outputs))
			return false;
		for (const player_item_snapshot &output : outputs)
		{
			const critical_entity_key output_key = { critical_entity_type::item,
								 output.object_uid };
			command->keys.push_back(output_key);
			command->expected_revisions.push_back(
				{ output_key, ITEM_TRANSFER_ABSENT_REVISION });
		}
	}
	if (payload.collector.present)
	{
		const critical_entity_key catalog_key = { critical_entity_type::collector,
							  COLLECTOR_CATALOG_KEY };
		command->keys.push_back(catalog_key);
		// The SQL repository takes the current catalog row lock before any item
		// lock; zero is a serialization key, not an optimistic catalog fence.
		command->expected_revisions.push_back({ catalog_key, 0 });
	}
	if (payload.native_recovery.present &&
	    payload.native_mobile.action == item_native_mobile_action::consumption)
	{
		const critical_entity_key giver_key = { critical_entity_type::player,
							payload.native_recovery.player_pid };
		command->keys.push_back(giver_key);
		// A serialization key only; the acknowledged save fence remains distinct
		// from a player custody-owner revision and is checked by the original root.
		command->expected_revisions.push_back({ giver_key, 0 });
	}
	if (payload.native_recovery.present && command->keys.size() > CRITICAL_COMMAND_MAX_KEYS)
		return false;
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

namespace
{
bool encode_payload(const item_transfer_payload &payload, uint16_t version,
		    std::vector<uint8_t> *encoded)
{
	std::vector<uint8_t> corpse_context;
	std::vector<uint8_t> collector_context;
	std::vector<uint8_t> recovery_context;
	if (!encoded || !validate_payload(payload, version) ||
	    !encode_corpse_context(payload.corpse, &corpse_context) ||
	    !encode_collector_context(payload.collector, &collector_context))
		return false;
	if (version == ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION &&
	    !encode_native_recovery(payload.native_recovery, &recovery_context))
		return false;
	const size_t item_section_size =
		ITEM_TRANSFER_HEADER_BYTES + payload.item_count * ITEM_TRANSFER_ENTRY_BYTES;
	const size_t payload_size =
		item_section_size + sizeof(uint32_t) + payload.item_blob_size + sizeof(uint32_t) +
		corpse_context.size() + sizeof(uint32_t) + collector_context.size() +
		sizeof(payload.logical_source_id) + sizeof(uint32_t) * 2 +
		payload.continuation.data.size() +
		(native_mobile_version(version) ? ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES : 0) +
		recovery_context.size();
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
	const uint64_t selected =
		(payload.multi_root && payload.reason != item_transfer_reason::craft) ?
			0 :
			(payload.selected_item_uid ? payload.selected_item_uid :
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
	const size_t collector_size_offset =
		corpse_size_offset + sizeof(uint32_t) + corpse_context.size();
	put_u32(encoded->data() + collector_size_offset,
		static_cast<uint32_t>(collector_context.size()));
	std::copy(collector_context.begin(), collector_context.end(),
		  encoded->begin() + collector_size_offset + sizeof(uint32_t));
	put_u64(encoded->data() + collector_size_offset + sizeof(uint32_t) +
			collector_context.size(),
		payload.logical_source_id);
	const size_t continuation_offset = collector_size_offset + sizeof(uint32_t) +
					   collector_context.size() +
					   sizeof(payload.logical_source_id);
	put_u32(encoded->data() + continuation_offset,
		static_cast<uint32_t>(payload.continuation.kind));
	put_u32(encoded->data() + continuation_offset + sizeof(uint32_t),
		static_cast<uint32_t>(payload.continuation.data.size()));
	std::copy(payload.continuation.data.begin(), payload.continuation.data.end(),
		  encoded->begin() + continuation_offset + sizeof(uint32_t) * 2);
	if (native_mobile_version(version))
	{
		const size_t tail = continuation_offset + sizeof(uint32_t) * 2 +
				    payload.continuation.data.size();
		uint8_t *output = encoded->data() + tail;
		put_u16(output, ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_VERSION);
		output[2] = static_cast<uint8_t>(payload.native_mobile.action);
		put_u32(output + 4, ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES);
		put_u32(output + 8, payload.native_mobile.final_giver_pid);
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference = {};
		if (quest_mobile_native_reference_encode(payload.native_mobile.reference,
							 &reference) !=
		    player_snapshot_codec_result::ok)
			return false;
		std::copy(reference.begin(), reference.end(), output + 16);
		std::copy(recovery_context.begin(), recovery_context.end(),
			  output + ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES);
	}
	return true;
}
} // namespace

bool item_transfer_command_encode_payload(const item_transfer_payload &payload,
					  std::vector<uint8_t> *encoded)
{
	return encode_payload(payload, ITEM_TRANSFER_PAYLOAD_VERSION, encoded);
}

bool item_transfer_native_mobile_shape_valid(const item_transfer_payload &payload) noexcept
{
	try
	{
		return validate_payload(payload, ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_command_encode_native_mobile(const item_transfer_payload &payload,
						std::vector<uint8_t> *encoded) noexcept
{
	if (!encoded)
		return false;
	try
	{
		std::vector<uint8_t> candidate;
		if (!encode_payload(payload, ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION,
				    &candidate))
			return false;
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_native_mobile_recovery_shape_valid(const item_transfer_payload &payload) noexcept
{
	try
	{
		return validate_payload(payload,
					ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_native_mobile_recovery_freeze(
	item_transfer_payload *payload, uint32_t player_pid, uint64_t acknowledged_save_revision,
	std::span<const uint8_t> original_player_items) noexcept
{
	return item_transfer_native_mobile_recovery_freeze(
		payload, player_pid, acknowledged_save_revision, original_player_items, {});
}

bool item_transfer_native_mobile_recovery_freeze(
	item_transfer_payload *payload, uint32_t player_pid, uint64_t acknowledged_save_revision,
	std::span<const uint8_t> original_player_items,
	std::span<const uint64_t> consumed_root_order) noexcept
{
	return item_transfer_native_mobile_recovery_freeze(payload, player_pid,
							   acknowledged_save_revision,
							   original_player_items,
							   consumed_root_order, {});
}

bool item_transfer_native_mobile_recovery_freeze(
	item_transfer_payload *payload, uint32_t player_pid, uint64_t acknowledged_save_revision,
	std::span<const uint8_t> original_player_items,
	std::span<const uint64_t> consumed_root_order,
	const item_native_quest_publication_terms &publication_terms) noexcept
{
	if (!payload)
		return false;
	try
	{
		if (!valid_native_mobile_context(*payload,
						 ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION))
			return false;
		item_native_mobile_recovery_context candidate;
		candidate.present = true;
		candidate.player_pid = player_pid;
		candidate.acknowledged_save_revision = acknowledged_save_revision;
		if (consumed_root_order.size() > ITEM_TRANSFER_NATIVE_MOBILE_MAX_CONSUMED_ROOTS)
			return false;
		candidate.consumed_root_order.assign(consumed_root_order.begin(),
						     consumed_root_order.end());
		candidate.publication_terms = publication_terms;
		if (payload->native_mobile.action == item_native_mobile_action::acceptance)
		{
			std::vector<player_item_snapshot> before, selected, after;
			std::vector<uint8_t> selected_bytes, after_bytes;
			if (!shop_trade_recovery_forest_freeze(
				    original_player_items,
				    shop_trade_recovery_forest_role::player_before,
				    &candidate.player_before) ||
			    player_item_snapshot_list_decode(
				    original_player_items.data(), original_player_items.size(),
				    &before) != player_snapshot_codec_result::ok ||
			    player_item_snapshot_extract_subtree(
				    before, item_transfer_result_root(*payload), &selected,
				    &after) != player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_encode(selected, &selected_bytes) !=
				    player_snapshot_codec_result::ok ||
			    selected_bytes.size() != payload->item_blob_size ||
			    !std::equal(selected_bytes.begin(), selected_bytes.end(),
					payload->item_blob.begin()) ||
			    player_item_snapshot_list_encode(after, &after_bytes) !=
				    player_snapshot_codec_result::ok ||
			    !shop_trade_recovery_forest_freeze(
				    after_bytes, shop_trade_recovery_forest_role::player_after,
				    &candidate.player_after))
				return false;
		}
		else if (!original_player_items.empty())
			return false;
		if (!valid_native_recovery(*payload, candidate,
					   ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION))
			return false;
		payload->native_recovery = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_command_encode_native_mobile_recovery(const item_transfer_payload &payload,
							 std::vector<uint8_t> *encoded) noexcept
{
	if (!encoded)
		return false;
	try
	{
		std::vector<uint8_t> candidate;
		if (!encode_payload(payload, ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION,
				    &candidate))
			return false;
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

static bool decode_payload(const critical_command &command, item_transfer_payload *payload)
{
	if (command.payload_version == ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION &&
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	if (!payload || command.type != critical_command_type::item_transfer ||
	    (command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_SOURCE_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_COLLECTOR_PAYLOAD_VERSION &&
	     command.payload_version != ITEM_TRANSFER_BATCH_PAYLOAD_VERSION &&
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
	// Master v7 craft used 29 (the unpublished draft used 27). This branch
	// already owns 29 for wear, so normalize only the old same-player craft
	// shape. Wear targets its selected root; craft has no destination root.
	if (command.payload_version == 7 &&
	    (payload->reason == item_transfer_reason::soulbind ||
	     payload->reason == item_transfer_reason::player_wear) &&
	    payload->from_owner.type == item_owner_type::player &&
	    item_owner_identity_equal(payload->from_owner, payload->to_owner) &&
	    payload->selected_item_uid && !payload->target_root_item_uid &&
	    !payload->target_parent_item_uid)
		payload->reason = item_transfer_reason::craft;
	payload->multi_root =
		command.payload_version >= ITEM_TRANSFER_BATCH_PAYLOAD_VERSION &&
		(payload->selected_item_uid == 0 || payload->reason == item_transfer_reason::craft);
	if (!payload->item_count || payload->item_count > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	const bool variable_items = command.payload_version >= ITEM_TRANSFER_BATCH_PAYLOAD_VERSION;
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
		if (command.payload.size() < item_section_size + sizeof(uint32_t))
			return false;
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
			const size_t corpse_end = item_end + sizeof(uint32_t) + corpse_size;
			if (corpse_size > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
			    corpse_end > command.payload.size() ||
			    !decode_corpse_context(command.payload.data() + item_end +
							   sizeof(uint32_t),
						   corpse_size, &payload->corpse))
				return false;
			if (command.payload_version < ITEM_TRANSFER_COLLECTOR_PAYLOAD_VERSION)
			{
				if (command.payload.size() != corpse_end)
					return false;
			}
			else
			{
				if (command.payload.size() < corpse_end + sizeof(uint32_t))
					return false;
				const uint32_t collector_size =
					get_u32(command.payload.data() + corpse_end);
				const size_t collector_end =
					corpse_end + sizeof(uint32_t) + collector_size;
				if (collector_size > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
				    collector_end > command.payload.size() ||
				    !decode_collector_context(command.payload.data() + corpse_end +
								      sizeof(uint32_t),
							      collector_size, &payload->collector))
					return false;
				const size_t source_end =
					collector_end +
					(command.payload_version >=
							 ITEM_TRANSFER_SOURCE_PAYLOAD_VERSION ?
						 sizeof(payload->logical_source_id) :
						 0);
				if (source_end > command.payload.size())
					return false;
				if (command.payload_version >= ITEM_TRANSFER_SOURCE_PAYLOAD_VERSION)
					payload->logical_source_id =
						get_u64(command.payload.data() + collector_end);
				if (command.payload_version <
				    ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION)
				{
					if (command.payload.size() != source_end)
						return false;
				}
				else
				{
					if (command.payload.size() <
					    source_end + sizeof(uint32_t) * 2)
						return false;
					payload->continuation.kind =
						static_cast<item_transfer_continuation_kind>(
							get_u32(command.payload.data() +
								source_end));
					const uint32_t continuation_size =
						get_u32(command.payload.data() + source_end +
							sizeof(uint32_t));
					if (continuation_size >
						    item_transfer_continuation_limit(
							    payload->continuation.kind) ||
					    command.payload.size() <
						    source_end + sizeof(uint32_t) * 2 +
							    continuation_size +
							    (native_mobile_version(
								     command.payload_version) ?
								     ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES :
								     0) ||
					    (command.payload_version !=
						     ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION &&
					     command.payload.size() !=
						     source_end + sizeof(uint32_t) * 2 +
							     continuation_size +
							     (native_mobile_version(
								      command.payload_version) ?
								      ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES :
								      0)))
						return false;
					payload->continuation.data.assign(
						command.payload.begin() + source_end +
							sizeof(uint32_t) * 2,
						command.payload.begin() + source_end +
							sizeof(uint32_t) * 2 + continuation_size);
					if (native_mobile_version(command.payload_version))
					{
						const uint8_t *tail =
							command.payload.data() + source_end +
							sizeof(uint32_t) * 2 + continuation_size;
						if (get_u16(tail) !=
							    ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_VERSION ||
						    tail[3] ||
						    get_u32(tail + 4) !=
							    ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES ||
						    get_u32(tail + 12) || get_u32(tail + 164) ||
						    quest_mobile_native_reference_decode(
							    std::span<const uint8_t>(
								    tail + 16,
								    QUEST_MOBILE_NATIVE_REFERENCE_BYTES),
							    &payload->native_mobile.reference) !=
							    player_snapshot_codec_result::ok)
							return false;
						payload->native_mobile.present = true;
						payload->native_mobile.action =
							static_cast<item_native_mobile_action>(
								tail[2]);
						payload->native_mobile.final_giver_pid =
							get_u32(tail + 8);
						if (command.payload_version ==
						    ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION)
						{
							const size_t offset =
								static_cast<size_t>(
									tail -
									command.payload.data()) +
								ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES;
							if (!decode_native_recovery(
								    std::span<const uint8_t>(
									    command.payload)
									    .subspan(offset),
								    &payload->native_recovery))
								return false;
						}
					}
				}
			}
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

bool item_transfer_command_decode_payload(const critical_command &command,
					  item_transfer_payload *payload)
{
	if (!native_mobile_version(command.payload_version))
		return decode_payload(command, payload);
	if (!payload)
		return false;
	try
	{
		item_transfer_payload candidate = {};
		if (!decode_payload(command, &candidate))
			return false;
		std::vector<uint8_t> canonical;
		if (!(command.payload_version ==
				      ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION ?
			      item_transfer_command_encode_native_mobile_recovery(candidate,
										  &canonical) :
			      item_transfer_command_encode_native_mobile(candidate, &canonical)) ||
		    canonical != command.payload)
			return false;
		*payload = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
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
	(*encoded)[10] = result.collector_catalog_changed ? 1 : 0;
	return true;
}

bool item_transfer_command_decode_result(const uint8_t *encoded, size_t size,
					 item_transfer_result *result)
{
	if (!encoded ||
	    (size != ITEM_TRANSFER_RESULT_BYTES && size != ITEM_TRANSFER_LEGACY_RESULT_BYTES) ||
	    !result || encoded[10] > 1 || encoded[11] || encoded[12] || encoded[13] ||
	    encoded[14] || encoded[15])
		return false;
	*result = { get_u64(encoded),
		    get_u16(encoded + 8),
		    get_u64(encoded + 16),
		    get_u64(encoded + 24),
		    get_u64(encoded + 32),
		    size == ITEM_TRANSFER_RESULT_BYTES ? get_u64(encoded + 40) : 0,
		    encoded[10] != 0 };
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

// Transient typed value builder only; the parent must freeze schema2 authority
// and original admission. This schema1-shaped scaffold cannot execute legacy.
bool item_transfer_command_build_native_mobile(critical_command *command,
					       critical_operation_id operation_id,
					       const item_transfer_payload &payload,
					       critical_source_site source_site,
					       critical_deadline_class deadline_class) noexcept
{
	if (!command || critical_operation_id_is_zero(operation_id))
		return false;
	try
	{
		std::vector<uint8_t> encoded;
		if (!item_transfer_command_encode_native_mobile(payload, &encoded))
			return false;
		critical_command candidate = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
					       .operation_id = operation_id,
					       .type = critical_command_type::item_transfer,
					       .payload_version =
						       ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION,
					       .source_site = source_site,
					       .deadline_class = deadline_class,
					       .accepted_at_usec = 0,
					       .keys = {},
					       .expected_revisions = {},
					       .payload = std::move(encoded) };
		if (!populate_command_entities(&candidate, payload))
			return false;
		*command = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_command_build_native_mobile_recovery(
	critical_command *command, critical_operation_id operation_id,
	const item_transfer_payload &payload, critical_source_site source_site,
	critical_deadline_class deadline_class) noexcept
{
	if (!command || critical_operation_id_is_zero(operation_id))
		return false;
	try
	{
		std::vector<uint8_t> encoded;
		if (!item_transfer_command_encode_native_mobile_recovery(payload, &encoded))
			return false;
		critical_command candidate = {
			.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
			.operation_id = operation_id,
			.type = critical_command_type::item_transfer,
			.payload_version = ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION,
			.source_site = source_site,
			.deadline_class = deadline_class,
			.accepted_at_usec = 0,
			.keys = {},
			.expected_revisions = {},
			.payload = std::move(encoded)
		};
		if (!populate_command_entities(&candidate, payload))
			return false;
		*command = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
