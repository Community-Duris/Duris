#include "shop_trade_command.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

namespace
{
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

bool creation(const shop_trade_payload &payload)
{
	return payload.action == shop_trade_action::buy_produced;
}

bool valid_payload(const shop_trade_payload &payload)
{
	if (payload.action <= shop_trade_action::unknown ||
	    payload.action > shop_trade_action::sell_destroy || !payload.player_pid ||
	    !valid_name(payload.account_name) || payload.price <= 0 || payload.price > INT_MAX ||
	    !payload.expected_shop_revision || !payload.selected_item_uid || !payload.item_count ||
	    payload.item_count > payload.items.size() || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size())
		return false;
	const bool creates = creation(payload);
	if (!payload.target_root_item_uid)
		return false;
	const uint64_t target_root = payload.target_root_item_uid;
	if ((payload.target_parent_item_uid &&
	     (!creates || target_root == payload.selected_item_uid ||
	      !payload.expected_target_parent_revision)) ||
	    (!payload.target_parent_item_uid &&
	     (target_root != payload.selected_item_uid || payload.expected_target_parent_revision)))
		return false;
	if (creates)
	{
		if (!payload.stock_item_uid ||
		    payload.stock_item_uid == payload.selected_item_uid ||
		    !payload.expected_stock_item_revision ||
		    payload.expected_stock_item_revision == ITEM_TRANSFER_ABSENT_REVISION ||
		    payload.stock_vnum <= 0)
			return false;
	}
	else if (payload.action == shop_trade_action::buy_existing)
	{
		if (payload.stock_item_uid != payload.selected_item_uid ||
		    !payload.expected_stock_item_revision || payload.stock_vnum <= 0)
			return false;
	}
	else if (payload.stock_item_uid || payload.expected_stock_item_revision ||
		 payload.stock_vnum)
		return false;
	const uint64_t root_uid = payload.items[0].root_item_uid;
	if (root_uid != payload.selected_item_uid)
		return false;
	bool selected = false;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &item = payload.items[index];
		if (!item.item_uid || !root_uid || item.root_item_uid != root_uid ||
		    item.vnum <= 0 || (creates && item.item_uid == payload.stock_item_uid) ||
		    item.expected_state !=
			    (creates ? item_custody_state::absent : item_custody_state::active) ||
		    (creates ? item.expected_item_revision != ITEM_TRANSFER_ABSENT_REVISION :
			       !item.expected_item_revision ||
				       item.expected_item_revision ==
					       ITEM_TRANSFER_ABSENT_REVISION) ||
		    item.item_uid == payload.target_parent_item_uid ||
		    (index && payload.items[index - 1].item_uid >= item.item_uid))
			return false;
		if (item.item_uid == payload.selected_item_uid)
		{
			if (selected || item.parent_item_uid)
				return false;
			selected = true;
		}
		else if (!item.parent_item_uid)
			return false;
	}
	if (!selected)
		return false;
	const auto root = std::find_if(payload.items.begin(),
				       payload.items.begin() + payload.item_count,
				       [&](const auto &item)
				       { return item.item_uid == payload.selected_item_uid; });
	if (root == payload.items.begin() + payload.item_count ||
	    ((creates || payload.action == shop_trade_action::buy_existing) &&
	     (payload.stock_vnum != root->vnum ||
	      (!creates && payload.expected_stock_item_revision != root->expected_item_revision))))
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		if (payload.items[index].item_uid == payload.selected_item_uid)
			continue;
		uint64_t parent_uid = payload.items[index].parent_item_uid;
		bool reaches_selected = false;
		for (size_t depth = 0; depth < payload.item_count; ++depth)
		{
			if (parent_uid == payload.selected_item_uid)
			{
				reaches_selected = true;
				break;
			}
			auto parent = std::find_if(payload.items.begin(),
						   payload.items.begin() + payload.item_count,
						   [&](const auto &candidate)
						   { return candidate.item_uid == parent_uid; });
			if (parent == payload.items.begin() + payload.item_count)
				break;
			parent_uid = parent->parent_item_uid;
		}
		if (!reaches_selected)
			return false;
	}
	return true;
}

