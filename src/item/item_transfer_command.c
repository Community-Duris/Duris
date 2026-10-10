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
