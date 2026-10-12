#include "item/item_transfer_command.h"
#include "item/lockpick_retirement_continuation.h"
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
	// Every legacy/generic encoder must refuse rather than silently drop money.
	if (payload.native_money.present || payload.native_money.original_room_vnum ||
	    payload.native_money.player_wallet_mapping_id ||
	    payload.native_money.mobile_wallet_mapping_id ||
	    payload.native_money.projection != native_quest_coin_give_projection{})
		return false;
	// Every legacy/generic encoder must refuse rather than silently drop cost.
	if (payload.native_cost.present || payload.native_cost.fee_only ||
	    payload.native_cost.completion_slot || payload.native_cost.wallet_mapping_id ||
	    payload.native_cost.projection != native_quest_cost_projection{})
		return false;
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
	    (payload.continuation.kind == item_transfer_continuation_kind::lockpick_retirement &&
	     (payload_version < ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION ||
	      !lockpick_retirement_payload_valid(payload))) ||
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
	     payload.continuation.kind != item_transfer_continuation_kind::craft_recipe &&
	     payload.continuation.kind != item_transfer_continuation_kind::lockpick_retirement))
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
			    (craft_recipe_is_alchemy(recipe.discipline) ||
					     recipe.discipline == craft_recipe_discipline::refine ?
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
						     static_cast<int32_t>(recipe.recipe_vnum)) ||
			    (recipe.discipline == craft_recipe_discipline::refine &&
			     (!outputs.empty() &&
			      (outputs.size() != 1 ||
			       outputs[0].vnum != static_cast<int32_t>(recipe.recipe_vnum)))))
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
	    payload.native_mobile.action == item_native_mobile_action::consumption &&
	    !payload.native_cost.fee_only)
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
	// Lockpick retirement has no destination tree; keep its validated zero root.
	put_u64(encoded->data() + TARGET_ROOT_OFFSET,
		(payload.multi_root ||
		 payload.continuation.kind ==
			 item_transfer_continuation_kind::lockpick_retirement) ?
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

namespace
{
bool native_cost_version(uint16_t version) noexcept
{
	return version == ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION ||
	       version == ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION;
}

bool native_cost_value_valid(const item_transfer_payload &payload) noexcept
{
	if (!payload.native_cost.present || !payload.native_cost.wallet_mapping_id ||
	    payload.native_mobile.action != item_native_mobile_action::consumption ||
	    payload.native_cost.projection.attempts.empty())
		return false;
	std::vector<uint8_t> exact;
	return native_quest_cost_projection_encode(payload.native_cost.projection, &exact) ==
	       native_quest_cost_projection_result::ok;
}

constexpr size_t native_fee_header_bytes = 64;
bool native_fee_shape(const item_transfer_payload &payload, bool acknowledged)
{
	const auto &fee = payload.native_cost;
	const auto &native = payload.native_mobile;
	const auto &recovery = payload.native_recovery;
	if (!fee.fee_only || !native_cost_value_valid(payload) || !native.present ||
	    native.action != item_native_mobile_action::consumption || !native.final_giver_pid ||
	    native.final_giver_pid > INT32_MAX || native.reference.mobile_revision == UINT64_MAX ||
	    payload.from_owner.type != item_owner_type::native_mobile ||
	    payload.from_owner.id != native.reference.mobile_instance_id ||
	    payload.from_owner.context_id || payload.to_owner.type != item_owner_type::player ||
	    payload.to_owner.id != native.final_giver_pid || payload.to_owner.context_id ||
	    payload.reason != item_transfer_reason::quest_turnin ||
	    payload.reason_id != native.reference.mobile_vnum || payload.logical_source_id ||
	    payload.multi_root || payload.item_count || payload.item_blob_size ||
	    payload.selected_item_uid || payload.target_root_item_uid ||
	    payload.target_parent_item_uid || payload.expected_target_parent_revision ||
	    payload.corpse.present || payload.collector.present ||
	    !valid_collector_context(payload, ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION) ||
	    payload.native_money.present || payload.native_money.original_room_vnum ||
	    payload.native_money.player_wallet_mapping_id ||
	    payload.native_money.mobile_wallet_mapping_id ||
	    payload.native_money.projection != native_quest_coin_give_projection{} ||
	    payload.expected_from_revision != native.reference.stock_revision ||
	    payload.expected_to_revision == UINT64_MAX || !recovery.consumed_root_order.empty())
		return false;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
	if (quest_mobile_native_reference_encode(native.reference, &reference) !=
	    player_snapshot_codec_result::ok)
		return false;
	if (payload.continuation.kind == item_transfer_continuation_kind::quest_offering)
	{
		quest_reward_continuation terms;
		if (!quest_fee_reward_continuation_decode(payload.continuation.data.data(),
							  payload.continuation.data.size(),
							  &terms) ||
		    terms.player_pid != native.final_giver_pid ||
		    terms.mobile_vnum != static_cast<uint32_t>(native.reference.mobile_vnum) ||
		    terms.action_mobile_instance_id != native.reference.mobile_instance_id ||
		    terms.completion_index != fee.completion_slot ||
		    terms.action_source.generation.bytes !=
			    native.reference.birth_source.generation.bytes ||
		    terms.action_source.sequence != native.reference.mobile_revision)
			return false;
	}
	else if (payload.continuation.kind != item_transfer_continuation_kind::none ||
		 !payload.continuation.data.empty())
		return false;
	if (!acknowledged)
		return !recovery.present && !recovery.player_pid &&
		       !recovery.acknowledged_save_revision &&
		       recovery.player_before == shop_trade_recovery_forest_binding{} &&
		       recovery.player_after == shop_trade_recovery_forest_binding{} &&
		       native_publication_terms_empty(recovery.publication_terms);
	return recovery.present && recovery.player_pid == native.final_giver_pid &&
	       recovery.acknowledged_save_revision &&
	       native_publication_terms_valid(recovery.publication_terms) &&
	       !recovery.publication_terms.disappear &&
	       recovery.player_before.canonical_bytes == recovery.player_after.canonical_bytes &&
	       recovery.player_before.ordered_item_uids ==
		       recovery.player_after.ordered_item_uids &&
	       shop_trade_recovery_forest_shape_valid(
		       recovery.player_before, shop_trade_recovery_forest_role::player_before) &&
	       shop_trade_recovery_forest_shape_valid(
		       recovery.player_after, shop_trade_recovery_forest_role::player_after);
}
bool encode_native_fee(const item_transfer_payload &payload, bool acknowledged,
		       std::vector<uint8_t> *output)
{
	if (!output || !native_fee_shape(payload, acknowledged))
		return false;
	std::vector<uint8_t> cost, recovery;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
	if (native_quest_cost_projection_encode(payload.native_cost.projection, &cost) !=
		    native_quest_cost_projection_result::ok ||
	    quest_mobile_native_reference_encode(payload.native_mobile.reference, &reference) !=
		    player_snapshot_codec_result::ok ||
	    (acknowledged && !encode_native_recovery(payload.native_recovery, &recovery)))
		return false;
	const size_t bytes = native_fee_header_bytes + reference.size() + cost.size() +
			     payload.continuation.data.size() + recovery.size();
	if (bytes > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	std::vector<uint8_t> value(native_fee_header_bytes, 0);
	value[0] = 'N';
	value[1] = 'Q';
	value[2] = 'F';
	value[3] = '2';
	put_u16(value.data() + 4, 1);
	put_u16(value.data() + 6,
		acknowledged ? ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION :
			       ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION);
	put_u32(value.data() + 8, static_cast<uint32_t>(bytes));
	put_u32(value.data() + 12, static_cast<uint32_t>(cost.size()));
	put_u32(value.data() + 16, static_cast<uint32_t>(payload.continuation.data.size()));
	put_u32(value.data() + 20, static_cast<uint32_t>(recovery.size()));
	put_u32(value.data() + 24, payload.native_mobile.final_giver_pid);
	put_u32(value.data() + 28, payload.native_cost.completion_slot);
	put_u64(value.data() + 32, payload.expected_from_revision);
	put_u64(value.data() + 40, payload.expected_to_revision);
	put_u64(value.data() + 48, payload.native_cost.wallet_mapping_id);
	put_u16(value.data() + 56, static_cast<uint16_t>(payload.continuation.kind));
	value.insert(value.end(), reference.begin(), reference.end());
	value.insert(value.end(), cost.begin(), cost.end());
	value.insert(value.end(), payload.continuation.data.begin(),
		     payload.continuation.data.end());
	value.insert(value.end(), recovery.begin(), recovery.end());
	*output = std::move(value);
	return true;
}
bool decode_native_fee(const critical_command &command, item_transfer_payload *output)
{
	constexpr size_t fixed = native_fee_header_bytes + QUEST_MOBILE_NATIVE_REFERENCE_BYTES;
	if (!output || !native_cost_version(command.payload_version) ||
	    command.type != critical_command_type::item_transfer ||
	    command.payload.size() < fixed ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	const auto *wire = command.payload.data();
	const bool acknowledged = command.payload_version ==
				  ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION;
	if (wire[0] != 'N' || wire[1] != 'Q' || wire[2] != 'F' || wire[3] != '2' ||
	    get_u16(wire + 4) != 1 || get_u16(wire + 6) != command.payload_version ||
	    get_u32(wire + 8) != command.payload.size() || get_u16(wire + 58) || get_u32(wire + 60))
		return false;
	const size_t cost_size = get_u32(wire + 12), continuation_size = get_u32(wire + 16),
		     recovery_size = get_u32(wire + 20);
	size_t remaining = command.payload.size() - fixed;
	if (cost_size > remaining)
		return false;
	remaining -= cost_size;
	if (continuation_size > remaining ||
	    continuation_size > ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
		return false;
	remaining -= continuation_size;
	if (recovery_size != remaining || (!acknowledged && recovery_size))
		return false;
	item_transfer_payload value{};
	value.native_mobile.present = true;
	value.native_mobile.action = item_native_mobile_action::consumption;
	value.native_mobile.final_giver_pid = get_u32(wire + 24);
	value.native_cost.present = value.native_cost.fee_only = true;
	value.native_cost.completion_slot = get_u32(wire + 28);
	value.native_cost.wallet_mapping_id = get_u64(wire + 48);
	value.expected_from_revision = get_u64(wire + 32);
	value.expected_to_revision = get_u64(wire + 40);
	if (quest_mobile_native_reference_decode(
		    { wire + native_fee_header_bytes, QUEST_MOBILE_NATIVE_REFERENCE_BYTES },
		    &value.native_mobile.reference) != player_snapshot_codec_result::ok ||
	    native_quest_cost_projection_decode({ wire + fixed, cost_size },
						&value.native_cost.projection) !=
		    native_quest_cost_projection_result::ok)
		return false;
	value.continuation.kind = static_cast<item_transfer_continuation_kind>(get_u16(wire + 56));
	value.continuation.data.assign(wire + fixed + cost_size,
				       wire + fixed + cost_size + continuation_size);
	if (acknowledged &&
	    !decode_native_recovery({ wire + fixed + cost_size + continuation_size, recovery_size },
				    &value.native_recovery))
		return false;
	value.from_owner = { item_owner_type::native_mobile,
			     value.native_mobile.reference.mobile_instance_id, 0 };
	value.to_owner = { item_owner_type::player, value.native_mobile.final_giver_pid, 0 };
	value.reason = item_transfer_reason::quest_turnin;
	value.reason_id = value.native_mobile.reference.mobile_vnum;
	std::vector<uint8_t> canonical;
	critical_command expected{};
	if (!encode_native_fee(value, acknowledged, &canonical) || canonical != command.payload ||
	    !populate_command_entities(&expected, value) ||
	    command.keys.size() != expected.keys.size() ||
	    command.expected_revisions.size() != expected.expected_revisions.size() ||
	    !std::equal(command.keys.begin(), command.keys.end(), expected.keys.begin(),
			critical_entity_key_equal) ||
	    !std::equal(command.expected_revisions.begin(), command.expected_revisions.end(),
			expected.expected_revisions.begin(),
			[](const auto &a, const auto &b) {
				return critical_entity_key_equal(a.key, b.key) &&
				       a.revision == b.revision;
			}))
		return false;
	if (value.continuation.kind == item_transfer_continuation_kind::quest_offering)
	{
		quest_reward_continuation terms;
		if (!quest_fee_reward_continuation_decode(value.continuation.data.data(),
							  value.continuation.data.size(), &terms) ||
		    terms.action_operation.bytes != command.operation_id.bytes)
			return false;
	}
	*output = std::move(value);
	return true;
}

bool encode_native_cost(const item_transfer_payload &payload, bool recovery,
			std::vector<uint8_t> *encoded)
{
	if (!encoded || !native_cost_value_valid(payload))
		return false;
	if (payload.native_cost.fee_only)
		return encode_native_fee(payload, recovery, encoded);
	if (payload.native_cost.completion_slot)
		return false;
	auto original = payload;
	original.native_cost = {};
	std::vector<uint8_t> body, cost;
	const uint16_t original_version =
		recovery ? ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION :
			   ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION;
	if (!encode_payload(original, original_version, &body) ||
	    native_quest_cost_projection_encode(payload.native_cost.projection, &cost) !=
		    native_quest_cost_projection_result::ok ||
	    body.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES -
				  ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES ||
	    cost.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES -
				  ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES - body.size())
		return false;
	std::vector<uint8_t> exact(ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES + body.size() +
				   cost.size());
	exact[0] = 'N';
	exact[1] = 'Q';
	exact[2] = 'F';
	exact[3] = '1';
	put_u16(exact.data() + 4, 1);
	put_u16(exact.data() + 6, original_version);
	put_u32(exact.data() + 8, static_cast<uint32_t>(body.size()));
	put_u32(exact.data() + 12, static_cast<uint32_t>(cost.size()));
	put_u64(exact.data() + 16, payload.native_cost.wallet_mapping_id);
	std::copy(body.begin(), body.end(),
		  exact.begin() + ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES);
	std::copy(cost.begin(), cost.end(),
		  exact.begin() + ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES + body.size());
	*encoded = std::move(exact);
	return true;
}
} // namespace

namespace
{
bool native_money_version(uint16_t version) noexcept
{
	return version == ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION ||
	       version == ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION;
}
bool native_money_shape(const item_transfer_payload &payload, bool acknowledged) noexcept
{
	const auto &money = payload.native_money;
	const auto &native = payload.native_mobile;
	const auto &recovery = payload.native_recovery;
	if (!money.present || money.original_room_vnum < 0 || !money.player_wallet_mapping_id ||
	    !money.mobile_wallet_mapping_id ||
	    money.player_wallet_mapping_id == money.mobile_wallet_mapping_id ||
	    payload.native_cost.present || payload.native_cost.fee_only ||
	    payload.native_cost.completion_slot || payload.native_cost.wallet_mapping_id ||
	    payload.native_cost.projection != native_quest_cost_projection{} || !native.present ||
	    native.action != item_native_mobile_action::acceptance || !native.final_giver_pid ||
	    native.final_giver_pid > INT32_MAX ||
	    payload.from_owner.type != item_owner_type::player ||
	    payload.from_owner.id != native.final_giver_pid || payload.from_owner.context_id ||
	    payload.to_owner.type != item_owner_type::native_mobile ||
	    payload.to_owner.id != native.reference.mobile_instance_id ||
	    payload.to_owner.context_id || payload.reason != item_transfer_reason::player_give ||
	    payload.reason_id != native.reference.mobile_vnum || payload.logical_source_id ||
	    payload.multi_root || payload.item_count || payload.item_blob_size ||
	    payload.selected_item_uid || payload.target_root_item_uid ||
	    payload.target_parent_item_uid || payload.expected_target_parent_revision ||
	    payload.corpse.present || payload.collector.present ||
	    !valid_collector_context(payload, ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION) ||
	    payload.continuation.kind != item_transfer_continuation_kind::none ||
	    !payload.continuation.data.empty() || payload.expected_from_revision == UINT64_MAX ||
	    payload.expected_to_revision != native.reference.stock_revision ||
	    !native_publication_terms_empty(recovery.publication_terms) ||
	    !recovery.consumed_root_order.empty())
		return false;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> encoded{};
	native_quest_coin_give_projection projected;
	if (quest_mobile_native_reference_encode(native.reference, &encoded) !=
		    player_snapshot_codec_result::ok ||
	    native.reference.mobile_revision == UINT64_MAX ||
	    native_quest_coin_give_project(
		    money.projection.player_before, money.projection.player_before_revision,
		    money.projection.mobile_before, money.projection.mobile_before_revision,
		    money.projection.denomination, money.projection.quantity,
		    &projected) != native_quest_coin_give_result::ok ||
	    projected != money.projection)
		return false;
	if (!acknowledged)
		return !recovery.present && !recovery.player_pid &&
		       !recovery.acknowledged_save_revision &&
		       recovery.player_before == shop_trade_recovery_forest_binding{} &&
		       recovery.player_after == shop_trade_recovery_forest_binding{};
	return recovery.present && recovery.player_pid == native.final_giver_pid &&
	       recovery.acknowledged_save_revision && recovery.player_before.present &&
	       recovery.player_after.present &&
	       recovery.player_before.canonical_bytes == recovery.player_after.canonical_bytes &&
	       recovery.player_before.ordered_item_uids ==
		       recovery.player_after.ordered_item_uids &&
	       shop_trade_recovery_forest_shape_valid(
		       recovery.player_before, shop_trade_recovery_forest_role::player_before) &&
	       shop_trade_recovery_forest_shape_valid(
		       recovery.player_after, shop_trade_recovery_forest_role::player_after);
}
bool encode_native_money(const item_transfer_payload &payload, bool acknowledged,
			 std::vector<uint8_t> *output)
{
	if (!output || !native_money_shape(payload, acknowledged))
		return false;
	std::vector<uint8_t> money, recovery;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
	if (native_quest_coin_give_encode(payload.native_money.projection, &money) !=
		    native_quest_coin_give_result::ok ||
	    quest_mobile_native_reference_encode(payload.native_mobile.reference, &reference) !=
		    player_snapshot_codec_result::ok ||
	    (acknowledged && !encode_native_recovery(payload.native_recovery, &recovery)))
		return false;
	const size_t bytes = ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES + reference.size() +
			     money.size() + recovery.size();
	if (bytes > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	std::vector<uint8_t> result(ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES, 0);
	result[0] = 'N';
	result[1] = 'Q';
	result[2] = 'M';
	result[3] = '1';
	put_u16(result.data() + 4, 1);
	put_u16(result.data() + 6,
		acknowledged ? ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION :
			       ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION);
	put_u32(result.data() + 8, static_cast<uint32_t>(bytes));
	put_u32(result.data() + 12, static_cast<uint32_t>(recovery.size()));
	put_u32(result.data() + 16, payload.native_mobile.final_giver_pid);
	put_u32(result.data() + 20, static_cast<uint32_t>(payload.native_money.original_room_vnum));
	put_u64(result.data() + 24, payload.expected_from_revision);
	put_u64(result.data() + 32, payload.expected_to_revision);
	put_u64(result.data() + 40, payload.native_money.player_wallet_mapping_id);
	put_u64(result.data() + 48, payload.native_money.mobile_wallet_mapping_id);
	result.insert(result.end(), reference.begin(), reference.end());
	result.insert(result.end(), money.begin(), money.end());
	result.insert(result.end(), recovery.begin(), recovery.end());
	*output = std::move(result);
	return true;
}
bool decode_native_money(const critical_command &command, item_transfer_payload *output)
{
	constexpr size_t fixed = ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES +
				 QUEST_MOBILE_NATIVE_REFERENCE_BYTES + NATIVE_QUEST_COIN_GIVE_BYTES;
	if (!output || !native_money_version(command.payload_version) ||
	    command.type != critical_command_type::item_transfer ||
	    command.payload.size() < fixed ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	const auto *wire = command.payload.data();
	const bool acknowledged = command.payload_version ==
				  ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION;
	if (wire[0] != 'N' || wire[1] != 'Q' || wire[2] != 'M' || wire[3] != '1' ||
	    get_u16(wire + 4) != 1 || get_u16(wire + 6) != command.payload_version ||
	    get_u32(wire + 8) != command.payload.size() || get_u32(wire + 20) > INT32_MAX ||
	    get_u64(wire + 56) || get_u32(wire + 12) != command.payload.size() - fixed ||
	    (!acknowledged && command.payload.size() != fixed))
		return false;
	item_transfer_payload candidate{};
	candidate.native_mobile.present = true;
	candidate.native_mobile.action = item_native_mobile_action::acceptance;
	candidate.native_mobile.final_giver_pid = get_u32(wire + 16);
	candidate.expected_from_revision = get_u64(wire + 24);
	candidate.expected_to_revision = get_u64(wire + 32);
	candidate.native_money.present = true;
	candidate.native_money.original_room_vnum = static_cast<int32_t>(get_u32(wire + 20));
	candidate.native_money.player_wallet_mapping_id = get_u64(wire + 40);
	candidate.native_money.mobile_wallet_mapping_id = get_u64(wire + 48);
	if (quest_mobile_native_reference_decode(
		    { wire + ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES,
		      QUEST_MOBILE_NATIVE_REFERENCE_BYTES },
		    &candidate.native_mobile.reference) != player_snapshot_codec_result::ok ||
	    native_quest_coin_give_decode({ wire + ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES +
						    QUEST_MOBILE_NATIVE_REFERENCE_BYTES,
					    NATIVE_QUEST_COIN_GIVE_BYTES },
					  &candidate.native_money.projection) !=
		    native_quest_coin_give_result::ok ||
	    (acknowledged &&
	     !decode_native_recovery(std::span<const uint8_t>(command.payload).subspan(fixed),
				     &candidate.native_recovery)))
		return false;
	candidate.from_owner = { item_owner_type::player, candidate.native_mobile.final_giver_pid,
				 0 };
	candidate.to_owner = { item_owner_type::native_mobile,
			       candidate.native_mobile.reference.mobile_instance_id, 0 };
	candidate.reason = item_transfer_reason::player_give;
	candidate.reason_id = candidate.native_mobile.reference.mobile_vnum;
	std::vector<uint8_t> canonical;
	critical_command expected{};
	if (!encode_native_money(candidate, acknowledged, &canonical) ||
	    canonical != command.payload || !populate_command_entities(&expected, candidate) ||
	    command.keys.size() != expected.keys.size() ||
	    command.expected_revisions.size() != expected.expected_revisions.size() ||
	    !std::equal(command.keys.begin(), command.keys.end(), expected.keys.begin(),
			critical_entity_key_equal) ||
	    !std::equal(command.expected_revisions.begin(), command.expected_revisions.end(),
			expected.expected_revisions.begin(),
			[](const auto &left, const auto &right) {
				return critical_entity_key_equal(left.key, right.key) &&
				       left.revision == right.revision;
			}))
		return false;
	*output = std::move(candidate);
	return true;
}
} // namespace

bool item_transfer_native_mobile_structural_version(uint16_t version) noexcept
{
	return version == ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION ||
	       version == ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION ||
	       version == ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION;
}
bool item_transfer_native_mobile_acknowledged_version(uint16_t version) noexcept
{
	return version == ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION ||
	       version == ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION ||
	       version == ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION;
}

bool item_transfer_native_mobile_shape_valid(const item_transfer_payload &payload) noexcept
{
	try
	{
		if (payload.native_money.present)
			return native_money_shape(payload, false);
		if (payload.native_cost.present)
		{
			std::vector<uint8_t> exact;
			return encode_native_cost(payload, false, &exact);
		}
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
		if (!(payload.native_money.present ?
			      encode_native_money(payload, false, &candidate) :
		      payload.native_cost.present ?
			      encode_native_cost(payload, false, &candidate) :
			      encode_payload(payload, ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION,
					     &candidate)))
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
		if (payload.native_money.present)
			return native_money_shape(payload, true);
		if (payload.native_cost.present)
		{
			std::vector<uint8_t> exact;
			return encode_native_cost(payload, true, &exact);
		}
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
		if (payload->native_money.present)
		{
			if (!native_money_shape(*payload, false) || !consumed_root_order.empty() ||
			    !native_publication_terms_empty(publication_terms))
				return false;
			auto candidate = *payload;
			auto &recovery = candidate.native_recovery;
			recovery.present = true;
			recovery.player_pid = player_pid;
			recovery.acknowledged_save_revision = acknowledged_save_revision;
			if (!shop_trade_recovery_forest_freeze(
				    original_player_items,
				    shop_trade_recovery_forest_role::player_before,
				    &recovery.player_before) ||
			    !shop_trade_recovery_forest_freeze(
				    original_player_items,
				    shop_trade_recovery_forest_role::player_after,
				    &recovery.player_after) ||
			    !native_money_shape(candidate, true))
				return false;
			*payload = std::move(candidate);
			return true;
		}
		if (payload->native_cost.fee_only)
		{
			if (!native_fee_shape(*payload, false) || !consumed_root_order.empty() ||
			    !native_publication_terms_valid(publication_terms) ||
			    publication_terms.disappear)
				return false;
			auto candidate = *payload;
			auto &r = candidate.native_recovery;
			r.present = true;
			r.player_pid = player_pid;
			r.acknowledged_save_revision = acknowledged_save_revision;
			r.publication_terms = publication_terms;
			if (!shop_trade_recovery_forest_freeze(
				    original_player_items,
				    shop_trade_recovery_forest_role::player_before,
				    &r.player_before) ||
			    !shop_trade_recovery_forest_freeze(
				    original_player_items,
				    shop_trade_recovery_forest_role::player_after,
				    &r.player_after) ||
			    !native_fee_shape(candidate, true))
				return false;
			*payload = std::move(candidate);
			return true;
		}
		if (payload->native_money.player_wallet_mapping_id ||
		    payload->native_money.mobile_wallet_mapping_id ||
		    payload->native_money.projection != native_quest_coin_give_projection{})
			return false;
		if (payload->native_cost.present && !native_cost_value_valid(*payload))
			return false;
		if (!payload->native_cost.present &&
		    (payload->native_cost.fee_only || payload->native_cost.completion_slot ||
		     payload->native_cost.wallet_mapping_id ||
		     payload->native_cost.projection != native_quest_cost_projection{}))
			return false;
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
		if (!(payload.native_money.present ?
			      encode_native_money(payload, true, &candidate) :
		      payload.native_cost.present ?
			      encode_native_cost(payload, true, &candidate) :
			      encode_payload(payload,
					     ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION,
					     &candidate)))
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
	if (native_money_version(command.payload_version))
	{
		try
		{
			return decode_native_money(command, payload);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	if (native_cost_version(command.payload_version))
	{
		if (command.payload.size() >= 4 && command.payload[0] == 'N' &&
		    command.payload[1] == 'Q' && command.payload[2] == 'F' &&
		    command.payload[3] == '2')
			try
			{
				return decode_native_fee(command, payload);
			}
			catch (const std::bad_alloc &)
			{
				return false;
			}
		if (!payload || command.type != critical_command_type::item_transfer ||
		    command.payload.size() < ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES ||
		    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
			return false;
		try
		{
			const auto *wire = command.payload.data();
			const uint16_t inner_version =
				command.payload_version ==
						ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION ?
					ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION :
					ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION;
			if (wire[0] != 'N' || wire[1] != 'Q' || wire[2] != 'F' || wire[3] != '1' ||
			    get_u16(wire + 4) != 1 || get_u16(wire + 6) != inner_version ||
			    !get_u64(wire + 16))
				return false;
			const size_t body_size = get_u32(wire + 8), cost_size = get_u32(wire + 12);
			const size_t available = command.payload.size() -
						 ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES;
			if (body_size > available || cost_size != available - body_size)
				return false;
			auto original = command;
			original.payload_version = inner_version;
			original.payload.assign(
				command.payload.begin() +
					ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES,
				command.payload.begin() +
					ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES + body_size);
			item_transfer_payload candidate{};
			if (!item_transfer_command_decode_payload(original, &candidate))
				return false;
			candidate.native_cost.present = true;
			candidate.native_cost.wallet_mapping_id = get_u64(wire + 16);
			if (native_quest_cost_projection_decode(
				    { wire + ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES +
					      body_size,
				      cost_size },
				    &candidate.native_cost.projection) !=
				    native_quest_cost_projection_result::ok ||
			    !native_cost_value_valid(candidate))
				return false;
			std::vector<uint8_t> canonical;
			if (!encode_native_cost(
				    candidate,
				    inner_version ==
					    ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION,
				    &canonical) ||
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
		critical_command candidate = {
			.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
			.operation_id = operation_id,
			.type = critical_command_type::item_transfer,
			.payload_version =
				payload.native_money.present ?
					ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION :
				payload.native_cost.present ?
					ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION :
					ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION,
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
			.payload_version =
				payload.native_money.present ?
					ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION :
				payload.native_cost.present ?
					ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION :
					ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION,
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

namespace
{
bool native_money_result_valid(const item_native_mobile_money_result &value) noexcept
{
	return value.mobile_instance_id && value.mobile_instance_id != UINT64_MAX &&
	       value.player_pid && value.player_pid <= INT32_MAX && value.mobile_cash_revision &&
	       value.mobile_revision && value.stock_revision &&
	       value.player_custody_revision != UINT64_MAX;
}
}
bool item_native_mobile_fee_result_build(const item_transfer_payload &payload,
					 item_native_mobile_fee_result *output) noexcept
try
{
	if (!output || !native_fee_shape(payload, true))
		return false;
	item_native_mobile_fee_result value{};
	value.mobile_instance_id = payload.native_mobile.reference.mobile_instance_id;
	value.player_pid = payload.native_mobile.final_giver_pid;
	value.mobile_cash_revision = payload.native_cost.projection.after_revision;
	value.mobile_revision = payload.native_mobile.reference.mobile_revision + 1;
	value.stock_revision = payload.native_mobile.reference.stock_revision;
	value.native_custody_revision = payload.expected_from_revision;
	value.player_custody_revision = payload.expected_to_revision;
	*output = value;
	return true;
}
catch (const std::bad_alloc &)
{
	return false;
}
bool item_native_mobile_fee_result_encode(
	const item_native_mobile_fee_result &value,
	std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES> *output) noexcept
{
	if (!output || !value.mobile_instance_id || value.mobile_instance_id == UINT64_MAX ||
	    !value.player_pid || value.player_pid > INT32_MAX || !value.mobile_cash_revision ||
	    !value.mobile_revision || value.stock_revision != value.native_custody_revision)
		return false;
	std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES> bytes{};
	bytes[0] = 'N';
	bytes[1] = 'F';
	bytes[2] = 'R';
	bytes[3] = '1';
	put_u16(bytes.data() + 4, 1);
	put_u64(bytes.data() + 8, value.mobile_instance_id);
	put_u32(bytes.data() + 16, value.player_pid);
	put_u64(bytes.data() + 24, value.mobile_cash_revision);
	put_u64(bytes.data() + 32, value.mobile_revision);
	put_u64(bytes.data() + 40, value.stock_revision);
	put_u64(bytes.data() + 48, value.native_custody_revision);
	put_u64(bytes.data() + 56, value.player_custody_revision);
	*output = bytes;
	return true;
}
bool item_native_mobile_fee_result_decode(std::span<const uint8_t> bytes,
					  item_native_mobile_fee_result *output) noexcept
{
	if (!output || bytes.size() != ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES ||
	    bytes[0] != 'N' || bytes[1] != 'F' || bytes[2] != 'R' || bytes[3] != '1' ||
	    get_u16(bytes.data() + 4) != 1 || get_u16(bytes.data() + 6) ||
	    get_u32(bytes.data() + 20))
		return false;
	item_native_mobile_fee_result value{};
	value.mobile_instance_id = get_u64(bytes.data() + 8);
	value.player_pid = get_u32(bytes.data() + 16);
	value.mobile_cash_revision = get_u64(bytes.data() + 24);
	value.mobile_revision = get_u64(bytes.data() + 32);
	value.stock_revision = get_u64(bytes.data() + 40);
	value.native_custody_revision = get_u64(bytes.data() + 48);
	value.player_custody_revision = get_u64(bytes.data() + 56);
	std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES> canonical{};
	if (!item_native_mobile_fee_result_encode(value, &canonical) ||
	    !std::equal(bytes.begin(), bytes.end(), canonical.begin()))
		return false;
	*output = value;
	return true;
}

bool item_native_mobile_money_result_build(const item_transfer_payload &payload,
					   item_native_mobile_money_result *output) noexcept
{
	if (!output || !native_money_shape(payload, payload.native_recovery.present))
		return false;
	const auto &reference = payload.native_mobile.reference;
	const auto &projection = payload.native_money.projection;
	const item_native_mobile_money_result result{
		reference.mobile_instance_id,	  payload.native_mobile.final_giver_pid,
		projection.player_after_revision, projection.mobile_after_revision,
		reference.mobile_revision + 1,	  payload.expected_from_revision,
		reference.stock_revision
	};
	if (!native_money_result_valid(result))
		return false;
	*output = result;
	return true;
}
bool item_native_mobile_money_result_encode(
	const item_native_mobile_money_result &value,
	std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RESULT_BYTES> *output) noexcept
{
	if (!output || !native_money_result_valid(value))
		return false;
	std::array<uint8_t, ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RESULT_BYTES> result{};
	result[0] = 'N';
	result[1] = 'Q';
	result[2] = 'R';
	result[3] = '1';
	put_u16(result.data() + 4, 1);
	put_u16(result.data() + 6, ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RESULT_BYTES);
	// Existing item decoder always refuses count zero at offset8, preserving
	// exact legacy result semantics even for small native instance identities.
	put_u64(result.data() + 16, value.mobile_instance_id);
	put_u32(result.data() + 24, value.player_pid);
	put_u64(result.data() + 32, value.player_wallet_revision);
	put_u64(result.data() + 40, value.mobile_cash_revision);
	put_u64(result.data() + 48, value.mobile_revision);
	put_u64(result.data() + 56, value.player_custody_revision);
	put_u64(result.data() + 64, value.stock_revision);
	*output = result;
	return true;
}
bool item_native_mobile_money_result_decode(std::span<const uint8_t> bytes,
					    item_native_mobile_money_result *output) noexcept
{
	if (!output || bytes.size() != ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RESULT_BYTES ||
	    bytes[0] != 'N' || bytes[1] != 'Q' || bytes[2] != 'R' || bytes[3] != '1' ||
	    get_u16(bytes.data() + 4) != 1 || get_u16(bytes.data() + 6) != bytes.size() ||
	    get_u64(bytes.data() + 8) || get_u32(bytes.data() + 28))
		return false;
	const item_native_mobile_money_result value{
		get_u64(bytes.data() + 16), get_u32(bytes.data() + 24), get_u64(bytes.data() + 32),
		get_u64(bytes.data() + 40), get_u64(bytes.data() + 48), get_u64(bytes.data() + 56),
		get_u64(bytes.data() + 64)
	};
	if (!native_money_result_valid(value))
		return false;
	*output = value;
	return true;
}
#include <type_traits>
namespace
{
using payload_clone_reserve_fn = bool (*)(size_t, void *) noexcept;
bool payload_clone_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
constexpr size_t payload_clone_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t payload_clone_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t payload_clone_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t payload_clone_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t payload_clone_vector_frames =
	payload_clone_allocator_frames + payload_clone_copy_frames + payload_clone_relocate_frames +
	payload_clone_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t payload_clone_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + payload_clone_allocator_frames;

// Fitting _M_replace calls _M_disjunct(this,s). Both actual less pointer
// temporaries can coexist through the full || expression; their operator()
// has this/x/y/result and is_constant_evaluated result. Data/size queries.
constexpr size_t payload_clone_disjunct_frames =
	2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(std::less<const char *>) +
	2 * (3 * sizeof(void *) + 2 * sizeof(bool)) + 2 * (2 * sizeof(void *)) + sizeof(void *) +
	sizeof(size_t);
constexpr size_t payload_clone_string_frames =
	payload_clone_disjunct_frames +
	// assign(s,n): this/s/n/ref-return; _M_replace(this,pos,len1,s,len2),
	// old_size/new_size/p/how_much/ref-return, actual length checks/queries.
	3 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + 5 * sizeof(size_t) +
	6 * (sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	// _M_mutate(this,pos,len1,s,len2), how_much/new_capacity/r;
	// _M_create(this,capacityref,oldcapacity), max_size, allocation return.
	3 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	payload_clone_allocator_frames +
	// _S_copy(d,s,n), traits::copy(s1,s2,n) returned pointer and memcopy
	// argument/result carriers; one-character assign reference/char scopes.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(void *) + sizeof(char) +
	// old block dispose/destroy plus data/capacity/set-length and final NUL.
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + sizeof(char);

// basic_string copy constructor: this/source, allocator select/copy result,
// allocator hider/local-data, _M_construct forward this/beg/end/tag, dnew,
// its real one-pointer _Guard and constructor/destructor this parameters;
// distance/__distance and returned difference, _S_copy_chars arguments.
// Existing string/allocator profiles own _M_create( n,0 ), data/capacity/
// set-length, traits copy, runtime memcpy and unwind disposal.
constexpr size_t payload_clone_string_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<char>) + 6 * sizeof(void *) +
	3 * sizeof(void *) + sizeof(std::forward_iterator_tag) + sizeof(size_t) + sizeof(void *) +
	3 * sizeof(void *) + 2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) +
	sizeof(std::random_access_iterator_tag) + 4 * sizeof(void *) + payload_clone_string_frames;
// vector copy constructor calls _Vector_base(size,selected_allocator), then
// __uninitialized_copy_a; the allocated capacity is exactly source.size().
// Actual constructor/base/impl/data/create-storage/select/query carriers.
// Existing vector allocator/copy profiles own the trivial-element path.
constexpr size_t payload_clone_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<int32_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + payload_clone_vector_frames;
// String move assignment/operator and _M_assign path are allocation-free
// for the standard equal allocator, including the actual _M_is_local tests,
// old-pointer/old-capacity temporaries, memcpy args and source reset. The
// existing string profile conservatively also retains all disposal scopes.
constexpr size_t payload_clone_string_move_frames =
	2 * sizeof(void *) + sizeof(char) + sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	12 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(bool) + 2 * sizeof(void *) +
	sizeof(char) + payload_clone_string_frames;

constexpr size_t payload_clone_observation_frames = 12 * sizeof(void *) + 8 * sizeof(size_t) +
						    5 * sizeof(bool) +
						    12 * (sizeof(void *) + sizeof(size_t));
// Actual generated payload/corpse/collector/continuation/recovery/forest/
// publication/cost/projection copy and move this/source scopes. Fixed arrays
// belong to the actual payload object; none allocates. Six string and six
// vector copies have the identical actual constructors profiled above.
constexpr size_t payload_clone_frames =
	payload_clone_observation_frames +
	// Ten actual nontrivial generated aggregate copy + move scopes:
	// payload, corpse, collector, continuation, recovery, two forests,
	// publication, cost context and cost projection. Each copy and move has
	// this/source; each destructor has this. Native mobile/money/owners and
	// fixed arrays are trivial member copies with no allocating closure.
	(1 + 1 + 1 + 1 + 1 + 2 + 1 + 1 + 1) * (4 * sizeof(void *) + sizeof(void *)) +
	6 * payload_clone_string_constructor_frames + 6 * payload_clone_vector_constructor_frames +
	6 * payload_clone_string_move_frames + 6 * payload_clone_move_frames +
	// Actual string destructor/dispose/local/destroy and six vector destructor
	// parameter/source scopes, with full real allocator/delete closure.
	6 * (5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	6 * payload_clone_allocator_frames;
template <typename T>
bool payload_clone_vector_heap(const std::vector<T> &value, bool fresh, size_t &bytes) noexcept
{
	const size_t count = fresh ? value.size() : value.capacity();
	return count <= SIZE_MAX / sizeof(T) && payload_clone_add(bytes, count * sizeof(T));
}
bool payload_clone_string_heap(const std::string &value, bool fresh, size_t &bytes) noexcept
{
	const size_t count = fresh ? value.size() : value.capacity();
	return count <= 15 || (count < SIZE_MAX && payload_clone_add(bytes, count + 1));
}
bool payload_clone_heap(const item_transfer_payload &value, bool fresh, size_t &bytes) noexcept
{
	return payload_clone_string_heap(value.corpse.owner_name, fresh, bytes) &&
	       payload_clone_string_heap(value.corpse.short_description, fresh, bytes) &&
	       payload_clone_string_heap(value.corpse.description, fresh, bytes) &&
	       payload_clone_string_heap(value.corpse.keywords, fresh, bytes) &&
	       payload_clone_string_heap(value.native_recovery.publication_terms.message, fresh,
					 bytes) &&
	       payload_clone_string_heap(value.native_recovery.publication_terms.disappear_message,
					 fresh, bytes) &&
	       payload_clone_vector_heap(value.collector.eligible_item_uids, fresh, bytes) &&
	       payload_clone_vector_heap(value.continuation.data, fresh, bytes) &&
	       payload_clone_vector_heap(value.native_recovery.player_before.ordered_item_uids,
					 fresh, bytes) &&
	       payload_clone_vector_heap(value.native_recovery.player_after.ordered_item_uids,
					 fresh, bytes) &&
	       payload_clone_vector_heap(value.native_recovery.consumed_root_order, fresh, bytes) &&
	       payload_clone_vector_heap(value.native_cost.projection.attempts, fresh, bytes);
}
bool payload_clone_policy_supported() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	return true;
#else
	return false;
#endif
}
} // namespace

bool item_transfer_payload_current_heap_bytes(const item_transfer_payload &value,
					      size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t bytes = 0;
	if (!payload_clone_heap(value, false, bytes))
		return false;
	*output = bytes;
	return true;
}
bool item_transfer_payload_fresh_copy_request_bytes(const item_transfer_payload &value,
						    size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t bytes = 0;
	if (!payload_clone_heap(value, true, bytes))
		return false;
	*output = bytes;
	return true;
}
size_t item_transfer_payload_copy_frame_bytes() noexcept
{
	return payload_clone_frames;
}
bool item_transfer_payload_clone_bounded(const item_transfer_payload &source,
					 item_transfer_payload *output,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer) noexcept
{
	if (!output || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t own = sizeof(item_transfer_payload) + 4 * sizeof(void *) +
			       3 * sizeof(size_t) + sizeof(bool) + payload_clone_observation_frames;
	size_t peak = outer;
	if (!payload_clone_add(peak, own) || !reserve(peak, context))
		return false;
	size_t request = 0;
	if (!item_transfer_payload_fresh_copy_request_bytes(source, &request) ||
	    !payload_clone_add(peak, request) || !payload_clone_add(peak, payload_clone_frames) ||
	    !reserve(peak, context))
		return false;
	try
	{
		item_transfer_payload candidate(source);
		static_assert(std::is_nothrow_move_assignable_v<item_transfer_payload>);
		// Prior output remains outer-owned through actual nonthrowing move
		// and destruction, including swapped surviving old string heaps.
		*output = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
namespace
{
using item_sidecar_reserve_fn = bool (*)(size_t, void *) noexcept;
// vector(n,a), actual default allocator temporary, _S_check_init_len n/a/return
// and allocator copy, _Vector_base this/n/a, _Vector_impl this/a/copy,
// _Vector_impl_data this and _M_create_storage this/n. The existing vector
// allocator profile owns _S_max_size and allocation scopes.
constexpr size_t item_sidecar_size_constructor_frames =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<uint8_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t);

// Original collector fill-assign's fresh allocation path is separate from
// generic reserve/forward-copy profiles. Actual named fill scopes are charged
// at the genuine owning cut immediately before assign(n,0).
constexpr size_t item_sidecar_fill_assign_frames =
	// assign(n,value) and _M_fill_assign(this,n,val), actual vector __tmp;
	// capacity/get-allocator references and resulting values.
	2 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(std::vector<uint8_t>) +
	4 * sizeof(void *) + sizeof(size_t) +
	// vector(n,val,a) this/n/val/a; _M_fill_initialize this/n/value.
	3 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) + sizeof(size_t) +
	// __uninitialized_fill_n_a first/n/value/allocator/return;
	// uninitialized_fill_n and __uninit_fill_n<true>, real __can_fill.
	4 * sizeof(void *) + sizeof(size_t) + 2 * (3 * sizeof(void *) + sizeof(size_t)) +
	sizeof(bool) +
	// fill_n/__size_to_integer/__fill_n_a/__fill_a/__fill_a1<unsignedchar>:
	// real first/n/value/return/tag and runtime uchar temporary/memset args.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(std::random_access_iterator_tag) +
	2 * sizeof(size_t) + 2 * (3 * sizeof(void *)) + sizeof(uint8_t) + sizeof(void *) +
	sizeof(int) + sizeof(size_t) + sizeof(void *) +
	// Actual _M_swap_data/_M_copy_data temporary with three pointer fields,
	// source references and destructor's true allocation-free closure.
	3 * sizeof(void *) + 2 * 2 * sizeof(void *) + payload_clone_move_frames;
constexpr size_t item_sidecar_pure_frames =
	// Original fixed get/put32/64/read_u32, byte loops/values/offsets and
	// std::copy/copy_n/runtime memmove scopes (already profiled in vector).
	8 * sizeof(void *) + 8 * sizeof(size_t) + 4 * sizeof(uint32_t) + 2 * sizeof(uint64_t) +
	sizeof(int32_t) + sizeof(bool) + payload_clone_copy_frames;
struct item_sidecar_budget
{
	item_sidecar_reserve_fn reserve;
	void *context;
	size_t outer, frames;
	const item_corpse_metadata *corpse = nullptr;
	const item_collector_death_enrollment *collector = nullptr;
	const std::vector<uint8_t> *bytes = nullptr;
	bool prefix(size_t &output, size_t extra = 0) const noexcept
	{
		size_t total = outer;
		constexpr size_t observation =
			10 * sizeof(void *) + 7 * sizeof(size_t) + 4 * sizeof(bool);
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    (corpse &&
		     (!payload_clone_string_heap(corpse->owner_name, false, total) ||
		      !payload_clone_string_heap(corpse->short_description, false, total) ||
		      !payload_clone_string_heap(corpse->description, false, total) ||
		      !payload_clone_string_heap(corpse->keywords, false, total))) ||
		    (collector &&
		     !payload_clone_vector_heap(collector->eligible_item_uids, false, total)) ||
		    (bytes && !payload_clone_vector_heap(*bytes, false, total)) ||
		    !payload_clone_add(total, observation) || !payload_clone_add(total, extra))
			return false;
		output = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	template <typename T> bool fresh(size_t count, size_t extra) const noexcept
	{
		constexpr size_t own = sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
		return count <= SIZE_MAX / sizeof(T) &&
		       payload_clone_add(extra, count * sizeof(T)) &&
		       payload_clone_add(extra, own) && peak(extra);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count,
					  size_t caller_frames = 0) const noexcept
	{
		constexpr size_t own = 2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
		size_t request = payload_clone_vector_frames;
		if (count > value.max_size() - value.size())
			return false;
		if (value.size() + count > value.capacity())
		{
			size_t next = value.size();
			if (!payload_clone_add(next, std::max(value.size(), count)) ||
			    next > value.max_size())
				next = value.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !payload_clone_add(request, next * sizeof(T)))
				return false;
		}
		return payload_clone_add(request, own) &&
		       payload_clone_add(request, caller_frames) && peak(request);
	}
	bool text(std::string &value, const char *source, size_t length, size_t caller_frames) const
	{
		constexpr size_t own = 3 * sizeof(void *) + 5 * sizeof(size_t) + sizeof(bool);
		size_t request = payload_clone_string_frames;
		if (length > value.capacity())
		{
			const size_t capacity = value.capacity();
			size_t next = length;
			if (capacity > SIZE_MAX / 2)
				return false;
			if (next < 2 * capacity)
				next = 2 * capacity;
			if (next > value.max_size())
				next = value.max_size();
			if (next == SIZE_MAX || !payload_clone_add(request, next + 1))
				return false;
		}
		if (!payload_clone_add(request, own) ||
		    !payload_clone_add(request, caller_frames) || !peak(request))
			return false;
		value.assign(source, length);
		return true;
	}
};
bool item_sidecar_append_u32(item_sidecar_budget &owner, std::vector<uint8_t> *output,
			     uint32_t value)
{
	if (!output)
		return false;
	constexpr size_t own =
		2 * sizeof(void *) + sizeof(uint32_t) + sizeof(size_t) + sizeof(bool);
	try
	{
		const size_t offset = output->size();
		if (!owner.growth(*output, sizeof(value), own))
			return false;
		output->resize(offset + sizeof(value));
		put_u32(output->data() + offset, value);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}
bool item_sidecar_append_text(item_sidecar_budget &owner, std::vector<uint8_t> *output,
			      const std::string &value)
{
	constexpr size_t own = 3 * sizeof(void *) + sizeof(bool);
	owner.frames += own;
	// Scalar end restores the genuine frame on every original return.
	struct frame_end
	{
		item_sidecar_budget &owner;
		size_t bytes;
		~frame_end() { owner.frames -= bytes; }
	};
	owner.frames += sizeof(frame_end) + sizeof(void *);
	frame_end tail{ owner, own + sizeof(frame_end) + sizeof(void *) };
	if (!output || value.size() > UINT32_MAX ||
	    !item_sidecar_append_u32(owner, output, static_cast<uint32_t>(value.size())))
		return false;
	try
	{
		if (!owner.growth(*output, value.size()))
			return false;
		output->insert(output->end(), value.begin(), value.end());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}
bool item_sidecar_read_text(item_sidecar_budget &owner, const uint8_t *input, size_t size,
			    size_t *offset, size_t maximum, std::string *value)
{
	constexpr size_t own =
		4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(uint32_t) + sizeof(bool);
	uint32_t length = 0;
	if (!value || !read_u32(input, size, offset, &length) || length > maximum ||
	    *offset > size || size - *offset < length)
		return false;
	try
	{
		if (!owner.text(*value, reinterpret_cast<const char *>(input + *offset), length,
				own))
			return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*offset += length;
	return true;
}
bool item_sidecar_encode_corpse_context(const item_corpse_metadata &corpse,
					std::vector<uint8_t> *encoded, item_sidecar_budget &owner)
{
	if (!encoded)
		return false;
	encoded->clear();
	if (!corpse.present)
		return true;
	if (!item_sidecar_append_u32(owner, encoded, CORPSE_CONTEXT_VERSION) ||
	    !item_sidecar_append_u32(owner, encoded, static_cast<uint32_t>(corpse.room_vnum)) ||
	    !item_sidecar_append_u32(owner, encoded, static_cast<uint32_t>(corpse.weight)) ||
	    !item_sidecar_append_u32(owner, encoded, corpse.actor_racewar))
		return false;
	for (int32_t value : corpse.values)
		if (!item_sidecar_append_u32(owner, encoded, static_cast<uint32_t>(value)))
			return false;
	return item_sidecar_append_text(owner, encoded, corpse.owner_name) &&
	       item_sidecar_append_text(owner, encoded, corpse.short_description) &&
	       item_sidecar_append_text(owner, encoded, corpse.description) &&
	       item_sidecar_append_text(owner, encoded, corpse.keywords);
}

bool item_sidecar_decode_corpse_context(const uint8_t *encoded, size_t size,
					item_corpse_metadata *corpse, item_sidecar_budget &owner)
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
	return item_sidecar_read_text(owner, encoded, size, &offset,
				      ITEM_TRANSFER_CORPSE_NAME_MAX_BYTES, &corpse->owner_name) &&
	       item_sidecar_read_text(owner, encoded, size, &offset,
				      ITEM_TRANSFER_CORPSE_SHORT_DESCRIPTION_MAX_BYTES,
				      &corpse->short_description) &&
	       item_sidecar_read_text(owner, encoded, size, &offset,
				      ITEM_TRANSFER_CORPSE_DESCRIPTION_MAX_BYTES,
				      &corpse->description) &&
	       item_sidecar_read_text(owner, encoded, size, &offset,
				      ITEM_TRANSFER_CORPSE_KEYWORDS_MAX_BYTES, &corpse->keywords) &&
	       offset == size;
}

bool item_sidecar_encode_collector_context(const item_collector_death_enrollment &collector,
					   std::vector<uint8_t> *encoded,
					   item_sidecar_budget &owner)
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
		const size_t count =
			fixed_size + collector.eligible_item_uids.size() * sizeof(uint64_t);
		if (!owner.fresh<uint8_t>(count, payload_clone_vector_frames +
							 item_sidecar_size_constructor_frames +
							 item_sidecar_fill_assign_frames +
							 sizeof(std::allocator<uint8_t>) +
							 sizeof(uint8_t)))
			return false;
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

bool item_sidecar_decode_collector_context(const uint8_t *encoded, size_t size,
					   item_collector_death_enrollment *collector,
					   item_sidecar_budget &owner)
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
		if (!owner.fresh<uint64_t>(count, payload_clone_vector_frames))
			return false;
		collector->eligible_item_uids.reserve(count);
		for (uint32_t index = 0; index < count; ++index)
		{
			if (!owner.growth(collector->eligible_item_uids, 1))
				return false;
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

} // namespace

bool item_transfer_corpse_context_encode_bounded(const item_corpse_metadata &corpse,
						 std::vector<uint8_t> *encoded,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer) noexcept
{
	if (!encoded || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = sizeof(std::vector<uint8_t>) + 7 * sizeof(void *) +
				  4 * sizeof(size_t) + sizeof(uint64_t) + sizeof(int32_t) +
				  sizeof(bool) + item_sidecar_pure_frames +
				  payload_clone_vector_frames + payload_clone_move_frames;
	item_sidecar_budget owner{ reserve, context, outer, frames };
	if (!owner.peak())
		return false;
	try
	{
		std::vector<uint8_t> candidate;
		owner.bytes = &candidate;
		if (!item_sidecar_encode_corpse_context(corpse, &candidate, owner) ||
		    !owner.peak(payload_clone_move_frames))
			return false;
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_corpse_context_decode_bounded(const uint8_t *encoded, size_t size,
						 item_corpse_metadata *corpse,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer) noexcept
{
	if (!corpse || (!encoded && size) || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		2 * sizeof(item_corpse_metadata) + 8 * sizeof(void *) + 6 * sizeof(size_t) +
		5 * sizeof(uint32_t) + sizeof(uint64_t) + sizeof(int32_t) + sizeof(bool) +
		item_sidecar_pure_frames +
		4 * (payload_clone_string_constructor_frames + payload_clone_string_move_frames);
	item_sidecar_budget owner{ reserve, context, outer, frames };
	if (!owner.peak())
		return false;
	try
	{
		item_corpse_metadata candidate;
		owner.corpse = &candidate;
		if (!item_sidecar_decode_corpse_context(encoded, size, &candidate, owner) ||
		    !owner.peak())
			return false;
		static_assert(std::is_nothrow_move_assignable_v<item_corpse_metadata>);
		*corpse = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_collector_context_encode_bounded(
	const item_collector_death_enrollment &collector, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!encoded || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		sizeof(std::vector<uint8_t>) + 7 * sizeof(void *) + 4 * sizeof(size_t) +
		sizeof(uint64_t) + sizeof(int32_t) + sizeof(bool) + item_sidecar_pure_frames +
		payload_clone_vector_frames + payload_clone_move_frames + sizeof(size_t);
	item_sidecar_budget owner{ reserve, context, outer, frames };
	if (!owner.peak())
		return false;
	try
	{
		std::vector<uint8_t> candidate;
		owner.bytes = &candidate;
		if (!item_sidecar_encode_collector_context(collector, &candidate, owner) ||
		    !owner.peak(payload_clone_move_frames))
			return false;
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_collector_context_decode_bounded(const uint8_t *encoded, size_t size,
						    item_collector_death_enrollment *collector,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer) noexcept
{
	if (!collector || (!encoded && size) || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 2 * sizeof(item_collector_death_enrollment) + 8 * sizeof(void *) +
				  6 * sizeof(size_t) + 5 * sizeof(uint32_t) + sizeof(uint64_t) +
				  sizeof(int32_t) + sizeof(bool) + item_sidecar_pure_frames +
				  payload_clone_vector_frames + payload_clone_move_frames;
	item_sidecar_budget owner{ reserve, context, outer, frames };
	if (!owner.peak())
		return false;
	try
	{
		item_collector_death_enrollment candidate;
		owner.collector = &candidate;
		if (!item_sidecar_decode_collector_context(encoded, size, &candidate, owner) ||
		    !owner.peak())
			return false;
		static_assert(std::is_nothrow_move_assignable_v<item_collector_death_enrollment>);
		*collector = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
// Pinned OpenSSL3.0.13 Linux x86_64 SHA256 allocator-free source closure.
constexpr size_t item_key_sha_assembly_frames =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t item_key_sha_c_small_frames = 16 * sizeof(unsigned int) +
					       12 * sizeof(unsigned int) + sizeof(unsigned int) +
					       sizeof(int) + sizeof(const uint8_t *);
constexpr size_t item_key_sha_c_normal_frames = 16 * sizeof(unsigned int) +
						11 * sizeof(unsigned int) + 2 * sizeof(int) +
						2 * sizeof(void *);
constexpr size_t item_key_sha_init_frames = sizeof(void *) + sizeof(int);
constexpr size_t item_key_sha_update_frames = 2 * sizeof(void *) + sizeof(size_t) +
					      2 * sizeof(void *) + sizeof(unsigned int) +
					      sizeof(size_t) + sizeof(int);
constexpr size_t item_key_sha_final_frames = 3 * sizeof(void *) + sizeof(size_t) +
					     sizeof(unsigned long) + sizeof(unsigned int) +
					     sizeof(int);
[[maybe_unused]] constexpr size_t item_key_sha_frames =
	std::max(item_key_sha_assembly_frames,
		 std::max(item_key_sha_c_small_frames, item_key_sha_c_normal_frames)) +
	std::max(item_key_sha_init_frames,
		 std::max(item_key_sha_update_frames, item_key_sha_final_frames));
}

// Same original identity bytes and little-endian digest projection. No source,
// admission or custody authority, native state, RNG or retained heap.
bool item_owner_key_bounded(const item_owner_identity &owner, critical_entity_key *key,
			    bool (*reserve)(size_t, void *) noexcept, void *context,
			    size_t outer_live) noexcept
{
	constexpr size_t parameters = 4 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
	constexpr size_t fixed_frames =
		parameters + sizeof(size_t) + sizeof(critical_entity_key) +
		// Original identity predicate owner-reference/result; entity conversion
		// type parameter and returned type; genuine braced key assignment carrier.
		sizeof(void *) + sizeof(bool) + sizeof(item_owner_type) +
		sizeof(critical_entity_type) + sizeof(critical_entity_key) +
		// Actual add/admit argument/result scopes, no allocation.
		4 * sizeof(size_t) + 3 * sizeof(void *) + 2 * sizeof(bool);
	size_t base = outer_live;
	if (!key || !reserve || !payload_clone_add(base, fixed_frames) || !reserve(base, context) ||
	    !item_owner_identity_valid(owner))
		return false;
	critical_entity_key candidate{};
	if (owner.id && !owner.context_id)
	{
		candidate = { entity_type_for_owner(owner.type), owner.id };
		*key = candidate;
		return true;
	}
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) &&     \
	defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 &&  \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&  \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return false;
	struct workspace
	{
		std::array<uint8_t, 17> encoded{};
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
		SHA256_CTX digest_context;
		uint64_t identity = 0;
		bool hashed = false;
	};
	constexpr size_t leaf_frames =
		// Original encode_owner(output,owner), nested put_u64(output,value,byte).
		2 * sizeof(void *) + sizeof(void *) + sizeof(uint64_t) + sizeof(unsigned int) +
		// Original get_u64(input,value,byte) and distinct returned uint64 carrier.
		sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(unsigned int) +
		// Array data/_S_ptr parameter-return scopes and size this/result.
		4 * (2 * sizeof(void *)) + sizeof(void *) + sizeof(size_t);
	if (!payload_clone_add(base, sizeof(workspace)) ||
	    !payload_clone_add(base, leaf_frames + item_key_sha_frames) || !reserve(base, context))
		return false;
	workspace work;
	encode_owner(work.encoded.data(), owner);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	work.hashed = SHA256_Init(&work.digest_context) == 1 &&
		      SHA256_Update(&work.digest_context, work.encoded.data(),
				    work.encoded.size()) == 1 &&
		      SHA256_Final(work.digest.data(), &work.digest_context) == 1;
#pragma GCC diagnostic pop
	if (!work.hashed)
		return false;
	work.identity = get_u64(work.digest.data());
	if (!work.identity)
		work.identity = 1;
	candidate = { entity_type_for_owner(owner.type), work.identity };
	*key = candidate;
	return true;
#else
	return false;
#endif
}

namespace
{
using item_native_recovery_reserve_fn = bool (*)(size_t, void *) noexcept;
constexpr size_t item_native_recovery_fixed_frames =
	// Original fixed get/put16/32/64 params/values/byte/results, two span
	// subspan by-value carriers and its this/offset/count/newspan scopes.
	8 * sizeof(void *) + 4 * sizeof(uint16_t) + 6 * sizeof(uint32_t) + 4 * sizeof(uint64_t) +
	4 * sizeof(unsigned int) + 2 * sizeof(std::span<const uint8_t>) + 6 * sizeof(size_t) +
	// Original publication predicate: reference/result, size/empty queries,
	// find(char,pos) this/char/pos/size/data/result; traits find s/n/charref
	// and pointer result; real runtime memchr args/result. No NUL copies.
	6 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(bool) + sizeof(char) +
	3 * sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(int) + sizeof(size_t) +
	// Range/vector/string size/data/begin/end, normal-iterator and actual
	// pointer/string null terminator query carriers; no heap.
	8 * (sizeof(void *) + sizeof(size_t));
// Genuine original vector(n,value,allocator) fill constructor, not substitute
// reserve or an encoded-byte estimate. Existing profiles cover allocation.
constexpr size_t item_native_recovery_fill_constructor_frames =
	// Constructor this/n/value/a; _S_check_init_len and _Vector_base(n,a),
	// actual copied allocator, _Vector_impl(a), _Vector_impl_data and create.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<uint8_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t) +
	// _M_fill_initialize and __uninitialized_fill_n_a; real value/allocator.
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(size_t) +
	// uninitialized_fill_n/__uninit_fill_n<true>, actual __can_fill,
	// fill_n/__fill_n_a/__fill_a/__fill_a1 runtime uchar/memset carriers.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(std::random_access_iterator_tag) +
	2 * sizeof(size_t) + 2 * (3 * sizeof(void *)) + sizeof(uint8_t) + sizeof(void *) +
	sizeof(int) + sizeof(size_t) + sizeof(void *) + payload_clone_vector_frames;
constexpr size_t item_native_recovery_default_frames =
	// Real recovery/2 forests/publication aggregate constructor/destructor
	// this parameters, three vector/base/impl/data default constructors,
	// and two string/hider/local-data/set-length/traits-NUL constructors.
	8 * sizeof(void *) + 3 * (4 * sizeof(void *) + sizeof(std::allocator<uint64_t>)) +
	2 * (7 * sizeof(void *) + sizeof(std::allocator<char>) + sizeof(size_t) + sizeof(char));
struct item_native_recovery_budget
{
	item_native_recovery_reserve_fn reserve;
	void *context;
	size_t outer, frames;
	const std::vector<uint8_t> *before = nullptr, *after = nullptr, *bytes = nullptr;
	const item_native_mobile_recovery_context *recovery = nullptr;
	bool prefix(size_t &output, size_t extra = 0) const noexcept
	{
		constexpr size_t observation = 12 * sizeof(void *) + 7 * sizeof(size_t) +
					       4 * sizeof(bool) +
					       5 * (sizeof(void *) + sizeof(size_t));
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    !payload_clone_add(total, observation) ||
		    (before && (!payload_clone_add(total, sizeof(*before)) ||
				!payload_clone_vector_heap(*before, false, total))) ||
		    (after && (!payload_clone_add(total, sizeof(*after)) ||
			       !payload_clone_vector_heap(*after, false, total))) ||
		    (bytes && (!payload_clone_add(total, sizeof(*bytes)) ||
			       !payload_clone_vector_heap(*bytes, false, total))) ||
		    (recovery &&
		     (!payload_clone_add(total, sizeof(*recovery)) ||
		      !payload_clone_vector_heap(recovery->player_before.ordered_item_uids, false,
						 total) ||
		      !payload_clone_vector_heap(recovery->player_after.ordered_item_uids, false,
						 total) ||
		      !payload_clone_vector_heap(recovery->consumed_root_order, false, total) ||
		      !payload_clone_string_heap(recovery->publication_terms.message, false,
						 total) ||
		      !payload_clone_string_heap(recovery->publication_terms.disappear_message,
						 false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		output = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count) const noexcept
	{
		constexpr size_t own = 2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
		size_t request = payload_clone_vector_frames;
		if (count > value.max_size() - value.size())
			return false;
		if (count > value.capacity() - value.size())
		{
			size_t next = value.size();
			if (!payload_clone_add(next, std::max(value.size(), count)) ||
			    next > value.max_size())
				next = value.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !payload_clone_add(request, next * sizeof(T)))
				return false;
		}
		return payload_clone_add(request, own) && peak(request);
	}
	template <typename T> bool fresh(size_t count) const noexcept
	{
		constexpr size_t own = sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
		size_t request = payload_clone_vector_frames;
		return count <= SIZE_MAX / sizeof(T) &&
		       payload_clone_add(request, count * sizeof(T)) &&
		       payload_clone_add(request, own) && peak(request);
	}
	bool text(std::string &value, const char *source, size_t length) const
	{
		constexpr size_t own = 3 * sizeof(void *) + 5 * sizeof(size_t) + sizeof(bool);
		size_t request = payload_clone_string_frames;
		if (length > value.capacity())
		{
			const size_t capacity = value.capacity();
			size_t next = length;
			if (capacity > SIZE_MAX / 2)
				return false;
			if (next < 2 * capacity)
				next = 2 * capacity;
			if (next > value.max_size())
				next = value.max_size();
			if (next == SIZE_MAX || !payload_clone_add(request, next + 1))
				return false;
		}
		if (!payload_clone_add(request, own) || !peak(request))
			return false;
		value.assign(source, length);
		return true;
	}
};
bool item_native_recovery_encode_owned(const item_native_mobile_recovery_context &recovery,
				       std::vector<uint8_t> *out,
				       item_native_recovery_budget &owner,
				       size_t *retained_encoded_heap_bytes)
{
	if (!owner.peak(2 * sizeof(std::vector<uint8_t>) +
			2 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>))))
		return false;
	std::vector<uint8_t> before, after;
	owner.before = &before;
	owner.after = &after;
	size_t prefix = 0;
	if (!owner.prefix(prefix))
		return false;
	if (!shop_trade_recovery_forest_encode_bounded(
		    recovery.player_before, shop_trade_recovery_forest_role::player_before, &before,
		    owner.reserve, owner.context, prefix, nullptr) ||
	    !owner.prefix(prefix) ||
	    !shop_trade_recovery_forest_encode_bounded(
		    recovery.player_after, shop_trade_recovery_forest_role::player_after, &after,
		    owner.reserve, owner.context, prefix, nullptr))
		return false;
	if (!owner.peak(sizeof(std::vector<uint8_t>) +
			item_native_recovery_fill_constructor_frames +
			ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_HEADER_BYTES + sizeof(uint8_t)))
		return false;
	std::vector<uint8_t> candidate(ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_HEADER_BYTES, 0);
	owner.bytes = &candidate;
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
	if (!owner.growth(candidate, before.size()))
		return false;
	candidate.insert(candidate.end(), before.begin(), before.end());
	if (!owner.growth(candidate, after.size()))
		return false;
	candidate.insert(candidate.end(), after.begin(), after.end());
	for (uint64_t root : recovery.consumed_root_order)
	{
		const size_t offset = candidate.size();
		if (!owner.growth(candidate, sizeof(uint64_t)))
			return false;
		candidate.resize(offset + sizeof(uint64_t));
		put_u64(candidate.data() + offset, root);
	}
	const size_t publication_start = candidate.size();
	if (!owner.growth(candidate, ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES))
		return false;
	candidate.resize(publication_start + ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES);
	put_u32(candidate.data() + publication_start,
		(recovery.publication_terms.echo_all ? 1U : 0U) |
			(recovery.publication_terms.disappear ? 2U : 0U));
	put_u32(candidate.data() + publication_start + 4,
		recovery.publication_terms.message.size());
	put_u32(candidate.data() + publication_start + 8,
		recovery.publication_terms.disappear_message.size());
	if (!owner.growth(candidate, recovery.publication_terms.message.size()))
		return false;
	candidate.insert(candidate.end(), recovery.publication_terms.message.begin(),
			 recovery.publication_terms.message.end());
	if (!owner.growth(candidate, recovery.publication_terms.disappear_message.size()))
		return false;
	candidate.insert(candidate.end(), recovery.publication_terms.disappear_message.begin(),
			 recovery.publication_terms.disappear_message.end());
	if (!owner.peak(payload_clone_move_frames))
		return false;
	const size_t retained = candidate.capacity();
	*out = std::move(candidate);
	if (retained_encoded_heap_bytes)
		*retained_encoded_heap_bytes = retained;
	return true;
}

bool item_native_recovery_decode_owned(std::span<const uint8_t> bytes,
				       item_native_mobile_recovery_context *out,
				       item_native_recovery_budget &owner,
				       size_t *retained_recovery_heap_bytes)
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
	if (!owner.peak(sizeof(item_native_mobile_recovery_context) +
			item_native_recovery_default_frames))
		return false;
	item_native_mobile_recovery_context candidate;
	owner.recovery = &candidate;
	size_t prefix = 0;
	if (!owner.prefix(prefix))
		return false;
	candidate.present = true;
	candidate.player_pid = get_u32(bytes.data() + 8);
	candidate.acknowledged_save_revision = get_u64(bytes.data() + 16);
	if (!shop_trade_recovery_forest_decode_bounded(
		    bytes.subspan(first, before_bytes),
		    shop_trade_recovery_forest_role::player_before, &candidate.player_before,
		    owner.reserve, owner.context, prefix, nullptr) ||
	    !owner.prefix(prefix) ||
	    !shop_trade_recovery_forest_decode_bounded(
		    bytes.subspan(after_start, after_bytes),
		    shop_trade_recovery_forest_role::player_after, &candidate.player_after,
		    owner.reserve, owner.context, prefix, nullptr))
		return false;
	if (!owner.fresh<uint64_t>(root_count))
		return false;
	candidate.consumed_root_order.reserve(root_count);
	for (size_t i = 0; i < root_count; ++i)
	{
		if (!owner.growth(candidate.consumed_root_order, 1))
			return false;
		candidate.consumed_root_order.push_back(
			get_u64(bytes.data() + roots_start + i * sizeof(uint64_t)));
	}
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
	if (!owner.text(candidate.publication_terms.message,
			reinterpret_cast<const char *>(bytes.data() + text_start), message_bytes))
		return false;
	if (!owner.text(candidate.publication_terms.disappear_message,
			reinterpret_cast<const char *>(bytes.data() + text_start + message_bytes),
			disappear_bytes))
		return false;
	if (!native_publication_terms_valid(candidate.publication_terms))
		return false;
	// Four genuine recovery/forest/publication aggregate moves, each this/source,
	// three actual vector moves and two string moves, including cleanup scopes.
	if (!owner.peak(8 * sizeof(void *) + 3 * payload_clone_move_frames +
			2 * payload_clone_string_move_frames))
		return false;
	size_t retained = 0;
	if (!payload_clone_vector_heap(candidate.player_before.ordered_item_uids, false,
				       retained) ||
	    !payload_clone_vector_heap(candidate.player_after.ordered_item_uids, false, retained) ||
	    !payload_clone_vector_heap(candidate.consumed_root_order, false, retained) ||
	    !payload_clone_string_heap(candidate.publication_terms.message, false, retained) ||
	    !payload_clone_string_heap(candidate.publication_terms.disappear_message, false,
				       retained))
		return false;
	*out = std::move(candidate);
	if (retained_recovery_heap_bytes)
		*retained_recovery_heap_bytes = retained;
	return true;
}

} // namespace
bool item_transfer_native_recovery_encode_bounded(
	const item_native_mobile_recovery_context &recovery, std::vector<uint8_t> *out,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_encoded_heap_bytes) noexcept
{
	if (!out || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		// Public input/out/reserve/context/scalar/outer/result; private input/out/
		// owner/scalar/result. Genuine prefix/publication/offset/retained locals,
		// root by-value and range iterators; allocator-only failed cleanup.
		9 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + 4 * sizeof(size_t) +
		sizeof(uint64_t) + 2 * sizeof(void *) + item_native_recovery_fixed_frames +
		payload_clone_allocator_frames;
	item_native_recovery_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak())
		return false;
	static_assert(std::is_nothrow_move_assignable_v<std::vector<uint8_t>>);
	try
	{
		return item_native_recovery_encode_owned(recovery, out, owner,
							 retained_encoded_heap_bytes);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_recovery_decode_bounded(std::span<const uint8_t> bytes,
						  item_native_mobile_recovery_context *out,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer_live,
						  size_t *retained_recovery_heap_bytes) noexcept
{
	if (!out || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		// Public/private original bytes span carriers and pointer/scalar/result
		// args; all actual first/before/after/root/publication/message/tail/loop/
		// prefix/retained source locals, original flags and failed cleanup.
		2 * sizeof(std::span<const uint8_t>) + 8 * sizeof(void *) + sizeof(size_t) +
		2 * sizeof(bool) + 14 * sizeof(size_t) + sizeof(uint32_t) +
		item_native_recovery_fixed_frames + payload_clone_allocator_frames;
	item_native_recovery_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak())
		return false;
	static_assert(std::is_nothrow_move_assignable_v<item_native_mobile_recovery_context>);
	try
	{
		return item_native_recovery_decode_owned(bytes, out, owner,
							 retained_recovery_heap_bytes);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
// Original algorithm source scopes from installed GCC13 stl_algo/algobase and
// predefined_ops. All comparisons are allocation-free; recursion is actual sort.
constexpr size_t item_native_validation_find_frames =
	// find: first/last/value/return; find_if: first/last/captured uid/return;
	// both actual __find_if overloads first/last/predicate/return and RA tag
	// plus real trip_count. Largest real predicate owns the captured uid.
	4 * sizeof(void *) + 3 * sizeof(void *) + sizeof(uint64_t) +
	2 * (3 * sizeof(void *) + sizeof(uint64_t)) + sizeof(std::random_access_iterator_tag) +
	sizeof(std::ptrdiff_t) +
	// __pred_iter constructor/call/move + _Iter_pred constructor/operator(),
	// actual lambda this/item/result and uid-bearing temporary predicate.
	4 * sizeof(void *) + 3 * sizeof(uint64_t) + 2 * sizeof(void *) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(bool) +
	// __iter_equals_val constructor/source result wrapper/operator();
	// actual iterator-category/reference/returned tag and begin/end/operators.
	4 * sizeof(void *) + sizeof(bool) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + 10 * (2 * sizeof(void *)) +
	sizeof(std::ptrdiff_t);
constexpr size_t item_native_validation_count_frames =
	// count_if and __count_if first/last/empty closure/returned difference;
	// __count_if real n; predicate constructor/call/reference/result.
	4 * sizeof(void *) + 3 * sizeof(std::ptrdiff_t) + 2 * sizeof(char) + 4 * sizeof(void *) +
	2 * sizeof(char) + 2 * sizeof(bool) +
	// Real empty root lambda call this/item/result, ++/comparison/deref.
	2 * sizeof(void *) + sizeof(bool) + 6 * (2 * sizeof(void *));
constexpr size_t item_native_validation_equal_frames =
	// equal/__equal_aux/__equal_aux1/__equal<true>::equal: three iterators
	// and returned bool each; simple flag/length and three niter calls; memcmp.
	4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) + sizeof(size_t) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int);
struct item_native_validation_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const std::vector<player_item_snapshot> *items = nullptr;
	const std::vector<uint8_t> *canonical = nullptr;
	const std::vector<uint64_t> *roots = nullptr, *uids = nullptr;
	const quest_reward_continuation *terms = nullptr;
	const std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH> *path = nullptr;
	bool prefix(size_t &output, size_t extra = 0) const noexcept
	{
		// Real scan/add/multiply/current-row observer source frames; the shared
		// row provider reports capacity storage, not item-encoded size.
		constexpr size_t observation = 12 * sizeof(void *) + 8 * sizeof(size_t) +
					       4 * sizeof(bool) +
					       5 * (sizeof(void *) + sizeof(size_t));
		size_t total = outer, row = 0;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    !payload_clone_add(total, observation) ||
		    !payload_clone_add(total, player_item_snapshot_copy_frame_bytes()))
			return false;
		if (items)
		{
			if (!payload_clone_add(total, sizeof(*items)) ||
			    !payload_clone_vector_heap(*items, false, total))
				return false;
			for (const auto &item : *items)
				if (!player_item_snapshot_current_heap_bytes(item, &row) ||
				    !payload_clone_add(total, row))
					return false;
		}
		if ((canonical && (!payload_clone_add(total, sizeof(*canonical)) ||
				   !payload_clone_vector_heap(*canonical, false, total))) ||
		    (roots && (!payload_clone_add(total, sizeof(*roots)) ||
			       !payload_clone_vector_heap(*roots, false, total))) ||
		    (uids && (!payload_clone_add(total, sizeof(*uids)) ||
			      !payload_clone_vector_heap(*uids, false, total))) ||
		    (terms && (!payload_clone_add(total, sizeof(*terms)) ||
			       !payload_clone_string_heap(terms->character_name, false, total) ||
			       !payload_clone_string_heap(terms->definition_id, false, total))) ||
		    (path && !payload_clone_add(total, sizeof(*path))) ||
		    !payload_clone_add(total, extra))
			return false;
		output = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
bool item_native_validation_recovery_owned(const item_transfer_payload &payload,
					   const item_native_mobile_recovery_context &recovery,
					   uint16_t version, item_native_validation_budget &owner)
{
	size_t budget_prefix = 0;
	const auto before_role = shop_trade_recovery_forest_role::player_before;
	const auto after_role = shop_trade_recovery_forest_role::player_after;
	if (!native_publication_terms_valid(recovery.publication_terms) ||
	    !owner.prefix(budget_prefix) ||
	    !shop_trade_recovery_forest_shape_valid_bounded(recovery.player_before, before_role,
							    owner.reserve, owner.context,
							    budget_prefix) ||
	    !owner.prefix(budget_prefix) ||
	    !shop_trade_recovery_forest_shape_valid_bounded(
		    recovery.player_after, after_role, owner.reserve, owner.context, budget_prefix))
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
		if (!owner.peak(sizeof(std::vector<player_item_snapshot>) + 4 * sizeof(void *) +
				sizeof(std::allocator<player_item_snapshot>)))
			return false;
		std::vector<player_item_snapshot> selected;
		owner.items = &selected;
		if (!owner.prefix(budget_prefix))
			return false;
		if (player_item_snapshot_list_decode_bounded(
			    payload.item_blob.data(), payload.item_blob_size, &selected,
			    owner.reserve, owner.context, budget_prefix,
			    nullptr) != player_snapshot_codec_result::ok ||
		    selected.size() != payload.item_count)
			return false;
		if (!owner.peak(item_native_validation_count_frames +
				item_native_validation_find_frames +
				item_native_validation_equal_frames))
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
		if (!owner.peak(sizeof(quest_reward_continuation) + 5 * sizeof(void *) +
				2 * (7 * sizeof(void *) + sizeof(std::allocator<char>) +
				     sizeof(size_t) + sizeof(char))))
			return false;
		quest_reward_continuation terms;
		owner.terms = &terms;
		if (!owner.prefix(budget_prefix))
			return false;
		return payload.continuation.kind ==
			       item_transfer_continuation_kind::quest_offering &&
		       quest_reward_continuation_decode_bounded(payload.continuation.data.data(),
								payload.continuation.data.size(),
								&terms, owner.reserve,
								owner.context, budget_prefix) &&
		       terms.version == 5 &&
		       terms.root_count == recovery.consumed_root_order.size() &&
		       owner.peak(item_native_validation_equal_frames) &&
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
	if (!owner.peak(sizeof(std::vector<player_item_snapshot>) + 4 * sizeof(void *) +
			sizeof(std::allocator<player_item_snapshot>)))
		return false;
	std::vector<player_item_snapshot> selected;
	owner.items = &selected;
	if (!owner.prefix(budget_prefix))
		return false;
	if (player_item_snapshot_list_decode_bounded(
		    payload.item_blob.data(), payload.item_blob_size, &selected, owner.reserve,
		    owner.context, budget_prefix, nullptr) != player_snapshot_codec_result::ok ||
	    selected.size() != payload.item_count)
		return false;
	if (!owner.peak(item_native_validation_find_frames + item_native_validation_equal_frames))
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

} // namespace
bool item_transfer_native_recovery_valid_bounded(
	const item_transfer_payload &payload, const item_native_mobile_recovery_context &recovery,
	uint16_t version, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 8 * sizeof(void *) + 2 * sizeof(uint16_t) + sizeof(size_t) +
				  2 * sizeof(bool) + 2 * sizeof(shop_trade_recovery_forest_role) +
				  4 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(uint64_t) +
				  5 * sizeof(void *) + item_native_validation_find_frames +
				  item_native_validation_equal_frames +
				  // Original publication predicate find/char_traits/memchr scopes.
				  10 * sizeof(void *) + 7 * sizeof(size_t) + 3 * sizeof(bool) +
				  sizeof(char) + sizeof(int) + payload_clone_allocator_frames;
	item_native_validation_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak())
		return false;
	try
	{
		return item_native_validation_recovery_owned(payload, recovery, version, owner);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
// Exact original fixed-array lower_bound lookup, not an allocation wrapper.
constexpr size_t item_continuation_lower_bound_frames =
	// find_payload_item payload/uid/found/returned pointer; original lambda
	// this/entry/by-value UID/result and array begin/_S_ptr source scopes.
	sizeof(void *) + sizeof(uint64_t) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(uint64_t) + sizeof(bool) + 4 * sizeof(void *) +
	// lower_bound comparator overload first/last/value/comp/return;
	// __lower_bound first/last/value/comp/len/step/middle/return.
	4 * sizeof(void *) + sizeof(char) + 5 * sizeof(void *) + sizeof(char) +
	2 * sizeof(std::ptrdiff_t) +
	// __iter_comp_val empty closure by value/result; _Iter_comp_val ctor
	// this/closure then operator(this,iterator,value)/bool.
	2 * sizeof(char) + sizeof(void *) + sizeof(char) + 3 * sizeof(void *) + sizeof(bool) +
	// distance and __distance: first/last and difference results, actual RA tag.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) +
	sizeof(std::random_access_iterator_tag) +
	// advance iterator-reference/n/local__d; __advance reference/n/tag;
	// actual iterator-category reference and returned RA tag.
	sizeof(void *) + 2 * sizeof(std::ptrdiff_t) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag);
constexpr size_t item_continuation_sort_recursive_frame =
	3 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(char);
constexpr size_t item_continuation_sort_leaf_frames =
	// sort/__sort and median/partition/iter_swap carrier scopes.
	18 * sizeof(void *) + 7 * sizeof(char) + sizeof(uint64_t) +
	// final/insertion/unguarded-insertion/linear-insert and move-backward.
	16 * sizeof(void *) + 6 * sizeof(char) + 2 * sizeof(uint64_t) +
	// partial_sort/heap_select/make_heap/adjust_heap/push_heap/pop_heap/sort_heap.
	23 * sizeof(void *) + 11 * sizeof(std::ptrdiff_t) + 7 * sizeof(char) +
	4 * sizeof(uint64_t) +
	// comparator adapters and scalar compare/result carriers.
	8 * sizeof(void *) + 5 * sizeof(char) + 4 * sizeof(bool) +
	// Original copy/copy_move_a/a1/a2/copy_m: five 3-iterator parameter
	// scopes and their actual returned iterator carriers, then miter/niter,
	// niter-wrap, assign-one, memmove parameters/result and real Num/length.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// adjacent_find's 2 input+returned iterators; __adjacent_find's 2
	// input+next+returned; iter-equal's 2 input iterators/boolean result.
	9 * sizeof(void *) + 2 * sizeof(char) + sizeof(bool);
bool item_continuation_quest_valid_owned(const item_transfer_payload &payload,
					 item_native_validation_budget &owner)
{
	const std::vector<uint8_t> &data = payload.continuation.data;
	if (!owner.peak(sizeof(quest_reward_continuation) + 5 * sizeof(void *) +
			2 * (7 * sizeof(void *) + sizeof(std::allocator<char>) + sizeof(size_t) +
			     sizeof(char))))
		return false;
	quest_reward_continuation continuation;
	owner.terms = &continuation;
	size_t prefix = 0;
	if (!owner.prefix(prefix))
		return false;
	if (!quest_reward_continuation_decode_bounded(data.data(), data.size(), &continuation,
						      owner.reserve, owner.context, prefix) ||
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

bool item_continuation_duplicate_valid_owned(const item_transfer_payload &payload,
					     uint16_t payload_version,
					     item_native_validation_budget &owner)
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
	if (!owner.peak(sizeof(std::vector<uint64_t>) + 4 * sizeof(void *) +
			sizeof(std::allocator<uint64_t>)))
		return false;
	std::vector<uint64_t> child_uids;
	owner.uids = &child_uids;
	try
	{
		if (!owner.peak(direct_child_count * sizeof(uint64_t) +
				payload_clone_vector_frames))
			return false;
		child_uids.reserve(direct_child_count);
		for (size_t index = 0; index < direct_child_count; ++index)
		{
			const uint64_t uid = get_u64(data.data() + 40 + index * sizeof(uint64_t));
			const item_transfer_entry *child = find_payload_item(payload, uid);
			if (!uid || !child || child->parent_item_uid != duplicate_uid ||
			    std::find(child_uids.begin(), child_uids.end(), uid) !=
				    child_uids.end())
				return false;
			if (!owner.peak(payload_clone_vector_frames))
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

bool item_continuation_craft_outputs_owned(const item_transfer_payload &payload,
					   std::vector<player_item_snapshot> *outputs,
					   item_native_validation_budget &owner)
{
	if (!outputs || payload.reason != item_transfer_reason::craft ||
	    payload.item_blob_size > payload.item_blob.size())
		return false;
	outputs->clear();
	if (!payload.item_blob_size)
		return true;
	size_t prefix = 0;
	if (!owner.prefix(prefix))
		return false;
	if (player_item_snapshot_list_decode_bounded(
		    payload.item_blob.data(), payload.item_blob_size, outputs, owner.reserve,
		    owner.context, prefix, nullptr) != player_snapshot_codec_result::ok ||
	    outputs->empty() || outputs->size() > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	if (!owner.peak(sizeof(std::vector<uint64_t>) + 4 * sizeof(void *) +
			sizeof(std::allocator<uint64_t>)))
		return false;
	std::vector<uint64_t> uids;
	owner.uids = &uids;
	try
	{
		if (outputs->size() > SIZE_MAX / sizeof(uint64_t) ||
		    !owner.peak(outputs->size() * sizeof(uint64_t) + payload_clone_vector_frames))
			return false;
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
			if (!owner.peak(payload_clone_vector_frames))
				return false;
			uids.push_back(output.object_uid);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	size_t levels = 0, remaining = uids.size();
	while (remaining > 1)
	{
		remaining >>= 1;
		++levels;
	}
	if (!owner.peak((2 * levels + 1) * item_continuation_sort_recursive_frame +
			item_continuation_sort_leaf_frames))
		return false;
	std::sort(uids.begin(), uids.end());
	return std::adjacent_find(uids.begin(), uids.end()) == uids.end();
}

} // namespace
bool item_transfer_quest_offering_continuation_valid_bounded(
	const item_transfer_payload &payload, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 6 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) +
				  4 * sizeof(size_t) + sizeof(uint64_t) + 3 * sizeof(void *) +
				  item_continuation_lower_bound_frames +
				  payload_clone_allocator_frames;
	item_native_validation_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak())
		return false;
	try
	{
		return item_continuation_quest_valid_owned(payload, owner);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_duplicate_promotion_continuation_valid_bounded(
	const item_transfer_payload &payload, uint16_t version,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		6 * sizeof(void *) + 2 * sizeof(uint16_t) + sizeof(size_t) + 2 * sizeof(bool) +
		sizeof(void *) + 3 * sizeof(uint64_t) + sizeof(uint32_t) + 3 * sizeof(size_t) +
		item_continuation_lower_bound_frames + item_native_validation_find_frames +
		// Original get32/64 input/result/value/byte, array/vector size/data/query.
		5 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(uint64_t) +
		2 * sizeof(uint32_t) + 2 * sizeof(unsigned int) + payload_clone_allocator_frames;
	item_native_validation_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak())
		return false;
	try
	{
		return item_continuation_duplicate_valid_owned(payload, version, owner);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_craft_outputs_decode_bounded(const item_transfer_payload &payload,
						std::vector<player_item_snapshot> *outputs,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer_live,
						size_t *retained_item_heap_bytes) noexcept
{
	if (!outputs || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 8 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) +
				  4 * sizeof(size_t) + sizeof(void *) +
				  4 * (sizeof(void *) + sizeof(size_t)) +
				  payload_clone_allocator_frames;
	item_native_validation_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak(sizeof(std::vector<player_item_snapshot>) + 4 * sizeof(void *) +
			sizeof(std::allocator<player_item_snapshot>)))
		return false;
	std::vector<player_item_snapshot> candidate;
	owner.items = &candidate;
	static_assert(std::is_nothrow_move_assignable_v<std::vector<player_item_snapshot>>);
	try
	{
		if (!item_continuation_craft_outputs_owned(payload, &candidate, owner))
			return false;
		// Private UID scratch has died; stop observing that expired local.
		owner.uids = nullptr;
		if (!owner.peak(payload_clone_move_frames))
			return false;
		size_t retained = 0, row = 0;
		if (!payload_clone_vector_heap(candidate, false, retained))
			return false;
		for (const auto &item : candidate)
			if (!player_item_snapshot_current_heap_bytes(item, &row) ||
			    !payload_clone_add(retained, row))
				return false;
		*outputs = std::move(candidate);
		if (retained_item_heap_bytes)
			*retained_item_heap_bytes = retained;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
bool item_native_mobile_validation_owned(const item_transfer_payload &payload, uint16_t version,
					 item_native_validation_budget &budget)
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
	size_t reference_prefix = 0;
	if (!budget.prefix(reference_prefix))
		return false;
	if (!item_owner_identity_valid(owner) || owner.id != context.reference.mobile_instance_id ||
	    quest_mobile_native_reference_encode_bounded(
		    context.reference, &reference, budget.reserve, budget.context,
		    reference_prefix) != player_snapshot_codec_result::ok ||
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
	if (!budget.peak(sizeof(std::vector<player_item_snapshot>) + 4 * sizeof(void *) +
			 sizeof(std::allocator<player_item_snapshot>)))
		return false;
	std::vector<player_item_snapshot> items;
	budget.items = &items;
	size_t prefix = 0;
	if (!budget.prefix(prefix))
		return false;
	if (player_item_snapshot_list_decode_bounded(
		    payload.item_blob.data(), payload.item_blob_size, &items, budget.reserve,
		    budget.context, prefix, nullptr) != player_snapshot_codec_result::ok ||
	    items.size() != payload.item_count)
		return false;
	if (!budget.peak(sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
			 sizeof(std::allocator<uint8_t>)))
		return false;
	std::vector<uint8_t> canonical;
	budget.canonical = &canonical;
	if (!budget.prefix(prefix))
		return false;
	if (player_item_snapshot_list_encode_bounded(items, &canonical, budget.reserve,
						     budget.context,
						     prefix) != player_snapshot_codec_result::ok ||
	    canonical.size() != payload.item_blob_size ||
	    !budget.peak(item_native_validation_equal_frames) ||
	    !std::equal(canonical.begin(), canonical.end(), payload.item_blob.begin()))
		return false;
	if (items.size() > SIZE_MAX / sizeof(uint64_t) ||
	    !budget.peak(2 * sizeof(std::vector<uint64_t>) + items.size() * sizeof(uint64_t) +
			 payload_clone_vector_constructor_frames + 4 * sizeof(void *) +
			 sizeof(std::allocator<uint64_t>)))
		return false;
	std::vector<uint64_t> roots(items.size()), uids;
	budget.roots = &roots;
	budget.uids = &uids;
	if (!budget.peak(sizeof(std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH>)))
		return false;
	std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH> path = {};
	budget.path = &path;
	size_t depth = 0;
	if (!budget.peak(items.size() * sizeof(uint64_t) + payload_clone_vector_frames))
		return false;
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
		if (!budget.peak(payload_clone_vector_frames))
			return false;
		uids.push_back(item.object_uid);
	}
	size_t levels = 0, remaining = uids.size();
	while (remaining > 1)
	{
		remaining >>= 1;
		++levels;
	}
	if (!budget.peak((2 * levels + 1) * item_continuation_sort_recursive_frame +
			 item_continuation_sort_leaf_frames))
		return false;
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
} // namespace

bool item_transfer_native_mobile_context_valid_bounded(const item_transfer_payload &payload,
						       uint16_t version,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		// Public/private argument/result and genuine original scalar/ref locals,
		// actual reference output array and separate reference-prefix scratch.
		7 * sizeof(void *) + 2 * sizeof(uint16_t) + sizeof(size_t) + 7 * sizeof(bool) +
		10 * sizeof(size_t) + 2 * sizeof(uint64_t) + 6 * sizeof(void *) +
		sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
		// Original version/identity predicates and actual array/path []/_S_ref
		// parameters/results. Reference codec's full nested scope is callee-owned.
		5 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(bool) +
		item_continuation_lower_bound_frames + item_native_validation_equal_frames +
		payload_clone_allocator_frames;
	item_native_validation_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_native_mobile_validation_owned(payload, version, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
constexpr size_t item_payload_validation_pure_frames =
	// Original selected_root_for and target_topology_for this/input/result,
	// entry/parent pointers, UID/depth/selected-root named local scopes.
	3 * sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(size_t) + 5 * sizeof(void *) +
	2 * sizeof(uint64_t) + sizeof(bool) + item_continuation_lower_bound_frames +
	// Original reason/version/owner/equality and fixed continuation predicates.
	sizeof(item_transfer_reason) + sizeof(uint16_t) + 4 * sizeof(void *) + 5 * sizeof(bool) +
	// Original spell value-codec and retirement value-codec data refs/version/
	// bools/offset/effect/nested-UID/item ref plus fixed get32/get64 source scopes.
	3 * sizeof(void *) + 3 * sizeof(uint16_t) + 4 * sizeof(bool) + sizeof(size_t) +
	3 * sizeof(uint32_t) + sizeof(void *) + 2 * sizeof(uint64_t) + 2 * sizeof(unsigned int) +
	// Original collector payload/version/collector reference and zero-ID
	// range byte/iterators; all_of lambda captures original payload reference.
	4 * sizeof(void *) + sizeof(uint16_t) + 2 * sizeof(bool) + sizeof(uint8_t) +
	2 * sizeof(void *) +
	// is_sorted/is_sorted_until/__is_sorted_until first/last/next/returned
	// iterator and empty comparator; iter-less this/twoiter/bool.
	3 * (3 * sizeof(void *) + sizeof(char)) + sizeof(void *) + sizeof(bool) +
	3 * sizeof(void *) + sizeof(bool) +
	// all_of/find_if_not/__find_if_not/__negate/_Iter_negate wrappers and
	// lambda reference/UID/results. Actual find iterator/category scopes.
	4 * (2 * sizeof(void *) + sizeof(char) + sizeof(bool)) + 3 * sizeof(void *) +
	2 * sizeof(char) + 3 * sizeof(bool) + sizeof(uint64_t) +
	item_native_validation_find_frames +
	// valid_text string/max/nonempty/result, original string query/source
	// all_of byte lambda; array/span/vector queries at literal fixed sites.
	sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + 2 * sizeof(void *) +
	sizeof(unsigned char) + sizeof(bool) + 12 * (sizeof(void *) + sizeof(size_t)) +
	3 * sizeof(std::span<const uint8_t>) +
	// Native projection default temporaries, vector default constructor and
	// aggregate/vector/array equality + destruction declared scopes.
	sizeof(native_quest_coin_give_projection) + sizeof(native_quest_cost_projection) +
	4 * sizeof(void *) + sizeof(std::allocator<native_quest_cost_attempt>) +
	6 * sizeof(void *) + 4 * sizeof(bool) + item_native_validation_equal_frames +
	payload_clone_allocator_frames;
struct item_payload_validation_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const std::vector<player_item_snapshot> *outputs = nullptr;
	const craft_pouch_mutation *pouch = nullptr;
	const craft_recipe_continuation *recipe = nullptr;
	bool prefix(size_t &output, size_t extra = 0) const noexcept
	{
		constexpr size_t observations = 12 * sizeof(void *) + 9 * sizeof(size_t) +
						4 * sizeof(bool) +
						6 * (sizeof(void *) + sizeof(size_t));
		size_t total = outer, row = 0;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    !payload_clone_add(total, observations) ||
		    !payload_clone_add(total, player_item_snapshot_copy_frame_bytes()))
			return false;
		if (outputs)
		{
			if (!payload_clone_add(total, sizeof(*outputs)) ||
			    !payload_clone_vector_heap(*outputs, false, total))
				return false;
			for (const auto &item : *outputs)
				if (!player_item_snapshot_current_heap_bytes(item, &row) ||
				    !payload_clone_add(total, row))
					return false;
		}
		if (pouch)
		{
			if (!payload_clone_add(total, sizeof(*pouch)) ||
			    !payload_clone_vector_heap(pouch->usage, false, total) ||
			    !player_item_snapshot_current_heap_bytes(pouch->before, &row) ||
			    !payload_clone_add(total, row) ||
			    !player_item_snapshot_current_heap_bytes(pouch->after, &row) ||
			    !payload_clone_add(total, row))
				return false;
		}
		if (recipe &&
		    (!payload_clone_add(total, sizeof(*recipe)) ||
		     !payload_clone_vector_heap(recipe->pouch_mutation, false, total) ||
		     !payload_clone_vector_heap(recipe->refine_root_order, false, total) ||
		     !payload_clone_string_heap(recipe->refine_material_name, false, total)))
			return false;
		if (!payload_clone_add(total, extra))
			return false;
		output = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
bool item_full_payload_validation_owned(const item_transfer_payload &payload,
					uint16_t payload_version,
					item_payload_validation_budget &budget)
{
	size_t admission_prefix = 0;
	// Every legacy/generic encoder must refuse rather than silently drop money.
	if (payload.native_money.present || payload.native_money.original_room_vnum ||
	    payload.native_money.player_wallet_mapping_id ||
	    payload.native_money.mobile_wallet_mapping_id ||
	    payload.native_money.projection != native_quest_coin_give_projection{})
		return false;
	// Every legacy/generic encoder must refuse rather than silently drop cost.
	if (payload.native_cost.present || payload.native_cost.fee_only ||
	    payload.native_cost.completion_slot || payload.native_cost.wallet_mapping_id ||
	    payload.native_cost.projection != native_quest_cost_projection{})
		return false;
	if (!(budget.prefix(admission_prefix) && item_transfer_native_mobile_context_valid_bounded(
							 payload, payload_version, budget.reserve,
							 budget.context, admission_prefix)) ||
	    !(budget.prefix(admission_prefix) &&
	      item_transfer_native_recovery_valid_bounded(payload, payload.native_recovery,
							  payload_version, budget.reserve,
							  budget.context, admission_prefix)))
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
	      !(budget.prefix(admission_prefix) &&
		item_transfer_quest_offering_continuation_valid_bounded(
			payload, budget.reserve, budget.context, admission_prefix)))) ||
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
	     !(budget.prefix(admission_prefix) &&
	       item_transfer_duplicate_promotion_continuation_valid_bounded(
		       payload, payload_version, budget.reserve, budget.context,
		       admission_prefix))) ||
	    (payload.continuation.kind == item_transfer_continuation_kind::craft_pouch_usage &&
	     payload_version < ITEM_TRANSFER_PAYLOAD_VERSION) ||
	    (payload.continuation.kind == item_transfer_continuation_kind::lockpick_retirement &&
	     (payload_version < ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION ||
	      !(budget.prefix(admission_prefix) &&
		lockpick_retirement_payload_valid_bounded(payload, budget.reserve, budget.context,
							  admission_prefix)))) ||
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
	     payload.continuation.kind != item_transfer_continuation_kind::craft_recipe &&
	     payload.continuation.kind != item_transfer_continuation_kind::lockpick_retirement))
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
		if (!budget.peak(sizeof(std::vector<player_item_snapshot>) +
				 sizeof(craft_pouch_mutation) +
				 // Original two row default constructions and three genuine vector
				 // default constructors; shared source profile covers row closure.
				 2 * player_item_snapshot_copy_frame_bytes() + 13 * sizeof(void *) +
				 2 * sizeof(std::allocator<player_item_snapshot>) +
				 sizeof(std::allocator<chaos_material_pouch_usage>)))
			return false;
		std::vector<player_item_snapshot> outputs;
		craft_pouch_mutation pouch;
		budget.outputs = &outputs;
		budget.pouch = &pouch;
		if (payload_version < ITEM_TRANSFER_BATCH_PAYLOAD_VERSION ||
		    payload.from_owner.type != item_owner_type::player ||
		    payload.to_owner.type != item_owner_type::player ||
		    !item_owner_identity_equal(payload.from_owner, payload.to_owner) ||
		    !payload.from_owner.id || payload.from_owner.context_id ||
		    payload.to_owner.context_id || !payload.multi_root ||
		    !payload.selected_item_uid || payload.target_root_item_uid ||
		    payload.target_parent_item_uid || payload.expected_target_parent_revision ||
		    !(budget.prefix(admission_prefix) &&
		      item_transfer_craft_outputs_decode_bounded(payload, &outputs, budget.reserve,
								 budget.context, admission_prefix,
								 nullptr)) ||
		    !(budget.prefix(admission_prefix) &&
		      craft_pouch_mutation_from_payload_bounded(
			      payload, &pouch, budget.reserve, budget.context, admission_prefix)) ||
		    (outputs.empty() && !find_payload_item(payload, payload.selected_item_uid)) ||
		    payload.item_count + outputs.size() > CRITICAL_COMMAND_MAX_KEYS - 2)
			return false;
		if (payload.continuation.kind == item_transfer_continuation_kind::craft_recipe)
		{
			if (!budget.peak(
				    sizeof(craft_recipe_continuation) +
				    3 * duris_craft_recipe_bounded_detail::empty_constructor_frames +
				    duris_craft_recipe_bounded_detail::string_lifetime_frames))
				return false;
			craft_recipe_continuation recipe;
			budget.recipe = &recipe;
			if (!(budget.prefix(admission_prefix) &&
			      craft_recipe_continuation_decode_bounded(
				      payload.continuation.data, &recipe, budget.reserve,
				      budget.context, admission_prefix)) ||
			    (!budget.peak(
				     duris_craft_recipe_bounded_detail::pure_frames +
				     item_native_validation_count_frames +
				     // Original matches payload/recipe refs, materials/ores/roots,
				     // last_ore, UID and source-root loops/entry source locals.
				     3 * sizeof(void *) + 5 * sizeof(size_t) + sizeof(uint32_t) +
				     sizeof(uint64_t)) ||
			     !craft_recipe_continuation_matches(recipe, payload)) ||
			    (craft_recipe_is_alchemy(recipe.discipline) ||
					     recipe.discipline == craft_recipe_discipline::refine ?
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
						     static_cast<int32_t>(recipe.recipe_vnum)) ||
			    (recipe.discipline == craft_recipe_discipline::refine &&
			     (!outputs.empty() &&
			      (outputs.size() != 1 ||
			       outputs[0].vnum != static_cast<int32_t>(recipe.recipe_vnum)))))
				return false;
			budget.recipe = nullptr;
		}
		for (const player_item_snapshot &output : outputs)
			for (size_t index = 0; index < payload.item_count; ++index)
				if (output.object_uid == payload.items[index].item_uid)
					return false;
		// Original locals die at this brace; later census must not read them.
		budget.outputs = nullptr;
		budget.pouch = nullptr;
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
bool item_transfer_payload_valid_bounded(const item_transfer_payload &payload, uint16_t version,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		// Public/private payload/owner/reserve/context and original version
		// parameters/results; prefix scratch. No inline payload clone exists.
		5 * sizeof(void *) + 2 * sizeof(uint16_t) + 2 * sizeof(size_t) + 2 * sizeof(bool) +
		// All 22 original boolean declaration sites, four loop index sites,
		// two depth sites, original owner PID/saveID, ten declared UID/topology
		// values and original entry/owner/parent/output/range ref carriers.
		22 * sizeof(bool) + 6 * sizeof(size_t) + 2 * sizeof(uint32_t) +
		10 * sizeof(uint64_t) + 10 * sizeof(void *) + item_payload_validation_pure_frames +
		item_native_validation_find_frames + payload_clone_allocator_frames;
	item_payload_validation_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_full_payload_validation_owned(payload, version, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
template <typename T, typename Comparator> constexpr size_t item_entity_sort_leaf_frames()
{
	// Same real GCC13 sort/partition/insertion/heap/copy/adjacent call scopes
	// as UID sorting. Values and comparator carriers use their genuine types.
	// Original key less/equal this-free argument/result scopes and revision
	// lambda this/left/right/result plus its nested key less call.
	return 3 * (2 * sizeof(void *) + sizeof(bool)) + 3 * sizeof(void *) + sizeof(bool) +
	       18 * sizeof(void *) + 7 * sizeof(Comparator) + sizeof(T) + 16 * sizeof(void *) +
	       6 * sizeof(Comparator) + 2 * sizeof(T) + 23 * sizeof(void *) +
	       11 * sizeof(std::ptrdiff_t) + 7 * sizeof(Comparator) + 4 * sizeof(T) +
	       8 * sizeof(void *) + 5 * sizeof(Comparator) + 4 * sizeof(bool) +
	       5 * (4 * sizeof(void *)) + 2 * (2 * sizeof(void *)) + 3 * (2 * sizeof(void *)) +
	       2 * sizeof(void *) + sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	       sizeof(size_t) + sizeof(std::ptrdiff_t) + 9 * sizeof(void *) +
	       2 * sizeof(Comparator) + sizeof(bool);
}
struct item_entity_construction_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *command = nullptr;
	const std::vector<uint64_t> *roots = nullptr;
	const std::vector<player_item_snapshot> *outputs = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observation = 10 * sizeof(void *) + 7 * sizeof(size_t) +
					       4 * sizeof(bool) +
					       5 * (sizeof(void *) + sizeof(size_t));
		size_t total = outer, row = 0;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    !payload_clone_add(total, observation) ||
		    !payload_clone_add(total, player_item_snapshot_copy_frame_bytes()))
			return false;
		if (command &&
		    (!payload_clone_add(total, sizeof(*command)) ||
		     !payload_clone_vector_heap(command->keys, false, total) ||
		     !payload_clone_vector_heap(command->expected_revisions, false, total) ||
		     !payload_clone_vector_heap(command->payload, false, total) ||
		     !payload_clone_vector_heap(command->accounting_intent, false, total)))
			return false;
		if (roots && (!payload_clone_add(total, sizeof(*roots)) ||
			      !payload_clone_vector_heap(*roots, false, total)))
			return false;
		if (outputs)
		{
			if (!payload_clone_add(total, sizeof(*outputs)) ||
			    !payload_clone_vector_heap(*outputs, false, total))
				return false;
			for (const auto &item : *outputs)
				if (!player_item_snapshot_current_heap_bytes(item, &row) ||
				    !payload_clone_add(total, row))
					return false;
		}
		if (!payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count) const noexcept
	{
		constexpr size_t own = 2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
		size_t request = payload_clone_vector_frames;
		if (count > value.max_size() - value.size())
			return false;
		if (count > value.capacity() - value.size())
		{
			size_t next = value.size();
			if (!payload_clone_add(next, std::max(value.size(), count)) ||
			    next > value.max_size())
				next = value.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !payload_clone_add(request, next * sizeof(T)))
				return false;
		}
		return payload_clone_add(request, own) && peak(request);
	}
	template <typename T> bool fresh(size_t count, size_t extra = 0) const noexcept
	{
		size_t request = payload_clone_vector_frames;
		return count <= SIZE_MAX / sizeof(T) &&
		       payload_clone_add(request, count * sizeof(T)) &&
		       payload_clone_add(request, extra) &&
		       payload_clone_add(request,
					 sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool)) &&
		       peak(request);
	}
	template <typename T, typename Comparator> bool sort_frame(size_t count) const noexcept
	{
		size_t levels = 0, remaining = count,
		       request = item_entity_sort_leaf_frames<T, Comparator>();
		while (remaining > 1)
		{
			remaining >>= 1;
			++levels;
		}
		const size_t recursive =
			3 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(Comparator);
		return 2 * levels + 1 <= SIZE_MAX / recursive &&
		       payload_clone_add(request, (2 * levels + 1) * recursive) &&
		       payload_clone_add(request,
					 sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool)) &&
		       peak(request);
	}
};
// unique/__unique first,last,comp,result plus __dest,++first; equality
// wrapper and iterator/move assignment. erase/_M_erase and _M_erase_at_end
// reference/iterator/return/count/destructor scopes, with genuine copy closure.
constexpr size_t item_entity_unique_erase_frames =
	14 * sizeof(void *) + 2 * sizeof(char) + 3 * sizeof(bool) + sizeof(std::ptrdiff_t) +
	payload_clone_copy_frames + payload_clone_allocator_frames;
bool item_entity_selected_roots_owned(const item_transfer_payload &payload,
				      std::vector<uint64_t> *roots,
				      item_entity_construction_budget &budget)
{
	if (!roots)
		return false;
	try
	{
		roots->clear();
		if (!budget.fresh<uint64_t>(payload.item_count))
			return false;
		roots->reserve(payload.item_count);
		for (size_t index = 0; index < payload.item_count; ++index)
			if (selected_root_for(payload, payload.items[index].item_uid) ==
			    payload.items[index].item_uid)
			{
				if (!budget.growth(*roots, 1))
					return false;
				roots->push_back(payload.items[index].item_uid);
			}
		if (!budget.sort_frame<uint64_t, char>(roots->size()))
			return false;
		std::sort(roots->begin(), roots->end());
		if (!budget.peak(item_entity_unique_erase_frames))
			return false;
		roots->erase(std::unique(roots->begin(), roots->end()), roots->end());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return !roots->empty();
}

bool item_entity_populate_owned(critical_command *command, const item_transfer_payload &payload,
				item_entity_construction_budget &budget)
{
	critical_entity_key from_key = {}, to_key = {};
	size_t admission_prefix = 0;
	if (!command ||
	    !(budget.prefix(admission_prefix) &&
	      item_owner_key_bounded(payload.from_owner, &from_key, budget.reserve, budget.context,
				     admission_prefix)) ||
	    !(budget.prefix(admission_prefix) &&
	      item_owner_key_bounded(payload.to_owner, &to_key, budget.reserve, budget.context,
				     admission_prefix)))
		return false;
	if (!budget.fresh<critical_entity_key>(
		    2, 2 * sizeof(critical_entity_key) +
			       sizeof(std::initializer_list<critical_entity_key>) +
			       2 * sizeof(void *)))
		return false;
	command->keys = { from_key, to_key };
	if (!budget.fresh<critical_expected_revision>(
		    2, 2 * sizeof(critical_expected_revision) +
			       sizeof(std::initializer_list<critical_expected_revision>) +
			       2 * sizeof(void *)))
		return false;
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
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(item_key);
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back(
			{ item_key, payload.items[index].expected_item_revision });
	}
	if (payload.target_parent_item_uid)
	{
		critical_entity_key parent_key = { critical_entity_type::item,
						   payload.target_parent_item_uid };
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(parent_key);
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back(
			{ parent_key, payload.expected_target_parent_revision });
	}
	if (payload.reason == item_transfer_reason::craft)
	{
		if (!budget.peak(sizeof(std::vector<player_item_snapshot>) + 4 * sizeof(void *) +
				 sizeof(std::allocator<player_item_snapshot>)))
			return false;
		std::vector<player_item_snapshot> outputs;
		budget.outputs = &outputs;
		if (!(budget.prefix(admission_prefix) &&
		      item_transfer_craft_outputs_decode_bounded(payload, &outputs, budget.reserve,
								 budget.context, admission_prefix,
								 nullptr)))
			return false;
		for (const player_item_snapshot &output : outputs)
		{
			const critical_entity_key output_key = { critical_entity_type::item,
								 output.object_uid };
			if (!budget.growth(command->keys, 1))
				return false;
			command->keys.push_back(output_key);
			if (!budget.growth(command->expected_revisions, 1))
				return false;
			command->expected_revisions.push_back(
				{ output_key, ITEM_TRANSFER_ABSENT_REVISION });
		}
		budget.outputs = nullptr;
	}
	if (payload.collector.present)
	{
		const critical_entity_key catalog_key = { critical_entity_type::collector,
							  COLLECTOR_CATALOG_KEY };
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(catalog_key);
		// The SQL repository takes the current catalog row lock before any item
		// lock; zero is a serialization key, not an optimistic catalog fence.
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back({ catalog_key, 0 });
	}
	if (payload.native_recovery.present &&
	    payload.native_mobile.action == item_native_mobile_action::consumption &&
	    !payload.native_cost.fee_only)
	{
		const critical_entity_key giver_key = { critical_entity_type::player,
							payload.native_recovery.player_pid };
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(giver_key);
		// A serialization key only; the acknowledged save fence remains distinct
		// from a player custody-owner revision and is checked by the original root.
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back({ giver_key, 0 });
	}
	if (payload.native_recovery.present && command->keys.size() > CRITICAL_COMMAND_MAX_KEYS)
		return false;
	if (!budget.sort_frame<critical_entity_key, decltype(&critical_entity_key_less)>(
		    command->keys.size()))
		return false;
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	if (std::adjacent_find(command->keys.begin(), command->keys.end(),
			       critical_entity_key_equal) != command->keys.end())
		return false;
	if (!budget.sort_frame<critical_expected_revision, char>(
		    command->expected_revisions.size()))
		return false;
	std::sort(command->expected_revisions.begin(), command->expected_revisions.end(),
		  [](const critical_expected_revision &left,
		     const critical_expected_revision &right)
		  { return critical_entity_key_less(left.key, right.key); });
	return true;
}
} // namespace
bool item_transfer_selected_roots_bounded(const item_transfer_payload &payload,
					  std::vector<uint64_t> *roots,
					  bool (*reserve)(size_t, void *) noexcept, void *context,
					  size_t outer_live) noexcept
{
	if (!roots || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 8 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(bool) +
				  item_payload_validation_pure_frames +
				  item_continuation_lower_bound_frames +
				  payload_clone_vector_frames + payload_clone_move_frames;
	item_entity_construction_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(std::vector<uint64_t>) + 4 * sizeof(void *) +
			 sizeof(std::allocator<uint64_t>)))
		return false;
	try
	{
		std::vector<uint64_t> candidate;
		budget.roots = &candidate;
		if (!item_entity_selected_roots_owned(payload, &candidate, budget) ||
		    !budget.peak(payload_clone_move_frames))
			return false;
		*roots = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_command_entities_bounded(critical_command *command,
					    const item_transfer_payload &payload,
					    bool (*reserve)(size_t, void *) noexcept, void *context,
					    size_t outer_live) noexcept
{
	if (!command || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		8 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool) +
		// Original from/to/item/parent/output/catalog/giver key declarations,
		// actual expected-revision aggregate argument temporary and loop refs.
		7 * sizeof(critical_entity_key) + sizeof(critical_expected_revision) +
		4 * sizeof(void *) + sizeof(size_t) + item_payload_validation_pure_frames +
		payload_clone_vector_frames + 2 * payload_clone_move_frames;
	item_entity_construction_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(critical_command) +
			 4 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>))))
		return false;
	try
	{
		// Only original entity vectors are populated; authentic existing command
		// payload/accounting/scalars stay untouched and caller-owned throughout.
		critical_command candidate = {};
		budget.command = &candidate;
		if (!item_entity_populate_owned(&candidate, payload, budget) ||
		    !budget.peak(2 * payload_clone_move_frames))
			return false;
		command->keys = std::move(candidate.keys);
		command->expected_revisions = std::move(candidate.expected_revisions);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
struct item_payload_wire_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const std::vector<uint8_t> *bytes = nullptr, *corpse = nullptr, *collector = nullptr,
				   *recovery = nullptr;
	const item_transfer_payload *payload = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observations =
			13 * sizeof(void *) + 9 * sizeof(size_t) + 4 * sizeof(bool) +
			5 * (sizeof(void *) + sizeof(size_t)) + payload_clone_observation_frames;
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    !payload_clone_add(total, observations))
			return false;
		if ((bytes && (!payload_clone_add(total, sizeof(*bytes)) ||
			       !payload_clone_vector_heap(*bytes, false, total))) ||
		    (corpse && (!payload_clone_add(total, sizeof(*corpse)) ||
				!payload_clone_vector_heap(*corpse, false, total))) ||
		    (collector && (!payload_clone_add(total, sizeof(*collector)) ||
				   !payload_clone_vector_heap(*collector, false, total))) ||
		    (recovery && (!payload_clone_add(total, sizeof(*recovery)) ||
				  !payload_clone_vector_heap(*recovery, false, total))) ||
		    (payload && (!payload_clone_add(total, sizeof(*payload)) ||
				 !payload_clone_heap(*payload, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
bool item_generic_wire_encode_owned(const item_transfer_payload &payload, uint16_t version,
				    std::vector<uint8_t> *encoded, item_payload_wire_budget &budget)
{
	if (!budget.peak(3 * sizeof(std::vector<uint8_t>) +
			 3 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>))))
		return false;
	std::vector<uint8_t> corpse_context;
	std::vector<uint8_t> collector_context;
	std::vector<uint8_t> recovery_context;
	budget.corpse = &corpse_context;
	budget.collector = &collector_context;
	budget.recovery = &recovery_context;
	size_t admission_prefix = 0;
	if (!encoded ||
	    !(budget.prefix(admission_prefix) &&
	      item_transfer_payload_valid_bounded(payload, version, budget.reserve, budget.context,
						  admission_prefix)) ||
	    !(budget.prefix(admission_prefix) &&
	      item_transfer_corpse_context_encode_bounded(payload.corpse, &corpse_context,
							  budget.reserve, budget.context,
							  admission_prefix)) ||
	    !(budget.prefix(admission_prefix) &&
	      item_transfer_collector_context_encode_bounded(payload.collector, &collector_context,
							     budget.reserve, budget.context,
							     admission_prefix)))
		return false;
	if (version == ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION &&
	    !(budget.prefix(admission_prefix) &&
	      item_transfer_native_recovery_encode_bounded(
		      payload.native_recovery, &recovery_context, budget.reserve, budget.context,
		      admission_prefix, nullptr)))
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
	if (!budget.peak(payload_size + item_sidecar_fill_assign_frames +
			 item_sidecar_size_constructor_frames + payload_clone_vector_frames))
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
	// Lockpick retirement has no destination tree; keep its validated zero root.
	put_u64(encoded->data() + TARGET_ROOT_OFFSET,
		(payload.multi_root ||
		 payload.continuation.kind ==
			 item_transfer_continuation_kind::lockpick_retirement) ?
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
		if (!budget.peak(sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
				 2 * sizeof(void *) + sizeof(size_t)))
			return false;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference = {};
		if (!budget.prefix(admission_prefix, sizeof(reference)))
			return false;
		if (quest_mobile_native_reference_encode_bounded(
			    payload.native_mobile.reference, &reference, budget.reserve,
			    budget.context, admission_prefix) != player_snapshot_codec_result::ok)
			return false;
		std::copy(reference.begin(), reference.end(), output + 16);
		std::copy(recovery_context.begin(), recovery_context.end(),
			  output + ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES);
	}
	budget.corpse = nullptr;
	budget.collector = nullptr;
	budget.recovery = nullptr;
	return true;
}

} // namespace
bool item_transfer_payload_encode_version_bounded(const item_transfer_payload &payload,
						  uint16_t version, std::vector<uint8_t> *encoded,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer_live,
						  size_t *retained_encoded_heap_bytes) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		// Public/private input/output/budget/callback/context args, version,
		// original size/tail/selected/index/output/entry named local carriers.
		11 * sizeof(void *) + 2 * sizeof(uint16_t) + 14 * sizeof(size_t) +
		3 * sizeof(bool) + sizeof(uint64_t) +
		// Full original fixed owner serialization and put16/32/64 values,
		// byte-loop source locals and actual scalar cast/return carriers.
		8 * sizeof(void *) + 3 * sizeof(uint16_t) + 2 * sizeof(uint32_t) +
		5 * sizeof(uint64_t) + 4 * sizeof(unsigned int) + item_sidecar_pure_frames +
		// Original copy/copy_n nested scopes plus actual copy_n this/first/n/
		// result, __copy_n first/n/result/tag, iterator increment/subtraction.
		payload_clone_copy_frames + 8 * sizeof(void *) + 3 * sizeof(size_t) +
		sizeof(std::random_access_iterator_tag) + payload_clone_vector_frames +
		payload_clone_move_frames;
	item_payload_wire_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
			 sizeof(std::allocator<uint8_t>)))
		return false;
	try
	{
		std::vector<uint8_t> candidate;
		budget.bytes = &candidate;
		if (!item_generic_wire_encode_owned(payload, version, &candidate, budget) ||
		    !budget.peak(payload_clone_move_frames))
			return false;
		const size_t heap = candidate.capacity();
		*encoded = std::move(candidate);
		if (retained_encoded_heap_bytes)
			*retained_encoded_heap_bytes = heap;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_command_encode_payload_bounded(const item_transfer_payload &payload,
						  std::vector<uint8_t> *encoded,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer_live) noexcept
{
	constexpr size_t frame = 5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
	size_t prefix = outer_live;
	return payload_clone_add(prefix, frame) &&
	       item_transfer_payload_encode_version_bounded(payload, ITEM_TRANSFER_PAYLOAD_VERSION,
							    encoded, reserve, context, prefix,
							    nullptr);
}

namespace
{
constexpr size_t item_generic_decode_default_payload_frames =
	// Actual ten nontrivial aggregate default ctor/destructor this scopes,
	// six vector/base/impl/data and six string/hider/local/NUL defaults.
	2 * 10 * sizeof(void *) + 6 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>)) +
	6 * (7 * sizeof(void *) + sizeof(std::allocator<char>) + sizeof(size_t) + sizeof(char)) +
	payload_clone_allocator_frames;
constexpr size_t item_generic_decode_predicate_equal_frames =
	// equal(first,last,second,pred), __niter_base and actual normal iterator
	// bool/++/dereference scope. This is the original predicate overload,
	// not the byte memcmp specialization of the other equality provider.
	3 * sizeof(void *) + sizeof(char) + sizeof(bool) + 3 * (2 * sizeof(void *)) +
	6 * (2 * sizeof(void *)) + sizeof(bool) +
	// Original key lambda this/left/right/bool, expected-revision lambda
	// this/left/right/bool plus nested key equality left/right/bool.
	2 * (3 * sizeof(void *) + sizeof(bool)) + 2 * (2 * sizeof(void *) + sizeof(bool));
struct item_generic_decode_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const item_transfer_payload *payload = nullptr;
	const critical_command *expected = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observations =
			9 * sizeof(void *) + 6 * sizeof(size_t) + 4 * sizeof(bool) +
			5 * (sizeof(void *) + sizeof(size_t)) + payload_clone_observation_frames;
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    !payload_clone_add(total, observations) ||
		    (payload && (!payload_clone_add(total, sizeof(*payload)) ||
				 !payload_clone_heap(*payload, false, total))) ||
		    (expected &&
		     (!payload_clone_add(total, sizeof(*expected)) ||
		      !payload_clone_vector_heap(expected->keys, false, total) ||
		      !payload_clone_vector_heap(expected->expected_revisions, false, total) ||
		      !payload_clone_vector_heap(expected->payload, false, total) ||
		      !payload_clone_vector_heap(expected->accounting_intent, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
bool item_generic_decode_owned(const critical_command &command, item_transfer_payload *payload,
			       item_generic_decode_budget &budget)
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
	if (!budget.peak(sizeof(item_transfer_payload) +
			 item_generic_decode_default_payload_frames + payload_clone_frames))
		return false;
	*payload = {};
	size_t admission_prefix = 0;
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
			    !(budget.prefix(admission_prefix) &&
			      item_transfer_corpse_context_decode_bounded(
				      command.payload.data() + item_end + sizeof(uint32_t),
				      corpse_size, &payload->corpse, budget.reserve, budget.context,
				      admission_prefix)))
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
				    !(budget.prefix(admission_prefix) &&
				      item_transfer_collector_context_decode_bounded(
					      command.payload.data() + corpse_end + sizeof(uint32_t),
					      collector_size, &payload->collector, budget.reserve,
					      budget.context, admission_prefix)))
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
					if (!budget.peak(continuation_size +
							 payload_clone_vector_frames))
						return false;
					payload->continuation.data.assign(
						command.payload.begin() + source_end +
							sizeof(uint32_t) * 2,
						command.payload.begin() + source_end +
							sizeof(uint32_t) * 2 + continuation_size);
					if (native_mobile_version(command.payload_version))
					{
						if (!budget.prefix(admission_prefix))
							return false;
						const uint8_t *tail =
							command.payload.data() + source_end +
							sizeof(uint32_t) * 2 + continuation_size;
						if (get_u16(tail) !=
							    ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_VERSION ||
						    tail[3] ||
						    get_u32(tail + 4) !=
							    ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES ||
						    get_u32(tail + 12) || get_u32(tail + 164) ||
						    quest_mobile_native_reference_decode_bounded(
							    std::span<const uint8_t>(
								    tail + 16,
								    QUEST_MOBILE_NATIVE_REFERENCE_BYTES),
							    &payload->native_mobile.reference,
							    budget.reserve, budget.context,
							    admission_prefix) !=
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
							if (!(budget.prefix(admission_prefix) &&
							      item_transfer_native_recovery_decode_bounded(
								      std::span<const uint8_t>(
									      command.payload)
									      .subspan(offset),
								      &payload->native_recovery,
								      budget.reserve,
								      budget.context,
								      admission_prefix, nullptr)))
								return false;
						}
					}
				}
			}
		}
	}
	if (!(budget.prefix(admission_prefix) &&
	      item_transfer_payload_valid_bounded(*payload, command.payload_version, budget.reserve,
						  budget.context, admission_prefix)) ||
	    (command.payload_version == ITEM_TRANSFER_LEGACY_PAYLOAD_VERSION &&
	     payload->reason > item_transfer_reason::auction_claim) ||
	    command.expected_revisions.size() != command.keys.size())
		return false;
	if (!budget.peak(sizeof(critical_command) +
			 4 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>))))
		return false;
	critical_command expected = {};
	budget.expected = &expected;
	if (!(budget.prefix(admission_prefix) &&
	      item_transfer_command_entities_bounded(&expected, *payload, budget.reserve,
						     budget.context, admission_prefix)))
		return false;
	if (!budget.peak(item_generic_decode_predicate_equal_frames))
		return false;
	const bool matched =
		command.keys.size() == expected.keys.size() &&
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
	budget.expected = nullptr;
	return matched;
}

} // namespace
bool item_transfer_payload_decode_generic_bounded(const critical_command &command,
						  item_transfer_payload *payload,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer_live,
						  size_t *retained_payload_heap_bytes) noexcept
{
	if (!payload || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		// Public/private command/payload/budget/callback/context/outer/scalar
		// carriers, original version/variable-items/indices/offsets/byte/tail.
		10 * sizeof(void *) + 18 * sizeof(size_t) + 6 * sizeof(uint32_t) +
		sizeof(uint16_t) + 5 * sizeof(bool) + sizeof(uint8_t) +
		// Original decode_owner input/result and two caller-owned returned
		// owner identity carriers; fixed get16/32/64 source loops and values.
		sizeof(void *) + 3 * sizeof(item_owner_identity) + 8 * sizeof(void *) +
		2 * sizeof(uint16_t) + 3 * sizeof(uint32_t) + 6 * sizeof(uint64_t) +
		4 * sizeof(unsigned int) +
		// Original fixed item entry initializer and two real span parameter/
		// return values; subspan this/offset/count/constructed-span scopes.
		sizeof(item_transfer_entry) + 3 * sizeof(std::span<const uint8_t>) +
		3 * sizeof(size_t) + item_sidecar_pure_frames + payload_clone_copy_frames +
		8 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::random_access_iterator_tag) +
		payload_clone_vector_frames + item_generic_decode_default_payload_frames +
		payload_clone_frames + sizeof(size_t);
	item_generic_decode_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(item_transfer_payload) +
			 item_generic_decode_default_payload_frames))
		return false;
	try
	{
		item_transfer_payload candidate = {};
		budget.payload = &candidate;
		if (!item_generic_decode_owned(command, &candidate, budget) ||
		    !budget.peak(payload_clone_frames))
			return false;
		size_t heap = 0;
		if (!payload_clone_heap(candidate, false, heap))
			return false;
		static_assert(std::is_nothrow_move_assignable_v<item_transfer_payload>);
		*payload = std::move(candidate);
		if (retained_payload_heap_bytes)
			*retained_payload_heap_bytes = heap;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
struct item_native_shape_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const std::vector<uint8_t> *exact = nullptr;
	const quest_reward_continuation *terms = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observation = 8 * sizeof(void *) + 6 * sizeof(size_t) +
					       3 * sizeof(bool) +
					       3 * (sizeof(void *) + sizeof(size_t));
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    !payload_clone_add(total, observation) ||
		    (exact && (!payload_clone_add(total, sizeof(*exact)) ||
			       !payload_clone_vector_heap(*exact, false, total))) ||
		    (terms && (!payload_clone_add(total, sizeof(*terms)) ||
			       !payload_clone_string_heap(terms->character_name, false, total) ||
			       !payload_clone_string_heap(terms->definition_id, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
constexpr size_t item_native_shape_fixed_frames =
	// Original pure collector/publication/owner/key/reference/cash equality
	// queries; genuine empty projections and absent forest temporaries.
	item_payload_validation_pure_frames + item_native_validation_equal_frames +
	2 * sizeof(shop_trade_recovery_forest_binding) + sizeof(native_quest_cost_projection) +
	sizeof(native_quest_coin_give_projection) +
	2 * (4 * sizeof(void *) + sizeof(std::allocator<uint64_t>)) +
	4 * (sizeof(void *) + sizeof(size_t)) + payload_clone_allocator_frames;
// Original native_quest_coin_give_project and native_counts are proved
// allocation-free from their complete installed source. Its real parameters,
// fixed candidate/units array and scalar/loop temporaries coexist with caller
// projected, which is separately charged in the money method frame below.
constexpr size_t item_native_shape_coin_project_frames =
	3 * sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(uint8_t) + sizeof(int32_t) +
	sizeof(native_quest_coin_give_result) + sizeof(native_quest_coin_give_projection) +
	sizeof(std::array<int64_t, 4>) + 2 * sizeof(int64_t) + sizeof(size_t) + sizeof(void *) +
	sizeof(int64_t) + sizeof(bool) + 8 * (sizeof(void *) + sizeof(size_t)) +
	sizeof(std::array<int64_t, 4>);
bool item_native_cost_value_valid_owned(const item_transfer_payload &payload,
					item_native_shape_budget &budget)
{
	if (!payload.native_cost.present || !payload.native_cost.wallet_mapping_id ||
	    payload.native_mobile.action != item_native_mobile_action::consumption ||
	    payload.native_cost.projection.attempts.empty())
		return false;
	if (!budget.peak(sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
			 sizeof(std::allocator<uint8_t>)))
		return false;
	std::vector<uint8_t> exact;
	budget.exact = &exact;
	size_t admission_prefix = 0;
	if (!budget.prefix(admission_prefix))
		return false;
	return native_quest_cost_projection_encode_bounded(payload.native_cost.projection, &exact,
							   budget.reserve, budget.context,
							   admission_prefix) ==
	       native_quest_cost_projection_result::ok;
}

bool item_native_fee_shape_owned(const item_transfer_payload &payload, bool acknowledged,
				 item_native_shape_budget &budget)
{
	const auto &fee = payload.native_cost;
	const auto &native = payload.native_mobile;
	const auto &recovery = payload.native_recovery;
	size_t admission_prefix = 0;
	if (!fee.fee_only ||
	    !(budget.prefix(admission_prefix) &&
	      item_transfer_native_cost_value_valid_bounded(payload, budget.reserve, budget.context,
							    admission_prefix)) ||
	    !native.present || native.action != item_native_mobile_action::consumption ||
	    !native.final_giver_pid || native.final_giver_pid > INT32_MAX ||
	    native.reference.mobile_revision == UINT64_MAX ||
	    payload.from_owner.type != item_owner_type::native_mobile ||
	    payload.from_owner.id != native.reference.mobile_instance_id ||
	    payload.from_owner.context_id || payload.to_owner.type != item_owner_type::player ||
	    payload.to_owner.id != native.final_giver_pid || payload.to_owner.context_id ||
	    payload.reason != item_transfer_reason::quest_turnin ||
	    payload.reason_id != native.reference.mobile_vnum || payload.logical_source_id ||
	    payload.multi_root || payload.item_count || payload.item_blob_size ||
	    payload.selected_item_uid || payload.target_root_item_uid ||
	    payload.target_parent_item_uid || payload.expected_target_parent_revision ||
	    payload.corpse.present || payload.collector.present ||
	    !valid_collector_context(payload, ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION) ||
	    payload.native_money.present || payload.native_money.original_room_vnum ||
	    payload.native_money.player_wallet_mapping_id ||
	    payload.native_money.mobile_wallet_mapping_id ||
	    payload.native_money.projection != native_quest_coin_give_projection{} ||
	    payload.expected_from_revision != native.reference.stock_revision ||
	    payload.expected_to_revision == UINT64_MAX || !recovery.consumed_root_order.empty())
		return false;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
	if (!budget.prefix(admission_prefix))
		return false;
	if (quest_mobile_native_reference_encode_bounded(
		    native.reference, &reference, budget.reserve, budget.context,
		    admission_prefix) != player_snapshot_codec_result::ok)
		return false;
	if (payload.continuation.kind == item_transfer_continuation_kind::quest_offering)
	{
		if (!budget.peak(sizeof(quest_reward_continuation) +
				 duris_quest_continuation_bounded_detail::string_lifetime_frames +
				 sizeof(void *)))
			return false;
		quest_reward_continuation terms;
		budget.terms = &terms;
		if (!budget.prefix(admission_prefix))
			return false;
		if (!quest_fee_reward_continuation_decode_bounded(
			    payload.continuation.data.data(), payload.continuation.data.size(),
			    &terms, budget.reserve, budget.context, admission_prefix) ||
		    terms.player_pid != native.final_giver_pid ||
		    terms.mobile_vnum != static_cast<uint32_t>(native.reference.mobile_vnum) ||
		    terms.action_mobile_instance_id != native.reference.mobile_instance_id ||
		    terms.completion_index != fee.completion_slot ||
		    terms.action_source.generation.bytes !=
			    native.reference.birth_source.generation.bytes ||
		    terms.action_source.sequence != native.reference.mobile_revision)
			return false;
		budget.terms = nullptr;
	}
	else if (payload.continuation.kind != item_transfer_continuation_kind::none ||
		 !payload.continuation.data.empty())
		return false;
	if (!acknowledged)
		return !recovery.present && !recovery.player_pid &&
		       !recovery.acknowledged_save_revision &&
		       recovery.player_before == shop_trade_recovery_forest_binding{} &&
		       recovery.player_after == shop_trade_recovery_forest_binding{} &&
		       native_publication_terms_empty(recovery.publication_terms);
	return recovery.present && recovery.player_pid == native.final_giver_pid &&
	       recovery.acknowledged_save_revision &&
	       native_publication_terms_valid(recovery.publication_terms) &&
	       !recovery.publication_terms.disappear &&
	       recovery.player_before.canonical_bytes == recovery.player_after.canonical_bytes &&
	       recovery.player_before.ordered_item_uids ==
		       recovery.player_after.ordered_item_uids &&
	       (budget.prefix(admission_prefix) &&
		shop_trade_recovery_forest_shape_valid_bounded(
			recovery.player_before, shop_trade_recovery_forest_role::player_before,
			budget.reserve, budget.context, admission_prefix)) &&
	       (budget.prefix(admission_prefix) &&
		shop_trade_recovery_forest_shape_valid_bounded(
			recovery.player_after, shop_trade_recovery_forest_role::player_after,
			budget.reserve, budget.context, admission_prefix));
}

bool item_native_money_shape_owned(const item_transfer_payload &payload, bool acknowledged,
				   item_native_shape_budget &budget)
{
	const auto &money = payload.native_money;
	const auto &native = payload.native_mobile;
	const auto &recovery = payload.native_recovery;
	size_t admission_prefix = 0;
	if (!money.present || money.original_room_vnum < 0 || !money.player_wallet_mapping_id ||
	    !money.mobile_wallet_mapping_id ||
	    money.player_wallet_mapping_id == money.mobile_wallet_mapping_id ||
	    payload.native_cost.present || payload.native_cost.fee_only ||
	    payload.native_cost.completion_slot || payload.native_cost.wallet_mapping_id ||
	    payload.native_cost.projection != native_quest_cost_projection{} || !native.present ||
	    native.action != item_native_mobile_action::acceptance || !native.final_giver_pid ||
	    native.final_giver_pid > INT32_MAX ||
	    payload.from_owner.type != item_owner_type::player ||
	    payload.from_owner.id != native.final_giver_pid || payload.from_owner.context_id ||
	    payload.to_owner.type != item_owner_type::native_mobile ||
	    payload.to_owner.id != native.reference.mobile_instance_id ||
	    payload.to_owner.context_id || payload.reason != item_transfer_reason::player_give ||
	    payload.reason_id != native.reference.mobile_vnum || payload.logical_source_id ||
	    payload.multi_root || payload.item_count || payload.item_blob_size ||
	    payload.selected_item_uid || payload.target_root_item_uid ||
	    payload.target_parent_item_uid || payload.expected_target_parent_revision ||
	    payload.corpse.present || payload.collector.present ||
	    !valid_collector_context(payload, ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION) ||
	    payload.continuation.kind != item_transfer_continuation_kind::none ||
	    !payload.continuation.data.empty() || payload.expected_from_revision == UINT64_MAX ||
	    payload.expected_to_revision != native.reference.stock_revision ||
	    !native_publication_terms_empty(recovery.publication_terms) ||
	    !recovery.consumed_root_order.empty())
		return false;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> encoded{};
	native_quest_coin_give_projection projected;
	if (!budget.prefix(admission_prefix) || !budget.peak(item_native_shape_coin_project_frames))
		return false;
	if (quest_mobile_native_reference_encode_bounded(native.reference, &encoded, budget.reserve,
							 budget.context, admission_prefix) !=
		    player_snapshot_codec_result::ok ||
	    native.reference.mobile_revision == UINT64_MAX ||
	    native_quest_coin_give_project(
		    money.projection.player_before, money.projection.player_before_revision,
		    money.projection.mobile_before, money.projection.mobile_before_revision,
		    money.projection.denomination, money.projection.quantity,
		    &projected) != native_quest_coin_give_result::ok ||
	    projected != money.projection)
		return false;
	if (!acknowledged)
		return !recovery.present && !recovery.player_pid &&
		       !recovery.acknowledged_save_revision &&
		       recovery.player_before == shop_trade_recovery_forest_binding{} &&
		       recovery.player_after == shop_trade_recovery_forest_binding{};
	return recovery.present && recovery.player_pid == native.final_giver_pid &&
	       recovery.acknowledged_save_revision && recovery.player_before.present &&
	       recovery.player_after.present &&
	       recovery.player_before.canonical_bytes == recovery.player_after.canonical_bytes &&
	       recovery.player_before.ordered_item_uids ==
		       recovery.player_after.ordered_item_uids &&
	       (budget.prefix(admission_prefix) &&
		shop_trade_recovery_forest_shape_valid_bounded(
			recovery.player_before, shop_trade_recovery_forest_role::player_before,
			budget.reserve, budget.context, admission_prefix)) &&
	       (budget.prefix(admission_prefix) &&
		shop_trade_recovery_forest_shape_valid_bounded(
			recovery.player_after, shop_trade_recovery_forest_role::player_after,
			budget.reserve, budget.context, admission_prefix));
}
} // namespace
bool item_transfer_native_cost_value_valid_bounded(const item_transfer_payload &payload,
						   bool (*reserve)(size_t, void *) noexcept,
						   void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 7 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool) +
				  payload_clone_vector_frames;
	item_native_shape_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_native_cost_value_valid_owned(payload, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_fee_shape_valid_bounded(const item_transfer_payload &payload,
						  bool acknowledged,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 10 * sizeof(void *) + 4 * sizeof(size_t) + 4 * sizeof(bool) +
				  sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
				  item_native_shape_fixed_frames;
	item_native_shape_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_native_fee_shape_owned(payload, acknowledged, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_money_shape_valid_bounded(const item_transfer_payload &payload,
						    bool acknowledged,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 10 * sizeof(void *) + 4 * sizeof(size_t) + 4 * sizeof(bool) +
				  sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
				  sizeof(native_quest_coin_give_projection) +
				  item_native_shape_fixed_frames;
	item_native_shape_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_native_money_shape_owned(payload, acknowledged, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
struct item_native_wire_encode_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const std::vector<uint8_t> *first = nullptr, *second = nullptr, *value = nullptr;
	const item_transfer_payload *original = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observation =
			11 * sizeof(void *) + 7 * sizeof(size_t) + 4 * sizeof(bool) +
			4 * (sizeof(void *) + sizeof(size_t)) + payload_clone_observation_frames;
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    !payload_clone_add(total, observation) ||
		    (first && (!payload_clone_add(total, sizeof(*first)) ||
			       !payload_clone_vector_heap(*first, false, total))) ||
		    (second && (!payload_clone_add(total, sizeof(*second)) ||
				!payload_clone_vector_heap(*second, false, total))) ||
		    (value && (!payload_clone_add(total, sizeof(*value)) ||
			       !payload_clone_vector_heap(*value, false, total))) ||
		    (original && (!payload_clone_add(total, sizeof(*original)) ||
				  !payload_clone_heap(*original, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	bool growth(const std::vector<uint8_t> &bytes, size_t count) const noexcept
	{
		constexpr size_t own = 2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
		size_t request = payload_clone_vector_frames;
		if (count > bytes.max_size() - bytes.size())
			return false;
		if (count > bytes.capacity() - bytes.size())
		{
			size_t next = bytes.size();
			if (!payload_clone_add(next, std::max(bytes.size(), count)) ||
			    next > bytes.max_size())
				next = bytes.max_size();
			if (!payload_clone_add(request, next))
				return false;
		}
		return payload_clone_add(request, own) && peak(request);
	}
	bool clone_payload(const item_transfer_payload &source) const noexcept
	{
		size_t request = 0;
		return item_transfer_payload_fresh_copy_request_bytes(source, &request) &&
		       payload_clone_add(request, sizeof(item_transfer_payload)) &&
		       payload_clone_add(request, payload_clone_frames + 2 * sizeof(void *) +
							  2 * sizeof(size_t) + sizeof(bool)) &&
		       peak(request);
	}
};
bool item_encode_native_fee_owned(const item_transfer_payload &payload, bool acknowledged,
				  std::vector<uint8_t> *output,
				  item_native_wire_encode_budget &budget)
{
	size_t admission_prefix = 0;
	if (!output ||
	    !(budget.prefix(admission_prefix) &&
	      item_transfer_native_fee_shape_valid_bounded(payload, acknowledged, budget.reserve,
							   budget.context, admission_prefix)))
		return false;
	if (!budget.peak(2 * sizeof(std::vector<uint8_t>) +
			 2 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>))))
		return false;
	std::vector<uint8_t> cost, recovery;
	budget.first = &cost;
	budget.second = &recovery;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
	if (!budget.prefix(admission_prefix))
		return false;
	if (native_quest_cost_projection_encode_bounded(
		    payload.native_cost.projection, &cost, budget.reserve, budget.context,
		    admission_prefix) != native_quest_cost_projection_result::ok ||
	    (!budget.prefix(admission_prefix) ?
		     player_snapshot_codec_result::invalid_value :
		     quest_mobile_native_reference_encode_bounded(
			     payload.native_mobile.reference, &reference, budget.reserve,
			     budget.context, admission_prefix)) !=
		    player_snapshot_codec_result::ok ||
	    (acknowledged && !(budget.prefix(admission_prefix) &&
			       item_transfer_native_recovery_encode_bounded(
				       payload.native_recovery, &recovery, budget.reserve,
				       budget.context, admission_prefix, nullptr))))
		return false;
	const size_t bytes = native_fee_header_bytes + reference.size() + cost.size() +
			     payload.continuation.data.size() + recovery.size();
	if (bytes > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	if (!budget.peak(sizeof(std::vector<uint8_t>) + native_fee_header_bytes +
			 item_native_recovery_fill_constructor_frames))
		return false;
	std::vector<uint8_t> value(native_fee_header_bytes, 0);
	budget.value = &value;
	value[0] = 'N';
	value[1] = 'Q';
	value[2] = 'F';
	value[3] = '2';
	put_u16(value.data() + 4, 1);
	put_u16(value.data() + 6,
		acknowledged ? ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION :
			       ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION);
	put_u32(value.data() + 8, static_cast<uint32_t>(bytes));
	put_u32(value.data() + 12, static_cast<uint32_t>(cost.size()));
	put_u32(value.data() + 16, static_cast<uint32_t>(payload.continuation.data.size()));
	put_u32(value.data() + 20, static_cast<uint32_t>(recovery.size()));
	put_u32(value.data() + 24, payload.native_mobile.final_giver_pid);
	put_u32(value.data() + 28, payload.native_cost.completion_slot);
	put_u64(value.data() + 32, payload.expected_from_revision);
	put_u64(value.data() + 40, payload.expected_to_revision);
	put_u64(value.data() + 48, payload.native_cost.wallet_mapping_id);
	put_u16(value.data() + 56, static_cast<uint16_t>(payload.continuation.kind));
	if (!budget.growth(value, reference.size()))
		return false;
	value.insert(value.end(), reference.begin(), reference.end());
	if (!budget.growth(value, cost.size()))
		return false;
	value.insert(value.end(), cost.begin(), cost.end());
	if (!budget.growth(value, payload.continuation.data.size()))
		return false;
	value.insert(value.end(), payload.continuation.data.begin(),
		     payload.continuation.data.end());
	if (!budget.growth(value, recovery.size()))
		return false;
	value.insert(value.end(), recovery.begin(), recovery.end());
	if (!budget.peak(payload_clone_move_frames))
		return false;
	*output = std::move(value);
	return true;
}

bool item_encode_native_money_owned(const item_transfer_payload &payload, bool acknowledged,
				    std::vector<uint8_t> *output,
				    item_native_wire_encode_budget &budget)
{
	size_t admission_prefix = 0;
	if (!output ||
	    !(budget.prefix(admission_prefix) &&
	      item_transfer_native_money_shape_valid_bounded(payload, acknowledged, budget.reserve,
							     budget.context, admission_prefix)))
		return false;
	if (!budget.peak(2 * sizeof(std::vector<uint8_t>) +
			 2 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>))))
		return false;
	std::vector<uint8_t> money, recovery;
	budget.first = &money;
	budget.second = &recovery;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
	if (!budget.prefix(admission_prefix))
		return false;
	if (native_quest_coin_give_encode_bounded(
		    payload.native_money.projection, &money, budget.reserve, budget.context,
		    admission_prefix) != native_quest_coin_give_result::ok ||
	    (!budget.prefix(admission_prefix) ?
		     player_snapshot_codec_result::invalid_value :
		     quest_mobile_native_reference_encode_bounded(
			     payload.native_mobile.reference, &reference, budget.reserve,
			     budget.context, admission_prefix)) !=
		    player_snapshot_codec_result::ok ||
	    (acknowledged && !(budget.prefix(admission_prefix) &&
			       item_transfer_native_recovery_encode_bounded(
				       payload.native_recovery, &recovery, budget.reserve,
				       budget.context, admission_prefix, nullptr))))
		return false;
	const size_t bytes = ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES + reference.size() +
			     money.size() + recovery.size();
	if (bytes > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	if (!budget.peak(sizeof(std::vector<uint8_t>) +
			 ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES +
			 item_native_recovery_fill_constructor_frames))
		return false;
	std::vector<uint8_t> result(ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES, 0);
	budget.value = &result;
	result[0] = 'N';
	result[1] = 'Q';
	result[2] = 'M';
	result[3] = '1';
	put_u16(result.data() + 4, 1);
	put_u16(result.data() + 6,
		acknowledged ? ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION :
			       ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION);
	put_u32(result.data() + 8, static_cast<uint32_t>(bytes));
	put_u32(result.data() + 12, static_cast<uint32_t>(recovery.size()));
	put_u32(result.data() + 16, payload.native_mobile.final_giver_pid);
	put_u32(result.data() + 20, static_cast<uint32_t>(payload.native_money.original_room_vnum));
	put_u64(result.data() + 24, payload.expected_from_revision);
	put_u64(result.data() + 32, payload.expected_to_revision);
	put_u64(result.data() + 40, payload.native_money.player_wallet_mapping_id);
	put_u64(result.data() + 48, payload.native_money.mobile_wallet_mapping_id);
	if (!budget.growth(result, reference.size()))
		return false;
	result.insert(result.end(), reference.begin(), reference.end());
	if (!budget.growth(result, money.size()))
		return false;
	result.insert(result.end(), money.begin(), money.end());
	if (!budget.growth(result, recovery.size()))
		return false;
	result.insert(result.end(), recovery.begin(), recovery.end());
	if (!budget.peak(payload_clone_move_frames))
		return false;
	*output = std::move(result);
	return true;
}

bool item_encode_native_cost_owned(const item_transfer_payload &payload, bool recovery,
				   std::vector<uint8_t> *encoded,
				   item_native_wire_encode_budget &budget)
{
	size_t admission_prefix = 0;
	if (!encoded || !(budget.prefix(admission_prefix) &&
			  item_transfer_native_cost_value_valid_bounded(
				  payload, budget.reserve, budget.context, admission_prefix)))
		return false;
	if (payload.native_cost.fee_only)
		return (budget.prefix(admission_prefix) &&
			item_transfer_native_fee_encode_bounded(payload, recovery, encoded,
								budget.reserve, budget.context,
								admission_prefix));
	if (payload.native_cost.completion_slot)
		return false;
	if (!budget.clone_payload(payload))
		return false;
	auto original = payload;
	budget.original = &original;
	if (!budget.peak(sizeof(item_native_mobile_cost_context) + 4 * sizeof(void *) +
			 sizeof(std::allocator<native_quest_cost_attempt>) +
			 payload_clone_move_frames))
		return false;
	original.native_cost = {};
	if (!budget.peak(2 * sizeof(std::vector<uint8_t>) +
			 2 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>))))
		return false;
	std::vector<uint8_t> body, cost;
	budget.first = &body;
	budget.second = &cost;
	const uint16_t original_version =
		recovery ? ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION :
			   ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION;
	if (!(budget.prefix(admission_prefix) &&
	      item_transfer_payload_encode_version_bounded(original, original_version, &body,
							   budget.reserve, budget.context,
							   admission_prefix, nullptr)) ||
	    (!budget.prefix(admission_prefix) ?
		     native_quest_cost_projection_result::invalid :
		     native_quest_cost_projection_encode_bounded(
			     payload.native_cost.projection, &cost, budget.reserve, budget.context,
			     admission_prefix)) != native_quest_cost_projection_result::ok ||
	    body.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES -
				  ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES ||
	    cost.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES -
				  ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES - body.size())
		return false;
	if (!budget.peak(sizeof(std::vector<uint8_t>) +
			 ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES + body.size() + cost.size() +
			 item_sidecar_size_constructor_frames + payload_clone_vector_frames))
		return false;
	std::vector<uint8_t> exact(ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES + body.size() +
				   cost.size());
	budget.value = &exact;
	exact[0] = 'N';
	exact[1] = 'Q';
	exact[2] = 'F';
	exact[3] = '1';
	put_u16(exact.data() + 4, 1);
	put_u16(exact.data() + 6, original_version);
	put_u32(exact.data() + 8, static_cast<uint32_t>(body.size()));
	put_u32(exact.data() + 12, static_cast<uint32_t>(cost.size()));
	put_u64(exact.data() + 16, payload.native_cost.wallet_mapping_id);
	std::copy(body.begin(), body.end(),
		  exact.begin() + ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES);
	std::copy(cost.begin(), cost.end(),
		  exact.begin() + ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES + body.size());
	if (!budget.peak(payload_clone_move_frames))
		return false;
	*encoded = std::move(exact);
	return true;
}
} // namespace

bool item_transfer_native_fee_encode_bounded(const item_transfer_payload &payload,
					     bool acknowledged, std::vector<uint8_t> *encoded,
					     bool (*reserve)(size_t, void *) noexcept,
					     void *context, size_t outer_live) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 10 * sizeof(void *) + 8 * sizeof(size_t) + 4 * sizeof(bool) +
				  2 * sizeof(uint16_t) +
				  sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
				  item_sidecar_pure_frames + payload_clone_copy_frames +
				  payload_clone_allocator_frames;
	item_native_wire_encode_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_encode_native_fee_owned(payload, acknowledged, encoded, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_native_money_encode_bounded(const item_transfer_payload &payload,
					       bool acknowledged, std::vector<uint8_t> *encoded,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer_live) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 10 * sizeof(void *) + 8 * sizeof(size_t) + 4 * sizeof(bool) +
				  2 * sizeof(uint16_t) +
				  sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
				  item_sidecar_pure_frames + payload_clone_copy_frames +
				  payload_clone_allocator_frames;
	item_native_wire_encode_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_encode_native_money_owned(payload, acknowledged, encoded, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_native_cost_encode_bounded(const item_transfer_payload &payload,
					      bool acknowledged, std::vector<uint8_t> *encoded,
					      bool (*reserve)(size_t, void *) noexcept,
					      void *context, size_t outer_live) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 10 * sizeof(void *) + 8 * sizeof(size_t) + 4 * sizeof(bool) +
				  2 * sizeof(uint16_t) +
				  sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
				  item_sidecar_pure_frames + payload_clone_copy_frames +
				  payload_clone_allocator_frames;
	item_native_wire_encode_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_encode_native_cost_owned(payload, acknowledged, encoded, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
struct item_native_wire_decode_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const item_transfer_payload *payload = nullptr;
	const critical_command *expected = nullptr;
	const std::vector<uint8_t> *canonical = nullptr;
	const quest_reward_continuation *terms = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observation =
			12 * sizeof(void *) + 8 * sizeof(size_t) + 4 * sizeof(bool) +
			7 * (sizeof(void *) + sizeof(size_t)) + payload_clone_observation_frames;
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    !payload_clone_add(total, observation) ||
		    (payload && (!payload_clone_add(total, sizeof(*payload)) ||
				 !payload_clone_heap(*payload, false, total))) ||
		    (canonical && (!payload_clone_add(total, sizeof(*canonical)) ||
				   !payload_clone_vector_heap(*canonical, false, total))) ||
		    (expected &&
		     (!payload_clone_add(total, sizeof(*expected)) ||
		      !payload_clone_vector_heap(expected->keys, false, total) ||
		      !payload_clone_vector_heap(expected->expected_revisions, false, total) ||
		      !payload_clone_vector_heap(expected->payload, false, total) ||
		      !payload_clone_vector_heap(expected->accounting_intent, false, total))) ||
		    (terms && (!payload_clone_add(total, sizeof(*terms)) ||
			       !payload_clone_string_heap(terms->character_name, false, total) ||
			       !payload_clone_string_heap(terms->definition_id, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
bool item_decode_native_fee_owned(const critical_command &command, item_transfer_payload *output,
				  item_native_wire_decode_budget &budget)
{
	constexpr size_t fixed = native_fee_header_bytes + QUEST_MOBILE_NATIVE_REFERENCE_BYTES;
	if (!output || !native_cost_version(command.payload_version) ||
	    command.type != critical_command_type::item_transfer ||
	    command.payload.size() < fixed ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	const auto *wire = command.payload.data();
	const bool acknowledged = command.payload_version ==
				  ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION;
	if (wire[0] != 'N' || wire[1] != 'Q' || wire[2] != 'F' || wire[3] != '2' ||
	    get_u16(wire + 4) != 1 || get_u16(wire + 6) != command.payload_version ||
	    get_u32(wire + 8) != command.payload.size() || get_u16(wire + 58) || get_u32(wire + 60))
		return false;
	const size_t cost_size = get_u32(wire + 12), continuation_size = get_u32(wire + 16),
		     recovery_size = get_u32(wire + 20);
	size_t remaining = command.payload.size() - fixed;
	if (cost_size > remaining)
		return false;
	remaining -= cost_size;
	if (continuation_size > remaining ||
	    continuation_size > ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
		return false;
	remaining -= continuation_size;
	if (recovery_size != remaining || (!acknowledged && recovery_size))
		return false;
	if (!budget.peak(sizeof(item_transfer_payload) +
			 item_generic_decode_default_payload_frames))
		return false;
	item_transfer_payload value{};
	budget.payload = &value;
	size_t admission_prefix = 0;
	if (!budget.prefix(admission_prefix))
		return false;
	value.native_mobile.present = true;
	value.native_mobile.action = item_native_mobile_action::consumption;
	value.native_mobile.final_giver_pid = get_u32(wire + 24);
	value.native_cost.present = value.native_cost.fee_only = true;
	value.native_cost.completion_slot = get_u32(wire + 28);
	value.native_cost.wallet_mapping_id = get_u64(wire + 48);
	value.expected_from_revision = get_u64(wire + 32);
	value.expected_to_revision = get_u64(wire + 40);
	if (quest_mobile_native_reference_decode_bounded(
		    { wire + native_fee_header_bytes, QUEST_MOBILE_NATIVE_REFERENCE_BYTES },
		    &value.native_mobile.reference, budget.reserve, budget.context,
		    admission_prefix) != player_snapshot_codec_result::ok ||
	    (!budget.prefix(admission_prefix) ?
		     native_quest_cost_projection_result::invalid :
		     native_quest_cost_projection_decode_bounded(
			     { wire + fixed, cost_size }, &value.native_cost.projection,
			     budget.reserve, budget.context, admission_prefix)) !=
		    native_quest_cost_projection_result::ok)
		return false;
	value.continuation.kind = static_cast<item_transfer_continuation_kind>(get_u16(wire + 56));
	if (!budget.peak(continuation_size + payload_clone_vector_frames))
		return false;
	value.continuation.data.assign(wire + fixed + cost_size,
				       wire + fixed + cost_size + continuation_size);
	if (acknowledged &&
	    !(budget.prefix(admission_prefix) &&
	      item_transfer_native_recovery_decode_bounded(
		      { wire + fixed + cost_size + continuation_size, recovery_size },
		      &value.native_recovery, budget.reserve, budget.context, admission_prefix,
		      nullptr)))
		return false;
	value.from_owner = { item_owner_type::native_mobile,
			     value.native_mobile.reference.mobile_instance_id, 0 };
	value.to_owner = { item_owner_type::player, value.native_mobile.final_giver_pid, 0 };
	value.reason = item_transfer_reason::quest_turnin;
	value.reason_id = value.native_mobile.reference.mobile_vnum;
	if (!budget.peak(sizeof(std::vector<uint8_t>) + sizeof(critical_command) +
			 5 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>))))
		return false;
	std::vector<uint8_t> canonical;
	critical_command expected{};
	budget.canonical = &canonical;
	budget.expected = &expected;
	if (!(budget.prefix(admission_prefix) &&
	      item_transfer_native_fee_encode_bounded(value, acknowledged, &canonical,
						      budget.reserve, budget.context,
						      admission_prefix)) ||
	    (!budget.peak(item_native_validation_equal_frames) || canonical != command.payload) ||
	    !(budget.prefix(admission_prefix) &&
	      item_transfer_command_entities_bounded(&expected, value, budget.reserve,
						     budget.context, admission_prefix)) ||
	    command.keys.size() != expected.keys.size() ||
	    command.expected_revisions.size() != expected.expected_revisions.size() ||
	    !budget.peak(item_generic_decode_predicate_equal_frames +
			 sizeof(decltype(&critical_entity_key_equal))) ||
	    !std::equal(command.keys.begin(), command.keys.end(), expected.keys.begin(),
			critical_entity_key_equal) ||
	    !std::equal(command.expected_revisions.begin(), command.expected_revisions.end(),
			expected.expected_revisions.begin(),
			[](const auto &a, const auto &b) {
				return critical_entity_key_equal(a.key, b.key) &&
				       a.revision == b.revision;
			}))
		return false;
	if (value.continuation.kind == item_transfer_continuation_kind::quest_offering)
	{
		if (!budget.peak(sizeof(quest_reward_continuation) +
				 duris_quest_continuation_bounded_detail::string_lifetime_frames +
				 sizeof(void *)))
			return false;
		quest_reward_continuation terms;
		budget.terms = &terms;
		if (!budget.prefix(admission_prefix))
			return false;
		if (!quest_fee_reward_continuation_decode_bounded(
			    value.continuation.data.data(), value.continuation.data.size(), &terms,
			    budget.reserve, budget.context, admission_prefix) ||
		    terms.action_operation.bytes != command.operation_id.bytes)
			return false;
		budget.terms = nullptr;
	}
	if (!budget.peak(payload_clone_frames))
		return false;
	*output = std::move(value);
	return true;
}

bool item_decode_native_money_owned(const critical_command &command, item_transfer_payload *output,
				    item_native_wire_decode_budget &budget)
{
	constexpr size_t fixed = ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES +
				 QUEST_MOBILE_NATIVE_REFERENCE_BYTES + NATIVE_QUEST_COIN_GIVE_BYTES;
	if (!output || !native_money_version(command.payload_version) ||
	    command.type != critical_command_type::item_transfer ||
	    command.payload.size() < fixed ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	const auto *wire = command.payload.data();
	const bool acknowledged = command.payload_version ==
				  ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION;
	if (wire[0] != 'N' || wire[1] != 'Q' || wire[2] != 'M' || wire[3] != '1' ||
	    get_u16(wire + 4) != 1 || get_u16(wire + 6) != command.payload_version ||
	    get_u32(wire + 8) != command.payload.size() || get_u32(wire + 20) > INT32_MAX ||
	    get_u64(wire + 56) || get_u32(wire + 12) != command.payload.size() - fixed ||
	    (!acknowledged && command.payload.size() != fixed))
		return false;
	if (!budget.peak(sizeof(item_transfer_payload) +
			 item_generic_decode_default_payload_frames))
		return false;
	item_transfer_payload candidate{};
	budget.payload = &candidate;
	size_t admission_prefix = 0;
	if (!budget.prefix(admission_prefix))
		return false;
	candidate.native_mobile.present = true;
	candidate.native_mobile.action = item_native_mobile_action::acceptance;
	candidate.native_mobile.final_giver_pid = get_u32(wire + 16);
	candidate.expected_from_revision = get_u64(wire + 24);
	candidate.expected_to_revision = get_u64(wire + 32);
	candidate.native_money.present = true;
	candidate.native_money.original_room_vnum = static_cast<int32_t>(get_u32(wire + 20));
	candidate.native_money.player_wallet_mapping_id = get_u64(wire + 40);
	candidate.native_money.mobile_wallet_mapping_id = get_u64(wire + 48);
	if (quest_mobile_native_reference_decode_bounded(
		    { wire + ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES,
		      QUEST_MOBILE_NATIVE_REFERENCE_BYTES },
		    &candidate.native_mobile.reference, budget.reserve, budget.context,
		    admission_prefix) != player_snapshot_codec_result::ok ||
	    (!budget.prefix(admission_prefix) ?
		     native_quest_coin_give_result::invalid :
		     native_quest_coin_give_decode_bounded(
			     { wire + ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES +
				       QUEST_MOBILE_NATIVE_REFERENCE_BYTES,
			       NATIVE_QUEST_COIN_GIVE_BYTES },
			     &candidate.native_money.projection, budget.reserve, budget.context,
			     admission_prefix)) != native_quest_coin_give_result::ok ||
	    (acknowledged && !(budget.prefix(admission_prefix) &&
			       item_transfer_native_recovery_decode_bounded(
				       std::span<const uint8_t>(command.payload).subspan(fixed),
				       &candidate.native_recovery, budget.reserve, budget.context,
				       admission_prefix, nullptr))))
		return false;
	candidate.from_owner = { item_owner_type::player, candidate.native_mobile.final_giver_pid,
				 0 };
	candidate.to_owner = { item_owner_type::native_mobile,
			       candidate.native_mobile.reference.mobile_instance_id, 0 };
	candidate.reason = item_transfer_reason::player_give;
	candidate.reason_id = candidate.native_mobile.reference.mobile_vnum;
	if (!budget.peak(sizeof(std::vector<uint8_t>) + sizeof(critical_command) +
			 5 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>))))
		return false;
	std::vector<uint8_t> canonical;
	critical_command expected{};
	budget.canonical = &canonical;
	budget.expected = &expected;
	if (!(budget.prefix(admission_prefix) &&
	      item_transfer_native_money_encode_bounded(candidate, acknowledged, &canonical,
							budget.reserve, budget.context,
							admission_prefix)) ||
	    (!budget.peak(item_native_validation_equal_frames) || canonical != command.payload) ||
	    !(budget.prefix(admission_prefix) &&
	      item_transfer_command_entities_bounded(&expected, candidate, budget.reserve,
						     budget.context, admission_prefix)) ||
	    command.keys.size() != expected.keys.size() ||
	    command.expected_revisions.size() != expected.expected_revisions.size() ||
	    !budget.peak(item_generic_decode_predicate_equal_frames +
			 sizeof(decltype(&critical_entity_key_equal))) ||
	    !std::equal(command.keys.begin(), command.keys.end(), expected.keys.begin(),
			critical_entity_key_equal) ||
	    !std::equal(command.expected_revisions.begin(), command.expected_revisions.end(),
			expected.expected_revisions.begin(),
			[](const auto &left, const auto &right) {
				return critical_entity_key_equal(left.key, right.key) &&
				       left.revision == right.revision;
			}))
		return false;
	if (!budget.peak(payload_clone_frames))
		return false;
	*output = std::move(candidate);
	return true;
}
} // namespace

bool item_transfer_native_fee_decode_bounded(const critical_command &command,
					     item_transfer_payload *payload,
					     bool (*reserve)(size_t, void *) noexcept,
					     void *context, size_t outer_live) noexcept
{
	if (!payload || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 10 * sizeof(void *) + 14 * sizeof(size_t) + 6 * sizeof(uint32_t) +
				  4 * sizeof(uint64_t) + 4 * sizeof(bool) +
				  3 * sizeof(std::span<const uint8_t>) +
				  2 * sizeof(item_owner_identity) + item_sidecar_pure_frames +
				  payload_clone_copy_frames + payload_clone_frames;
	item_native_wire_decode_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_decode_native_fee_owned(command, payload, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_native_money_decode_bounded(const critical_command &command,
					       item_transfer_payload *payload,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer_live) noexcept
{
	if (!payload || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = 10 * sizeof(void *) + 14 * sizeof(size_t) + 6 * sizeof(uint32_t) +
				  4 * sizeof(uint64_t) + 4 * sizeof(bool) +
				  3 * sizeof(std::span<const uint8_t>) +
				  2 * sizeof(item_owner_identity) + item_sidecar_pure_frames +
				  payload_clone_copy_frames + payload_clone_frames;
	item_native_wire_decode_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_decode_native_money_owned(command, payload, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
struct item_public_dispatch_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *original = nullptr;
	const item_transfer_payload *candidate = nullptr;
	const std::vector<uint8_t> *canonical = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observation =
			10 * sizeof(void *) + 7 * sizeof(size_t) + 4 * sizeof(bool) +
			5 * (sizeof(void *) + sizeof(size_t)) + payload_clone_observation_frames;
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    !payload_clone_add(total, observation) ||
		    (original &&
		     (!payload_clone_add(total, sizeof(*original)) ||
		      !payload_clone_vector_heap(original->keys, false, total) ||
		      !payload_clone_vector_heap(original->expected_revisions, false, total) ||
		      !payload_clone_vector_heap(original->payload, false, total) ||
		      !payload_clone_vector_heap(original->accounting_intent, false, total))) ||
		    (candidate && (!payload_clone_add(total, sizeof(*candidate)) ||
				   !payload_clone_heap(*candidate, false, total))) ||
		    (canonical && (!payload_clone_add(total, sizeof(*canonical)) ||
				   !payload_clone_vector_heap(*canonical, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	bool command_copy(const critical_command &source) const noexcept
	{
		// Actual generated COPY, all four fresh vector capacities equal SIZE.
		// No command bytes/canonical length is substituted for member storage.
		size_t request = sizeof(critical_command) +
				 4 * payload_clone_vector_constructor_frames + 5 * sizeof(void *) +
				 2 * sizeof(size_t) + sizeof(bool);
		return payload_clone_vector_heap(source.keys, true, request) &&
		       payload_clone_vector_heap(source.expected_revisions, true, request) &&
		       payload_clone_vector_heap(source.payload, true, request) &&
		       payload_clone_vector_heap(source.accounting_intent, true, request) &&
		       peak(request);
	}
};
bool item_public_encode_owned(const item_transfer_payload &payload, std::vector<uint8_t> *encoded,
			      item_public_dispatch_budget &budget)
{
	size_t admission_prefix = 0;
	if (!encoded)
		return false;
	try
	{
		if (!budget.peak(sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
				 sizeof(std::allocator<uint8_t>)))
			return false;
		std::vector<uint8_t> candidate;
		budget.canonical = &candidate;
		if (!(payload.native_money.present ?
			      (budget.prefix(admission_prefix) &&
			       item_transfer_native_money_encode_bounded(
				       payload, false, &candidate, budget.reserve, budget.context,
				       admission_prefix)) :
		      payload.native_cost.present ?
			      (budget.prefix(admission_prefix) &&
			       item_transfer_native_cost_encode_bounded(
				       payload, false, &candidate, budget.reserve, budget.context,
				       admission_prefix)) :
			      (budget.prefix(admission_prefix) &&
			       item_transfer_payload_encode_version_bounded(
				       payload, ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION,
				       &candidate, budget.reserve, budget.context, admission_prefix,
				       nullptr))))
			return false;
		if (!budget.peak(payload_clone_move_frames))
			return false;
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_public_encode_recovery_owned(const item_transfer_payload &payload,
				       std::vector<uint8_t> *encoded,
				       item_public_dispatch_budget &budget)
{
	size_t admission_prefix = 0;
	if (!encoded)
		return false;
	try
	{
		if (!budget.peak(sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
				 sizeof(std::allocator<uint8_t>)))
			return false;
		std::vector<uint8_t> candidate;
		budget.canonical = &candidate;
		if (!(payload.native_money.present ?
			      (budget.prefix(admission_prefix) &&
			       item_transfer_native_money_encode_bounded(
				       payload, true, &candidate, budget.reserve, budget.context,
				       admission_prefix)) :
		      payload.native_cost.present ?
			      (budget.prefix(admission_prefix) &&
			       item_transfer_native_cost_encode_bounded(
				       payload, true, &candidate, budget.reserve, budget.context,
				       admission_prefix)) :
			      (budget.prefix(admission_prefix) &&
			       item_transfer_payload_encode_version_bounded(
				       payload, ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION,
				       &candidate, budget.reserve, budget.context, admission_prefix,
				       nullptr))))
			return false;
		if (!budget.peak(payload_clone_move_frames))
			return false;
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_public_decode_owned(const critical_command &command, item_transfer_payload *payload,
			      item_public_dispatch_budget &budget)
{
	size_t admission_prefix = 0;
	if (native_money_version(command.payload_version))
	{
		try
		{
			return (budget.prefix(admission_prefix) &&
				item_transfer_native_money_decode_bounded(
					command, payload, budget.reserve, budget.context,
					admission_prefix));
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	if (native_cost_version(command.payload_version))
	{
		if (command.payload.size() >= 4 && command.payload[0] == 'N' &&
		    command.payload[1] == 'Q' && command.payload[2] == 'F' &&
		    command.payload[3] == '2')
			try
			{
				return (budget.prefix(admission_prefix) &&
					item_transfer_native_fee_decode_bounded(
						command, payload, budget.reserve, budget.context,
						admission_prefix));
			}
			catch (const std::bad_alloc &)
			{
				return false;
			}
		if (!payload || command.type != critical_command_type::item_transfer ||
		    command.payload.size() < ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES ||
		    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
			return false;
		try
		{
			const auto *wire = command.payload.data();
			const uint16_t inner_version =
				command.payload_version ==
						ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION ?
					ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION :
					ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION;
			if (wire[0] != 'N' || wire[1] != 'Q' || wire[2] != 'F' || wire[3] != '1' ||
			    get_u16(wire + 4) != 1 || get_u16(wire + 6) != inner_version ||
			    !get_u64(wire + 16))
				return false;
			const size_t body_size = get_u32(wire + 8), cost_size = get_u32(wire + 12);
			const size_t available = command.payload.size() -
						 ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES;
			if (body_size > available || cost_size != available - body_size)
				return false;
			if (!budget.command_copy(command))
				return false;
			auto original = command;
			budget.original = &original;
			original.payload_version = inner_version;
			if (!budget.peak(payload_clone_vector_frames))
				return false;
			original.payload.assign(
				command.payload.begin() +
					ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES,
				command.payload.begin() +
					ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES + body_size);
			if (!budget.peak(sizeof(item_transfer_payload) +
					 item_generic_decode_default_payload_frames))
				return false;
			item_transfer_payload candidate{};
			budget.candidate = &candidate;
			if (!(budget.prefix(admission_prefix) &&
			      item_transfer_command_decode_payload_bounded(
				      original, &candidate, budget.reserve, budget.context,
				      admission_prefix)))
				return false;
			candidate.native_cost.present = true;
			candidate.native_cost.wallet_mapping_id = get_u64(wire + 16);
			if ((!budget.prefix(admission_prefix) ?
				     native_quest_cost_projection_result::invalid :
				     native_quest_cost_projection_decode_bounded(
					     { wire + ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES +
						       body_size,
					       cost_size },
					     &candidate.native_cost.projection, budget.reserve,
					     budget.context, admission_prefix)) !=
				    native_quest_cost_projection_result::ok ||
			    !(budget.prefix(admission_prefix) &&
			      item_transfer_native_cost_value_valid_bounded(
				      candidate, budget.reserve, budget.context, admission_prefix)))
				return false;
			if (!budget.peak(sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
					 sizeof(std::allocator<uint8_t>)))
				return false;
			std::vector<uint8_t> canonical;
			budget.canonical = &canonical;
			if (!(budget.prefix(admission_prefix) &&
			      item_transfer_native_cost_encode_bounded(
				      candidate,
				      inner_version ==
					      ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION,
				      &canonical, budget.reserve, budget.context,
				      admission_prefix)) ||
			    (!budget.peak(item_native_validation_equal_frames) ||
			     canonical != command.payload))
				return false;
			if (!budget.peak(payload_clone_frames))
				return false;
			*payload = std::move(candidate);
			return true;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	if (!native_mobile_version(command.payload_version))
		return (budget.prefix(admission_prefix) &&
			item_transfer_payload_decode_generic_bounded(command, payload,
								     budget.reserve, budget.context,
								     admission_prefix, nullptr));
	if (!payload)
		return false;
	try
	{
		if (!budget.peak(sizeof(item_transfer_payload) +
				 item_generic_decode_default_payload_frames))
			return false;
		item_transfer_payload candidate = {};
		budget.candidate = &candidate;
		if (!(budget.prefix(admission_prefix) &&
		      item_transfer_payload_decode_generic_bounded(command, &candidate,
								   budget.reserve, budget.context,
								   admission_prefix, nullptr)))
			return false;
		if (!budget.peak(sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
				 sizeof(std::allocator<uint8_t>)))
			return false;
		std::vector<uint8_t> canonical;
		budget.canonical = &canonical;
		if (!(command.payload_version ==
				      ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION ?
			      (budget.prefix(admission_prefix) &&
			       item_transfer_command_encode_native_mobile_recovery_bounded(
				       candidate, &canonical, budget.reserve, budget.context,
				       admission_prefix)) :
			      (budget.prefix(admission_prefix) &&
			       item_transfer_command_encode_native_mobile_bounded(
				       candidate, &canonical, budget.reserve, budget.context,
				       admission_prefix))) ||
		    (!budget.peak(item_native_validation_equal_frames) ||
		     canonical != command.payload))
			return false;
		if (!budget.peak(payload_clone_frames))
			return false;
		*payload = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
} // namespace

bool item_transfer_command_encode_native_mobile_bounded(const item_transfer_payload &payload,
							std::vector<uint8_t> *encoded,
							bool (*reserve)(size_t, void *) noexcept,
							void *context, size_t outer_live) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		11 * sizeof(void *) + 10 * sizeof(size_t) + 3 * sizeof(uint16_t) +
		5 * sizeof(bool) + item_sidecar_pure_frames + payload_clone_copy_frames +
		payload_clone_vector_frames + payload_clone_frames +
		4 * payload_clone_vector_constructor_frames + 4 * payload_clone_move_frames;
	item_public_dispatch_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_public_encode_owned(payload, encoded, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_command_encode_native_mobile_recovery_bounded(
	const item_transfer_payload &payload, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		11 * sizeof(void *) + 10 * sizeof(size_t) + 3 * sizeof(uint16_t) +
		5 * sizeof(bool) + item_sidecar_pure_frames + payload_clone_copy_frames +
		payload_clone_vector_frames + payload_clone_frames +
		4 * payload_clone_vector_constructor_frames + 4 * payload_clone_move_frames;
	item_public_dispatch_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_public_encode_recovery_owned(payload, encoded, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_command_decode_payload_bounded(const critical_command &command,
						  item_transfer_payload *payload,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer_live) noexcept
{
	if (!payload || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		11 * sizeof(void *) + 10 * sizeof(size_t) + 3 * sizeof(uint16_t) +
		5 * sizeof(bool) + item_sidecar_pure_frames + payload_clone_copy_frames +
		payload_clone_vector_frames + payload_clone_frames +
		4 * payload_clone_vector_constructor_frames + 4 * payload_clone_move_frames;
	item_public_dispatch_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_public_decode_owned(command, payload, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
struct item_full_build_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const std::vector<uint8_t> *encoded = nullptr;
	const critical_command *working = nullptr, *candidate = nullptr;
	bool command_heap(const critical_command &value, size_t &total) const noexcept
	{
		return payload_clone_add(total, sizeof(value)) &&
		       payload_clone_vector_heap(value.keys, false, total) &&
		       payload_clone_vector_heap(value.expected_revisions, false, total) &&
		       payload_clone_vector_heap(value.payload, false, total) &&
		       payload_clone_vector_heap(value.accounting_intent, false, total);
	}
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observation = 10 * sizeof(void *) + 8 * sizeof(size_t) +
					       4 * sizeof(bool) +
					       4 * (sizeof(void *) + sizeof(size_t));
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    !payload_clone_add(total, observation) ||
		    (encoded && (!payload_clone_add(total, sizeof(*encoded)) ||
				 !payload_clone_vector_heap(*encoded, false, total))) ||
		    (working && !command_heap(*working, total)) ||
		    (candidate && !command_heap(*candidate, total)) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
constexpr size_t item_full_build_command_lifetime_frames =
	// Genuine four vector/base/impl/data default constructors and all four
	// vector move assignments/destruction. Generated command this/source
	// carriers and fixed operation-array copy/query/zero-ID range scopes.
	4 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>)) + 4 * payload_clone_move_frames +
	4 * payload_clone_allocator_frames + 5 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(bool) + sizeof(uint8_t);
bool item_full_build_ordinary_owned(critical_command *command, critical_operation_id operation_id,
				    const item_transfer_payload &payload,
				    critical_source_site source_site,
				    critical_deadline_class deadline_class,
				    item_full_build_budget &budget)
{
	if (!command || critical_operation_id_is_zero(operation_id))
		return false;
	if (!budget.peak(sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
			 sizeof(std::allocator<uint8_t>)))
		return false;
	std::vector<uint8_t> encoded;
	budget.encoded = &encoded;
	size_t admission_prefix = 0;
	if (!(budget.prefix(admission_prefix) &&
	      item_transfer_command_encode_payload_bounded(payload, &encoded, budget.reserve,
							   budget.context, admission_prefix)))
		return false;
	if (!budget.peak(sizeof(critical_command) + item_full_build_command_lifetime_frames))
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
	const bool populated =
		budget.prefix(admission_prefix) &&
		item_transfer_command_entities_bounded(command, payload, budget.reserve,
						       budget.context, admission_prefix);
	budget.encoded = nullptr;
	return populated;
}

bool item_full_build_native_owned(critical_command *command, critical_operation_id operation_id,
				  const item_transfer_payload &payload,
				  critical_source_site source_site,
				  critical_deadline_class deadline_class,
				  item_full_build_budget &budget)
{
	if (!command || critical_operation_id_is_zero(operation_id))
		return false;
	try
	{
		if (!budget.peak(sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
				 sizeof(std::allocator<uint8_t>)))
			return false;
		std::vector<uint8_t> encoded;
		budget.encoded = &encoded;
		size_t admission_prefix = 0;
		if (!(budget.prefix(admission_prefix) &&
		      item_transfer_command_encode_native_mobile_bounded(
			      payload, &encoded, budget.reserve, budget.context, admission_prefix)))
			return false;
		if (!budget.peak(sizeof(critical_command) +
				 item_full_build_command_lifetime_frames))
			return false;
		critical_command candidate = {
			.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
			.operation_id = operation_id,
			.type = critical_command_type::item_transfer,
			.payload_version =
				payload.native_money.present ?
					ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION :
				payload.native_cost.present ?
					ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION :
					ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION,
			.source_site = source_site,
			.deadline_class = deadline_class,
			.accepted_at_usec = 0,
			.keys = {},
			.expected_revisions = {},
			.payload = std::move(encoded)
		};
		budget.candidate = &candidate;
		if (!(budget.prefix(admission_prefix) &&
		      item_transfer_command_entities_bounded(&candidate, payload, budget.reserve,
							     budget.context, admission_prefix)))
			return false;
		if (!budget.peak(item_full_build_command_lifetime_frames))
			return false;
		*command = std::move(candidate);
		budget.encoded = nullptr;
		budget.candidate = nullptr;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_full_build_recovery_owned(critical_command *command, critical_operation_id operation_id,
				    const item_transfer_payload &payload,
				    critical_source_site source_site,
				    critical_deadline_class deadline_class,
				    item_full_build_budget &budget)
{
	if (!command || critical_operation_id_is_zero(operation_id))
		return false;
	try
	{
		if (!budget.peak(sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
				 sizeof(std::allocator<uint8_t>)))
			return false;
		std::vector<uint8_t> encoded;
		budget.encoded = &encoded;
		size_t admission_prefix = 0;
		if (!(budget.prefix(admission_prefix) &&
		      item_transfer_command_encode_native_mobile_recovery_bounded(
			      payload, &encoded, budget.reserve, budget.context, admission_prefix)))
			return false;
		if (!budget.peak(sizeof(critical_command) +
				 item_full_build_command_lifetime_frames))
			return false;
		critical_command candidate = {
			.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
			.operation_id = operation_id,
			.type = critical_command_type::item_transfer,
			.payload_version =
				payload.native_money.present ?
					ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION :
				payload.native_cost.present ?
					ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION :
					ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION,
			.source_site = source_site,
			.deadline_class = deadline_class,
			.accepted_at_usec = 0,
			.keys = {},
			.expected_revisions = {},
			.payload = std::move(encoded)
		};
		budget.candidate = &candidate;
		if (!(budget.prefix(admission_prefix) &&
		      item_transfer_command_entities_bounded(&candidate, payload, budget.reserve,
							     budget.context, admission_prefix)))
			return false;
		if (!budget.peak(item_full_build_command_lifetime_frames))
			return false;
		*command = std::move(candidate);
		budget.encoded = nullptr;
		budget.candidate = nullptr;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
} // namespace

bool item_transfer_command_build_bounded(critical_command *command,
					 critical_operation_id operation_id,
					 const item_transfer_payload &payload,
					 critical_source_site source_site,
					 critical_deadline_class deadline_class,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer_live) noexcept
{
	if (!command || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		9 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(bool) +
		2 * sizeof(critical_operation_id) + 2 * sizeof(critical_source_site) +
		2 * sizeof(critical_deadline_class) + item_full_build_command_lifetime_frames;
	item_full_build_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(critical_command) + item_full_build_command_lifetime_frames))
		return false;
	try
	{
		critical_command working = {};
		budget.working = &working;
		if (!item_full_build_ordinary_owned(&working, operation_id, payload, source_site,
						    deadline_class, budget) ||
		    !budget.peak(item_full_build_command_lifetime_frames))
			return false;
		static_assert(std::is_nothrow_move_assignable_v<critical_command>);
		*command = std::move(working);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_command_build_native_mobile_bounded(critical_command *command,
						       critical_operation_id operation_id,
						       const item_transfer_payload &payload,
						       critical_source_site source_site,
						       critical_deadline_class deadline_class,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer_live) noexcept
{
	if (!command || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		9 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(bool) +
		2 * sizeof(critical_operation_id) + 2 * sizeof(critical_source_site) +
		2 * sizeof(critical_deadline_class) + item_full_build_command_lifetime_frames;
	item_full_build_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(critical_command) + item_full_build_command_lifetime_frames))
		return false;
	try
	{
		critical_command working = {};
		budget.working = &working;
		if (!item_full_build_native_owned(&working, operation_id, payload, source_site,
						  deadline_class, budget) ||
		    !budget.peak(item_full_build_command_lifetime_frames))
			return false;
		static_assert(std::is_nothrow_move_assignable_v<critical_command>);
		*command = std::move(working);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool item_transfer_command_build_native_mobile_recovery_bounded(
	critical_command *command, critical_operation_id operation_id,
	const item_transfer_payload &payload, critical_source_site source_site,
	critical_deadline_class deadline_class, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!command || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		9 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(bool) +
		2 * sizeof(critical_operation_id) + 2 * sizeof(critical_source_site) +
		2 * sizeof(critical_deadline_class) + item_full_build_command_lifetime_frames;
	item_full_build_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(critical_command) + item_full_build_command_lifetime_frames))
		return false;
	try
	{
		critical_command working = {};
		budget.working = &working;
		if (!item_full_build_recovery_owned(&working, operation_id, payload, source_site,
						    deadline_class, budget) ||
		    !budget.peak(item_full_build_command_lifetime_frames))
			return false;
		static_assert(std::is_nothrow_move_assignable_v<critical_command>);
		*command = std::move(working);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

// Complete original native recovery-shape dispatch. The payload and all caller
// owners remain in outer; only the original cost branch's private exact wire
// vector and genuine constructor/destructor/parameter scopes are callee-owned.
bool item_transfer_native_mobile_recovery_shape_valid_bounded(
	const item_transfer_payload &payload, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		// public payload/reserve/context, outer, actual prefix/result/catch ref;
		// vector exact plus vector/base/impl/data/allocator default constructors;
		// its real destructor/_Destroy/deallocation tail is already mapped.
		5 * sizeof(void *) + 2 * sizeof(size_t) + 3 * sizeof(bool) +
		sizeof(std::vector<uint8_t>) + 4 * sizeof(void *) +
		sizeof(std::allocator<uint8_t>) + payload_clone_vector_frames;
	size_t prefix = outer_live;
	if (!payload_clone_add(prefix, frames) || !reserve(prefix, context))
		return false;
	try
	{
		if (payload.native_money.present)
			return item_transfer_native_money_shape_valid_bounded(
				payload, true, reserve, context, prefix);
		if (payload.native_cost.present)
		{
			std::vector<uint8_t> exact;
			return item_transfer_native_cost_encode_bounded(payload, true, &exact,
									reserve, context, prefix);
		}
		return item_transfer_payload_valid_bounded(
			payload, ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION, reserve,
			context, prefix);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

size_t item_transfer_payload_current_heap_observer_frame_bytes() noexcept
{
	// Exact current getter/shared six-string/six-vector observation source;
	// no aggregate copies, codec scratch or allocation requests are borrowed.
	// capacity -> _M_is_local -> const _M_data/_M_local_data -> pointer_to ->
	// addressof/__addressof has 11P+B beneath the already inventoried capacity
	// this/result. CURRENT public params/value/policy/strong-result and this
	// numeric profile's returned size_t are explicit additional source scopes.
	return payload_clone_observation_frames + 11 * sizeof(void *) + sizeof(bool) +
	       2 * sizeof(void *) + 2 * sizeof(size_t) + 3 * sizeof(bool);
}

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) && \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
// Actual first item_owner_key_bounded fixed allowance, not a full item codec.
// 4P+N+B formals/result; base N; actual candidate and braced key; identity-valid
// owner ref/result; type conversion parameter/result; actual add/reserve graph.
constexpr size_t item_owner_source_fixed_owned =
	4 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(size_t) +
	2 * sizeof(critical_entity_key) + sizeof(void *) + sizeof(bool) + sizeof(item_owner_type) +
	sizeof(critical_entity_type) + 4 * sizeof(size_t) + 3 * sizeof(void *) + 2 * sizeof(bool);
// The exact actual original hash leaf: encode_owner and put_u64; get_u64's
// input/value/byte/result; four array.data(this,pointer-result) call sites and
// encoded.size(this,size-result). Actual GNU13 array data is a direct _M_elems
// return; the old comment's _S_ptr label does not imply another function call.
constexpr size_t item_owner_source_leaf_owned =
	2 * sizeof(void *) + sizeof(void *) + sizeof(uint64_t) + sizeof(unsigned int) +
	sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(unsigned int) + 4 * (2 * sizeof(void *)) +
	sizeof(void *) + sizeof(size_t);
// The original fixed group's 4N distributes to parameters/fixed_frames constexpr
// objects and genuine add/reserve scalar arguments; its 3P to add's bytes-ref,
// reserve's context, and one generated key-assignment reference. That assignment
// actually owns this+source references and returned reference, so two further P
// are required even on the
// direct owner-id branch. Hash branch's automatic constexpr leaf_frames adds N.
constexpr size_t item_owner_source_supplement = 2 * sizeof(void *) + sizeof(size_t);

// Same authenticated fixed SHA phase law as the currency account leaf. Init
// memset, Update memcpy/memset and Final memset/cleanse are sequential with block
// calls. OPENSSL_cleanse's ptr/len + volatile loaded target + memset are explicit.
constexpr size_t item_owner_sha_memory_copy = 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_owner_sha_memory_set = 2 * sizeof(void *) + sizeof(int) + sizeof(size_t);
constexpr size_t item_owner_sha_cleanse =
	sizeof(void *) + sizeof(size_t) + sizeof(void *) + item_owner_sha_memory_set;
constexpr size_t item_owner_sha_block_source =
	std::max(item_key_sha_assembly_frames,
		 std::max(item_key_sha_c_small_frames, item_key_sha_c_normal_frames));
static_assert(item_owner_sha_block_source >= item_owner_sha_memory_copy);
static_assert(item_owner_sha_block_source >= item_owner_sha_memory_set);
static_assert(item_owner_sha_block_source >= item_owner_sha_cleanse);
// Hash workspace default/member initialization and cleanup are sequential before
// the first SHA call/after the last SHA call. Its generated constructor/destructor
// receivers and the two trivial fixed-array destructor receivers are this pointers;
// array brace initialization has no separate allocating constructor graph. This
// real hash-phase alternative is dominated by its already-admitted fixed SHA graph.
constexpr size_t item_owner_hash_lifetime_source = 4 * sizeof(void *);
static_assert(item_key_sha_frames >= item_owner_hash_lifetime_source);

// Whole named SOURCE alternative union. Initial candidate is a separate preentry
// inline object; old actual workspace is created only after the second original
// reserve and remains child-owned. No workspace/encoded-body heap is added here.
constexpr size_t item_owner_source_full =
	item_owner_source_fixed_owned - sizeof(critical_entity_key) + item_owner_source_leaf_owned +
	item_key_sha_frames + item_owner_source_supplement;
#endif
}

bool item_owner_key_source_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) && \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return false;
	*output = item_owner_source_full;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool item_owner_key_source_supplement_frame_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) && \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return false;
	*output = item_owner_source_supplement;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool item_owner_key_initial_inline_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) && \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return false;
	*output = sizeof(critical_entity_key);
	return true;
#else
	(void)output;
	return false;
#endif
}

namespace
{
struct item_native_validation_complete_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const std::vector<player_item_snapshot> *items = nullptr;
	const std::vector<uint8_t> *canonical = nullptr;
	const std::vector<uint64_t> *roots = nullptr, *uids = nullptr;
	const quest_reward_continuation *terms = nullptr;
	const std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH> *path = nullptr;
	bool prefix(size_t &output, size_t extra = 0) const noexcept
	{
		// Real scan/add/multiply/current-row observer source frames; the shared
		// row provider reports capacity storage, not item-encoded size.

		size_t total = outer, row = 0;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames))
			return false;
		if (items)
		{
			if (!payload_clone_add(total, sizeof(*items)) ||
			    !payload_clone_vector_heap(*items, false, total))
				return false;
			for (const auto &item : *items)
				if (!player_item_snapshot_current_heap_bytes(item, &row) ||
				    !payload_clone_add(total, row))
					return false;
		}
		if ((canonical && (!payload_clone_add(total, sizeof(*canonical)) ||
				   !payload_clone_vector_heap(*canonical, false, total))) ||
		    (roots && (!payload_clone_add(total, sizeof(*roots)) ||
			       !payload_clone_vector_heap(*roots, false, total))) ||
		    (uids && (!payload_clone_add(total, sizeof(*uids)) ||
			      !payload_clone_vector_heap(*uids, false, total))) ||
		    (terms && (!payload_clone_add(total, sizeof(*terms)) ||
			       !payload_clone_string_heap(terms->character_name, false, total) ||
			       !payload_clone_string_heap(terms->definition_id, false, total))) ||
		    (path && !payload_clone_add(total, sizeof(*path))) ||
		    !payload_clone_add(total, extra))
			return false;
		output = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
struct item_complete_payload_uid_less
{
	bool operator()(const item_transfer_entry &entry, uint64_t uid) const noexcept
	{
		return entry.item_uid < uid;
	}
};

constexpr size_t item_complete_offering_terms_default_string_local_source = 4 * 2 * sizeof(void *);
constexpr size_t item_complete_offering_terms_default_string_init_source =
	sizeof(void *) + sizeof(size_t) + sizeof(char) + sizeof(bool);
constexpr size_t item_complete_offering_terms_default_string_set_length_source =
	2 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(char) + sizeof(bool);
constexpr size_t item_complete_offering_terms_default_string_source =
	// String receiver; hider receiver/data/default-allocator reference; genuine
	// temporary allocator; allocator/new_allocator default and const-copy;
	// move reference/result; real temporary allocator cleanup receiver.
	// The new_allocator destructor declaration is PP-excluded in C++20.
	sizeof(void *) + 3 * sizeof(void *) + sizeof(std::allocator<char>) + 2 * sizeof(void *) +
	4 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	item_complete_offering_terms_default_string_local_source +
	item_complete_offering_terms_default_string_init_source +
	item_complete_offering_terms_default_string_set_length_source;
constexpr size_t item_complete_offering_terms_cleanup_string_source =
	// String/dispose/is-local/destroy receivers, destroy size and locality bool.
	4 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// Actual const data and local_data/pointer_to/addressof/__addressof.
	2 * sizeof(void *) + 4 * 2 * sizeof(void *) +
	// _M_get_allocator receiver/reference result. String destroy directly
	// calls traits deallocate: it does not call vector _M_deallocate.
	2 * sizeof(void *) + 3 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) +
	sizeof(size_t) + sizeof(bool) +
	// Actual hider/allocator destruction receivers. new_allocator destructor
	// is PP-excluded in selected C++20; allocator predicate has one result.
	2 * sizeof(void *);

using item_complete_offering_L = item_complete_payload_uid_less;
using item_complete_offering_A =
	decltype(__gnu_cxx::__ops::__iter_comp_val(item_complete_offering_L{}));
constexpr size_t item_complete_offering_lower_bound_source =
	// Actual std::lower_bound first/last/value/return + by-value comparator.
	4 * sizeof(void *) + sizeof(item_complete_offering_L) +
	// __lower_bound first/last/value/middle/return, len/half and adapter.
	5 * sizeof(void *) + 2 * sizeof(std::ptrdiff_t) + sizeof(item_complete_offering_A) +
	// __iter_comp_val comparator; returned adapter is downstream parameter.
	sizeof(item_complete_offering_L) +
	// Explicit adapter constructor this/comparator, two distinct std::move<L>
	// occurrences; operator iterator/value/this + bool, named comparator.
	sizeof(void *) + sizeof(item_complete_offering_L) + 4 * sizeof(void *) +
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(uint64_t) + sizeof(bool) +
	// Raw-pointer distance/category/RA distance and raw advance/category/RA.
	2 * sizeof(void *) + sizeof(std::ptrdiff_t) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(void *) +
	2 * sizeof(std::ptrdiff_t) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *);
constexpr size_t item_complete_offering_find_source =
	// Exact helper payload/found/return + UID. Three actual const array begin
	// call occurrences each own this/return and nested data this/return.
	3 * sizeof(void *) + sizeof(uint64_t) + 3 * (4 * sizeof(void *)) +
	item_complete_offering_lower_bound_source;
constexpr size_t item_complete_offering_direct_access_source =
	// data(): receiver/pointer result and typed _M_data_ptr own3P; size().
	5 * sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	// Distinct const entry and mutable continuation-root array subscripts.
	2 * (2 * sizeof(void *) + sizeof(size_t));
constexpr size_t item_complete_offering_terms_lifetime_source =
	// Plain enclosing nontrivial default and destructor families. Arrays{}
	// and scalar aggregates in DMIs do not manufacture callable operations.
	2 * sizeof(void *) + item_complete_offering_terms_default_string_source +
	item_complete_offering_terms_cleanup_string_source;
constexpr size_t item_complete_offering_terms_current_source =
	// Unchanged payload_clone_string_heap(value/fresh/bytes/count/return),
	// actual false branch capacity plus const islocal/data/local pointer chain.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	11 * sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_offering_controller_selected_source =
	// Actual terms-only prefix: this/output2P, extra/total/row3N and bool.
	// Other observer pointers remain null throughout this endpoint; no range.
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	// Peak this/extra/total/result; checked-add and reserve declared interface.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	sizeof(bool) + sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	item_complete_offering_terms_current_source;
constexpr size_t item_complete_offering_continuation_adapter_source =
	// Adapter own exact three object pointers,F and eight scalar locals/formals.
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 8 * sizeof(size_t) +
	sizeof(bool) +
	// Full getter real50 Source; two simpler initial/supp getters P+B each (9 LP64).
	quest_reward_continuation_decode_source_query_frame_bytes() +
	2 * (sizeof(void *) + sizeof(bool));
constexpr size_t item_complete_offering_retained_source =
	// Wrapper3P+F2N+B; owned4P4N+uid+B. Policy result is genuine runtime.
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 2 * sizeof(size_t) +
	sizeof(bool) + 4 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(uint64_t) + sizeof(bool) +
	sizeof(bool) + item_complete_offering_find_source +
	item_complete_offering_direct_access_source + item_complete_offering_terms_lifetime_source +
	item_complete_offering_controller_selected_source +
	item_complete_offering_continuation_adapter_source;

bool item_complete_generic_reward_continuation_decode_child(
	const uint8_t *data, size_t size, quest_reward_continuation *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	constexpr size_t query = 3 * quest_reward_continuation_decode_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !quest_reward_continuation_decode_source_frame_bytes(&source) ||
	    !quest_reward_continuation_decode_initial_inline_bytes(&initial) ||
	    !quest_reward_continuation_decode_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	// The selecting parent retains its output terms and both string heaps.
	// Full Source and INITIAL are transient; only the old child's uncovered
	// Source remains in outer while its original owner admits its own fields.
	return quest_reward_continuation_decode_bounded(data, size, output, reserve, context,
							child_outer);
}

const item_transfer_entry *item_complete_find_payload_item(const item_transfer_payload &payload,
							   uint64_t item_uid)
{
	auto found = std::lower_bound(payload.items.begin(),
				      payload.items.begin() + payload.item_count, item_uid,
				      item_complete_payload_uid_less{});
	return found != payload.items.begin() + payload.item_count && found->item_uid == item_uid ?
		       &*found :
		       nullptr;
}
bool item_continuation_quest_valid_complete_owned(const item_transfer_payload &payload,
						  item_native_validation_complete_budget &owner)
{
	const std::vector<uint8_t> &data = payload.continuation.data;
	if (!owner.peak(sizeof(quest_reward_continuation)))
		return false;
	quest_reward_continuation continuation;
	owner.terms = &continuation;
	size_t prefix = 0;
	if (!owner.prefix(prefix))
		return false;
	if (!item_complete_generic_reward_continuation_decode_child(data.data(), data.size(),
								    &continuation, owner.reserve,
								    owner.context, prefix) ||
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
		const item_transfer_entry *root = item_complete_find_payload_item(payload, uid);
		if (!root || root->parent_item_uid || root->root_item_uid != uid)
			return false;
	}
	return true;
}
} // namespace
bool item_transfer_quest_offering_continuation_valid_complete_bounded(
	const item_transfer_payload &payload, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_offering_retained_source;
	item_native_validation_complete_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak())
		return false;
	try
	{
		return item_continuation_quest_valid_complete_owned(payload, owner);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_quest_offering_continuation_valid_complete_source_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_offering_retained_source;
	size_t child = 0;
	if (!quest_reward_continuation_decode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*bytes = result;
	return true;
}
bool item_transfer_quest_offering_continuation_valid_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_quest_offering_continuation_valid_complete_initial_inline_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_native_validation_complete_budget);
	return true;
}

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
template <class T> constexpr size_t item_complete_native_vector_deallocate_source =
	// _M_deallocate, allocator_traits::deallocate, allocator::deallocate,
	// new_allocator::deallocate each owns this/alloc, pointer, count.
	4 * (2 * sizeof(void *) + sizeof(size_t)) +
	// Actual sized operator delete pointer/count. Const-evaluation alternate
	// unsized delete is not selected; single helper bool is actual query.
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
template <class T> constexpr size_t item_complete_native_vector_cleanup_source =
	// Actual vector, Vector_base, generated nontrivial Vector_impl and
	// allocator destructor receivers. Impl_data has no declared destructor;
	// new_allocator destructor is excluded by the C++20 preprocessor path.
	// Neither contributes a runtime selected destructor scope.
	4 * sizeof(void *) +
	// _Destroy(first,last,allocator), _Destroy(first,last) and selected aux:
	// no invented cursor local: aux<false> increments its first parameter.
	3 * sizeof(void *) + 2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
	// Mutable get_Tp_allocator receiver/reference result.
	2 * sizeof(void *) + item_complete_native_vector_deallocate_source<T> +
	(std::is_trivially_destructible_v<T> ?
		 0 :
		 // Nontrivial aux reaches __addressof, _Destroy(pointer), destroy_at
		 // and the genuine T destructor receiver. Member cleanup is separate;
		 // raw-pointer ++ and == use no iterator object/helper family.
		 2 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(void *));
constexpr size_t item_complete_native_member_string_cleanup_source =
	// String/dispose/is-local/destroy receivers, destroy size and locality bool.
	4 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// Actual const data and local_data/pointer_to/addressof/__addressof.
	2 * sizeof(void *) + 4 * 2 * sizeof(void *) +
	// _M_get_allocator receiver/reference result. String destroy directly
	// calls traits deallocate: it does not call vector _M_deallocate.
	2 * sizeof(void *) + 3 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) +
	sizeof(size_t) + sizeof(bool) +
	// Actual hider/allocator destruction receivers. new_allocator destructor
	// is PP-excluded in selected C++20; allocator predicate has one result.
	2 * sizeof(void *);

namespace
{
constexpr size_t item_complete_native_validation_empty_vectors_source =
	// Parent constructs only empty row/byte/u64 vectors, not any row/ED/string.
	// roots(count) selected constructor is a separate authentic subtree.
	3 * (6 * sizeof(void *));
constexpr size_t item_complete_native_validation_cleanup_source =
	// Outer Item vector destruction reaches generated nontrivial row dtorP
	// in its selected nontrivial cleanup backend; ED dtorP is similarly inside
	// its typed vector cleanup. No new row COPY/move/default or ED COPY lane.
	item_complete_native_vector_cleanup_source<player_item_snapshot> +
	item_complete_native_vector_cleanup_source<uint8_t> +
	item_complete_native_vector_cleanup_source<uint64_t> +
	// Genuine row and nested ED member vectors; DA/i32 are trivial elements.
	item_complete_native_vector_cleanup_source<player_item_dynamic_affect_snapshot> +
	item_complete_native_vector_cleanup_source<player_item_extra_description_snapshot> +
	item_complete_native_vector_cleanup_source<int32_t> +
	// Same declared char-string cleanup lane shared after return across real
	// row/ED fields; every physical string and nested capacity stays external.
	item_complete_native_member_string_cleanup_source;
constexpr size_t item_complete_native_validation_current_row_source =
	// Exact accepted selected CURRENT-only projection, no unused FRESH root
	// or size query. Public current + heap/request/string/ED-range scopes.
	44 * sizeof(void *) + 10 * sizeof(size_t) + 15 * sizeof(bool);
constexpr size_t item_complete_native_validation_current_vector_source =
	// Three distinct parent vector<T> false heap observers: row/byte/u64;
	// roots/uids share the completed same-type scope, physical heaps separate.
	3 * (3 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool));
constexpr size_t item_complete_native_validation_current_range_source =
	// const outer Item begin/end receivers2P; explicit same CI pointer ctor2P;
	// rewritten != through == refs2P+B plus both actual base() receivers and
	// returned references4P, ++2P, dereference2P. CI result objects belong to
	// controller hidden-range own storage separately; no implicit copy callee.
	14 * sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_native_validation_current_controller_source =
	// New prefix this/out+hiddenrange/elementrefs4P; extra/total/row3N;
	// hidden begin/end CIs and returned B. No removed observation local.
	4 * sizeof(void *) + 3 * sizeof(size_t) +
	2 * sizeof(std::vector<player_item_snapshot>::const_iterator) + sizeof(bool) +
	// Actual peak this/extra/total/B, checked add, selected callback interface.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	sizeof(bool) + sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	item_complete_native_validation_current_row_source +
	item_complete_native_validation_current_vector_source +
	item_complete_native_validation_current_range_source;
} // namespace

namespace
{
using item_complete_native_equal_I = std::vector<uint8_t>::iterator;
constexpr size_t item_complete_native_canonical_equal_source =
	// Three-iterator equal(I,I,constbyte*) and equal_aux's genuine copied
	// by-value I parameters. Implicit trivial I copies have no callable body.
	2 * (2 * sizeof(item_complete_native_equal_I) + sizeof(void *) + sizeof(bool)) +
	// Two actual niter_base(I) occurrences: by-value I, raw pointer return,
	// base() receiver/reference result. Pointers return by scalar value.
	2 * (sizeof(item_complete_native_equal_I) + 3 * sizeof(void *)) +
	// Raw constbyte* niter_base parameter/result, typed separately.
	2 * sizeof(void *) +
	// equal_aux1 raw first/last/first2, declared __simple and bool return.
	3 * sizeof(void *) + 2 * sizeof(bool) +
	// equal<true> takes three const byte pointers, named size_t __len and bool.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// Genuine std::__memcmp(first1,first2,num)->int, actual public
	// std::is_constant_evaluated resultB. Bare builtin has no Source callee.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(bool) +
	// Caller mutable begin/end each has receiverP and real pointer ctor2P.
	// Their returned I objects ARE the equal() argument destinations above;
	// no second I storage or undeclared implicit move constructor is added.
	2 * (3 * sizeof(void *)) +
	// Actual payload.item_blob const array begin/data this/returned-pointer
	// scopes. This raw scalar pointer has no guaranteed class result alias.
	4 * sizeof(void *);
} // namespace

namespace
{
constexpr size_t item_complete_native_uid_fitting_const_push_source =
	// vector<uint64_t>::push_back(const value_type&): this/x2P.
	2 * sizeof(void *) +
	// allocator_traits<allocator<u64>>::construct allocator/location/arg3P.
	3 * sizeof(void *) +
	// construct_at<u64,constu64&> location/arg/returned pointer3P.
	3 * sizeof(void *) +
	// Two actual std::forward<constu64&> occurrences; each arg/returnedref2P.
	4 * sizeof(void *) +
	// Declared global placement new(size,place): sizeN/placeP/resultP.
	2 * sizeof(void *) + sizeof(size_t);
} // namespace

namespace
{
constexpr size_t item_complete_native_mobile_local_predicates_source =
	// native_mobile_version u16 parameter/B return; identity constref/B return.
	sizeof(uint16_t) + sizeof(bool) + sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_native_mobile_array_access_source =
	// const array<entry,MAX> operator[]; const byte-array size and data.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) +
	// mutable array<int32_t,DEPTH> operator[] and its const size method.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_native_mobile_vector_access_source =
	// Mutable Item/u64 operator[]: this/ref-result2P+indexN, separate types.
	2 * (2 * sizeof(void *) + sizeof(size_t)) +
	// Genuine parent row/byte/u64 size receivers and returned scalar N.
	// These direct caller invocations are separately retained from lower
	// constructor/reserve/provider queries; no unrelated profile subtraction.
	3 * (sizeof(void *) + sizeof(size_t));
constexpr size_t item_complete_native_mobile_find_source =
	// Actual published named same helper/type scopes. No duplicate definition
	// or relabelled old partial lower-bound margin in this successor.
	item_complete_offering_find_source;
} // namespace

namespace
{
constexpr size_t item_complete_native_uid_iterator_acquisition_source =
	// Mutable vector<u64> begin/end each receiverP, same explicit pointer-CI
	// constructor family this/reference2P reused only after completed call.
	4 * sizeof(void *);
constexpr size_t item_complete_native_uid_adjacent_caller_source =
	// Adjacent returned iterator and separate uids.end() comparison operand
	// are two real caller objects, not counted in algorithm parameter rows.
	2 * sizeof(std::vector<uint64_t>::iterator) +
	// C++20 rewritten != uses == refs2P+B and both base methods4P.
	6 * sizeof(void *) + sizeof(bool);
} // namespace

namespace
{
using item_complete_native_uid_iterator = std::vector<uint64_t>::iterator;
using item_complete_native_uid_iter_less = __gnu_cxx::__ops::_Iter_less_iter;
using item_complete_native_uid_val_less = __gnu_cxx::__ops::_Val_less_iter;
using item_complete_native_uid_iter_val_less = __gnu_cxx::__ops::_Iter_less_val;
using item_complete_native_uid_iter_equal = __gnu_cxx::__ops::_Iter_equal_to_iter;
constexpr size_t item_complete_native_uid_sort_recursive_source =
	// Actual introsort first/last/cut I objects, depth D and comparator C.
	3 * sizeof(item_complete_native_uid_iterator) + sizeof(std::ptrdiff_t) +
	sizeof(item_complete_native_uid_iter_less);
constexpr size_t item_complete_native_uid_sort_nonrecursive_source =
	// Explicit normal-iterator own/operator occurrence and child table;
	// no raw-pointer substitution, legacy iterator< or implicit trivial call.
	68 * sizeof(item_complete_native_uid_iterator) + 314 * sizeof(void *) +
	40 * sizeof(std::ptrdiff_t) + 12 * sizeof(item_complete_native_uid_iter_less) +
	sizeof(item_complete_native_uid_val_less) + sizeof(item_complete_native_uid_iter_val_less) +
	7 * sizeof(uint64_t) + 2 * sizeof(unsigned long) + 8 * sizeof(int) + 10 * sizeof(bool) +
	2 * sizeof(std::strong_ordering) + 2 * sizeof(std::__cmp_cat::__unspec);
constexpr size_t item_complete_native_uid_adjacent_algorithm_source =
	// Public/core actual byvalue iterator and predicate objects. Real returned
	// class result is owned once by caller comparison, not another core local.
	7 * sizeof(item_complete_native_uid_iterator) + 19 * sizeof(void *) +
	sizeof(item_complete_native_uid_iter_equal) + 3 * sizeof(bool);
constexpr size_t item_complete_native_uid_sort_max_recursive_calls = []
{
	size_t levels = 0, remaining = ITEM_TRANSFER_MAX_ITEMS;
	while (remaining > 1)
	{
		remaining >>= 1;
		++levels;
	}
	return 2 * levels + 1;
}();
} // namespace

namespace
{
constexpr size_t item_complete_native_roots_default_n_source =
	// Selected u64 scalar default_n: no byte-specific __len, predicate or
	// intrinsic. Actual first allocated u64 is the scalar fill source.
	26 * sizeof(void *) + 8 * sizeof(size_t) + 3 * sizeof(bool) +
	sizeof(std::random_access_iterator_tag) + sizeof(uint64_t);
constexpr size_t item_complete_native_roots_count_failure_cleanup_source =
	// Actual generated nontrivial Vector_impl and allocator destructor,
	// selected allocation-failure member unwind (named shared family only).
	2 * sizeof(void *);
constexpr size_t item_complete_native_roots_count_constructor_source =
	// Public vector(n, allocator default), actual allocator object A.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<uint64_t>) +
	// Default allocator/newallocator ctors and actual allocator dtor.
	3 * sizeof(void *) +
	// check_init count/ref/return, real converted allocator object A.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(std::allocator<uint64_t>) +
	// Actual const allocator/newallocator copy constructors and cleanup.
	5 * sizeof(void *) +
	// Smax own+direct C++20 allocator_traits max+actual min<size_t>.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// Base/impl/allocator copy/data-default/create_storage genuine scopes.
	10 * sizeof(void *) + 2 * sizeof(size_t) +
	// allocate typed helper + standard allocator/newallocator/max/new.
	11 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(bool) +
	// default_initialize this/count and mutable allocator getter.
	sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) +
	item_complete_native_roots_default_n_source +
	item_complete_native_roots_count_failure_cleanup_source;
} // namespace

namespace
{
constexpr size_t item_complete_native_uid_fresh_reserve_source =
	duris_craft_recipe_bounded_detail::complete_recipe_vector_reserve_own_source<uint64_t> +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_max_size_source<uint64_t> +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_capacity_source<uint64_t> +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_size_source<uint64_t> +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_allocate_source<uint64_t> +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_mutable_allocator_source<uint64_t> +
	// Actual fresh size/capacity0: three relocation helpers and raw niter
	// calls have zero count and no reached count>0 predicate or element move.
	21 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	// Null old storage reaches _M_deallocate only, no lower delete chain.
	2 * sizeof(void *) + sizeof(size_t);
} // namespace

namespace
{
constexpr size_t item_complete_snapshot_list_decode_child_own_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 8 * sizeof(size_t) +
	sizeof(player_snapshot_codec_result);
constexpr size_t item_complete_snapshot_list_encode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 6 * sizeof(size_t) +
	sizeof(player_snapshot_codec_result);
} // namespace

namespace
{
player_snapshot_codec_result
item_complete_snapshot_list_decode_child(const uint8_t *data, size_t size,
					 std::vector<player_item_snapshot> *output,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer, size_t *retained_item_heap_bytes) noexcept
{
	constexpr size_t query =
		3 * player_item_snapshot_list_decode_bounded_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !player_item_snapshot_list_decode_bounded_source_frame_bytes(&source) ||
	    !player_item_snapshot_list_decode_bounded_initial_inline_bytes(&initial) ||
	    !player_item_snapshot_list_decode_bounded_source_supplement_frame_bytes(&supplement))
		return player_snapshot_codec_result::limit_exceeded;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return player_snapshot_codec_result::limit_exceeded;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return player_snapshot_codec_result::limit_exceeded;
	return player_item_snapshot_list_decode_bounded(data, size, output, reserve, context,
							child_outer, retained_item_heap_bytes);
}

player_snapshot_codec_result item_complete_snapshot_list_encode_child(
	const std::vector<player_item_snapshot> &input, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	constexpr size_t query = 2 * player_item_snapshot_list_encode_source_query_frame_bytes();
	size_t source = 0, initial = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !player_item_snapshot_list_encode_source_frame_bytes(&source) ||
	    !player_item_snapshot_list_encode_initial_inline_bytes(&initial))
		return player_snapshot_codec_result::limit_exceeded;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return player_snapshot_codec_result::limit_exceeded;
	size_t child_outer = outer;
	// This published encoder's complete Source is genuinely caller-retained,
	// not self-owned. INITIAL alone is transient. No invented supplement API.
	if (!payload_clone_add(child_outer, source))
		return player_snapshot_codec_result::limit_exceeded;
	return player_item_snapshot_list_encode_bounded(input, output, reserve, context,
							child_outer);
}

} // namespace

namespace
{
constexpr size_t item_complete_native_reference_encode_child_own_source =
	// input reference, output/context2P, actual reserve function-pointer F,
	// outer/query plus source/initial/supplement/request/child_outer7N, returned enum.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) +
	7 * sizeof(size_t) + sizeof(player_snapshot_codec_result);
player_snapshot_codec_result item_complete_native_reference_encode_child(
	const quest_mobile_native_reference &input,
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * quest_mobile_native_reference_encode_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer, child_outer = outer;
	// These are three distinct genuine scalar getter call occurrences. Their
	// output values are already the parent's actual adapter-owned locals.
	if (!payload_clone_add(request, query) || !reserve || !reserve(request, context) ||
	    !quest_mobile_native_reference_encode_source_frame_bytes(&source) ||
	    !quest_mobile_native_reference_encode_initial_inline_bytes(&initial) ||
	    !quest_mobile_native_reference_encode_source_supplement_frame_bytes(&supplement))
		return player_snapshot_codec_result::allocation_failure;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context) || !payload_clone_add(child_outer, supplement))
		return player_snapshot_codec_result::allocation_failure;
	return quest_mobile_native_reference_encode_bounded(input, output, reserve, context,
							    child_outer);
}

} // namespace

namespace
{
constexpr size_t item_complete_native_mobile_valid_retained_source =
	// Exact original-compatible wrapper Source and validator named own.
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + sizeof(uint16_t) +
	2 * sizeof(size_t) + sizeof(bool) + 7 * sizeof(void *) + 8 * sizeof(size_t) +
	2 * sizeof(uint64_t) + sizeof(uint16_t) + 6 * sizeof(bool) +
	// Actual runtime policy result, no mandatory-constant policy callee.
	sizeof(bool) + item_complete_native_validation_empty_vectors_source +
	item_complete_native_validation_cleanup_source +
	item_complete_native_validation_current_controller_source +
	item_complete_native_mobile_local_predicates_source +
	item_complete_native_mobile_array_access_source +
	item_complete_native_mobile_vector_access_source + item_complete_native_mobile_find_source +
	item_complete_native_canonical_equal_source +
	item_complete_native_roots_count_constructor_source +
	item_complete_native_uid_fresh_reserve_source +
	item_complete_native_uid_fitting_const_push_source +
	item_complete_native_uid_iterator_acquisition_source +
	item_complete_native_uid_adjacent_caller_source +
	item_complete_native_uid_sort_nonrecursive_source +
	item_complete_native_uid_adjacent_algorithm_source +
	item_complete_snapshot_list_decode_child_own_source +
	item_complete_snapshot_list_encode_child_own_source +
	item_complete_native_reference_encode_child_own_source +
	// Genuine three/three/two selected scalar getter call occurrences.
	3 * quest_mobile_native_reference_encode_source_query_frame_bytes() +
	3 * player_item_snapshot_list_decode_bounded_source_query_frame_bytes() +
	2 * player_item_snapshot_list_encode_source_query_frame_bytes();
} // namespace

namespace
{
bool item_native_mobile_validation_complete_owned(const item_transfer_payload &payload,
						  uint16_t version,
						  item_native_validation_complete_budget &budget)
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
	// The actual fixed reference survives every later child call. Admit it
	// before construction, then retain this physical INLINE once in frames;
	// the public Source constant excludes it. No later callback after unwind.
	if (!budget.peak(sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>)) ||
	    !payload_clone_add(budget.frames,
			       sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>)))
		return false;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference = {};
	size_t reference_prefix = 0;
	if (!budget.prefix(reference_prefix))
		return false;
	if (!item_owner_identity_valid(owner) || owner.id != context.reference.mobile_instance_id ||
	    item_complete_native_reference_encode_child(
		    context.reference, &reference, budget.reserve, budget.context,
		    reference_prefix) != player_snapshot_codec_result::ok ||
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
	if (!budget.peak(sizeof(std::vector<player_item_snapshot>)))
		return false;
	std::vector<player_item_snapshot> items;
	budget.items = &items;
	size_t prefix = 0;
	if (!budget.prefix(prefix))
		return false;
	if (item_complete_snapshot_list_decode_child(
		    payload.item_blob.data(), payload.item_blob_size, &items, budget.reserve,
		    budget.context, prefix, nullptr) != player_snapshot_codec_result::ok ||
	    items.size() != payload.item_count)
		return false;
	if (!budget.peak(sizeof(std::vector<uint8_t>)))
		return false;
	std::vector<uint8_t> canonical;
	budget.canonical = &canonical;
	if (!budget.prefix(prefix))
		return false;
	if (item_complete_snapshot_list_encode_child(items, &canonical, budget.reserve,
						     budget.context,
						     prefix) != player_snapshot_codec_result::ok ||
	    canonical.size() != payload.item_blob_size || !budget.peak() ||
	    !std::equal(canonical.begin(), canonical.end(), payload.item_blob.begin()))
		return false;
	if (items.size() > SIZE_MAX / sizeof(uint64_t) ||
	    !budget.peak(2 * sizeof(std::vector<uint64_t>) + items.size() * sizeof(uint64_t)))
		return false;
	std::vector<uint64_t> roots(items.size()), uids;
	budget.roots = &roots;
	budget.uids = &uids;
	if (!budget.peak(sizeof(std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH>)))
		return false;
	std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH> path = {};
	budget.path = &path;
	size_t depth = 0;
	if (!budget.peak(items.size() * sizeof(uint64_t)))
		return false;
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
		const auto *entry = item_complete_find_payload_item(payload, item.object_uid);
		if (!entry || entry->vnum != item.vnum || entry->parent_item_uid != parent ||
		    entry->root_item_uid != roots[index])
			return false;
		if (!budget.peak())
			return false;
		uids.push_back(item.object_uid);
	}
	size_t levels = 0, remaining = uids.size();
	while (remaining > 1)
	{
		remaining >>= 1;
		++levels;
	}
	if (!budget.peak((2 * levels + 1) * item_complete_native_uid_sort_recursive_source))
		return false;
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
} // namespace

bool item_transfer_native_mobile_context_valid_complete_bounded(
	const item_transfer_payload &payload, uint16_t version,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_native_mobile_valid_retained_source;
	item_native_validation_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_native_mobile_validation_complete_owned(payload, version, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_mobile_context_valid_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_mobile_valid_retained_source, child = 0;
	if (!payload_clone_add(result, item_complete_native_uid_sort_max_recursive_calls *
					       item_complete_native_uid_sort_recursive_source) ||
	    !quest_mobile_native_reference_encode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !player_item_snapshot_list_decode_bounded_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !player_item_snapshot_list_encode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*bytes = result;
	return true;
}
bool item_transfer_native_mobile_context_valid_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_native_mobile_context_valid_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_native_validation_complete_budget);
	return true;
}

#else
bool item_transfer_native_mobile_context_valid_complete_bounded(const item_transfer_payload &,
								uint16_t,
								bool (*)(size_t, void *) noexcept,
								void *, size_t) noexcept
{
	return false;
}
bool item_transfer_native_mobile_context_valid_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_mobile_context_valid_complete_source_supplement_frame_bytes(
	size_t *) noexcept
{
	return false;
}
bool item_transfer_native_mobile_context_valid_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
#endif

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
namespace
{
struct item_native_recovery_complete_budget
{
	item_native_recovery_reserve_fn reserve;
	void *context;
	size_t outer, frames;
	const std::vector<uint8_t> *before = nullptr, *after = nullptr, *bytes = nullptr;
	const item_native_mobile_recovery_context *recovery = nullptr;
	bool prefix(size_t &output, size_t extra = 0) const noexcept
	{
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    (before && (!payload_clone_add(total, sizeof(*before)) ||
				!payload_clone_vector_heap(*before, false, total))) ||
		    (after && (!payload_clone_add(total, sizeof(*after)) ||
			       !payload_clone_vector_heap(*after, false, total))) ||
		    (bytes && (!payload_clone_add(total, sizeof(*bytes)) ||
			       !payload_clone_vector_heap(*bytes, false, total))) ||
		    (recovery &&
		     (!payload_clone_add(total, sizeof(*recovery)) ||
		      !payload_clone_vector_heap(recovery->player_before.ordered_item_uids, false,
						 total) ||
		      !payload_clone_vector_heap(recovery->player_after.ordered_item_uids, false,
						 total) ||
		      !payload_clone_vector_heap(recovery->consumed_root_order, false, total) ||
		      !payload_clone_string_heap(recovery->publication_terms.message, false,
						 total) ||
		      !payload_clone_string_heap(recovery->publication_terms.disappear_message,
						 false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		output = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count) const noexcept
	{
		size_t request = 0;
		if (count > value.max_size() - value.size())
			return false;
		if (count > value.capacity() - value.size())
		{
			size_t next = value.size();
			if (!payload_clone_add(next, std::max(value.size(), count)) ||
			    next > value.max_size())
				next = value.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !payload_clone_add(request, next * sizeof(T)))
				return false;
		}
		return peak(request);
	}
	template <typename T> bool fresh(size_t count) const noexcept
	{
		size_t request = 0;
		return count <= SIZE_MAX / sizeof(T) &&
		       payload_clone_add(request, count * sizeof(T)) && peak(request);
	}
	bool text(std::string &value, const char *source, size_t length) const
	{
		size_t request = 0;
		if (length > value.capacity())
		{
			const size_t capacity = value.capacity();
			size_t next = length;
			if (capacity > SIZE_MAX / 2)
				return false;
			if (next < 2 * capacity)
				next = 2 * capacity;
			if (next > value.max_size())
				next = value.max_size();
			if (next == SIZE_MAX || !payload_clone_add(request, next + 1))
				return false;
		}
		if (!peak(request))
			return false;
		value.assign(source, length);
		return true;
	}
};
} // namespace

struct item_complete_native_recovery_root_count_predicate
{
	bool operator()(const player_item_snapshot &item) const
	{
		return item.parent_index == PLAYER_SNAPSHOT_NO_PARENT;
	}
};
struct item_complete_native_recovery_uid_predicate
{
	uint64_t uid;
	bool operator()(const player_item_snapshot &item) const { return item.object_uid == uid; }
};

namespace
{
constexpr size_t item_complete_recovery_encode_public_own_source =
	5 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 2 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_recovery_encode_owned_own_source =
	5 * sizeof(void *) + 2 * sizeof(std::vector<uint64_t>::const_iterator) +
	4 * sizeof(size_t) + sizeof(uint64_t) + sizeof(bool);
constexpr size_t item_complete_recovery_decode_public_own_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) +
	sizeof(std::span<const uint8_t>) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_recovery_decode_owned_own_source =
	3 * sizeof(void *) + sizeof(std::span<const uint8_t>) + 13 * sizeof(size_t) +
	sizeof(uint32_t) + sizeof(bool);
constexpr size_t item_complete_recovery_valid_public_own_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 2 * sizeof(size_t) +
	sizeof(uint16_t) + sizeof(bool);
constexpr size_t item_complete_recovery_valid_owned_own_source =
	// Common payload/recovery/owner refs, version, prefix, two role locals/B.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(uint16_t) +
	2 * sizeof(shop_trade_recovery_forest_role) + sizeof(bool) +
	// Consumption named root_count/i/uid/root (actual mutable row iterator).
	sizeof(std::ptrdiff_t) + sizeof(size_t) + sizeof(uint64_t) +
	sizeof(std::vector<player_item_snapshot>::iterator) +
	// Acceptance named before/after refs, first const UID iterator,index/prefix.
	2 * sizeof(void *) + sizeof(std::vector<uint64_t>::const_iterator) + 2 * sizeof(size_t);
constexpr size_t item_complete_recovery_root_count_predicate_operator_source =
	2 * sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_recovery_uid_predicate_operator_source =
	2 * sizeof(void *) + sizeof(bool);
} // namespace

namespace
{
namespace item_recovery_member_source = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_recovery_byte_default_source = 6 * sizeof(void *);
constexpr size_t item_complete_recovery_uid_default_source = 6 * sizeof(void *);
constexpr size_t item_complete_recovery_encode_lifetime_source =
	// before/after are actual empty byte vectors: same completed default lane.
	// candidate is fill-constructed separately, not default-constructed here.
	item_complete_recovery_byte_default_source +
	// Final candidate move owns its real temporary vector/allocator, exact
	// selected declared move/allocator/data scopes and temporary cleanup.
	item_recovery_member_source::complete_recipe_vector_move_selected_source<uint8_t> +
	// Candidate/before/after cleanup uses the same type after each call ends;
	// this distinct cleanup occurrence is kept in addition to the move temp.
	item_recovery_member_source::member_vector_cleanup_source<uint8_t>() +
	// Actual caller std::move(candidate) formal and returned reference.
	2 * sizeof(void *);
constexpr size_t item_complete_recovery_default_source =
	// Plain candidate DMI constructors: recovery/two forests/publication.
	// Arrays/scalars in DMIs have no undeclared trivial callable operations.
	4 * sizeof(void *) +
	// Three real empty u64 members, one completed same-type default lane;
	// both strings likewise reuse the char default lane sequentially.
	item_complete_recovery_uid_default_source +
	item_recovery_member_source::member_string_default_source;
constexpr size_t item_complete_recovery_cleanup_source =
	// Four generated nontrivial enclosing destructors; selected member
	// cleanup is separate. No new_allocator C++20 or trivial-array dtor.
	4 * sizeof(void *) + item_recovery_member_source::member_vector_cleanup_source<uint64_t>() +
	item_recovery_member_source::member_string_destroy_source;
constexpr size_t item_complete_recovery_move_source =
	// Recovery/two forest/publication generated nontrivial assignments each
	// owns this/other/returned reference3P; actual root std::move adds2P.
	4 * 3 * sizeof(void *) + 2 * sizeof(void *) +
	item_recovery_member_source::complete_recipe_vector_move_selected_source<uint64_t> +
	item_recovery_member_source::member_string_move_assignment_source +
	// Both mutable allocator getter references coexist in alloc_on_move.
	item_recovery_member_source::complete_recipe_string_mutable_allocator_source;
constexpr size_t item_complete_recovery_decode_lifetime_source =
	item_complete_recovery_default_source + item_complete_recovery_move_source +
	item_complete_recovery_cleanup_source;
constexpr size_t item_complete_recovery_valid_lifetime_source =
	// Parent selected row vector is empty before genuine child decoding.
	// Actual row/ED/DA/i32/string cleanup descendants are all reached after
	// the child produces populated rows and on each failure unwind.
	6 * sizeof(void *) + item_complete_native_vector_cleanup_source<player_item_snapshot> +
	item_complete_native_vector_cleanup_source<player_item_dynamic_affect_snapshot> +
	item_complete_native_vector_cleanup_source<player_item_extra_description_snapshot> +
	item_complete_native_vector_cleanup_source<int32_t> +
	item_recovery_member_source::member_string_destroy_source +
	// Optional plain continuation constructor+destructor, with actual two
	// char strings defaulted/destroyed. Its arrays{} add no trivial callees.
	2 * sizeof(void *) + item_recovery_member_source::member_string_default_source;
} // namespace

namespace
{
namespace item_recovery_control_source = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_recovery_checked_add_source =
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_recovery_callback_source =
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
template <class T> constexpr size_t item_complete_recovery_vector_current_source =
	// Actual false heap observer(value/bytes refs,fresh/count/result) then capacity.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) +
	item_recovery_control_source::complete_recipe_vector_capacity_source<T>;
constexpr size_t item_complete_recovery_string_current_source =
	// Actual false string heap: value/bytes refs,fresh/count/result; capacity
	// and its actual const islocal/data/local pointer/address chain.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	11 * sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_recovery_prefix_peak_source =
	// Accepted new recovery prefix this/output, extra/total, result.
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) +
	// peak this, extra/total and result; selected checked add/callback.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) +
	item_complete_recovery_checked_add_source + item_complete_recovery_callback_source;
constexpr size_t item_complete_recovery_encode_controller_source =
	item_complete_recovery_prefix_peak_source +
	// Only before/after/bytes observer pointers are set in the encoder.
	item_complete_recovery_vector_current_source<uint8_t> +
	// growth<byte> this/value, count/request/next and result; reached max,
	// size/capacity and std::max scopes. n+1/max arithmetic unchanged.
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	item_recovery_control_source::complete_recipe_vector_max_size_source<uint8_t> +
	item_recovery_control_source::complete_recipe_vector_size_source<uint8_t> +
	item_recovery_control_source::complete_recipe_vector_capacity_source<uint8_t> +
	item_recovery_control_source::complete_recipe_size_max_source;
constexpr size_t item_complete_recovery_decode_controller_source =
	item_complete_recovery_prefix_peak_source +
	// Only recovery pointer is set; three u64 and two char member heaps are
	// physically retained, same completed typed observers shared after return.
	item_complete_recovery_vector_current_source<uint64_t> +
	item_complete_recovery_string_current_source +
	// fresh<u64> this/count/request/result.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) +
	// Pre-reserved exact root_count means every growth<u64>(1) fits; no nextN.
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) +
	item_recovery_control_source::complete_recipe_vector_max_size_source<uint64_t> +
	item_recovery_control_source::complete_recipe_vector_size_source<uint64_t> +
	item_recovery_control_source::complete_recipe_vector_capacity_source<uint64_t> +
	// text this/value/source, length/request/capacity/next, result. Actual
	// capacity/local chain already above; only max_size query added here.
	3 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool) +
	item_recovery_control_source::complete_recipe_string_max_size_source;
constexpr size_t item_complete_recovery_valid_controller_source =
	// Actual shared validation prefix with items and optional terms only.
	// canonical/roots/uids/path stay null; their observers are not selected.
	4 * sizeof(void *) + 3 * sizeof(size_t) +
	2 * sizeof(std::vector<player_item_snapshot>::const_iterator) + sizeof(bool) +
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) +
	item_complete_recovery_checked_add_source + item_complete_recovery_callback_source +
	item_complete_recovery_vector_current_source<player_item_snapshot> +
	// Genuine selected row CURRENT and const row-range method family.
	44 * sizeof(void *) + 10 * sizeof(size_t) + 15 * sizeof(bool) + 14 * sizeof(void *) +
	sizeof(bool) + item_complete_recovery_string_current_source;
constexpr size_t item_complete_recovery_publication_valid_source =
	// Actual native_publication_terms_valid signature+bool; size, find(char,
	// pos) and traits find. Both string fields share after each call returns.
	sizeof(void *) + sizeof(bool) +
	item_recovery_control_source::complete_recipe_string_size_source +
	item_recovery_control_source::complete_recipe_string_find_own_source +
	item_recovery_control_source::complete_recipe_traits_char_find_source;
constexpr size_t item_complete_recovery_publication_empty_source =
	sizeof(void *) + sizeof(bool) +
	item_recovery_control_source::complete_recipe_string_empty_source;
constexpr size_t item_complete_recovery_span_direct_source =
	// Actual data receiver/result2P; size and dynamic extent receiver/result.
	2 * sizeof(void *) + 2 * (sizeof(void *) + sizeof(size_t));
constexpr size_t item_complete_recovery_span_subspan_source =
	// subspan(offset,explicitcount): this/offset/count. Explicit count is
	// never dynamic_extent here; assertion-only size calls are not executed.
	sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
	// Pointer/count span constructor, to_address/raw __to_address and actual
	// dynamic extent ctor. Class result is downstream forest adapter formal.
	7 * sizeof(void *) + 2 * sizeof(size_t) +
	// No-_GLIBCXX_ASSERTIONS still reaches the declared internal predicate.
	// Three sequential assert calls share this same completed Boolean lane;
	// assertion RHS size() calls remain unexecuted on this selected path.
	sizeof(bool);
constexpr size_t item_complete_recovery_span_member_lifetime_source =
	// Wrapper span passed by lvalue to owned helper: explicit defaulted copy
	// receiver/source2P and one shared explicit defaulted destructor receiver.
	// Caller-owned by-value span DTOs remain in true signatures/INLINE rows.
	3 * sizeof(void *);
} // namespace

namespace
{
using item_recovery_row_I = std::vector<player_item_snapshot>::iterator;
using item_recovery_uid_CI = std::vector<uint64_t>::const_iterator;
using item_recovery_count_C = item_complete_native_recovery_root_count_predicate;
using item_recovery_find_U = item_complete_native_recovery_uid_predicate;
using item_recovery_count_AC = decltype(__gnu_cxx::__ops::__pred_iter(item_recovery_count_C{}));
using item_recovery_find_AU = decltype(__gnu_cxx::__ops::__pred_iter(item_recovery_find_U{}));
using item_recovery_equal_E =
	decltype(__gnu_cxx::__ops::__iter_equals_val(std::declval<const uint64_t &>()));
constexpr size_t item_complete_recovery_count_graph_source =
	// Actual public/core mutable-row iterator objects, three distinct C
	// objects, core AC object and selected formal/local/result scopes.
	5 * sizeof(item_recovery_row_I) + 3 * sizeof(item_recovery_count_C) +
	sizeof(item_recovery_count_AC) + 18 * sizeof(void *) + 3 * sizeof(std::ptrdiff_t) +
	3 * sizeof(bool);
constexpr size_t item_complete_recovery_find_if_graph_source =
	// Actual mutable-row public/dispatch/RA iterators, three U/two AU
	// objects, trip-count/category and selected predicates/iterator helpers.
	7 * sizeof(item_recovery_row_I) + 3 * sizeof(item_recovery_find_U) +
	2 * sizeof(item_recovery_find_AU) + 19 * sizeof(void *) + 2 * sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(bool);
constexpr size_t item_complete_recovery_find_uid_graph_source =
	// Actual const-u64 find(value constref), equals factory/adapter and RA
	// path; returned class owns the one eventual caller result destination.
	7 * sizeof(item_recovery_uid_CI) + 2 * sizeof(item_recovery_equal_E) + 16 * sizeof(void *) +
	2 * sizeof(std::ptrdiff_t) + sizeof(std::random_access_iterator_tag) + sizeof(bool);
constexpr size_t item_complete_recovery_count_find_if_union_source =
	item_complete_recovery_count_graph_source + item_complete_recovery_find_if_graph_source -
	// Exact same row-I dereference2P, ++2P and two base reference scopes4P
	// return before the next algorithm; no same-type predicate/adapter share.
	8 * sizeof(void *);
constexpr size_t item_complete_recovery_count_find_caller_source =
	// Distinct row-I and UID-CI begin/end helper families. Getter class
	// results initialize actual algorithm formal objects, not extra DTOs.
	2 * 4 * sizeof(void *) +
	// Duplicate UID find result/RHS offset result + BOTH begin() receiver
	// temporaries for operator+; class results/formals elsewhere retained.
	4 * sizeof(item_recovery_uid_CI) +
	// Two genuine offset methods: this+nD+pointer materialization+ctor2P.
	2 * (4 * sizeof(void *) + sizeof(std::ptrdiff_t)) +
	// Final UID comparison owns both base reference returns distinctly.
	6 * sizeof(void *) + sizeof(bool) +
	// root==selected.end(): named root already own, actual end operand I;
	// row equality family already retained in count graph, genuine arrow2P.
	sizeof(item_recovery_row_I) + 2 * sizeof(void *) +
	// Acceptance first==before.end / before.end()-first / first-before.begin
	// each has a distinct CI operand; first itself remains named parent local.
	3 * sizeof(item_recovery_uid_CI) +
	// Difference this-pair/base-pair refs and genuine returned difference.
	6 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	// first[index] actual const iterator subscript this+n/ref return.
	2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	// selected.front(): this/ref, real returned begin I as implicit deref
	// receiver, explicit pointer ctor2P; this call occurrence kept separately.
	7 * sizeof(void *) + sizeof(item_recovery_row_I);
} // namespace

namespace
{
using item_recovery_equal_CI = std::vector<uint64_t>::const_iterator;
constexpr size_t item_complete_recovery_equal3_uid_raw_graph_source =
	// equal/aux own2CI+rawP+B each; two normal niter CI+3P and raw2P;
	// actual raw aux1/true-equal/memcmp wrapper and runtime query declarations.
	6 * sizeof(item_recovery_equal_CI) + 18 * sizeof(void *) + 2 * sizeof(size_t) +
	sizeof(int) + 6 * sizeof(bool);
constexpr size_t item_complete_recovery_equal3_uid_uid_graph_source =
	// equal/aux actual3CI+B each, three normal niter CI+3P and same typed
	// raw tail. No comparator, equal4/distance or intrinsic callee scopes.
	9 * sizeof(item_recovery_equal_CI) + 17 * sizeof(void *) + 2 * sizeof(size_t) +
	sizeof(int) + 6 * sizeof(bool);
constexpr size_t item_complete_recovery_consumption_equal_source =
	item_complete_recovery_equal3_uid_raw_graph_source +
	// const-u64 vector begin/end helper occurrences6P and mutable fixed
	// roots array begin/data4P; class results are equal formal destinations.
	10 * sizeof(void *);
constexpr size_t item_complete_recovery_acceptance_prefix_equal_source =
	item_complete_recovery_equal3_uid_uid_graph_source +
	// before.begin and after.begin scopes6P. Named first CI already parent
	// own, copied byvalue equal formal genuine; no implicit copy callable.
	6 * sizeof(void *);
constexpr size_t item_complete_recovery_acceptance_suffix_equal_source =
	item_complete_recovery_equal3_uid_uid_graph_source +
	// before.end and after.begin helper occurrences6P; two offset methods
	// each thisP+nD+real pointer prvalueP+explicit constref ctor2P.
	6 * sizeof(void *) + 2 * (4 * sizeof(void *) + sizeof(std::ptrdiff_t)) +
	// after.begin() is a real temporary CI implicit receiver of operator+;
	// its PLUS result directly initializes the third equal formal instead.
	sizeof(item_recovery_equal_CI);
constexpr size_t item_complete_recovery_validation_equal_source =
	// Conservative sum of THREE actual selected call occurrences; each has
	// exact mandatory result/formal identities. No arbitrary margin or
	// subtraction from unrelated historical pools. Parent named first/root
	// remain outside this leaf; selected.size() is direct-access family.
	item_complete_recovery_consumption_equal_source +
	item_complete_recovery_acceptance_prefix_equal_source +
	item_complete_recovery_acceptance_suffix_equal_source;
} // namespace

namespace
{
namespace item_recovery_wire_source = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_recovery_byte_default_n_a_own_source =
	3 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_recovery_byte_default_n_own_source =
	// first/n/returned first; declared constexpr can_fillB and actual
	// std::is_constant_evaluated resultB (its constant branch is unselected).
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t item_complete_recovery_byte_default_n_true_own_source =
	3 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_recovery_byte_default_construct_source =
	// __addressof(value-ref/pointer)2P; _Construct(p)P and actual internal
	// predicateB; runtime placement new(count,location,returned pointer).
	// No scalar ctor or runtime construct_at in the constant-only arm.
	2 * sizeof(void *) + sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_recovery_byte_default_fill_suffix_source =
	item_recovery_wire_source::complete_recipe_byte_fill_n_source +
	item_recovery_wire_source::complete_recipe_size_to_integer_source +
	item_recovery_wire_source::complete_recipe_byte_fill_n_a_source +
	// Category result aliases actual RA parameter, so only its constrefP.
	sizeof(void *) + item_recovery_wire_source::complete_recipe_byte_fill_a_source +
	item_recovery_wire_source::complete_recipe_byte_fill_a1_source +
	item_recovery_wire_source::complete_recipe_std_constant_evaluated_source;
constexpr size_t item_complete_recovery_byte_default_n_source =
	item_complete_recovery_byte_default_n_a_own_source +
	item_complete_recovery_byte_default_n_own_source +
	item_complete_recovery_byte_default_n_true_own_source +
	item_complete_recovery_byte_default_construct_source +
	item_complete_recovery_byte_default_fill_suffix_source;
constexpr size_t item_complete_recovery_byte_default_append_own_source =
	// this,n,size,navail,old_start,old_finish,len,new_start. No discarded
	// __destroy_from nor non-relocate branch controls; catch(...) has no ref.
	4 * sizeof(void *) + 4 * sizeof(size_t);
constexpr size_t item_complete_recovery_byte_resize_positive_source =
	// Actual resize(this,new_size), two reached size() occurrences; recovery encoder
	// calls resize(oldsize+8) or resize(oldsize+12), so erase/shrink branch is unselected.
	sizeof(void *) + sizeof(size_t) +
	2 * item_recovery_wire_source::complete_recipe_vector_size_source<uint8_t> +
	item_complete_recovery_byte_default_append_own_source +
	item_recovery_wire_source::complete_recipe_vector_size_source<uint8_t> +
	// Two runtime max_size occurrences in default_append's valid-size check.
	2 * item_recovery_wire_source::complete_recipe_vector_max_size_source<uint8_t> +
	item_complete_recovery_byte_default_n_source +
	// Growing path _M_check_len: exact own row, four actual size calls and
	// two max calls; bounded payload sizes cannot select overflow's third.
	item_recovery_wire_source::complete_recipe_vector_check_len_own_source<uint8_t> +
	4 * item_recovery_wire_source::complete_recipe_vector_size_source<uint8_t> +
	2 * item_recovery_wire_source::complete_recipe_vector_max_size_source<uint8_t> +
	item_recovery_wire_source::complete_recipe_size_max_source +
	item_recovery_wire_source::complete_recipe_vector_allocate_source<uint8_t> +
	item_recovery_wire_source::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	item_recovery_wire_source::complete_recipe_vector_relocate_source<uint8_t> +
	item_recovery_wire_source::complete_recipe_vector_deallocate_graph_source<uint8_t>;
constexpr size_t item_complete_recovery_byte_fill_static_max_source =
	// _S_max_size allocref/diffmax/allocmax/return=P3N, traits max=P+N,
	// min(two constrefs/returned ref)=3P; no public max/getconstallocator.
	5 * sizeof(void *) + 4 * sizeof(size_t);
constexpr size_t item_complete_recovery_byte_fill_allocator_reference_constructor_source =
	// Actual tmp vector(n,value,existing allocator-ref), no default allocator
	// argument object or its default ctor. Actual zero-byte belongs assign.
	3 * sizeof(void *) + sizeof(size_t) +
	item_recovery_wire_source::complete_recipe_byte_check_init_len_source +
	item_recovery_wire_source::complete_recipe_allocator_const_copy_source +
	item_recovery_wire_source::complete_recipe_allocator_cleanup_source +
	item_complete_recovery_byte_fill_static_max_source +
	item_recovery_wire_source::complete_recipe_byte_fill_base_source +
	item_recovery_wire_source::complete_recipe_vector_allocate_source<uint8_t> +
	item_recovery_wire_source::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	item_recovery_wire_source::complete_recipe_byte_fill_initialize_source +
	item_recovery_wire_source::complete_recipe_byte_uninitialized_fill_a_source +
	item_recovery_wire_source::complete_recipe_byte_uninitialized_fill_source +
	item_recovery_wire_source::complete_recipe_byte_uninitialized_fill_true_source +
	item_complete_recovery_byte_default_fill_suffix_source;
using item_recovery_byte_I = std::vector<uint8_t>::iterator;
using item_recovery_text_K = std::string::const_iterator;
constexpr size_t item_complete_recovery_header_fill_source =
	// Actual candidate(header24,0) has DEFAULT allocator argument, unlike
	// accepted borrowed-allocator fill-assign temporary. Own default object
	// plus allocator/new_allocator default receiver2P and allocator dtorP.
	item_complete_recovery_byte_fill_allocator_reference_constructor_source +
	sizeof(std::allocator<uint8_t>) + 3 * sizeof(void *);
constexpr size_t item_complete_recovery_forest_byte_insert_source =
	item_recovery_wire_source::complete_recipe_byte_end_insert_graph_source<
		item_recovery_byte_I, uint8_t *> -
	// Exact five unselected assign_one subtrees only: input uninitialized
	// copy and final fitting copy are count>=40 or0; fitting old move0 and
	// both growing old-move halves are >=24 or0. None can have count1.
	2 * item_recovery_wire_source::complete_recipe_byte_assign_one_source -
	3 * item_recovery_wire_source::complete_recipe_byte_move_assign_one_source;
constexpr size_t item_complete_recovery_text_insert_source =
	item_recovery_wire_source::complete_recipe_byte_end_insert_graph_source<
		item_recovery_text_K, const char *> -
	// char->byte scalar path remains complete even count1; only the three
	// actual old-byte move branches have guaranteed wide/zero counts.
	3 * item_recovery_wire_source::complete_recipe_byte_move_assign_one_source;
constexpr size_t item_complete_recovery_encode_insert_callers_source =
	// before/after mutable begin/end class prvalues initialize actual Input
	// formals. ReceiverP+explicit pointer ctor2P per actual getter occurrence.
	2 *
		(sizeof(void *) +
		 item_recovery_wire_source::complete_recipe_normal_pointer_constructor_source<
			 item_recovery_byte_I>)+
		// Both actual message K begin/end helpers own pointer-return materialized
		// objects and real const-data+constructor scopes; formals own K results.
		2 *
		(2 * sizeof(void *) +
		 item_recovery_wire_source::complete_recipe_string_const_data_source +
		 item_recovery_wire_source::complete_recipe_normal_pointer_constructor_source<
			 item_recovery_text_K>)+
		// Destination end I remains real through explicit I->CI conversion.
		item_recovery_wire_source::complete_recipe_wire_end_own_source +
	item_recovery_wire_source::complete_recipe_normal_pointer_constructor_source<
		item_recovery_byte_I>;
constexpr size_t item_complete_recovery_decode_uid_fitting_push_source =
	// Actual get_u64 prvalue selects fitting RVALUES, not mobile const push.
	// push/move/emplace/forward/construct/back/end/offset/deref exact scope;
	// two I temporaries are back's real end and offset implicit receivers.
	32 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	2 * sizeof(std::vector<uint64_t>::iterator);
constexpr size_t item_complete_recovery_decode_uid_reserve_source =
	// Original candidate empty; reserve(root_count) may be0, positive path
	// is exact reviewed fresh zero-oldsize reserve; no count>0 relocation B.
	48 * sizeof(void *) + 16 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(bool);
constexpr size_t item_complete_recovery_fresh_text_operations_source =
	item_recovery_wire_source::complete_recipe_string_assign_entry_source +
	item_recovery_wire_source::complete_recipe_string_replace_own_source +
	item_recovery_wire_source::complete_recipe_string_check_length_source +
	item_recovery_wire_source::complete_recipe_string_disjunct_source +
	2 * item_recovery_wire_source::complete_recipe_string_less_source +
	item_recovery_wire_source::complete_recipe_string_mutate_own_source +
	item_recovery_wire_source::complete_recipe_string_create_own_source +
	item_recovery_wire_source::complete_recipe_string_char_allocation_source;
// Selected fresh fields begin length0/disjoint, matching text39. Exact same
// copy/data-capacity setters/mutable allocator/size/default/move/destroy and
// runtime predicate families reside in actual member-lifetime14. max_size and
// capacity queries reside in actual controller15. No old umbrella borrowing.
} // namespace

namespace
{
constexpr size_t item_complete_recovery_policy_source = sizeof(bool);
constexpr size_t item_complete_recovery_put_wire_source =
	// Three actual unchanged wire helper definitions, not intrinsic shifts.
	sizeof(void *) + sizeof(uint16_t) + sizeof(void *) + sizeof(uint32_t) +
	sizeof(unsigned int) + sizeof(void *) + sizeof(uint64_t) + sizeof(unsigned int);
constexpr size_t item_complete_recovery_get_wire_source =
	// u16 input/returned value; u32/u64 input, named value, returned value,
	// and each helper's actual unsigned loop byte. Scalar NRVO not assumed.
	sizeof(void *) + sizeof(uint16_t) + sizeof(void *) + 2 * sizeof(uint32_t) +
	sizeof(unsigned int) + sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(unsigned int);
constexpr size_t item_complete_recovery_byte_data_source =
	// vector<byte>::data this/returned pointer2P, selected raw-pointer
	// _M_data_ptr const template this/input/returned pointer3P.
	5 * sizeof(void *);
constexpr size_t item_complete_recovery_encode_direct_source =
	item_complete_recovery_policy_source + item_complete_recovery_put_wire_source +
	item_complete_recovery_byte_data_source +
	// Direct byte/u64 size and char-string size calls in the encoder.
	// Actual caller occurrences retained separately from growth/mutation queries.
	3 * (sizeof(void *) + sizeof(size_t)) +
	// const u64 range: begin/end2P, two actual declared pointer ctors4P,
	// rewritten != ==2P/B with two simultaneously returned base refs4P,
	// prefix increment2P and dereference2P. Hidden two CI objects are own17.
	16 * sizeof(void *) + sizeof(bool) +
	// Final actual byte capacity receiver/scalar return.
	sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_recovery_decode_direct_source =
	// Decoder input span access and subspan are in the separate span leaf.
	item_complete_recovery_policy_source + item_complete_recovery_get_wire_source;
template <class T> constexpr size_t item_complete_recovery_const_vector_empty_source =
	// empty receiver/B; begin/end receiver2P, two genuine pointer ctors4P,
	// two live comparison operands CI, equality2P/B and base references4P.
	// No implicit iterator copy/qualification-conversion temporary.
	13 * sizeof(void *) + 2 * sizeof(typename std::vector<T>::const_iterator) +
	2 * sizeof(bool);
constexpr size_t item_complete_recovery_valid_direct_source =
	item_complete_recovery_policy_source +
	// Consumed roots and continuation data are the distinct actual types.
	item_complete_recovery_const_vector_empty_source<uint64_t> +
	item_complete_recovery_const_vector_empty_source<uint8_t> +
	// actual const u64 and mutable row subscript methods, scalar index/results.
	2 * (2 * sizeof(void *) + sizeof(size_t)) +
	// Direct u64/row/byte size and const byte-data calls. Each same typed
	// direct method lane is reused only after its completed call returns.
	3 * (sizeof(void *) + sizeof(size_t)) + item_complete_recovery_byte_data_source +
	// Const item_blob array data: direct captured _M_elems, no _S_ptr.
	2 * sizeof(void *) +
	// Mutable terms.roots array begin/data is in the consumption equal3 leaf;
	// char string empty/disappear and publication fields are in publication.
	// Optional reward decoder enum/bool result is priced by the real adapter.
	0;
} // namespace

namespace
{
constexpr size_t item_complete_native_forest_shape_adapter_source =
	2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) +
	sizeof(shop_trade_recovery_forest_role) + 7 * sizeof(size_t) + sizeof(bool);
bool item_complete_native_forest_shape_child(const shop_trade_recovery_forest_binding &forest,
					     shop_trade_recovery_forest_role role,
					     bool (*reserve)(size_t, void *) noexcept,
					     void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * shop_trade_recovery_forest_shape_valid_bounded_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer, child_outer = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !shop_trade_recovery_forest_shape_valid_bounded_source_frame_bytes(&source) ||
	    !shop_trade_recovery_forest_shape_valid_bounded_initial_inline_bytes(&initial) ||
	    !shop_trade_recovery_forest_shape_valid_bounded_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context) || !payload_clone_add(child_outer, supplement))
		return false;
	return shop_trade_recovery_forest_shape_valid_bounded(forest, role, reserve, context,
							      child_outer);
}

constexpr size_t item_complete_native_forest_encode_adapter_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) +
	sizeof(shop_trade_recovery_forest_role) + 7 * sizeof(size_t) + sizeof(bool);
bool item_complete_native_forest_encode_child(const shop_trade_recovery_forest_binding &forest,
					      shop_trade_recovery_forest_role role,
					      std::vector<uint8_t> *output,
					      bool (*reserve)(size_t, void *) noexcept,
					      void *context, size_t outer,
					      size_t *retained) noexcept
{
	constexpr size_t query =
		3 * shop_trade_recovery_forest_encode_bounded_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer, child_outer = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !shop_trade_recovery_forest_encode_bounded_source_frame_bytes(&source) ||
	    !shop_trade_recovery_forest_encode_bounded_initial_inline_bytes(&initial) ||
	    !shop_trade_recovery_forest_encode_bounded_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context) || !payload_clone_add(child_outer, supplement))
		return false;
	return shop_trade_recovery_forest_encode_bounded(forest, role, output, reserve, context,
							 child_outer, retained);
}

constexpr size_t item_complete_native_forest_decode_adapter_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) +
	sizeof(std::span<const uint8_t>) + sizeof(shop_trade_recovery_forest_role) +
	7 * sizeof(size_t) + sizeof(bool) + 3 * sizeof(void *);
bool item_complete_native_forest_decode_child(std::span<const uint8_t> bytes,
					      shop_trade_recovery_forest_role role,
					      shop_trade_recovery_forest_binding *output,
					      bool (*reserve)(size_t, void *) noexcept,
					      void *context, size_t outer,
					      size_t *retained) noexcept
{
	constexpr size_t query =
		3 * shop_trade_recovery_forest_decode_bounded_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer, child_outer = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !shop_trade_recovery_forest_decode_bounded_source_frame_bytes(&source) ||
	    !shop_trade_recovery_forest_decode_bounded_initial_inline_bytes(&initial) ||
	    !shop_trade_recovery_forest_decode_bounded_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context) || !payload_clone_add(child_outer, supplement))
		return false;
	return shop_trade_recovery_forest_decode_bounded(bytes, role, output, reserve, context,
							 child_outer, retained);
}
} // namespace

namespace
{
constexpr size_t item_complete_native_recovery_encode_retained_source =
	item_complete_recovery_encode_public_own_source +
	item_complete_recovery_encode_owned_own_source +
	item_complete_recovery_encode_lifetime_source +
	item_complete_recovery_encode_controller_source +
	item_complete_recovery_encode_direct_source + item_complete_recovery_header_fill_source +
	item_complete_recovery_forest_byte_insert_source +
	item_complete_recovery_text_insert_source +
	item_complete_recovery_encode_insert_callers_source +
	item_complete_recovery_byte_resize_positive_source +
	item_complete_native_forest_encode_adapter_source +
	3 * shop_trade_recovery_forest_encode_bounded_source_query_frame_bytes();
constexpr size_t item_complete_native_recovery_decode_retained_source =
	item_complete_recovery_decode_public_own_source +
	item_complete_recovery_decode_owned_own_source +
	item_complete_recovery_decode_lifetime_source +
	item_complete_recovery_decode_controller_source +
	item_complete_recovery_decode_direct_source +
	item_complete_recovery_decode_uid_reserve_source +
	item_complete_recovery_decode_uid_fitting_push_source +
	item_complete_recovery_fresh_text_operations_source +
	item_complete_recovery_publication_valid_source +
	item_complete_recovery_span_direct_source + item_complete_recovery_span_subspan_source +
	item_complete_recovery_span_member_lifetime_source +
	item_complete_native_forest_decode_adapter_source +
	3 * shop_trade_recovery_forest_decode_bounded_source_query_frame_bytes();
constexpr size_t item_complete_native_recovery_valid_retained_source =
	item_complete_recovery_valid_public_own_source +
	item_complete_recovery_valid_owned_own_source +
	item_complete_recovery_valid_lifetime_source +
	item_complete_recovery_valid_controller_source +
	item_complete_recovery_valid_direct_source +
	item_complete_recovery_publication_valid_source +
	item_complete_recovery_publication_empty_source +
	item_complete_recovery_count_find_if_union_source +
	item_complete_recovery_find_uid_graph_source +
	item_complete_recovery_count_find_caller_source +
	item_complete_recovery_validation_equal_source +
	item_complete_native_forest_shape_adapter_source +
	3 * shop_trade_recovery_forest_shape_valid_bounded_source_query_frame_bytes() +
	item_complete_snapshot_list_decode_child_own_source +
	3 * player_item_snapshot_list_decode_bounded_source_query_frame_bytes() +
	// Exact published generic continuation adapter owns its actual getter scopes.
	item_complete_offering_continuation_adapter_source;
} // namespace

namespace
{
bool item_native_recovery_encode_complete_owned(const item_native_mobile_recovery_context &recovery,
						std::vector<uint8_t> *out,
						item_native_recovery_complete_budget &owner,
						size_t *retained_encoded_heap_bytes)
{
	if (!owner.peak(2 * sizeof(std::vector<uint8_t>)))
		return false;
	std::vector<uint8_t> before, after;
	owner.before = &before;
	owner.after = &after;
	size_t prefix = 0;
	if (!owner.prefix(prefix))
		return false;
	if (!item_complete_native_forest_encode_child(
		    recovery.player_before, shop_trade_recovery_forest_role::player_before, &before,
		    owner.reserve, owner.context, prefix, nullptr) ||
	    !owner.prefix(prefix) ||
	    !item_complete_native_forest_encode_child(
		    recovery.player_after, shop_trade_recovery_forest_role::player_after, &after,
		    owner.reserve, owner.context, prefix, nullptr))
		return false;
	if (!owner.peak(sizeof(std::vector<uint8_t>) +
			ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_HEADER_BYTES + sizeof(uint8_t)))
		return false;
	std::vector<uint8_t> candidate(ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_HEADER_BYTES, 0);
	owner.bytes = &candidate;
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
	if (!owner.growth(candidate, before.size()))
		return false;
	candidate.insert(candidate.end(), before.begin(), before.end());
	if (!owner.growth(candidate, after.size()))
		return false;
	candidate.insert(candidate.end(), after.begin(), after.end());
	for (uint64_t root : recovery.consumed_root_order)
	{
		const size_t offset = candidate.size();
		if (!owner.growth(candidate, sizeof(uint64_t)))
			return false;
		candidate.resize(offset + sizeof(uint64_t));
		put_u64(candidate.data() + offset, root);
	}
	const size_t publication_start = candidate.size();
	if (!owner.growth(candidate, ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES))
		return false;
	candidate.resize(publication_start + ITEM_TRANSFER_NATIVE_MOBILE_PUBLICATION_HEADER_BYTES);
	put_u32(candidate.data() + publication_start,
		(recovery.publication_terms.echo_all ? 1U : 0U) |
			(recovery.publication_terms.disappear ? 2U : 0U));
	put_u32(candidate.data() + publication_start + 4,
		recovery.publication_terms.message.size());
	put_u32(candidate.data() + publication_start + 8,
		recovery.publication_terms.disappear_message.size());
	if (!owner.growth(candidate, recovery.publication_terms.message.size()))
		return false;
	candidate.insert(candidate.end(), recovery.publication_terms.message.begin(),
			 recovery.publication_terms.message.end());
	if (!owner.growth(candidate, recovery.publication_terms.disappear_message.size()))
		return false;
	candidate.insert(candidate.end(), recovery.publication_terms.disappear_message.begin(),
			 recovery.publication_terms.disappear_message.end());
	if (!owner.peak())
		return false;
	const size_t retained = candidate.capacity();
	*out = std::move(candidate);
	if (retained_encoded_heap_bytes)
		*retained_encoded_heap_bytes = retained;
	return true;
}
} // namespace

namespace
{
bool item_native_recovery_decode_complete_owned(std::span<const uint8_t> bytes,
						item_native_mobile_recovery_context *out,
						item_native_recovery_complete_budget &owner,
						size_t *retained_recovery_heap_bytes)
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
	if (!owner.peak(sizeof(item_native_mobile_recovery_context)))
		return false;
	item_native_mobile_recovery_context candidate;
	owner.recovery = &candidate;
	size_t prefix = 0;
	if (!owner.prefix(prefix))
		return false;
	candidate.present = true;
	candidate.player_pid = get_u32(bytes.data() + 8);
	candidate.acknowledged_save_revision = get_u64(bytes.data() + 16);
	if (!item_complete_native_forest_decode_child(
		    bytes.subspan(first, before_bytes),
		    shop_trade_recovery_forest_role::player_before, &candidate.player_before,
		    owner.reserve, owner.context, prefix, nullptr) ||
	    !owner.prefix(prefix) ||
	    !item_complete_native_forest_decode_child(bytes.subspan(after_start, after_bytes),
						      shop_trade_recovery_forest_role::player_after,
						      &candidate.player_after, owner.reserve,
						      owner.context, prefix, nullptr))
		return false;
	if (!owner.fresh<uint64_t>(root_count))
		return false;
	candidate.consumed_root_order.reserve(root_count);
	for (size_t i = 0; i < root_count; ++i)
	{
		if (!owner.growth(candidate.consumed_root_order, 1))
			return false;
		candidate.consumed_root_order.push_back(
			get_u64(bytes.data() + roots_start + i * sizeof(uint64_t)));
	}
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
	if (!owner.text(candidate.publication_terms.message,
			reinterpret_cast<const char *>(bytes.data() + text_start), message_bytes))
		return false;
	if (!owner.text(candidate.publication_terms.disappear_message,
			reinterpret_cast<const char *>(bytes.data() + text_start + message_bytes),
			disappear_bytes))
		return false;
	if (!native_publication_terms_valid(candidate.publication_terms))
		return false;
	// Full retained-own Source contains genuine generated nontrivial moves and
	// selected vector/string assignment+cleanup. Physical old output remains outer.
	if (!owner.peak())
		return false;
	size_t retained = 0;
	if (!payload_clone_vector_heap(candidate.player_before.ordered_item_uids, false,
				       retained) ||
	    !payload_clone_vector_heap(candidate.player_after.ordered_item_uids, false, retained) ||
	    !payload_clone_vector_heap(candidate.consumed_root_order, false, retained) ||
	    !payload_clone_string_heap(candidate.publication_terms.message, false, retained) ||
	    !payload_clone_string_heap(candidate.publication_terms.disappear_message, false,
				       retained))
		return false;
	*out = std::move(candidate);
	if (retained_recovery_heap_bytes)
		*retained_recovery_heap_bytes = retained;
	return true;
}
} // namespace

namespace
{
bool item_native_validation_recovery_complete_owned(
	const item_transfer_payload &payload, const item_native_mobile_recovery_context &recovery,
	uint16_t version, item_native_validation_complete_budget &owner)
{
	size_t budget_prefix = 0;
	const auto before_role = shop_trade_recovery_forest_role::player_before;
	const auto after_role = shop_trade_recovery_forest_role::player_after;
	if (!native_publication_terms_valid(recovery.publication_terms) ||
	    !owner.prefix(budget_prefix) ||
	    !item_complete_native_forest_shape_child(recovery.player_before, before_role,
						     owner.reserve, owner.context, budget_prefix) ||
	    !owner.prefix(budget_prefix) ||
	    !item_complete_native_forest_shape_child(recovery.player_after, after_role,
						     owner.reserve, owner.context, budget_prefix))
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
		if (!owner.peak(sizeof(std::vector<player_item_snapshot>)))
			return false;
		std::vector<player_item_snapshot> selected;
		owner.items = &selected;
		if (!owner.prefix(budget_prefix))
			return false;
		if (item_complete_snapshot_list_decode_child(
			    payload.item_blob.data(), payload.item_blob_size, &selected,
			    owner.reserve, owner.context, budget_prefix,
			    nullptr) != player_snapshot_codec_result::ok ||
		    selected.size() != payload.item_count)
			return false;
		if (!owner.peak(0))
			return false;
		const auto root_count =
			std::count_if(selected.begin(), selected.end(),
				      item_complete_native_recovery_root_count_predicate{});
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
			auto root =
				std::find_if(selected.begin(), selected.end(),
					     item_complete_native_recovery_uid_predicate{ uid });
			if (root == selected.end() ||
			    root->parent_index != PLAYER_SNAPSHOT_NO_PARENT || root->equipment_slot)
				return false;
		}
		if (payload.continuation.kind == item_transfer_continuation_kind::none)
			return payload.continuation.data.empty() &&
			       !recovery.publication_terms.disappear &&
			       recovery.publication_terms.disappear_message.empty();
		if (!owner.peak(sizeof(quest_reward_continuation)))
			return false;
		quest_reward_continuation terms;
		owner.terms = &terms;
		if (!owner.prefix(budget_prefix))
			return false;
		return payload.continuation.kind ==
			       item_transfer_continuation_kind::quest_offering &&
		       item_complete_generic_reward_continuation_decode_child(
			       payload.continuation.data.data(), payload.continuation.data.size(),
			       &terms, owner.reserve, owner.context, budget_prefix) &&
		       terms.version == 5 &&
		       terms.root_count == recovery.consumed_root_order.size() && owner.peak() &&
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
	if (!owner.peak(sizeof(std::vector<player_item_snapshot>)))
		return false;
	std::vector<player_item_snapshot> selected;
	owner.items = &selected;
	if (!owner.prefix(budget_prefix))
		return false;
	if (item_complete_snapshot_list_decode_child(
		    payload.item_blob.data(), payload.item_blob_size, &selected, owner.reserve,
		    owner.context, budget_prefix, nullptr) != player_snapshot_codec_result::ok ||
	    selected.size() != payload.item_count)
		return false;
	if (!owner.peak())
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
} // namespace

bool item_transfer_native_recovery_encode_complete_bounded(
	const item_native_mobile_recovery_context &recovery, std::vector<uint8_t> *out,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_encoded_heap_bytes) noexcept
{
	if (!out || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_native_recovery_encode_retained_source;
	item_native_recovery_complete_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak())
		return false;
	static_assert(std::is_nothrow_move_assignable_v<std::vector<uint8_t>>);
	try
	{
		return item_native_recovery_encode_complete_owned(recovery, out, owner,
								  retained_encoded_heap_bytes);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_recovery_decode_complete_bounded(
	std::span<const uint8_t> bytes, item_native_mobile_recovery_context *out,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_recovery_heap_bytes) noexcept
{
	if (!out || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_native_recovery_decode_retained_source;
	item_native_recovery_complete_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak())
		return false;
	static_assert(std::is_nothrow_move_assignable_v<item_native_mobile_recovery_context>);
	try
	{
		return item_native_recovery_decode_complete_owned(bytes, out, owner,
								  retained_recovery_heap_bytes);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_recovery_valid_complete_bounded(
	const item_transfer_payload &payload, const item_native_mobile_recovery_context &recovery,
	uint16_t version, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_native_recovery_valid_retained_source;
	item_native_validation_complete_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak())
		return false;
	try
	{
		return item_native_validation_recovery_complete_owned(payload, recovery, version,
								      owner);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_recovery_encode_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_recovery_encode_retained_source, child = 0;
	if (!shop_trade_recovery_forest_encode_bounded_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*bytes = result;
	return true;
}

bool item_transfer_native_recovery_encode_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_native_recovery_encode_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_native_recovery_complete_budget);
	return true;
}

bool item_transfer_native_recovery_decode_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_recovery_decode_retained_source, child = 0;
	if (!shop_trade_recovery_forest_decode_bounded_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*bytes = result;
	return true;
}

bool item_transfer_native_recovery_decode_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_native_recovery_decode_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_native_recovery_complete_budget);
	return true;
}

bool item_transfer_native_recovery_valid_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_recovery_valid_retained_source, child = 0;
	if (!shop_trade_recovery_forest_shape_valid_bounded_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !player_item_snapshot_list_decode_bounded_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !quest_reward_continuation_decode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*bytes = result;
	return true;
}

bool item_transfer_native_recovery_valid_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_native_recovery_valid_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_native_validation_complete_budget);
	return true;
}

#else
bool item_transfer_native_recovery_encode_complete_bounded(
	const item_native_mobile_recovery_context &, std::vector<uint8_t> *,
	bool (*)(size_t, void *) noexcept, void *, size_t, size_t *) noexcept
{
	return false;
}
bool item_transfer_native_recovery_decode_complete_bounded(std::span<const uint8_t>,
							   item_native_mobile_recovery_context *,
							   bool (*)(size_t, void *) noexcept,
							   void *, size_t, size_t *) noexcept
{
	return false;
}
bool item_transfer_native_recovery_valid_complete_bounded(
	const item_transfer_payload &, const item_native_mobile_recovery_context &, uint16_t,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept
{
	return false;
}
bool item_transfer_native_recovery_encode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_recovery_encode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_recovery_encode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_recovery_decode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_recovery_decode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_recovery_decode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_recovery_valid_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_recovery_valid_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_recovery_valid_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
#endif

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) && \
	!defined(_GLIBCXX_CONCEPT_CHECKS)
namespace
{
struct item_native_shape_complete_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const std::vector<uint8_t> *exact = nullptr;
	const quest_reward_continuation *terms = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    (exact && (!payload_clone_add(total, sizeof(*exact)) ||
			       !payload_clone_vector_heap(*exact, false, total))) ||
		    (terms && (!payload_clone_add(total, sizeof(*terms)) ||
			       !payload_clone_string_heap(terms->character_name, false, total) ||
			       !payload_clone_string_heap(terms->definition_id, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
namespace item_native_decl_source
{
constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool);
constexpr size_t F = sizeof(bool (*)(size_t, void *) noexcept);
constexpr size_t item_complete_native_cost_value_owned_own_source = 2 * P + N + B;
constexpr size_t item_complete_native_cost_value_wrapper_own_source = 3 * P + F + 2 * N + B;
} // namespace item_native_decl_source
constexpr size_t item_complete_native_cost_encode_child_own_source =
	// input reference, output/context2P, actual reserve function-pointer F,
	// outer/query plus source/initial/supplement/request/child_outer7N, returned enum.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) +
	7 * sizeof(size_t) + sizeof(native_quest_cost_projection_result);
constexpr size_t item_complete_native_cost_encode_child_query_source =
	3 * native_quest_cost_projection_encode_source_query_frame_bytes();

} // namespace
namespace
{
constexpr size_t item_complete_native_cost_value_controller_source =
	// Exact/terms selectors: this endpoint sets only exact byte vector.
	// Prefix2P2N+B; peakP2N+B; checked add P+N+B; actual reserve callback P+N+B.
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	// Actual false byte heap helper refs2P,countN,fresh/return2B + capacityP+N.
	3 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t item_complete_native_cost_value_lifetime_source =
	// Plain empty exact vector, then populated/empty destruction after child.
	6 * sizeof(void *) + item_complete_native_vector_cleanup_source<uint8_t>;
constexpr size_t item_complete_native_cost_value_direct_source =
	// payload attempts.empty(): const vector receiver/returnB, real two CI
	// operands, begin/end receiver2P, both ctor occurrences4P, ==2P+B/base4P.
	13 * sizeof(void *) + 2 * sizeof(std::vector<native_quest_cost_attempt>::const_iterator) +
	2 * sizeof(bool) +
	// Actual no-argument supported-policy returned bool in wrapper.
	sizeof(bool);
constexpr size_t item_complete_native_cost_value_retained_source =
	item_native_decl_source::item_complete_native_cost_value_wrapper_own_source +
	item_native_decl_source::item_complete_native_cost_value_owned_own_source +
	item_complete_native_cost_value_controller_source +
	item_complete_native_cost_value_lifetime_source +
	item_complete_native_cost_value_direct_source +
	item_complete_native_cost_encode_child_own_source +
	item_complete_native_cost_encode_child_query_source;
} // namespace

namespace
{
native_quest_cost_projection_result item_complete_native_cost_encode_child(
	const native_quest_cost_projection &input, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	constexpr size_t query = 3 * native_quest_cost_projection_encode_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer, child_outer = outer;
	// These are three distinct genuine scalar getter call occurrences. Their
	// output values are already the parent's actual adapter-owned locals.
	if (!payload_clone_add(request, query) || !reserve || !reserve(request, context) ||
	    !native_quest_cost_projection_encode_source_frame_bytes(&source) ||
	    !native_quest_cost_projection_encode_initial_inline_bytes(&initial) ||
	    !native_quest_cost_projection_encode_source_supplement_frame_bytes(&supplement))
		return native_quest_cost_projection_result::allocation_failure;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context) || !payload_clone_add(child_outer, supplement))
		return native_quest_cost_projection_result::allocation_failure;
	return native_quest_cost_projection_encode_bounded(input, output, reserve, context,
							   child_outer);
}

bool item_native_cost_value_valid_complete_owned(const item_transfer_payload &payload,
						 item_native_shape_complete_budget &budget)
{
	if (!payload.native_cost.present || !payload.native_cost.wallet_mapping_id ||
	    payload.native_mobile.action != item_native_mobile_action::consumption ||
	    payload.native_cost.projection.attempts.empty())
		return false;
	if (!budget.peak(sizeof(std::vector<uint8_t>)))
		return false;
	std::vector<uint8_t> exact;
	budget.exact = &exact;
	size_t admission_prefix = 0;
	if (!budget.prefix(admission_prefix))
		return false;
	return item_complete_native_cost_encode_child(payload.native_cost.projection, &exact,
						      budget.reserve, budget.context,
						      admission_prefix) ==
	       native_quest_cost_projection_result::ok;
}
} // namespace
bool item_transfer_native_cost_value_valid_complete_bounded(
	const item_transfer_payload &payload, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_native_cost_value_retained_source;
	item_native_shape_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_native_cost_value_valid_complete_owned(payload, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_cost_value_valid_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	size_t total = item_complete_native_cost_value_retained_source, child = 0;
	if (!native_quest_cost_projection_encode_source_frame_bytes(&child) ||
	    !payload_clone_add(total, child))
		return false;
	*bytes = total;
	return true;
}
bool item_transfer_native_cost_value_valid_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_native_cost_value_valid_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_native_shape_complete_budget);
	return true;
}

#else
bool item_transfer_native_cost_value_valid_complete_bounded(const item_transfer_payload &,
							    bool (*)(size_t, void *) noexcept,
							    void *, size_t) noexcept
{
	return false;
}
bool item_transfer_native_cost_value_valid_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_cost_value_valid_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_cost_value_valid_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
#endif

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) && \
	!defined(_GLIBCXX_CONCEPT_CHECKS)
namespace
{
struct item_native_wire_encode_complete_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const std::vector<uint8_t> *first = nullptr, *second = nullptr, *value = nullptr;
	const item_transfer_payload *original = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    (first && (!payload_clone_add(total, sizeof(*first)) ||
			       !payload_clone_vector_heap(*first, false, total))) ||
		    (second && (!payload_clone_add(total, sizeof(*second)) ||
				!payload_clone_vector_heap(*second, false, total))) ||
		    (value && (!payload_clone_add(total, sizeof(*value)) ||
			       !payload_clone_vector_heap(*value, false, total))) ||
		    (original && (!payload_clone_add(total, sizeof(*original)) ||
				  !payload_clone_heap(*original, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	bool growth(const std::vector<uint8_t> &bytes, size_t count) const noexcept
	{
		size_t request = 0;
		if (count > bytes.max_size() - bytes.size())
			return false;
		if (count > bytes.capacity() - bytes.size())
		{
			size_t next = bytes.size();
			if (!payload_clone_add(next, std::max(bytes.size(), count)) ||
			    next > bytes.max_size())
				next = bytes.max_size();
			if (!payload_clone_add(request, next))
				return false;
		}
		return peak(request);
	}
	bool clone_payload(const item_transfer_payload &source) const noexcept
	{
		size_t request = 0;
		return item_transfer_payload_fresh_copy_request_bytes(source, &request) &&
		       payload_clone_add(request, sizeof(item_transfer_payload)) && peak(request);
	}
};
struct item_native_wire_decode_complete_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const item_transfer_payload *payload = nullptr;
	const critical_command *expected = nullptr;
	const std::vector<uint8_t> *canonical = nullptr;
	const quest_reward_continuation *terms = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    (payload && (!payload_clone_add(total, sizeof(*payload)) ||
				 !payload_clone_heap(*payload, false, total))) ||
		    (canonical && (!payload_clone_add(total, sizeof(*canonical)) ||
				   !payload_clone_vector_heap(*canonical, false, total))) ||
		    (expected &&
		     (!payload_clone_add(total, sizeof(*expected)) ||
		      !payload_clone_vector_heap(expected->keys, false, total) ||
		      !payload_clone_vector_heap(expected->expected_revisions, false, total) ||
		      !payload_clone_vector_heap(expected->payload, false, total) ||
		      !payload_clone_vector_heap(expected->accounting_intent, false, total))) ||
		    (terms && (!payload_clone_add(total, sizeof(*terms)) ||
			       !payload_clone_string_heap(terms->character_name, false, total) ||
			       !payload_clone_string_heap(terms->definition_id, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
} // namespace

namespace
{
namespace item_native_decl_source
{
constexpr size_t item_complete_native_fee_shape_owned_own_source = 5 * P + N + 2 * B;
constexpr size_t item_complete_native_fee_shape_wrapper_own_source = 3 * P + F + 2 * N + 2 * B;
constexpr size_t item_complete_native_fee_encode_owned_own_source = 3 * P + 2 * N + 2 * B;
constexpr size_t item_complete_native_fee_encode_wrapper_own_source = 4 * P + F + 2 * N + 2 * B;
constexpr size_t item_complete_native_fee_decode_owned_own_source = 4 * P + 6 * N + 2 * B;
constexpr size_t item_complete_native_fee_decode_wrapper_own_source = 4 * P + F + 2 * N + B;
constexpr size_t item_complete_native_money_shape_owned_own_source = 5 * P + N + 2 * B;
constexpr size_t item_complete_native_money_shape_wrapper_own_source = 3 * P + F + 2 * N + 2 * B;
constexpr size_t item_complete_native_money_encode_owned_own_source = 3 * P + 2 * N + 2 * B;
constexpr size_t item_complete_native_money_encode_wrapper_own_source = 4 * P + F + 2 * N + 2 * B;
constexpr size_t item_complete_native_money_decode_owned_own_source = 4 * P + 2 * N + 2 * B;
constexpr size_t item_complete_native_money_decode_wrapper_own_source = 4 * P + F + 2 * N + B;
} // namespace item_native_decl_source

} // namespace

namespace
{
constexpr size_t item_complete_native_cost_decode_child_own_source =
	// input span object, output/context2P, actual reserve function-pointer F,
	// outer/query plus source/initial/supplement/request/child_outer7N, returned enum.
	sizeof(std::span<const uint8_t>) + 2 * sizeof(void *) +
	sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(native_quest_cost_projection_result);
constexpr size_t item_complete_native_cost_decode_child_span_call_source =
	// Actual child call copies adapter span lvalue into by-value input2P;
	// explicitly declared adapter span destructor receiverP. Child owns its
	// own span formal/destructor; pointer/count expression belongs caller.
	3 * sizeof(void *);
constexpr size_t item_complete_native_cost_decode_child_query_source =
	3 * native_quest_cost_projection_decode_source_query_frame_bytes();
constexpr size_t item_complete_native_money_encode_child_own_source =
	// input reference, output/context2P, actual reserve function-pointer F,
	// outer/query plus source/initial/supplement/request/child_outer7N, returned enum.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) +
	7 * sizeof(size_t) + sizeof(native_quest_coin_give_result);
constexpr size_t item_complete_native_money_encode_child_query_source =
	3 * native_quest_coin_give_encode_source_query_frame_bytes();
constexpr size_t item_complete_native_money_decode_child_own_source =
	// input span object, output/context2P, actual reserve function-pointer F,
	// outer/query plus source/initial/supplement/request/child_outer7N, returned enum.
	sizeof(std::span<const uint8_t>) + 2 * sizeof(void *) +
	sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(native_quest_coin_give_result);
constexpr size_t item_complete_native_money_decode_child_span_call_source =
	// Actual child call copies adapter span lvalue into by-value input2P;
	// explicitly declared adapter span destructor receiverP. Child owns its
	// own span formal/destructor; pointer/count expression belongs caller.
	3 * sizeof(void *);
constexpr size_t item_complete_native_money_decode_child_query_source =
	3 * native_quest_coin_give_decode_source_query_frame_bytes();
constexpr size_t item_complete_native_reference_encode_child_query_source =
	3 * quest_mobile_native_reference_encode_source_query_frame_bytes();
constexpr size_t item_complete_native_reference_decode_child_own_source =
	// input span object, output/context2P, actual reserve function-pointer F,
	// outer/query plus source/initial/supplement/request/child_outer7N, returned enum.
	sizeof(std::span<const uint8_t>) + 2 * sizeof(void *) +
	sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(player_snapshot_codec_result);
constexpr size_t item_complete_native_reference_decode_child_span_call_source =
	// Actual child call copies adapter span lvalue into by-value input2P;
	// explicitly declared adapter span destructor receiverP. Child owns its
	// own span formal/destructor; pointer/count expression belongs caller.
	3 * sizeof(void *);
constexpr size_t item_complete_native_reference_decode_child_query_source =
	3 * quest_mobile_native_reference_decode_source_query_frame_bytes();
} // namespace

namespace
{
native_quest_cost_projection_result item_complete_native_cost_decode_child(
	std::span<const uint8_t> input, native_quest_cost_projection *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	constexpr size_t query = 3 * native_quest_cost_projection_decode_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer, child_outer = outer;
	// These are three distinct genuine scalar getter call occurrences. Their
	// output values are already the parent's actual adapter-owned locals.
	if (!payload_clone_add(request, query) || !reserve || !reserve(request, context) ||
	    !native_quest_cost_projection_decode_source_frame_bytes(&source) ||
	    !native_quest_cost_projection_decode_initial_inline_bytes(&initial) ||
	    !native_quest_cost_projection_decode_source_supplement_frame_bytes(&supplement))
		return native_quest_cost_projection_result::allocation_failure;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context) || !payload_clone_add(child_outer, supplement))
		return native_quest_cost_projection_result::allocation_failure;
	return native_quest_cost_projection_decode_bounded(input, output, reserve, context,
							   child_outer);
}

} // namespace

namespace
{
native_quest_coin_give_result item_complete_native_money_encode_child(
	const native_quest_coin_give_projection &input, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	constexpr size_t query = 3 * native_quest_coin_give_encode_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer, child_outer = outer;
	// These are three distinct genuine scalar getter call occurrences. Their
	// output values are already the parent's actual adapter-owned locals.
	if (!payload_clone_add(request, query) || !reserve || !reserve(request, context) ||
	    !native_quest_coin_give_encode_source_frame_bytes(&source) ||
	    !native_quest_coin_give_encode_initial_inline_bytes(&initial) ||
	    !native_quest_coin_give_encode_source_supplement_frame_bytes(&supplement))
		return native_quest_coin_give_result::allocation_failure;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context) || !payload_clone_add(child_outer, supplement))
		return native_quest_coin_give_result::allocation_failure;
	return native_quest_coin_give_encode_bounded(input, output, reserve, context, child_outer);
}

} // namespace

namespace
{
native_quest_coin_give_result item_complete_native_money_decode_child(
	std::span<const uint8_t> input, native_quest_coin_give_projection *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	constexpr size_t query = 3 * native_quest_coin_give_decode_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer, child_outer = outer;
	// These are three distinct genuine scalar getter call occurrences. Their
	// output values are already the parent's actual adapter-owned locals.
	if (!payload_clone_add(request, query) || !reserve || !reserve(request, context) ||
	    !native_quest_coin_give_decode_source_frame_bytes(&source) ||
	    !native_quest_coin_give_decode_initial_inline_bytes(&initial) ||
	    !native_quest_coin_give_decode_source_supplement_frame_bytes(&supplement))
		return native_quest_coin_give_result::allocation_failure;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context) || !payload_clone_add(child_outer, supplement))
		return native_quest_coin_give_result::allocation_failure;
	return native_quest_coin_give_decode_bounded(input, output, reserve, context, child_outer);
}

} // namespace

namespace
{
player_snapshot_codec_result item_complete_native_reference_decode_child(
	std::span<const uint8_t> input, quest_mobile_native_reference *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * quest_mobile_native_reference_decode_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer, child_outer = outer;
	// These are three distinct genuine scalar getter call occurrences. Their
	// output values are already the parent's actual adapter-owned locals.
	if (!payload_clone_add(request, query) || !reserve || !reserve(request, context) ||
	    !quest_mobile_native_reference_decode_source_frame_bytes(&source) ||
	    !quest_mobile_native_reference_decode_initial_inline_bytes(&initial) ||
	    !quest_mobile_native_reference_decode_source_supplement_frame_bytes(&supplement))
		return player_snapshot_codec_result::allocation_failure;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context) || !payload_clone_add(child_outer, supplement))
		return player_snapshot_codec_result::allocation_failure;
	return quest_mobile_native_reference_decode_bounded(input, output, reserve, context,
							    child_outer);
}

} // namespace

namespace
{
constexpr size_t item_complete_fee_reward_continuation_decode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 8 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_fee_reward_continuation_decode_child_query_source =
	3 * quest_fee_reward_continuation_decode_source_query_frame_bytes();
bool item_complete_fee_reward_continuation_decode_child(const uint8_t *data, size_t size,
							quest_reward_continuation *output,
							bool (*reserve)(size_t, void *) noexcept,
							void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * quest_fee_reward_continuation_decode_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !quest_fee_reward_continuation_decode_source_frame_bytes(&source) ||
	    !quest_fee_reward_continuation_decode_initial_inline_bytes(&initial) ||
	    !quest_fee_reward_continuation_decode_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	// The selecting parent retains its output terms and both string heaps.
	// Full Source and INITIAL are transient; only the old child's uncovered
	// Source remains in outer while its original owner admits its own fields.
	return quest_fee_reward_continuation_decode_bounded(data, size, output, reserve, context,
							    child_outer);
}

} // namespace

namespace
{
template <class T> constexpr size_t item_payload_default_vector_source =
	// vector/base/impl/data + selected allocator/new_allocator default receivers.
	6 * sizeof(void *);
constexpr size_t item_payload_default_string_local_source = 4 * 2 * sizeof(void *);
constexpr size_t item_payload_default_string_init_source =
	sizeof(void *) + sizeof(size_t) + sizeof(char) + sizeof(bool);
constexpr size_t item_payload_default_string_set_length_source =
	2 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(char) + sizeof(bool);
constexpr size_t item_payload_default_string_source =
	// String receiver; hider receiver/data/default-allocator reference; genuine
	// temporary allocator; allocator/new_allocator default and const-copy;
	// move reference/result; real temporary allocator cleanup receiver.
	// The new_allocator destructor declaration is PP-excluded in C++20.
	sizeof(void *) + 3 * sizeof(void *) + sizeof(std::allocator<char>) + 2 * sizeof(void *) +
	4 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	item_payload_default_string_local_source + item_payload_default_string_init_source +
	item_payload_default_string_set_length_source;
constexpr size_t item_payload_aggregate_default_source =
	// Six strings and six vectors are initialized sequentially, with no
	// callback or nested member initialization through these leaf bodies.
	// Four uint64 vectors reuse their completed typed lane. The byte/attempt
	// specializations remain separate. No default/copy/move aggregate receiver
	// is invented for recursive aggregate initialization of value = {}.
	item_payload_default_string_source + item_payload_default_vector_source<uint8_t> +
	item_payload_default_vector_source<uint64_t> +
	item_payload_default_vector_source<native_quest_cost_attempt>;
template <class T> constexpr size_t item_payload_cleanup_vector_source =
	// Actual vector/base/generated _Vector_impl/allocator cleanup receivers.
	// _Vector_impl is nontrivial because allocator has a declared destructor;
	// the new_allocator destructor declaration is PP-excluded in C++20.
	4 * sizeof(void *) +
	// _Destroy(first,last,allocator), _Destroy(first,last), trivial aux<true>.
	3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	// _M_get_Tp_allocator this/reference result; _M_deallocate, traits,
	// allocator and new_allocator pointer/count receivers; sized delete;
	// two actual predicate wrapper results: _Destroy and allocator deallocate.
	// Their calls are separate; no intrinsic result is invented. T is trivial.
	2 * sizeof(void *) + 4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) +
	sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t item_payload_cleanup_string_source =
	// String/dispose/is-local/destroy receivers, destroy size and locality bool.
	4 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// Actual const data and local_data/pointer_to/addressof/__addressof.
	2 * sizeof(void *) + 4 * 2 * sizeof(void *) +
	// _M_get_allocator receiver/reference result. String destroy directly
	// calls traits deallocate: it does not call vector _M_deallocate.
	2 * sizeof(void *) + 3 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) +
	sizeof(size_t) + sizeof(bool) +
	// Actual hider/allocator destruction receivers. new_allocator destructor
	// is PP-excluded in selected C++20; allocator predicate has one result.
	2 * sizeof(void *);
constexpr size_t item_payload_cleanup_source =
	// Ten reached generated nontrivial aggregate destructor receiver sites:
	// payload, corpse, collector, continuation, recovery, two forests,
	// publication, native-cost context and native-cost projection.
	10 * sizeof(void *) + item_payload_cleanup_string_source +
	item_payload_cleanup_vector_source<uint8_t> + item_payload_cleanup_vector_source<uint64_t> +
	item_payload_cleanup_vector_source<native_quest_cost_attempt>;
constexpr size_t item_payload_caller_lifetime_source =
	item_payload_aggregate_default_source + item_payload_cleanup_source;
constexpr size_t item_complete_payload_plain_default_source =
	item_payload_aggregate_default_source +
	// Actual plain payload default constructors: payload/corpse/collector/
	// continuation/native-mobile/reference/recovery/two forests/publication.
	// native_cost={} and native_money={} initialize as aggregates, not these
	// enclosing default constructor lanes; their members remain in aggregate.
	10 * sizeof(void *);
constexpr size_t item_complete_payload_move_additional_source =
	// Exact ten nontrivial enclosing move sites from authenticated clone map:
	// payload/corpse/collector/continuation/recovery/two forests/publication/
	// native-cost context/projection. this/source/ref return3P per actual site.
	// Scalar-only native mobile/money/owner/entry/arrays add no invented body.
	10 * 3 * sizeof(void *) + 2 * sizeof(void *) +
	// Genuine three typed equal-standard-allocator vector assignments. Their
	// exact named cleanup subtree is already retained in lifetime above; this
	// subtraction removes only that same subtree, not a whole copy envelope.
	duris_craft_recipe_bounded_detail::complete_recipe_vector_move_selected_source<uint8_t> -
	duris_craft_recipe_bounded_detail::member_vector_cleanup_source<uint8_t>() +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_move_selected_source<uint64_t> -
	duris_craft_recipe_bounded_detail::member_vector_cleanup_source<uint64_t>() +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_move_selected_source<
		native_quest_cost_attempt> -
	duris_craft_recipe_bounded_detail::member_vector_cleanup_source<native_quest_cost_attempt>() +
	// Selected complete string assignment graph, plus second genuine mutable
	// allocator getter occurrence for two coexisting alloc_on_move references.
	// Default, assignment and destruction are distinct actual phase occurrences;
	// no unused COPY constructor/string allocation subtree is imported.
	duris_craft_recipe_bounded_detail::member_string_move_assignment_source +
	2 * sizeof(void *);
constexpr size_t item_complete_payload_plain_move_cleanup_source =
	item_complete_payload_plain_default_source + item_payload_cleanup_source +
	item_complete_payload_move_additional_source;
constexpr size_t item_complete_payload_aggregate_move_cleanup_source =
	item_payload_caller_lifetime_source + item_complete_payload_move_additional_source;
static_assert(std::is_trivially_destructible_v<native_quest_cost_attempt>);
} // namespace

namespace
{
template <class T> constexpr size_t item_complete_command_vector_copy_source =
	(19 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::allocator<T>)) +
	(11 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(bool)) + 6 * sizeof(void *) +
	// Trivial uninitialized-copy backend; actual _Num is ptrdiff_t.
	// No bare builtin memmove callee, implicit trivial T assignment, or
	// invented normal-iterator copy/move call. Actual wrapper objects stay.
	46 * sizeof(void *) + sizeof(std::ptrdiff_t) + 4 * sizeof(bool) -
	// The two begin/end result objects ARE the two by-value parameters.
	2 * sizeof(void *);
template <class T> constexpr size_t item_complete_command_vector_default_source =
	// vector/base/Impl/allocator/newallocator/Impl_data constructors.
	6 * sizeof(void *);
constexpr size_t item_complete_command_aggregate_default_source =
	item_complete_command_vector_default_source<critical_entity_key> +
	item_complete_command_vector_default_source<critical_expected_revision> +
	item_complete_command_vector_default_source<uint8_t>;
constexpr size_t item_complete_command_plain_default_source =
	// Plain command's generated nontrivial DMI default receiver is selected;
	// expected{} instead aggregate-initializes members without this call.
	sizeof(void *) + item_complete_command_aggregate_default_source;
constexpr size_t item_complete_command_copy_source =
	// Generated nontrivial command copy receiver/source; COPY construction.
	2 * sizeof(void *) + item_complete_command_vector_copy_source<critical_entity_key> +
	item_complete_command_vector_copy_source<critical_expected_revision> +
	item_complete_command_vector_copy_source<uint8_t>;
constexpr size_t item_complete_command_move_source =
	// Generated nontrivial assignment receiver/source/ref return3P. The actual
	// caller std::move<critical_command>2P belongs to the selecting parent.
	3 * sizeof(void *) +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_move_selected_source<
		critical_entity_key> +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_move_selected_source<
		critical_expected_revision> +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_move_selected_source<uint8_t>;
constexpr size_t item_complete_command_cleanup_source =
	// Generated command destructor receiver and three actual typed tails.
	// Payload/accounting_intent byte members reuse their completed same-T lane.
	sizeof(void *) +
	duris_craft_recipe_bounded_detail::member_vector_cleanup_source<critical_entity_key>() +
	duris_craft_recipe_bounded_detail::member_vector_cleanup_source<
		critical_expected_revision>() +
	duris_craft_recipe_bounded_detail::member_vector_cleanup_source<uint8_t>();
static_assert(std::is_trivial_v<critical_entity_key> &&
	      std::is_trivial_v<critical_expected_revision>);
static_assert(std::is_trivially_copyable_v<critical_entity_key> &&
	      std::is_trivially_copyable_v<critical_expected_revision>);
static_assert(std::is_trivially_destructible_v<critical_entity_key> &&
	      std::is_trivially_destructible_v<critical_expected_revision>);
} // namespace

namespace
{
namespace item_native_control_source = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_native_controller_checked_add_source =
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_native_controller_callback_source =
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_native_controller_prefix_peak_source =
	// Same actual shape/encode/decode methods each have their own parent
	// selected constant; equal widths do not merge distinct method bodies.
	// prefix this/out, extra/total, returnedbool; peak this/extra/total/result.
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(bool) + item_complete_native_controller_checked_add_source +
	item_complete_native_controller_callback_source;
template <class T> constexpr size_t item_complete_native_vector_current_source =
	// Actual payload_clone_vector_heap(false): value/bytes refs,fresh/count/result,
	// then selected capacity this/returnedN. size() true branch unselected.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) +
	item_native_control_source::complete_recipe_vector_capacity_source<T>;
constexpr size_t item_complete_native_string_current_source =
	// Actual payload_clone_string_heap(false), string capacity/is_local and
	// const data/local/pointer_to/addressof/__addressof. Same exact published
	// recovery/current family, no fresh string size() or allocation imported.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	11 * sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_native_fee_shape_controller_source =
	item_complete_native_controller_prefix_peak_source +
	// Only optional terms set; exact stays null in fee shape.
	item_complete_native_string_current_source;
constexpr size_t item_complete_native_money_shape_controller_source =
	// Money shape sets neither exact nor terms. Cost-value's distinct budget
	// and exact vector are child-owned and are not imported as this root.
	item_complete_native_controller_prefix_peak_source;
constexpr size_t item_complete_native_fee_money_encode_controller_source =
	item_complete_native_controller_prefix_peak_source +
	// first/second/value actual byte vectors set; original stays null in both
	// fee/money encoders. Three heap calls sequentially share exact typed body.
	item_complete_native_vector_current_source<uint8_t> +
	// growth: this/bytes,count/request/next,returned bool. All count/capacity
	// branches are possible; actual max_size,size,std::max added, capacity
	// already in the live current observation family above.
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	item_native_control_source::complete_recipe_vector_max_size_source<uint8_t> +
	item_native_control_source::complete_recipe_vector_size_source<uint8_t> +
	item_native_control_source::complete_recipe_size_max_source;
constexpr size_t item_complete_native_payload_current_source =
	// Actual payload_clone_heap(false): value/bytes refs,fresh/returnedbool.
	// Its six strings/six vectors reduce only completed same-type families;
	// each physical payload and all capacities remain separately in prefix.
	2 * sizeof(void *) + 2 * sizeof(bool) + item_complete_native_string_current_source +
	item_complete_native_vector_current_source<uint8_t> +
	item_complete_native_vector_current_source<uint64_t> +
	item_complete_native_vector_current_source<native_quest_cost_attempt>;
constexpr size_t item_complete_native_fee_decode_controller_source =
	item_complete_native_controller_prefix_peak_source +
	item_complete_native_payload_current_source +
	// expected keys/revisions additional typed false observers; expected's
	// two byte fields and canonical byte already share payload's typed lane.
	item_complete_native_vector_current_source<critical_entity_key> +
	item_complete_native_vector_current_source<critical_expected_revision>;
constexpr size_t item_complete_native_money_decode_controller_source =
	// Same genuine method set under this distinct decode parent; no terms
	// pointer is set, but actual payload has six string fields already mapped.
	item_complete_native_fee_decode_controller_source;
} // namespace

namespace
{
namespace item_native_lifetime_source = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_native_terms_lifetime_source =
	// Actual plain quest_reward_continuation terms has generated nontrivial
	// constructor/destructor receivers. Two strings reuse completed same-type
	// default/cleanup families. Scalar/array and source{} DMIs add no calls.
	2 * sizeof(void *) + item_payload_default_string_source +
	item_payload_cleanup_string_source;
constexpr size_t item_complete_native_empty_rhs_nonnull_deallocation_tail_source =
	// Each RHS forest{} / cost_projection{} vector stays size/capacity0.
	// Actual _M_deallocate(nullptr,0) reaches own2P+N only; these traits,
	// allocator,newallocator and sized-delete descendants are unselected.
	7 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_native_forest_aggregate_lifetime_source =
	// Actual two binding{} temporaries share completed u64 member method
	// families; enclosing aggregate constructor not called. Both physical
	// forest objects are already retained once by accepted119 shape wrapper.
	6 * sizeof(void *) + sizeof(void *) +
	item_native_lifetime_source::member_vector_cleanup_source<uint64_t>() -
	item_complete_native_empty_rhs_nonnull_deallocation_tail_source;
constexpr size_t item_complete_native_empty_cost_projection_lifetime_source =
	// native_quest_cost_projection{} aggregate temporary directly constructs
	// attempts; genuine nontrivial enclosing destructor receiver is reached.
	6 * sizeof(void *) + sizeof(void *) +
	item_native_lifetime_source::member_vector_cleanup_source<native_quest_cost_attempt>() -
	item_complete_native_empty_rhs_nonnull_deallocation_tail_source;
constexpr size_t item_complete_native_fee_shape_lifetime_source =
	item_complete_native_terms_lifetime_source +
	item_complete_native_forest_aggregate_lifetime_source;
constexpr size_t item_complete_native_money_shape_lifetime_source =
	item_complete_native_empty_cost_projection_lifetime_source +
	item_complete_native_forest_aggregate_lifetime_source +
	// Separate actual plain projected DTO default has scalar/array DMIs;
	// raw child candidate's own default receiver is a DIFFERENT call site.
	sizeof(void *);
constexpr size_t item_complete_native_byte_vector_lifetime_source =
	// Genuine empty vector cost/money/recovery/canonical variable sites.
	// Header count/value fill constructor is a separate operations fragment.
	6 * sizeof(void *) + item_native_lifetime_source::member_vector_cleanup_source<uint8_t>();
constexpr size_t item_complete_native_encode_value_move_source =
	// Parent caller std::move2P, selected byte assignment closure. Exact
	// named cleanup subtree already retained by byte lifetime above once.
	2 * sizeof(void *) +
	item_native_lifetime_source::complete_recipe_vector_move_selected_source<uint8_t> -
	item_native_lifetime_source::member_vector_cleanup_source<uint8_t>();
constexpr size_t item_complete_native_fee_money_encode_lifetime_source =
	item_complete_native_byte_vector_lifetime_source +
	item_complete_native_encode_value_move_source;
constexpr size_t item_complete_native_money_decode_lifetime_source =
	// Actual119 candidate/value is aggregate{}, not plain payload default.
	item_complete_payload_aggregate_move_cleanup_source +
	// Canonical byte and expected{} are distinct actual caller variables;
	// these operation occurrences conservatively retain their real families.
	item_complete_native_byte_vector_lifetime_source +
	item_complete_command_aggregate_default_source + item_complete_command_cleanup_source;
constexpr size_t item_complete_native_fee_decode_lifetime_source =
	item_complete_native_money_decode_lifetime_source +
	item_complete_native_terms_lifetime_source;
} // namespace

namespace
{
template <class T> constexpr size_t item_complete_native_integral_equal_tail_source =
	// Same selected declared __equal_aux1, __equal<true> and __memcmp scopes
	// for byte/int64/u64 integer pointers. Actual constant __simple object
	// exists; its traits initializer introduces no runtime callee.
	3 * sizeof(void *) + 2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(bool);
template <class T, size_t Count> constexpr size_t item_complete_native_integral_array_equal_source =
	// Actual array<T,Count> == refs/result; three const begin/end/data calls,
	// raw equal/aux, three raw niter and genuine typed selected true tail.
	2 * sizeof(void *) + sizeof(bool) + 12 * sizeof(void *) + 6 * sizeof(void *) +
	2 * sizeof(bool) + 6 * sizeof(void *) + item_complete_native_integral_equal_tail_source<T>;
template <class T> constexpr size_t item_complete_native_integral_vector_equal_source =
	// vector<T> == refs/result and TWO real size calls. Endpoint results
	// alias public equal formals; aux/niter iterator objects remain distinct.
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	9 * sizeof(void *) + 6 * sizeof(typename std::vector<T>::const_iterator) +
	2 * sizeof(bool) + 3 * sizeof(typename std::vector<T>::const_iterator) +
	9 * sizeof(void *) + item_complete_native_integral_equal_tail_source<T>;
constexpr size_t item_complete_native_empty_attempt_vector_equal_source =
	// Native money compares projection against a fresh projection{} whose
	// attempts size0. Size mismatch short-circuits; only equal-length zero
	// reaches __equal<false>. Its element loop is unexecuted, so no attempt,
	// requirement or nested cash comparisons belong to this selected lane.
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	9 * sizeof(void *) + 6 * sizeof(std::vector<native_quest_cost_attempt>::const_iterator) +
	2 * sizeof(bool) + 3 * sizeof(std::vector<native_quest_cost_attempt>::const_iterator) +
	9 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(bool) + 3 * sizeof(void *) +
	sizeof(bool);
constexpr size_t item_complete_native_coin_projection_equal_source =
	// Explicit defaulted equality this/other/result. Four same array<int64,4>
	// members call their exact selected family sequentially; reuse only after
	// each completed call. All four physical arrays remain input/output DTO.
	2 * sizeof(void *) + sizeof(bool) +
	item_complete_native_integral_array_equal_source<int64_t, 4>;
constexpr size_t item_complete_native_empty_cost_projection_equal_source =
	2 * sizeof(void *) + sizeof(bool) +
	// Two exact cash-array member calls sequentially reuse their family.
	item_complete_native_integral_array_equal_source<int64_t, 4> +
	item_complete_native_empty_attempt_vector_equal_source;
constexpr size_t item_complete_native_forest_value_equal_source =
	// Explicit defaulted forest== receiver/other/result, actual digest array
	// and UID vector. Both RHS forest{} calls share after completion; two real
	// RHS objects/empty vectors stay separately physical in original119.
	2 * sizeof(void *) + sizeof(bool) +
	item_complete_native_integral_array_equal_source<uint8_t, 32> +
	item_complete_native_integral_vector_equal_source<uint64_t>;
template <class T> constexpr size_t item_complete_native_const_vector_empty_source =
	// Empty root P+B, endpoint receiver/ctor6P, equality+base pair6P+B,
	// with TWO genuine const iterator comparison operands, no qualifier temp.
	13 * sizeof(void *) + 2 * sizeof(typename std::vector<T>::const_iterator) +
	2 * sizeof(bool);
constexpr size_t item_complete_native_absent_collector_helper_source =
	// payload ref, local collector ref, version formal, returned bool. Only
	// absent collector branch is reached after shape's explicit rejection.
	2 * sizeof(void *) + sizeof(uint16_t) + sizeof(bool) +
	item_complete_native_const_vector_empty_source<uint64_t>;
constexpr size_t item_complete_native_publication_empty_standalone_source =
	// Helper owns const terms ref/result. Actual string.empty invokes size;
	// both fields sequentially share the same exact char-string families.
	sizeof(void *) + sizeof(bool) +
	duris_craft_recipe_bounded_detail::complete_recipe_string_empty_source +
	duris_craft_recipe_bounded_detail::complete_recipe_string_size_source;
constexpr size_t item_complete_native_publication_valid_standalone_source =
	// Complete helper-only union, including find's real data getter. Parent
	// controller/member calls may share only explicitly named matching lanes.
	sizeof(void *) + sizeof(bool) +
	duris_craft_recipe_bounded_detail::complete_recipe_string_size_source +
	duris_craft_recipe_bounded_detail::complete_recipe_string_find_own_source +
	duris_craft_recipe_bounded_detail::complete_recipe_traits_char_find_source +
	duris_craft_recipe_bounded_detail::complete_recipe_string_const_data_source;
constexpr size_t item_complete_native_fee_shape_helper_source =
	item_complete_native_absent_collector_helper_source +
	item_complete_native_coin_projection_equal_source +
	item_complete_native_const_vector_empty_source<uint8_t> +
	// consumed_order empty shares the exact U64 empty branch above.
	item_complete_native_publication_empty_standalone_source +
	item_complete_native_publication_valid_standalone_source -
	// Same size() family selected by publication empty/valid, completed in
	// mutually exclusive acknowledgement branches. No unrelated margin.
	duris_craft_recipe_bounded_detail::complete_recipe_string_size_source +
	// Actual offering action-source generation comparison is array<byte,16>.
	item_complete_native_integral_array_equal_source<uint8_t, 16> +
	item_complete_native_forest_value_equal_source;
constexpr size_t item_complete_native_money_shape_helper_source =
	item_complete_native_absent_collector_helper_source +
	item_complete_native_empty_cost_projection_equal_source +
	item_complete_native_coin_projection_equal_source -
	// Exact cash array<int64,4> family is already retained by the earlier
	// empty-cost projection comparison; its completed child calls do not
	// coexist with this later coin projection comparison.
	item_complete_native_integral_array_equal_source<int64_t, 4> +
	item_complete_native_const_vector_empty_source<uint8_t> +
	item_complete_native_publication_empty_standalone_source +
	item_complete_native_forest_value_equal_source;
static_assert(std::is_integral_v<int64_t> && std::is_integral_v<uint64_t>);
static_assert(std::is_trivial_v<native_quest_cost_attempt> == false);
} // namespace

namespace
{
namespace item_native_direct_source = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_native_fee_shape_direct_source =
	// Each recorded accessor occurrence owns its actual receiver/returned
	// scalar carriers. Wire helpers reuse only after completed calls.
	sizeof(bool) + item_complete_recovery_byte_data_source + (sizeof(void *) + sizeof(size_t));
constexpr size_t item_complete_native_fee_encode_direct_source =
	// Each recorded accessor occurrence owns its actual receiver/returned
	// scalar carriers. Wire helpers reuse only after completed calls.
	sizeof(bool) + (sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	(sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	item_complete_recovery_byte_data_source + item_complete_recovery_byte_data_source +
	item_complete_recovery_byte_data_source + item_complete_recovery_byte_data_source +
	(sizeof(void *) + sizeof(size_t)) + item_complete_recovery_byte_data_source +
	(sizeof(void *) + sizeof(size_t)) + item_complete_recovery_byte_data_source +
	(sizeof(void *) + sizeof(size_t)) + item_complete_recovery_byte_data_source +
	item_complete_recovery_byte_data_source + item_complete_recovery_byte_data_source +
	item_complete_recovery_byte_data_source + item_complete_recovery_byte_data_source +
	item_complete_recovery_byte_data_source + (sizeof(void *) + sizeof(size_t)) +
	(sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	(sizeof(void *) + sizeof(size_t)) + item_complete_recovery_put_wire_source +
	2 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_native_fee_decode_direct_source =
	// Each recorded accessor occurrence owns its actual receiver/returned
	// scalar carriers. Wire helpers reuse only after completed calls.
	sizeof(bool) + (sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	item_complete_recovery_byte_data_source + (sizeof(void *) + sizeof(size_t)) +
	(sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	(sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	(sizeof(void *) + sizeof(size_t)) + item_complete_recovery_byte_data_source +
	(sizeof(void *) + sizeof(size_t)) + item_complete_recovery_get_wire_source +
	sizeof(uint16_t) + sizeof(bool) + 3 * (7 * sizeof(void *) + 2 * sizeof(size_t));
constexpr size_t item_complete_native_money_shape_direct_source =
	// Each recorded accessor occurrence owns its actual receiver/returned
	// scalar carriers. Wire helpers reuse only after completed calls.
	sizeof(bool);
constexpr size_t item_complete_native_money_encode_direct_source =
	// Each recorded accessor occurrence owns its actual receiver/returned
	// scalar carriers. Wire helpers reuse only after completed calls.
	sizeof(bool) + (sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	(sizeof(void *) + sizeof(size_t)) + item_complete_recovery_byte_data_source +
	item_complete_recovery_byte_data_source + item_complete_recovery_byte_data_source +
	item_complete_recovery_byte_data_source + (sizeof(void *) + sizeof(size_t)) +
	item_complete_recovery_byte_data_source + item_complete_recovery_byte_data_source +
	item_complete_recovery_byte_data_source + item_complete_recovery_byte_data_source +
	item_complete_recovery_byte_data_source + item_complete_recovery_byte_data_source +
	(sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	(sizeof(void *) + sizeof(size_t)) + item_complete_recovery_put_wire_source +
	2 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_native_money_decode_direct_source =
	// Each recorded accessor occurrence owns its actual receiver/returned
	// scalar carriers. Wire helpers reuse only after completed calls.
	sizeof(bool) + (sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	item_complete_recovery_byte_data_source + (sizeof(void *) + sizeof(size_t)) +
	(sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	(sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	(sizeof(void *) + sizeof(size_t)) + (sizeof(void *) + sizeof(size_t)) +
	item_complete_recovery_get_wire_source + sizeof(uint16_t) + sizeof(bool) +
	2 * (7 * sizeof(void *) + 2 * sizeof(size_t));
} // namespace

namespace
{
constexpr size_t item_complete_native_money_vector_range_span_source =
	// span(Range&&) receiver/ref2P; _Data CPO this/ref/pointer3P;
	// const vector.data2P and raw _M_data_ptr this/arg/result3P;
	// _Size CPO this/ref/size2P+N and const vector.size P+N;
	// delegated raw pointer/count2P+N, to_address/raw __to_address4P,
	// dynamic extent ctor P+N. CPO globals are not new temporary objects.
	20 * sizeof(void *) + 4 * sizeof(size_t);
constexpr size_t item_complete_native_money_default_subspan_source =
	// subspan(fixed) default-count route: this/offset/count P+2N;
	// genuine nonassert internal queryB, actual size/dynamicextent2P+2N,
	// data2P, downstream pointer/count constructor closure7P+2N.
	// Assertion RHS size() is short-circuited, but size()-offset executes.
	12 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_native_money_range_span_source =
	item_complete_native_money_vector_range_span_source +
	item_complete_native_money_default_subspan_source +
	// Explicit outer range receiver destructor. Adapter3P owns its different
	// onward copy2P and destructorP; do not share the two physical receivers.
	sizeof(void *);
constexpr size_t item_complete_native_money_range_span_inline =
	// Distinct outer temporary is subspan's implicit receiver. The returned
	// subspan object aliases existing child-adapter by-value formal once.
	sizeof(std::span<const uint8_t>);
} // namespace

namespace
{
// Selected original native fee/money byte writer Source only.
// Same published recipe header and same-TU published recovery byte field;
// no whole recovery/recipe envelope, new API, adapter, runtime or gate.
namespace item_native_byte_source = duris_craft_recipe_bounded_detail;
using item_native_byte_I = std::vector<uint8_t>::iterator;
using item_native_byte_CI = std::vector<uint8_t>::const_iterator;
using item_native_byte_raw = uint8_t *;
using item_native_byte_const_raw = const uint8_t *;

constexpr size_t item_complete_native_byte_header_failure_cleanup_source =
	// Genuine _Vector_impl and allocator receivers on allocation failure;
	// share only these exact2P with already retained typed parent cleanup.
	2 * sizeof(void *);
constexpr size_t item_complete_native_byte_header_fill_source =
	// Authentic published byte fill(n,zero,default allocator). The default
	// allocator object and copy/check allocator object are real Source here.
	// Caller vector DTO/allocated header heap and zero argument are separate.
	item_complete_recovery_header_fill_source +
	item_complete_native_byte_header_failure_cleanup_source;
constexpr size_t item_complete_native_byte_header_zero_inline_bytes = sizeof(uint8_t);

template <class Pointer> constexpr size_t item_complete_native_byte_raw_distance_source =
	// Public distance owns first/last and scalar difference result. Category
	// class prvalue is __distance's RA formal, not an extra returned object.
	item_native_byte_source::complete_recipe_distance_own_source<Pointer> + sizeof(void *) +
	item_native_byte_source::complete_recipe_ra_distance_own_source<Pointer>;

constexpr size_t item_complete_native_byte_reference_insert_source =
	// Actual mutable array.begin/end raw uint8_t* input. Public insert own
	// this/CI/first/last/offset/returnedI retains true types and destinations.
	item_native_byte_source::complete_recipe_byte_insert_own_source<item_native_byte_raw> +
	// Actual growing _M_range_insert: this, rawfirst/last, four old/new ptrs,
	// positionI, n/len, forward tag. No fitting mid/elems_after/oldfinish.
	7 * sizeof(void *) + sizeof(item_native_byte_I) + 2 * sizeof(size_t) +
	sizeof(std::forward_iterator_tag) +
	item_native_byte_source::complete_recipe_byte_const_iterator_conversion_source +
	item_native_byte_source::complete_recipe_normal_base_source<item_native_byte_I> +
	item_native_byte_source::complete_recipe_byte_cbegin_own_source +
	item_native_byte_source::complete_recipe_normal_pointer_constructor_source<
		item_native_byte_CI> +
	item_native_byte_source::complete_recipe_normal_subtract_own_source<item_native_byte_CI> +
	2 * item_native_byte_source::complete_recipe_normal_base_source<item_native_byte_CI> +
	// Both public begin()+offset have real begin receiver/temp and actual
	// operator+ pointer argument temporary. Results alias range/public I.
	2 * (item_native_byte_source::complete_recipe_wire_begin_own_source +
	     item_native_byte_source::complete_recipe_normal_pointer_constructor_source<
		     item_native_byte_I> +
	     item_native_byte_source::complete_recipe_normal_offset_argument_source<
		     item_native_byte_I> +
	     item_native_byte_source::complete_recipe_normal_pointer_constructor_source<
		     item_native_byte_I>)+item_native_byte_source::
			complete_recipe_iterator_category_source<item_native_byte_raw> +
	item_complete_native_byte_raw_distance_source<item_native_byte_raw> +
	// Raw pointer != and difference are builtins, not normal iterator calls.
	// Actual reference length148 never selects byte copy's assign_one.
	item_native_byte_source::complete_recipe_uninitialized_copy_graph_source<
		item_native_byte_raw, item_native_byte_raw> -
	item_native_byte_source::complete_recipe_byte_assign_one_source +
	// Growing halves move original header64 and empty suffix0. Neither is1.
	2 * (item_native_byte_source::complete_recipe_uninitialized_move_if_noexcept_graph_source -
	     item_native_byte_source::complete_recipe_byte_move_assign_one_source) +
	item_native_byte_source::complete_recipe_vector_check_len_own_source<uint8_t> +
	// Actual bounded _M_check_len calls size four times and max_size
	// twice. Retain per-occurrence scalar results, including the two
	// size() results overlapping in size()+max(size(),n). No family
	// reuse or unrelated old caller allowance supplies these carriers.
	2 * item_native_byte_source::complete_recipe_vector_max_size_source<uint8_t> +
	4 * item_native_byte_source::complete_recipe_vector_size_source<uint8_t> +
	item_native_byte_source::complete_recipe_size_max_source +
	item_native_byte_source::complete_recipe_vector_allocate_source<uint8_t> +
	item_native_byte_source::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	item_native_byte_source::complete_recipe_vector_destroy_range_source<uint8_t> +
	item_native_byte_source::complete_recipe_vector_deallocate_graph_source<uint8_t> +
	// The two real growing position.base() occurrences, no fitting-end lane.
	2 * item_native_byte_source::complete_recipe_normal_base_source<item_native_byte_I>;

constexpr size_t item_complete_native_byte_mutable_end_insert_source =
	item_native_byte_source::complete_recipe_byte_end_insert_graph_source<item_native_byte_I,
									      uint8_t *> -
	// Input cost/giver/recovery lengths are0 or>=24/96/176; fitting final
	// copy0, fitting oldmove0, growing oldprefix>=64 and suffix0. No count1.
	2 * item_native_byte_source::complete_recipe_byte_assign_one_source -
	3 * item_native_byte_source::complete_recipe_byte_move_assign_one_source;
constexpr size_t item_complete_native_byte_const_end_insert_source =
	item_native_byte_source::complete_recipe_byte_end_insert_graph_source<item_native_byte_CI,
									      const uint8_t *> -
	// Genuine continuation count1 input-copy lane stays. Only final fitting
	// copy[first,mid] is0 (advance(mid,elems_after0)), and old moves are wide/0.
	item_native_byte_source::complete_recipe_byte_assign_one_source -
	3 * item_native_byte_source::complete_recipe_byte_move_assign_one_source;

constexpr size_t item_complete_native_byte_raw_fresh_assign_source =
	// Fresh payload continuation has size/capacity0; raw wire pointers, not
	// span J or normal iterator. Positive assign allocates; zero copies0.
	item_native_byte_source::complete_recipe_byte_assign_own_source<item_native_byte_const_raw> +
	item_native_byte_source::complete_recipe_byte_fresh_assign_aux_own_source<
		item_native_byte_const_raw> +
	item_native_byte_source::complete_recipe_iterator_category_source<
		item_native_byte_const_raw> +
	item_complete_native_byte_raw_distance_source<item_native_byte_const_raw> +
	item_native_byte_source::complete_recipe_vector_capacity_source<uint8_t> +
	item_native_byte_source::complete_recipe_vector_size_source<uint8_t> +
	item_native_byte_source::complete_recipe_byte_check_init_len_source +
	item_native_byte_source::complete_recipe_allocator_const_copy_source +
	item_native_byte_source::complete_recipe_allocator_cleanup_source +
	// Exact _S_max_size5P4N, not unreached public max_size/getconstallocator.
	item_complete_recovery_byte_fill_static_max_source +
	item_native_byte_source::complete_recipe_byte_allocate_and_copy_own_source<
		item_native_byte_const_raw> +
	item_native_byte_source::complete_recipe_vector_allocate_source<uint8_t> +
	item_native_byte_source::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	item_native_byte_source::complete_recipe_uninitialized_copy_graph_source<
		item_native_byte_const_raw, item_native_byte_const_raw> +
	// Positive path destroys the old empty range; declaration wrappers are
	// reached, without element dtor. Old _M_deallocate(nullptr,0) stops at its
	// own this/pointer/count; no traits/allocator/delete body selected here.
	item_native_byte_source::complete_recipe_vector_destroy_range_source<uint8_t> +
	2 * sizeof(void *) + sizeof(size_t) +
	// Zero path calls actual raw copy and erase. copy count0 excludes only
	// assign_one; erase's localn0 excludes its conditional Destroy subtree.
	item_native_byte_source::complete_recipe_copy_graph_source<
		false, item_native_byte_const_raw, uint8_t *, item_native_byte_const_raw> -
	item_native_byte_source::complete_recipe_byte_assign_one_source +
	item_native_byte_source::complete_recipe_byte_erase_at_end_own_source;

constexpr size_t item_complete_native_byte_reference_insert_callers_source =
	// Two mutable array begin/end each receiver/result2P + data2P. Destination
	// end() returns a real I temporary bound to converting const-iterator ctor.
	8 * sizeof(void *) + item_native_byte_source::complete_recipe_wire_end_own_source +
	item_native_byte_source::complete_recipe_normal_pointer_constructor_source<
		item_native_byte_I>;
constexpr size_t item_complete_native_byte_mutable_end_insert_callers_source =
	// Source begin/end class results initialize public Input formals. Actual
	// getter receiver and pointer-ref constructor2P each, no second I result.
	6 * sizeof(void *) + item_native_byte_source::complete_recipe_wire_end_own_source +
	item_native_byte_source::complete_recipe_normal_pointer_constructor_source<
		item_native_byte_I>;
constexpr size_t item_complete_native_byte_const_end_insert_callers_source =
	// Stored pointer lvalues bind reference-compatibly; no qualification temp.
	6 * sizeof(void *) + item_native_byte_source::complete_recipe_wire_end_own_source +
	item_native_byte_source::complete_recipe_normal_pointer_constructor_source<
		item_native_byte_I>;
constexpr size_t item_complete_native_byte_raw_fresh_assign_callers_source = 0;
// Raw pointer arithmetic has no separately declared library function. Its
// inputs/lengths/wire pointer are genuine existing decoder-own locals.

} // namespace

namespace
{
// Selected GNU13 ordinary C++20 equality graph only. Includes genuine
// caller begin/end declarations and their selected constructor temporaries.
// Count per actual call occurrence conservatively; loop iterations reuse
// those same physical scopes. Input container/element heaps stay caller-owned.
// No old equal_frames, recovery-u64 profile, encode, sort or byte allocation.
// Each first vector iterator class return is the corresponding public equal
// by-value formal destination (guaranteed elision), not another result object.
// Subsequent by-value aux/niter formals are real independent objects; implicit
// trivial iterator/predicate special members have no declared callee scopes.

constexpr size_t item_complete_native_predicate_iterators_source =
	// Caller const begin/end and mutable begin:3 receiverP plus the
	// selected ctor this/reference6P. Stored pointer lvalues bind
	// reference-compatibly: no pointer qualification temporary.
	9 * sizeof(void *) +
	// Loop != is C++20 rewritten ==: two references+B; its two base calls
	// each have this/reference2P. No additional operator!= callable scope.
	6 * sizeof(void *) + sizeof(bool) +
	// Prefix++ on const/mutable each has this/returned-reference2P;
	// dereference on each likewise has this/returned-reference2P.
	8 * sizeof(void *);

constexpr size_t item_complete_native_keys_equal_source =
	item_complete_native_predicate_iterators_source +
	// std::equal actual2constI+mutableI, function-pointer predicate, resultB.
	2 * sizeof(std::vector<critical_entity_key>::const_iterator) +
	sizeof(std::vector<critical_entity_key>::iterator) +
	sizeof(decltype(&critical_entity_key_equal)) + sizeof(bool) +
	// Real critical_entity_key_equal refs2P/resultB, not an adapter.
	2 * sizeof(void *) + sizeof(bool);

template <class Predicate> constexpr size_t item_complete_native_revisions_equal_source =
	item_complete_native_predicate_iterators_source +
	// Public equal's actual input formals and genuine empty Predicate object.
	// Caller Predicate{} initializes this formal directly; no second object.
	2 * sizeof(std::vector<critical_expected_revision>::const_iterator) +
	sizeof(std::vector<critical_expected_revision>::iterator) + sizeof(Predicate) +
	sizeof(bool) +
	// Actual const Predicate::operator():this/two refs3P/resultB. Nested
	// key_equal refs2P/resultB. Scalar revision comparison has no callee.
	5 * sizeof(void *) + 2 * sizeof(bool);

constexpr size_t item_complete_native_byte_equal_tail_source =
	// __equal_aux1 raw3P/resultB + actual const local __simpleB. Its traits
	// initializer is required constant-evaluated (template argument), with
	// no runtime trait callee. The automatic local object still exists.
	3 * sizeof(void *) + 2 * sizeof(bool) +
	// Selected __equal<true>::equal: raw3P/local size_t __len/resultB.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// __memcmp raw2P/countN/int result; genuine runtime public
	// std::is_constant_evaluated returnedB. No invented builtin callee.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(bool);

constexpr size_t item_complete_native_fee_money_canonical_equal_source =
	// C++20 != rewrites vector==:refs2P/resultB only. Const size twice:
	// two actual thisP/size_t-resultN occurrences (no old root size credit).
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	// Three const begin/end receivers3P and selected pointer-ref ctors6P.
	// Stored pointer lvalues bind reference-compatibly with no
	// qualification temporary. Class results are public equal formals.
	9 * sizeof(void *) +
	// public equal and __equal_aux: each3 real iterator formals/resultB.
	6 * sizeof(std::vector<uint8_t>::const_iterator) + 2 * sizeof(bool) +
	// Three normal-iterator niter calls:by-valueI/raw-pointer resultP,
	// each real base this/reference2P. Three actual distinct call occurrences.
	3 * sizeof(std::vector<uint8_t>::const_iterator) + 9 * sizeof(void *) +
	item_complete_native_byte_equal_tail_source;

constexpr size_t item_complete_native_action_operation_equal_source =
	// C++20 != rewrites array== refs2P/resultB. Three const begin/end
	// calls each this/raw-result2P and reached data this/raw-result2P.
	// Array data returns _M_elems directly: no _S_ptr/_S_ref helper.
	2 * sizeof(void *) + sizeof(bool) + 12 * sizeof(void *) +
	// public equal/aux each raw3P/resultB; three fallback niter calls
	// each raw-by-valueP/raw-resultP. No normal iterator or array DTO callee.
	6 * sizeof(void *) + 2 * sizeof(bool) + 6 * sizeof(void *) +
	item_complete_native_byte_equal_tail_source;

} // namespace

namespace
{
struct item_native_fee_revision_equal
{
	bool operator()(const critical_expected_revision &a,
			const critical_expected_revision &b) const
	{
		return critical_entity_key_equal(a.key, b.key) && a.revision == b.revision;
	}
};
struct item_native_money_revision_equal
{
	bool operator()(const critical_expected_revision &left,
			const critical_expected_revision &right) const
	{
		return critical_entity_key_equal(left.key, right.key) &&
		       left.revision == right.revision;
	}
};

} // namespace

namespace
{
struct item_native_entities_complete_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *command = nullptr;
	const std::vector<uint64_t> *roots = nullptr;
	const std::vector<player_item_snapshot> *outputs = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		size_t total = outer, row = 0;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames))
			return false;
		if (command &&
		    (!payload_clone_add(total, sizeof(*command)) ||
		     !payload_clone_vector_heap(command->keys, false, total) ||
		     !payload_clone_vector_heap(command->expected_revisions, false, total) ||
		     !payload_clone_vector_heap(command->payload, false, total) ||
		     !payload_clone_vector_heap(command->accounting_intent, false, total)))
			return false;
		if (roots && (!payload_clone_add(total, sizeof(*roots)) ||
			      !payload_clone_vector_heap(*roots, false, total)))
			return false;
		if (outputs)
		{
			if (!payload_clone_add(total, sizeof(*outputs)) ||
			    !payload_clone_vector_heap(*outputs, false, total))
				return false;
			for (const auto &item : *outputs)
				if (!player_item_snapshot_current_heap_bytes(item, &row) ||
				    !payload_clone_add(total, row))
					return false;
		}
		if (!payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count) const noexcept
	{
		size_t request = 0;
		if (count > value.max_size() - value.size())
			return false;
		if (count > value.capacity() - value.size())
		{
			size_t next = value.size();
			if (!payload_clone_add(next, std::max(value.size(), count)) ||
			    next > value.max_size())
				next = value.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !payload_clone_add(request, next * sizeof(T)))
				return false;
		}
		return peak(request);
	}
	template <typename T> bool fresh(size_t count, size_t extra = 0) const noexcept
	{
		size_t request = 0;
		return count <= SIZE_MAX / sizeof(T) &&
		       payload_clone_add(request, count * sizeof(T)) &&
		       payload_clone_add(request, extra) && peak(request);
	}
	template <typename T, typename Comparator> bool sort_frame(size_t count) const noexcept
	{
		// Only the shape-proven native count-two callsites select this class.
		// Actual two-element sort Source is already retained by frames. No
		// introsort recursive/pivot/heap phase is executed at these callsites.
		(void)count;
		return peak();
	}
};
} // namespace

namespace
{
// Exact named equivalent of the original revision-sort lambda. The native
// selected companion will bind this real type, without claiming lambda layout.
struct item_native_entities_revision_less
{
	bool operator()(const critical_expected_revision &left,
			const critical_expected_revision &right) const
	{
		return critical_entity_key_less(left.key, right.key);
	}
};
template <class T> constexpr size_t item_complete_native_entities_fresh_assign_source =
	// Selected fresh vector initializer-list assignment of exactly two trivial
	// elements. IL formal/backing array are caller INLINE, excluded here.
	// Public assignment2P; actual IL pointer/count ctor and begin/end/begin/size.
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) + 7 * sizeof(void *) +
	sizeof(size_t) +
	// forward aux this/first/last/tmp, len,F plus sliced RA; raw distance.
	4 * sizeof(void *) + sizeof(size_t) + sizeof(std::forward_iterator_tag) +
	sizeof(std::random_access_iterator_tag) + 5 * sizeof(void *) + 2 * sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(size_t) +
	6 * sizeof(void *) +
	// check_init converted allocator object, const-copy/cleanup, max/traits/min.
	10 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(std::allocator<T>) + 5 * sizeof(void *) +
	sizeof(size_t) + 11 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(bool) +
	// raw const T* -> T* uninitialized copy; actual Num2 skips assign_one.
	46 * sizeof(void *) + sizeof(std::ptrdiff_t) + 4 * sizeof(bool) +
	// Null old Destroy still reaches its actual runtime predicate; null
	// deallocate reaches only its root, no allocator/delete child.
	7 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t);
template <class T, class Comparator>
constexpr size_t item_complete_native_entities_sort_two_common_source =
	// Actual count2 public/factory/__sort/introsort/final/insertion/adapter.
	// Introsort has no __cut declaration reached, recursion or pivot/heap.
	13 * sizeof(typename std::vector<T>::iterator) + 3 * sizeof(Comparator) +
	4 * sizeof(__gnu_cxx::__ops::_Iter_comp_iter<Comparator>) + 28 * sizeof(void *) +
	5 * sizeof(std::ptrdiff_t) + 2 * sizeof(bool);
template <class T> constexpr size_t item_complete_native_entities_fee_backward_extra_source =
	// Actual reversed two-element insertion owns __val and one-element backward.
	// Same completed normal offset4P+D shares with common first+1; real
	// std::move<T>2P in backward serves completed insertion moves. No implicit
	// trivial element/iterator callable is introduced. Root discarded I stays.
	sizeof(T) + 14 * sizeof(typename std::vector<T>::iterator) + 29 * sizeof(void *) +
	sizeof(std::ptrdiff_t) + sizeof(bool);
template <class T, class Comparator>
constexpr size_t item_complete_native_entities_money_linear_extra_source =
	// Already-sorted count2 invokes unguarded insertion and val comparator;
	// loop body false. Actual converting adapter and two real move calls.
	3 * sizeof(typename std::vector<T>::iterator) +
	sizeof(__gnu_cxx::__ops::_Iter_comp_iter<Comparator>) +
	sizeof(__gnu_cxx::__ops::_Val_comp_iter<Comparator>) + sizeof(T) + 12 * sizeof(void *) +
	sizeof(bool);
constexpr size_t item_complete_native_entities_key_less_target_source =
	2 * sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_native_entities_revision_less_target_source =
	// Named operator this/left/right/result, then genuine key comparator.
	3 * sizeof(void *) + sizeof(bool) + item_complete_native_entities_key_less_target_source;
template <class T, class Equal> constexpr size_t item_complete_native_entities_adjacent_source =
	// Standalone selected function-pointer predicate path, no default equality.
	7 * sizeof(typename std::vector<T>::iterator) + 3 * sizeof(Equal) +
	sizeof(__gnu_cxx::__ops::_Iter_comp_iter<Equal>) + 20 * sizeof(void *) + 3 * sizeof(bool);
template <class T> constexpr size_t item_complete_native_entities_sort_caller_source =
	// Both vector endpoint method receivers and actual explicit pointer ctors;
	// returned iterators are public sort's by-value formal destinations.
	6 * sizeof(void *);
template <class T> constexpr size_t item_complete_native_entities_adjacent_caller_source =
	// Three actual endpoint calls; result and RHS end are distinct I operands.
	// Rewritten != is equality with both simultaneous returned base refs.
	9 * sizeof(void *) + 2 * sizeof(typename std::vector<T>::iterator) + 6 * sizeof(void *) +
	sizeof(bool);
template <bool Fee> constexpr size_t item_complete_native_entities_key_algorithm_source =
	item_complete_native_entities_sort_two_common_source<critical_entity_key,
							     decltype(&critical_entity_key_less)> +
	item_complete_native_entities_key_less_target_source +
	(Fee ? item_complete_native_entities_fee_backward_extra_source<critical_entity_key> :
	       item_complete_native_entities_money_linear_extra_source<
		       critical_entity_key,
		       decltype(&critical_entity_key_less)>)+item_complete_native_entities_sort_caller_source<critical_entity_key> +
	// Conservatively distinct actual adjacency invocation. No borrowing of
	// ended sort credits or opaque broad-sort reservation.
	item_complete_native_entities_adjacent_source<critical_entity_key,
						      decltype(&critical_entity_key_equal)> +
	item_complete_native_entities_adjacent_caller_source<critical_entity_key>;
template <bool Fee> constexpr size_t item_complete_native_entities_revision_algorithm_source =
	item_complete_native_entities_sort_two_common_source<critical_expected_revision,
							     item_native_entities_revision_less> +
	item_complete_native_entities_revision_less_target_source +
	(Fee ? item_complete_native_entities_fee_backward_extra_source<critical_expected_revision> :
	       item_complete_native_entities_money_linear_extra_source<
		       critical_expected_revision,
		       item_native_entities_revision_less>)+item_complete_native_entities_sort_caller_source<critical_expected_revision>;
static_assert(std::is_trivial_v<critical_entity_key> &&
	      std::is_trivial_v<critical_expected_revision>);
} // namespace

constexpr size_t item_complete_native_entities_owned_declaration_source =
	// admission_prefix and zero-count for-loop index initializer are both reached.
	3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_native_entities_wrapper_declaration_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 2 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_native_entities_prefix_peak_source =
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_native_entities_fresh_source =
	sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_native_entities_sort_frame_source =
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_native_entities_owner_key_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_native_entities_owner_key_child_query_source =
	3 * item_owner_key_source_query_frame_bytes();

namespace
{
template <class T> constexpr size_t item_complete_native_entities_vector_current_source =
	// Actual payload_clone_vector_heap(false): value/bytes refs,fresh/count/result,
	// then selected capacity this/returnedN. size() true branch unselected.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_capacity_source<T>;
constexpr size_t item_complete_native_entities_unselected_byte_deallocation_tail_source =
	// Candidate payload/accounting_intent stay empty. Actual null _M_deallocate
	// reaches its root only; traits/allocator/newallocator and sized delete
	// descendants7P4NB are unselected for this byte cleanup (not K/R).
	7 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_native_entities_command_lifetime_source =
	item_complete_command_aggregate_default_source + item_complete_command_cleanup_source -
	item_complete_native_entities_unselected_byte_deallocation_tail_source +
	// Only two member-vector assignments are selected, not whole command MOVE.
	// Their real V/A locals stay in Source. Exact named typed cleanup is shared
	// with the candidate command cleanup; caller std::move is distinct per T.
	duris_craft_recipe_bounded_detail::complete_recipe_vector_move_selected_source<
		critical_entity_key> -
	duris_craft_recipe_bounded_detail::member_vector_cleanup_source<critical_entity_key>() +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_move_selected_source<
		critical_expected_revision> -
	duris_craft_recipe_bounded_detail::member_vector_cleanup_source<
		critical_expected_revision>() +
	4 * sizeof(void *);
constexpr size_t item_complete_native_entities_selected_controller_source =
	item_complete_native_entities_prefix_peak_source +
	// Two genuinely distinct fresh<T> and sort_frame<T,Comparator> methods.
	2 * item_complete_native_entities_fresh_source +
	2 * item_complete_native_entities_sort_frame_source + sizeof(void *) + sizeof(size_t) +
	sizeof(bool) + sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	item_complete_native_entities_vector_current_source<critical_entity_key> +
	item_complete_native_entities_vector_current_source<critical_expected_revision> +
	item_complete_native_entities_vector_current_source<uint8_t>;
constexpr size_t item_complete_native_entities_selected_direct_source =
	// policy-supported result; owner identity equality references/result;
	// key/revision size() roots (max-keys and sort arguments).
	sizeof(bool) + 2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t));
constexpr size_t item_complete_native_entities_common_retained_source =
	item_complete_native_entities_owned_declaration_source +
	item_complete_native_entities_wrapper_declaration_source +
	item_complete_native_entities_selected_controller_source +
	item_complete_native_entities_selected_direct_source +
	item_complete_native_entities_command_lifetime_source +
	item_complete_native_entities_fresh_assign_source<critical_entity_key> +
	item_complete_native_entities_fresh_assign_source<critical_expected_revision> +
	item_complete_native_entities_owner_key_child_own_source +
	item_complete_native_entities_owner_key_child_query_source;
constexpr size_t item_complete_native_fee_entities_retained_source =
	item_complete_native_entities_common_retained_source +
	item_complete_native_entities_key_algorithm_source<true> +
	item_complete_native_entities_revision_algorithm_source<true>;
constexpr size_t item_complete_native_money_entities_retained_source =
	item_complete_native_entities_common_retained_source +
	item_complete_native_entities_key_algorithm_source<false> +
	item_complete_native_entities_revision_algorithm_source<false>;
} // namespace

namespace
{
bool item_complete_native_entities_owner_key_child(const item_owner_identity &owner,
						   critical_entity_key *output,
						   bool (*reserve)(size_t, void *) noexcept,
						   void *context, size_t outer) noexcept
{
	constexpr size_t query = 3 * item_owner_key_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_owner_key_source_frame_bytes(&source) ||
	    !item_owner_key_initial_inline_bytes(&initial) ||
	    !item_owner_key_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_owner_key_bounded(owner, output, reserve, context, child_outer);
}

} // namespace

namespace
{
bool item_native_entities_populate_complete_owned(critical_command *command,
						  const item_transfer_payload &payload,
						  item_native_entities_complete_budget &budget)
{
	critical_entity_key from_key = {}, to_key = {};
	size_t admission_prefix = 0;
	if (!command ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_native_entities_owner_key_child(payload.from_owner, &from_key,
							    budget.reserve, budget.context,
							    admission_prefix)) ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_native_entities_owner_key_child(
		      payload.to_owner, &to_key, budget.reserve, budget.context, admission_prefix)))
		return false;
	if (!budget.fresh<critical_entity_key>(
		    2, 2 * sizeof(critical_entity_key) +
			       sizeof(std::initializer_list<critical_entity_key>)))
		return false;
	command->keys = { from_key, to_key };
	if (!budget.fresh<critical_expected_revision>(
		    2, 2 * sizeof(critical_expected_revision) +
			       sizeof(std::initializer_list<critical_expected_revision>)))
		return false;
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
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(item_key);
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back(
			{ item_key, payload.items[index].expected_item_revision });
	}
	if (payload.target_parent_item_uid)
	{
		critical_entity_key parent_key = { critical_entity_type::item,
						   payload.target_parent_item_uid };
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(parent_key);
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back(
			{ parent_key, payload.expected_target_parent_revision });
	}
	if (payload.reason == item_transfer_reason::craft)
	{
		if (!budget.peak(sizeof(std::vector<player_item_snapshot>) + 4 * sizeof(void *) +
				 sizeof(std::allocator<player_item_snapshot>)))
			return false;
		std::vector<player_item_snapshot> outputs;
		budget.outputs = &outputs;
		if (!(budget.prefix(admission_prefix) &&
		      item_transfer_craft_outputs_decode_bounded(payload, &outputs, budget.reserve,
								 budget.context, admission_prefix,
								 nullptr)))
			return false;
		for (const player_item_snapshot &output : outputs)
		{
			const critical_entity_key output_key = { critical_entity_type::item,
								 output.object_uid };
			if (!budget.growth(command->keys, 1))
				return false;
			command->keys.push_back(output_key);
			if (!budget.growth(command->expected_revisions, 1))
				return false;
			command->expected_revisions.push_back(
				{ output_key, ITEM_TRANSFER_ABSENT_REVISION });
		}
		budget.outputs = nullptr;
	}
	if (payload.collector.present)
	{
		const critical_entity_key catalog_key = { critical_entity_type::collector,
							  COLLECTOR_CATALOG_KEY };
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(catalog_key);
		// The SQL repository takes the current catalog row lock before any item
		// lock; zero is a serialization key, not an optimistic catalog fence.
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back({ catalog_key, 0 });
	}
	if (payload.native_recovery.present &&
	    payload.native_mobile.action == item_native_mobile_action::consumption &&
	    !payload.native_cost.fee_only)
	{
		const critical_entity_key giver_key = { critical_entity_type::player,
							payload.native_recovery.player_pid };
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(giver_key);
		// A serialization key only; the acknowledged save fence remains distinct
		// from a player custody-owner revision and is checked by the original root.
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back({ giver_key, 0 });
	}
	if (payload.native_recovery.present && command->keys.size() > CRITICAL_COMMAND_MAX_KEYS)
		return false;
	if (!budget.sort_frame<critical_entity_key, decltype(&critical_entity_key_less)>(
		    command->keys.size()))
		return false;
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	if (std::adjacent_find(command->keys.begin(), command->keys.end(),
			       critical_entity_key_equal) != command->keys.end())
		return false;
	if (!budget.sort_frame<critical_expected_revision, item_native_entities_revision_less>(
		    command->expected_revisions.size()))
		return false;
	std::sort(command->expected_revisions.begin(), command->expected_revisions.end(),
		  item_native_entities_revision_less{});
	return true;
}
} // namespace

namespace
{
bool item_complete_native_fee_entities_bounded(critical_command *command,
					       const item_transfer_payload &payload,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer_live) noexcept
{
	if (!command || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		item_complete_native_fee_entities_retained_source + 2 * sizeof(critical_entity_key);
	item_native_entities_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(critical_command)))
		return false;
	try
	{
		// Only original entity vectors are populated; authentic existing command
		// payload/accounting/scalars stay untouched and caller-owned throughout.
		critical_command candidate = {};
		budget.command = &candidate;
		if (!item_native_entities_populate_complete_owned(&candidate, payload, budget) ||
		    !budget.peak())
			return false;
		command->keys = std::move(candidate.keys);
		command->expected_revisions = std::move(candidate.expected_revisions);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
} // namespace

namespace
{
bool item_complete_native_money_entities_bounded(critical_command *command,
						 const item_transfer_payload &payload,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer_live) noexcept
{
	if (!command || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_native_money_entities_retained_source +
				  2 * sizeof(critical_entity_key);
	item_native_entities_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(critical_command)))
		return false;
	try
	{
		// Only original entity vectors are populated; authentic existing command
		// payload/accounting/scalars stay untouched and caller-owned throughout.
		critical_command candidate = {};
		budget.command = &candidate;
		if (!item_native_entities_populate_complete_owned(&candidate, payload, budget) ||
		    !budget.peak())
			return false;
		command->keys = std::move(candidate.keys);
		command->expected_revisions = std::move(candidate.expected_revisions);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
} // namespace

namespace
{
bool item_complete_native_fee_entities_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_fee_entities_retained_source, child = 0;
	if (!item_owner_key_source_frame_bytes(&child) || !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_complete_native_fee_entities_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_complete_native_fee_entities_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_native_entities_complete_budget);
	return true;
}
constexpr size_t item_complete_native_fee_entities_source_query_frame_bytes() noexcept
{
	// Full getter owns outputP/resultN/childN/returnedB, policyB,
	// genuine owner getter(P+B), checked-add(P+N+B). Other getters are dominated.
	return 3 * sizeof(void *) + 3 * sizeof(size_t) + 4 * sizeof(bool);
}
bool item_complete_native_fee_entities_entry_child(critical_command *command,
						   const item_transfer_payload &payload,
						   bool (*reserve)(size_t, void *) noexcept,
						   void *context, size_t outer) noexcept
{
	constexpr size_t query = 3 * item_complete_native_fee_entities_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_complete_native_fee_entities_source_frame_bytes(&source) ||
	    !item_complete_native_fee_entities_initial_inline_bytes(&initial) ||
	    !item_complete_native_fee_entities_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_complete_native_fee_entities_bounded(command, payload, reserve, context,
							 child_outer);
}
constexpr size_t item_complete_native_fee_entities_entry_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_native_fee_entities_entry_child_query_source =
	3 * item_complete_native_fee_entities_source_query_frame_bytes();
bool item_complete_native_money_entities_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_money_entities_retained_source, child = 0;
	if (!item_owner_key_source_frame_bytes(&child) || !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_complete_native_money_entities_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_complete_native_money_entities_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_native_entities_complete_budget);
	return true;
}
constexpr size_t item_complete_native_money_entities_source_query_frame_bytes() noexcept
{
	// Full getter owns outputP/resultN/childN/returnedB, policyB,
	// genuine owner getter(P+B), checked-add(P+N+B). Other getters are dominated.
	return 3 * sizeof(void *) + 3 * sizeof(size_t) + 4 * sizeof(bool);
}
bool item_complete_native_money_entities_entry_child(critical_command *command,
						     const item_transfer_payload &payload,
						     bool (*reserve)(size_t, void *) noexcept,
						     void *context, size_t outer) noexcept
{
	constexpr size_t query = 3 * item_complete_native_money_entities_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_complete_native_money_entities_source_frame_bytes(&source) ||
	    !item_complete_native_money_entities_initial_inline_bytes(&initial) ||
	    !item_complete_native_money_entities_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_complete_native_money_entities_bounded(command, payload, reserve, context,
							   child_outer);
}
constexpr size_t item_complete_native_money_entities_entry_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_native_money_entities_entry_child_query_source =
	3 * item_complete_native_money_entities_source_query_frame_bytes();
} // namespace

namespace
{
constexpr size_t item_complete_native_cost_value_entry_child_own_source =
	2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	1 * sizeof(bool);
constexpr size_t item_complete_native_cost_value_entry_child_query_source =
	3 * item_transfer_native_cost_value_valid_complete_source_query_frame_bytes();
constexpr size_t item_complete_native_fee_shape_entry_child_own_source =
	2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	2 * sizeof(bool);
constexpr size_t item_complete_native_fee_shape_entry_child_query_source =
	3 * item_transfer_native_fee_shape_valid_complete_source_query_frame_bytes();
constexpr size_t item_complete_native_money_shape_entry_child_own_source =
	2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	2 * sizeof(bool);
constexpr size_t item_complete_native_money_shape_entry_child_query_source =
	3 * item_transfer_native_money_shape_valid_complete_source_query_frame_bytes();
constexpr size_t item_complete_native_fee_encode_entry_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	2 * sizeof(bool);
constexpr size_t item_complete_native_fee_encode_entry_child_query_source =
	3 * item_transfer_native_fee_encode_complete_source_query_frame_bytes();
constexpr size_t item_complete_native_money_encode_entry_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	2 * sizeof(bool);
constexpr size_t item_complete_native_money_encode_entry_child_query_source =
	3 * item_transfer_native_money_encode_complete_source_query_frame_bytes();
constexpr size_t item_complete_native_recovery_encode_entry_child_own_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	1 * sizeof(bool);
constexpr size_t item_complete_native_recovery_encode_entry_child_query_source =
	3 * item_transfer_native_recovery_encode_complete_source_query_frame_bytes();
constexpr size_t item_complete_native_recovery_decode_entry_child_own_source =
	sizeof(std::span<const uint8_t>) + 3 * sizeof(void *) +
	sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) + 1 * sizeof(bool) +
	3 * sizeof(void *);
constexpr size_t item_complete_native_recovery_decode_entry_child_query_source =
	3 * item_transfer_native_recovery_decode_complete_source_query_frame_bytes();
bool item_complete_native_cost_value_entry_child(const item_transfer_payload &payload,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_cost_value_valid_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_cost_value_valid_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_cost_value_valid_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_cost_value_valid_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_cost_value_valid_complete_bounded(payload, reserve, context,
								      child_outer);
}
bool item_complete_native_fee_shape_entry_child(const item_transfer_payload &payload,
						bool acknowledged,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_fee_shape_valid_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_fee_shape_valid_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_fee_shape_valid_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_fee_shape_valid_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_fee_shape_valid_complete_bounded(payload, acknowledged, reserve,
								     context, child_outer);
}
bool item_complete_native_money_shape_entry_child(const item_transfer_payload &payload,
						  bool acknowledged,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_money_shape_valid_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_money_shape_valid_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_money_shape_valid_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_money_shape_valid_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_money_shape_valid_complete_bounded(
		payload, acknowledged, reserve, context, child_outer);
}
bool item_complete_native_fee_encode_entry_child(const item_transfer_payload &payload,
						 bool acknowledged, std::vector<uint8_t> *output,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_fee_encode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_fee_encode_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_fee_encode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_fee_encode_complete_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_fee_encode_complete_bounded(payload, acknowledged, output,
								reserve, context, child_outer);
}
bool item_complete_native_money_encode_entry_child(const item_transfer_payload &payload,
						   bool acknowledged, std::vector<uint8_t> *output,
						   bool (*reserve)(size_t, void *) noexcept,
						   void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_money_encode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_money_encode_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_money_encode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_money_encode_complete_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_money_encode_complete_bounded(payload, acknowledged, output,
								  reserve, context, child_outer);
}
bool item_complete_native_recovery_encode_entry_child(
	const item_native_mobile_recovery_context &value, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	size_t *retained_heap) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_recovery_encode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_recovery_encode_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_recovery_encode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_recovery_encode_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_recovery_encode_complete_bounded(
		value, output, reserve, context, child_outer, retained_heap);
}
bool item_complete_native_recovery_decode_entry_child(std::span<const uint8_t> bytes,
						      item_native_mobile_recovery_context *output,
						      bool (*reserve)(size_t, void *) noexcept,
						      void *context, size_t outer,
						      size_t *retained_heap) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_recovery_decode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_recovery_decode_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_recovery_decode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_recovery_decode_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_recovery_decode_complete_bounded(
		bytes, output, reserve, context, child_outer, retained_heap);
}

} // namespace

// Actual allocation-free raw money projection SOURCE and real INLINE.
// No bounded giver encoder/decoder Source is imported as a substitute.
namespace
{
constexpr size_t item_complete_native_money_project_own_source =
	// two input array refs/output ptr; two revision values; denomination,
	// quantity; copper/index/added locals and returned genuine result enum.
	3 * sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(uint8_t) + sizeof(int32_t) +
	2 * sizeof(int64_t) + sizeof(size_t) + sizeof(native_quest_coin_give_result);
constexpr size_t item_complete_native_money_counts_own_source =
	// cash/range refs, hidden raw const begin/end pointers, count, bool result.
	4 * sizeof(void *) + sizeof(int64_t) + sizeof(bool);
constexpr size_t item_complete_native_money_project_array_source =
	// Actual array<int64_t,4> const begin/end/data receiver and pointer result;
	// size receiver/result; mutable and const subscript this/index/ref result.
	// No iterator class, array_traits body, array assignment/default callee.
	3 * (2 * sizeof(void *)) + (sizeof(void *) + sizeof(size_t)) +
	2 * (2 * sizeof(void *) + sizeof(size_t));
constexpr size_t item_complete_native_money_project_callee_source =
	item_complete_native_money_project_own_source +
	item_complete_native_money_counts_own_source +
	item_complete_native_money_project_array_source +
	// Plain candidate's generated nontrivial default constructor receiver,
	// due actual DMIs. Scalar/fixed-array member operations have no call body.
	sizeof(void *);
constexpr size_t item_complete_native_money_project_inline =
	sizeof(std::array<int64_t, 4>) + sizeof(native_quest_coin_give_projection);
constexpr size_t item_complete_native_money_project_adapter_own_source =
	// Same project params plus real budget reference; no named local or
	// runtime constexpr accessor call. Actual peak/prefix descendants separate.
	4 * sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(uint8_t) + sizeof(int32_t) +
	sizeof(native_quest_coin_give_result);
native_quest_coin_give_result item_complete_native_money_project_child(
	const std::array<int64_t, 4> &player_before, uint64_t player_revision,
	const std::array<int64_t, 4> &mobile_before, uint64_t mobile_revision, uint8_t denomination,
	int32_t quantity, native_quest_coin_give_projection *output,
	item_native_shape_complete_budget &budget) noexcept
{
	// Re-admit actual raw candidate/units immediately before entry: an earlier
	// reference child has ended and may have issued its own smaller reservation.
	// Parent owns genuine callee+adapter Source; this is only prospective INLINE.
	if (!budget.peak(item_complete_native_money_project_inline))
		return native_quest_coin_give_result::allocation_failure;
	return native_quest_coin_give_project(player_before, player_revision, mobile_before,
					      mobile_revision, denomination, quantity, output);
}
} // namespace

namespace
{
constexpr size_t item_complete_native_fee_shape_retained_source =
	item_native_decl_source::item_complete_native_fee_shape_owned_own_source +
	item_native_decl_source::item_complete_native_fee_shape_wrapper_own_source +
	item_complete_native_fee_shape_controller_source +
	item_complete_native_fee_shape_lifetime_source +
	item_complete_native_fee_shape_direct_source +
	item_complete_native_fee_shape_helper_source +
	item_complete_native_reference_encode_child_own_source +
	item_complete_native_reference_encode_child_query_source +
	item_complete_native_forest_shape_adapter_source +
	3 * shop_trade_recovery_forest_shape_valid_bounded_source_query_frame_bytes() +
	item_complete_native_cost_value_entry_child_own_source +
	item_complete_native_cost_value_entry_child_query_source +
	item_complete_fee_reward_continuation_decode_child_own_source +
	item_complete_fee_reward_continuation_decode_child_query_source;
constexpr size_t item_complete_native_fee_encode_retained_source =
	item_native_decl_source::item_complete_native_fee_encode_owned_own_source +
	item_native_decl_source::item_complete_native_fee_encode_wrapper_own_source +
	item_complete_native_fee_money_encode_controller_source +
	item_complete_native_fee_money_encode_lifetime_source +
	item_complete_native_fee_encode_direct_source +
	item_complete_native_fee_shape_entry_child_own_source +
	item_complete_native_fee_shape_entry_child_query_source +
	item_complete_native_cost_encode_child_own_source +
	item_complete_native_cost_encode_child_query_source +
	item_complete_native_reference_encode_child_own_source +
	item_complete_native_reference_encode_child_query_source +
	item_complete_native_recovery_encode_entry_child_own_source +
	item_complete_native_recovery_encode_entry_child_query_source +
	item_complete_native_byte_header_fill_source -
	item_complete_native_byte_header_failure_cleanup_source +
	item_complete_native_byte_reference_insert_source +
	item_complete_native_byte_reference_insert_callers_source +
	item_complete_native_byte_mutable_end_insert_source +
	item_complete_native_byte_mutable_end_insert_callers_source +
	item_complete_native_byte_const_end_insert_source +
	item_complete_native_byte_const_end_insert_callers_source;
constexpr size_t item_complete_native_fee_decode_retained_source =
	item_native_decl_source::item_complete_native_fee_decode_owned_own_source +
	item_native_decl_source::item_complete_native_fee_decode_wrapper_own_source +
	item_complete_native_fee_decode_controller_source +
	item_complete_native_fee_decode_lifetime_source +
	item_complete_native_fee_decode_direct_source +
	item_complete_native_reference_decode_child_own_source +
	item_complete_native_reference_decode_child_span_call_source +
	item_complete_native_reference_decode_child_query_source +
	item_complete_native_cost_decode_child_own_source +
	item_complete_native_cost_decode_child_span_call_source +
	item_complete_native_cost_decode_child_query_source +
	item_complete_native_recovery_decode_entry_child_own_source +
	item_complete_native_recovery_decode_entry_child_query_source +
	item_complete_native_fee_encode_entry_child_own_source +
	item_complete_native_fee_encode_entry_child_query_source +
	item_complete_native_fee_entities_entry_child_own_source +
	item_complete_native_fee_entities_entry_child_query_source +
	item_complete_native_fee_money_canonical_equal_source +
	item_complete_native_keys_equal_source +
	item_complete_native_revisions_equal_source<item_native_fee_revision_equal> +
	item_complete_native_byte_raw_fresh_assign_source +
	item_complete_fee_reward_continuation_decode_child_own_source +
	item_complete_fee_reward_continuation_decode_child_query_source +
	item_complete_native_action_operation_equal_source;
constexpr size_t item_complete_native_money_shape_retained_source =
	item_native_decl_source::item_complete_native_money_shape_owned_own_source +
	item_native_decl_source::item_complete_native_money_shape_wrapper_own_source +
	item_complete_native_money_shape_controller_source +
	item_complete_native_money_shape_lifetime_source +
	item_complete_native_money_shape_direct_source +
	item_complete_native_money_shape_helper_source +
	item_complete_native_reference_encode_child_own_source +
	item_complete_native_reference_encode_child_query_source +
	item_complete_native_forest_shape_adapter_source +
	3 * shop_trade_recovery_forest_shape_valid_bounded_source_query_frame_bytes() +
	item_complete_native_money_project_callee_source +
	item_complete_native_money_project_adapter_own_source;
constexpr size_t item_complete_native_money_encode_retained_source =
	item_native_decl_source::item_complete_native_money_encode_owned_own_source +
	item_native_decl_source::item_complete_native_money_encode_wrapper_own_source +
	item_complete_native_fee_money_encode_controller_source +
	item_complete_native_fee_money_encode_lifetime_source +
	item_complete_native_money_encode_direct_source +
	item_complete_native_money_shape_entry_child_own_source +
	item_complete_native_money_shape_entry_child_query_source +
	item_complete_native_money_encode_child_own_source +
	item_complete_native_money_encode_child_query_source +
	item_complete_native_reference_encode_child_own_source +
	item_complete_native_reference_encode_child_query_source +
	item_complete_native_recovery_encode_entry_child_own_source +
	item_complete_native_recovery_encode_entry_child_query_source +
	item_complete_native_byte_header_fill_source -
	item_complete_native_byte_header_failure_cleanup_source +
	item_complete_native_byte_reference_insert_source +
	item_complete_native_byte_reference_insert_callers_source +
	item_complete_native_byte_mutable_end_insert_source +
	item_complete_native_byte_mutable_end_insert_callers_source;
constexpr size_t item_complete_native_money_decode_retained_source =
	item_native_decl_source::item_complete_native_money_decode_owned_own_source +
	item_native_decl_source::item_complete_native_money_decode_wrapper_own_source +
	item_complete_native_money_decode_controller_source +
	item_complete_native_money_decode_lifetime_source +
	item_complete_native_money_decode_direct_source +
	item_complete_native_reference_decode_child_own_source +
	item_complete_native_reference_decode_child_span_call_source +
	item_complete_native_reference_decode_child_query_source +
	item_complete_native_money_decode_child_own_source +
	item_complete_native_money_decode_child_span_call_source +
	item_complete_native_money_decode_child_query_source +
	item_complete_native_recovery_decode_entry_child_own_source +
	item_complete_native_recovery_decode_entry_child_query_source +
	item_complete_native_money_encode_entry_child_own_source +
	item_complete_native_money_encode_entry_child_query_source +
	item_complete_native_money_entities_entry_child_own_source +
	item_complete_native_money_entities_entry_child_query_source +
	item_complete_native_fee_money_canonical_equal_source +
	item_complete_native_keys_equal_source +
	item_complete_native_revisions_equal_source<item_native_money_revision_equal> +
	item_complete_native_money_range_span_source;
} // namespace

namespace
{
bool item_native_fee_shape_complete_owned(const item_transfer_payload &payload, bool acknowledged,
					  item_native_shape_complete_budget &budget)
{
	const auto &fee = payload.native_cost;
	const auto &native = payload.native_mobile;
	const auto &recovery = payload.native_recovery;
	size_t admission_prefix = 0;
	if (!fee.fee_only ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_native_cost_value_entry_child(payload, budget.reserve, budget.context,
							  admission_prefix)) ||
	    !native.present || native.action != item_native_mobile_action::consumption ||
	    !native.final_giver_pid || native.final_giver_pid > INT32_MAX ||
	    native.reference.mobile_revision == UINT64_MAX ||
	    payload.from_owner.type != item_owner_type::native_mobile ||
	    payload.from_owner.id != native.reference.mobile_instance_id ||
	    payload.from_owner.context_id || payload.to_owner.type != item_owner_type::player ||
	    payload.to_owner.id != native.final_giver_pid || payload.to_owner.context_id ||
	    payload.reason != item_transfer_reason::quest_turnin ||
	    payload.reason_id != native.reference.mobile_vnum || payload.logical_source_id ||
	    payload.multi_root || payload.item_count || payload.item_blob_size ||
	    payload.selected_item_uid || payload.target_root_item_uid ||
	    payload.target_parent_item_uid || payload.expected_target_parent_revision ||
	    payload.corpse.present || payload.collector.present ||
	    !valid_collector_context(payload, ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION) ||
	    payload.native_money.present || payload.native_money.original_room_vnum ||
	    payload.native_money.player_wallet_mapping_id ||
	    payload.native_money.mobile_wallet_mapping_id ||
	    payload.native_money.projection != native_quest_coin_give_projection{} ||
	    payload.expected_from_revision != native.reference.stock_revision ||
	    payload.expected_to_revision == UINT64_MAX || !recovery.consumed_root_order.empty())
		return false;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
	if (!budget.prefix(admission_prefix))
		return false;
	if (item_complete_native_reference_encode_child(
		    native.reference, &reference, budget.reserve, budget.context,
		    admission_prefix) != player_snapshot_codec_result::ok)
		return false;
	if (payload.continuation.kind == item_transfer_continuation_kind::quest_offering)
	{
		if (!budget.peak(sizeof(quest_reward_continuation)))
			return false;
		quest_reward_continuation terms;
		budget.terms = &terms;
		if (!budget.prefix(admission_prefix))
			return false;
		if (!item_complete_fee_reward_continuation_decode_child(
			    payload.continuation.data.data(), payload.continuation.data.size(),
			    &terms, budget.reserve, budget.context, admission_prefix) ||
		    terms.player_pid != native.final_giver_pid ||
		    terms.mobile_vnum != static_cast<uint32_t>(native.reference.mobile_vnum) ||
		    terms.action_mobile_instance_id != native.reference.mobile_instance_id ||
		    terms.completion_index != fee.completion_slot ||
		    terms.action_source.generation.bytes !=
			    native.reference.birth_source.generation.bytes ||
		    terms.action_source.sequence != native.reference.mobile_revision)
			return false;
		budget.terms = nullptr;
	}
	else if (payload.continuation.kind != item_transfer_continuation_kind::none ||
		 !payload.continuation.data.empty())
		return false;
	if (!acknowledged)
		return !recovery.present && !recovery.player_pid &&
		       !recovery.acknowledged_save_revision &&
		       recovery.player_before == shop_trade_recovery_forest_binding{} &&
		       recovery.player_after == shop_trade_recovery_forest_binding{} &&
		       native_publication_terms_empty(recovery.publication_terms);
	return recovery.present && recovery.player_pid == native.final_giver_pid &&
	       recovery.acknowledged_save_revision &&
	       native_publication_terms_valid(recovery.publication_terms) &&
	       !recovery.publication_terms.disappear &&
	       recovery.player_before.canonical_bytes == recovery.player_after.canonical_bytes &&
	       recovery.player_before.ordered_item_uids ==
		       recovery.player_after.ordered_item_uids &&
	       (budget.prefix(admission_prefix) &&
		item_complete_native_forest_shape_child(
			recovery.player_before, shop_trade_recovery_forest_role::player_before,
			budget.reserve, budget.context, admission_prefix)) &&
	       (budget.prefix(admission_prefix) &&
		item_complete_native_forest_shape_child(
			recovery.player_after, shop_trade_recovery_forest_role::player_after,
			budget.reserve, budget.context, admission_prefix));
}
} // namespace

namespace
{
bool item_encode_native_fee_complete_owned(const item_transfer_payload &payload, bool acknowledged,
					   std::vector<uint8_t> *output,
					   item_native_wire_encode_complete_budget &budget)
{
	size_t admission_prefix = 0;
	if (!output ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_native_fee_shape_entry_child(payload, acknowledged, budget.reserve,
							 budget.context, admission_prefix)))
		return false;
	if (!budget.peak(2 * sizeof(std::vector<uint8_t>)))
		return false;
	std::vector<uint8_t> cost, recovery;
	budget.first = &cost;
	budget.second = &recovery;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
	if (!budget.prefix(admission_prefix))
		return false;
	if (item_complete_native_cost_encode_child(
		    payload.native_cost.projection, &cost, budget.reserve, budget.context,
		    admission_prefix) != native_quest_cost_projection_result::ok ||
	    (!budget.prefix(admission_prefix) ?
		     player_snapshot_codec_result::invalid_value :
		     item_complete_native_reference_encode_child(
			     payload.native_mobile.reference, &reference, budget.reserve,
			     budget.context, admission_prefix)) !=
		    player_snapshot_codec_result::ok ||
	    (acknowledged && !(budget.prefix(admission_prefix) &&
			       item_complete_native_recovery_encode_entry_child(
				       payload.native_recovery, &recovery, budget.reserve,
				       budget.context, admission_prefix, nullptr))))
		return false;
	const size_t bytes = native_fee_header_bytes + reference.size() + cost.size() +
			     payload.continuation.data.size() + recovery.size();
	if (bytes > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	if (!budget.peak(sizeof(std::vector<uint8_t>) + native_fee_header_bytes))
		return false;
	std::vector<uint8_t> value(native_fee_header_bytes, 0);
	budget.value = &value;
	value[0] = 'N';
	value[1] = 'Q';
	value[2] = 'F';
	value[3] = '2';
	put_u16(value.data() + 4, 1);
	put_u16(value.data() + 6,
		acknowledged ? ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION :
			       ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION);
	put_u32(value.data() + 8, static_cast<uint32_t>(bytes));
	put_u32(value.data() + 12, static_cast<uint32_t>(cost.size()));
	put_u32(value.data() + 16, static_cast<uint32_t>(payload.continuation.data.size()));
	put_u32(value.data() + 20, static_cast<uint32_t>(recovery.size()));
	put_u32(value.data() + 24, payload.native_mobile.final_giver_pid);
	put_u32(value.data() + 28, payload.native_cost.completion_slot);
	put_u64(value.data() + 32, payload.expected_from_revision);
	put_u64(value.data() + 40, payload.expected_to_revision);
	put_u64(value.data() + 48, payload.native_cost.wallet_mapping_id);
	put_u16(value.data() + 56, static_cast<uint16_t>(payload.continuation.kind));
	if (!budget.growth(value, reference.size()))
		return false;
	value.insert(value.end(), reference.begin(), reference.end());
	if (!budget.growth(value, cost.size()))
		return false;
	value.insert(value.end(), cost.begin(), cost.end());
	if (!budget.growth(value, payload.continuation.data.size()))
		return false;
	value.insert(value.end(), payload.continuation.data.begin(),
		     payload.continuation.data.end());
	if (!budget.growth(value, recovery.size()))
		return false;
	value.insert(value.end(), recovery.begin(), recovery.end());
	if (!budget.peak())
		return false;
	*output = std::move(value);
	return true;
}
} // namespace

namespace
{
bool item_decode_native_fee_complete_owned(const critical_command &command,
					   item_transfer_payload *output,
					   item_native_wire_decode_complete_budget &budget)
{
	constexpr size_t fixed = native_fee_header_bytes + QUEST_MOBILE_NATIVE_REFERENCE_BYTES;
	if (!output || !native_cost_version(command.payload_version) ||
	    command.type != critical_command_type::item_transfer ||
	    command.payload.size() < fixed ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	const auto *wire = command.payload.data();
	const bool acknowledged = command.payload_version ==
				  ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION;
	if (wire[0] != 'N' || wire[1] != 'Q' || wire[2] != 'F' || wire[3] != '2' ||
	    get_u16(wire + 4) != 1 || get_u16(wire + 6) != command.payload_version ||
	    get_u32(wire + 8) != command.payload.size() || get_u16(wire + 58) || get_u32(wire + 60))
		return false;
	const size_t cost_size = get_u32(wire + 12), continuation_size = get_u32(wire + 16),
		     recovery_size = get_u32(wire + 20);
	size_t remaining = command.payload.size() - fixed;
	if (cost_size > remaining)
		return false;
	remaining -= cost_size;
	if (continuation_size > remaining ||
	    continuation_size > ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
		return false;
	remaining -= continuation_size;
	if (recovery_size != remaining || (!acknowledged && recovery_size))
		return false;
	if (!budget.peak(sizeof(item_transfer_payload)))
		return false;
	item_transfer_payload value{};
	budget.payload = &value;
	size_t admission_prefix = 0;
	if (!budget.prefix(admission_prefix))
		return false;
	value.native_mobile.present = true;
	value.native_mobile.action = item_native_mobile_action::consumption;
	value.native_mobile.final_giver_pid = get_u32(wire + 24);
	value.native_cost.present = value.native_cost.fee_only = true;
	value.native_cost.completion_slot = get_u32(wire + 28);
	value.native_cost.wallet_mapping_id = get_u64(wire + 48);
	value.expected_from_revision = get_u64(wire + 32);
	value.expected_to_revision = get_u64(wire + 40);
	if (item_complete_native_reference_decode_child(
		    { wire + native_fee_header_bytes, QUEST_MOBILE_NATIVE_REFERENCE_BYTES },
		    &value.native_mobile.reference, budget.reserve, budget.context,
		    admission_prefix) != player_snapshot_codec_result::ok ||
	    (!budget.prefix(admission_prefix) ?
		     native_quest_cost_projection_result::invalid :
		     item_complete_native_cost_decode_child(
			     { wire + fixed, cost_size }, &value.native_cost.projection,
			     budget.reserve, budget.context, admission_prefix)) !=
		    native_quest_cost_projection_result::ok)
		return false;
	value.continuation.kind = static_cast<item_transfer_continuation_kind>(get_u16(wire + 56));
	if (!budget.peak(continuation_size))
		return false;
	value.continuation.data.assign(wire + fixed + cost_size,
				       wire + fixed + cost_size + continuation_size);
	if (acknowledged &&
	    !(budget.prefix(admission_prefix) &&
	      item_complete_native_recovery_decode_entry_child(
		      { wire + fixed + cost_size + continuation_size, recovery_size },
		      &value.native_recovery, budget.reserve, budget.context, admission_prefix,
		      nullptr)))
		return false;
	value.from_owner = { item_owner_type::native_mobile,
			     value.native_mobile.reference.mobile_instance_id, 0 };
	value.to_owner = { item_owner_type::player, value.native_mobile.final_giver_pid, 0 };
	value.reason = item_transfer_reason::quest_turnin;
	value.reason_id = value.native_mobile.reference.mobile_vnum;
	if (!budget.peak(sizeof(std::vector<uint8_t>) + sizeof(critical_command)))
		return false;
	std::vector<uint8_t> canonical;
	critical_command expected{};
	budget.canonical = &canonical;
	budget.expected = &expected;
	if (!(budget.prefix(admission_prefix) &&
	      item_complete_native_fee_encode_entry_child(value, acknowledged, &canonical,
							  budget.reserve, budget.context,
							  admission_prefix)) ||
	    (!budget.peak() || canonical != command.payload) ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_native_fee_entities_entry_child(&expected, value, budget.reserve,
							    budget.context, admission_prefix)) ||
	    command.keys.size() != expected.keys.size() ||
	    command.expected_revisions.size() != expected.expected_revisions.size() ||
	    !budget.peak() ||
	    !std::equal(command.keys.begin(), command.keys.end(), expected.keys.begin(),
			critical_entity_key_equal) ||
	    !std::equal(command.expected_revisions.begin(), command.expected_revisions.end(),
			expected.expected_revisions.begin(), item_native_fee_revision_equal{}))
		return false;
	if (value.continuation.kind == item_transfer_continuation_kind::quest_offering)
	{
		if (!budget.peak(sizeof(quest_reward_continuation)))
			return false;
		quest_reward_continuation terms;
		budget.terms = &terms;
		if (!budget.prefix(admission_prefix))
			return false;
		if (!item_complete_fee_reward_continuation_decode_child(
			    value.continuation.data.data(), value.continuation.data.size(), &terms,
			    budget.reserve, budget.context, admission_prefix) ||
		    terms.action_operation.bytes != command.operation_id.bytes)
			return false;
		budget.terms = nullptr;
	}
	if (!budget.peak())
		return false;
	*output = std::move(value);
	return true;
}
} // namespace

namespace
{
bool item_native_money_shape_complete_owned(const item_transfer_payload &payload, bool acknowledged,
					    item_native_shape_complete_budget &budget)
{
	const auto &money = payload.native_money;
	const auto &native = payload.native_mobile;
	const auto &recovery = payload.native_recovery;
	size_t admission_prefix = 0;
	if (!money.present || money.original_room_vnum < 0 || !money.player_wallet_mapping_id ||
	    !money.mobile_wallet_mapping_id ||
	    money.player_wallet_mapping_id == money.mobile_wallet_mapping_id ||
	    payload.native_cost.present || payload.native_cost.fee_only ||
	    payload.native_cost.completion_slot || payload.native_cost.wallet_mapping_id ||
	    payload.native_cost.projection != native_quest_cost_projection{} || !native.present ||
	    native.action != item_native_mobile_action::acceptance || !native.final_giver_pid ||
	    native.final_giver_pid > INT32_MAX ||
	    payload.from_owner.type != item_owner_type::player ||
	    payload.from_owner.id != native.final_giver_pid || payload.from_owner.context_id ||
	    payload.to_owner.type != item_owner_type::native_mobile ||
	    payload.to_owner.id != native.reference.mobile_instance_id ||
	    payload.to_owner.context_id || payload.reason != item_transfer_reason::player_give ||
	    payload.reason_id != native.reference.mobile_vnum || payload.logical_source_id ||
	    payload.multi_root || payload.item_count || payload.item_blob_size ||
	    payload.selected_item_uid || payload.target_root_item_uid ||
	    payload.target_parent_item_uid || payload.expected_target_parent_revision ||
	    payload.corpse.present || payload.collector.present ||
	    !valid_collector_context(payload, ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION) ||
	    payload.continuation.kind != item_transfer_continuation_kind::none ||
	    !payload.continuation.data.empty() || payload.expected_from_revision == UINT64_MAX ||
	    payload.expected_to_revision != native.reference.stock_revision ||
	    !native_publication_terms_empty(recovery.publication_terms) ||
	    !recovery.consumed_root_order.empty())
		return false;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> encoded{};
	native_quest_coin_give_projection projected;
	if (!budget.prefix(admission_prefix) || !budget.peak())
		return false;
	if (item_complete_native_reference_encode_child(native.reference, &encoded, budget.reserve,
							budget.context, admission_prefix) !=
		    player_snapshot_codec_result::ok ||
	    native.reference.mobile_revision == UINT64_MAX ||
	    item_complete_native_money_project_child(
		    money.projection.player_before, money.projection.player_before_revision,
		    money.projection.mobile_before, money.projection.mobile_before_revision,
		    money.projection.denomination, money.projection.quantity, &projected,
		    budget) != native_quest_coin_give_result::ok ||
	    projected != money.projection)
		return false;
	if (!acknowledged)
		return !recovery.present && !recovery.player_pid &&
		       !recovery.acknowledged_save_revision &&
		       recovery.player_before == shop_trade_recovery_forest_binding{} &&
		       recovery.player_after == shop_trade_recovery_forest_binding{};
	return recovery.present && recovery.player_pid == native.final_giver_pid &&
	       recovery.acknowledged_save_revision && recovery.player_before.present &&
	       recovery.player_after.present &&
	       recovery.player_before.canonical_bytes == recovery.player_after.canonical_bytes &&
	       recovery.player_before.ordered_item_uids ==
		       recovery.player_after.ordered_item_uids &&
	       (budget.prefix(admission_prefix) &&
		item_complete_native_forest_shape_child(
			recovery.player_before, shop_trade_recovery_forest_role::player_before,
			budget.reserve, budget.context, admission_prefix)) &&
	       (budget.prefix(admission_prefix) &&
		item_complete_native_forest_shape_child(
			recovery.player_after, shop_trade_recovery_forest_role::player_after,
			budget.reserve, budget.context, admission_prefix));
}
} // namespace

namespace
{
bool item_encode_native_money_complete_owned(const item_transfer_payload &payload,
					     bool acknowledged, std::vector<uint8_t> *output,
					     item_native_wire_encode_complete_budget &budget)
{
	size_t admission_prefix = 0;
	if (!output ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_native_money_shape_entry_child(payload, acknowledged, budget.reserve,
							   budget.context, admission_prefix)))
		return false;
	if (!budget.peak(2 * sizeof(std::vector<uint8_t>)))
		return false;
	std::vector<uint8_t> money, recovery;
	budget.first = &money;
	budget.second = &recovery;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference{};
	if (!budget.prefix(admission_prefix))
		return false;
	if (item_complete_native_money_encode_child(
		    payload.native_money.projection, &money, budget.reserve, budget.context,
		    admission_prefix) != native_quest_coin_give_result::ok ||
	    (!budget.prefix(admission_prefix) ?
		     player_snapshot_codec_result::invalid_value :
		     item_complete_native_reference_encode_child(
			     payload.native_mobile.reference, &reference, budget.reserve,
			     budget.context, admission_prefix)) !=
		    player_snapshot_codec_result::ok ||
	    (acknowledged && !(budget.prefix(admission_prefix) &&
			       item_complete_native_recovery_encode_entry_child(
				       payload.native_recovery, &recovery, budget.reserve,
				       budget.context, admission_prefix, nullptr))))
		return false;
	const size_t bytes = ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES + reference.size() +
			     money.size() + recovery.size();
	if (bytes > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	if (!budget.peak(sizeof(std::vector<uint8_t>) +
			 ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES))
		return false;
	std::vector<uint8_t> result(ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES, 0);
	budget.value = &result;
	result[0] = 'N';
	result[1] = 'Q';
	result[2] = 'M';
	result[3] = '1';
	put_u16(result.data() + 4, 1);
	put_u16(result.data() + 6,
		acknowledged ? ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION :
			       ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION);
	put_u32(result.data() + 8, static_cast<uint32_t>(bytes));
	put_u32(result.data() + 12, static_cast<uint32_t>(recovery.size()));
	put_u32(result.data() + 16, payload.native_mobile.final_giver_pid);
	put_u32(result.data() + 20, static_cast<uint32_t>(payload.native_money.original_room_vnum));
	put_u64(result.data() + 24, payload.expected_from_revision);
	put_u64(result.data() + 32, payload.expected_to_revision);
	put_u64(result.data() + 40, payload.native_money.player_wallet_mapping_id);
	put_u64(result.data() + 48, payload.native_money.mobile_wallet_mapping_id);
	if (!budget.growth(result, reference.size()))
		return false;
	result.insert(result.end(), reference.begin(), reference.end());
	if (!budget.growth(result, money.size()))
		return false;
	result.insert(result.end(), money.begin(), money.end());
	if (!budget.growth(result, recovery.size()))
		return false;
	result.insert(result.end(), recovery.begin(), recovery.end());
	if (!budget.peak())
		return false;
	*output = std::move(result);
	return true;
}
} // namespace

namespace
{
bool item_decode_native_money_complete_owned(const critical_command &command,
					     item_transfer_payload *output,
					     item_native_wire_decode_complete_budget &budget)
{
	constexpr size_t fixed = ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES +
				 QUEST_MOBILE_NATIVE_REFERENCE_BYTES + NATIVE_QUEST_COIN_GIVE_BYTES;
	if (!output || !native_money_version(command.payload_version) ||
	    command.type != critical_command_type::item_transfer ||
	    command.payload.size() < fixed ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	const auto *wire = command.payload.data();
	const bool acknowledged = command.payload_version ==
				  ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION;
	if (wire[0] != 'N' || wire[1] != 'Q' || wire[2] != 'M' || wire[3] != '1' ||
	    get_u16(wire + 4) != 1 || get_u16(wire + 6) != command.payload_version ||
	    get_u32(wire + 8) != command.payload.size() || get_u32(wire + 20) > INT32_MAX ||
	    get_u64(wire + 56) || get_u32(wire + 12) != command.payload.size() - fixed ||
	    (!acknowledged && command.payload.size() != fixed))
		return false;
	if (!budget.peak(sizeof(item_transfer_payload)))
		return false;
	item_transfer_payload candidate{};
	budget.payload = &candidate;
	size_t admission_prefix = 0;
	if (!budget.prefix(admission_prefix))
		return false;
	candidate.native_mobile.present = true;
	candidate.native_mobile.action = item_native_mobile_action::acceptance;
	candidate.native_mobile.final_giver_pid = get_u32(wire + 16);
	candidate.expected_from_revision = get_u64(wire + 24);
	candidate.expected_to_revision = get_u64(wire + 32);
	candidate.native_money.present = true;
	candidate.native_money.original_room_vnum = static_cast<int32_t>(get_u32(wire + 20));
	candidate.native_money.player_wallet_mapping_id = get_u64(wire + 40);
	candidate.native_money.mobile_wallet_mapping_id = get_u64(wire + 48);
	if (item_complete_native_reference_decode_child(
		    { wire + ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES,
		      QUEST_MOBILE_NATIVE_REFERENCE_BYTES },
		    &candidate.native_mobile.reference, budget.reserve, budget.context,
		    admission_prefix) != player_snapshot_codec_result::ok ||
	    (!budget.prefix(admission_prefix) ?
		     native_quest_coin_give_result::invalid :
		     item_complete_native_money_decode_child(
			     { wire + ITEM_TRANSFER_NATIVE_MOBILE_MONEY_HEADER_BYTES +
				       QUEST_MOBILE_NATIVE_REFERENCE_BYTES,
			       NATIVE_QUEST_COIN_GIVE_BYTES },
			     &candidate.native_money.projection, budget.reserve, budget.context,
			     admission_prefix)) != native_quest_coin_give_result::ok ||
	    (acknowledged && !(budget.prefix(admission_prefix) &&
			       item_complete_native_recovery_decode_entry_child(
				       std::span<const uint8_t>(command.payload).subspan(fixed),
				       &candidate.native_recovery, budget.reserve, budget.context,
				       admission_prefix, nullptr))))
		return false;
	candidate.from_owner = { item_owner_type::player, candidate.native_mobile.final_giver_pid,
				 0 };
	candidate.to_owner = { item_owner_type::native_mobile,
			       candidate.native_mobile.reference.mobile_instance_id, 0 };
	candidate.reason = item_transfer_reason::player_give;
	candidate.reason_id = candidate.native_mobile.reference.mobile_vnum;
	if (!budget.peak(sizeof(std::vector<uint8_t>) + sizeof(critical_command)))
		return false;
	std::vector<uint8_t> canonical;
	critical_command expected{};
	budget.canonical = &canonical;
	budget.expected = &expected;
	if (!(budget.prefix(admission_prefix) &&
	      item_complete_native_money_encode_entry_child(candidate, acknowledged, &canonical,
							    budget.reserve, budget.context,
							    admission_prefix)) ||
	    (!budget.peak() || canonical != command.payload) ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_native_money_entities_entry_child(&expected, candidate, budget.reserve,
							      budget.context, admission_prefix)) ||
	    command.keys.size() != expected.keys.size() ||
	    command.expected_revisions.size() != expected.expected_revisions.size() ||
	    !budget.peak() ||
	    !std::equal(command.keys.begin(), command.keys.end(), expected.keys.begin(),
			critical_entity_key_equal) ||
	    !std::equal(command.expected_revisions.begin(), command.expected_revisions.end(),
			expected.expected_revisions.begin(), item_native_money_revision_equal{}))
		return false;
	if (!budget.peak())
		return false;
	*output = std::move(candidate);
	return true;
}
} // namespace

bool item_transfer_native_fee_shape_valid_complete_bounded(const item_transfer_payload &payload,
							   bool acknowledged,
							   bool (*reserve)(size_t, void *) noexcept,
							   void *context,
							   size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_native_fee_shape_retained_source +
				  sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
				  2 * sizeof(shop_trade_recovery_forest_binding) +
				  sizeof(native_quest_coin_give_projection);
	item_native_shape_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_native_fee_shape_complete_owned(payload, acknowledged, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_fee_encode_complete_bounded(const item_transfer_payload &payload,
						      bool acknowledged,
						      std::vector<uint8_t> *encoded,
						      bool (*reserve)(size_t, void *) noexcept,
						      void *context, size_t outer_live) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_native_fee_encode_retained_source +
				  sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
				  item_complete_native_byte_header_zero_inline_bytes;
	item_native_wire_encode_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_encode_native_fee_complete_owned(payload, acknowledged, encoded,
							     budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_fee_decode_complete_bounded(const critical_command &command,
						      item_transfer_payload *payload,
						      bool (*reserve)(size_t, void *) noexcept,
						      void *context, size_t outer_live) noexcept
{
	if (!payload || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		item_complete_native_fee_decode_retained_source + 2 * sizeof(item_owner_identity);
	item_native_wire_decode_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_decode_native_fee_complete_owned(command, payload, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_money_shape_valid_complete_bounded(
	const item_transfer_payload &payload, bool acknowledged,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_native_money_shape_retained_source +
				  sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
				  2 * sizeof(shop_trade_recovery_forest_binding) +
				  sizeof(native_quest_cost_projection) +
				  sizeof(native_quest_coin_give_projection);
	item_native_shape_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_native_money_shape_complete_owned(payload, acknowledged, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_money_encode_complete_bounded(const item_transfer_payload &payload,
							bool acknowledged,
							std::vector<uint8_t> *encoded,
							bool (*reserve)(size_t, void *) noexcept,
							void *context, size_t outer_live) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_native_money_encode_retained_source +
				  sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
				  item_complete_native_byte_header_zero_inline_bytes;
	item_native_wire_encode_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_encode_native_money_complete_owned(payload, acknowledged, encoded,
							       budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_money_decode_complete_bounded(const critical_command &command,
							item_transfer_payload *payload,
							bool (*reserve)(size_t, void *) noexcept,
							void *context, size_t outer_live) noexcept
{
	if (!payload || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_native_money_decode_retained_source +
				  2 * sizeof(item_owner_identity) +
				  item_complete_native_money_range_span_inline;
	item_native_wire_decode_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_decode_native_money_complete_owned(command, payload, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_fee_shape_valid_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_fee_shape_retained_source, child = 0;
	if (!item_transfer_native_cost_value_valid_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!quest_mobile_native_reference_encode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!quest_fee_reward_continuation_decode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!shop_trade_recovery_forest_shape_valid_bounded_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_native_fee_shape_valid_complete_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_native_fee_shape_valid_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_native_shape_complete_budget);
	return true;
}
bool item_transfer_native_money_shape_valid_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_money_shape_retained_source, child = 0;
	if (!quest_mobile_native_reference_encode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!shop_trade_recovery_forest_shape_valid_bounded_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_native_money_shape_valid_complete_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_native_money_shape_valid_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_native_shape_complete_budget);
	return true;
}
bool item_transfer_native_fee_encode_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_fee_encode_retained_source, child = 0;
	if (!item_transfer_native_fee_shape_valid_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!native_quest_cost_projection_encode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!quest_mobile_native_reference_encode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!item_transfer_native_recovery_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_native_fee_encode_complete_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_native_fee_encode_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_native_wire_encode_complete_budget);
	return true;
}
bool item_transfer_native_money_encode_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_money_encode_retained_source, child = 0;
	if (!item_transfer_native_money_shape_valid_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!native_quest_coin_give_encode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!quest_mobile_native_reference_encode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!item_transfer_native_recovery_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_native_money_encode_complete_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_native_money_encode_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_native_wire_encode_complete_budget);
	return true;
}
bool item_transfer_native_fee_decode_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_fee_decode_retained_source, child = 0;
	if (!quest_mobile_native_reference_decode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!native_quest_cost_projection_decode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!item_transfer_native_recovery_decode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!item_transfer_native_fee_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!item_complete_native_fee_entities_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!quest_fee_reward_continuation_decode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_native_fee_decode_complete_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_native_fee_decode_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_native_wire_decode_complete_budget);
	return true;
}
bool item_transfer_native_money_decode_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_money_decode_retained_source, child = 0;
	if (!quest_mobile_native_reference_decode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!native_quest_coin_give_decode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!item_transfer_native_recovery_decode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!item_transfer_native_money_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	if (!item_complete_native_money_entities_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_native_money_decode_complete_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_native_money_decode_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_native_wire_decode_complete_budget);
	return true;
}

#else
bool item_transfer_native_fee_shape_valid_complete_bounded(const item_transfer_payload &, bool,
							   bool (*)(size_t, void *) noexcept,
							   void *, size_t) noexcept
{
	return false;
}
bool item_transfer_native_fee_encode_complete_bounded(const item_transfer_payload &, bool,
						      std::vector<uint8_t> *,
						      bool (*)(size_t, void *) noexcept, void *,
						      size_t) noexcept
{
	return false;
}
bool item_transfer_native_fee_decode_complete_bounded(const critical_command &,
						      item_transfer_payload *,
						      bool (*)(size_t, void *) noexcept, void *,
						      size_t) noexcept
{
	return false;
}
bool item_transfer_native_money_shape_valid_complete_bounded(const item_transfer_payload &, bool,
							     bool (*)(size_t, void *) noexcept,
							     void *, size_t) noexcept
{
	return false;
}
bool item_transfer_native_money_encode_complete_bounded(const item_transfer_payload &, bool,
							std::vector<uint8_t> *,
							bool (*)(size_t, void *) noexcept, void *,
							size_t) noexcept
{
	return false;
}
bool item_transfer_native_money_decode_complete_bounded(const critical_command &,
							item_transfer_payload *,
							bool (*)(size_t, void *) noexcept, void *,
							size_t) noexcept
{
	return false;
}
bool item_transfer_native_fee_shape_valid_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_fee_shape_valid_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_fee_shape_valid_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_fee_encode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_fee_encode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_fee_encode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_fee_decode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_fee_decode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_fee_decode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_money_shape_valid_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_money_shape_valid_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_money_shape_valid_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_money_encode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_money_encode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_money_encode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_money_decode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_money_decode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_money_decode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
#endif

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) && \
	!defined(_GLIBCXX_CONCEPT_CHECKS)
namespace
{
struct item_sidecar_complete_budget;
struct item_entities_revision_less;
struct item_general_entities_complete_budget;
struct item_payload_validation_complete_budget;
struct item_complete_validation_ancestor_predicate;
struct item_generic_decode_complete_budget;
struct item_payload_wire_complete_budget;
struct item_complete_generic_key_equal;
struct item_complete_generic_revision_equal;
struct item_public_dispatch_complete_budget;
struct item_complete_collector_uid_present;
struct item_complete_text_printable_predicate;
bool item_sidecar_complete_append_u32(item_sidecar_complete_budget &owner,
				      std::vector<uint8_t> *output, uint32_t value);
bool item_sidecar_complete_append_text(item_sidecar_complete_budget &owner,
				       std::vector<uint8_t> *output, const std::string &value);
bool item_sidecar_complete_read_text(item_sidecar_complete_budget &owner, const uint8_t *input,
				     size_t size, size_t *offset, size_t maximum,
				     std::string *value);
bool item_sidecar_complete_encode_corpse_context(const item_corpse_metadata &corpse,
						 std::vector<uint8_t> *encoded,
						 item_sidecar_complete_budget &owner);
bool item_sidecar_complete_decode_corpse_context(const uint8_t *encoded, size_t size,
						 item_corpse_metadata *corpse,
						 item_sidecar_complete_budget &owner);
bool item_sidecar_complete_encode_collector_context(const item_collector_death_enrollment &collector,
						    std::vector<uint8_t> *encoded,
						    item_sidecar_complete_budget &owner);
bool item_sidecar_complete_decode_collector_context(const uint8_t *encoded, size_t size,
						    item_collector_death_enrollment *collector,
						    item_sidecar_complete_budget &owner);
bool item_continuation_duplicate_valid_complete_owned(const item_transfer_payload &payload,
						      uint16_t payload_version,
						      item_native_validation_complete_budget &owner);
bool item_continuation_craft_outputs_complete_owned(const item_transfer_payload &payload,
						    std::vector<player_item_snapshot> *outputs,
						    item_native_validation_complete_budget &owner);
bool item_complete_general_validated_entities_source_frame_bytes(size_t *output) noexcept;
bool item_complete_general_validated_entities_source_supplement_frame_bytes(size_t *output) noexcept;
bool item_complete_general_validated_entities_initial_inline_bytes(size_t *output) noexcept;
bool item_complete_general_craft_outputs_child(const item_transfer_payload &input,
					       std::vector<player_item_snapshot> *output,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer,
					       size_t *retained_heap) noexcept;
bool item_complete_general_entities_owner_key_child(const item_owner_identity &owner,
						    critical_entity_key *output,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer) noexcept;
bool item_complete_general_validated_entities_bounded(critical_command *command,
						      const item_transfer_payload &payload,
						      bool (*reserve)(size_t, void *) noexcept,
						      void *context, size_t outer_live) noexcept;
bool item_general_entities_populate_complete_owned(critical_command *command,
						   const item_transfer_payload &payload,
						   item_general_entities_complete_budget &budget);
bool item_full_payload_validation_complete_owned(const item_transfer_payload &payload,
						 uint16_t payload_version,
						 item_payload_validation_complete_budget &budget);
bool item_complete_general_mobile_valid_child(const item_transfer_payload &input, uint16_t version,
					      bool (*reserve)(size_t, void *) noexcept,
					      void *context, size_t outer) noexcept;
bool item_complete_general_recovery_valid_child(const item_transfer_payload &input,
						const item_native_mobile_recovery_context &recovery,
						uint16_t version,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer) noexcept;
bool item_complete_general_offering_valid_child(const item_transfer_payload &input,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer) noexcept;
bool item_complete_general_duplicate_valid_child(const item_transfer_payload &input,
						 uint16_t version,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer) noexcept;
bool item_complete_general_lockpick_valid_child(const item_transfer_payload &input,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer) noexcept;
bool item_complete_general_pouch_payload_child(const item_transfer_payload &input,
					       craft_pouch_mutation *output,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer) noexcept;
bool item_complete_general_recipe_decode_child(std::span<const uint8_t> input,
					       craft_recipe_continuation *output,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer) noexcept;
bool item_complete_general_payload_valid_child(const item_transfer_payload &value, uint16_t version,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer) noexcept;
bool item_complete_general_validated_entities_child(critical_command *output,
						    const item_transfer_payload &value,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer) noexcept;
bool item_generic_decode_complete_owned(const critical_command &command,
					item_transfer_payload *payload,
					item_generic_decode_complete_budget &budget);
bool item_generic_wire_encode_complete_owned(const item_transfer_payload &payload, uint16_t version,
					     std::vector<uint8_t> *encoded,
					     item_payload_wire_complete_budget &budget);
bool item_complete_corpse_encode_child(const item_corpse_metadata &input,
				       std::vector<uint8_t> *output,
				       bool (*reserve)(size_t, void *) noexcept, void *context,
				       size_t outer) noexcept;
bool item_complete_corpse_decode_child(const uint8_t *input, size_t size,
				       item_corpse_metadata *output,
				       bool (*reserve)(size_t, void *) noexcept, void *context,
				       size_t outer) noexcept;
bool item_complete_collector_encode_child(const item_collector_death_enrollment &input,
					  std::vector<uint8_t> *output,
					  bool (*reserve)(size_t, void *) noexcept, void *context,
					  size_t outer) noexcept;
bool item_complete_collector_decode_child(const uint8_t *input, size_t size,
					  item_collector_death_enrollment *output,
					  bool (*reserve)(size_t, void *) noexcept, void *context,
					  size_t outer) noexcept;
bool item_complete_general_payload_encode_child(const item_transfer_payload &value,
						uint16_t version, std::vector<uint8_t> *output,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer, size_t *heap) noexcept;
bool item_complete_general_payload_decode_child(const critical_command &value,
						item_transfer_payload *output,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer, size_t *heap) noexcept;
bool item_complete_general_native_cost_encode_child(const item_transfer_payload &value,
						    bool acknowledged, std::vector<uint8_t> *output,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer) noexcept;
bool item_complete_general_public_native_encode_child(const item_transfer_payload &value,
						      std::vector<uint8_t> *output,
						      bool (*reserve)(size_t, void *) noexcept,
						      void *context, size_t outer) noexcept;
bool item_complete_general_public_native_recovery_encode_child(
	const item_transfer_payload &value, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept;
bool item_complete_general_public_inner_decode_child(const critical_command &value,
						     item_transfer_payload *output,
						     bool (*reserve)(size_t, void *) noexcept,
						     void *context, size_t outer) noexcept;
bool item_complete_general_fee_decode_child(const critical_command &value,
					    item_transfer_payload *output,
					    bool (*reserve)(size_t, void *) noexcept, void *context,
					    size_t outer) noexcept;
bool item_complete_general_money_decode_child(const critical_command &value,
					      item_transfer_payload *output,
					      bool (*reserve)(size_t, void *) noexcept,
					      void *context, size_t outer) noexcept;
bool item_encode_native_cost_complete_owned(const item_transfer_payload &payload, bool recovery,
					    std::vector<uint8_t> *encoded,
					    item_native_wire_encode_complete_budget &budget);
bool item_public_decode_complete_owned(const critical_command &command,
				       item_transfer_payload *payload,
				       item_public_dispatch_complete_budget &budget);
bool item_public_encode_complete_owned(const item_transfer_payload &payload,
				       std::vector<uint8_t> *encoded,
				       item_public_dispatch_complete_budget &budget);
bool item_public_encode_recovery_complete_owned(const item_transfer_payload &payload,
						std::vector<uint8_t> *encoded,
						item_public_dispatch_complete_budget &budget);
bool item_complete_general_public_encode_native_only_source_frame_bytes(size_t *output) noexcept;
bool item_complete_general_public_encode_native_only_source_supplement_frame_bytes(
	size_t *output) noexcept;
bool item_complete_general_public_encode_native_only_initial_inline_bytes(size_t *output) noexcept;
bool item_complete_general_public_recovery_encode_native_only_source_frame_bytes(
	size_t *output) noexcept;
bool item_complete_general_public_recovery_encode_native_only_source_supplement_frame_bytes(
	size_t *output) noexcept;
bool item_complete_general_public_recovery_encode_native_only_initial_inline_bytes(
	size_t *output) noexcept;
bool item_complete_general_public_decode_inner_native_source_frame_bytes(size_t *output) noexcept;
bool item_complete_general_public_decode_inner_native_source_supplement_frame_bytes(
	size_t *output) noexcept;
bool item_complete_general_public_decode_inner_native_initial_inline_bytes(size_t *output) noexcept;
bool item_complete_valid_collector_context(const item_transfer_payload &payload,
					   uint16_t payload_version);
bool item_complete_valid_text(const std::string &value, size_t maximum, bool required);
bool item_complete_valid_soulbind_continuation(const item_transfer_payload &payload);
bool item_complete_target_topology_for(const item_transfer_payload &payload, uint64_t item_uid,
				       uint64_t *root_item_uid, uint64_t *parent_item_uid);
uint64_t item_complete_selected_root_for(const item_transfer_payload &payload, uint64_t item_uid);
struct item_sidecar_complete_budget
{
	item_sidecar_reserve_fn reserve;
	void *context;
	size_t outer, frames;
	const item_corpse_metadata *corpse = nullptr;
	const item_collector_death_enrollment *collector = nullptr;
	const std::vector<uint8_t> *bytes = nullptr;
	bool prefix(size_t &output, size_t extra = 0) const noexcept
	{
		size_t total = outer;

		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    (corpse &&
		     (!payload_clone_string_heap(corpse->owner_name, false, total) ||
		      !payload_clone_string_heap(corpse->short_description, false, total) ||
		      !payload_clone_string_heap(corpse->description, false, total) ||
		      !payload_clone_string_heap(corpse->keywords, false, total))) ||
		    (collector &&
		     !payload_clone_vector_heap(collector->eligible_item_uids, false, total)) ||
		    (bytes && !payload_clone_vector_heap(*bytes, false, total)) ||
		    !payload_clone_add(total, extra))
			return false;
		output = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	template <typename T> bool fresh(size_t count, size_t extra) const noexcept
	{
		return count <= SIZE_MAX / sizeof(T) &&
		       payload_clone_add(extra, count * sizeof(T)) && peak(extra);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count,
					  size_t caller_frames = 0) const noexcept
	{
		size_t request = 0;
		if (count > value.max_size() - value.size())
			return false;
		if (value.size() + count > value.capacity())
		{
			size_t next = value.size();
			if (!payload_clone_add(next, std::max(value.size(), count)) ||
			    next > value.max_size())
				next = value.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !payload_clone_add(request, next * sizeof(T)))
				return false;
		}
		return payload_clone_add(request, caller_frames) && peak(request);
	}
	bool text(std::string &value, const char *source, size_t length, size_t caller_frames) const
	{
		size_t request = 0;
		if (length > value.capacity())
		{
			const size_t capacity = value.capacity();
			size_t next = length;
			if (capacity > SIZE_MAX / 2)
				return false;
			if (next < 2 * capacity)
				next = 2 * capacity;
			if (next > value.max_size())
				next = value.max_size();
			if (next == SIZE_MAX || !payload_clone_add(request, next + 1))
				return false;
		}
		if (!payload_clone_add(request, caller_frames) || !peak(request))
			return false;
		value.assign(source, length);
		return true;
	}
};
constexpr size_t item_complete_put_u16_own_source = sizeof(void *) + sizeof(uint16_t);
constexpr size_t item_complete_put_u32_own_source =
	sizeof(void *) + sizeof(uint32_t) + sizeof(unsigned int);
constexpr size_t item_complete_put_u64_own_source =
	sizeof(void *) + sizeof(uint64_t) + sizeof(unsigned int);
constexpr size_t item_complete_get_u16_own_source = sizeof(void *) + sizeof(uint16_t);
constexpr size_t item_complete_get_u32_own_source =
	sizeof(void *) + 2 * sizeof(uint32_t) + sizeof(unsigned int);
constexpr size_t item_complete_get_u64_own_source =
	sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(unsigned int);
constexpr size_t item_complete_encode_owner_own_source = 2 * sizeof(void *);
constexpr size_t item_complete_decode_owner_own_source = sizeof(void *);
constexpr size_t item_complete_decode_owner_result_inline = sizeof(item_owner_identity);
constexpr size_t item_complete_read_u32_own_source =
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_append_u32_own_source =
	// owner/output + catch3P, valueU32, actual offsetN, bool result.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(uint32_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_append_text_own_source =
	// owner/output/value + catch4P, bool result. Removed old constexpr ownN.
	4 * sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_sidecar_frame_end_destructor_source = sizeof(void *);
constexpr size_t item_complete_sidecar_read_text_own_source =
	// owner/input/offset/value + catch5P, size/maximum2N, lengthU32, bool.
	5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(uint32_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_encode_corpse_own_source =
	// args3P + actual hidden array range/begin/end3P + byvalue field i32.
	6 * sizeof(void *) + sizeof(int32_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_decode_corpse_own_source =
	// args3P + range/begin/end/value-ref4P; size/offset2N; four header U32,
	// actual decoded field U32 and returned bool. Reset {} is separate DTO.
	7 * sizeof(void *) + 2 * sizeof(size_t) + 5 * sizeof(uint32_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_encode_collector_own_source =
	// args3P/catch/range2P, fixed_size/count/offset3N, uidU64 + actual2CI.
	5 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(uint64_t) + sizeof(bool) +
	2 * sizeof(std::vector<uint64_t>::const_iterator);
constexpr size_t item_complete_sidecar_decode_collector_own_source =
	4 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(uint32_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_encode_wrapper_own_source =
	// Four genuine parameters/catch5P, outer/constexpr frames2N, bool.
	5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_decode_wrapper_own_source =
	// Four pointer params/catch5P, size/outer/frames3N, bool.
	5 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
namespace item_sidecar_recipe_source = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_byte_default_n_a_own_source = 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_byte_default_n_own_source =
	// first/n/returned first; declared constexpr can_fillB and actual
	// std::is_constant_evaluated resultB (its constant branch is unselected).
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool);
constexpr size_t item_complete_byte_default_n_true_own_source = 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_byte_default_construct_source =
	// __addressof(value-ref/pointer)2P; _Construct(p)P and actual internal
	// predicateB; runtime placement new(count,location,returned pointer).
	// No scalar ctor or runtime construct_at in the constant-only arm.
	2 * sizeof(void *) + sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_byte_default_fill_suffix_source =
	item_sidecar_recipe_source::complete_recipe_byte_fill_n_source +
	item_sidecar_recipe_source::complete_recipe_size_to_integer_source +
	item_sidecar_recipe_source::complete_recipe_byte_fill_n_a_source +
	// Category result aliases actual RA parameter, so only its constrefP.
	sizeof(void *) + item_sidecar_recipe_source::complete_recipe_byte_fill_a_source +
	item_sidecar_recipe_source::complete_recipe_byte_fill_a1_source +
	item_sidecar_recipe_source::complete_recipe_std_constant_evaluated_source;
constexpr size_t item_complete_byte_default_n_source =
	item_complete_byte_default_n_a_own_source + item_complete_byte_default_n_own_source +
	item_complete_byte_default_n_true_own_source + item_complete_byte_default_construct_source +
	item_complete_byte_default_fill_suffix_source;
constexpr size_t item_complete_byte_default_append_own_source =
	// this,n,size,navail,old_start,old_finish,len,new_start. No discarded
	// __destroy_from nor non-relocate branch controls; catch(...) has no ref.
	4 * sizeof(void *) + 4 * sizeof(size_t);
constexpr size_t item_complete_byte_resize_positive_source =
	// Actual resize(this,new_size), two reached size() occurrences; helper
	// always calls resize(oldsize+4), so erase/shrink branch is unselected.
	sizeof(void *) + sizeof(size_t) +
	2 * item_sidecar_recipe_source::complete_recipe_vector_size_source<uint8_t> +
	item_complete_byte_default_append_own_source +
	item_sidecar_recipe_source::complete_recipe_vector_size_source<uint8_t> +
	// Two runtime max_size occurrences in default_append's valid-size check.
	2 * item_sidecar_recipe_source::complete_recipe_vector_max_size_source<uint8_t> +
	item_complete_byte_default_n_source +
	// Growing path _M_check_len: exact own row, four actual size calls and
	// two max calls; bounded payload sizes cannot select overflow's third.
	item_sidecar_recipe_source::complete_recipe_vector_check_len_own_source<uint8_t> +
	4 * item_sidecar_recipe_source::complete_recipe_vector_size_source<uint8_t> +
	2 * item_sidecar_recipe_source::complete_recipe_vector_max_size_source<uint8_t> +
	item_sidecar_recipe_source::complete_recipe_size_max_source +
	item_sidecar_recipe_source::complete_recipe_vector_allocate_source<uint8_t> +
	item_sidecar_recipe_source::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	item_sidecar_recipe_source::complete_recipe_vector_relocate_source<uint8_t> +
	item_sidecar_recipe_source::complete_recipe_vector_deallocate_graph_source<uint8_t>;
constexpr size_t item_complete_byte_fill_static_max_source =
	// _S_max_size allocref/diffmax/allocmax/return=P3N, traits max=P+N,
	// min(two constrefs/returned ref)=3P; no public max/getconstallocator.
	5 * sizeof(void *) + 4 * sizeof(size_t);
constexpr size_t item_complete_byte_fill_allocator_reference_constructor_source =
	// Actual tmp vector(n,value,existing allocator-ref), no default allocator
	// argument object or its default ctor. Actual zero-byte belongs assign.
	3 * sizeof(void *) + sizeof(size_t) +
	item_sidecar_recipe_source::complete_recipe_byte_check_init_len_source +
	item_sidecar_recipe_source::complete_recipe_allocator_const_copy_source +
	item_sidecar_recipe_source::complete_recipe_allocator_cleanup_source +
	item_complete_byte_fill_static_max_source +
	item_sidecar_recipe_source::complete_recipe_byte_fill_base_source +
	item_sidecar_recipe_source::complete_recipe_vector_allocate_source<uint8_t> +
	item_sidecar_recipe_source::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	item_sidecar_recipe_source::complete_recipe_byte_fill_initialize_source +
	item_sidecar_recipe_source::complete_recipe_byte_uninitialized_fill_a_source +
	item_sidecar_recipe_source::complete_recipe_byte_uninitialized_fill_source +
	item_sidecar_recipe_source::complete_recipe_byte_uninitialized_fill_true_source +
	item_complete_byte_default_fill_suffix_source;
constexpr size_t item_complete_byte_fill_tmp_null_cleanup_source =
	// vector/base/impl/allocator4P, actual empty destroy7P+predicateB,
	// mutable allocator getter2P and _M_deallocate(nullptr,0)=2P+N.
	// No lower traits/delete body when swapped old allocation is null.
	15 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_byte_fill_swap_data_source =
	// Exact this/other2P + real data temporary3P + declared default receiverP
	// and THREE actual _M_copy_data call occurrences2P each. Conservative
	// actual-call summation; no implicit trivial data copy/assignment body.
	12 * sizeof(void *);
constexpr size_t item_complete_byte_fresh_fill_assign_source =
	// Public assign(this,n,valref)+real converted zero-byte argument;
	// _M_fill_assign(this,n,valref). Fresh candidate capacity0,n>0 selects
	// only its vector tmp allocation/swap/cleanup path, never fitting fill.
	2 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(uint8_t) +
	item_sidecar_recipe_source::complete_recipe_vector_capacity_source<uint8_t> +
	item_sidecar_recipe_source::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	item_complete_byte_fill_allocator_reference_constructor_source +
	item_complete_byte_fill_swap_data_source + item_complete_byte_fill_tmp_null_cleanup_source;
constexpr size_t item_complete_byte_fill_temporary_inline = sizeof(std::vector<uint8_t>);
using item_complete_collector_uid = uint64_t;
using item_complete_collector_uid_vector = std::vector<uint64_t>;
namespace item_collector_recipe_source = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_collector_uid_default_source =
	// Explicit/defaulted vector/base/impl/data/allocator/newallocator
	// constructor receiver families. Physical vector/subobjects stay in DTO.
	6 * sizeof(void *);
constexpr size_t item_complete_collector_uid_cleanup_source =
	item_collector_recipe_source::member_vector_cleanup_source<item_complete_collector_uid>();
constexpr size_t item_complete_collector_uid_reserve_source =
	// Fresh candidate size/capacity0; positive count reaches this lane.
	// count0 skips reserve allocation and has no subsequent pushes.
	// Zero-count _S_relocate/a/a1 still owns formals/niter/count, but its
	// count>0 constant-evaluation predicate call is not reached. Null old
	// allocation reaches only _M_deallocate(this,p,n), no lower delete chain.
	item_collector_recipe_source::complete_recipe_vector_reserve_own_source<
		item_complete_collector_uid> +
	item_collector_recipe_source::complete_recipe_vector_max_size_source<
		item_complete_collector_uid> +
	item_collector_recipe_source::complete_recipe_vector_capacity_source<
		item_complete_collector_uid> +
	item_collector_recipe_source::complete_recipe_vector_size_source<
		item_complete_collector_uid> +
	item_collector_recipe_source::complete_recipe_vector_allocate_source<
		item_complete_collector_uid> +
	item_collector_recipe_source::complete_recipe_vector_mutable_allocator_source<
		item_complete_collector_uid> +
	21 * sizeof(void *) + sizeof(std::ptrdiff_t) + 2 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_collector_uid_push_source =
	// Actual push_back(get_u64(...)) binds that original returned scalar.
	// No second uint64 argument object or scalar constructor body is invented.
	// The returned wire scalar is owned by the actual get_u64 source lane.
	// Reserve(count), then exactly count pushes: each fitting branch is
	// selected. There is no realloc/check_len/allocation/relocation/delete
	// at these push occurrences. Actual emplace returns back()->end()->minus.
	item_collector_recipe_source::complete_recipe_vector_push_own_source<
		item_complete_collector_uid> +
	item_collector_recipe_source::complete_recipe_scalar_move_source<
		item_complete_collector_uid> +
	item_collector_recipe_source::complete_recipe_vector_emplace_own_source<
		item_complete_collector_uid> +
	item_collector_recipe_source::complete_recipe_scalar_forward_source<
		item_complete_collector_uid> +
	item_collector_recipe_source::complete_recipe_vector_single_construct_source<
		item_complete_collector_uid> +
	sizeof(void *) + sizeof(item_complete_collector_uid_vector::iterator) +
	item_collector_recipe_source::complete_recipe_normal_pointer_constructor_source<
		item_complete_collector_uid_vector::iterator> +
	item_collector_recipe_source::complete_recipe_vector_back_own_source<
		item_complete_collector_uid> +
	item_collector_recipe_source::complete_recipe_normal_offset_own_source<
		item_complete_collector_uid_vector::iterator> +
	item_collector_recipe_source::complete_recipe_normal_pointer_constructor_source<
		item_complete_collector_uid_vector::iterator> +
	item_collector_recipe_source::complete_recipe_normal_dereference_source<
		item_complete_collector_uid_vector::iterator>;
constexpr size_t item_complete_collector_uid_move_assignment_source =
	item_collector_recipe_source::complete_recipe_vector_move_selected_source<
		item_complete_collector_uid>;
constexpr size_t item_complete_collector_uid_fitting_growth_query_source =
	// Actual complete sidecar growth(max_size-size,size+1<=capacity) through
	// exactly count reserved pushes. No next/reallocation branch is reached.
	item_collector_recipe_source::complete_recipe_vector_max_size_source<uint64_t> +
	2 * item_collector_recipe_source::complete_recipe_vector_size_source<uint64_t> +
	item_collector_recipe_source::complete_recipe_vector_capacity_source<uint64_t>;
constexpr size_t item_complete_collector_nontrivial_value_source =
	// Real enclosing collector move assignment this/source/result3P and
	// generated nontrivial destructorP. The P for the actual PLAIN
	// wrapper candidate default constructor belongs separately to endpoint
	// lifetime18; reset{} directly initializes vector member without that
	// enclosing constructor. No trivial policy/ID/array callable is invented.
	// Physical candidate/reset DTOs remain separately admitted.
	4 * sizeof(void *);
namespace item_sidecar_lifetime_source = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_sidecar_byte_default_source = 6 * sizeof(void *);
constexpr size_t item_complete_sidecar_byte_cleanup_source =
	item_sidecar_lifetime_source::member_vector_cleanup_source<uint8_t>();
constexpr size_t item_complete_sidecar_byte_move_source =
	item_sidecar_lifetime_source::complete_recipe_vector_move_selected_source<uint8_t>;
constexpr size_t item_complete_sidecar_encode_lifetime_source =
	// Actual local byte candidate default, final move assignment and cleanup.
	// Caller std::move<byte-vector> is actual ref/result2P. No corpse/collector
	// input default/COPY/cleanup is reached in either encoder endpoint.
	item_complete_sidecar_byte_default_source + item_complete_sidecar_byte_move_source +
	item_complete_sidecar_byte_cleanup_source + 2 * sizeof(void *);
constexpr size_t item_complete_sidecar_corpse_decode_lifetime_source =
	// Plain candidate default reaches actual enclosing nontrivial ctorP;
	// reset{} directly constructs four char string members without that ctor;
	// exact same completed library family may reuse across sequential fields.
	// Generated nontrivial corpse move receiver/source/result3P, destructorP;
	// actual caller std::move<corpse> refs/results2P. No trivial array callee.
	7 * sizeof(void *) + item_sidecar_lifetime_source::member_string_default_source +
	item_sidecar_lifetime_source::member_string_move_assignment_source +
	// Actual sibling mutable allocator getter at alloc_on_move: selected
	// smaller char-move projection requires its separate second getter2P.
	item_sidecar_lifetime_source::complete_recipe_string_mutable_allocator_source +
	item_sidecar_lifetime_source::member_string_destroy_source;
constexpr size_t item_complete_sidecar_collector_decode_lifetime_source =
	// Plain enclosing candidate default ctor receiverP; reset{} is aggregate.
	sizeof(void *) + item_complete_collector_uid_default_source +
	item_complete_collector_uid_move_assignment_source +
	item_complete_collector_uid_cleanup_source +
	item_complete_collector_nontrivial_value_source + 2 * sizeof(void *);
constexpr size_t item_complete_sidecar_checked_add_source =
	// Actual unchanged payload_clone_add(total-ref,amount)/bool.
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_string_current_heap_source =
	// payload_clone_string_heap(value-ref,fresh,bytes-ref,count,bool), then
	// actual capacity and const islocal/data/local pointer_to/addressof chain.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + (sizeof(void *) + sizeof(size_t)) +
	(11 * sizeof(void *) + sizeof(bool));
template <class T> constexpr size_t item_complete_sidecar_vector_current_heap_source =
	// payload_clone_vector_heap<T> own value/bytes refs,fresh/count/result;
	// only capacity() on actual false branch. No fresh size/query imported.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) +
	item_sidecar_lifetime_source::complete_recipe_vector_capacity_source<T>;
constexpr size_t item_complete_sidecar_encode_observation_source =
	// Encoder owner.bytes only: its corpse/collector selectors stay null.
	item_complete_sidecar_vector_current_heap_source<uint8_t> +
	item_complete_sidecar_checked_add_source;
constexpr size_t item_complete_sidecar_corpse_decode_observation_source =
	// Decoder owner.corpse points to genuine candidate. Four sequential
	// string observations reuse identical completed helper/type lanes;
	// parent DTO keeps all four physical strings/capacities separately.
	item_complete_sidecar_string_current_heap_source + item_complete_sidecar_checked_add_source;
constexpr size_t item_complete_sidecar_collector_decode_observation_source =
	// Decoder owner.collector candidate UID vector only, no byte/string lane.
	item_complete_sidecar_vector_current_heap_source<uint64_t> +
	item_complete_sidecar_checked_add_source;
namespace item_identity_source = duris_craft_recipe_bounded_detail;
using item_complete_identity_byte_I = std::vector<uint8_t>::iterator;
template <class Output> constexpr size_t item_complete_identity_selected_copy_source =
	item_identity_source::complete_recipe_copy_projected_source<const uint8_t *, Output> +
	2 * item_identity_source::complete_recipe_miter_argument_source<const uint8_t *> +
	item_identity_source::complete_recipe_copy_projected_source<const uint8_t *, Output> +
	2 * item_identity_source::complete_recipe_niter_graph_source<const uint8_t *> +
	item_identity_source::complete_recipe_niter_graph_source<Output> +
	item_identity_source::complete_recipe_raw_copy_a1_source<const uint8_t *, uint8_t *> +
	item_identity_source::complete_recipe_raw_copy_a2_source<const uint8_t *, uint8_t *> +
	// Actual identity width exceeds1: _Num local stays, assign_one is not
	// reached. Builtin memmove declares no separately priced C++ callee.
	item_identity_source::complete_recipe_byte_simple_copy_m_source +
	item_identity_source::complete_recipe_std_constant_evaluated_source +
	(std::is_pointer_v<Output> ?
		 item_identity_source::complete_recipe_raw_niter_wrap_source :
		 item_identity_source::complete_recipe_normal_niter_wrap_own_source<Output> +
			 item_identity_source::complete_recipe_niter_graph_source<Output> +
			 item_identity_source::complete_recipe_normal_offset_argument_source<Output> +
			 item_identity_source::complete_recipe_normal_pointer_constructor_source<
				 Output>);
constexpr size_t item_complete_collector_identity_encode_source =
	// Array const begin/end each(this/returned raw pointer)2P and their
	// const data(this/returned raw pointer)2P. Conservative distinct calls.
	8 * sizeof(void *) +
	// Actual destination begin() iterator survives to operator+; the latter
	// returned I initializes std::copy's by-value output parameter directly.
	item_identity_source::complete_recipe_wire_begin_own_source +
	item_identity_source::complete_recipe_normal_pointer_constructor_source<
		item_complete_identity_byte_I> +
	item_identity_source::complete_recipe_normal_offset_argument_source<
		item_complete_identity_byte_I> +
	item_identity_source::complete_recipe_normal_pointer_constructor_source<
		item_complete_identity_byte_I> +
	item_complete_identity_selected_copy_source<item_complete_identity_byte_I>;
constexpr size_t item_complete_collector_identity_decode_source =
	// Actual array size(this/returned N), mutable begin/data each2P.
	sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) +
	// std::copy_n firstP,nN,resultP,returnedP, const n2N; actual
	// __size_to_integer(size_t) parameter/return2N remains separate.
	3 * sizeof(void *) + 4 * sizeof(size_t) +
	// __copy_n<RA> firstP,nN,resultP,returnedP, by-value RA tag.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(std::random_access_iterator_tag) +
	// iterator_category const iterator-ref P; RA result is the same direct
	// by-value tag destination above, not another temporary tag object.
	sizeof(void *) + item_complete_identity_selected_copy_source<uint8_t *>;
namespace item_sidecar_operation_source = duris_craft_recipe_bounded_detail;
using item_complete_sidecar_K = std::string::const_iterator;
using item_complete_sidecar_u64_CI = std::vector<uint64_t>::const_iterator;
constexpr size_t item_complete_sidecar_byte_clear_source =
	// clear thisP, _M_erase_at_end this/pos/n (2P+N), actual _Destroy
	// first/last/allocator then first/last then trivial aux first/last7P.
	// Its reached internal predicate is a real declared Boolean. No element
	// destructor body, no deallocation, no constant-only destroy_at branch.
	10 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_byte_data_source =
	// Public mutable data this/returned pointer2P and selected _M_data_ptr
	// this/pointer/returned pointer3P. Plain byte allocator raw pointer route.
	5 * sizeof(void *);
constexpr size_t item_complete_sidecar_byte_growth_query_source =
	// Four real sequential size occurrences and two max_size occurrences
	// through valid bounded growth. Actual std::max ref/ref/returnedref.
	4 * item_sidecar_operation_source::complete_recipe_vector_size_source<uint8_t> +
	2 * item_sidecar_operation_source::complete_recipe_vector_max_size_source<uint8_t> +
	item_sidecar_operation_source::complete_recipe_vector_capacity_source<uint8_t> +
	item_sidecar_operation_source::complete_recipe_size_max_source;
constexpr size_t item_complete_sidecar_corpse_encode_array_source =
	// Actual const array<int32_t,8> begin/end and const data scopes.
	// Hidden raw pointer results are in encode_corpse own range storage;
	// raw scalar returns remain distinct under existing Source convention.
	8 * sizeof(void *);
constexpr size_t item_complete_sidecar_corpse_decode_array_source =
	// Different mutable array<int32_t,8> begin/end/data specialization.
	8 * sizeof(void *);
constexpr size_t item_complete_sidecar_collector_encode_uid_range_source =
	// Actual hidden const CI results are in encode_collector own storage.
	2 * (sizeof(void *) +
	     item_sidecar_operation_source::complete_recipe_normal_pointer_constructor_source<
		     item_complete_sidecar_u64_CI>)+item_sidecar_operation_source::
			complete_recipe_normal_equal_own_source<item_complete_sidecar_u64_CI> +
	2 * item_sidecar_operation_source::complete_recipe_normal_base_source<
		    item_complete_sidecar_u64_CI> +
	item_sidecar_operation_source::complete_recipe_normal_preincrement_source<
		item_complete_sidecar_u64_CI> +
	item_sidecar_operation_source::complete_recipe_normal_dereference_source<
		item_complete_sidecar_u64_CI>;
constexpr size_t item_complete_sidecar_append_text_input_source =
	// Both string begin/end K prvalues initialize actual insert Input
	// formals. Actual pointer-return/offset materialization2P each; const
	// data getters2P and explicit normal constructors2P each are reached.
	2 * (2 * sizeof(void *) +
	     item_sidecar_operation_source::complete_recipe_string_const_data_source +
	     item_sidecar_operation_source::complete_recipe_normal_pointer_constructor_source<
		     item_complete_sidecar_K>)+item_sidecar_operation_source::
			complete_recipe_string_size_source +
	// Actual destination end I survives conversion to public position CI.
	item_sidecar_operation_source::complete_recipe_wire_end_own_source +
	item_sidecar_operation_source::complete_recipe_normal_pointer_constructor_source<
		std::vector<uint8_t>::iterator>;
constexpr size_t item_complete_sidecar_append_text_insert_source =
	// Same selected GNU13 end-insert K<const char*,string>→byte as recipe;
	// fitting zero-old-range and growing branches are both real here.
	item_sidecar_operation_source::complete_recipe_byte_end_insert_graph_source<
		item_complete_sidecar_K, const char *> +
	item_complete_sidecar_append_text_input_source;
constexpr size_t item_complete_sidecar_fresh_string_operations_source =
	// Candidate/reset make each actual destination length0 before its one
	// assign. Empty/disjoint fitting and allocate/mutate routes are reached;
	// old-content copy is not. No unrelated recipe budget/DTO/lifetime total.
	item_sidecar_operation_source::complete_recipe_string_assign_entry_source +
	item_sidecar_operation_source::complete_recipe_string_replace_own_source +
	item_sidecar_operation_source::complete_recipe_string_check_length_source +
	item_sidecar_operation_source::complete_recipe_string_disjunct_source +
	2 * item_sidecar_operation_source::complete_recipe_string_less_source +
	item_sidecar_operation_source::complete_recipe_string_mutate_own_source +
	item_sidecar_operation_source::complete_recipe_string_create_own_source +
	item_sidecar_operation_source::complete_recipe_string_char_allocation_source +
	// Exact controller queries occur before assign. Their returned carriers
	// are separate from controller own request/capacity/next declarations.
	2 * item_sidecar_operation_source::complete_recipe_string_capacity_own_source +
	item_sidecar_operation_source::complete_recipe_string_max_size_source;
constexpr size_t item_complete_controller_prefix_source =
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_controller_peak_source =
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_validation_prefix_source =
	4 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	2 * sizeof(std::vector<player_item_snapshot>::const_iterator);
constexpr size_t item_complete_sidecar_fresh_source =
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_growth_source =
	2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_fitting_growth_source =
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_sidecar_text_source =
	3 * sizeof(void *) + 5 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_native_encode_growth_source =
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_native_encode_clone_payload_source =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_dispatch_command_copy_source =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
namespace item_complete_sidecar_source = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_sidecar_common_source =
	// Actual old policy resultB; complete controller prefix/peak declarations;
	// selected reserve interface only. Its target Source remains caller-owned.
	sizeof(bool) + item_complete_controller_prefix_source +
	item_complete_controller_peak_source +
	item_complete_sidecar_source::complete_recipe_callback_interface_source;
constexpr size_t item_complete_sidecar_encode_wrapper_typed_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 2 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_sidecar_decode_wrapper_typed_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 3 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_sidecar_corpse_encode_retained_source =
	item_complete_sidecar_common_source + item_complete_sidecar_encode_wrapper_typed_source +
	item_complete_sidecar_encode_corpse_own_source +
	item_complete_sidecar_append_u32_own_source + item_complete_sidecar_append_text_own_source +
	item_complete_sidecar_frame_end_destructor_source +
	item_complete_sidecar_encode_lifetime_source +
	item_complete_sidecar_encode_observation_source + item_complete_sidecar_growth_source +
	item_complete_sidecar_byte_growth_query_source + item_complete_byte_resize_positive_source +
	item_complete_put_u32_own_source + item_complete_sidecar_byte_clear_source +
	item_complete_sidecar_byte_data_source +
	item_complete_sidecar_source::complete_recipe_vector_size_source<uint8_t> +
	item_complete_sidecar_append_text_insert_source +
	item_complete_sidecar_corpse_encode_array_source;
constexpr size_t item_complete_sidecar_corpse_decode_retained_source =
	item_complete_sidecar_common_source + item_complete_sidecar_decode_wrapper_typed_source +
	item_complete_sidecar_decode_corpse_own_source +
	item_complete_sidecar_read_text_own_source + item_complete_read_u32_own_source +
	item_complete_get_u32_own_source + item_complete_sidecar_corpse_decode_lifetime_source +
	item_complete_sidecar_corpse_decode_observation_source + item_complete_sidecar_text_source +
	item_complete_sidecar_fresh_string_operations_source +
	item_complete_sidecar_corpse_decode_array_source;
constexpr size_t item_complete_sidecar_collector_encode_retained_source =
	item_complete_sidecar_common_source + item_complete_sidecar_encode_wrapper_typed_source +
	item_complete_sidecar_encode_collector_own_source +
	item_complete_sidecar_encode_lifetime_source +
	item_complete_sidecar_encode_observation_source + item_complete_sidecar_fresh_source +
	item_complete_byte_fresh_fill_assign_source + item_complete_put_u32_own_source +
	item_complete_put_u64_own_source + item_complete_sidecar_byte_clear_source +
	item_complete_sidecar_byte_data_source +
	item_complete_sidecar_source::complete_recipe_vector_size_source<uint8_t> +
	item_complete_sidecar_source::complete_recipe_vector_size_source<uint64_t> +
	item_complete_collector_identity_encode_source +
	item_complete_sidecar_collector_encode_uid_range_source;
constexpr size_t item_complete_sidecar_collector_decode_retained_source =
	item_complete_sidecar_common_source + item_complete_sidecar_decode_wrapper_typed_source +
	item_complete_sidecar_decode_collector_own_source +
	item_complete_sidecar_collector_decode_lifetime_source +
	item_complete_sidecar_collector_decode_observation_source +
	item_complete_sidecar_fresh_source + item_complete_sidecar_fitting_growth_source +
	item_complete_collector_uid_fitting_growth_query_source +
	item_complete_collector_uid_reserve_source + item_complete_collector_uid_push_source +
	item_complete_get_u32_own_source + item_complete_get_u64_own_source +
	item_complete_collector_identity_decode_source;
constexpr size_t item_complete_duplicate_public_own_source =
	// payload/context/catch3P,F,version,outer/frames2N and returned bool.
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + sizeof(uint16_t) +
	2 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_duplicate_owned_own_source =
	// payload/owner/data/child/catch5P; version,duplicate/target/uid3U64,
	// direct_countU32, two lexical index and payload_direct_children3N/B.
	5 * sizeof(void *) + sizeof(uint16_t) + 3 * sizeof(uint64_t) + sizeof(uint32_t) +
	3 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_craft_outputs_public_own_source =
	// payload/output/context/retained/catch + hidden candidate range/item7P,
	// F,outer/frames/retained/row4N,B and two mutable hidden row I objects.
	7 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 4 * sizeof(size_t) +
	sizeof(bool) + 2 * sizeof(std::vector<player_item_snapshot>::iterator);
constexpr size_t item_complete_craft_outputs_owned_own_source =
	// payload/output/owner/output-row/catch5P; prefix/index/levels/remaining4N.
	5 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_duplicate_controller_source =
	// Shared actual validation prefix this/output,extra/total/row and B.
	// Only uids pointer is set; unconditional row local remains a real N.
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	// peak this,extra/total,B; actual checked-add and callback signatures.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) +
	2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	// Actual false UID heap helper value/bytes,fresh/count/B then capacity.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_craft_outputs_controller_source =
	// Actual items/uids selectors, const row-range hidden physical CI locals,
	// prefix range/item refs are distinct from wrapper's mutable I range.
	4 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	2 * sizeof(std::vector<player_item_snapshot>::const_iterator) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	// Separate row and UID false vector heap helper instantiations/capacity.
	2 * (2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + sizeof(void *) +
	     sizeof(size_t)) +
	// Actual row CURRENT and const row range, no COPY/default/decoder profile.
	44 * sizeof(void *) + 10 * sizeof(size_t) + 15 * sizeof(bool) + 14 * sizeof(void *) +
	sizeof(bool);
using item_duplicate_uid_I = std::vector<uint64_t>::iterator;
using item_duplicate_equal_E =
	decltype(__gnu_cxx::__ops::__iter_equals_val(std::declval<const uint64_t &>()));
constexpr size_t item_complete_duplicate_find_graph_source =
	// Genuine MUTABLE UID iterator specialization, not recovery const-CI.
	// Public find 2I+valueP; equals factory/constructor3P; dispatch2I+E;
	// RA core2I+E+tripD+RA, categoryP; difference6P+D; ++2P;
	// equality adapter P+I+B and dereference2P. Class return propagates
	// to one caller comparison operand, owned separately below.
	7 * sizeof(item_duplicate_uid_I) + 2 * sizeof(item_duplicate_equal_E) +
	16 * sizeof(void *) + 2 * sizeof(std::ptrdiff_t) + sizeof(std::random_access_iterator_tag) +
	sizeof(bool);
constexpr size_t item_complete_duplicate_find_caller_source =
	// Two actual find expression sites: each find result and end operand.
	// begin/end input results alias public algorithm formal objects. Their
	// receiver family2P plus completed explicit pointer constructor2P is
	// shared across both sites; equality/base scopes6P+B finish before reuse.
	4 * sizeof(item_duplicate_uid_I) + 4 * sizeof(void *) + 6 * sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_duplicate_lifetime_source =
	// One plain empty UID vector; no element ctor/default/move is selected.
	6 * sizeof(void *) + item_complete_native_vector_cleanup_source<uint64_t>;
constexpr size_t item_complete_duplicate_direct_source =
	// Genuine put/get helper scope subsets: get32 and get64 only.
	sizeof(void *) + 2 * sizeof(uint32_t) + sizeof(unsigned) + sizeof(void *) +
	2 * sizeof(uint64_t) + sizeof(unsigned) +
	// const continuation-byte data5P, size P+N, const payload array index2P+N.
	5 * sizeof(void *) + sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) + sizeof(size_t) +
	// Wrapper's actual supported policy returned bool and published named
	// raw-array lookup/comparator/lower_bound full selected graph.
	sizeof(bool) + item_complete_offering_find_source;
constexpr size_t item_complete_craft_outputs_lifetime_source =
	// Actual empty candidate row and UID vectors, never default row members.
	2 * (6 * sizeof(void *)) +
	// Candidate/old-output vector row destruction reaches genuine nontrivial
	// row and ED destructor receivers inside these exact selected backends.
	item_complete_native_vector_cleanup_source<player_item_snapshot> +
	item_complete_native_vector_cleanup_source<uint64_t> +
	item_complete_native_vector_cleanup_source<player_item_dynamic_affect_snapshot> +
	item_complete_native_vector_cleanup_source<player_item_extra_description_snapshot> +
	item_complete_native_vector_cleanup_source<int32_t> +
	item_complete_native_member_string_cleanup_source +
	// Genuine equal-allocator vector<Row> assignment entry/temp/allocator/
	// swap graph. Remove ONLY the exact named trivial cleanup included in
	// that reviewed generic expression; actual nontrivial row backend is
	// retained above once. It is not a whole-copy credit or spare subtraction.
	duris_craft_recipe_bounded_detail::complete_recipe_vector_move_selected_source<
		player_item_snapshot> -
	duris_craft_recipe_bounded_detail::member_vector_cleanup_source<player_item_snapshot>() +
	// Actual caller std::move<vector<Row>> reference/result.
	2 * sizeof(void *);
constexpr size_t item_complete_craft_outputs_direct_source =
	// Blob fixed-array size/data and mutable output row subscript.
	sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// Distinct row/UID vector size getters; fresh output clear plus erase
	// owns this, position and declared n. n==0 excludes Destroy descendants.
	2 * (sizeof(void *) + sizeof(size_t)) + sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t) +
	// const row empty: this/result B, two real CI result operands, begin/end
	// and their ctor scopes, comparison/base pair. No implicit trivial copy.
	13 * sizeof(void *) + 2 * sizeof(std::vector<player_item_snapshot>::const_iterator) +
	2 * sizeof(bool) +
	// Wrapper mutable candidate range: begin/end2P, pointer ctor2P, equality
	// refs/base6P+B, ++2P,deref2P. Hidden mutable I objects stay in own row.
	14 * sizeof(void *) + sizeof(bool) + sizeof(bool);
constexpr size_t item_complete_duplicate_retained_source =
	item_complete_duplicate_public_own_source + item_complete_duplicate_owned_own_source +
	item_complete_duplicate_controller_source + item_complete_duplicate_lifetime_source +
	item_complete_duplicate_direct_source + item_complete_duplicate_find_graph_source +
	item_complete_duplicate_find_caller_source + item_complete_native_uid_fresh_reserve_source +
	item_complete_native_uid_fitting_const_push_source;
constexpr size_t item_complete_craft_outputs_retained_source =
	item_complete_craft_outputs_public_own_source +
	item_complete_craft_outputs_owned_own_source +
	item_complete_craft_outputs_controller_source +
	item_complete_craft_outputs_lifetime_source + item_complete_craft_outputs_direct_source +
	item_complete_native_uid_fresh_reserve_source +
	item_complete_native_uid_fitting_const_push_source +
	// Actual same published mutable UID normal-iterator sort family. Runtime
	// recursive frames are admitted dynamically by the unchanged level count.
	item_complete_native_uid_sort_nonrecursive_source +
	item_complete_native_uid_adjacent_algorithm_source +
	item_complete_native_uid_iterator_acquisition_source +
	item_complete_native_uid_adjacent_caller_source +
	// Adapter declaration/query Source retained while old list child runs.
	// Child full Source+INITIAL preflight is transient; genuine old-child
	// supplement alone enters that child's outer. Not self-own0 relabeling.
	item_complete_snapshot_list_decode_child_own_source +
	3 * player_item_snapshot_list_decode_bounded_source_query_frame_bytes();
namespace item_general_entities_selected_recipe = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_general_entities_outputs_range_source =
	// Mutable row-I range: begin/end receivers2P, completed pointer ctor2P,
	// comparison2P+B with real base pair4P, ++2P and dereference2P.
	// The two actual hidden I objects already belong parent own19.
	14 * sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_general_entities_direct_source =
	// Authentic const fixed item-array subscript in input-key loop.
	2 * sizeof(void *) + sizeof(size_t) +
	// Original same-owner test and runtime policy query.
	2 * sizeof(void *) + 2 * sizeof(bool) +
	item_general_entities_selected_recipe::complete_recipe_vector_size_source<
		critical_entity_key> +
	item_general_entities_selected_recipe::complete_recipe_vector_size_source<
		critical_expected_revision>;
constexpr size_t item_complete_general_entities_command_default_source =
	// Actual critical_command candidate{} aggregate initialization. The three
	// distinct vector specialization families are exactly command10's selected
	// default source; both byte DTO members are physically candidate-owned.
	item_complete_command_aggregate_default_source;
constexpr size_t item_complete_general_entities_unselected_byte_deallocation_source =
	// Candidate payload/accounting_intent stay empty/null on every normal or
	// early-return path. _M_deallocate's root still executes; only downstream
	// traits/allocator/newallocator/delete fields below are truly unselected.
	7 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_general_entities_command_cleanup_source =
	// Keep actual generated destructor receiver and full K/R cleanup. Only
	// the candidate byte tail is projected to the proven null path15P+N+B.
	item_complete_command_cleanup_source -
	item_complete_general_entities_unselected_byte_deallocation_source;
constexpr size_t item_complete_general_entities_member_move_source =
	// Exactly two member-vector assignments, to arbitrary old output K/R
	// vectors. Their internal genuine V/A objects remain in each child Source.
	// Share only their identical actual typed cleanup methods with the parent
	// full K/R tail. Parent candidate DTO and caller/prior-output heaps stay out.
	duris_craft_recipe_bounded_detail::complete_recipe_vector_move_selected_source<
		critical_entity_key> -
	duris_craft_recipe_bounded_detail::member_vector_cleanup_source<critical_entity_key>() +
	duris_craft_recipe_bounded_detail::complete_recipe_vector_move_selected_source<
		critical_expected_revision> -
	duris_craft_recipe_bounded_detail::member_vector_cleanup_source<
		critical_expected_revision>() +
	// Two real outer std::move<vector<T>&> reference/formal result declarations.
	// These are selecting expressions, distinct from each child's inner move.
	4 * sizeof(void *);
constexpr size_t item_complete_general_entities_command_lifecycle_source =
	item_complete_general_entities_command_default_source +
	item_complete_general_entities_command_cleanup_source +
	item_complete_general_entities_member_move_source;
constexpr size_t item_complete_general_entities_owned_declaration_source =
	// command/payload/budget, actual output range ref and element ref;
	// admission_prefix and zero-or-positive for initializer index.
	5 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(std::vector<player_item_snapshot>::iterator) + sizeof(bool);
constexpr size_t item_complete_general_entities_wrapper_declaration_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 2 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_general_entities_prefix_declaration_source =
	4 * sizeof(void *) + 3 * sizeof(size_t) +
	2 * sizeof(std::vector<player_item_snapshot>::const_iterator) + sizeof(bool);
constexpr size_t item_complete_general_entities_peak_declaration_source =
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_general_entities_growth_declaration_source =
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_general_entities_fresh_declaration_source =
	sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_general_entities_sort_frame_declaration_source =
	// this, count, levels, remaining, request, real constexpr recursive local.
	sizeof(void *) + 5 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_general_entities_owner_key_child_declaration_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
struct item_entities_revision_less
{
	bool operator()(const critical_expected_revision &left,
			const critical_expected_revision &right) const
	{
		return critical_entity_key_less(left.key, right.key);
	}
};
template <class T> using item_general_entities_iterator = typename std::vector<T>::iterator;
template <class C> using item_general_entities_sort_adapter = __gnu_cxx::__ops::_Iter_comp_iter<C>;
template <class C> using item_general_entities_val_adapter = __gnu_cxx::__ops::_Val_comp_iter<C>;
template <class C> using item_general_entities_iter_val_adapter =
	__gnu_cxx::__ops::_Iter_comp_val<C>;
template <class T, class C>
constexpr size_t item_complete_general_entities_sort_nonrecursive_source =
	// Explicit declaration ledger42I+6P+9D+10A+C+V+W+7T, nine otherwise
	// unowned arithmetic/discarded resultsI; exact typed helper families
	// and lexical occurrences yield68I+327P+40D+11A+3C+V+W+7T below.
	68 * sizeof(item_general_entities_iterator<T>) + 327 * sizeof(void *) +
	40 * sizeof(typename std::iterator_traits<
		     item_general_entities_iterator<T>>::difference_type) +
	11 * sizeof(item_general_entities_sort_adapter<C>) + 3 * sizeof(C) +
	sizeof(item_general_entities_val_adapter<C>) +
	sizeof(item_general_entities_iter_val_adapter<C>) + 7 * sizeof(T) +
	2 * sizeof(std::make_unsigned_t<typename std::iterator_traits<
			   item_general_entities_iterator<T>>::difference_type>) +
	8 * sizeof(int) + 10 * sizeof(bool) + 2 * sizeof(std::strong_ordering) +
	2 * sizeof(std::__cmp_cat::__unspec) +
	// Actual target: key_less2P+B; named revision op3P+B nesting key_less.
	(std::is_same_v<T, critical_entity_key> ? 2 * sizeof(void *) + sizeof(bool) :
						  5 * sizeof(void *) + 2 * sizeof(bool));
template <class T, class C> constexpr size_t item_complete_general_entities_sort_recursive_source =
	3 * sizeof(item_general_entities_iterator<T>) +
	sizeof(typename std::iterator_traits<item_general_entities_iterator<T>>::difference_type) +
	sizeof(item_general_entities_sort_adapter<C>);
template <class T, class C> constexpr size_t item_complete_general_entities_sort_full_source =
	item_complete_general_entities_sort_nonrecursive_source<T, C> +
	// Validated noncraft <=2owners+3000items+parent+collector=3004;
	// craft <=3002, recovery explicitly <=3003. floorlog2(3004)=11.
	// Full prospective retains23; actual runtime controller must admit its
	// genuine (2*floorlog2(count)+1) recursive rows separately.
	23 * item_complete_general_entities_sort_recursive_source<T, C>;
template <class T> constexpr size_t item_complete_general_entities_sort_caller_source =
	// Actual begin/end two receivers + explicit pointer ctor2P each.
	// Result class prvalues directly initialize public sort formals.
	6 * sizeof(void *);
constexpr size_t item_complete_general_entities_key_adjacent_source =
	7 * sizeof(item_general_entities_iterator<critical_entity_key>) +
	3 * sizeof(decltype(&critical_entity_key_equal)) +
	sizeof(item_general_entities_sort_adapter<decltype(&critical_entity_key_equal)>) +
	20 * sizeof(void *) + 3 * sizeof(bool);
constexpr size_t item_complete_general_entities_key_adjacent_caller_source =
	// Actual begin/end/final end three receivers and pointer ctors9P.
	// Root result and separately evaluated end comparison operand eachI;
	// C++20 != rewrites ==6P+B, no independent != callable declaration.
	9 * sizeof(void *) + 2 * sizeof(item_general_entities_iterator<critical_entity_key>) +
	6 * sizeof(void *) + sizeof(bool);
namespace item_general_entity_vector_source_detail
{
constexpr size_t P = sizeof(void *), N = sizeof(size_t), D = sizeof(std::ptrdiff_t),
		 B = sizeof(bool);
// These are existing trivial DTOs, not surrogate types or new runtime guards.
static_assert(std::is_trivial_v<critical_entity_key>);
static_assert(std::is_trivial_v<critical_expected_revision>);
using namespace duris_craft_recipe_bounded_detail;

template <class T> constexpr size_t single_construct =
	// Exact selected traits construct(A&,T*,Arg&), two forward scopes,
	// construct_at(T*,Arg&), real placement-new count/location/result.
	complete_recipe_vector_single_construct_source<T>;
template <class T> constexpr size_t push_public = 2 * P;
template <class T> constexpr size_t check_len =
	// Genuine valid varied-count path, not old one-size/one-max inventory:
	// four size results, two max_size results and referenced std::max inputs.
	complete_recipe_vector_check_len_own_source<T> + 4 * complete_recipe_vector_size_source<T> +
	2 * complete_recipe_vector_max_size_source<T> + complete_recipe_size_max_source;
template <class T> constexpr size_t growth_end_argument =
	// end() returned I is _M_realloc_insert's position by-value destination.
	P + complete_recipe_normal_pointer_constructor_source<typename std::vector<T>::iterator>;
template <class T> constexpr size_t growth_position_minus_begin =
	// Genuine begin() temporary remains distinct from position I. Both base
	// refs survive through the actual iterator subtraction and its D result.
	P + complete_recipe_normal_pointer_constructor_source<typename std::vector<T>::iterator> +
	sizeof(typename std::vector<T>::iterator) +
	complete_recipe_normal_subtract_own_source<typename std::vector<T>::iterator> +
	2 * complete_recipe_normal_base_source<typename std::vector<T>::iterator>;
template <class T> constexpr size_t end_relocations =
	// Two real _S_relocate/__relocate_a/bitwise-a1 invocations. First old
	// prefix is nonempty; second end suffix is empty and does not execute
	// is_constant_evaluated. The declared wrappers/count still execute.
	complete_recipe_vector_relocate_source<T> + (21 * P + D) +
	2 * complete_recipe_vector_mutable_allocator_source<T> +
	2 * complete_recipe_normal_base_source<typename std::vector<T>::iterator>;
template <class T> constexpr size_t return_back =
	// back() has this/reference, end() temporary I and returned minus I.
	// The computed pointer argument of minus is a genuine prvalue; ctor
	// owns its two references. Dereference and back/reference results differ.
	complete_recipe_vector_back_own_source<T> + P + sizeof(typename std::vector<T>::iterator) +
	complete_recipe_normal_pointer_constructor_source<typename std::vector<T>::iterator> +
	complete_recipe_normal_offset_own_source<typename std::vector<T>::iterator> +
	complete_recipe_normal_pointer_constructor_source<typename std::vector<T>::iterator> +
	complete_recipe_normal_dereference_source<typename std::vector<T>::iterator>;
template <class T> constexpr size_t growth_core =
	complete_recipe_vector_realloc_insert_own_source<T> + growth_end_argument<T> +
	growth_position_minus_begin<T> + check_len<T> + complete_recipe_vector_allocate_source<T> +
	2 * P + single_construct<T> + end_relocations<T> +
	// Actual realloc tail invokes _M_deallocate directly, without a third
	// allocator getter. Keep the two genuine relocation getters above.
	(complete_recipe_vector_deallocate_graph_source<T> - 2 * P);
}
constexpr size_t item_complete_general_entities_key_push_fitting_source =
	item_general_entity_vector_source_detail::push_public<critical_entity_key> +
	item_general_entity_vector_source_detail::single_construct<critical_entity_key>;
constexpr size_t item_complete_general_entities_key_push_growing_source =
	item_general_entity_vector_source_detail::push_public<critical_entity_key> +
	item_general_entity_vector_source_detail::growth_core<critical_entity_key>;
constexpr size_t item_complete_general_entities_key_push_shared_source =
	// Only exact typed public push and traits/construct-at subtree. This
	// matches all fitting declarations retained by the growing path.
	item_general_entity_vector_source_detail::push_public<critical_entity_key> +
	item_general_entity_vector_source_detail::single_construct<critical_entity_key>;
constexpr size_t item_complete_general_entities_key_push_source =
	item_complete_general_entities_key_push_fitting_source +
	item_complete_general_entities_key_push_growing_source -
	item_complete_general_entities_key_push_shared_source;
constexpr size_t item_complete_general_entities_revision_push_fitting_source =
	item_general_entity_vector_source_detail::push_public<critical_expected_revision> +
	// Actual std::move, emplace(this,arg,reference result), outer forward.
	2 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	item_general_entity_vector_source_detail::single_construct<critical_expected_revision> +
	item_general_entity_vector_source_detail::return_back<critical_expected_revision>;
constexpr size_t item_complete_general_entities_revision_push_growing_source =
	item_general_entity_vector_source_detail::push_public<critical_expected_revision> +
	2 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	item_general_entity_vector_source_detail::growth_core<critical_expected_revision> +
	item_general_entity_vector_source_detail::return_back<critical_expected_revision>;
constexpr size_t item_complete_general_entities_revision_push_shared_source =
	// Conservatively share only two genuinely identical typed method families.
	// Both possible outer move/emplace/back occurrences remain individually
	// represented; this is not a max of unrelated per-call paths.
	item_general_entity_vector_source_detail::push_public<critical_expected_revision> +
	item_general_entity_vector_source_detail::single_construct<critical_expected_revision>;
constexpr size_t item_complete_general_entities_revision_push_source =
	item_complete_general_entities_revision_push_fitting_source +
	item_complete_general_entities_revision_push_growing_source -
	item_complete_general_entities_revision_push_shared_source;
template <class T> constexpr size_t item_complete_general_entities_pop_source =
	// pop receiver; actual traits destroy(A&,T*) and destroy_at(T*) only.
	// Trivial implicit T destructor has no declared callable Source body.
	4 * sizeof(void *);
template <class T> constexpr size_t item_complete_general_entities_fresh_assign_source =
	// Selected fresh vector initializer-list assignment of exactly two trivial
	// elements. IL formal/backing array are caller INLINE, excluded here.
	// Public assignment2P; actual IL pointer/count ctor and begin/end/begin/size.
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) + 7 * sizeof(void *) +
	sizeof(size_t) +
	// forward aux this/first/last/tmp, len,F plus sliced RA; raw distance.
	4 * sizeof(void *) + sizeof(size_t) + sizeof(std::forward_iterator_tag) +
	sizeof(std::random_access_iterator_tag) + 5 * sizeof(void *) + 2 * sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(size_t) +
	6 * sizeof(void *) +
	// check_init converted allocator object, const-copy/cleanup, max/traits/min.
	10 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(std::allocator<T>) + 5 * sizeof(void *) +
	sizeof(size_t) + 11 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(bool) +
	// raw const T* -> T* uninitialized copy; actual Num2 skips assign_one.
	46 * sizeof(void *) + sizeof(std::ptrdiff_t) + 4 * sizeof(bool) +
	// Null old Destroy still reaches its actual runtime predicate; null
	// deallocate reaches only its root, no allocator/delete child.
	7 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_general_validated_entities_source_query_frame_bytes() noexcept
{
	// Full getter output/result/child, result+policy and checked-add signature;
	// child query getter families are genuinely reached before their bodies.
	return 2 * sizeof(void *) + 3 * sizeof(size_t) + 3 * sizeof(bool) +
	       item_owner_key_source_query_frame_bytes() +
	       item_transfer_craft_outputs_decode_complete_source_query_frame_bytes();
}
constexpr size_t item_complete_general_entities_craft_child_declaration_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
struct item_general_entities_complete_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *command = nullptr;
	const std::vector<uint64_t> *roots = nullptr;
	const std::vector<player_item_snapshot> *outputs = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		size_t total = outer, row = 0;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames))
			return false;
		if (command &&
		    (!payload_clone_add(total, sizeof(*command)) ||
		     !payload_clone_vector_heap(command->keys, false, total) ||
		     !payload_clone_vector_heap(command->expected_revisions, false, total) ||
		     !payload_clone_vector_heap(command->payload, false, total) ||
		     !payload_clone_vector_heap(command->accounting_intent, false, total)))
			return false;
		if (roots && (!payload_clone_add(total, sizeof(*roots)) ||
			      !payload_clone_vector_heap(*roots, false, total)))
			return false;
		if (outputs)
		{
			if (!payload_clone_add(total, sizeof(*outputs)) ||
			    !payload_clone_vector_heap(*outputs, false, total))
				return false;
			for (const auto &item : *outputs)
				if (!player_item_snapshot_current_heap_bytes(item, &row) ||
				    !payload_clone_add(total, row))
					return false;
		}
		if (!payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count) const noexcept
	{
		size_t request = 0;
		if (count > value.max_size() - value.size())
			return false;
		if (count > value.capacity() - value.size())
		{
			size_t next = value.size();
			if (!payload_clone_add(next, std::max(value.size(), count)) ||
			    next > value.max_size())
				next = value.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !payload_clone_add(request, next * sizeof(T)))
				return false;
		}
		return peak(request);
	}
	template <typename T> bool fresh(size_t count, size_t extra = 0) const noexcept
	{
		size_t request = 0;
		return count <= SIZE_MAX / sizeof(T) &&
		       payload_clone_add(request, count * sizeof(T)) &&
		       payload_clone_add(request, extra) && peak(request);
	}
	template <typename T, typename Comparator> bool sort_frame(size_t count) const noexcept
	{
		size_t levels = 0, remaining = count, request = 0;
		while (remaining > 1)
		{
			remaining >>= 1;
			++levels;
		}
		constexpr size_t recursive =
			item_complete_general_entities_sort_recursive_source<T, Comparator>;
		// Nonrecursive genuine Source is already retained in frames. Admit
		// the actual depth-dependent overlapping introsort declarations here.
		return 2 * levels + 1 <= SIZE_MAX / recursive &&
		       payload_clone_add(request, (2 * levels + 1) * recursive) && peak(request);
	}
};
struct item_payload_validation_complete_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const std::vector<player_item_snapshot> *outputs = nullptr;
	const craft_pouch_mutation *pouch = nullptr;
	const craft_recipe_continuation *recipe = nullptr;
	bool prefix(size_t &output, size_t extra = 0) const noexcept
	{
		size_t total = outer, row = 0;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames))
			return false;
		if (outputs)
		{
			if (!payload_clone_add(total, sizeof(*outputs)) ||
			    !payload_clone_vector_heap(*outputs, false, total))
				return false;
			for (const auto &item : *outputs)
				if (!player_item_snapshot_current_heap_bytes(item, &row) ||
				    !payload_clone_add(total, row))
					return false;
		}
		if (pouch)
		{
			if (!payload_clone_add(total, sizeof(*pouch)) ||
			    !payload_clone_vector_heap(pouch->usage, false, total) ||
			    !player_item_snapshot_current_heap_bytes(pouch->before, &row) ||
			    !payload_clone_add(total, row) ||
			    !player_item_snapshot_current_heap_bytes(pouch->after, &row) ||
			    !payload_clone_add(total, row))
				return false;
		}
		if (recipe &&
		    (!payload_clone_add(total, sizeof(*recipe)) ||
		     !payload_clone_vector_heap(recipe->pouch_mutation, false, total) ||
		     !payload_clone_vector_heap(recipe->refine_root_order, false, total) ||
		     !payload_clone_string_heap(recipe->refine_material_name, false, total)))
			return false;
		if (!payload_clone_add(total, extra))
			return false;
		output = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
using item_complete_collector_ci = std::vector<uint64_t>::const_iterator;
using item_complete_collector_difference =
	typename std::iterator_traits<item_complete_collector_ci>::difference_type;
using item_complete_collector_less = __gnu_cxx::__ops::_Iter_less_iter;
using item_complete_collector_equal = __gnu_cxx::__ops::_Iter_equal_to_iter;
constexpr size_t item_complete_collector_is_sorted_source =
	13 * sizeof(void *) + 10 * sizeof(item_complete_collector_ci) +
	sizeof(item_complete_collector_less) + 3 * sizeof(bool);
constexpr size_t item_complete_collector_adjacent_source =
	13 * sizeof(void *) + 8 * sizeof(item_complete_collector_ci) +
	sizeof(item_complete_collector_equal) + 2 * sizeof(bool);
constexpr size_t item_complete_collector_operation_zero_source =
	// Original zero predicate: argument/range refs and two raw loop iterators,
	// copied byte and Boolean return; const array begin/end each own4P.
	12 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool);
constexpr size_t item_complete_collector_absent_empty_source =
	13 * sizeof(void *) + 2 * sizeof(item_complete_collector_ci) + 2 * sizeof(bool);
constexpr size_t item_complete_collector_present_caller_source =
	// Seven actual const begin/end calls each own thisP plus explicit pointer
	// constructor2P. Returned objects alias algorithm formals except the final
	// adjacent comparison RHS, whose iterator object and comparison remain.
	21 * sizeof(void *) + sizeof(item_complete_collector_ci) + 6 * sizeof(void *) +
	sizeof(bool) + sizeof(void *) + sizeof(size_t);
namespace item_complete_general_control_recipe = duris_craft_recipe_bounded_detail;
template <class T> constexpr size_t item_complete_general_vector_current_source =
	// Actual payload_clone_vector_heap(false): value/bytes refs,fresh/count/result,
	// then selected capacity this/returnedN. size() true branch unselected.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) +
	item_complete_general_control_recipe::complete_recipe_vector_capacity_source<T>;
constexpr size_t item_complete_general_string_current_source =
	// Actual payload_clone_string_heap(false), string capacity/is_local and
	// const data/local/pointer_to/addressof/__addressof. Same exact published
	// recovery/current family, no fresh string size() or allocation imported.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	11 * sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_general_payload_current_source =
	// Actual payload_clone_heap(false): value/bytes refs,fresh/returnedbool.
	// Its six strings/six vectors reduce only completed same-type families;
	// each physical payload and all capacities remain separately in prefix.
	2 * sizeof(void *) + 2 * sizeof(bool) + item_complete_general_string_current_source +
	item_complete_general_vector_current_source<uint8_t> +
	item_complete_general_vector_current_source<uint64_t> +
	item_complete_general_vector_current_source<native_quest_cost_attempt>;
constexpr size_t item_complete_general_control_prefix_peak_source =
	// Actual complete classes remove original observation local only.
	// Prefix(this,out,extra,total,B), peak(this,extra,total,B), add+callback.
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool));
constexpr size_t item_complete_general_validation_controller_source =
	// Exact published own/range/current-row/row-byte-u64 false observer union.
	// Same const row vector range and same current-row getter declarations.
	item_complete_native_validation_current_controller_source +
	// Pouch usage adds a distinct actual vector<T> observer; byte and u64
	// recipe vectors already share published typed observer families.
	item_complete_general_vector_current_source<chaos_material_pouch_usage> +
	// Actual payload_clone_string_heap(false) is additionally selected by
	// recipe material-name. Its declared scope and capacity-chain occurrence
	// stay separately retained; no opaque row margin is subtracted.
	item_complete_general_string_current_source;
constexpr size_t item_complete_general_wire_controller_source =
	item_complete_general_control_prefix_peak_source +
	item_complete_general_payload_current_source;
constexpr size_t item_complete_general_decode_controller_source =
	item_complete_general_control_prefix_peak_source +
	item_complete_general_payload_current_source +
	item_complete_general_vector_current_source<critical_entity_key> +
	item_complete_general_vector_current_source<critical_expected_revision>;
constexpr size_t item_complete_general_public_encode_controller_source =
	// Only actual private canonical byte vector is registered.
	item_complete_general_control_prefix_peak_source +
	item_complete_general_vector_current_source<uint8_t>;
constexpr size_t item_complete_general_public_decode_controller_source =
	// All real branch registers: copied original command, payload candidate,
	// canonical. Same completed byte false observer covers command fields.
	item_complete_general_decode_controller_source +
	// command_copy this/source/request/result, three actual fresh-size types.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	item_complete_general_control_recipe::complete_recipe_vector_size_source<
		critical_entity_key> +
	item_complete_general_control_recipe::complete_recipe_vector_size_source<
		critical_expected_revision> +
	item_complete_general_control_recipe::complete_recipe_vector_size_source<uint8_t>;
constexpr size_t item_complete_general_native_cost_encode_controller_source =
	// Original payload + first/second/value bytes actually registered;
	// existing payload false observer owns the same byte typed family.
	item_complete_general_wire_controller_source +
	// Actual NQF1 owned encoder never calls controller growth. Its fresh
	// clone_payload request does genuinely select byte size() below.
	item_complete_general_control_recipe::complete_recipe_vector_size_source<uint8_t> +
	// clone_payload(this,source,request,result), fresh public getter own2P,N,B
	// + actual policy resultB. Heap root and typed heap helper declarations
	// already retained in false union; fresh selects only new size getters.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(bool) +
	item_complete_general_control_recipe::complete_recipe_vector_size_source<uint64_t> +
	item_complete_general_control_recipe::complete_recipe_vector_size_source<
		native_quest_cost_attempt> +
	sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_validation_root_count_source =
	// Same genuine mutable row iterator and exact published root predicate.
	// Two distinct begin/end receiver+explicit pointer constructor calls3P
	// each; class results directly initialize count_if by-value formals.
	item_complete_recovery_count_graph_source + 6 * sizeof(void *);
struct item_complete_validation_ancestor_predicate
{
	uint64_t &ancestor_uid;
	bool operator()(const item_transfer_entry &candidate) const
	{
		return candidate.item_uid == ancestor_uid;
	}
};
namespace item_general_validator_lifetime_source = duris_craft_recipe_bounded_detail;
template <class T> constexpr size_t item_general_validator_vector_deallocate_source =
	// _M_deallocate, allocator_traits::deallocate, allocator::deallocate,
	// new_allocator::deallocate each owns this/alloc, pointer, count.
	4 * (2 * sizeof(void *) + sizeof(size_t)) +
	// Actual sized operator delete pointer/count. Const-evaluation alternate
	// unsized delete is not selected; single helper bool is actual query.
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
template <class T> constexpr size_t item_general_validator_vector_cleanup_source =
	// Actual vector, Vector_base, generated nontrivial Vector_impl and
	// allocator destructor receivers. Impl_data has no declared destructor;
	// new_allocator destructor is excluded by the C++20 preprocessor path.
	// Neither contributes a runtime selected destructor scope.
	4 * sizeof(void *) +
	// _Destroy(first,last,allocator), _Destroy(first,last) and selected aux:
	// no invented cursor local: aux<false> increments its first parameter.
	3 * sizeof(void *) + 2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
	// Mutable get_Tp_allocator receiver/reference result.
	2 * sizeof(void *) + item_general_validator_vector_deallocate_source<T> +
	(std::is_trivially_destructible_v<T> ?
		 0 :
		 // Nontrivial aux reaches __addressof, _Destroy(pointer), destroy_at
		 // and the genuine T destructor receiver. Member cleanup is separate;
		 // raw-pointer ++ and == use no iterator object/helper family.
		 2 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(void *));
constexpr size_t item_complete_general_validator_pouch_default_cleanup_source =
	// Plain craft_pouch_mutation contributes enclosing default/dtor2P.
	// before/after={} directly default row string + DA/ED member vectors,
	// with no row default receiver. ED default belongs to list child only.
	2 * sizeof(void *) + 3 * (6 * sizeof(void *)) +
	item_general_validator_vector_cleanup_source<chaos_material_pouch_usage> +
	item_general_validator_vector_cleanup_source<player_item_dynamic_affect_snapshot> +
	item_general_validator_vector_cleanup_source<player_item_extra_description_snapshot> +
	item_general_validator_vector_cleanup_source<int32_t>;
constexpr size_t item_complete_general_validator_outputs_default_cleanup_source =
	// Actual outer outputs vector default and nontrivial row destruction;
	// this selected graph owns the generated row destructor receiver once.
	6 * sizeof(void *) + item_general_validator_vector_cleanup_source<player_item_snapshot>;
constexpr size_t item_complete_general_validator_recipe_default_cleanup_source =
	// Plain recipe and refine_cost DMI two default receivers, recipe dtorP.
	// Byte/u64 default and cleanup reached, without recipe COPY/MOVE.
	3 * sizeof(void *) + 2 * (6 * sizeof(void *)) +
	item_general_validator_lifetime_source::member_vector_cleanup_source<uint8_t>() +
	item_general_validator_lifetime_source::member_vector_cleanup_source<uint64_t>();
constexpr size_t item_complete_general_validator_shared_char_lifetime_source =
	// Row and nested ED fields plus optional recipe string reach the same
	// completed char-string default/cleanup method families, retained once.
	item_general_validator_lifetime_source::member_string_default_source +
	item_general_validator_lifetime_source::member_string_destroy_source;
constexpr size_t item_complete_general_validator_empty_cost_rhs_lifetime_source =
	// Actual native_quest_cost_projection{} temporary has attempts default,
	// nontrivial enclosing dtorP and real null vector cleanup15P+N+B.
	// No enclosing aggregate constructor or nonnull delete tail is selected.
	7 * sizeof(void *) + 15 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_general_validator_local_lifetime_source =
	item_complete_general_validator_pouch_default_cleanup_source +
	item_complete_general_validator_outputs_default_cleanup_source +
	item_complete_general_validator_recipe_default_cleanup_source +
	item_complete_general_validator_shared_char_lifetime_source +
	item_complete_general_validator_empty_cost_rhs_lifetime_source;
constexpr size_t item_complete_general_validator_own_source =
	// Four index declaration sites, depth/admission6N, 22 real named bools
	// plus returned bool; outputs hidden actual mutable I begin/end objects.
	10 * sizeof(void *) + 6 * sizeof(size_t) + 2 * sizeof(uint32_t) + 10 * sizeof(uint64_t) +
	sizeof(uint16_t) + 23 * sizeof(bool) +
	2 * sizeof(std::vector<player_item_snapshot>::iterator);
constexpr size_t item_complete_general_wire_encode_own_source =
	6 * sizeof(void *) + 8 * sizeof(size_t) + sizeof(uint16_t) + sizeof(uint64_t) +
	sizeof(bool);
constexpr size_t item_complete_general_generic_decode_own_source =
	5 * sizeof(void *) + 10 * sizeof(size_t) + 3 * sizeof(uint32_t) + 3 * sizeof(bool);
constexpr size_t item_complete_general_vector_range_span_source =
	// span(Range&&) receiver/ref2P; _Data CPO this/ref/pointer3P;
	// const vector.data2P and raw _M_data_ptr this/arg/result3P;
	// _Size CPO this/ref/size2P+N and const vector.size P+N;
	// delegated raw pointer/count2P+N, to_address/raw __to_address4P,
	// dynamic extent ctor P+N. CPO globals are not new temporary objects.
	20 * sizeof(void *) + 4 * sizeof(size_t);
constexpr size_t item_complete_general_default_subspan_source =
	// subspan(fixed) default-count route: this/offset/count P+2N;
	// genuine nonassert internal queryB, actual size/dynamicextent2P+2N,
	// data2P, downstream pointer/count constructor closure7P+2N.
	// Assertion RHS size() is short-circuited, but size()-offset executes.
	12 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_general_valid_wrapper_own_source =
	// payload/context/catch refs3P, actual callback F, outer/frames2N.
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 2 * sizeof(size_t) +
	sizeof(uint16_t) + sizeof(bool);
constexpr size_t item_complete_general_encode_wrapper_own_source =
	// payload/encoded/context/retained-heap/catch5P plus callback F;
	// outer/frames/heap3N and actual version/returned Boolean.
	5 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 3 * sizeof(size_t) +
	sizeof(uint16_t) + sizeof(bool);
constexpr size_t item_complete_general_decode_wrapper_own_source =
	// command/payload/context/retained-heap/catch5P plus callback F;
	// outer/frames/heap3N and actual Boolean result.
	5 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 3 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_general_decoder_reference_span_constructor_source =
	// Dynamic pointer/count span: receiver/byvalue pointer/count2P+N,
	// to_address/raw to_address4P, dynamic extent ctorP+N.
	// Destination S is the existing reference adapter formal, not extra INLINE.
	7 * sizeof(void *) + 2 * sizeof(size_t);
constexpr size_t item_complete_general_decoder_recovery_span_source =
	item_complete_general_vector_range_span_source +
	item_complete_general_default_subspan_source +
	// Distinct outer vector-range temporary is a subspan receiver and has
	// explicit destructorP. Its S storage is retained by actual wrapper.
	sizeof(void *);
constexpr size_t item_complete_general_validator_recipe_span_source =
	// Direct vector-range result initializes recipe adapter's S formal once.
	// Its onward declared copy/destructor already belong adapter own27.
	item_complete_general_vector_range_span_source;
constexpr size_t item_complete_general_policy_query_source = sizeof(bool);
template <class T> constexpr size_t item_complete_generic_validator_integral_equal_tail_source =
	// Same selected declared __equal_aux1, __equal<true> and __memcmp scopes
	// for byte/int64/u64 integer pointers. Actual constant __simple object
	// exists; its traits initializer introduces no runtime callee.
	3 * sizeof(void *) + 2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(bool);
template <class T, size_t Count>
constexpr size_t item_complete_generic_validator_integral_array_equal_source =
	// Actual array<T,Count> == refs/result; three const begin/end/data calls,
	// raw equal/aux, three raw niter and genuine typed selected true tail.
	2 * sizeof(void *) + sizeof(bool) + 12 * sizeof(void *) + 6 * sizeof(void *) +
	2 * sizeof(bool) + 6 * sizeof(void *) +
	item_complete_generic_validator_integral_equal_tail_source<T>;
constexpr size_t item_complete_generic_validator_empty_attempt_vector_equal_source =
	// Native money compares projection against a fresh projection{} whose
	// attempts size0. Size mismatch short-circuits; only equal-length zero
	// reaches __equal<false>. Its element loop is unexecuted, so no attempt,
	// requirement or nested cash comparisons belong to this selected lane.
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	9 * sizeof(void *) + 6 * sizeof(std::vector<native_quest_cost_attempt>::const_iterator) +
	2 * sizeof(bool) + 3 * sizeof(std::vector<native_quest_cost_attempt>::const_iterator) +
	9 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(bool) + 3 * sizeof(void *) +
	sizeof(bool);
constexpr size_t item_complete_generic_validator_coin_projection_equal_source =
	// Explicit defaulted equality this/other/result. Four same array<int64,4>
	// members call their exact selected family sequentially; reuse only after
	// each completed call. All four physical arrays remain input/output DTO.
	2 * sizeof(void *) + sizeof(bool) +
	item_complete_generic_validator_integral_array_equal_source<int64_t, 4>;
constexpr size_t item_complete_generic_validator_empty_cost_projection_equal_source =
	2 * sizeof(void *) + sizeof(bool) +
	// Two exact cash-array member calls sequentially reuse their family.
	item_complete_generic_validator_integral_array_equal_source<int64_t, 4> +
	item_complete_generic_validator_empty_attempt_vector_equal_source;
constexpr size_t item_complete_generic_validator_projection_equal_source =
	item_complete_generic_validator_coin_projection_equal_source +
	item_complete_generic_validator_empty_cost_projection_equal_source -
	// Same array<int64_t,4> equality family completes in initial money
	// comparison before the cost comparison; actual RHS DTOs stay separate.
	item_complete_generic_validator_integral_array_equal_source<int64_t, 4>;
constexpr size_t item_complete_generic_validator_recipe_output_selectors_source =
	// Actual const empty() family with two CI operands, size() and mutable
	// outputs[] methods; real output object/heap and count-if stay external.
	13 * sizeof(void *) + 2 * sizeof(std::vector<player_item_snapshot>::const_iterator) +
	2 * sizeof(bool) + sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) + sizeof(size_t) +
	sizeof(craft_recipe_discipline) + sizeof(bool);
namespace item_general_recipe_matches_detail
{
using namespace duris_craft_recipe_bounded_detail;
using complete_recipe_matches_root_ci = std::vector<uint64_t>::const_iterator;
constexpr size_t complete_recipe_matches_own_source =
	// terms/payload, entry pointer and hidden range reference; materials,
	// ores,roots and two separate i declarations; last_ore and actual uid;
	// hidden begin/end iterators; bool result. The two returned class values
	// initialize those exact hidden iterator objects by mandatory elision.
	4 * source_P + 5 * source_N + sizeof(uint32_t) + sizeof(uint64_t) +
	2 * sizeof(complete_recipe_matches_root_ci) + source_B;
constexpr size_t complete_recipe_matches_root_iterator_source =
	// begin/end returned CI storage is already the hidden objects above.
	// Real getter receivers and explicitly declared pointer ctors remain.
	2 * (source_P +
	     complete_recipe_normal_pointer_constructor_source<
		     complete_recipe_matches_root_ci>)+complete_recipe_normal_equal_own_source<complete_recipe_matches_root_ci> +
	2 * complete_recipe_normal_base_source<complete_recipe_matches_root_ci> +
	complete_recipe_normal_preincrement_source<complete_recipe_matches_root_ci> +
	complete_recipe_normal_dereference_source<complete_recipe_matches_root_ci>;
constexpr size_t complete_recipe_matches_byte_empty_source =
	// Actual const empty()->begin/end()->operator==. Unlike the root range,
	// these returned CIs are separate real temporary comparison operands.
	complete_recipe_vector_empty_own_source<uint8_t> +
	complete_recipe_vector_const_begin_own_source<uint8_t> +
	complete_recipe_vector_const_end_own_source<uint8_t> +
	2 * complete_recipe_normal_pointer_constructor_source<complete_recipe_byte_const_iterator> +
	complete_recipe_normal_equal_own_source<complete_recipe_byte_const_iterator> +
	2 * complete_recipe_normal_base_source<complete_recipe_byte_const_iterator>;
constexpr size_t complete_recipe_matches_refine_validation_source =
	// Exactly refine_terms_valid's selected descendants; no is_alchemy,
	// u64 empty(), recipe/cost default construction or codec call is reached.
	complete_recipe_refine_succeeded_own_source + complete_recipe_refine_cost_valid_own_source +
	complete_recipe_refine_terms_valid_own_source +
	complete_recipe_refine_cost_compare_own_source +
	complete_recipe_array_const_i32_access_source +
	complete_recipe_array_const_i64_access_source + complete_recipe_array_i32_equal_source +
	complete_recipe_matches_byte_empty_source + complete_recipe_vector_size_source<uint64_t> +
	complete_recipe_vector_const_subscript_source<uint64_t> +
	complete_recipe_string_size_source + complete_recipe_string_empty_source +
	complete_recipe_string_find_own_source + complete_recipe_string_const_data_source +
	complete_recipe_traits_char_find_source;
constexpr size_t complete_recipe_matches_item_array_source =
	// Actual std::array<item_transfer_entry,...>::const operator[] direct
	// this/index/returned-reference; captured GNU13 directly indexes elems.
	// No old _AT_Type::_S_ref or array ctor/dtor is selected.
	2 * source_P + source_N;
constexpr size_t complete_recipe_matches_owner_identity_source =
	// Original item_owner_identity_equal(left,right): two refs/bool result.
	// Scalar field comparisons reach no extra C++ callable helper.
	2 * source_P + source_B;
constexpr size_t complete_recipe_matches_full_source =
	complete_recipe_matches_own_source + complete_recipe_matches_root_iterator_source +
	complete_recipe_matches_refine_validation_source +
	complete_recipe_matches_item_array_source + complete_recipe_matches_owner_identity_source;
}
constexpr size_t item_complete_general_mobile_valid_child_own_source =
	2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(uint16_t) + sizeof(bool);
constexpr size_t item_complete_general_recovery_valid_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(uint16_t) + sizeof(bool);
constexpr size_t item_complete_general_offering_valid_child_own_source =
	2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_general_duplicate_valid_child_own_source =
	2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(uint16_t) + sizeof(bool);
constexpr size_t item_complete_general_lockpick_valid_child_own_source =
	2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_general_craft_outputs_child_own_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_general_pouch_payload_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_general_recipe_decode_child_own_source =
	sizeof(std::span<const uint8_t>) + 2 * sizeof(void *) +
	sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) + sizeof(bool) +
	3 * sizeof(void *);
constexpr size_t item_complete_general_byte_size_source = sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_general_byte_const_index_source =
	2 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_general_byte_const_data_source = 5 * sizeof(void *);
constexpr size_t item_complete_general_entry_const_index_source =
	2 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_general_get_u32_source =
	sizeof(void *) + 2 * sizeof(uint32_t) + sizeof(unsigned int);
constexpr size_t item_complete_general_get_u64_source =
	sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(unsigned int);
constexpr size_t item_complete_general_soulbind_source =
	sizeof(void *) + sizeof(bool) + item_complete_general_byte_size_source +
	item_complete_general_byte_const_index_source + item_complete_offering_find_source;
constexpr size_t item_complete_general_spell_source =
	// Actual payload/data refs, version, effect_offset, effect, legacy/result.
	// Five size/nine index syntax sites reuse matching completed families.
	2 * sizeof(void *) + sizeof(uint16_t) + sizeof(size_t) + sizeof(uint32_t) +
	2 * sizeof(bool) + item_complete_general_byte_size_source +
	item_complete_general_byte_const_index_source;
constexpr size_t item_complete_general_account_retirement_source =
	// Actual payload/data/item refs, payload_version/version/nested_uid/result.
	3 * sizeof(void *) + sizeof(uint16_t) + sizeof(uint32_t) + 2 * sizeof(bool) +
	item_complete_general_byte_size_source + item_complete_general_byte_const_data_source +
	item_complete_general_entry_const_index_source + item_complete_general_get_u32_source +
	item_complete_general_get_u64_source;
constexpr size_t item_complete_general_selected_root_own_source =
	// Payload ref; craft/noncraft distinct entry and parent pointers;
	// two lexical depth locals; input UID and genuine returned scalar UID.
	5 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(uint64_t);
constexpr size_t item_complete_general_selected_root_source =
	item_complete_general_selected_root_own_source +
	item_complete_general_entry_const_index_source + item_complete_offering_find_source;
constexpr size_t item_complete_general_target_topology_source =
	// Payload/root/parent refs plus entry pointer; input UID+selected_root;
	// genuine Boolean result. Direct lookup ends before selected-root child.
	4 * sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(bool) +
	item_complete_general_selected_root_source;
constexpr size_t item_complete_general_reason_source = sizeof(item_transfer_reason) + sizeof(bool);
constexpr size_t item_complete_general_native_version_source = sizeof(uint16_t) + sizeof(bool);
constexpr size_t item_complete_general_owner_valid_source = sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_general_owner_equal_source = 2 * sizeof(void *) + sizeof(bool);
constexpr size_t item_complete_general_forced_drop_source =
	sizeof(item_transfer_reason) + sizeof(bool);
constexpr size_t item_complete_general_continuation_limit_source =
	sizeof(item_transfer_continuation_kind) + sizeof(size_t);
using item_complete_text_ci = std::string::const_iterator;
using item_complete_text_difference =
	typename std::iterator_traits<item_complete_text_ci>::difference_type;
constexpr size_t item_complete_text_endpoints_source =
	// Each actual begin/end: thisP, materialized pointer prvalueP,
	// _M_data this/returned pointer2P, explicit CI pointer ctor2P.
	// Their CI results are actual public all_of formals, not extra copies.
	// end's real size() family is held in the shared size row below.
	2 * (2 * sizeof(void *) +
	     duris_craft_recipe_bounded_detail::complete_recipe_string_const_data_source +
	     duris_craft_recipe_bounded_detail::complete_recipe_normal_pointer_constructor_source<
		     item_complete_text_ci>);
constexpr size_t item_complete_general_validator_small_helper_union_source =
	// Genuine same-type completed direct getter families, once each. Scalar
	// helper declarations remain distinct, with no whole-provider subtraction.
	item_complete_general_byte_size_source + item_complete_general_byte_const_index_source +
	item_complete_general_byte_const_data_source +
	item_complete_general_entry_const_index_source + item_complete_general_get_u32_source +
	item_complete_general_get_u64_source +
	// Soulbind own, spell own, account-retirement own, topology helpers own.
	sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(uint16_t) + sizeof(size_t) +
	sizeof(uint32_t) + 2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(uint16_t) +
	sizeof(uint32_t) + 2 * sizeof(bool) + item_complete_general_selected_root_own_source +
	4 * sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(bool) +
	item_complete_general_reason_source + item_complete_general_native_version_source +
	item_complete_general_owner_valid_source + item_complete_general_owner_equal_source +
	item_complete_general_forced_drop_source + item_complete_general_continuation_limit_source +
	// Lookup completes before all subsequent calls; one genuine selected
	// raw-array/named comparator family serves soulbind/topology/collector.
	item_complete_offering_find_source;
constexpr size_t item_complete_general_validator_direct_source =
	// Actual .none continuation check reaches empty: both real byte CIs plus
	// endpoint/constructor/comparison/base families. blob.size direct array.
	13 * sizeof(void *) + 2 * sizeof(std::vector<uint8_t>::const_iterator) + 2 * sizeof(bool) +
	sizeof(void *) + sizeof(size_t);
struct item_generic_decode_complete_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const item_transfer_payload *payload = nullptr;
	const critical_command *expected = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    (payload && (!payload_clone_add(total, sizeof(*payload)) ||
				 !payload_clone_heap(*payload, false, total))) ||
		    (expected &&
		     (!payload_clone_add(total, sizeof(*expected)) ||
		      !payload_clone_vector_heap(expected->keys, false, total) ||
		      !payload_clone_vector_heap(expected->expected_revisions, false, total) ||
		      !payload_clone_vector_heap(expected->payload, false, total) ||
		      !payload_clone_vector_heap(expected->accounting_intent, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
struct item_payload_wire_complete_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const std::vector<uint8_t> *bytes = nullptr, *corpse = nullptr, *collector = nullptr,
				   *recovery = nullptr;
	const item_transfer_payload *payload = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames))
			return false;
		if ((bytes && (!payload_clone_add(total, sizeof(*bytes)) ||
			       !payload_clone_vector_heap(*bytes, false, total))) ||
		    (corpse && (!payload_clone_add(total, sizeof(*corpse)) ||
				!payload_clone_vector_heap(*corpse, false, total))) ||
		    (collector && (!payload_clone_add(total, sizeof(*collector)) ||
				   !payload_clone_vector_heap(*collector, false, total))) ||
		    (recovery && (!payload_clone_add(total, sizeof(*recovery)) ||
				  !payload_clone_vector_heap(*recovery, false, total))) ||
		    (payload && (!payload_clone_add(total, sizeof(*payload)) ||
				 !payload_clone_heap(*payload, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
constexpr size_t item_complete_general_payload_valid_child_own_source =
	2 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(uint16_t) + sizeof(bool);
constexpr size_t item_complete_general_validated_entities_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
static_assert(std::is_trivial_v<critical_entity_key> &&
	      std::is_trivial_v<critical_expected_revision>);
static_assert(std::is_trivially_copyable_v<critical_entity_key> &&
	      std::is_trivially_copyable_v<critical_expected_revision>);
static_assert(std::is_trivially_destructible_v<critical_entity_key> &&
	      std::is_trivially_destructible_v<critical_expected_revision>);
namespace item_complete_wire_copy_recipe = duris_craft_recipe_bounded_detail;
using item_complete_wire_copy_i = std::vector<uint8_t>::iterator;
using item_complete_wire_copy_ci = std::vector<uint8_t>::const_iterator;
template <class Input, class Output, class RawInput>
constexpr size_t item_complete_wire_copy_n_source =
	// Public copy_n: first,n,result,n2 and scalar result where applicable.
	sizeof(Input) + sizeof(Output) + 2 * sizeof(uint32_t) +
	(std::is_pointer_v<Output> ? sizeof(Output) : 0) +
	// Selected unsigned __size_to_integer parameter and returned scalar.
	2 * sizeof(uint32_t) +
	// iterator_category receiver; RA result aliases __copy_n tag formal.
	sizeof(void *) +
	// __copy_n RA: first,n,result,tag and scalar result where applicable.
	sizeof(Input) + sizeof(uint32_t) + sizeof(Output) +
	sizeof(std::random_access_iterator_tag) + (std::is_pointer_v<Output> ? sizeof(Output) : 0) +
	// Normal first+n is real operator+ with a materialized raw pointer
	// and explicit pointer constructor; its class result is copy's formal.
	(std::is_pointer_v<Input> ?
		 0 :
		 4 * sizeof(void *) +
			 sizeof(typename std::iterator_traits<Input>::difference_type)) +
	item_complete_wire_copy_recipe::complete_recipe_copy_graph_source<false, Input, Output,
									  RawInput>;
constexpr size_t item_complete_generic_wire_copy_source =
	item_complete_wire_copy_n_source<const uint8_t *, item_complete_wire_copy_i,
					 const uint8_t *> +
	// Corpse and collector: two genuine completed mutable-input calls.
	2 * (item_complete_wire_copy_recipe::complete_recipe_copy_graph_source<
		     false, item_complete_wire_copy_i, item_complete_wire_copy_i, uint8_t *> -
	     item_complete_wire_copy_recipe::complete_recipe_byte_assign_one_source) +
	item_complete_wire_copy_recipe::complete_recipe_copy_graph_source<
		false, item_complete_wire_copy_ci, item_complete_wire_copy_i, const uint8_t *> +
	// Fixed native reference raw range and actual mutable recovery range.
	(item_complete_wire_copy_recipe::complete_recipe_copy_graph_source<false, uint8_t *,
									   uint8_t *, uint8_t *> -
	 item_complete_wire_copy_recipe::complete_recipe_byte_assign_one_source) +
	(item_complete_wire_copy_recipe::complete_recipe_copy_graph_source<
		 false, item_complete_wire_copy_i, uint8_t *, uint8_t *> -
	 item_complete_wire_copy_recipe::complete_recipe_byte_assign_one_source);
constexpr size_t item_complete_generic_wire_copy_caller_source =
	// Blob array begin/data4P; output begin/Pctor3P; two4P+D offsets.
	4 * sizeof(void *) + 3 * sizeof(void *) + 2 * (4 * sizeof(void *) + sizeof(ptrdiff_t)) +
	// Initial begin and first offset are distinct implicit-object I's;
	// final offset is public copy_n formal. Discarded root result I real.
	3 * sizeof(item_complete_wire_copy_i) +
	// Corpse/collector each input endpoints6P, output begin3P, two offsets,
	// two implicit-object I's and one discarded public result I.
	2 * (9 * sizeof(void *) + 2 * (4 * sizeof(void *) + sizeof(ptrdiff_t)) +
	     3 * sizeof(item_complete_wire_copy_i)) +
	// Continuation's two CI inputs directly initialize copy formals.
	9 * sizeof(void *) + 2 * (4 * sizeof(void *) + sizeof(ptrdiff_t)) +
	3 * sizeof(item_complete_wire_copy_i) +
	// Reference array begin/end each4P; scalar output already parent-owned.
	8 * sizeof(void *) +
	// Recovery two mutable endpoints3P each; scalar output already parent.
	6 * sizeof(void *);
constexpr size_t item_complete_generic_decode_blob_copy_source =
	item_complete_wire_copy_n_source<item_complete_wire_copy_ci, uint8_t *, const uint8_t *>;
constexpr size_t item_complete_generic_decode_blob_copy_caller_source =
	// command const begin3P, two offsets4P+D, two CI implicit receivers.
	// Final CI aliases copy_n formal; raw destination array begin/data4P.
	3 * sizeof(void *) + 2 * (4 * sizeof(void *) + sizeof(ptrdiff_t)) +
	2 * sizeof(item_complete_wire_copy_ci) + 4 * sizeof(void *);
constexpr size_t item_complete_general_wire_put16_source = sizeof(void *) + sizeof(uint16_t);
constexpr size_t item_complete_general_wire_put32_source =
	sizeof(void *) + sizeof(uint32_t) + sizeof(unsigned int);
constexpr size_t item_complete_general_wire_put64_source =
	sizeof(void *) + sizeof(uint64_t) + sizeof(unsigned int);
constexpr size_t item_complete_general_wire_get16_source = sizeof(void *) + sizeof(uint16_t);
constexpr size_t item_complete_general_wire_get32_source =
	sizeof(void *) + 2 * sizeof(uint32_t) + sizeof(unsigned int);
constexpr size_t item_complete_general_wire_get64_source =
	sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(unsigned int);
constexpr size_t item_complete_general_encode_owner_source =
	2 * sizeof(void *) + 2 * item_complete_general_wire_put64_source;
constexpr size_t item_complete_general_decode_owner_source =
	sizeof(void *) + 2 * item_complete_general_wire_get64_source;
constexpr size_t item_complete_general_decode_owner_result_inline = sizeof(item_owner_identity);
constexpr size_t item_complete_general_byte_data_source = 5 * sizeof(void *);
constexpr size_t item_complete_general_vector_size_source = sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_general_subscript_source = 2 * sizeof(void *) + sizeof(size_t);
constexpr size_t item_complete_general_encode_direct_wire_source =
	3 * item_complete_general_wire_put16_source + 8 * item_complete_general_wire_put32_source +
	12 * item_complete_general_wire_put64_source +
	2 * item_complete_general_encode_owner_source +
	19 * item_complete_general_byte_data_source +
	11 * item_complete_general_vector_size_source + 2 * item_complete_general_subscript_source;
constexpr size_t item_complete_general_decode_direct_wire_source =
	3 * item_complete_general_wire_get16_source + 10 * item_complete_general_wire_get32_source +
	12 * item_complete_general_wire_get64_source +
	2 * item_complete_general_decode_owner_source +
	22 * item_complete_general_byte_data_source +
	17 * item_complete_general_vector_size_source +
	3 * item_complete_general_vector_size_source +
	3 * item_complete_general_vector_size_source +
	1 * item_complete_general_vector_size_source + 1 * item_complete_general_subscript_source;
constexpr size_t item_complete_general_cost_direct_wire_source =
	2 * item_complete_general_wire_put16_source + 2 * item_complete_general_wire_put32_source +
	1 * item_complete_general_wire_put64_source + 5 * item_complete_general_byte_data_source +
	10 * item_complete_general_vector_size_source + 4 * item_complete_general_subscript_source;
struct item_complete_generic_key_equal
{
	bool operator()(const critical_entity_key &left, const critical_entity_key &right) const
	{
		return critical_entity_key_equal(left, right);
	}
};
struct item_complete_generic_revision_equal
{
	bool operator()(const critical_expected_revision &left,
			const critical_expected_revision &right) const
	{
		return critical_entity_key_equal(left.key, right.key) &&
		       left.revision == right.revision;
	}
};
constexpr size_t item_complete_generic_predicate_iterators_source =
	// Caller const begin/end and mutable begin:3 receiverP plus the
	// selected ctor this/reference6P. Stored pointer lvalues bind
	// reference-compatibly: no pointer qualification temporary.
	9 * sizeof(void *) +
	// Loop != is C++20 rewritten ==: two references+B; its two base calls
	// each have this/reference2P. No additional operator!= callable scope.
	6 * sizeof(void *) + sizeof(bool) +
	// Prefix++ on const/mutable each has this/returned-reference2P;
	// dereference on each likewise has this/returned-reference2P.
	8 * sizeof(void *);
template <class Predicate> constexpr size_t item_complete_generic_revisions_equal_source =
	item_complete_generic_predicate_iterators_source +
	// Public equal's actual input formals and genuine empty Predicate object.
	// Caller Predicate{} initializes this formal directly; no second object.
	2 * sizeof(std::vector<critical_expected_revision>::const_iterator) +
	sizeof(std::vector<critical_expected_revision>::iterator) + sizeof(Predicate) +
	sizeof(bool) +
	// Actual const Predicate::operator():this/two refs3P/resultB. Nested
	// key_equal refs2P/resultB. Scalar revision comparison has no callee.
	5 * sizeof(void *) + 2 * sizeof(bool);
template <class Predicate> constexpr size_t item_complete_generic_key_predicate_equal_source =
	item_complete_generic_predicate_iterators_source +
	// Public equal's actual input formals and genuine empty Predicate object.
	// Caller Predicate{} initializes this formal directly; no second object.
	2 * sizeof(std::vector<critical_entity_key>::const_iterator) +
	sizeof(std::vector<critical_entity_key>::iterator) + sizeof(Predicate) + sizeof(bool) +
	// Actual const Predicate::operator():this/two refs3P/resultB. Nested
	// key_equal refs2P/resultB. No scalar arithmetic has a callee.
	5 * sizeof(void *) + 2 * sizeof(bool);
constexpr size_t item_complete_generic_byte_equal_tail_source =
	// __equal_aux1 raw3P/resultB + actual const local __simpleB. Its traits
	// initializer is required constant-evaluated (template argument), with
	// no runtime trait callee. The automatic local object still exists.
	3 * sizeof(void *) + 2 * sizeof(bool) +
	// Selected __equal<true>::equal: raw3P/local size_t __len/resultB.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// __memcmp raw2P/countN/int result; genuine runtime public
	// std::is_constant_evaluated returnedB. No invented builtin callee.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(bool);
constexpr size_t item_complete_generic_canonical_equal_source =
	// C++20 != rewrites vector==:refs2P/resultB only. Const size twice:
	// two actual thisP/size_t-resultN occurrences (no old root size credit).
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	// Three const begin/end receivers3P and selected pointer-ref ctors6P.
	// Stored pointer lvalues bind reference-compatibly with no
	// qualification temporary. Class results are public equal formals.
	9 * sizeof(void *) +
	// public equal and __equal_aux: each3 real iterator formals/resultB.
	6 * sizeof(std::vector<uint8_t>::const_iterator) + 2 * sizeof(bool) +
	// Three normal-iterator niter calls:by-valueI/raw-pointer resultP,
	// each real base this/reference2P. Three actual distinct call occurrences.
	3 * sizeof(std::vector<uint8_t>::const_iterator) + 9 * sizeof(void *) +
	item_complete_generic_byte_equal_tail_source;
constexpr size_t item_complete_generic_command_equal_source =
	item_complete_generic_key_predicate_equal_source<item_complete_generic_key_equal> +
	item_complete_generic_revisions_equal_source<item_complete_generic_revision_equal>;
namespace item_complete_generic_fresh_fill_recipe = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_generic_fresh_fill_null_cleanup_source =
	// Swapped-out __tmp owns the original fresh empty output vector.
	// Genuine vector/base/Impl/allocator destruction, Destroy wrapper and
	// null _M_deallocate root; no nonnull allocator/delete tail is reached.
	15 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_generic_fresh_positive_fill_assign_source =
	// Actual assign(n,zero): public and _M_fill_assign each this/valref/n.
	// __add is declared only in the unselected fitting-increase branch.
	4 * sizeof(void *) + 2 * sizeof(size_t) +
	item_complete_generic_fresh_fill_recipe::complete_recipe_vector_capacity_source<uint8_t> +
	item_complete_generic_fresh_fill_recipe::complete_recipe_vector_mutable_allocator_source<
		uint8_t> +
	// Growing-only borrowed-allocator fill constructor, then swap_data's
	// own2P, real _Vector_impl_data __tmp3P/default receiverP and
	// three genuine copy_data occurrences6P.
	item_complete_recovery_byte_fill_allocator_reference_constructor_source +
	12 * sizeof(void *) + item_complete_generic_fresh_fill_null_cleanup_source;
constexpr size_t item_complete_generic_fresh_fill_tmp_inline = sizeof(std::vector<uint8_t>);
constexpr size_t item_complete_generic_fresh_fill_zero_inline = sizeof(uint8_t);
namespace item_generic_fresh_ci_source = duris_craft_recipe_bounded_detail;
using item_generic_continuation_CI = std::vector<uint8_t>::const_iterator;
constexpr size_t item_complete_generic_continuation_fresh_assign_source =
	// Actual C++20 assign(CI,CI) and forward _M_assign_aux on a vector
	// emptied by the preceding aggregate payload reset. Positive length
	// allocates from null; zero length reaches copy(0) then erase(n0).
	item_generic_fresh_ci_source::complete_recipe_byte_assign_own_source<
		item_generic_continuation_CI> +
	item_generic_fresh_ci_source::complete_recipe_byte_fresh_assign_aux_own_source<
		item_generic_continuation_CI> +
	item_generic_fresh_ci_source::complete_recipe_iterator_category_source<
		item_generic_continuation_CI> +
	item_generic_fresh_ci_source::complete_recipe_normal_distance_graph_source<
		item_generic_continuation_CI> +
	item_generic_fresh_ci_source::complete_recipe_byte_check_init_len_source +
	item_generic_fresh_ci_source::complete_recipe_allocator_const_copy_source +
	item_generic_fresh_ci_source::complete_recipe_allocator_cleanup_source +
	// _S_max_size static allocref/diffmax/allocmax/return plus traits max
	// and min refs/result; no public max_size or const allocator getter.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	item_generic_fresh_ci_source::complete_recipe_vector_capacity_source<uint8_t> +
	item_generic_fresh_ci_source::complete_recipe_byte_allocate_and_copy_own_source<
		item_generic_continuation_CI> +
	item_generic_fresh_ci_source::complete_recipe_vector_allocate_source<uint8_t> +
	// Actual three getters: check_init argument, allocate_and_copy's
	// uninitialized-copy allocator argument, and old null Destroy argument.
	3 * item_generic_fresh_ci_source::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	item_generic_fresh_ci_source::complete_recipe_uninitialized_copy_graph_source<
		item_generic_continuation_CI, const uint8_t *> +
	item_generic_fresh_ci_source::complete_recipe_vector_destroy_range_source<uint8_t> +
	// Null old deallocation only reaches vector's this/pointer/count root.
	2 * sizeof(void *) + sizeof(size_t) +
	// len0 copy preserves all selected wrappers but not count1 assign_one.
	item_generic_fresh_ci_source::complete_recipe_copy_graph_source<
		false, item_generic_continuation_CI, uint8_t *, const uint8_t *> -
	item_generic_fresh_ci_source::complete_recipe_byte_assign_one_source +
	item_generic_fresh_ci_source::complete_recipe_byte_erase_at_end_own_source +
	// Fitting len0 calls actual size() before erase; its n0 skips Destroy.
	item_generic_fresh_ci_source::complete_recipe_vector_size_source<uint8_t>;
constexpr size_t item_complete_generic_continuation_assign_caller_source =
	// First input has two left-associative offsets, second has three.
	// Each intermediate receiver CI exists; final two class results are
	// assign's by-value formals. Five genuine offsets each include the
	// arithmetic pointer temporary at explicit const-pointer-ref ctor.
	5 * sizeof(item_generic_continuation_CI) + 6 * sizeof(void *) +
	5 * (4 * sizeof(void *) + sizeof(std::ptrdiff_t));
constexpr size_t item_complete_corpse_encode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_corpse_encode_child_query_source =
	3 * item_transfer_corpse_context_encode_complete_source_query_frame_bytes();
constexpr size_t item_complete_corpse_decode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 8 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_corpse_decode_child_query_source =
	3 * item_transfer_corpse_context_decode_complete_source_query_frame_bytes();
constexpr size_t item_complete_collector_encode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_collector_encode_child_query_source =
	3 * item_transfer_collector_context_encode_complete_source_query_frame_bytes();
constexpr size_t item_complete_collector_decode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 8 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_collector_decode_child_query_source =
	3 * item_transfer_collector_context_decode_complete_source_query_frame_bytes();
namespace item_general_wire_lifetime_recipe = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_general_wire_byte_lifecycle_source =
	// Four actual vector<byte> objects default sequentially. One typed family
	// suffices; each physical object/capacity is admitted separately in body41.
	6 * sizeof(void *) +
	// Final output member move retains real internal V/A, all byte cleanup
	// descendants. Same-type outer vector cleanup reuses those declarations.
	item_general_wire_lifetime_recipe::complete_recipe_vector_move_selected_source<uint8_t> +
	2 * sizeof(void *);
constexpr size_t item_complete_general_wire_copy_caller_projected_source =
	item_complete_generic_wire_copy_caller_source -
	// Four normal-output calls: blob copy_n, corpse, collector, continuation.
	// Published graph already owns each propagated niter_wrap root result I.
	// For blob count zero, the same root I owns copy_n's mutually exclusive
	// return object; niter_wrap is not claimed to execute on that path.
	// Remove exactly the duplicate discarded caller result, keeping all begin
	// and intermediate offset receiver objects. Scalar-output paths unchanged.
	4 * sizeof(std::vector<uint8_t>::iterator);
constexpr size_t item_complete_general_wire_encode_retained_source =
	item_complete_general_wire_encode_own_source +
	item_complete_general_encode_wrapper_own_source +
	item_complete_general_policy_query_source +
	// payload remains null; four observed byte buffers share one typed family.
	item_complete_general_control_prefix_peak_source +
	item_complete_general_vector_current_source<uint8_t> +
	item_complete_general_encode_direct_wire_source +
	// Actual native_mobile_version(uint16_t) parameter and result.
	sizeof(uint16_t) + sizeof(bool) + item_complete_general_wire_byte_lifecycle_source +
	item_complete_generic_fresh_positive_fill_assign_source +
	item_complete_generic_wire_copy_source +
	item_complete_general_wire_copy_caller_projected_source +
	item_complete_general_payload_valid_child_own_source +
	3 * item_transfer_payload_valid_complete_source_query_frame_bytes() +
	item_complete_corpse_encode_child_own_source +
	item_complete_corpse_encode_child_query_source +
	item_complete_collector_encode_child_own_source +
	item_complete_collector_encode_child_query_source +
	item_complete_native_reference_encode_child_own_source +
	item_complete_native_reference_encode_child_query_source +
	item_complete_native_recovery_encode_entry_child_own_source +
	item_complete_native_recovery_encode_entry_child_query_source;
constexpr size_t item_complete_general_decoder_expected_lifetime_source =
	// expected{} defaults only K/R plus byte, whose exact completed default
	// family already belongs payload aggregate default. Expected byte members
	// never populate and same typed cleanup is reached by payload lifetime/
	// final MOVE. Keep generated command dtorP and full K/R cleanup only.
	item_complete_command_aggregate_default_source - 6 * sizeof(void *) +
	item_complete_command_cleanup_source -
	item_general_wire_lifetime_recipe::member_vector_cleanup_source<uint8_t>();
constexpr size_t item_complete_general_generic_decode_retained_source =
	item_complete_general_generic_decode_own_source +
	item_complete_general_decode_wrapper_own_source +
	item_complete_general_policy_query_source + item_complete_general_decode_controller_source +
	item_complete_general_decode_direct_wire_source +
	// Genuine direct parent calls; an ended validator child supplies no credit.
	item_complete_general_native_version_source + item_complete_general_owner_equal_source +
	item_complete_general_continuation_limit_source +
	// Aggregate candidate, reset RHS, final payload MOVE and cleanup. The
	// accepted aggregate stem already owns outer std::move<Payload>2P once.
	item_complete_payload_aggregate_move_cleanup_source +
	item_complete_general_decoder_expected_lifetime_source +
	item_complete_generic_decode_blob_copy_source +
	item_complete_generic_decode_blob_copy_caller_source +
	item_complete_generic_continuation_fresh_assign_source +
	item_complete_generic_continuation_assign_caller_source +
	item_complete_generic_command_equal_source +
	item_complete_general_decoder_reference_span_constructor_source +
	item_complete_general_decoder_recovery_span_source +
	item_complete_general_payload_valid_child_own_source +
	3 * item_transfer_payload_valid_complete_source_query_frame_bytes() +
	item_complete_general_validated_entities_child_own_source +
	3 * item_complete_general_validated_entities_source_query_frame_bytes() +
	item_complete_corpse_decode_child_own_source +
	item_complete_corpse_decode_child_query_source +
	item_complete_collector_decode_child_own_source +
	item_complete_collector_decode_child_query_source +
	item_complete_native_reference_decode_child_own_source +
	item_complete_native_reference_decode_child_span_call_source +
	item_complete_native_reference_decode_child_query_source +
	item_complete_native_recovery_decode_entry_child_own_source +
	item_complete_native_recovery_decode_entry_child_query_source;
constexpr size_t item_complete_general_native_cost_owned_source =
	3 * sizeof(void *) + sizeof(size_t) + sizeof(uint16_t) + 2 * sizeof(bool);
constexpr size_t item_complete_general_native_cost_wrapper_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 2 * sizeof(size_t) +
	2 * sizeof(bool);
constexpr size_t item_complete_general_public_encode_owned_source =
	4 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_general_public_recovery_encode_owned_source =
	4 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_general_public_encode_wrapper_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 2 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_general_public_recovery_encode_wrapper_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 2 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_general_public_decode_owned_source =
	8 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(uint16_t) + sizeof(bool);
constexpr size_t item_complete_general_public_decode_wrapper_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 2 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t item_complete_general_payload_encode_child_own_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	sizeof(uint16_t) + 1 * sizeof(bool);
constexpr size_t item_complete_general_payload_decode_child_own_source =
	4 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	1 * sizeof(bool);
constexpr size_t item_complete_general_native_cost_encode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	2 * sizeof(bool);
constexpr size_t item_complete_general_public_native_encode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	1 * sizeof(bool);
constexpr size_t item_complete_general_public_native_recovery_encode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	1 * sizeof(bool);
constexpr size_t item_complete_general_public_inner_decode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	1 * sizeof(bool);
constexpr size_t item_complete_general_fee_decode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	1 * sizeof(bool);
constexpr size_t item_complete_general_money_decode_child_own_source =
	3 * sizeof(void *) + sizeof(bool (*)(size_t, void *) noexcept) + 7 * sizeof(size_t) +
	1 * sizeof(bool);
namespace item_complete_payload_copy_recipe = duris_craft_recipe_bounded_detail;
template <class T> constexpr size_t item_complete_payload_copy_vector_source =
	(19 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::allocator<T>)) +
	(11 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(bool)) + 6 * sizeof(void *) +
	// Trivial uninitialized-copy backend; actual _Num is ptrdiff_t.
	// No bare builtin memmove callee, implicit trivial T assignment, or
	// invented normal-iterator copy/move call. Actual wrapper objects stay.
	46 * sizeof(void *) + sizeof(std::ptrdiff_t) + 4 * sizeof(bool) -
	// The two begin/end result objects ARE the two by-value parameters.
	2 * sizeof(void *);
constexpr size_t item_complete_native_cost_reset_selected_source =
	// Original native_cost={} directly aggregate-initializes its temporary;
	// no enclosing default constructor. Its attempts vector defaults6P.
	6 * sizeof(void *) +
	// Cost context and projection nontrivial move assignments each3P.
	2 * 3 * sizeof(void *) +
	item_complete_payload_copy_recipe::complete_recipe_vector_move_selected_source<
		native_quest_cost_attempt> -
	// Exact same typed cleanup already retained above for COPY unwind and
	// final original cleanup; MOVE's real temporary V/A objects stay intact.
	item_complete_payload_copy_recipe::member_vector_cleanup_source<native_quest_cost_attempt>();
constexpr size_t item_complete_payload_string_copy_frontend_source =
	// copy this/source2P; mutable local-data+pointer/address chain8P;
	// const allocator getter2P; ext selectP; traits selectP; allocator and
	// newallocator const-copy constructors4P; rvalue hider3P; stdmove2P.
	// One selected allocator object aliases the traits/ext prvalue chain.
	23 * sizeof(void *) + sizeof(std::allocator<char>);
constexpr size_t item_complete_payload_string_copy_construct_source =
	// _M_construct<char*> this/beg/end/Guard4P,dnewN,F; guard ctor2P/dtorP;
	// distance2P+D, categoryP+R,__distance2P+D,char* copy_chars3P.
	// The category prvalue is __distance's actual by-value R parameter.
	15 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(std::ptrdiff_t) +
	sizeof(std::forward_iterator_tag) + sizeof(std::random_access_iterator_tag);
constexpr size_t item_complete_payload_string_copy_allocation_source =
	// create3P+N,max_sizeP+N,traits maxP+N,S_allocate3P+N,traits allocate2P+N,
	// allocator allocate2P+N+B,newallocator allocate3P+N,_M_max_sizeP+N,
	// actual declared operator new pointer resultP/countN.
	// Const getter is in frontend; mutable getter is in reached cleanup.
	17 * sizeof(void *) + 9 * sizeof(size_t) + sizeof(bool);
constexpr size_t item_complete_payload_string_copy_remaining_source =
	// data setter2P,capacity setterP+N,init-localP+B plus retained conservative
	// body N/C (not runtime execution evidence); set_length6P+2N+C+B after
	// only actual data getter2P is shared with cleanup; runtime S_copy/traits
	// copy5P+2N+B and public lengthP+N. Bare memcpy has no callee Source.
	16 * sizeof(void *) + 7 * sizeof(size_t) + 2 * sizeof(char) + 3 * sizeof(bool);
constexpr size_t item_complete_payload_string_copy_cleanup_selected_source =
	item_complete_payload_string_copy_frontend_source +
	item_complete_payload_string_copy_construct_source +
	item_complete_payload_string_copy_allocation_source +
	item_complete_payload_string_copy_remaining_source +
	item_complete_native_member_string_cleanup_source;
static_assert(item_complete_payload_string_copy_cleanup_selected_source ==
	      96 * sizeof(void *) + 22 * sizeof(size_t) + 2 * sizeof(std::ptrdiff_t) +
		      sizeof(std::forward_iterator_tag) + sizeof(std::random_access_iterator_tag) +
		      sizeof(std::allocator<char>) + 2 * sizeof(char) + 6 * sizeof(bool));
namespace item_complete_general_byte_recipe = duris_craft_recipe_bounded_detail;
using item_complete_general_byte_i = std::vector<uint8_t>::iterator;
constexpr size_t item_complete_general_byte_count_failure_cleanup_source = 2 * sizeof(void *);
constexpr size_t item_complete_general_byte_count_constructor_source =
	// Actual vector<byte>(positive n,default allocator), NOT fill(n,zero).
	// Public this/allocatorref/count + real default allocator A.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<uint8_t>) + 3 * sizeof(void *) +
	// check_init_len count/ref/returned count + converted allocator A,
	// const-copy+cleanup5P, direct Smax/traits/min5P4N.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(std::allocator<uint8_t>) + 5 * sizeof(void *) +
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// Base/impl/data/default/allocator-copy/create_storage10P2N;
	// typed allocation11P6N+B; default_initialize P+N/getallocator2P.
	10 * sizeof(void *) + 2 * sizeof(size_t) +
	item_complete_general_byte_recipe::complete_recipe_vector_allocate_source<uint8_t> +
	sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) +
	item_complete_recovery_byte_default_n_source +
	item_complete_general_byte_count_failure_cleanup_source;
constexpr size_t item_complete_general_byte_normal_fill_source =
	// fill2I+valP, fill_a2I+valP, normal fill_a1 2I+valP,
	// two actual base refs4P, byte raw fill_a1 3P+lenN+tmp byte,
	// genuine public std::is_constant_evaluated B.
	6 * sizeof(item_complete_general_byte_i) + 10 * sizeof(void *) + sizeof(size_t) +
	sizeof(uint8_t) + sizeof(bool) +
	// Caller begin/end result objects alias public fill formals;
	// two genuine receivers + explicit pointer constructors3P each.
	6 * sizeof(void *);
constexpr size_t item_complete_general_byte_fill_assign_source =
	// Public assign(this,count,valref), fill_assign(this,count,valref,add).
	// Internal vector __tmp DTO and caller zero argument are separate INLINE.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) + 2 * sizeof(size_t) +
	item_complete_general_byte_recipe::complete_recipe_vector_capacity_source<uint8_t> +
	item_complete_general_byte_recipe::complete_recipe_vector_size_source<uint8_t> +
	item_complete_general_byte_recipe::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	// Growing allocation path uses actual borrowed allocator fill ctor,
	// data swap2P+tmp3P+defaultP+three copy_data occurrences6P.
	item_complete_recovery_byte_fill_allocator_reference_constructor_source +
	12 * sizeof(void *) +
	// Swap's old output may be nonempty, so full actual byte cleanup;
	// its Impl/allocator destructor2P also owns ctor-failure unwind family.
	item_complete_general_byte_recipe::member_vector_cleanup_source<uint8_t>() +
	// Fitting increase: fill existing normal range then uninit_fill_n_a
	// positive appended raw suffix. Keep real can_fill B; public query
	// is retained by accepted raw fill suffix, no constexpr trait callee.
	item_complete_general_byte_normal_fill_source +
	item_complete_general_byte_recipe::complete_recipe_byte_uninitialized_fill_a_source +
	item_complete_general_byte_recipe::complete_recipe_byte_uninitialized_fill_source +
	item_complete_general_byte_recipe::complete_recipe_byte_uninitialized_fill_true_source +
	item_complete_recovery_byte_default_fill_suffix_source +
	// Fitting shrink/equal: raw fill_n then erase. Erase true n>0 and
	// trivial Destroy wrappers are genuine possible old-output scopes.
	item_complete_recovery_byte_default_fill_suffix_source +
	item_complete_general_byte_recipe::complete_recipe_byte_erase_at_end_own_source +
	item_complete_general_byte_recipe::complete_recipe_vector_destroy_range_source<uint8_t>;
constexpr size_t item_complete_general_byte_fill_assign_tmp_inline = sizeof(std::vector<uint8_t>);
constexpr size_t item_complete_general_byte_fill_assign_zero_inline = sizeof(uint8_t);
namespace item_complete_public_shrink_recipe = duris_craft_recipe_bounded_detail;
using item_complete_public_shrink_ci = std::vector<uint8_t>::const_iterator;
using item_complete_public_shrink_difference =
	typename std::iterator_traits<item_complete_public_shrink_ci>::difference_type;
constexpr size_t item_complete_public_cost_payload_shrink_caller_source =
	// The right expression is begin()+24+body_size: three actual offsets.
	// Two begin() temporaries and the right intermediate +24 result are
	// three distinct implicit-object CI receivers. Final two results are
	// the existing assign by-value formal destinations, priced below.
	2 *
	(sizeof(void *) +
	 item_complete_public_shrink_recipe::complete_recipe_normal_pointer_constructor_source<
		 item_complete_public_shrink_ci>)+3 *
	(item_complete_public_shrink_recipe::complete_recipe_normal_offset_argument_source<
		 item_complete_public_shrink_ci> +
	 item_complete_public_shrink_recipe::complete_recipe_normal_pointer_constructor_source<
		 item_complete_public_shrink_ci>)+3 *
	sizeof(item_complete_public_shrink_ci);
constexpr size_t item_complete_public_cost_payload_shrink_aux_own_source =
	// Only this/first/last/len/forward-tag. The unselected allocating tmp
	// and fitting-growing mid/n locals are absent at this actual call.
	sizeof(void *) + 2 * sizeof(item_complete_public_shrink_ci) + sizeof(size_t) +
	sizeof(std::forward_iterator_tag);
constexpr size_t item_complete_public_cost_payload_shrink_assign_source =
	item_complete_public_cost_payload_shrink_caller_source +
	item_complete_public_shrink_recipe::complete_recipe_byte_assign_own_source<
		item_complete_public_shrink_ci> +
	// Actual public assign category call and RA-to-forward slicing retain
	// the RA source temporary as well as the target aux forward-tag formal.
	sizeof(void *) + sizeof(std::random_access_iterator_tag) +
	item_complete_public_cost_payload_shrink_aux_own_source +
	item_complete_public_shrink_recipe::complete_recipe_normal_distance_graph_source<
		item_complete_public_shrink_ci> +
	item_complete_public_shrink_recipe::complete_recipe_vector_capacity_source<uint8_t> +
	item_complete_public_shrink_recipe::complete_recipe_vector_size_source<uint8_t> +
	// body_size==1 is genuinely reachable before recursive validation:
	// keep false const-byte copy's scalar assign_one specialization.
	item_complete_public_shrink_recipe::complete_recipe_copy_graph_source<
		false, item_complete_public_shrink_ci, uint8_t *, const uint8_t *> +
	item_complete_public_shrink_recipe::complete_recipe_byte_erase_at_end_own_source +
	item_complete_public_shrink_recipe::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	item_complete_public_shrink_recipe::complete_recipe_vector_destroy_range_source<uint8_t>;
using item_native_cost_copy_iterator = std::vector<uint8_t>::iterator;
using item_native_cost_copy_difference =
	typename std::iterator_traits<item_native_cost_copy_iterator>::difference_type;
namespace item_native_cost_copy_recipe = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_native_cost_copy_projected_source =
	item_native_cost_copy_recipe::complete_recipe_copy_graph_source<
		false, item_native_cost_copy_iterator, item_native_cost_copy_iterator, uint8_t *> -
	// Successful body and Cost buffers are each >=96: no count-one assign.
	item_native_cost_copy_recipe::complete_recipe_byte_assign_one_source -
	// Project only __niter_wrap's returned I to the explicit discarded result
	// in caller below. Public/forwarding I formals remain genuine separate DTOs.
	sizeof(item_native_cost_copy_iterator);
constexpr size_t item_complete_native_cost_two_copy_caller_source =
	// First copy: body.begin/end, exact.begin = three genuine endpoints9P;
	// output offset owns real pointer prvalue4P+D. exact.begin temporary I
	// is offset receiver; another I is the discarded std::copy result.
	(9 * sizeof(void *) + 4 * sizeof(void *) + sizeof(item_native_cost_copy_difference) +
	 2 * sizeof(item_native_cost_copy_iterator)) +
	// Second copy: same three actual endpoint instances, two offsets. The
	// first offset I becomes second offset receiver, plus begin receiver and
	// discarded result. Final offset I initializes public copy output formal.
	(9 * sizeof(void *) + 2 * (4 * sizeof(void *) + sizeof(item_native_cost_copy_difference)) +
	 3 * sizeof(item_native_cost_copy_iterator));
constexpr size_t item_complete_native_cost_two_copy_source =
	2 * item_complete_native_cost_copy_projected_source +
	item_complete_native_cost_two_copy_caller_source;
constexpr size_t item_complete_general_public_encode_native_only_source_query_frame_bytes() noexcept
{
	return 2 * sizeof(void *) + 3 * sizeof(size_t) + 3 * sizeof(bool) +
	       item_transfer_payload_encode_version_complete_source_query_frame_bytes();
}
constexpr size_t
item_complete_general_public_recovery_encode_native_only_source_query_frame_bytes() noexcept
{
	return 2 * sizeof(void *) + 3 * sizeof(size_t) + 3 * sizeof(bool) +
	       item_transfer_payload_encode_version_complete_source_query_frame_bytes();
}
constexpr size_t
item_complete_general_public_decode_inner_native_source_query_frame_bytes() noexcept
{
	return 2 * sizeof(void *) + 3 * sizeof(size_t) + 3 * sizeof(bool) +
	       item_transfer_payload_decode_generic_complete_source_query_frame_bytes() +
	       item_complete_general_public_encode_native_only_source_query_frame_bytes() +
	       item_complete_general_public_recovery_encode_native_only_source_query_frame_bytes();
}
namespace item_complete_general_public_recipe = duris_craft_recipe_bounded_detail;
constexpr size_t item_complete_general_native_cost_byte_lifetime_source =
	// Body/cost empty defaults share one typed family; exact count constructor
	// is distinct. Final MOVE keeps its real V/A and caller std::move2P.
	6 * sizeof(void *) +
	item_complete_general_public_recipe::complete_recipe_vector_move_selected_source<uint8_t> -
	// Actual typed cleanup is already selected by original payload COPY unwind.
	item_complete_general_public_recipe::member_vector_cleanup_source<uint8_t>() +
	2 * sizeof(void *);
constexpr size_t item_complete_general_public_byte_lifetime_source =
	6 * sizeof(void *) +
	item_complete_general_public_recipe::complete_recipe_vector_move_selected_source<uint8_t> +
	2 * sizeof(void *);
constexpr size_t item_complete_general_public_encode_selected_common_source =
	item_complete_general_policy_query_source +
	item_complete_general_public_encode_controller_source +
	item_complete_general_public_byte_lifetime_source +
	item_complete_native_money_encode_entry_child_own_source +
	item_complete_native_money_encode_entry_child_query_source +
	item_complete_general_native_cost_encode_child_own_source +
	3 * item_transfer_native_cost_encode_complete_source_query_frame_bytes() +
	item_complete_general_payload_encode_child_own_source +
	3 * item_transfer_payload_encode_version_complete_source_query_frame_bytes();
constexpr size_t item_complete_general_public_encode_retained_source =
	item_complete_general_public_encode_owned_source +
	item_complete_general_public_encode_wrapper_source +
	item_complete_general_public_encode_selected_common_source;
constexpr size_t item_complete_general_public_recovery_encode_retained_source =
	item_complete_general_public_recovery_encode_owned_source +
	item_complete_general_public_recovery_encode_wrapper_source +
	item_complete_general_public_encode_selected_common_source;
constexpr size_t item_complete_general_public_decode_direct_source =
	// Three genuinely distinct same-file version helper definitions.
	3 * (sizeof(uint16_t) + sizeof(bool)) +
	// Const byte size/index/data and wire leaves, completed typed calls shared.
	sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) + sizeof(size_t) + 5 * sizeof(void *) +
	sizeof(void *) + sizeof(uint16_t) + item_complete_general_get_u32_source +
	item_complete_general_get_u64_source +
	// Brace Cost span initializes child by-value formal, not another DTO.
	7 * sizeof(void *) + 2 * sizeof(size_t);
constexpr size_t item_complete_general_public_decode_retained_source =
	// This same unconditional constant is retained by actual outer and inner
	// dispatcher entries; no branch-reduced retained-own replacement.
	item_complete_general_public_decode_owned_source +
	item_complete_general_public_decode_wrapper_source +
	item_complete_general_policy_query_source +
	item_complete_general_public_decode_controller_source +
	item_complete_general_public_decode_direct_source +
	item_complete_payload_aggregate_move_cleanup_source +
	// Payload stem owns outer payload std::move2P once. Byte default/cleanup
	// also serves canonical, while actual command COPY keeps all3 typed lanes.
	item_complete_command_copy_source + item_complete_command_cleanup_source -
	item_complete_general_public_recipe::member_vector_cleanup_source<uint8_t>() +
	item_complete_public_cost_payload_shrink_assign_source +
	item_complete_generic_canonical_equal_source +
	item_complete_general_money_decode_child_own_source +
	3 * item_transfer_native_money_decode_complete_source_query_frame_bytes() +
	item_complete_general_fee_decode_child_own_source +
	3 * item_transfer_native_fee_decode_complete_source_query_frame_bytes() +
	item_complete_general_payload_decode_child_own_source +
	3 * item_transfer_payload_decode_generic_complete_source_query_frame_bytes() +
	item_complete_general_public_inner_decode_child_own_source +
	3 * item_complete_general_public_decode_inner_native_source_query_frame_bytes() +
	item_complete_native_cost_decode_child_own_source +
	item_complete_native_cost_decode_child_span_call_source +
	item_complete_native_cost_decode_child_query_source +
	item_complete_native_cost_value_entry_child_own_source +
	item_complete_native_cost_value_entry_child_query_source +
	item_complete_general_native_cost_encode_child_own_source +
	3 * item_transfer_native_cost_encode_complete_source_query_frame_bytes() +
	item_complete_general_public_native_encode_child_own_source +
	3 * item_complete_general_public_encode_native_only_source_query_frame_bytes() +
	item_complete_general_public_native_recovery_encode_child_own_source +
	3 * item_complete_general_public_recovery_encode_native_only_source_query_frame_bytes();
struct item_public_dispatch_complete_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *original = nullptr;
	const item_transfer_payload *candidate = nullptr;
	const std::vector<uint8_t> *canonical = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    (original &&
		     (!payload_clone_add(total, sizeof(*original)) ||
		      !payload_clone_vector_heap(original->keys, false, total) ||
		      !payload_clone_vector_heap(original->expected_revisions, false, total) ||
		      !payload_clone_vector_heap(original->payload, false, total) ||
		      !payload_clone_vector_heap(original->accounting_intent, false, total))) ||
		    (candidate && (!payload_clone_add(total, sizeof(*candidate)) ||
				   !payload_clone_heap(*candidate, false, total))) ||
		    (canonical && (!payload_clone_add(total, sizeof(*canonical)) ||
				   !payload_clone_vector_heap(*canonical, false, total))) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	bool command_copy(const critical_command &source) const noexcept
	{
		// Actual generated COPY, all four fresh vector capacities equal SIZE.
		// No command bytes/canonical length is substituted for member storage.
		size_t request = sizeof(critical_command);
		return payload_clone_vector_heap(source.keys, true, request) &&
		       payload_clone_vector_heap(source.expected_revisions, true, request) &&
		       payload_clone_vector_heap(source.payload, true, request) &&
		       payload_clone_vector_heap(source.accounting_intent, true, request) &&
		       peak(request);
	}
};
struct item_complete_collector_uid_present
{
	const item_transfer_payload &payload;
	bool operator()(uint64_t uid) const
	{
		return item_complete_find_payload_item(payload, uid) != nullptr;
	}
};
struct item_complete_text_printable_predicate
{
	bool operator()(unsigned char character) const
	{
		return character >= 0x20 && character != 0x7f;
	}
};
template <class T> constexpr size_t item_complete_general_entities_growth_controller_source =
	// Actual typed growth(this,value,count,request,next,B). Both fitting
	// and growing are reached in the general path; next belongs growing.
	item_complete_general_entities_growth_declaration_source +
	item_general_entities_selected_recipe::complete_recipe_vector_max_size_source<T> +
	item_general_entities_selected_recipe::complete_recipe_vector_size_source<T> +
	item_general_entities_selected_recipe::complete_recipe_vector_capacity_source<T> +
	// Genuine std::max<size_t> two const refs and returned const-ref.
	3 * sizeof(void *);
constexpr size_t item_complete_general_entities_controller_source =
	item_complete_general_entities_prefix_declaration_source +
	item_complete_general_entities_peak_declaration_source +
	// Common checked add + actual callback declaration signatures.
	2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	item_complete_general_vector_current_source<critical_entity_key> +
	item_complete_general_vector_current_source<critical_expected_revision> +
	item_complete_general_vector_current_source<uint8_t> +
	item_complete_general_vector_current_source<player_item_snapshot> +
	// Actual outputs-only CURRENT census and const range. roots is never
	// registered in these owned bodies; no roots/u64 observer is selected.
	item_complete_native_validation_current_row_source +
	item_complete_native_validation_current_range_source +
	item_complete_general_entities_growth_controller_source<critical_entity_key> +
	item_complete_general_entities_growth_controller_source<critical_expected_revision> +
	// Two distinct T-specialized fresh and T,C-specialized sort methods.
	2 * item_complete_general_entities_fresh_declaration_source +
	2 * item_complete_general_entities_sort_frame_declaration_source;
constexpr size_t item_complete_general_entities_output_lifetime_source =
	item_complete_general_validator_outputs_default_cleanup_source +
	// Full root cleanup owns row destructor receiver, while actual member
	// vector destructors and nested ED spell-vector cleanup are separate.
	item_general_validator_vector_cleanup_source<player_item_dynamic_affect_snapshot> +
	item_general_validator_vector_cleanup_source<player_item_extra_description_snapshot> +
	item_general_validator_vector_cleanup_source<int32_t> +
	// Rows returned by craft child destruct here; the same completed char
	// cleanup family is shared across row/nested ED fields, never defaulted.
	item_general_entities_selected_recipe::member_string_destroy_source;
constexpr size_t item_complete_general_entities_retained_source =
	item_complete_general_entities_owned_declaration_source +
	item_complete_general_entities_wrapper_declaration_source +
	item_complete_general_entities_controller_source +
	item_complete_general_entities_outputs_range_source +
	item_complete_general_entities_output_lifetime_source +
	item_complete_general_entities_direct_source +
	item_complete_general_entities_command_lifecycle_source +
	// Fresh candidate vectors select exactly-two initializer-list assignments.
	// Their IL objects/backing arrays are separately admitted by fresh().
	item_complete_general_entities_fresh_assign_source<critical_entity_key> +
	item_complete_general_entities_fresh_assign_source<critical_expected_revision> +
	item_complete_general_entities_key_push_source +
	item_complete_general_entities_revision_push_source +
	item_complete_general_entities_pop_source<critical_entity_key> +
	item_complete_general_entities_pop_source<critical_expected_revision> +
	// Genuine nonrecursive declarations stay parent-retained. Recursive rows
	// are admitted at each actual sort_frame(), not duplicated here.
	item_complete_general_entities_sort_nonrecursive_source<
		critical_entity_key, decltype(&critical_entity_key_less)> +
	item_complete_general_entities_sort_nonrecursive_source<critical_expected_revision,
								item_entities_revision_less> +
	item_complete_general_entities_sort_caller_source<critical_entity_key> +
	item_complete_general_entities_sort_caller_source<critical_expected_revision> +
	item_complete_general_entities_key_adjacent_source +
	item_complete_general_entities_key_adjacent_caller_source +
	item_complete_general_entities_owner_key_child_declaration_source +
	item_complete_general_entities_craft_child_declaration_source +
	// Both owner-key calls complete sequentially; one real adapter family.
	// Only these pure child-query scopes are retained, never full child totals.
	3 * item_owner_key_source_query_frame_bytes() +
	3 * item_transfer_craft_outputs_decode_complete_source_query_frame_bytes();
using item_complete_collector_pred = item_complete_collector_uid_present;
using item_complete_collector_pred_adapter =
	__gnu_cxx::__ops::_Iter_pred<item_complete_collector_pred>;
using item_complete_collector_negate = __gnu_cxx::__ops::_Iter_negate<item_complete_collector_pred>;
constexpr size_t item_complete_collector_all_of_source =
	29 * sizeof(void *) + 10 * sizeof(item_complete_collector_ci) +
	5 * sizeof(item_complete_collector_pred) +
	2 * sizeof(item_complete_collector_pred_adapter) + sizeof(item_complete_collector_negate) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(item_complete_collector_difference) +
	sizeof(uint64_t) + 4 * sizeof(bool) + item_complete_offering_find_source;
constexpr size_t item_complete_collector_context_source =
	// Union of genuine own declarations and actual selected branch families.
	// Distinct completed algorithm call occurrences are conservatively summed.
	2 * sizeof(void *) + sizeof(uint16_t) + sizeof(bool) +
	item_complete_collector_absent_empty_source +
	item_complete_collector_operation_zero_source +
	item_complete_collector_present_caller_source + item_complete_collector_is_sorted_source +
	item_complete_collector_adjacent_source + item_complete_collector_all_of_source;
using item_complete_validation_find_pred = item_complete_validation_ancestor_predicate;
using item_complete_validation_find_adapter =
	__gnu_cxx::__ops::_Iter_pred<item_complete_validation_find_pred>;
constexpr size_t item_complete_validation_raw_find_source =
	// Public first/last/returned pointer3P + C; factory C; explicit adapter
	// ctor P+C and two actual move<C> calls4P; dispatch3P+A;
	// RA3P+A+tripD+RA; categoryP; adapter2P+B; named predicate2P+B.
	// Raw pointer ++/subtraction/dereference introduce no callable scopes.
	19 * sizeof(void *) + 3 * sizeof(item_complete_validation_find_pred) +
	2 * sizeof(item_complete_validation_find_adapter) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(bool);
constexpr size_t item_complete_validation_raw_find_caller_source =
	// Three actual const array begin() calls: direct this/result2P plus
	// data this/result2P each. The raw parent pointer declaration remains
	// separately owned by the validator; scalar raw returns are not elided.
	item_complete_validation_raw_find_source + 12 * sizeof(void *);
constexpr size_t item_complete_generic_validator_projection_recipe_source =
	item_complete_generic_validator_projection_equal_source +
	item_general_recipe_matches_detail::complete_recipe_matches_full_source +
	item_complete_generic_validator_recipe_output_selectors_source;
using item_complete_text_pred = item_complete_text_printable_predicate;
using item_complete_text_adapter = __gnu_cxx::__ops::_Iter_pred<item_complete_text_pred>;
using item_complete_text_negate = __gnu_cxx::__ops::_Iter_negate<item_complete_text_pred>;
constexpr size_t item_complete_text_all_of_source =
	// Same declared all_of/find_if_not/pred/negate/four-argument RA route
	// as collector, genuinely instantiated on char-string CI and this type.
	// Named target is thisP+unsigned-char formal+returned B; no lookup child.
	29 * sizeof(void *) + 10 * sizeof(item_complete_text_ci) +
	5 * sizeof(item_complete_text_pred) + 2 * sizeof(item_complete_text_adapter) +
	sizeof(item_complete_text_negate) + sizeof(std::random_access_iterator_tag) +
	2 * sizeof(item_complete_text_difference) + sizeof(unsigned char) + 4 * sizeof(bool);
constexpr size_t item_complete_valid_text_source =
	// Own value-reference/maximum/required/returned B. String empty owns
	// thisP+returnedB and uses genuine size(), shared with maximum check
	// and end() only after each completed matching same-string call.
	sizeof(void *) + sizeof(size_t) + 2 * sizeof(bool) + sizeof(void *) + sizeof(bool) +
	sizeof(void *) + sizeof(size_t) + item_complete_text_endpoints_source +
	item_complete_text_all_of_source;
constexpr size_t item_complete_general_validator_retained_source =
	item_complete_general_validator_own_source +
	item_complete_general_valid_wrapper_own_source + item_complete_general_policy_query_source +
	item_complete_general_validation_controller_source +
	item_complete_general_validator_local_lifetime_source +
	item_complete_generic_validator_projection_recipe_source +
	item_complete_general_validator_small_helper_union_source +
	item_complete_general_validator_direct_source +
	// Collector's identical selected lookup was explicitly retained above.
	// No predicate/capture/iterator fields or complete collector envelope cut.
	item_complete_collector_context_source - item_complete_offering_find_source +
	item_complete_valid_text_source + item_complete_validation_root_count_source +
	item_complete_validation_raw_find_caller_source +
	// The count graph already reaches the actual mutable output range method
	// families, including comparison/base, increment and dereference. Hidden
	// loop iterator objects remain in validator own, no extra result objects.
	item_complete_general_validator_recipe_span_source +
	item_complete_general_mobile_valid_child_own_source +
	item_complete_general_recovery_valid_child_own_source +
	item_complete_general_offering_valid_child_own_source +
	item_complete_general_duplicate_valid_child_own_source +
	item_complete_general_lockpick_valid_child_own_source +
	item_complete_general_craft_outputs_child_own_source +
	item_complete_general_pouch_payload_child_own_source +
	item_complete_general_recipe_decode_child_own_source +
	3 * (item_transfer_native_mobile_context_valid_complete_source_query_frame_bytes() +
	     item_transfer_native_recovery_valid_complete_source_query_frame_bytes() +
	     item_transfer_quest_offering_continuation_valid_complete_source_query_frame_bytes() +
	     item_transfer_duplicate_promotion_continuation_valid_complete_source_query_frame_bytes() +
	     lockpick_retirement_payload_valid_complete_source_query_frame_bytes() +
	     item_transfer_craft_outputs_decode_complete_source_query_frame_bytes() +
	     craft_pouch_mutation_from_payload_complete_source_query_frame_bytes() +
	     craft_recipe_continuation_decode_complete_source_query_frame_bytes());
constexpr size_t item_complete_payload_copy_cleanup_selected_source =
	// Genuine COPY this/source + dtor this at ten allocating enclosing
	// sites: payload/corpse/collector/continuation/cost-context/cost-projection/
	// recovery/two forest bindings/publication. This is NOT the different
	// ten plain-default DMI receiver map (which includes mobile/reference).
	10 * (2 * sizeof(void *) + sizeof(void *)) +
	item_complete_payload_copy_vector_source<uint8_t> +
	item_complete_payload_copy_vector_source<uint64_t> +
	item_complete_payload_copy_vector_source<native_quest_cost_attempt> +
	item_complete_payload_copy_recipe::member_vector_cleanup_source<uint8_t>() +
	item_complete_payload_copy_recipe::member_vector_cleanup_source<uint64_t>() +
	item_complete_payload_copy_recipe::member_vector_cleanup_source<native_quest_cost_attempt>() +
	item_complete_payload_string_copy_cleanup_selected_source;
constexpr size_t item_complete_general_native_cost_encode_retained_source =
	item_complete_general_native_cost_owned_source +
	item_complete_general_native_cost_wrapper_source +
	item_complete_general_policy_query_source +
	item_complete_general_native_cost_encode_controller_source +
	item_complete_general_cost_direct_wire_source +
	item_complete_payload_copy_cleanup_selected_source +
	item_complete_native_cost_reset_selected_source +
	item_complete_general_native_cost_byte_lifetime_source +
	item_complete_general_byte_count_constructor_source -
	// Exact generated impl/allocator failure cleanup already selected by byte
	// COPY/MOVE cleanup; preserve every allocator object and normal ctor row.
	2 * sizeof(void *) + item_complete_native_cost_two_copy_source +
	item_complete_native_cost_value_entry_child_own_source +
	item_complete_native_cost_value_entry_child_query_source +
	item_complete_native_fee_encode_entry_child_own_source +
	item_complete_native_fee_encode_entry_child_query_source +
	item_complete_general_payload_encode_child_own_source +
	3 * item_transfer_payload_encode_version_complete_source_query_frame_bytes() +
	item_complete_native_cost_encode_child_own_source +
	item_complete_native_cost_encode_child_query_source;
bool item_sidecar_complete_append_u32(item_sidecar_complete_budget &owner,
				      std::vector<uint8_t> *output, uint32_t value)
{
	if (!output)
		return false;

	try
	{
		const size_t offset = output->size();
		if (!owner.growth(*output, sizeof(value)))
			return false;
		output->resize(offset + sizeof(value));
		put_u32(output->data() + offset, value);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}
bool item_sidecar_complete_append_text(item_sidecar_complete_budget &owner,
				       std::vector<uint8_t> *output, const std::string &value)
{
	// Scalar end restores the genuine frame on every original return.
	struct frame_end
	{
		item_sidecar_complete_budget &owner;
		size_t bytes;
		~frame_end() { owner.frames -= bytes; }
	};
	// Admit actual RAII object before construction. Its destructor Source
	// is already in the retained parent; frames retain only this live INLINE.
	if (!owner.peak(sizeof(frame_end)))
		return false;
	owner.frames += sizeof(frame_end);
	frame_end tail{ owner, sizeof(frame_end) };
	if (!output || value.size() > UINT32_MAX ||
	    !item_sidecar_complete_append_u32(owner, output, static_cast<uint32_t>(value.size())))
		return false;
	try
	{
		if (!owner.growth(*output, value.size()))
			return false;
		output->insert(output->end(), value.begin(), value.end());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}
bool item_sidecar_complete_read_text(item_sidecar_complete_budget &owner, const uint8_t *input,
				     size_t size, size_t *offset, size_t maximum,
				     std::string *value)
{
	uint32_t length = 0;
	if (!value || !read_u32(input, size, offset, &length) || length > maximum ||
	    *offset > size || size - *offset < length)
		return false;
	try
	{
		if (!owner.text(*value, reinterpret_cast<const char *>(input + *offset), length, 0))
			return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*offset += length;
	return true;
}
bool item_sidecar_complete_encode_corpse_context(const item_corpse_metadata &corpse,
						 std::vector<uint8_t> *encoded,
						 item_sidecar_complete_budget &owner)
{
	if (!encoded)
		return false;
	encoded->clear();
	if (!corpse.present)
		return true;
	if (!item_sidecar_complete_append_u32(owner, encoded, CORPSE_CONTEXT_VERSION) ||
	    !item_sidecar_complete_append_u32(owner, encoded,
					      static_cast<uint32_t>(corpse.room_vnum)) ||
	    !item_sidecar_complete_append_u32(owner, encoded,
					      static_cast<uint32_t>(corpse.weight)) ||
	    !item_sidecar_complete_append_u32(owner, encoded, corpse.actor_racewar))
		return false;
	for (int32_t value : corpse.values)
		if (!item_sidecar_complete_append_u32(owner, encoded, static_cast<uint32_t>(value)))
			return false;
	return item_sidecar_complete_append_text(owner, encoded, corpse.owner_name) &&
	       item_sidecar_complete_append_text(owner, encoded, corpse.short_description) &&
	       item_sidecar_complete_append_text(owner, encoded, corpse.description) &&
	       item_sidecar_complete_append_text(owner, encoded, corpse.keywords);
}
bool item_sidecar_complete_decode_corpse_context(const uint8_t *encoded, size_t size,
						 item_corpse_metadata *corpse,
						 item_sidecar_complete_budget &owner)
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
	return item_sidecar_complete_read_text(owner, encoded, size, &offset,
					       ITEM_TRANSFER_CORPSE_NAME_MAX_BYTES,
					       &corpse->owner_name) &&
	       item_sidecar_complete_read_text(owner, encoded, size, &offset,
					       ITEM_TRANSFER_CORPSE_SHORT_DESCRIPTION_MAX_BYTES,
					       &corpse->short_description) &&
	       item_sidecar_complete_read_text(owner, encoded, size, &offset,
					       ITEM_TRANSFER_CORPSE_DESCRIPTION_MAX_BYTES,
					       &corpse->description) &&
	       item_sidecar_complete_read_text(owner, encoded, size, &offset,
					       ITEM_TRANSFER_CORPSE_KEYWORDS_MAX_BYTES,
					       &corpse->keywords) &&
	       offset == size;
}
bool item_sidecar_complete_encode_collector_context(const item_collector_death_enrollment &collector,
						    std::vector<uint8_t> *encoded,
						    item_sidecar_complete_budget &owner)
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
		const size_t count =
			fixed_size + collector.eligible_item_uids.size() * sizeof(uint64_t);
		if (!owner.fresh<uint8_t>(count, sizeof(std::vector<uint8_t>)))
			return false;
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
bool item_sidecar_complete_decode_collector_context(const uint8_t *encoded, size_t size,
						    item_collector_death_enrollment *collector,
						    item_sidecar_complete_budget &owner)
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
		if (!owner.fresh<uint64_t>(count, 0))
			return false;
		collector->eligible_item_uids.reserve(count);
		for (uint32_t index = 0; index < count; ++index)
		{
			if (!owner.growth(collector->eligible_item_uids, 1))
				return false;
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
bool item_continuation_duplicate_valid_complete_owned(const item_transfer_payload &payload,
						      uint16_t payload_version,
						      item_native_validation_complete_budget &owner)
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
	if (!owner.peak(sizeof(std::vector<uint64_t>)))
		return false;
	std::vector<uint64_t> child_uids;
	owner.uids = &child_uids;
	try
	{
		if (!owner.peak(direct_child_count * sizeof(uint64_t)))
			return false;
		child_uids.reserve(direct_child_count);
		for (size_t index = 0; index < direct_child_count; ++index)
		{
			const uint64_t uid = get_u64(data.data() + 40 + index * sizeof(uint64_t));
			const item_transfer_entry *child =
				item_complete_find_payload_item(payload, uid);
			if (!uid || !child || child->parent_item_uid != duplicate_uid ||
			    std::find(child_uids.begin(), child_uids.end(), uid) !=
				    child_uids.end())
				return false;
			if (!owner.peak(0))
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
bool item_continuation_craft_outputs_complete_owned(const item_transfer_payload &payload,
						    std::vector<player_item_snapshot> *outputs,
						    item_native_validation_complete_budget &owner)
{
	if (!outputs || payload.reason != item_transfer_reason::craft ||
	    payload.item_blob_size > payload.item_blob.size())
		return false;
	outputs->clear();
	if (!payload.item_blob_size)
		return true;
	size_t prefix = 0;
	if (!owner.prefix(prefix))
		return false;
	if (item_complete_snapshot_list_decode_child(
		    payload.item_blob.data(), payload.item_blob_size, outputs, owner.reserve,
		    owner.context, prefix, nullptr) != player_snapshot_codec_result::ok ||
	    outputs->empty() || outputs->size() > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	if (!owner.peak(sizeof(std::vector<uint64_t>)))
		return false;
	std::vector<uint64_t> uids;
	owner.uids = &uids;
	try
	{
		if (outputs->size() > SIZE_MAX / sizeof(uint64_t) ||
		    !owner.peak(outputs->size() * sizeof(uint64_t)))
			return false;
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
			if (!owner.peak(0))
				return false;
			uids.push_back(output.object_uid);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	size_t levels = 0, remaining = uids.size();
	while (remaining > 1)
	{
		remaining >>= 1;
		++levels;
	}
	if (!owner.peak((2 * levels + 1) * item_complete_native_uid_sort_recursive_source))
		return false;
	std::sort(uids.begin(), uids.end());
	return std::adjacent_find(uids.begin(), uids.end()) == uids.end();
}
bool item_complete_general_validated_entities_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_general_entities_retained_source, child = 0;
	// Full prospective maximum covers validated general counts <=3004; real
	// runtime sort_frame keeps its original actual count/depth arithmetic.
	if (!payload_clone_add(
		    result,
		    23 * item_complete_general_entities_sort_recursive_source<
				 critical_entity_key, decltype(&critical_entity_key_less)>) ||
	    !payload_clone_add(
		    result,
		    23 * item_complete_general_entities_sort_recursive_source<
				 critical_expected_revision, item_entities_revision_less>) ||
	    !item_owner_key_source_frame_bytes(&child) || !payload_clone_add(result, child) ||
	    !item_transfer_craft_outputs_decode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_complete_general_validated_entities_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_complete_general_validated_entities_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_general_entities_complete_budget);
	return true;
}
bool item_complete_general_craft_outputs_child(const item_transfer_payload &input,
					       std::vector<player_item_snapshot> *output,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer,
					       size_t *retained_heap) noexcept
{
	constexpr size_t query =
		3 * item_transfer_craft_outputs_decode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_craft_outputs_decode_complete_source_frame_bytes(&source) ||
	    !item_transfer_craft_outputs_decode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_craft_outputs_decode_complete_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_craft_outputs_decode_complete_bounded(input, output, reserve, context,
								   child_outer, retained_heap);
}
bool item_complete_general_entities_owner_key_child(const item_owner_identity &owner,
						    critical_entity_key *output,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer) noexcept
{
	constexpr size_t query = 3 * item_owner_key_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_owner_key_source_frame_bytes(&source) ||
	    !item_owner_key_initial_inline_bytes(&initial) ||
	    !item_owner_key_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_owner_key_bounded(owner, output, reserve, context, child_outer);
}
bool item_complete_general_validated_entities_bounded(critical_command *command,
						      const item_transfer_payload &payload,
						      bool (*reserve)(size_t, void *) noexcept,
						      void *context, size_t outer_live) noexcept
{
	if (!command || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_general_entities_retained_source +
				  7 * sizeof(critical_entity_key) +
				  sizeof(critical_expected_revision);
	item_general_entities_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(critical_command)))
		return false;
	try
	{
		// Only original entity vectors are populated; authentic existing command
		// payload/accounting/scalars stay untouched and caller-owned throughout.
		critical_command candidate = {};
		budget.command = &candidate;
		if (!item_general_entities_populate_complete_owned(&candidate, payload, budget) ||
		    !budget.peak())
			return false;
		command->keys = std::move(candidate.keys);
		command->expected_revisions = std::move(candidate.expected_revisions);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_general_entities_populate_complete_owned(critical_command *command,
						   const item_transfer_payload &payload,
						   item_general_entities_complete_budget &budget)
{
	critical_entity_key from_key = {}, to_key = {};
	size_t admission_prefix = 0;
	if (!command ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_general_entities_owner_key_child(payload.from_owner, &from_key,
							     budget.reserve, budget.context,
							     admission_prefix)) ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_general_entities_owner_key_child(
		      payload.to_owner, &to_key, budget.reserve, budget.context, admission_prefix)))
		return false;
	if (!budget.fresh<critical_entity_key>(
		    2, 2 * sizeof(critical_entity_key) +
			       sizeof(std::initializer_list<critical_entity_key>)))
		return false;
	command->keys = { from_key, to_key };
	if (!budget.fresh<critical_expected_revision>(
		    2, 2 * sizeof(critical_expected_revision) +
			       sizeof(std::initializer_list<critical_expected_revision>)))
		return false;
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
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(item_key);
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back(
			{ item_key, payload.items[index].expected_item_revision });
	}
	if (payload.target_parent_item_uid)
	{
		critical_entity_key parent_key = { critical_entity_type::item,
						   payload.target_parent_item_uid };
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(parent_key);
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back(
			{ parent_key, payload.expected_target_parent_revision });
	}
	if (payload.reason == item_transfer_reason::craft)
	{
		if (!budget.peak(sizeof(std::vector<player_item_snapshot>)))
			return false;
		std::vector<player_item_snapshot> outputs;
		budget.outputs = &outputs;
		if (!(budget.prefix(admission_prefix) &&
		      item_complete_general_craft_outputs_child(payload, &outputs, budget.reserve,
								budget.context, admission_prefix,
								nullptr)))
			return false;
		for (const player_item_snapshot &output : outputs)
		{
			const critical_entity_key output_key = { critical_entity_type::item,
								 output.object_uid };
			if (!budget.growth(command->keys, 1))
				return false;
			command->keys.push_back(output_key);
			if (!budget.growth(command->expected_revisions, 1))
				return false;
			command->expected_revisions.push_back(
				{ output_key, ITEM_TRANSFER_ABSENT_REVISION });
		}
		budget.outputs = nullptr;
	}
	if (payload.collector.present)
	{
		const critical_entity_key catalog_key = { critical_entity_type::collector,
							  COLLECTOR_CATALOG_KEY };
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(catalog_key);
		// The SQL repository takes the current catalog row lock before any item
		// lock; zero is a serialization key, not an optimistic catalog fence.
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back({ catalog_key, 0 });
	}
	if (payload.native_recovery.present &&
	    payload.native_mobile.action == item_native_mobile_action::consumption &&
	    !payload.native_cost.fee_only)
	{
		const critical_entity_key giver_key = { critical_entity_type::player,
							payload.native_recovery.player_pid };
		if (!budget.growth(command->keys, 1))
			return false;
		command->keys.push_back(giver_key);
		// A serialization key only; the acknowledged save fence remains distinct
		// from a player custody-owner revision and is checked by the original root.
		if (!budget.growth(command->expected_revisions, 1))
			return false;
		command->expected_revisions.push_back({ giver_key, 0 });
	}
	if (payload.native_recovery.present && command->keys.size() > CRITICAL_COMMAND_MAX_KEYS)
		return false;
	if (!budget.sort_frame<critical_entity_key, decltype(&critical_entity_key_less)>(
		    command->keys.size()))
		return false;
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	if (std::adjacent_find(command->keys.begin(), command->keys.end(),
			       critical_entity_key_equal) != command->keys.end())
		return false;
	if (!budget.sort_frame<critical_expected_revision, item_entities_revision_less>(
		    command->expected_revisions.size()))
		return false;
	std::sort(command->expected_revisions.begin(), command->expected_revisions.end(),
		  item_entities_revision_less{});
	return true;
}
bool item_full_payload_validation_complete_owned(const item_transfer_payload &payload,
						 uint16_t payload_version,
						 item_payload_validation_complete_budget &budget)
{
	size_t admission_prefix = 0;
	if (!budget.peak(sizeof(native_quest_coin_give_projection)))
		return false;
	// Every legacy/generic encoder must refuse rather than silently drop money.
	if (payload.native_money.present || payload.native_money.original_room_vnum ||
	    payload.native_money.player_wallet_mapping_id ||
	    payload.native_money.mobile_wallet_mapping_id ||
	    payload.native_money.projection != native_quest_coin_give_projection{})
		return false;
	if (!budget.peak(sizeof(native_quest_cost_projection)))
		return false;
	// Every legacy/generic encoder must refuse rather than silently drop cost.
	if (payload.native_cost.present || payload.native_cost.fee_only ||
	    payload.native_cost.completion_slot || payload.native_cost.wallet_mapping_id ||
	    payload.native_cost.projection != native_quest_cost_projection{})
		return false;
	if (!(budget.prefix(admission_prefix) &&
	      item_complete_general_mobile_valid_child(payload, payload_version, budget.reserve,
						       budget.context, admission_prefix)) ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_general_recovery_valid_child(payload, payload.native_recovery,
							 payload_version, budget.reserve,
							 budget.context, admission_prefix)))
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
	    !item_complete_valid_collector_context(payload, payload_version) ||
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
	      !(budget.prefix(admission_prefix) &&
		item_complete_general_offering_valid_child(payload, budget.reserve, budget.context,
							   admission_prefix)))) ||
	    (payload.continuation.kind == item_transfer_continuation_kind::soulbind_transfer &&
	     (payload_version < ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION ||
	      !item_complete_valid_soulbind_continuation(payload))) ||
	    (payload.continuation.kind ==
		     item_transfer_continuation_kind::spell_component_retirement &&
	     !valid_spell_component_continuation(payload, payload_version)) ||
	    (payload.continuation.kind ==
		     item_transfer_continuation_kind::account_reward_retirement &&
	     !valid_account_reward_retirement_continuation(payload, payload_version)) ||
	    (payload.continuation.kind ==
		     item_transfer_continuation_kind::account_reward_duplicate_promotion &&
	     !(budget.prefix(admission_prefix) &&
	       item_complete_general_duplicate_valid_child(payload, payload_version, budget.reserve,
							   budget.context, admission_prefix))) ||
	    (payload.continuation.kind == item_transfer_continuation_kind::craft_pouch_usage &&
	     payload_version < ITEM_TRANSFER_PAYLOAD_VERSION) ||
	    (payload.continuation.kind == item_transfer_continuation_kind::lockpick_retirement &&
	     (payload_version < ITEM_TRANSFER_CONTINUATION_PAYLOAD_VERSION ||
	      !(budget.prefix(admission_prefix) &&
		item_complete_general_lockpick_valid_child(payload, budget.reserve, budget.context,
							   admission_prefix)))) ||
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
	     payload.continuation.kind != item_transfer_continuation_kind::craft_recipe &&
	     payload.continuation.kind != item_transfer_continuation_kind::lockpick_retirement))
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
	     !item_complete_find_payload_item(payload, payload.selected_item_uid) ||
	     item_complete_find_payload_item(payload, payload.selected_item_uid)->parent_item_uid))
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
	     !item_complete_find_payload_item(payload, payload.selected_item_uid) ||
	     item_complete_find_payload_item(payload, payload.selected_item_uid)->parent_item_uid))
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
		if (!budget.peak(sizeof(std::vector<player_item_snapshot>) +
				 sizeof(craft_pouch_mutation)))
			return false;
		std::vector<player_item_snapshot> outputs;
		craft_pouch_mutation pouch;
		budget.outputs = &outputs;
		budget.pouch = &pouch;
		if (payload_version < ITEM_TRANSFER_BATCH_PAYLOAD_VERSION ||
		    payload.from_owner.type != item_owner_type::player ||
		    payload.to_owner.type != item_owner_type::player ||
		    !item_owner_identity_equal(payload.from_owner, payload.to_owner) ||
		    !payload.from_owner.id || payload.from_owner.context_id ||
		    payload.to_owner.context_id || !payload.multi_root ||
		    !payload.selected_item_uid || payload.target_root_item_uid ||
		    payload.target_parent_item_uid || payload.expected_target_parent_revision ||
		    !(budget.prefix(admission_prefix) &&
		      item_complete_general_craft_outputs_child(payload, &outputs, budget.reserve,
								budget.context, admission_prefix,
								nullptr)) ||
		    !(budget.prefix(admission_prefix) &&
		      item_complete_general_pouch_payload_child(
			      payload, &pouch, budget.reserve, budget.context, admission_prefix)) ||
		    (outputs.empty() &&
		     !item_complete_find_payload_item(payload, payload.selected_item_uid)) ||
		    payload.item_count + outputs.size() > CRITICAL_COMMAND_MAX_KEYS - 2)
			return false;
		if (payload.continuation.kind == item_transfer_continuation_kind::craft_recipe)
		{
			if (!budget.peak(sizeof(craft_recipe_continuation)))
				return false;
			craft_recipe_continuation recipe;
			budget.recipe = &recipe;
			if (!(budget.prefix(admission_prefix) &&
			      item_complete_general_recipe_decode_child(
				      payload.continuation.data, &recipe, budget.reserve,
				      budget.context, admission_prefix)) ||
			    (!budget.peak() ||
			     !craft_recipe_continuation_matches(recipe, payload)) ||
			    (craft_recipe_is_alchemy(recipe.discipline) ||
					     recipe.discipline == craft_recipe_discipline::refine ?
				     static_cast<uint32_t>(std::count_if(
					     outputs.begin(), outputs.end(),
					     item_complete_native_recovery_root_count_predicate{})) !=
						     recipe.output_count ||
					     (!outputs.empty() &&
					      outputs[0].object_uid != recipe.output_uid) :
				     outputs.size() != 1 ||
					     outputs[0].object_uid != recipe.output_uid ||
					     outputs[0].vnum !=
						     static_cast<int32_t>(recipe.recipe_vnum)) ||
			    (recipe.discipline == craft_recipe_discipline::refine &&
			     (!outputs.empty() &&
			      (outputs.size() != 1 ||
			       outputs[0].vnum != static_cast<int32_t>(recipe.recipe_vnum)))))
				return false;
			budget.recipe = nullptr;
		}
		for (const player_item_snapshot &output : outputs)
			for (size_t index = 0; index < payload.item_count; ++index)
				if (output.object_uid == payload.items[index].item_uid)
					return false;
		// Original locals die at this brace; later census must not read them.
		budget.outputs = nullptr;
		budget.pouch = nullptr;
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
		    !item_complete_valid_text(payload.corpse.owner_name,
					      ITEM_TRANSFER_CORPSE_NAME_MAX_BYTES, true) ||
		    !item_complete_valid_text(payload.corpse.short_description,
					      ITEM_TRANSFER_CORPSE_SHORT_DESCRIPTION_MAX_BYTES,
					      false) ||
		    !item_complete_valid_text(payload.corpse.description,
					      ITEM_TRANSFER_CORPSE_DESCRIPTION_MAX_BYTES, false) ||
		    !item_complete_valid_text(payload.corpse.keywords,
					      ITEM_TRANSFER_CORPSE_KEYWORDS_MAX_BYTES, false))
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
					item_complete_selected_root_for(payload, entry.item_uid);
				const item_transfer_entry *root =
					item_complete_find_payload_item(payload, selected_root);
				uint64_t target_root = 0, target_parent = 0;
				if (!root || selected_root != entry.root_item_uid ||
				    root->root_item_uid != root->item_uid ||
				    root->parent_item_uid ||
				    !item_complete_target_topology_for(payload, entry.item_uid,
								       &target_root,
								       &target_parent) ||
				    target_root != entry.root_item_uid ||
				    target_parent != entry.parent_item_uid)
					return false;
				continue;
			}
			const uint64_t selected_root =
				item_complete_selected_root_for(payload, entry.item_uid);
			const item_transfer_entry *selected =
				item_complete_find_payload_item(payload, selected_root);
			if (!selected || selected->root_item_uid != entry.root_item_uid)
				return false;
			uint64_t target_root = 0, target_parent = 0;
			if (!item_complete_target_topology_for(payload, entry.item_uid,
							       &target_root, &target_parent))
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
			auto parent = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				item_complete_validation_ancestor_predicate{ ancestor_uid });
			if (parent == payload.items.begin() + payload.item_count)
				break;
			ancestor_uid = parent->parent_item_uid;
		}
		if (!reaches_root)
			return false;
	}
	return true;
}
bool item_complete_general_mobile_valid_child(const item_transfer_payload &input, uint16_t version,
					      bool (*reserve)(size_t, void *) noexcept,
					      void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_mobile_context_valid_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_mobile_context_valid_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_mobile_context_valid_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_mobile_context_valid_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_mobile_context_valid_complete_bounded(input, version, reserve,
									  context, child_outer);
}
bool item_complete_general_recovery_valid_child(const item_transfer_payload &input,
						const item_native_mobile_recovery_context &recovery,
						uint16_t version,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_recovery_valid_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_recovery_valid_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_recovery_valid_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_recovery_valid_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_recovery_valid_complete_bounded(input, recovery, version,
								    reserve, context, child_outer);
}
bool item_complete_general_offering_valid_child(const item_transfer_payload &input,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 *
		item_transfer_quest_offering_continuation_valid_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_quest_offering_continuation_valid_complete_source_frame_bytes(&source) ||
	    !item_transfer_quest_offering_continuation_valid_complete_initial_inline_bytes(
		    &initial) ||
	    !item_transfer_quest_offering_continuation_valid_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_quest_offering_continuation_valid_complete_bounded(
		input, reserve, context, child_outer);
}
bool item_complete_general_duplicate_valid_child(const item_transfer_payload &input,
						 uint16_t version,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 *
		item_transfer_duplicate_promotion_continuation_valid_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_duplicate_promotion_continuation_valid_complete_source_frame_bytes(
		    &source) ||
	    !item_transfer_duplicate_promotion_continuation_valid_complete_initial_inline_bytes(
		    &initial) ||
	    !item_transfer_duplicate_promotion_continuation_valid_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_duplicate_promotion_continuation_valid_complete_bounded(
		input, version, reserve, context, child_outer);
}
bool item_complete_general_lockpick_valid_child(const item_transfer_payload &input,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * lockpick_retirement_payload_valid_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !lockpick_retirement_payload_valid_complete_source_frame_bytes(&source) ||
	    !lockpick_retirement_payload_valid_complete_initial_inline_bytes(&initial) ||
	    !lockpick_retirement_payload_valid_complete_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return lockpick_retirement_payload_valid_complete_bounded(input, reserve, context,
								  child_outer);
}
bool item_complete_general_pouch_payload_child(const item_transfer_payload &input,
					       craft_pouch_mutation *output,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * craft_pouch_mutation_from_payload_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !craft_pouch_mutation_from_payload_complete_source_frame_bytes(&source) ||
	    !craft_pouch_mutation_from_payload_complete_initial_inline_bytes(&initial) ||
	    !craft_pouch_mutation_from_payload_complete_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return craft_pouch_mutation_from_payload_complete_bounded(input, output, reserve, context,
								  child_outer);
}
bool item_complete_general_recipe_decode_child(std::span<const uint8_t> input,
					       craft_recipe_continuation *output,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * craft_recipe_continuation_decode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !craft_recipe_continuation_decode_complete_source_frame_bytes(&source) ||
	    !craft_recipe_continuation_decode_complete_initial_inline_bytes(&initial) ||
	    !craft_recipe_continuation_decode_complete_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return craft_recipe_continuation_decode_complete_bounded(input, output, reserve, context,
								 child_outer);
}
bool item_complete_general_payload_valid_child(const item_transfer_payload &value, uint16_t version,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_payload_valid_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_payload_valid_complete_source_frame_bytes(&source) ||
	    !item_transfer_payload_valid_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_payload_valid_complete_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_payload_valid_complete_bounded(value, version, reserve, context,
							    child_outer);
}
bool item_complete_general_validated_entities_child(critical_command *output,
						    const item_transfer_payload &value,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_complete_general_validated_entities_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_complete_general_validated_entities_source_frame_bytes(&source) ||
	    !item_complete_general_validated_entities_initial_inline_bytes(&initial) ||
	    !item_complete_general_validated_entities_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_complete_general_validated_entities_bounded(output, value, reserve, context,
								child_outer);
}
bool item_generic_decode_complete_owned(const critical_command &command,
					item_transfer_payload *payload,
					item_generic_decode_complete_budget &budget)
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
	if (!budget.peak(sizeof(item_transfer_payload)))
		return false;
	*payload = {};
	size_t admission_prefix = 0;
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
			    !(budget.prefix(admission_prefix) &&
			      item_complete_corpse_decode_child(
				      command.payload.data() + item_end + sizeof(uint32_t),
				      corpse_size, &payload->corpse, budget.reserve, budget.context,
				      admission_prefix)))
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
				    !(budget.prefix(admission_prefix) &&
				      item_complete_collector_decode_child(
					      command.payload.data() + corpse_end + sizeof(uint32_t),
					      collector_size, &payload->collector, budget.reserve,
					      budget.context, admission_prefix)))
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
					if (!budget.peak(continuation_size))
						return false;
					payload->continuation.data.assign(
						command.payload.begin() + source_end +
							sizeof(uint32_t) * 2,
						command.payload.begin() + source_end +
							sizeof(uint32_t) * 2 + continuation_size);
					if (native_mobile_version(command.payload_version))
					{
						if (!budget.prefix(admission_prefix))
							return false;
						const uint8_t *tail =
							command.payload.data() + source_end +
							sizeof(uint32_t) * 2 + continuation_size;
						if (get_u16(tail) !=
							    ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_VERSION ||
						    tail[3] ||
						    get_u32(tail + 4) !=
							    ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES ||
						    get_u32(tail + 12) || get_u32(tail + 164) ||
						    item_complete_native_reference_decode_child(
							    std::span<const uint8_t>(
								    tail + 16,
								    QUEST_MOBILE_NATIVE_REFERENCE_BYTES),
							    &payload->native_mobile.reference,
							    budget.reserve, budget.context,
							    admission_prefix) !=
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
							if (!(budget.prefix(admission_prefix) &&
							      item_complete_native_recovery_decode_entry_child(
								      std::span<const uint8_t>(
									      command.payload)
									      .subspan(offset),
								      &payload->native_recovery,
								      budget.reserve,
								      budget.context,
								      admission_prefix, nullptr)))
								return false;
						}
					}
				}
			}
		}
	}
	if (!(budget.prefix(admission_prefix) &&
	      item_complete_general_payload_valid_child(*payload, command.payload_version,
							budget.reserve, budget.context,
							admission_prefix)) ||
	    (command.payload_version == ITEM_TRANSFER_LEGACY_PAYLOAD_VERSION &&
	     payload->reason > item_transfer_reason::auction_claim) ||
	    command.expected_revisions.size() != command.keys.size())
		return false;
	if (!budget.peak(sizeof(critical_command)))
		return false;
	critical_command expected = {};
	budget.expected = &expected;
	if (!(budget.prefix(admission_prefix) &&
	      item_complete_general_validated_entities_child(&expected, *payload, budget.reserve,
							     budget.context, admission_prefix)))
		return false;
	if (!budget.peak())
		return false;
	const bool matched =
		command.keys.size() == expected.keys.size() &&
		command.expected_revisions.size() == expected.expected_revisions.size() &&
		std::equal(command.keys.begin(), command.keys.end(), expected.keys.begin(),
			   item_complete_generic_key_equal{}) &&
		std::equal(command.expected_revisions.begin(), command.expected_revisions.end(),
			   expected.expected_revisions.begin(),
			   item_complete_generic_revision_equal{});
	budget.expected = nullptr;
	return matched;
}
bool item_generic_wire_encode_complete_owned(const item_transfer_payload &payload, uint16_t version,
					     std::vector<uint8_t> *encoded,
					     item_payload_wire_complete_budget &budget)
{
	if (!budget.peak(3 * sizeof(std::vector<uint8_t>)))
		return false;
	std::vector<uint8_t> corpse_context;
	std::vector<uint8_t> collector_context;
	std::vector<uint8_t> recovery_context;
	budget.corpse = &corpse_context;
	budget.collector = &collector_context;
	budget.recovery = &recovery_context;
	size_t admission_prefix = 0;
	if (!encoded ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_general_payload_valid_child(payload, version, budget.reserve,
							budget.context, admission_prefix)) ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_corpse_encode_child(payload.corpse, &corpse_context, budget.reserve,
						budget.context, admission_prefix)) ||
	    !(budget.prefix(admission_prefix) &&
	      item_complete_collector_encode_child(payload.collector, &collector_context,
						   budget.reserve, budget.context,
						   admission_prefix)))
		return false;
	if (version == ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION &&
	    !(budget.prefix(admission_prefix) &&
	      item_complete_native_recovery_encode_entry_child(
		      payload.native_recovery, &recovery_context, budget.reserve, budget.context,
		      admission_prefix, nullptr)))
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
	if (!budget.peak(payload_size + item_complete_generic_fresh_fill_tmp_inline +
			 item_complete_generic_fresh_fill_zero_inline))
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
	// Lockpick retirement has no destination tree; keep its validated zero root.
	put_u64(encoded->data() + TARGET_ROOT_OFFSET,
		(payload.multi_root ||
		 payload.continuation.kind ==
			 item_transfer_continuation_kind::lockpick_retirement) ?
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
		if (!budget.peak(sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>)))
			return false;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> reference = {};
		if (!budget.prefix(admission_prefix, sizeof(reference)))
			return false;
		if (item_complete_native_reference_encode_child(
			    payload.native_mobile.reference, &reference, budget.reserve,
			    budget.context, admission_prefix) != player_snapshot_codec_result::ok)
			return false;
		std::copy(reference.begin(), reference.end(), output + 16);
		std::copy(recovery_context.begin(), recovery_context.end(),
			  output + ITEM_TRANSFER_NATIVE_MOBILE_CONTEXT_BYTES);
	}
	budget.corpse = nullptr;
	budget.collector = nullptr;
	budget.recovery = nullptr;
	return true;
}
bool item_complete_corpse_encode_child(const item_corpse_metadata &input,
				       std::vector<uint8_t> *output,
				       bool (*reserve)(size_t, void *) noexcept, void *context,
				       size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_corpse_context_encode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_corpse_context_encode_complete_source_frame_bytes(&source) ||
	    !item_transfer_corpse_context_encode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_corpse_context_encode_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	// Parent storage and old/foreign heaps stay in outer. The actual child
	// now self-owns Source/controller once; full transient never retained.
	return item_transfer_corpse_context_encode_complete_bounded(input, output, reserve, context,
								    child_outer);
}
bool item_complete_corpse_decode_child(const uint8_t *input, size_t size,
				       item_corpse_metadata *output,
				       bool (*reserve)(size_t, void *) noexcept, void *context,
				       size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_corpse_context_decode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_corpse_context_decode_complete_source_frame_bytes(&source) ||
	    !item_transfer_corpse_context_decode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_corpse_context_decode_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	// Parent storage and old/foreign heaps stay in outer. The actual child
	// now self-owns Source/controller once; full transient never retained.
	return item_transfer_corpse_context_decode_complete_bounded(input, size, output, reserve,
								    context, child_outer);
}
bool item_complete_collector_encode_child(const item_collector_death_enrollment &input,
					  std::vector<uint8_t> *output,
					  bool (*reserve)(size_t, void *) noexcept, void *context,
					  size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_collector_context_encode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_collector_context_encode_complete_source_frame_bytes(&source) ||
	    !item_transfer_collector_context_encode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_collector_context_encode_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	// Parent storage and old/foreign heaps stay in outer. The actual child
	// now self-owns Source/controller once; full transient never retained.
	return item_transfer_collector_context_encode_complete_bounded(input, output, reserve,
								       context, child_outer);
}
bool item_complete_collector_decode_child(const uint8_t *input, size_t size,
					  item_collector_death_enrollment *output,
					  bool (*reserve)(size_t, void *) noexcept, void *context,
					  size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_collector_context_decode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0;
	size_t request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_collector_context_decode_complete_source_frame_bytes(&source) ||
	    !item_transfer_collector_context_decode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_collector_context_decode_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	// Parent storage and old/foreign heaps stay in outer. The actual child
	// now self-owns Source/controller once; full transient never retained.
	return item_transfer_collector_context_decode_complete_bounded(input, size, output, reserve,
								       context, child_outer);
}
bool item_complete_general_payload_encode_child(const item_transfer_payload &value,
						uint16_t version, std::vector<uint8_t> *output,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer, size_t *heap) noexcept
{
	constexpr size_t query =
		3 * item_transfer_payload_encode_version_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_payload_encode_version_complete_source_frame_bytes(&source) ||
	    !item_transfer_payload_encode_version_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_payload_encode_version_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_payload_encode_version_complete_bounded(
		value, version, output, reserve, context, child_outer, heap);
}
bool item_complete_general_payload_decode_child(const critical_command &value,
						item_transfer_payload *output,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer, size_t *heap) noexcept
{
	constexpr size_t query =
		3 * item_transfer_payload_decode_generic_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_payload_decode_generic_complete_source_frame_bytes(&source) ||
	    !item_transfer_payload_decode_generic_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_payload_decode_generic_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_payload_decode_generic_complete_bounded(value, output, reserve,
								     context, child_outer, heap);
}
bool item_complete_general_native_cost_encode_child(const item_transfer_payload &value,
						    bool acknowledged, std::vector<uint8_t> *output,
						    bool (*reserve)(size_t, void *) noexcept,
						    void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_cost_encode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_cost_encode_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_cost_encode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_cost_encode_complete_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_cost_encode_complete_bounded(value, acknowledged, output,
								 reserve, context, child_outer);
}
bool item_complete_general_public_native_encode_child(const item_transfer_payload &value,
						      std::vector<uint8_t> *output,
						      bool (*reserve)(size_t, void *) noexcept,
						      void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_complete_general_public_encode_native_only_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_complete_general_public_encode_native_only_source_frame_bytes(&source) ||
	    !item_complete_general_public_encode_native_only_initial_inline_bytes(&initial) ||
	    !item_complete_general_public_encode_native_only_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_command_encode_native_mobile_complete_bounded(value, output, reserve,
									   context, child_outer);
}
bool item_complete_general_public_native_recovery_encode_child(
	const item_transfer_payload &value, std::vector<uint8_t> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 *
		item_complete_general_public_recovery_encode_native_only_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_complete_general_public_recovery_encode_native_only_source_frame_bytes(&source) ||
	    !item_complete_general_public_recovery_encode_native_only_initial_inline_bytes(
		    &initial) ||
	    !item_complete_general_public_recovery_encode_native_only_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_command_encode_native_mobile_recovery_complete_bounded(
		value, output, reserve, context, child_outer);
}
bool item_complete_general_public_inner_decode_child(const critical_command &value,
						     item_transfer_payload *output,
						     bool (*reserve)(size_t, void *) noexcept,
						     void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_complete_general_public_decode_inner_native_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_complete_general_public_decode_inner_native_source_frame_bytes(&source) ||
	    !item_complete_general_public_decode_inner_native_initial_inline_bytes(&initial) ||
	    !item_complete_general_public_decode_inner_native_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_command_decode_payload_complete_bounded(value, output, reserve,
								     context, child_outer);
}
bool item_complete_general_fee_decode_child(const critical_command &value,
					    item_transfer_payload *output,
					    bool (*reserve)(size_t, void *) noexcept, void *context,
					    size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_fee_decode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_fee_decode_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_fee_decode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_fee_decode_complete_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_fee_decode_complete_bounded(value, output, reserve, context,
								child_outer);
}
bool item_complete_general_money_decode_child(const critical_command &value,
					      item_transfer_payload *output,
					      bool (*reserve)(size_t, void *) noexcept,
					      void *context, size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_native_money_decode_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_native_money_decode_complete_source_frame_bytes(&source) ||
	    !item_transfer_native_money_decode_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_native_money_decode_complete_source_supplement_frame_bytes(&supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_native_money_decode_complete_bounded(value, output, reserve, context,
								  child_outer);
}
bool item_encode_native_cost_complete_owned(const item_transfer_payload &payload, bool recovery,
					    std::vector<uint8_t> *encoded,
					    item_native_wire_encode_complete_budget &budget)
{
	size_t admission_prefix = 0;
	if (!encoded || !(budget.prefix(admission_prefix) &&
			  item_complete_native_cost_value_entry_child(
				  payload, budget.reserve, budget.context, admission_prefix)))
		return false;
	if (payload.native_cost.fee_only)
		return (budget.prefix(admission_prefix) &&
			item_complete_native_fee_encode_entry_child(payload, recovery, encoded,
								    budget.reserve, budget.context,
								    admission_prefix));
	if (payload.native_cost.completion_slot)
		return false;
	if (!budget.clone_payload(payload))
		return false;
	auto original = payload;
	budget.original = &original;
	if (!budget.peak(sizeof(item_native_mobile_cost_context)))
		return false;
	original.native_cost = {};
	if (!budget.peak(2 * sizeof(std::vector<uint8_t>)))
		return false;
	std::vector<uint8_t> body, cost;
	budget.first = &body;
	budget.second = &cost;
	const uint16_t original_version =
		recovery ? ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION :
			   ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION;
	if (!(budget.prefix(admission_prefix) &&
	      item_complete_general_payload_encode_child(original, original_version, &body,
							 budget.reserve, budget.context,
							 admission_prefix, nullptr)) ||
	    (!budget.prefix(admission_prefix) ?
		     native_quest_cost_projection_result::invalid :
		     item_complete_native_cost_encode_child(
			     payload.native_cost.projection, &cost, budget.reserve, budget.context,
			     admission_prefix)) != native_quest_cost_projection_result::ok ||
	    body.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES -
				  ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES ||
	    cost.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES -
				  ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES - body.size())
		return false;
	if (!budget.peak(sizeof(std::vector<uint8_t>) +
			 ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES + body.size() + cost.size()))
		return false;
	std::vector<uint8_t> exact(ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES + body.size() +
				   cost.size());
	budget.value = &exact;
	exact[0] = 'N';
	exact[1] = 'Q';
	exact[2] = 'F';
	exact[3] = '1';
	put_u16(exact.data() + 4, 1);
	put_u16(exact.data() + 6, original_version);
	put_u32(exact.data() + 8, static_cast<uint32_t>(body.size()));
	put_u32(exact.data() + 12, static_cast<uint32_t>(cost.size()));
	put_u64(exact.data() + 16, payload.native_cost.wallet_mapping_id);
	std::copy(body.begin(), body.end(),
		  exact.begin() + ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES);
	std::copy(cost.begin(), cost.end(),
		  exact.begin() + ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES + body.size());
	if (!budget.peak())
		return false;
	*encoded = std::move(exact);
	return true;
}
bool item_public_decode_complete_owned(const critical_command &command,
				       item_transfer_payload *payload,
				       item_public_dispatch_complete_budget &budget)
{
	size_t admission_prefix = 0;
	if (native_money_version(command.payload_version))
	{
		try
		{
			return (budget.prefix(admission_prefix) &&
				item_complete_general_money_decode_child(
					command, payload, budget.reserve, budget.context,
					admission_prefix));
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	if (native_cost_version(command.payload_version))
	{
		if (command.payload.size() >= 4 && command.payload[0] == 'N' &&
		    command.payload[1] == 'Q' && command.payload[2] == 'F' &&
		    command.payload[3] == '2')
			try
			{
				return (budget.prefix(admission_prefix) &&
					item_complete_general_fee_decode_child(
						command, payload, budget.reserve, budget.context,
						admission_prefix));
			}
			catch (const std::bad_alloc &)
			{
				return false;
			}
		if (!payload || command.type != critical_command_type::item_transfer ||
		    command.payload.size() < ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES ||
		    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
			return false;
		try
		{
			const auto *wire = command.payload.data();
			const uint16_t inner_version =
				command.payload_version ==
						ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION ?
					ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION :
					ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION;
			if (wire[0] != 'N' || wire[1] != 'Q' || wire[2] != 'F' || wire[3] != '1' ||
			    get_u16(wire + 4) != 1 || get_u16(wire + 6) != inner_version ||
			    !get_u64(wire + 16))
				return false;
			const size_t body_size = get_u32(wire + 8), cost_size = get_u32(wire + 12);
			const size_t available = command.payload.size() -
						 ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES;
			if (body_size > available || cost_size != available - body_size)
				return false;
			if (!budget.command_copy(command))
				return false;
			auto original = command;
			budget.original = &original;
			original.payload_version = inner_version;
			if (!budget.peak())
				return false;
			original.payload.assign(
				command.payload.begin() +
					ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES,
				command.payload.begin() +
					ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES + body_size);
			if (!budget.peak(sizeof(item_transfer_payload)))
				return false;
			item_transfer_payload candidate{};
			budget.candidate = &candidate;
			if (!(budget.prefix(admission_prefix) &&
			      item_complete_general_public_inner_decode_child(
				      original, &candidate, budget.reserve, budget.context,
				      admission_prefix)))
				return false;
			candidate.native_cost.present = true;
			candidate.native_cost.wallet_mapping_id = get_u64(wire + 16);
			if ((!budget.prefix(admission_prefix) ?
				     native_quest_cost_projection_result::invalid :
				     item_complete_native_cost_decode_child(
					     { wire + ITEM_TRANSFER_NATIVE_MOBILE_COST_HEADER_BYTES +
						       body_size,
					       cost_size },
					     &candidate.native_cost.projection, budget.reserve,
					     budget.context, admission_prefix)) !=
				    native_quest_cost_projection_result::ok ||
			    !(budget.prefix(admission_prefix) &&
			      item_complete_native_cost_value_entry_child(
				      candidate, budget.reserve, budget.context, admission_prefix)))
				return false;
			if (!budget.peak(sizeof(std::vector<uint8_t>)))
				return false;
			std::vector<uint8_t> canonical;
			budget.canonical = &canonical;
			if (!(budget.prefix(admission_prefix) &&
			      item_complete_general_native_cost_encode_child(
				      candidate,
				      inner_version ==
					      ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION,
				      &canonical, budget.reserve, budget.context,
				      admission_prefix)) ||
			    (!budget.peak() || canonical != command.payload))
				return false;
			if (!budget.peak())
				return false;
			*payload = std::move(candidate);
			return true;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	if (!native_mobile_version(command.payload_version))
		return (budget.prefix(admission_prefix) &&
			item_complete_general_payload_decode_child(command, payload, budget.reserve,
								   budget.context, admission_prefix,
								   nullptr));
	if (!payload)
		return false;
	try
	{
		if (!budget.peak(sizeof(item_transfer_payload)))
			return false;
		item_transfer_payload candidate = {};
		budget.candidate = &candidate;
		if (!(budget.prefix(admission_prefix) &&
		      item_complete_general_payload_decode_child(command, &candidate,
								 budget.reserve, budget.context,
								 admission_prefix, nullptr)))
			return false;
		if (!budget.peak(sizeof(std::vector<uint8_t>)))
			return false;
		std::vector<uint8_t> canonical;
		budget.canonical = &canonical;
		if (!(command.payload_version ==
				      ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION ?
			      (budget.prefix(admission_prefix) &&
			       item_complete_general_public_native_recovery_encode_child(
				       candidate, &canonical, budget.reserve, budget.context,
				       admission_prefix)) :
			      (budget.prefix(admission_prefix) &&
			       item_complete_general_public_native_encode_child(
				       candidate, &canonical, budget.reserve, budget.context,
				       admission_prefix))) ||
		    (!budget.peak() || canonical != command.payload))
			return false;
		if (!budget.peak())
			return false;
		*payload = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_public_encode_complete_owned(const item_transfer_payload &payload,
				       std::vector<uint8_t> *encoded,
				       item_public_dispatch_complete_budget &budget)
{
	size_t admission_prefix = 0;
	if (!encoded)
		return false;
	try
	{
		if (!budget.peak(sizeof(std::vector<uint8_t>)))
			return false;
		std::vector<uint8_t> candidate;
		budget.canonical = &candidate;
		if (!(payload.native_money.present ?
			      (budget.prefix(admission_prefix) &&
			       item_complete_native_money_encode_entry_child(
				       payload, false, &candidate, budget.reserve, budget.context,
				       admission_prefix)) :
		      payload.native_cost.present ?
			      (budget.prefix(admission_prefix) &&
			       item_complete_general_native_cost_encode_child(
				       payload, false, &candidate, budget.reserve, budget.context,
				       admission_prefix)) :
			      (budget.prefix(admission_prefix) &&
			       item_complete_general_payload_encode_child(
				       payload, ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION,
				       &candidate, budget.reserve, budget.context, admission_prefix,
				       nullptr))))
			return false;
		if (!budget.peak())
			return false;
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_public_encode_recovery_complete_owned(const item_transfer_payload &payload,
						std::vector<uint8_t> *encoded,
						item_public_dispatch_complete_budget &budget)
{
	size_t admission_prefix = 0;
	if (!encoded)
		return false;
	try
	{
		if (!budget.peak(sizeof(std::vector<uint8_t>)))
			return false;
		std::vector<uint8_t> candidate;
		budget.canonical = &candidate;
		if (!(payload.native_money.present ?
			      (budget.prefix(admission_prefix) &&
			       item_complete_native_money_encode_entry_child(
				       payload, true, &candidate, budget.reserve, budget.context,
				       admission_prefix)) :
		      payload.native_cost.present ?
			      (budget.prefix(admission_prefix) &&
			       item_complete_general_native_cost_encode_child(
				       payload, true, &candidate, budget.reserve, budget.context,
				       admission_prefix)) :
			      (budget.prefix(admission_prefix) &&
			       item_complete_general_payload_encode_child(
				       payload, ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION,
				       &candidate, budget.reserve, budget.context, admission_prefix,
				       nullptr))))
			return false;
		if (!budget.peak())
			return false;
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_complete_general_public_encode_native_only_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_general_public_encode_retained_source, child = 0;
	if (!item_transfer_payload_encode_version_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_complete_general_public_encode_native_only_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_complete_general_public_encode_native_only_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_public_dispatch_complete_budget);
	return true;
}
bool item_complete_general_public_recovery_encode_native_only_source_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_general_public_recovery_encode_retained_source, child = 0;
	if (!item_transfer_payload_encode_version_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_complete_general_public_recovery_encode_native_only_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_complete_general_public_recovery_encode_native_only_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_public_dispatch_complete_budget);
	return true;
}
bool item_complete_general_public_decode_inner_native_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_general_public_decode_retained_source, child = 0;
	if (!item_transfer_payload_decode_generic_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_complete_general_public_encode_native_only_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_complete_general_public_recovery_encode_native_only_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_complete_general_public_decode_inner_native_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_complete_general_public_decode_inner_native_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_public_dispatch_complete_budget);
	return true;
}
bool item_complete_valid_collector_context(const item_transfer_payload &payload,
					   uint16_t payload_version)
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
			   item_complete_collector_uid_present{ payload });
}
bool item_complete_valid_text(const std::string &value, size_t maximum, bool required)
{
	if ((required && value.empty()) || value.size() > maximum)
		return false;
	return std::all_of(value.begin(), value.end(), item_complete_text_printable_predicate{});
}
bool item_complete_valid_soulbind_continuation(const item_transfer_payload &payload)
{
	return payload.continuation.data.size() == 1 && payload.continuation.data[0] <= 1 &&
	       payload.reason == item_transfer_reason::soulbind && !payload.multi_root &&
	       payload.from_owner.type == item_owner_type::player &&
	       payload.to_owner.type == item_owner_type::player && payload.selected_item_uid &&
	       payload.item_count > 0 &&
	       item_complete_find_payload_item(payload, payload.selected_item_uid) != nullptr;
}
bool item_complete_target_topology_for(const item_transfer_payload &payload, uint64_t item_uid,
				       uint64_t *root_item_uid, uint64_t *parent_item_uid)
{
	const item_transfer_entry *entry = item_complete_find_payload_item(payload, item_uid);
	const uint64_t selected_root = item_complete_selected_root_for(payload, item_uid);
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
uint64_t item_complete_selected_root_for(const item_transfer_payload &payload, uint64_t item_uid)
{
	if (payload.reason == item_transfer_reason::craft)
	{
		const item_transfer_entry *entry =
			item_complete_find_payload_item(payload, item_uid);
		for (size_t depth = 0; entry && depth <= payload.item_count; ++depth)
		{
			const item_transfer_entry *parent =
				item_complete_find_payload_item(payload, entry->parent_item_uid);
			if (!parent)
				return entry->item_uid;
			entry = parent;
		}
		return 0;
	}
	if (!payload.multi_root)
		return payload.selected_item_uid ? payload.selected_item_uid :
						   payload.items[0].root_item_uid;
	const item_transfer_entry *entry = item_complete_find_payload_item(payload, item_uid);
	for (size_t depth = 0; entry && depth <= payload.item_count; ++depth)
	{
		const item_transfer_entry *parent =
			item_complete_find_payload_item(payload, entry->parent_item_uid);
		if (!parent)
			return entry->item_uid;
		entry = parent;
	}
	return 0;
}
} // namespace
bool item_transfer_corpse_context_encode_complete_bounded(const item_corpse_metadata &corpse,
							  std::vector<uint8_t> *encoded,
							  bool (*reserve)(size_t, void *) noexcept,
							  void *context, size_t outer) noexcept
{
	if (!encoded || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames =
		item_complete_sidecar_corpse_encode_retained_source + sizeof(std::vector<uint8_t>);
	item_sidecar_complete_budget owner{ reserve, context, outer, frames };
	if (!owner.peak())
		return false;
	try
	{
		std::vector<uint8_t> candidate;
		owner.bytes = &candidate;
		if (!item_sidecar_complete_encode_corpse_context(corpse, &candidate, owner) ||
		    !owner.peak())
			return false;
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_corpse_context_decode_complete_bounded(const uint8_t *encoded, size_t size,
							  item_corpse_metadata *corpse,
							  bool (*reserve)(size_t, void *) noexcept,
							  void *context, size_t outer) noexcept
{
	if (!corpse || (!encoded && size) || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_sidecar_corpse_decode_retained_source +
				  2 * sizeof(item_corpse_metadata);
	item_sidecar_complete_budget owner{ reserve, context, outer, frames };
	if (!owner.peak())
		return false;
	try
	{
		item_corpse_metadata candidate;
		owner.corpse = &candidate;
		if (!item_sidecar_complete_decode_corpse_context(encoded, size, &candidate,
								 owner) ||
		    !owner.peak())
			return false;
		static_assert(std::is_nothrow_move_assignable_v<item_corpse_metadata>);
		*corpse = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_collector_context_encode_complete_bounded(
	const item_collector_death_enrollment &collector, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!encoded || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_sidecar_collector_encode_retained_source +
				  sizeof(std::vector<uint8_t>);
	item_sidecar_complete_budget owner{ reserve, context, outer, frames };
	if (!owner.peak())
		return false;
	try
	{
		std::vector<uint8_t> candidate;
		owner.bytes = &candidate;
		if (!item_sidecar_complete_encode_collector_context(collector, &candidate, owner) ||
		    !owner.peak())
			return false;
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_collector_context_decode_complete_bounded(
	const uint8_t *encoded, size_t size, item_collector_death_enrollment *collector,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!collector || (!encoded && size) || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_sidecar_collector_decode_retained_source +
				  2 * sizeof(item_collector_death_enrollment);
	item_sidecar_complete_budget owner{ reserve, context, outer, frames };
	if (!owner.peak())
		return false;
	try
	{
		item_collector_death_enrollment candidate;
		owner.collector = &candidate;
		if (!item_sidecar_complete_decode_collector_context(encoded, size, &candidate,
								    owner) ||
		    !owner.peak())
			return false;
		static_assert(std::is_nothrow_move_assignable_v<item_collector_death_enrollment>);
		*collector = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_corpse_context_encode_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = item_complete_sidecar_corpse_encode_retained_source;
	return true;
}
bool item_transfer_corpse_context_encode_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_corpse_context_encode_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_sidecar_complete_budget);
	return true;
}
bool item_transfer_corpse_context_decode_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = item_complete_sidecar_corpse_decode_retained_source;
	return true;
}
bool item_transfer_corpse_context_decode_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_corpse_context_decode_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_sidecar_complete_budget);
	return true;
}
bool item_transfer_collector_context_encode_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = item_complete_sidecar_collector_encode_retained_source;
	return true;
}
bool item_transfer_collector_context_encode_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_collector_context_encode_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_sidecar_complete_budget);
	return true;
}
bool item_transfer_collector_context_decode_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = item_complete_sidecar_collector_decode_retained_source;
	return true;
}
bool item_transfer_collector_context_decode_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_collector_context_decode_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_sidecar_complete_budget);
	return true;
}
bool item_transfer_duplicate_promotion_continuation_valid_complete_bounded(
	const item_transfer_payload &payload, uint16_t version,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_duplicate_retained_source;
	item_native_validation_complete_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak())
		return false;
	try
	{
		return item_continuation_duplicate_valid_complete_owned(payload, version, owner);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_craft_outputs_decode_complete_bounded(const item_transfer_payload &payload,
							 std::vector<player_item_snapshot> *outputs,
							 bool (*reserve)(size_t, void *) noexcept,
							 void *context, size_t outer_live,
							 size_t *retained_item_heap_bytes) noexcept
{
	if (!outputs || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_craft_outputs_retained_source;
	item_native_validation_complete_budget owner{ reserve, context, outer_live, frames };
	if (!owner.peak(sizeof(std::vector<player_item_snapshot>)))
		return false;
	std::vector<player_item_snapshot> candidate;
	owner.items = &candidate;
	static_assert(std::is_nothrow_move_assignable_v<std::vector<player_item_snapshot>>);
	try
	{
		if (!item_continuation_craft_outputs_complete_owned(payload, &candidate, owner))
			return false;
		// Private UID scratch has died; stop observing that expired local.
		owner.uids = nullptr;
		if (!owner.peak(0))
			return false;
		size_t retained = 0, row = 0;
		if (!payload_clone_vector_heap(candidate, false, retained))
			return false;
		for (const auto &item : candidate)
			if (!player_item_snapshot_current_heap_bytes(item, &row) ||
			    !payload_clone_add(retained, row))
				return false;
		*outputs = std::move(candidate);
		if (retained_item_heap_bytes)
			*retained_item_heap_bytes = retained;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_duplicate_promotion_continuation_valid_complete_source_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = item_complete_duplicate_retained_source;
	return true;
}
bool item_transfer_duplicate_promotion_continuation_valid_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_duplicate_promotion_continuation_valid_complete_initial_inline_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_native_validation_complete_budget);
	return true;
}
bool item_transfer_craft_outputs_decode_complete_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_craft_outputs_retained_source +
			item_complete_native_uid_sort_max_recursive_calls *
				item_complete_native_uid_sort_recursive_source,
	       child = 0;
	if (!player_item_snapshot_list_decode_bounded_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*bytes = result;
	return true;
}
bool item_transfer_craft_outputs_decode_complete_source_supplement_frame_bytes(
	size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = 0;
	return true;
}
bool item_transfer_craft_outputs_decode_complete_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes || !payload_clone_policy_supported())
		return false;
	*bytes = sizeof(item_native_validation_complete_budget);
	return true;
}
bool item_transfer_payload_valid_complete_bounded(const item_transfer_payload &payload,
						  uint16_t version,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer_live) noexcept
{
	if (!reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_general_validator_retained_source;
	item_payload_validation_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_full_payload_validation_complete_owned(payload, version, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_payload_valid_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_general_validator_retained_source, child = 0;
	if (!item_transfer_native_mobile_context_valid_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_native_recovery_valid_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_quest_offering_continuation_valid_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_duplicate_promotion_continuation_valid_complete_source_frame_bytes(
		    &child) ||
	    !payload_clone_add(result, child) ||
	    !lockpick_retirement_payload_valid_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_craft_outputs_decode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !craft_pouch_mutation_from_payload_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !craft_recipe_continuation_decode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_payload_valid_complete_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_payload_valid_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_payload_validation_complete_budget);
	return true;
}
bool item_transfer_payload_decode_generic_complete_bounded(
	const critical_command &command, item_transfer_payload *payload,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_payload_heap_bytes) noexcept
{
	if (!payload || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_general_generic_decode_retained_source +
				  sizeof(item_owner_identity) + sizeof(item_transfer_entry) +
				  sizeof(std::span<const uint8_t>);
	item_generic_decode_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(item_transfer_payload)))
		return false;
	try
	{
		item_transfer_payload candidate = {};
		budget.payload = &candidate;
		if (!item_generic_decode_complete_owned(command, &candidate, budget) ||
		    !budget.peak())
			return false;
		size_t heap = 0;
		if (!payload_clone_heap(candidate, false, heap))
			return false;
		static_assert(std::is_nothrow_move_assignable_v<item_transfer_payload>);
		*payload = std::move(candidate);
		if (retained_payload_heap_bytes)
			*retained_payload_heap_bytes = heap;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_payload_encode_version_complete_bounded(
	const item_transfer_payload &payload, uint16_t version, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_encoded_heap_bytes) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_general_wire_encode_retained_source;
	item_payload_wire_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(sizeof(std::vector<uint8_t>)))
		return false;
	try
	{
		std::vector<uint8_t> candidate;
		budget.bytes = &candidate;
		if (!item_generic_wire_encode_complete_owned(payload, version, &candidate,
							     budget) ||
		    !budget.peak())
			return false;
		const size_t heap = candidate.capacity();
		*encoded = std::move(candidate);
		if (retained_encoded_heap_bytes)
			*retained_encoded_heap_bytes = heap;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_payload_encode_version_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_general_wire_encode_retained_source, child = 0;
	if (!item_transfer_payload_valid_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_corpse_context_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_collector_context_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !quest_mobile_native_reference_encode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_native_recovery_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_payload_encode_version_complete_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_payload_encode_version_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_payload_wire_complete_budget);
	return true;
}
bool item_transfer_payload_decode_generic_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_general_generic_decode_retained_source, child = 0;
	if (!item_transfer_payload_valid_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_complete_general_validated_entities_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_corpse_context_decode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_collector_context_decode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !quest_mobile_native_reference_decode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_native_recovery_decode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_payload_decode_generic_complete_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_payload_decode_generic_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_generic_decode_complete_budget);
	return true;
}
bool item_transfer_command_decode_payload_complete_bounded(const critical_command &command,
							   item_transfer_payload *payload,
							   bool (*reserve)(size_t, void *) noexcept,
							   void *context,
							   size_t outer_live) noexcept
{
	if (!payload || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_general_public_decode_retained_source;
	item_public_dispatch_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_public_decode_complete_owned(command, payload, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_command_encode_native_mobile_complete_bounded(
	const item_transfer_payload &payload, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_general_public_encode_retained_source;
	item_public_dispatch_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_public_encode_complete_owned(payload, encoded, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_command_encode_native_mobile_recovery_complete_bounded(
	const item_transfer_payload &payload, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_general_public_recovery_encode_retained_source;
	item_public_dispatch_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_public_encode_recovery_complete_owned(payload, encoded, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_cost_encode_complete_bounded(const item_transfer_payload &payload,
						       bool acknowledged,
						       std::vector<uint8_t> *encoded,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer_live) noexcept
{
	if (!encoded || !reserve || !payload_clone_policy_supported())
		return false;
	constexpr size_t frames = item_complete_general_native_cost_encode_retained_source;
	item_native_wire_encode_complete_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return item_encode_native_cost_complete_owned(payload, acknowledged, encoded,
							      budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_native_cost_encode_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_general_native_cost_encode_retained_source, child = 0;
	if (!item_transfer_native_cost_value_valid_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_native_fee_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_payload_encode_version_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !native_quest_cost_projection_encode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_native_cost_encode_complete_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_native_cost_encode_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_native_wire_encode_complete_budget);
	return true;
}
bool item_transfer_command_encode_native_mobile_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_general_public_encode_retained_source, child = 0;
	if (!item_transfer_native_money_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_native_cost_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_payload_encode_version_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_command_encode_native_mobile_complete_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_command_encode_native_mobile_complete_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_public_dispatch_complete_budget);
	return true;
}
bool item_transfer_command_encode_native_mobile_recovery_complete_source_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_general_public_recovery_encode_retained_source, child = 0;
	if (!item_transfer_native_money_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_native_cost_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_payload_encode_version_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_command_encode_native_mobile_recovery_complete_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_command_encode_native_mobile_recovery_complete_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_public_dispatch_complete_budget);
	return true;
}
bool item_transfer_command_decode_payload_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_general_public_decode_retained_source, child = 0;
	if (!item_transfer_native_money_decode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_native_fee_decode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_payload_decode_generic_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_complete_general_public_decode_inner_native_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !native_quest_cost_projection_decode_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_native_cost_value_valid_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_transfer_native_cost_encode_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_complete_general_public_encode_native_only_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_complete_general_public_recovery_encode_native_only_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_command_decode_payload_complete_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_command_decode_payload_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_public_dispatch_complete_budget);
	return true;
}
#else
bool item_transfer_corpse_context_encode_complete_bounded(const item_corpse_metadata &,
							  std::vector<uint8_t> *,
							  bool (*)(size_t, void *) noexcept, void *,
							  size_t) noexcept
{
	return false;
}
bool item_transfer_corpse_context_decode_complete_bounded(const uint8_t *, size_t,
							  item_corpse_metadata *,
							  bool (*)(size_t, void *) noexcept, void *,
							  size_t) noexcept
{
	return false;
}
bool item_transfer_collector_context_encode_complete_bounded(
	const item_collector_death_enrollment &, std::vector<uint8_t> *,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept
{
	return false;
}
bool item_transfer_collector_context_decode_complete_bounded(const uint8_t *, size_t,
							     item_collector_death_enrollment *,
							     bool (*)(size_t, void *) noexcept,
							     void *, size_t) noexcept
{
	return false;
}
bool item_transfer_corpse_context_encode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_corpse_context_encode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_corpse_context_encode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_corpse_context_decode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_corpse_context_decode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_corpse_context_decode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_collector_context_encode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_collector_context_encode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_collector_context_encode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_collector_context_decode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_collector_context_decode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_collector_context_decode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_duplicate_promotion_continuation_valid_complete_bounded(
	const item_transfer_payload &, uint16_t, bool (*)(size_t, void *) noexcept, void *,
	size_t) noexcept
{
	return false;
}
bool item_transfer_craft_outputs_decode_complete_bounded(const item_transfer_payload &,
							 std::vector<player_item_snapshot> *,
							 bool (*)(size_t, void *) noexcept, void *,
							 size_t, size_t *) noexcept
{
	return false;
}
bool item_transfer_duplicate_promotion_continuation_valid_complete_source_frame_bytes(
	size_t *) noexcept
{
	return false;
}
bool item_transfer_duplicate_promotion_continuation_valid_complete_source_supplement_frame_bytes(
	size_t *) noexcept
{
	return false;
}
bool item_transfer_duplicate_promotion_continuation_valid_complete_initial_inline_bytes(
	size_t *) noexcept
{
	return false;
}
bool item_transfer_craft_outputs_decode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_craft_outputs_decode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_craft_outputs_decode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_payload_valid_complete_bounded(const item_transfer_payload &, uint16_t,
						  bool (*)(size_t, void *) noexcept, void *,
						  size_t) noexcept
{
	return false;
}
bool item_transfer_payload_valid_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_payload_valid_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_payload_valid_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_payload_decode_generic_complete_bounded(const critical_command &,
							   item_transfer_payload *,
							   bool (*)(size_t, void *) noexcept,
							   void *, size_t, size_t *) noexcept
{
	return false;
}
bool item_transfer_payload_encode_version_complete_bounded(const item_transfer_payload &, uint16_t,
							   std::vector<uint8_t> *,
							   bool (*)(size_t, void *) noexcept,
							   void *, size_t, size_t *) noexcept
{
	return false;
}
bool item_transfer_payload_encode_version_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_payload_encode_version_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_payload_encode_version_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_payload_decode_generic_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_payload_decode_generic_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_payload_decode_generic_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_command_decode_payload_complete_bounded(const critical_command &,
							   item_transfer_payload *,
							   bool (*)(size_t, void *) noexcept,
							   void *, size_t) noexcept
{
	return false;
}
bool item_transfer_command_encode_native_mobile_complete_bounded(const item_transfer_payload &,
								 std::vector<uint8_t> *,
								 bool (*)(size_t, void *) noexcept,
								 void *, size_t) noexcept
{
	return false;
}
bool item_transfer_command_encode_native_mobile_recovery_complete_bounded(
	const item_transfer_payload &, std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept,
	void *, size_t) noexcept
{
	return false;
}
bool item_transfer_native_cost_encode_complete_bounded(const item_transfer_payload &, bool,
						       std::vector<uint8_t> *,
						       bool (*)(size_t, void *) noexcept, void *,
						       size_t) noexcept
{
	return false;
}
bool item_transfer_native_cost_encode_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_cost_encode_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_native_cost_encode_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_command_encode_native_mobile_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_command_encode_native_mobile_complete_source_supplement_frame_bytes(
	size_t *) noexcept
{
	return false;
}
bool item_transfer_command_encode_native_mobile_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_command_encode_native_mobile_recovery_complete_source_frame_bytes(
	size_t *) noexcept
{
	return false;
}
bool item_transfer_command_encode_native_mobile_recovery_complete_source_supplement_frame_bytes(
	size_t *) noexcept
{
	return false;
}
bool item_transfer_command_encode_native_mobile_recovery_complete_initial_inline_bytes(
	size_t *) noexcept
{
	return false;
}
bool item_transfer_command_decode_payload_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_command_decode_payload_complete_source_supplement_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_command_decode_payload_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
#endif

// Complete strong-output companion for the original native-mobile value builder.
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) && \
	!defined(_GLIBCXX_CONCEPT_CHECKS)
namespace
{
using item_native_build_reserve = bool (*)(size_t, void *) noexcept;
struct item_native_build_complete_budget
{
	item_native_build_reserve reserve;
	void *context;
	size_t outer, frames;
	const std::vector<uint8_t> *encoded = nullptr;
	const critical_command *candidate = nullptr;
	bool command_heap(const critical_command &value, size_t &total) const noexcept
	{
		return payload_clone_add(total, sizeof(value)) &&
		       payload_clone_vector_heap(value.keys, false, total) &&
		       payload_clone_vector_heap(value.expected_revisions, false, total) &&
		       payload_clone_vector_heap(value.payload, false, total) &&
		       payload_clone_vector_heap(value.accounting_intent, false, total);
	}
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		size_t total = outer;
		if (!payload_clone_add(total, sizeof(*this)) || !payload_clone_add(total, frames) ||
		    (encoded && (!payload_clone_add(total, sizeof(*encoded)) ||
				 !payload_clone_vector_heap(*encoded, false, total))) ||
		    (candidate && !command_heap(*candidate, total)) ||
		    !payload_clone_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
// Actual shared byte-vector MOVE CONSTRUCTION, not MOVE assignment:
// vector/base moves2P each; impl/allocator/newallocator/data moves2P each;
// completed std::move<_Vector_impl&>2P. Caller std::move<vector<byte>&>2P separate.
constexpr size_t item_complete_native_builder_byte_move_constructor_source =
	14 * sizeof(void *) + 2 * sizeof(void *);
constexpr size_t item_complete_native_builder_lifecycle_source =
	// K/R default candidate members, U encoded/accounting defaults share U.
	item_complete_command_aggregate_default_source +
	item_complete_native_builder_byte_move_constructor_source +
	// Destructor receiver plus genuine K/R/U tails; encoded is null after
	// transfer but may still be nonempty on pre-candidate refusal.
	item_complete_command_cleanup_source + item_complete_command_move_source -
	// Only same completed typed cleanup descendants are shared with the
	// selected assignment's genuine internal temporary cleanup. Receiver stays.
	(item_complete_command_cleanup_source - sizeof(void *)) +
	2 * sizeof(void *); // Actual caller std::move<critical_command>&.
constexpr size_t item_complete_native_builder_controller_source =
	// prefix(this,result,extra,total,bool), peak(this,extra,total,bool),
	// command_heap(this,value,total,bool). Physical controller remains INITIAL.
	6 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool) +
	item_complete_sidecar_checked_add_source +
	item_complete_general_vector_current_source<critical_entity_key> +
	item_complete_general_vector_current_source<critical_expected_revision> +
	item_complete_general_vector_current_source<uint8_t>;
constexpr size_t item_complete_native_builder_adapter_source =
	// Each actual adapter: value/output/context3P + callbackF, outer and
	// six genuine N locals (query/source/initial/supp/request/child_outer).
	2 * (3 * sizeof(void *) + sizeof(item_native_build_reserve) + 7 * sizeof(size_t) +
	     sizeof(bool));
constexpr size_t item_complete_native_builder_retained_source =
	// command/payload/context/catch4P; operation/source/deadline real values;
	// outer/frames/admission_prefix3N, callbackF and returned bool.
	4 * sizeof(void *) + sizeof(critical_operation_id) + sizeof(critical_source_site) +
	sizeof(critical_deadline_class) + sizeof(item_native_build_reserve) + 3 * sizeof(size_t) +
	2 * sizeof(bool) + item_complete_native_builder_controller_source +
	item_complete_native_builder_adapter_source +
	item_complete_native_builder_lifecycle_source +
	item_complete_collector_operation_zero_source +
	3 * item_transfer_command_encode_native_mobile_complete_source_query_frame_bytes() +
	3 * item_complete_general_validated_entities_source_query_frame_bytes();
bool item_complete_native_builder_encode_child(const item_transfer_payload &value,
					       std::vector<uint8_t> *output,
					       item_native_build_reserve reserve, void *context,
					       size_t outer) noexcept
{
	constexpr size_t query =
		3 * item_transfer_command_encode_native_mobile_complete_source_query_frame_bytes();
	size_t source = 0, initial = 0, supplement = 0, request = outer;
	if (!reserve || !payload_clone_add(request, query) || !reserve(request, context) ||
	    !item_transfer_command_encode_native_mobile_complete_source_frame_bytes(&source) ||
	    !item_transfer_command_encode_native_mobile_complete_initial_inline_bytes(&initial) ||
	    !item_transfer_command_encode_native_mobile_complete_source_supplement_frame_bytes(
		    &supplement))
		return false;
	request = outer;
	if (!payload_clone_add(request, source) || !payload_clone_add(request, initial) ||
	    !reserve(request, context))
		return false;
	size_t child_outer = outer;
	if (!payload_clone_add(child_outer, supplement))
		return false;
	return item_transfer_command_encode_native_mobile_complete_bounded(value, output, reserve,
									   context, child_outer);
}
}
bool item_transfer_command_build_native_mobile_complete_bounded(
	critical_command *command, critical_operation_id operation_id,
	const item_transfer_payload &payload, critical_source_site source_site,
	critical_deadline_class deadline_class, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
	if (!command || !reserve || !payload_clone_policy_supported() ||
	    critical_operation_id_is_zero(operation_id))
		return false;
	constexpr size_t frames = item_complete_native_builder_retained_source;
	item_native_build_complete_budget budget{ reserve, context, outer_live, frames };
	try
	{
		if (!budget.peak(sizeof(std::vector<uint8_t>)))
			return false;
		std::vector<uint8_t> encoded;
		budget.encoded = &encoded;
		size_t admission_prefix = 0;
		if (!budget.prefix(admission_prefix) ||
		    !item_complete_native_builder_encode_child(payload, &encoded, reserve, context,
							       admission_prefix))
			return false;
		if (!budget.peak(sizeof(critical_command)))
			return false;
		critical_command candidate = {
			.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
			.operation_id = operation_id,
			.type = critical_command_type::item_transfer,
			.payload_version =
				payload.native_money.present ?
					ITEM_TRANSFER_NATIVE_MOBILE_MONEY_PAYLOAD_VERSION :
				payload.native_cost.present ?
					ITEM_TRANSFER_NATIVE_MOBILE_COST_PAYLOAD_VERSION :
					ITEM_TRANSFER_NATIVE_MOBILE_PAYLOAD_VERSION,
			.source_site = source_site,
			.deadline_class = deadline_class,
			.accepted_at_usec = 0,
			.keys = {},
			.expected_revisions = {},
			.payload = std::move(encoded)
		};
		budget.candidate = &candidate;
		// Successful authentic native encode validates the same payload first.
		// Child's separate candidate remains child-owned, never this candidate.
		if (!budget.prefix(admission_prefix) ||
		    !item_complete_general_validated_entities_child(&candidate, payload, reserve,
								    context, admission_prefix) ||
		    !budget.peak())
			return false;
		static_assert(std::is_nothrow_move_assignable_v<critical_command>);
		*command = std::move(candidate);
		budget.encoded = nullptr;
		budget.candidate = nullptr;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool item_transfer_command_build_native_mobile_complete_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	size_t result = item_complete_native_builder_retained_source, child = 0;
	if (!item_transfer_command_encode_native_mobile_complete_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child) ||
	    !item_complete_general_validated_entities_source_frame_bytes(&child) ||
	    !payload_clone_add(result, child))
		return false;
	*output = result;
	return true;
}
bool item_transfer_command_build_native_mobile_complete_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = 0;
	return true;
}
bool item_transfer_command_build_native_mobile_complete_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !payload_clone_policy_supported())
		return false;
	*output = sizeof(item_native_build_complete_budget);
	return true;
}
#else
bool item_transfer_command_build_native_mobile_complete_bounded(
	critical_command *, critical_operation_id, const item_transfer_payload &,
	critical_source_site, critical_deadline_class, bool (*)(size_t, void *) noexcept, void *,
	size_t) noexcept
{
	return false;
}
bool item_transfer_command_build_native_mobile_complete_source_frame_bytes(size_t *) noexcept
{
	return false;
}
bool item_transfer_command_build_native_mobile_complete_source_supplement_frame_bytes(
	size_t *) noexcept
{
	return false;
}
bool item_transfer_command_build_native_mobile_complete_initial_inline_bytes(size_t *) noexcept
{
	return false;
}
#endif