bool matching_fences(const critical_command &left, const critical_command &right)
{
	return left.keys.size() == right.keys.size() &&
	       std::equal(left.keys.begin(), left.keys.end(), right.keys.begin(),
			  critical_entity_key_equal) &&
	       left.expected_revisions.size() == right.expected_revisions.size() &&
	       std::equal(left.expected_revisions.begin(), left.expected_revisions.end(),
			  right.expected_revisions.begin(),
			  [](const auto &first, const auto &second) {
				  return critical_entity_key_equal(first.key, second.key) &&
					 first.revision == second.revision;
			  });
}

void put_u16(uint8_t *output, uint16_t value)
{
	output[0] = static_cast<uint8_t>(value);
	output[1] = static_cast<uint8_t>(value >> 8);
}

void put_u64(uint8_t *output, uint64_t value)
{
	for (size_t byte = 0; byte < sizeof(value); ++byte)
		output[byte] = static_cast<uint8_t>(value >> (byte * 8));
}

uint16_t get_u16(const uint8_t *input)
{
	return static_cast<uint16_t>(input[0]) |
	       static_cast<uint16_t>(static_cast<uint16_t>(input[1]) << 8);
}

uint64_t get_u64(const uint8_t *input)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < sizeof(value); ++byte)
		value |= static_cast<uint64_t>(input[byte]) << (byte * 8);
	return value;
}
} // namespace

bool shop_trade_command_encode_payload(const shop_trade_payload &payload,
				       std::vector<uint8_t> *encoded)
{
	if (!encoded || !valid_payload(payload))
		return false;
	try
	{
		encoded->clear();
		encoded->reserve(128 + payload.item_count * 40 + payload.item_blob_size);
		append_le<uint8_t>(encoded, static_cast<uint8_t>(payload.action));
		append_le<uint32_t>(encoded, payload.player_pid);
		append_le<uint32_t>(encoded, payload.shop_id);
		append_le<uint8_t>(encoded, payload.racewar);
		append_le<int64_t>(encoded, payload.price);
		append_le<uint64_t>(encoded, payload.expected_wallet_revision);
		append_le<uint64_t>(encoded, payload.expected_bank_revision);
		append_le<uint64_t>(encoded, payload.expected_shop_revision);
		append_le<uint64_t>(encoded, payload.selected_item_uid);
		append_le<uint64_t>(encoded, payload.target_root_item_uid);
		append_le<uint64_t>(encoded, payload.target_parent_item_uid);
		append_le<uint64_t>(encoded, payload.expected_target_parent_revision);
		append_le<uint64_t>(encoded, payload.stock_item_uid);
		append_le<uint64_t>(encoded, payload.expected_stock_item_revision);
		append_le<int32_t>(encoded, payload.stock_vnum);
		append_le<uint16_t>(encoded, payload.item_count);
		const size_t name_length =
			strnlen(payload.account_name.data(), payload.account_name.size());
		append_le<uint8_t>(encoded, static_cast<uint8_t>(name_length));
		encoded->insert(encoded->end(), payload.account_name.begin(),
				payload.account_name.begin() + name_length);
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto &item = payload.items[index];
			append_le<uint64_t>(encoded, item.item_uid);
			append_le<uint64_t>(encoded, item.root_item_uid);
			append_le<uint64_t>(encoded, item.parent_item_uid);
			append_le<uint64_t>(encoded, item.expected_item_revision);
			append_le<int32_t>(encoded, item.vnum);
			append_le<uint8_t>(encoded, static_cast<uint8_t>(item.expected_state));
		}
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

bool shop_trade_command_decode_payload(const critical_command &command, shop_trade_payload *payload)
{
	if (!payload || command.type != critical_command_type::shop_trade ||
	    (command.payload_version != SHOP_TRADE_PAYLOAD_VERSION &&
	     command.payload_version != SHOP_TRADE_PREVIOUS_PAYLOAD_VERSION &&
	     command.payload_version != SHOP_TRADE_LEGACY_PAYLOAD_VERSION))
		return false;
	*payload = {};
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	uint8_t action = 0, name_length = 0;
	if (!read_le(&cursor, end, &action) || !read_le(&cursor, end, &payload->player_pid) ||
	    !read_le(&cursor, end, &payload->shop_id) ||
	    !read_le(&cursor, end, &payload->racewar) || !read_le(&cursor, end, &payload->price) ||
	    !read_le(&cursor, end, &payload->expected_wallet_revision) ||
	    !read_le(&cursor, end, &payload->expected_bank_revision) ||
	    !read_le(&cursor, end, &payload->expected_shop_revision) ||
	    !read_le(&cursor, end, &payload->selected_item_uid))
		return false;
	if (command.payload_version == SHOP_TRADE_PAYLOAD_VERSION &&
	    (!read_le(&cursor, end, &payload->target_root_item_uid) ||
	     !read_le(&cursor, end, &payload->target_parent_item_uid) ||
	     !read_le(&cursor, end, &payload->expected_target_parent_revision)))
		return false;
	else if (command.payload_version != SHOP_TRADE_PAYLOAD_VERSION)
		payload->target_root_item_uid = payload->selected_item_uid;
	if (command.payload_version != SHOP_TRADE_LEGACY_PAYLOAD_VERSION &&
	    (!read_le(&cursor, end, &payload->stock_item_uid) ||
	     !read_le(&cursor, end, &payload->expected_stock_item_revision) ||
	     !read_le(&cursor, end, &payload->stock_vnum)))
		return false;
	if (!read_le(&cursor, end, &payload->item_count) || !read_le(&cursor, end, &name_length) ||
	    !name_length || name_length > CURRENCY_ACCOUNT_NAME_MAX_BYTES ||
	    static_cast<size_t>(end - cursor) < name_length ||
	    payload->item_count > payload->items.size())
		return false;
	payload->action = static_cast<shop_trade_action>(action);
	memcpy(payload->account_name.data(), cursor, name_length);
	cursor += name_length;
	for (size_t index = 0; index < payload->item_count; ++index)
	{
		uint8_t state = 0;
		auto &item = payload->items[index];
		if (!read_le(&cursor, end, &item.item_uid) ||
		    !read_le(&cursor, end, &item.root_item_uid) ||
		    !read_le(&cursor, end, &item.parent_item_uid) ||
		    !read_le(&cursor, end, &item.expected_item_revision) ||
		    !read_le(&cursor, end, &item.vnum) || !read_le(&cursor, end, &state))
			return false;
		item.expected_state = static_cast<item_custody_state>(state);
	}
	if (!read_le(&cursor, end, &payload->item_blob_size) || !payload->item_blob_size ||
	    payload->item_blob_size > payload->item_blob.size() ||
	    static_cast<size_t>(end - cursor) != payload->item_blob_size)
		return false;
	memcpy(payload->item_blob.data(), cursor, payload->item_blob_size);
	if (command.payload_version == SHOP_TRADE_LEGACY_PAYLOAD_VERSION)
	{
		if (payload->action == shop_trade_action::buy_produced)
			return false;
		if (payload->action == shop_trade_action::buy_existing)
		{
			auto selected = std::find_if(
				payload->items.begin(),
				payload->items.begin() + payload->item_count, [&](const auto &item)
				{ return item.item_uid == payload->selected_item_uid; });
			if (selected == payload->items.begin() + payload->item_count)
				return false;
			payload->stock_item_uid = selected->item_uid;
			payload->expected_stock_item_revision = selected->expected_item_revision;
			payload->stock_vnum = selected->vnum;
		}
	}
	if (!valid_payload(*payload))
		return false;
	critical_command expected = {};
	return shop_trade_command_build(&expected, command.operation_id, *payload,
					command.source_site, command.deadline_class) &&
	       matching_fences(expected, command);
}

bool shop_trade_command_encode_result(const shop_trade_result &result,
				      std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> *encoded)
{
	if (!encoded || result.action <= shop_trade_action::unknown ||
	    result.action > shop_trade_action::sell_destroy ||
	    result.item_count > result.item_uids.size())
		return false;
	for (size_t index = 0; index < result.item_count; ++index)
		if (!result.item_uids[index])
			return false;
	encoded->fill(0);
	(*encoded)[0] = static_cast<uint8_t>(result.action);
	(*encoded)[1] = SHOP_TRADE_RESULT_VERSION;
	put_u16(encoded->data() + 2, result.item_count);
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
	{
		put_u64(encoded->data() + 8 + index * 8,
			static_cast<uint64_t>(result.wallet.amount[index]));
		put_u64(encoded->data() + 40 + index * 8,
			static_cast<uint64_t>(result.bank.amount[index]));
	}
	put_u64(encoded->data() + 72, result.wallet_revision);
	put_u64(encoded->data() + 80, result.bank_revision);
	put_u64(encoded->data() + 88, result.shop_revision);
	put_u64(encoded->data() + 96, result.player_owner_revision);
	put_u64(encoded->data() + 104, result.counterparty_owner_revision);
	for (size_t index = 0; index < result.item_count; ++index)
	{
		put_u64(encoded->data() + 112 + index * 16, result.item_uids[index]);
		put_u64(encoded->data() + 120 + index * 16, result.item_revisions[index]);
	}
	return true;
}

bool shop_trade_command_decode_result(const uint8_t *encoded, size_t size,
				      shop_trade_result *result)
{
	if (!encoded || size != SHOP_TRADE_RESULT_BYTES || !result ||
	    (encoded[1] && encoded[1] != SHOP_TRADE_RESULT_VERSION) || encoded[4] || encoded[5] ||
	    encoded[6] || encoded[7])
		return false;
	*result = {};
	result->action = static_cast<shop_trade_action>(encoded[0]);
	result->item_count = get_u16(encoded + 2);
	if (result->action <= shop_trade_action::unknown ||
	    result->action > shop_trade_action::sell_destroy ||
	    (!encoded[1] && (result->action == shop_trade_action::buy_produced ||
			     result->action == shop_trade_action::sell_destroy)) ||
	    result->item_count > result->item_uids.size())
		return false;
	for (size_t index = 0; index < CURRENCY_DENOMINATION_COUNT; ++index)
	{
		result->wallet.amount[index] =
			static_cast<int64_t>(get_u64(encoded + 8 + index * 8));
		result->bank.amount[index] =
			static_cast<int64_t>(get_u64(encoded + 40 + index * 8));
	}
	result->wallet_revision = get_u64(encoded + 72);
	result->bank_revision = get_u64(encoded + 80);
	result->shop_revision = get_u64(encoded + 88);
	result->player_owner_revision = get_u64(encoded + 96);
	result->counterparty_owner_revision = get_u64(encoded + 104);
	for (size_t index = 0; index < result->item_count; ++index)
	{
		result->item_uids[index] = get_u64(encoded + 112 + index * 16);
		result->item_revisions[index] = get_u64(encoded + 120 + index * 16);
		if (!result->item_uids[index])
			return false;
	}
	for (size_t offset = 112 + result->item_count * 16; offset < size; ++offset)
		if (encoded[offset])
			return false;
	return true;
}

bool shop_trade_command_build(critical_command *command, critical_operation_id operation_id,
			      const shop_trade_payload &payload, critical_source_site source_site,
			      critical_deadline_class deadline_class)
{
	if (!command || critical_operation_id_is_zero(operation_id) || !valid_payload(payload))
		return false;
	critical_entity_key account = {};
	if (!currency_account_key(payload.account_name.data(), payload.racewar, &account))
		return false;
	std::vector<uint8_t> encoded;
	if (!shop_trade_command_encode_payload(payload, &encoded))
		return false;
	const critical_entity_key player = { critical_entity_type::player, payload.player_pid };
	const critical_entity_key shop = { critical_entity_type::shopkeeper,
					   item_shopkeeper_owner_id(payload.shop_id) };
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::shop_trade,
		     .payload_version = SHOP_TRADE_PAYLOAD_VERSION,
		     .source_site = source_site,
		     .deadline_class = deadline_class,
		     .accepted_at_usec = 0,
		     .keys = { player, account, shop },
		     .expected_revisions = { { player, payload.expected_wallet_revision },
					     { account, payload.expected_bank_revision },
					     { shop, payload.expected_shop_revision } },
		     .payload = std::move(encoded) };
	try
	{
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const critical_entity_key item = { critical_entity_type::item,
							   payload.items[index].item_uid };
			command->keys.push_back(item);
			command->expected_revisions.push_back(
				{ item, payload.items[index].expected_item_revision });
		}
		if (creation(payload))
		{
			const critical_entity_key stock = { critical_entity_type::item,
							    payload.stock_item_uid };
			command->keys.push_back(stock);
			command->expected_revisions.push_back(
				{ stock, payload.expected_stock_item_revision });
		}
		if (payload.target_parent_item_uid)
		{
			const critical_entity_key parent = { critical_entity_type::item,
							     payload.target_parent_item_uid };
			command->keys.push_back(parent);
			command->expected_revisions.push_back(
				{ parent, payload.expected_target_parent_revision });
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	if (std::adjacent_find(command->keys.begin(), command->keys.end(),
			       critical_entity_key_equal) != command->keys.end())
		return false;
	std::sort(command->expected_revisions.begin(), command->expected_revisions.end(),
		  [](const auto &left, const auto &right)
		  { return critical_entity_key_less(left.key, right.key); });
	return true;
}
