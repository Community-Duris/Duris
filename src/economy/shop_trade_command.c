#include "economy/shop_trade_command.h"

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

bool cleanup(const shop_trade_payload &payload)
{
	return payload.action == shop_trade_action::discard_invalid;
}

bool purchase(const shop_trade_payload &payload)
{
	return payload.action == shop_trade_action::buy_existing ||
	       payload.action == shop_trade_action::buy_produced;
}

bool valid_payload(const shop_trade_payload &payload)
{
	if (payload.action <= shop_trade_action::unknown ||
	    payload.action > shop_trade_action::discard_invalid || !payload.player_pid ||
	    !valid_name(payload.account_name) || payload.price < 0 || payload.price > INT_MAX ||
	    payload.keeper_vnum < 0 || payload.expected_keeper_cash < 0 ||
	    payload.expected_keeper_cash > INT_MAX || payload.keeper_roaming > 1 ||
	    (!payload.keeper_vnum && payload.expected_keeper_cash) ||
	    (cleanup(payload) ? payload.price != 0 : (!purchase(payload) && payload.price == 0)) ||
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
	else if (payload.action == shop_trade_action::buy_existing || cleanup(payload))
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
	    ((creates || payload.action == shop_trade_action::buy_existing || cleanup(payload)) &&
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
	if (!encoded || payload.recovery_manifest_recorded ||
	    !shop_trade_recovery_manifest_is_empty(payload.recovery_manifest) ||
	    !valid_payload(payload) || payload.expected_player_save_revision ||
	    payload.expected_player_level || payload.native_destination_weight_recorded ||
	    payload.destination_weight != shop_trade_destination_weight{})
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
		append_le<int32_t>(encoded, payload.keeper_vnum);
		append_le<int64_t>(encoded, payload.expected_keeper_cash);
		append_le<uint8_t>(encoded, payload.keeper_roaming);
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

bool shop_trade_command_encode_accounted_payload(const shop_trade_payload &payload,
						 std::vector<uint8_t> *encoded)
{
	if (!encoded || payload.recovery_manifest_recorded ||
	    !shop_trade_recovery_manifest_is_empty(payload.recovery_manifest) ||
	    !payload.expected_player_save_revision || !payload.expected_player_level ||
	    payload.expected_player_level > UINT8_MAX ||
	    payload.native_destination_weight_recorded ||
	    payload.destination_weight != shop_trade_destination_weight{})
		return false;
	try
	{
		auto legacy = payload;
		legacy.expected_player_save_revision = 0;
		legacy.expected_player_level = 0;
		std::vector<uint8_t> candidate;
		if (!shop_trade_command_encode_payload(legacy, &candidate) ||
		    candidate.size() >
			    CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - SHOP_TRADE_ACCOUNTED_TAIL_BYTES)
			return false;
		candidate.reserve(candidate.size() + SHOP_TRADE_ACCOUNTED_TAIL_BYTES);
		append_le<uint64_t>(&candidate, payload.expected_player_save_revision);
		append_le<uint32_t>(&candidate, payload.expected_player_level);
		append_le<uint32_t>(&candidate, 0);
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool shop_trade_command_encode_native_payload(const shop_trade_payload &payload,
					      std::vector<uint8_t> *encoded)
{
	if (!encoded || payload.recovery_manifest_recorded ||
	    !shop_trade_recovery_manifest_is_empty(payload.recovery_manifest) ||
	    !payload.native_destination_weight_recorded ||
	    (payload.target_parent_item_uid ?
		     payload.action != shop_trade_action::buy_produced ||
			     payload.target_root_item_uid != payload.target_parent_item_uid :
		     payload.destination_weight != shop_trade_destination_weight{}))
		return false;
	try
	{
		auto previous = payload;
		previous.native_destination_weight_recorded = false;
		previous.destination_weight = {};
		std::vector<uint8_t> candidate;
		if (!shop_trade_command_encode_accounted_payload(previous, &candidate) ||
		    candidate.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - 16)
			return false;
		candidate.reserve(candidate.size() + 16);
		append_le<int32_t>(&candidate, payload.destination_weight.before);
		append_le<int32_t>(&candidate, payload.destination_weight.direct_contents);
		append_le<int32_t>(&candidate, payload.destination_weight.shell);
		append_le<int32_t>(&candidate, payload.destination_weight.after);
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool shop_trade_command_encode_recovery_payload(const shop_trade_payload &payload,
						std::vector<uint8_t> *encoded)
{
	if (!encoded || !payload.recovery_manifest_recorded ||
	    !shop_trade_recovery_manifest_shape_valid(payload.recovery_manifest) ||
	    payload.recovery_manifest.live_target_before.present !=
		    (payload.target_parent_item_uid != 0))
		return false;
	try
	{
		auto previous = payload;
		previous.recovery_manifest_recorded = false;
		previous.recovery_manifest = {};
		std::vector<uint8_t> candidate, extension;
		if (!shop_trade_command_encode_native_payload(previous, &candidate) ||
		    !shop_trade_recovery_manifest_encode(payload.recovery_manifest, &extension) ||
		    extension.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
		    candidate.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - extension.size())
			return false;
		candidate.insert(candidate.end(), extension.begin(), extension.end());
		*encoded = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

static bool decode_payload_impl(const critical_command &command, shop_trade_payload *payload)
{
	if (!payload || command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
	    command.type != critical_command_type::shop_trade ||
	    (!shop_trade_payload_version_is_accounted(command.payload_version) &&
	     command.payload_version != SHOP_TRADE_PAYLOAD_VERSION &&
	     command.payload_version != SHOP_TRADE_PREVIOUS_PAYLOAD_VERSION &&
	     command.payload_version != SHOP_TRADE_CONTAINER_PAYLOAD_VERSION &&
	     command.payload_version != SHOP_TRADE_STOCK_PAYLOAD_VERSION &&
	     command.payload_version != SHOP_TRADE_LEGACY_PAYLOAD_VERSION))
		return false;
	*payload = {};
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	uint8_t action = 0, name_length = 0;
	if (!read_le(&cursor, end, &action) || !read_le(&cursor, end, &payload->player_pid) ||
	    !read_le(&cursor, end, &payload->shop_id) ||
	    !read_le(&cursor, end, &payload->racewar) || !read_le(&cursor, end, &payload->price))
		return false;
	if (command.payload_version >= SHOP_TRADE_PAYLOAD_VERSION &&
	    (!read_le(&cursor, end, &payload->keeper_vnum) ||
	     !read_le(&cursor, end, &payload->expected_keeper_cash) ||
	     !read_le(&cursor, end, &payload->keeper_roaming)))
		return false;
	if (!read_le(&cursor, end, &payload->expected_wallet_revision) ||
	    !read_le(&cursor, end, &payload->expected_bank_revision) ||
	    !read_le(&cursor, end, &payload->expected_shop_revision) ||
	    !read_le(&cursor, end, &payload->selected_item_uid))
		return false;
	if (command.payload_version >= SHOP_TRADE_CONTAINER_PAYLOAD_VERSION)
	{
		if (!read_le(&cursor, end, &payload->target_root_item_uid) ||
		    !read_le(&cursor, end, &payload->target_parent_item_uid) ||
		    !read_le(&cursor, end, &payload->expected_target_parent_revision))
			return false;
	}
	else
		payload->target_root_item_uid = payload->selected_item_uid;
	if (command.payload_version >= SHOP_TRADE_STOCK_PAYLOAD_VERSION &&
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
	    (command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ?
		     static_cast<size_t>(end - cursor) <
			     payload->item_blob_size + SHOP_TRADE_NATIVE_TAIL_BYTES +
				     SHOP_TRADE_RECOVERY_MANIFEST_MIN_BYTES :
		     static_cast<size_t>(end - cursor) !=
			     payload->item_blob_size +
				     (command.payload_version == SHOP_TRADE_NATIVE_PAYLOAD_VERSION ?
					      SHOP_TRADE_NATIVE_TAIL_BYTES :
				      command.payload_version ==
						      SHOP_TRADE_ACCOUNTED_PAYLOAD_VERSION ?
					      SHOP_TRADE_ACCOUNTED_TAIL_BYTES :
					      0)))
		return false;
	memcpy(payload->item_blob.data(), cursor, payload->item_blob_size);
	if (shop_trade_payload_version_is_accounted(command.payload_version))
	{
		cursor += payload->item_blob_size;
		uint32_t padding = 0;
		if (!read_le(&cursor, end, &payload->expected_player_save_revision) ||
		    !read_le(&cursor, end, &payload->expected_player_level) ||
		    !read_le(&cursor, end, &padding) || padding ||
		    !payload->expected_player_save_revision || !payload->expected_player_level ||
		    payload->expected_player_level > UINT8_MAX)
			return false;
	}
	if (command.payload_version == SHOP_TRADE_NATIVE_PAYLOAD_VERSION ||
	    command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION)
	{
		payload->native_destination_weight_recorded = true;
		if (!read_le(&cursor, end, &payload->destination_weight.before) ||
		    !read_le(&cursor, end, &payload->destination_weight.direct_contents) ||
		    !read_le(&cursor, end, &payload->destination_weight.shell) ||
		    !read_le(&cursor, end, &payload->destination_weight.after) ||
		    (payload->target_parent_item_uid ?
			     payload->action != shop_trade_action::buy_produced ||
				     payload->target_root_item_uid !=
					     payload->target_parent_item_uid :
			     payload->destination_weight != shop_trade_destination_weight{}))
			return false;
	}
	if (command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION)
	{
		payload->recovery_manifest_recorded = true;
		if (!shop_trade_recovery_manifest_decode(
			    std::span<const uint8_t>(cursor, static_cast<size_t>(end - cursor)),
			    &payload->recovery_manifest) ||
		    payload->recovery_manifest.live_target_before.present !=
			    (payload->target_parent_item_uid != 0))
			return false;
		cursor = end;
	}
	if (shop_trade_payload_version_is_accounted(command.payload_version) && cursor != end)
		return false;
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
	const bool built =
		command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ?
			shop_trade_command_build_recovery(&expected, command.operation_id, *payload,
							  command.source_site,
							  command.deadline_class) :
		command.payload_version == SHOP_TRADE_NATIVE_PAYLOAD_VERSION ?
			shop_trade_command_build_native(&expected, command.operation_id, *payload,
							command.source_site,
							command.deadline_class) :
		command.payload_version == SHOP_TRADE_ACCOUNTED_PAYLOAD_VERSION ?
			shop_trade_command_build_accounted(&expected, command.operation_id,
							   *payload, command.source_site,
							   command.deadline_class) :
			shop_trade_command_build(&expected, command.operation_id, *payload,
						 command.source_site, command.deadline_class);
	return built && matching_fences(expected, command) &&
	       (command.payload_version != SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ||
		expected.payload == command.payload);
}

bool shop_trade_command_decode_payload(const critical_command &command, shop_trade_payload *payload)
{
	if (!payload)
		return false;
	try
	{
		shop_trade_payload candidate{};
		if (!decode_payload_impl(command, &candidate))
			return false;
		*payload = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool shop_trade_command_encode_result(const shop_trade_result &result,
				      std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> *encoded)
{
	if (!encoded || result.action <= shop_trade_action::unknown ||
	    result.action > shop_trade_action::discard_invalid ||
	    result.item_count > result.item_uids.size())
		return false;
	for (size_t index = 0; index < result.item_count; ++index)
		if (!result.item_uids[index])
			return false;
	encoded->fill(0);
	(*encoded)[0] = static_cast<uint8_t>(result.action);
	(*encoded)[1] = result.keeper_cash_recorded ? SHOP_TRADE_RESULT_VERSION : 1;
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
	if (result.keeper_cash_recorded)
	{
		if (result.keeper_cash < 0 || result.keeper_cash > INT_MAX)
			return false;
		put_u64(encoded->data() + SHOP_TRADE_PREVIOUS_RESULT_BYTES,
			static_cast<uint64_t>(result.keeper_cash));
	}
	return true;
}

bool shop_trade_command_decode_result(const uint8_t *encoded, size_t size,
				      shop_trade_result *result)
{
	if (!encoded ||
	    (size != SHOP_TRADE_RESULT_BYTES && size != SHOP_TRADE_PREVIOUS_RESULT_BYTES) ||
	    !result || (encoded[1] > SHOP_TRADE_RESULT_VERSION) || encoded[4] || encoded[5] ||
	    encoded[6] || encoded[7])
		return false;
	*result = {};
	result->action = static_cast<shop_trade_action>(encoded[0]);
	result->item_count = get_u16(encoded + 2);
	if (result->action <= shop_trade_action::unknown ||
	    result->action > shop_trade_action::discard_invalid ||
	    (!encoded[1] && (result->action == shop_trade_action::buy_produced ||
			     result->action == shop_trade_action::sell_destroy ||
			     result->action == shop_trade_action::discard_invalid)) ||
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
	result->keeper_cash_recorded = encoded[1] == SHOP_TRADE_RESULT_VERSION;
	if (result->keeper_cash_recorded)
	{
		if (size != SHOP_TRADE_RESULT_BYTES)
			return false;
		result->keeper_cash =
			static_cast<int64_t>(get_u64(encoded + SHOP_TRADE_PREVIOUS_RESULT_BYTES));
		if (result->keeper_cash < 0 || result->keeper_cash > INT_MAX)
			return false;
	}
	result->player_owner_revision = get_u64(encoded + 96);
	result->counterparty_owner_revision = get_u64(encoded + 104);
	for (size_t index = 0; index < result->item_count; ++index)
	{
		result->item_uids[index] = get_u64(encoded + 112 + index * 16);
		result->item_revisions[index] = get_u64(encoded + 120 + index * 16);
		if (!result->item_uids[index])
			return false;
	}
	for (size_t offset = 112 + result->item_count * 16;
	     offset < SHOP_TRADE_PREVIOUS_RESULT_BYTES; ++offset)
		if (encoded[offset])
			return false;
	for (size_t offset = result->keeper_cash_recorded ? SHOP_TRADE_RESULT_BYTES :
							    SHOP_TRADE_PREVIOUS_RESULT_BYTES;
	     offset < size; ++offset)
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

bool shop_trade_command_build_accounted(critical_command *command,
					critical_operation_id operation_id,
					const shop_trade_payload &payload,
					critical_source_site source_site,
					critical_deadline_class deadline_class)
{
	if (!command)
		return false;
	try
	{
		auto legacy = payload;
		legacy.expected_player_save_revision = 0;
		legacy.expected_player_level = 0;
		critical_command candidate;
		std::vector<uint8_t> encoded;
		if (!shop_trade_command_build(&candidate, operation_id, legacy, source_site,
					      deadline_class) ||
		    !shop_trade_command_encode_accounted_payload(payload, &encoded))
			return false;
		candidate.payload_version = SHOP_TRADE_ACCOUNTED_PAYLOAD_VERSION;
		candidate.payload = std::move(encoded);
		*command = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool shop_trade_command_build_native(critical_command *command, critical_operation_id operation_id,
				     const shop_trade_payload &payload,
				     critical_source_site source_site,
				     critical_deadline_class deadline_class)
{
	if (!command)
		return false;
	try
	{
		auto previous = payload;
		previous.native_destination_weight_recorded = false;
		previous.destination_weight = {};
		critical_command candidate;
		std::vector<uint8_t> encoded;
		if (!shop_trade_command_build_accounted(&candidate, operation_id, previous,
							source_site, deadline_class) ||
		    !shop_trade_command_encode_native_payload(payload, &encoded))
			return false;
		candidate.payload_version = SHOP_TRADE_NATIVE_PAYLOAD_VERSION;
		candidate.payload = std::move(encoded);
		*command = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool shop_trade_command_build_recovery(critical_command *command,
				       critical_operation_id operation_id,
				       const shop_trade_payload &payload,
				       critical_source_site source_site,
				       critical_deadline_class deadline_class)
{
	if (!command)
		return false;
	try
	{
		auto previous = payload;
		previous.recovery_manifest_recorded = false;
		previous.recovery_manifest = {};
		critical_command candidate;
		std::vector<uint8_t> encoded;
		if (!shop_trade_command_build_native(&candidate, operation_id, previous,
						     source_site, deadline_class) ||
		    !shop_trade_command_encode_recovery_payload(payload, &encoded))
			return false;
		candidate.payload_version = SHOP_TRADE_RECOVERY_PAYLOAD_VERSION;
		candidate.payload = std::move(encoded);
		*command = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

#include <limits>
#include <iterator>
#include <initializer_list>

namespace
{
using shop_codec_reserve_fn = bool (*)(size_t, void *) noexcept;
bool shop_codec_profile() noexcept
{
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG)
	return true;
#else
	return false;
#endif
}
bool shop_codec_add(size_t &sum, size_t amount) noexcept
{
	if (amount > std::numeric_limits<size_t>::max() - sum)
		return false;
	sum += amount;
	return true;
}
template <typename T> bool shop_codec_vector_heap(const std::vector<T> &value, size_t &sum) noexcept
{
	return value.capacity() <= std::numeric_limits<size_t>::max() / sizeof(T) &&
	       shop_codec_add(sum, value.capacity() * sizeof(T));
}
bool shop_codec_payload_heap(const shop_trade_payload &value, size_t &sum) noexcept
{
	return shop_codec_vector_heap(value.recovery_manifest.player_before.ordered_item_uids,
				      sum) &&
	       shop_codec_vector_heap(value.recovery_manifest.player_after.ordered_item_uids,
				      sum) &&
	       shop_codec_vector_heap(value.recovery_manifest.keeper_before.ordered_item_uids,
				      sum) &&
	       shop_codec_vector_heap(value.recovery_manifest.keeper_after.ordered_item_uids,
				      sum) &&
	       shop_codec_vector_heap(value.recovery_manifest.live_target_before.ordered_item_uids,
				      sum) &&
	       shop_codec_vector_heap(value.recovery_manifest.live_target_after.ordered_item_uids,
				      sum);
}
bool shop_codec_command_heap(const critical_command &value, size_t &sum) noexcept
{
	return shop_codec_vector_heap(value.keys, sum) &&
	       shop_codec_vector_heap(value.expected_revisions, sum) &&
	       shop_codec_vector_heap(value.payload, sum) &&
	       shop_codec_vector_heap(value.accounting_intent, sum);
}
struct shop_codec_live_owner;
// Source-declared carriers for the real census/add/admit/vector-size/capacity
// closure. These are genuine parameters/results/locals, not an emitted-stack
// claim or a logical encoded-byte proxy.
constexpr size_t shop_codec_census_frames =
	3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) +
	2 * (2 * sizeof(void *) + sizeof(bool)) + 2 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
struct shop_codec_budget
{
	shop_codec_reserve_fn reserve = nullptr;
	void *context = nullptr;
	size_t outer = 0;
	shop_codec_live_owner *head = nullptr;
	bool current(size_t *) noexcept;
	bool peak(size_t) noexcept;
};
struct shop_codec_live_owner
{
	shop_codec_budget *budget = nullptr;
	shop_codec_live_owner *previous = nullptr;
	size_t inline_bytes = 0;
	const shop_trade_payload *payload = nullptr;
	const critical_command *command = nullptr;
	const std::vector<uint8_t> *first = nullptr;
	const std::vector<uint8_t> *second = nullptr;
	shop_codec_live_owner(shop_codec_budget &b, size_t bytes, const shop_trade_payload *p,
			      const critical_command *c, const std::vector<uint8_t> *one,
			      const std::vector<uint8_t> *two) noexcept
		: budget(&b)
		, previous(b.head)
		, inline_bytes(bytes)
		, payload(p)
		, command(c)
		, first(one)
		, second(two)
	{
		b.head = this;
	}
	~shop_codec_live_owner() noexcept { budget->head = previous; }
	shop_codec_live_owner(const shop_codec_live_owner &) = delete;
	shop_codec_live_owner &operator=(const shop_codec_live_owner &) = delete;
};
bool shop_codec_budget::current(size_t *output) noexcept
{
	if (!output)
		return false;
	size_t total = outer;
	for (const auto *owner = head; owner; owner = owner->previous)
		if (!shop_codec_add(total, owner->inline_bytes) ||
		    (owner->payload && !shop_codec_payload_heap(*owner->payload, total)) ||
		    (owner->command && !shop_codec_command_heap(*owner->command, total)) ||
		    (owner->first && !shop_codec_vector_heap(*owner->first, total)) ||
		    (owner->second && !shop_codec_vector_heap(*owner->second, total)))
			return false;
	*output = total;
	return true;
}
bool shop_codec_budget::peak(size_t request) noexcept
{
	size_t live = 0;
	return reserve && current(&live) && shop_codec_add(live, request) &&
	       shop_codec_add(live, shop_codec_census_frames) && reserve(live, context);
}

// The full original fixed-array gameplay validation and fence comparison are
// allocation-free. Their genuine source-declared local/parameter/return scopes
// and instantiated find/equal adapters coexist with the caller workspace.
constexpr size_t shop_codec_find_frames(size_t closure) noexcept
{
	return
		// find_if/__find_if/__find_if<random_access>: each first,last,pred,
		// returned iterator; real trip-count/tag in the random-access scope.
		3 * (2 * sizeof(void *) + closure + sizeof(void *)) + sizeof(std::ptrdiff_t) +
		sizeof(char) +
		// pred_iter's passed/returned functor and Iter_pred constructor,
		// move's argument/result; actual adapter/lambda operator parameters.
		3 * closure + sizeof(void *) + 2 * sizeof(void *) + 4 * sizeof(void *) +
		2 * sizeof(bool) +
		// iterator_category reference parameter/returned empty tag.
		sizeof(void *) + sizeof(char);
}
constexpr size_t shop_codec_equal_frames =
	// 3-iterator equal/aux/aux1/simple-equal source scopes, simple flag/len,
	// niter-base parameter/return carriers and __memcmp args/result.
	4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) + sizeof(size_t) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int);
constexpr size_t shop_codec_pure_frames =
	// Original valid_payload declared ref/return, creates/selected/reaches,
	// target-root/root/parent UID, two indexes/depth, item/root/parent refs.
	5 * sizeof(void *) + 3 * sizeof(size_t) + 3 * sizeof(uint64_t) + 4 * sizeof(bool) +
	// valid_name ref/return/length/index; actual strnlen args/result;
	// creation/cleanup/purchase each original reference and boolean result.
	sizeof(void *) + sizeof(bool) + 2 * sizeof(size_t) + sizeof(void *) + 2 * sizeof(size_t) +
	3 * (sizeof(void *) + sizeof(bool)) +
	// Actual captured-by-reference item predicates; include both instantiations
	// and the complete genuine find source profile above, not item-count caps.
	2 * (2 * sizeof(void *) + sizeof(bool)) + shop_codec_find_frames(sizeof(void *)) +
	// Original manifest_is_empty's returned+destination binding arrays and
	// all_of/find_if_not/negated-predicate scope around its captureless functor.
	2 * sizeof(std::array<const shop_trade_recovery_forest_binding *,
			      SHOP_TRADE_RECOVERY_FOREST_COUNT>) +
	3 * (2 * sizeof(void *) + sizeof(char) + sizeof(void *)) +
	shop_codec_find_frames(sizeof(char)) +
	// binding_empty's actual zero-digest temporary/reference/result and full
	// array equality; weight comparison has two refs+fixed real temporary.
	sizeof(std::array<uint8_t, 32>) + 3 * sizeof(void *) + 2 * sizeof(bool) +
	shop_codec_equal_frames + sizeof(shop_trade_destination_weight) + 2 * sizeof(void *) +
	sizeof(bool) +
	// matching_fences left/right references/result; custom equal's three
	// iterator arguments, actual predicate carriers and full original key and
	// revision predicate invocation scopes. Canonical vector == uses the
	// separate complete three-iterator equality closure.
	2 * sizeof(void *) + sizeof(bool) +
	(3 * sizeof(void *) + sizeof(&critical_entity_key_equal) + sizeof(bool)) +
	2 * sizeof(void *) + sizeof(bool) + (3 * sizeof(void *) + sizeof(char) + sizeof(bool)) +
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
	sizeof(bool) + shop_codec_equal_frames;
// Complete installed GCC13 ordinary allocator/vector declared-carrier closure.
// All terms are genuine source scopes named below; conservative sums include
// mutually exclusive branches, never a guessed cap or emitted-stack claim.
constexpr size_t shop_codec_allocator_frames =
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
constexpr size_t shop_codec_copy_frames =
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
constexpr size_t shop_codec_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t shop_codec_default_frames =
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
constexpr size_t shop_codec_vector_frames =
	shop_codec_allocator_frames + shop_codec_copy_frames + shop_codec_relocate_frames +
	shop_codec_default_frames +
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
constexpr size_t shop_codec_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + shop_codec_allocator_frames;

constexpr size_t shop_codec_leaf_frames =
	// append/read/copy/allocator leaf actual values and argument/result carriers.
	shop_codec_vector_frames + 8 * sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	2 * sizeof(bool);

struct shop_codec_encode_workspace
{
	std::vector<uint8_t> candidate;
	std::vector<uint8_t> extension;
	size_t index = 0;
	size_t name_length = 0;
	size_t nested = 0;
};
struct shop_codec_copy_encode_workspace
{
	shop_trade_payload previous{};
	shop_codec_encode_workspace buffers;
};
struct shop_codec_build_workspace
{
	critical_command candidate{};
	std::vector<uint8_t> encoded;
	critical_entity_key account{};
	critical_entity_key player{};
	critical_entity_key shop{};
	critical_entity_key item{};
	size_t index = 0;
	size_t nested = 0;
};
struct shop_codec_copy_build_workspace
{
	shop_trade_payload previous{};
	shop_codec_build_workspace values;
};
struct shop_codec_decode_workspace
{
	shop_trade_payload candidate{};
	critical_command expected{};
	size_t nested = 0;
	size_t transferred_heap = 0;
};
constexpr size_t shop_codec_encode_parameters =
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(size_t) + sizeof(void *);
constexpr size_t shop_codec_build_parameters =
	3 * sizeof(void *) + sizeof(critical_operation_id) + sizeof(critical_source_site) +
	sizeof(critical_deadline_class) + sizeof(bool) + sizeof(char);
constexpr size_t shop_codec_workspace_fixed = sizeof(shop_codec_live_owner);

template <typename T>
bool shop_codec_growth(std::vector<T> &value, size_t count, shop_codec_budget &budget) noexcept
{
	constexpr size_t frames = sizeof(shop_codec_live_owner) + 3 * sizeof(void *) +
				  3 * sizeof(size_t) + sizeof(bool);
	if (!budget.peak(frames))
		return false;
	shop_codec_live_owner frame_owner(budget, frames, nullptr, nullptr, nullptr, nullptr);
	if (count > value.max_size() - value.size())
		return false;
	if (count <= value.capacity() - value.size())
		return budget.peak(shop_codec_vector_frames + sizeof(critical_expected_revision));
	size_t next = value.size() > count ? value.size() : count;
	if (next > value.max_size() - value.size())
		next = value.max_size();
	else
		next += value.size();
	size_t request = shop_codec_vector_frames + sizeof(critical_expected_revision);
	return next <= std::numeric_limits<size_t>::max() / sizeof(T) &&
	       shop_codec_add(request, next * sizeof(T)) && budget.peak(request);
}
template <typename T>
bool shop_codec_reserve(std::vector<T> &value, size_t count, shop_codec_budget &budget)
{
	constexpr size_t frames = sizeof(shop_codec_live_owner) + 3 * sizeof(void *) +
				  2 * sizeof(size_t) + sizeof(bool);
	if (!budget.peak(frames))
		return false;
	shop_codec_live_owner frame_owner(budget, frames, nullptr, nullptr, nullptr, nullptr);
	if (count > value.max_size())
		return false;
	if (count > value.capacity())
	{
		size_t request = shop_codec_vector_frames + sizeof(critical_expected_revision);
		if (count > std::numeric_limits<size_t>::max() / sizeof(T) ||
		    !shop_codec_add(request, count * sizeof(T)) || !budget.peak(request))
			return false;
		value.reserve(count);
	}
	return true;
}
template <typename T>
bool shop_codec_append(std::vector<uint8_t> &output, T value, shop_codec_budget &budget)
{
	constexpr size_t frames = sizeof(shop_codec_live_owner) + 3 * sizeof(void *) +
				  2 * sizeof(T) + sizeof(size_t) + sizeof(bool);
	if (!budget.peak(frames))
		return false;
	shop_codec_live_owner frame_owner(budget, frames, nullptr, nullptr, nullptr, nullptr);
	const auto encoded = static_cast<std::make_unsigned_t<T>>(value);
	for (size_t byte = 0; byte < sizeof(T); ++byte)
	{
		if (!shop_codec_growth(output, 1, budget))
			return false;
		output.push_back(static_cast<uint8_t>(encoded >> (byte * 8)));
	}
	return true;
}
template <typename Iterator> bool shop_codec_insert(std::vector<uint8_t> &output, Iterator first,
						    Iterator last, shop_codec_budget &budget)
{
	constexpr size_t frames = sizeof(shop_codec_live_owner) + 2 * sizeof(void *) +
				  2 * sizeof(Iterator) + sizeof(size_t) + sizeof(bool);
	if (!budget.peak(frames))
		return false;
	shop_codec_live_owner frame_owner(budget, frames, nullptr, nullptr, nullptr, nullptr);
	const size_t count = static_cast<size_t>(last - first);
	if (!shop_codec_growth(output, count, budget))
		return false;
	output.insert(output.end(), first, last);
	return true;
}

// Fresh destination only. Scalar/array fields preserve all original copy bytes;
// six real UID requests occur individually before allocation, retaining every
// previously copied UID capacity. No reserve-around-unbounded payload copy.
bool shop_codec_clone_payload(const shop_trade_payload &source, shop_trade_payload &target,
			      shop_codec_budget &budget)
{
	const size_t clone_frames = sizeof(shop_codec_live_owner) + 4 * sizeof(void *) +
				    3 * sizeof(size_t) + sizeof(bool) +
				    sizeof(std::array<shop_trade_recovery_forest_binding *,
						      SHOP_TRADE_RECOVERY_FOREST_COUNT>) +
				    sizeof(std::array<const shop_trade_recovery_forest_binding *,
						      SHOP_TRADE_RECOVERY_FOREST_COUNT>);
	if (!budget.peak(clone_frames))
		return false;
	shop_codec_live_owner frame_owner(budget, clone_frames, nullptr, nullptr, nullptr, nullptr);
	target.action = source.action;
	target.player_pid = source.player_pid;
	target.shop_id = source.shop_id;
	target.racewar = source.racewar;
	target.account_name = source.account_name;
	target.price = source.price;
	target.keeper_vnum = source.keeper_vnum;
	target.expected_keeper_cash = source.expected_keeper_cash;
	target.keeper_roaming = source.keeper_roaming;
	target.expected_wallet_revision = source.expected_wallet_revision;
	target.expected_bank_revision = source.expected_bank_revision;
	target.expected_shop_revision = source.expected_shop_revision;
	target.selected_item_uid = source.selected_item_uid;
	target.target_root_item_uid = source.target_root_item_uid;
	target.target_parent_item_uid = source.target_parent_item_uid;
	target.expected_target_parent_revision = source.expected_target_parent_revision;
	target.stock_item_uid = source.stock_item_uid;
	target.expected_stock_item_revision = source.expected_stock_item_revision;
	target.stock_vnum = source.stock_vnum;
	target.item_count = source.item_count;
	target.items = source.items;
	target.item_blob_size = source.item_blob_size;
	target.item_blob = source.item_blob;
	target.expected_player_save_revision = source.expected_player_save_revision;
	target.expected_player_level = source.expected_player_level;
	target.native_destination_weight_recorded = source.native_destination_weight_recorded;
	target.destination_weight = source.destination_weight;
	target.recovery_manifest_recorded = source.recovery_manifest_recorded;
	const std::array<const shop_trade_recovery_forest_binding *, 6> sources{
		&source.recovery_manifest.player_before,
		&source.recovery_manifest.player_after,
		&source.recovery_manifest.keeper_before,
		&source.recovery_manifest.keeper_after,
		&source.recovery_manifest.live_target_before,
		&source.recovery_manifest.live_target_after
	};
	const std::array<shop_trade_recovery_forest_binding *, 6> targets{
		&target.recovery_manifest.player_before,
		&target.recovery_manifest.player_after,
		&target.recovery_manifest.keeper_before,
		&target.recovery_manifest.keeper_after,
		&target.recovery_manifest.live_target_before,
		&target.recovery_manifest.live_target_after
	};
	for (size_t index = 0; index < sources.size(); ++index)
	{
		targets[index]->present = sources[index]->present;
		targets[index]->canonical_bytes = sources[index]->canonical_bytes;
		targets[index]->canonical_digest = sources[index]->canonical_digest;
		const size_t count = sources[index]->ordered_item_uids.size();
		if (count > std::numeric_limits<size_t>::max() / sizeof(uint64_t) ||
		    count * sizeof(uint64_t) >
			    std::numeric_limits<size_t>::max() - shop_codec_vector_frames ||
		    !budget.peak(shop_codec_vector_frames + count * sizeof(uint64_t)))
			return false;
		targets[index]->ordered_item_uids.assign(sources[index]->ordered_item_uids.begin(),
							 sources[index]->ordered_item_uids.end());
	}
	return true;
}
bool shop_codec_drop_manifest(shop_trade_payload &previous, shop_codec_budget &budget) noexcept
{
	// The real original default manifest assignment creates this temporary,
	// then releases the previously cloned blocks. Admission precedes the cut.
	if (!budget.peak(sizeof(shop_trade_recovery_manifest) + 2 * sizeof(void *) + sizeof(bool) +
			 shop_codec_move_frames))
		return false;
	previous.recovery_manifest = {};
	return true;
}

bool shop_bounded_encode_base(const shop_trade_payload &, std::vector<uint8_t> *,
			      shop_codec_budget &);
bool shop_bounded_encode_accounted(const shop_trade_payload &, std::vector<uint8_t> *,
				   shop_codec_budget &);
bool shop_bounded_encode_native(const shop_trade_payload &, std::vector<uint8_t> *,
				shop_codec_budget &);
bool shop_bounded_encode_recovery(const shop_trade_payload &, std::vector<uint8_t> *,
				  shop_codec_budget &);
bool shop_bounded_build_base(critical_command *, critical_operation_id, const shop_trade_payload &,
			     critical_source_site, critical_deadline_class, shop_codec_budget &);
bool shop_bounded_build_accounted(critical_command *, critical_operation_id,
				  const shop_trade_payload &, critical_source_site,
				  critical_deadline_class, shop_codec_budget &);
bool shop_bounded_build_native(critical_command *, critical_operation_id,
			       const shop_trade_payload &, critical_source_site,
			       critical_deadline_class, shop_codec_budget &);
bool shop_bounded_build_recovery(critical_command *, critical_operation_id,
				 const shop_trade_payload &, critical_source_site,
				 critical_deadline_class, shop_codec_budget &);

bool shop_bounded_encode_base(const shop_trade_payload &payload, std::vector<uint8_t> *out,
			      shop_codec_budget &budget)
{
	if (!out)
		return false;
	constexpr size_t own = sizeof(shop_codec_encode_workspace) + shop_codec_workspace_fixed +
			       shop_codec_encode_parameters;
	if (!budget.peak(own))
		return false;
	shop_codec_encode_workspace work;
	shop_codec_live_owner owner(budget, own, nullptr, nullptr, &work.candidate,
				    &work.extension);
	if (!budget.peak(shop_codec_pure_frames))
		return false;
	auto *encoded = &work.candidate;

	if (!encoded || payload.recovery_manifest_recorded ||
	    !shop_trade_recovery_manifest_is_empty(payload.recovery_manifest) ||
	    !valid_payload(payload) || payload.expected_player_save_revision ||
	    payload.expected_player_level || payload.native_destination_weight_recorded ||
	    payload.destination_weight != shop_trade_destination_weight{})
		return false;
	try
	{
		encoded->clear();
		if (!shop_codec_reserve(*encoded,
					128 + payload.item_count * 40 + payload.item_blob_size,
					budget))
			return false;
		if (!shop_codec_append<uint8_t>(*encoded, static_cast<uint8_t>(payload.action),
						budget))
			return false;
		if (!shop_codec_append<uint32_t>(*encoded, payload.player_pid, budget))
			return false;
		if (!shop_codec_append<uint32_t>(*encoded, payload.shop_id, budget))
			return false;
		if (!shop_codec_append<uint8_t>(*encoded, payload.racewar, budget))
			return false;
		if (!shop_codec_append<int64_t>(*encoded, payload.price, budget))
			return false;
		if (!shop_codec_append<int32_t>(*encoded, payload.keeper_vnum, budget))
			return false;
		if (!shop_codec_append<int64_t>(*encoded, payload.expected_keeper_cash, budget))
			return false;
		if (!shop_codec_append<uint8_t>(*encoded, payload.keeper_roaming, budget))
			return false;
		if (!shop_codec_append<uint64_t>(*encoded, payload.expected_wallet_revision,
						 budget))
			return false;
		if (!shop_codec_append<uint64_t>(*encoded, payload.expected_bank_revision, budget))
			return false;
		if (!shop_codec_append<uint64_t>(*encoded, payload.expected_shop_revision, budget))
			return false;
		if (!shop_codec_append<uint64_t>(*encoded, payload.selected_item_uid, budget))
			return false;
		if (!shop_codec_append<uint64_t>(*encoded, payload.target_root_item_uid, budget))
			return false;
		if (!shop_codec_append<uint64_t>(*encoded, payload.target_parent_item_uid, budget))
			return false;
		if (!shop_codec_append<uint64_t>(*encoded, payload.expected_target_parent_revision,
						 budget))
			return false;
		if (!shop_codec_append<uint64_t>(*encoded, payload.stock_item_uid, budget))
			return false;
		if (!shop_codec_append<uint64_t>(*encoded, payload.expected_stock_item_revision,
						 budget))
			return false;
		if (!shop_codec_append<int32_t>(*encoded, payload.stock_vnum, budget))
			return false;
		if (!shop_codec_append<uint16_t>(*encoded, payload.item_count, budget))
			return false;
		const size_t name_length =
			strnlen(payload.account_name.data(), payload.account_name.size());
		if (!shop_codec_append<uint8_t>(*encoded, static_cast<uint8_t>(name_length),
						budget))
			return false;
		if (!shop_codec_insert(*encoded, payload.account_name.begin(),
				       payload.account_name.begin() + name_length, budget))
			return false;
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto &item = payload.items[index];
			if (!shop_codec_append<uint64_t>(*encoded, item.item_uid, budget))
				return false;
			if (!shop_codec_append<uint64_t>(*encoded, item.root_item_uid, budget))
				return false;
			if (!shop_codec_append<uint64_t>(*encoded, item.parent_item_uid, budget))
				return false;
			if (!shop_codec_append<uint64_t>(*encoded, item.expected_item_revision,
							 budget))
				return false;
			if (!shop_codec_append<int32_t>(*encoded, item.vnum, budget))
				return false;
			if (!shop_codec_append<uint8_t>(
				    *encoded, static_cast<uint8_t>(item.expected_state), budget))
				return false;
		}
		if (!shop_codec_append<uint32_t>(*encoded, payload.item_blob_size, budget))
			return false;
		if (!shop_codec_insert(*encoded, payload.item_blob.begin(),
				       payload.item_blob.begin() + payload.item_blob_size, budget))
			return false;
	}
	catch (const std::bad_alloc &)
	{
		encoded->clear();
		return false;
	}
	if (encoded->size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	if (!budget.peak(shop_codec_move_frames))
		return false;
	*out = std::move(work.candidate);
	return true;
}

bool shop_bounded_encode_accounted(const shop_trade_payload &payload, std::vector<uint8_t> *out,
				   shop_codec_budget &budget)
{
	if (!out)
		return false;
	constexpr size_t own = sizeof(shop_codec_copy_encode_workspace) +
			       shop_codec_workspace_fixed + shop_codec_encode_parameters;
	if (!budget.peak(own))
		return false;
	shop_codec_copy_encode_workspace work;
	shop_codec_live_owner owner(budget, own, &work.previous, nullptr, &work.buffers.candidate,
				    &work.buffers.extension);
	if (!budget.peak(shop_codec_pure_frames))
		return false;
	if (payload.recovery_manifest_recorded ||
	    !shop_trade_recovery_manifest_is_empty(payload.recovery_manifest) ||
	    !payload.expected_player_save_revision || !payload.expected_player_level ||
	    payload.expected_player_level > UINT8_MAX ||
	    payload.native_destination_weight_recorded ||
	    payload.destination_weight != shop_trade_destination_weight{})
		return false;
	if (!shop_codec_clone_payload(payload, work.previous, budget))
		return false;
	work.previous.expected_player_save_revision = 0;
	work.previous.expected_player_level = 0;
	if (!shop_bounded_encode_base(work.previous, &work.buffers.candidate, budget) ||
	    work.buffers.candidate.size() >
		    CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - SHOP_TRADE_ACCOUNTED_TAIL_BYTES ||
	    !shop_codec_reserve(work.buffers.candidate,
				work.buffers.candidate.size() + SHOP_TRADE_ACCOUNTED_TAIL_BYTES,
				budget) ||
	    !shop_codec_append<uint64_t>(work.buffers.candidate,
					 payload.expected_player_save_revision, budget) ||
	    !shop_codec_append<uint32_t>(work.buffers.candidate, payload.expected_player_level,
					 budget) ||
	    !shop_codec_append<uint32_t>(work.buffers.candidate, 0, budget))
		return false;
	if (!budget.peak(shop_codec_move_frames))
		return false;
	*out = std::move(work.buffers.candidate);
	return true;
}
bool shop_bounded_encode_native(const shop_trade_payload &payload, std::vector<uint8_t> *out,
				shop_codec_budget &budget)
{
	if (!out)
		return false;
	constexpr size_t own = sizeof(shop_codec_copy_encode_workspace) +
			       shop_codec_workspace_fixed + shop_codec_encode_parameters;
	if (!budget.peak(own))
		return false;
	shop_codec_copy_encode_workspace work;
	shop_codec_live_owner owner(budget, own, &work.previous, nullptr, &work.buffers.candidate,
				    &work.buffers.extension);
	if (!budget.peak(shop_codec_pure_frames))
		return false;
	if (payload.recovery_manifest_recorded ||
	    !shop_trade_recovery_manifest_is_empty(payload.recovery_manifest) ||
	    !payload.native_destination_weight_recorded ||
	    (payload.target_parent_item_uid ?
		     payload.action != shop_trade_action::buy_produced ||
			     payload.target_root_item_uid != payload.target_parent_item_uid :
		     payload.destination_weight != shop_trade_destination_weight{}))
		return false;
	if (!shop_codec_clone_payload(payload, work.previous, budget))
		return false;
	work.previous.native_destination_weight_recorded = false;
	work.previous.destination_weight = {};
	if (!shop_bounded_encode_accounted(work.previous, &work.buffers.candidate, budget) ||
	    work.buffers.candidate.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - 16 ||
	    !shop_codec_reserve(work.buffers.candidate, work.buffers.candidate.size() + 16,
				budget) ||
	    !shop_codec_append<int32_t>(work.buffers.candidate, payload.destination_weight.before,
					budget) ||
	    !shop_codec_append<int32_t>(work.buffers.candidate,
					payload.destination_weight.direct_contents, budget) ||
	    !shop_codec_append<int32_t>(work.buffers.candidate, payload.destination_weight.shell,
					budget) ||
	    !shop_codec_append<int32_t>(work.buffers.candidate, payload.destination_weight.after,
					budget))
		return false;
	if (!budget.peak(shop_codec_move_frames))
		return false;
	*out = std::move(work.buffers.candidate);
	return true;
}
bool shop_bounded_encode_recovery(const shop_trade_payload &payload, std::vector<uint8_t> *out,
				  shop_codec_budget &budget)
{
	if (!out)
		return false;
	constexpr size_t own = sizeof(shop_codec_copy_encode_workspace) +
			       shop_codec_workspace_fixed + shop_codec_encode_parameters;
	if (!budget.peak(own))
		return false;
	shop_codec_copy_encode_workspace work;
	shop_codec_live_owner owner(budget, own, &work.previous, nullptr, &work.buffers.candidate,
				    &work.buffers.extension);
	if (!payload.recovery_manifest_recorded || !budget.current(&work.buffers.nested) ||
	    !shop_trade_recovery_manifest_shape_valid_bounded(payload.recovery_manifest,
							      budget.reserve, budget.context,
							      work.buffers.nested) ||
	    payload.recovery_manifest.live_target_before.present !=
		    (payload.target_parent_item_uid != 0))
		return false;
	if (!shop_codec_clone_payload(payload, work.previous, budget))
		return false;
	work.previous.recovery_manifest_recorded = false;
	if (!shop_codec_drop_manifest(work.previous, budget) ||
	    !shop_bounded_encode_native(work.previous, &work.buffers.candidate, budget) ||
	    !budget.current(&work.buffers.nested) ||
	    !shop_trade_recovery_manifest_encode_bounded(
		    payload.recovery_manifest, &work.buffers.extension, budget.reserve,
		    budget.context, work.buffers.nested, nullptr) ||
	    work.buffers.extension.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
	    work.buffers.candidate.size() >
		    CRITICAL_COMMAND_MAX_PAYLOAD_BYTES - work.buffers.extension.size() ||
	    !shop_codec_insert(work.buffers.candidate, work.buffers.extension.begin(),
			       work.buffers.extension.end(), budget))
		return false;
	if (!budget.peak(shop_codec_move_frames))
		return false;
	*out = std::move(work.buffers.candidate);
	return true;
}

bool shop_codec_sort_admit(size_t count, size_t element, size_t comparator,
			   shop_codec_budget &budget) noexcept
{
	size_t remaining = count, levels = 0;
	while (remaining > 1)
	{
		remaining >>= 1;
		++levels;
	}
	// Actual GCC13 introsort: first/last/cut + depth + comparator per
	// recursive scope, initial2*floor(log2(n))+1 exhaustion carrier.
	const size_t recursive = 3 * sizeof(void *) + sizeof(std::ptrdiff_t) + comparator;
	const size_t leaves =
		// sort/__sort, median/partition/iter_swap; final/unguarded insertion.
		18 * sizeof(void *) + 7 * comparator + element + 16 * sizeof(void *) +
		6 * comparator + 2 * element +
		// partial-sort/heap-select/make/adjust/push/pop/sort-heap.
		23 * sizeof(void *) + 11 * sizeof(std::ptrdiff_t) + 7 * comparator + 4 * element +
		// comparator adapter parameter/return scopes and scalar predicates.
		8 * sizeof(void *) + 5 * comparator + 4 * sizeof(bool) +
		// This real admission method's own args/locals/result carrier.
		2 * sizeof(void *) + 8 * sizeof(size_t) + sizeof(bool);
	if (levels > (std::numeric_limits<size_t>::max() - 1) / 2 ||
	    2 * levels + 1 > std::numeric_limits<size_t>::max() / recursive)
		return false;
	size_t request = (2 * levels + 1) * recursive;
	return shop_codec_add(request, leaves) && budget.peak(request);
}

bool shop_bounded_build_base(critical_command *out, critical_operation_id operation_id,
			     const shop_trade_payload &payload, critical_source_site source_site,
			     critical_deadline_class deadline_class, shop_codec_budget &budget)
{
	if (!out)
		return false;
	constexpr size_t own = sizeof(shop_codec_build_workspace) + shop_codec_workspace_fixed +
			       shop_codec_build_parameters;
	if (!budget.peak(own))
		return false;
	shop_codec_build_workspace work;
	shop_codec_live_owner owner(budget, own, nullptr, &work.candidate, &work.encoded, nullptr);
	if (!budget.peak(shop_codec_pure_frames))
		return false;
	if (critical_operation_id_is_zero(operation_id) || !valid_payload(payload) ||
	    !budget.current(&work.nested) ||
	    !currency_account_key_bounded(payload.account_name.data(), payload.racewar,
					  &work.account, budget.reserve, budget.context,
					  work.nested) ||
	    !shop_bounded_encode_base(payload, &work.encoded, budget))
		return false;
	work.player = { critical_entity_type::player, payload.player_pid };
	work.shop = { critical_entity_type::shopkeeper, item_shopkeeper_owner_id(payload.shop_id) };
	work.candidate.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	work.candidate.operation_id = operation_id;
	work.candidate.type = critical_command_type::shop_trade;
	work.candidate.payload_version = SHOP_TRADE_PAYLOAD_VERSION;
	work.candidate.source_site = source_site;
	work.candidate.deadline_class = deadline_class;
	work.candidate.accepted_at_usec = 0;
	// Exact original fresh capacities3, original initializer-list values and
	// their distinct source arrays+initializer-list carriers before allocation.
	if (!budget.peak(3 * sizeof(critical_entity_key) + 3 * sizeof(critical_entity_key) +
			 sizeof(std::initializer_list<critical_entity_key>) +
			 shop_codec_vector_frames))
		return false;
	work.candidate.keys = { work.player, work.account, work.shop };
	if (!budget.peak(3 * sizeof(critical_expected_revision) +
			 3 * sizeof(critical_expected_revision) +
			 sizeof(std::initializer_list<critical_expected_revision>) +
			 shop_codec_vector_frames))
		return false;
	work.candidate.expected_revisions = { { work.player, payload.expected_wallet_revision },
					      { work.account, payload.expected_bank_revision },
					      { work.shop, payload.expected_shop_revision } };
	if (!budget.peak(shop_codec_move_frames))
		return false;
	work.candidate.payload = std::move(work.encoded);
	for (work.index = 0; work.index < payload.item_count; ++work.index)
	{
		work.item = { critical_entity_type::item, payload.items[work.index].item_uid };
		if (!shop_codec_growth(work.candidate.keys, 1, budget))
			return false;
		work.candidate.keys.push_back(work.item);
		if (!shop_codec_growth(work.candidate.expected_revisions, 1, budget))
			return false;
		work.candidate.expected_revisions.push_back(
			{ work.item, payload.items[work.index].expected_item_revision });
	}
	if (creation(payload))
	{
		work.item = { critical_entity_type::item, payload.stock_item_uid };
		if (!shop_codec_growth(work.candidate.keys, 1, budget))
			return false;
		work.candidate.keys.push_back(work.item);
		if (!shop_codec_growth(work.candidate.expected_revisions, 1, budget))
			return false;
		work.candidate.expected_revisions.push_back(
			{ work.item, payload.expected_stock_item_revision });
	}
	if (payload.target_parent_item_uid)
	{
		work.item = { critical_entity_type::item, payload.target_parent_item_uid };
		if (!shop_codec_growth(work.candidate.keys, 1, budget))
			return false;
		work.candidate.keys.push_back(work.item);
		if (!shop_codec_growth(work.candidate.expected_revisions, 1, budget))
			return false;
		work.candidate.expected_revisions.push_back(
			{ work.item, payload.expected_target_parent_revision });
	}
	if (!shop_codec_sort_admit(work.candidate.keys.size(), sizeof(critical_entity_key),
				   sizeof(&critical_entity_key_less), budget))
		return false;
	std::sort(work.candidate.keys.begin(), work.candidate.keys.end(), critical_entity_key_less);
	if (!budget.peak(shop_codec_pure_frames))
		return false;
	if (std::adjacent_find(work.candidate.keys.begin(), work.candidate.keys.end(),
			       critical_entity_key_equal) != work.candidate.keys.end())
		return false;
	const auto revision_less =
		[](const critical_expected_revision &left, const critical_expected_revision &right)
	{ return critical_entity_key_less(left.key, right.key); };
	if (!shop_codec_sort_admit(work.candidate.expected_revisions.size(),
				   sizeof(critical_expected_revision), sizeof(revision_less),
				   budget))
		return false;
	std::sort(work.candidate.expected_revisions.begin(),
		  work.candidate.expected_revisions.end(), revision_less);
	if (!budget.peak(shop_codec_move_frames))
		return false;
	*out = std::move(work.candidate);
	return true;
}

bool shop_bounded_build_accounted(critical_command *out, critical_operation_id operation_id,
				  const shop_trade_payload &payload,
				  critical_source_site source_site,
				  critical_deadline_class deadline_class, shop_codec_budget &budget)
{
	if (!out)
		return false;
	constexpr size_t own = sizeof(shop_codec_copy_build_workspace) +
			       shop_codec_workspace_fixed + shop_codec_build_parameters;
	if (!budget.peak(own))
		return false;
	shop_codec_copy_build_workspace work;
	shop_codec_live_owner owner(budget, own, &work.previous, &work.values.candidate,
				    &work.values.encoded, nullptr);
	if (!shop_codec_clone_payload(payload, work.previous, budget))
		return false;
	work.previous.expected_player_save_revision = 0;
	work.previous.expected_player_level = 0;
	if (!shop_bounded_build_base(&work.values.candidate, operation_id, work.previous,
				     source_site, deadline_class, budget) ||
	    !shop_bounded_encode_accounted(payload, &work.values.encoded, budget))
		return false;
	work.values.candidate.payload_version = SHOP_TRADE_ACCOUNTED_PAYLOAD_VERSION;
	if (!budget.peak(shop_codec_move_frames))
		return false;
	work.values.candidate.payload = std::move(work.values.encoded);
	if (!budget.peak(shop_codec_move_frames))
		return false;
	*out = std::move(work.values.candidate);
	return true;
}
bool shop_bounded_build_native(critical_command *out, critical_operation_id operation_id,
			       const shop_trade_payload &payload, critical_source_site source_site,
			       critical_deadline_class deadline_class, shop_codec_budget &budget)
{
	if (!out)
		return false;
	constexpr size_t own = sizeof(shop_codec_copy_build_workspace) +
			       shop_codec_workspace_fixed + shop_codec_build_parameters;
	if (!budget.peak(own))
		return false;
	shop_codec_copy_build_workspace work;
	shop_codec_live_owner owner(budget, own, &work.previous, &work.values.candidate,
				    &work.values.encoded, nullptr);
	if (!shop_codec_clone_payload(payload, work.previous, budget))
		return false;
	work.previous.native_destination_weight_recorded = false;
	work.previous.destination_weight = {};
	if (!shop_bounded_build_accounted(&work.values.candidate, operation_id, work.previous,
					  source_site, deadline_class, budget) ||
	    !shop_bounded_encode_native(payload, &work.values.encoded, budget))
		return false;
	work.values.candidate.payload_version = SHOP_TRADE_NATIVE_PAYLOAD_VERSION;
	if (!budget.peak(shop_codec_move_frames))
		return false;
	work.values.candidate.payload = std::move(work.values.encoded);
	if (!budget.peak(shop_codec_move_frames))
		return false;
	*out = std::move(work.values.candidate);
	return true;
}
bool shop_bounded_build_recovery(critical_command *out, critical_operation_id operation_id,
				 const shop_trade_payload &payload,
				 critical_source_site source_site,
				 critical_deadline_class deadline_class, shop_codec_budget &budget)
{
	if (!out)
		return false;
	constexpr size_t own = sizeof(shop_codec_copy_build_workspace) +
			       shop_codec_workspace_fixed + shop_codec_build_parameters;
	if (!budget.peak(own))
		return false;
	shop_codec_copy_build_workspace work;
	shop_codec_live_owner owner(budget, own, &work.previous, &work.values.candidate,
				    &work.values.encoded, nullptr);
	if (!shop_codec_clone_payload(payload, work.previous, budget))
		return false;
	work.previous.recovery_manifest_recorded = false;
	if (!shop_codec_drop_manifest(work.previous, budget) ||
	    !shop_bounded_build_native(&work.values.candidate, operation_id, work.previous,
				       source_site, deadline_class, budget) ||
	    !shop_bounded_encode_recovery(payload, &work.values.encoded, budget))
		return false;
	work.values.candidate.payload_version = SHOP_TRADE_RECOVERY_PAYLOAD_VERSION;
	if (!budget.peak(shop_codec_move_frames))
		return false;
	work.values.candidate.payload = std::move(work.values.encoded);
	if (!budget.peak(shop_codec_move_frames))
		return false;
	*out = std::move(work.values.candidate);
	return true;
}

bool shop_bounded_decode_impl(const critical_command &command, shop_codec_decode_workspace &work,
			      shop_codec_budget &budget)
{
	auto *payload = &work.candidate;

	if (!payload || command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
	    command.type != critical_command_type::shop_trade ||
	    (!shop_trade_payload_version_is_accounted(command.payload_version) &&
	     command.payload_version != SHOP_TRADE_PAYLOAD_VERSION &&
	     command.payload_version != SHOP_TRADE_PREVIOUS_PAYLOAD_VERSION &&
	     command.payload_version != SHOP_TRADE_CONTAINER_PAYLOAD_VERSION &&
	     command.payload_version != SHOP_TRADE_STOCK_PAYLOAD_VERSION &&
	     command.payload_version != SHOP_TRADE_LEGACY_PAYLOAD_VERSION))
		return false;
	// Genuine fresh workspace candidate already has every original zero/default field.
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	uint8_t action = 0, name_length = 0;
	if (!read_le(&cursor, end, &action) || !read_le(&cursor, end, &payload->player_pid) ||
	    !read_le(&cursor, end, &payload->shop_id) ||
	    !read_le(&cursor, end, &payload->racewar) || !read_le(&cursor, end, &payload->price))
		return false;
	if (command.payload_version >= SHOP_TRADE_PAYLOAD_VERSION &&
	    (!read_le(&cursor, end, &payload->keeper_vnum) ||
	     !read_le(&cursor, end, &payload->expected_keeper_cash) ||
	     !read_le(&cursor, end, &payload->keeper_roaming)))
		return false;
	if (!read_le(&cursor, end, &payload->expected_wallet_revision) ||
	    !read_le(&cursor, end, &payload->expected_bank_revision) ||
	    !read_le(&cursor, end, &payload->expected_shop_revision) ||
	    !read_le(&cursor, end, &payload->selected_item_uid))
		return false;
	if (command.payload_version >= SHOP_TRADE_CONTAINER_PAYLOAD_VERSION)
	{
		if (!read_le(&cursor, end, &payload->target_root_item_uid) ||
		    !read_le(&cursor, end, &payload->target_parent_item_uid) ||
		    !read_le(&cursor, end, &payload->expected_target_parent_revision))
			return false;
	}
	else
		payload->target_root_item_uid = payload->selected_item_uid;
	if (command.payload_version >= SHOP_TRADE_STOCK_PAYLOAD_VERSION &&
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
	    (command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ?
		     static_cast<size_t>(end - cursor) <
			     payload->item_blob_size + SHOP_TRADE_NATIVE_TAIL_BYTES +
				     SHOP_TRADE_RECOVERY_MANIFEST_MIN_BYTES :
		     static_cast<size_t>(end - cursor) !=
			     payload->item_blob_size +
				     (command.payload_version == SHOP_TRADE_NATIVE_PAYLOAD_VERSION ?
					      SHOP_TRADE_NATIVE_TAIL_BYTES :
				      command.payload_version ==
						      SHOP_TRADE_ACCOUNTED_PAYLOAD_VERSION ?
					      SHOP_TRADE_ACCOUNTED_TAIL_BYTES :
					      0)))
		return false;
	memcpy(payload->item_blob.data(), cursor, payload->item_blob_size);
	if (shop_trade_payload_version_is_accounted(command.payload_version))
	{
		cursor += payload->item_blob_size;
		uint32_t padding = 0;
		if (!read_le(&cursor, end, &payload->expected_player_save_revision) ||
		    !read_le(&cursor, end, &payload->expected_player_level) ||
		    !read_le(&cursor, end, &padding) || padding ||
		    !payload->expected_player_save_revision || !payload->expected_player_level ||
		    payload->expected_player_level > UINT8_MAX)
			return false;
	}
	if (command.payload_version == SHOP_TRADE_NATIVE_PAYLOAD_VERSION ||
	    command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION)
	{
		payload->native_destination_weight_recorded = true;
		if (!read_le(&cursor, end, &payload->destination_weight.before) ||
		    !read_le(&cursor, end, &payload->destination_weight.direct_contents) ||
		    !read_le(&cursor, end, &payload->destination_weight.shell) ||
		    !read_le(&cursor, end, &payload->destination_weight.after) ||
		    (payload->target_parent_item_uid ?
			     payload->action != shop_trade_action::buy_produced ||
				     payload->target_root_item_uid !=
					     payload->target_parent_item_uid :
			     payload->destination_weight != shop_trade_destination_weight{}))
			return false;
	}
	if (command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION)
	{
		payload->recovery_manifest_recorded = true;
		if (!budget.current(&work.nested) ||
		    !shop_trade_recovery_manifest_decode_bounded(
			    std::span<const uint8_t>(cursor, static_cast<size_t>(end - cursor)),
			    &payload->recovery_manifest, budget.reserve, budget.context,
			    work.nested, nullptr) ||
		    payload->recovery_manifest.live_target_before.present !=
			    (payload->target_parent_item_uid != 0))
			return false;
		cursor = end;
	}
	if (shop_trade_payload_version_is_accounted(command.payload_version) && cursor != end)
		return false;
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
	if (!budget.peak(shop_codec_pure_frames))
		return false;
	if (!valid_payload(*payload))
		return false;
	auto &expected = work.expected;
	const bool built = command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ?
				   shop_bounded_build_recovery(&expected, command.operation_id,
							       *payload, command.source_site,
							       command.deadline_class, budget) :
			   command.payload_version == SHOP_TRADE_NATIVE_PAYLOAD_VERSION ?
				   shop_bounded_build_native(&expected, command.operation_id,
							     *payload, command.source_site,
							     command.deadline_class, budget) :
			   command.payload_version == SHOP_TRADE_ACCOUNTED_PAYLOAD_VERSION ?
				   shop_bounded_build_accounted(&expected, command.operation_id,
								*payload, command.source_site,
								command.deadline_class, budget) :
				   shop_bounded_build_base(&expected, command.operation_id,
							   *payload, command.source_site,
							   command.deadline_class, budget);
	if (!budget.peak(shop_codec_pure_frames))
		return false;
	return built && matching_fences(expected, command) &&
	       (command.payload_version != SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ||
		expected.payload == command.payload);
}

} // namespace

bool shop_trade_command_decode_payload_bounded(const critical_command &command,
					       shop_trade_payload *out,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer_live,
					       size_t *retained_payload_heap_bytes) noexcept
{
	if (!out || !reserve || !shop_codec_profile())
		return false;
	constexpr size_t public_frames = 5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
	size_t base = outer_live;
	if (!shop_codec_add(base, sizeof(shop_codec_budget) + public_frames) ||
	    !reserve(base, context))
		return false;
	shop_codec_budget budget{ reserve, context, base, nullptr };
	// Actual original decoder parameters/locals, fixed LE reader and native
	// tail/weight temporaries and manifest span argument carrier. Every fixed
	// candidate field lives in the genuine workspace, not a synthetic fixture.
	constexpr size_t decoder_frames = 9 * sizeof(void *) + 2 * sizeof(size_t) +
					  3 * sizeof(uint8_t) + sizeof(uint32_t) + sizeof(bool) +
					  sizeof(shop_trade_destination_weight) +
					  sizeof(std::span<const uint8_t>);
	constexpr size_t own =
		sizeof(shop_codec_decode_workspace) + shop_codec_workspace_fixed + decoder_frames;
	if (!budget.peak(own + shop_codec_leaf_frames + shop_codec_pure_frames))
		return false;
	shop_codec_decode_workspace work;
	shop_codec_live_owner owner(budget, own, &work.candidate, &work.expected, nullptr, nullptr);
	static_assert(std::is_nothrow_move_assignable_v<shop_trade_payload>);
	static_assert(std::is_nothrow_move_assignable_v<critical_command>);
	try
	{
		if (!shop_bounded_decode_impl(command, work, budget) ||
		    !shop_codec_payload_heap(work.candidate, work.transferred_heap) ||
		    !budget.peak(shop_codec_move_frames))
			return false;
		*out = std::move(work.candidate);
		if (retained_payload_heap_bytes)
			*retained_payload_heap_bytes = work.transferred_heap;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

// Additive SOURCE contracts for the unchanged complete v1-v8 bounded decoder.
#include <openssl/opensslv.h>
namespace
{
bool shop_command_source_supported() noexcept
{
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	       sizeof(std::allocator<uint8_t>) == 1 && sizeof(std::allocator<uint64_t>) == 1 &&
	       sizeof(std::allocator<critical_entity_key>) == 1 &&
	       sizeof(std::allocator<critical_expected_revision>) == 1 &&
	       sizeof(std::vector<uint8_t>::iterator) == sizeof(void *) &&
	       sizeof(std::vector<critical_entity_key>::iterator) == sizeof(void *) &&
	       sizeof(std::vector<critical_expected_revision>::iterator) == sizeof(void *);
#else
	return false;
#endif
}
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
// SOURCE_CORRECTION_GUARD
template <typename T> constexpr size_t shop_command_complete_move_source() noexcept
{
	// Same genuine selected four-vector movement graph, instantiated using
	// the actual element T; no unrelated codec/profile is borrowed.
	return 3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	       sizeof(std::true_type) + sizeof(std::vector<T>) + 7 * sizeof(void *) +
	       sizeof(std::allocator<T>) + 11 * sizeof(void *) +
	       2 * (2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(void *) +
		    3 * (2 * sizeof(void *))) +
	       2 * (2 * sizeof(void *)) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	       3 * sizeof(void *) + 3 * sizeof(void *) + sizeof(void *) +
	       shop_codec_allocator_frames;
}
constexpr size_t shop_command_move_missing =
	shop_command_complete_move_source<uint8_t>() - shop_codec_move_frames;
static_assert(shop_command_complete_move_source<uint8_t>() >= shop_codec_move_frames);
static_assert(shop_command_complete_move_source<uint64_t>() ==
	      shop_command_complete_move_source<uint8_t>());
static_assert(shop_command_complete_move_source<critical_entity_key>() ==
	      shop_command_complete_move_source<uint8_t>());
static_assert(shop_command_complete_move_source<critical_expected_revision>() ==
	      shop_command_complete_move_source<uint8_t>());

constexpr size_t shop_command_generated_lifetime =
	// Genuine generated decode/copy-build/copy-encode/base-build/base-encode
	// workspace default/destructor receivers. Each nested active workspace is
	// already inline-owned by its old prospective admission.
	2 * 5 * sizeof(void *) +
	// Payload default -> manifest -> binding -> six UID-vector default chains;
	// payload/manifest/binding generated destruction receivers. Actual route
	// holds candidate, three build projections and three encode projections.
	7 * (3 * sizeof(void *) + 6 * 6 * sizeof(void *) + 3 * sizeof(void *)) +
	// Conservative command default/cleanup source union: four reached families
	// cover decode/expected and original projection/base command construction.
	// These are sequential method closures, not a count of live command objects.
	// Their actual objects stay exclusively in old inline/workspace admissions.
	4 * (sizeof(void *) + 4 * 6 * sizeof(void *) + sizeof(void *)) +
	// Encoder/base build byte-vector defaults and cleanup. Original cleanup
	// runs outside the old growth/move peaks, so its allocator source remains
	// retained independently of successful allocation requests.
	8 * 6 * sizeof(void *) + 8 * (4 * sizeof(void *) + shop_codec_allocator_frames) +
	6 * (4 * sizeof(void *) + shop_codec_allocator_frames) +
	4 * (4 * sizeof(void *) + shop_codec_allocator_frames) +
	// Payload/manifest/binding generated move this/source and std::move
	// reference carriers; four command vectors use the exact correction above.
	3 * (2 * sizeof(void *)) + 6 * (2 * sizeof(void *) + 2 * sizeof(void *)) +
	2 * sizeof(void *) + 4 * shop_command_move_missing +
	// Real generated array/weight copies used by clone_payload, not an
	// unbounded payload copy: account, item array, blob, digest and weight.
	5 * (2 * sizeof(void *));

constexpr size_t shop_command_array_methods =
	// Actual array begin/end->data->_S_ptr, subscript->_S_ref, size.
	2 * (2 * sizeof(void *)) + 2 * (2 * sizeof(void *)) + 2 * sizeof(void *) +
	2 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t);
constexpr size_t shop_command_array_span_source =
	// account chars, item-entry array, blob and clone source/target pointer arrays.
	5 * shop_command_array_methods +
	// Real std::span(cursor,count) -> extent constructor; its data/size leaves.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) +
	2 * (sizeof(void *) + sizeof(size_t)) +
	// span pointer ctor: to_address(ptr,result) -> raw __to_address(ptr,result).
	2 * sizeof(void *) + 2 * sizeof(void *);
constexpr size_t shop_command_control_missing =
	// live_owner constructor this/budget/payload/command/two buffers/bytes,
	// destructor this; every genuine instantiation has the same six pointers.
	6 * sizeof(void *) + sizeof(size_t) + sizeof(void *) +
	// Profile result and max() constexpr returned scalar on real numeric checks.
	sizeof(bool) + sizeof(size_t) +
	// Original operation-ID zero range: ref/range/begin/end, uint8 value, bool;
	// its genuine array methods are outside shop_codec_pure_frames.
	4 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool) + shop_command_array_methods +
	// Actual original item_shopkeeper_owner_id formal uint32/result uint64.
	sizeof(uint32_t) + sizeof(uint64_t) +
	// Key less/equal originals and revision_less lambda receiver/refs/result.
	2 * (2 * sizeof(void *) + sizeof(bool)) + 3 * sizeof(void *) + sizeof(bool) +
	// bounded clone's six source/target pointer-array access wrappers, plus
	// real destination weight equality's generated this/other/bool.
	3 * sizeof(void *) + sizeof(bool) +
	// Actual valid_name's unsigned-char temporary and caught bad_alloc refs.
	sizeof(unsigned char) + 2 * sizeof(void *);

using shop_command_key_equal_adapter =
	__gnu_cxx::__ops::_Iter_comp_iter<decltype(&critical_entity_key_equal)>;
constexpr size_t shop_command_adjacent_source =
	// Original std::adjacent_find comparator overload and __adjacent_find:
	// input/result/next iterators, actual comparator/adapter and boolean.
	3 * sizeof(void *) + sizeof(&critical_entity_key_equal) + 4 * sizeof(void *) +
	sizeof(shop_command_key_equal_adapter) + sizeof(bool) +
	// __iter_comp_iter(fn) returned adapter, adapter ctor(this,fn), two move
	// reference chains and operator(this,left,right,bool).
	sizeof(&critical_entity_key_equal) + sizeof(shop_command_key_equal_adapter) +
	sizeof(void *) + sizeof(&critical_entity_key_equal) + 2 * (2 * sizeof(void *)) +
	3 * sizeof(void *) + sizeof(bool) +
	// Actual normal-iterator equality, ++, dereference and base chains.
	4 * (2 * sizeof(void *)) + sizeof(bool) + 2 * (2 * sizeof(void *));
constexpr size_t shop_command_sort_missing =
	// The old sort admission covers actual recursion, partition/insertion/heap
	// scalar/value scopes, but not their move/copy descendants. Real typed
	// pointer transport graph used by move_backward remains active in sort;
	// the old vector allocation admission cannot supply that later scope.
	shop_codec_copy_frames +
	// Function-pointer and revision-lambda comparison adapters: their real
	// conversion/ctor/std::move/invocation receiver carriers.
	2 * (3 * (sizeof(void *) + sizeof(&critical_entity_key_less)) + 6 * (2 * sizeof(void *)) +
	     3 * (3 * sizeof(void *) + sizeof(bool))) +
	// iter_swap/swap actual refs and key/revision value temporary branches.
	2 * (4 * sizeof(void *) + 2 * (2 * sizeof(void *))) + sizeof(critical_entity_key) +
	sizeof(critical_expected_revision);
constexpr size_t shop_command_local_supplement =
	shop_command_generated_lifetime + shop_command_array_span_source +
	shop_command_control_missing + shop_command_adjacent_source + shop_command_sort_missing;

// Exact named old shop_codec_pure_frames credit for original is_empty's
// binding arrays, all_of/find_if_not/negated predicate, empty digest/equality.
// This is a field-by-field existing-source credit, not a guessed heap deduction.
constexpr size_t shop_command_old_empty_credit =
	2 * sizeof(std::array<const shop_trade_recovery_forest_binding *, 6>) +
	3 * (2 * sizeof(void *) + sizeof(char) + sizeof(void *)) +
	shop_codec_find_frames(sizeof(char)) + sizeof(std::array<uint8_t, 32>) +
	3 * sizeof(void *) + 2 * sizeof(bool) + shop_codec_equal_frames;

constexpr size_t shop_command_old_selected_source =
	// Public args/base/result and original decode parameters/read leaves;
	// body workspaces themselves are separately initial/child-owned.
	5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + 9 * sizeof(void *) +
	2 * sizeof(size_t) + 3 * sizeof(uint8_t) + sizeof(uint32_t) + sizeof(bool) +
	sizeof(shop_trade_destination_weight) + sizeof(std::span<const uint8_t>) +
	shop_codec_census_frames + shop_codec_leaf_frames + shop_codec_pure_frames +
	// Four original build and four encode wrapper signatures are genuine
	// mutually exclusive/sequential union members, with nested v8 projections.
	4 * shop_codec_build_parameters + 4 * shop_codec_encode_parameters +
	shop_codec_move_frames +
	// Existing sort carrier admission is evaluated for two actual families.
	// Valid payload bounds item_count<=12; keys/revisions therefore <=12+5.
	// 2*floor(log2(17))+1=9 source recursion scopes; no new arbitrary cap.
	2 * (9 * (3 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(&critical_entity_key_less)) +
	     18 * sizeof(void *) + 7 * sizeof(&critical_entity_key_less) +
	     sizeof(critical_expected_revision) + 16 * sizeof(void *) +
	     6 * sizeof(&critical_entity_key_less) + 2 * sizeof(critical_expected_revision) +
	     23 * sizeof(void *) + 11 * sizeof(std::ptrdiff_t) +
	     7 * sizeof(&critical_entity_key_less) + 4 * sizeof(critical_expected_revision) +
	     8 * sizeof(void *) + 5 * sizeof(&critical_entity_key_less) + 4 * sizeof(bool) +
	     2 * sizeof(void *) + 8 * sizeof(size_t) + sizeof(bool));

#endif

bool shop_command_selected_source(size_t *output, bool supplement) noexcept
{
	if (!output || !shop_command_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	size_t account = 0, manifest = 0, empty = 0;
	if (!(supplement ? currency_account_key_source_supplement_frame_bytes(&account) :
			   currency_account_key_source_frame_bytes(&account)) ||
	    !(supplement ?
		      shop_trade_recovery_manifest_codec_source_supplement_frame_bytes(&manifest) :
		      shop_trade_recovery_manifest_codec_source_frame_bytes(&manifest)) ||
	    !shop_trade_recovery_manifest_is_empty_source_frame_bytes(&empty) ||
	    empty < shop_command_old_empty_credit)
		return false;
	size_t total = shop_command_local_supplement;
	if (!shop_codec_add(total, account) || !shop_codec_add(total, manifest) ||
	    !shop_codec_add(total, supplement ? empty - shop_command_old_empty_credit : empty) ||
	    (!supplement && !shop_codec_add(total, shop_command_old_selected_source)))
		return false;
	*output = total;
	return true;
#else
	(void)supplement;
	return false;
#endif
}
}
bool shop_trade_command_decode_payload_source_frame_bytes(size_t *output) noexcept
{
	return shop_command_selected_source(output, false);
}
bool shop_trade_command_decode_payload_source_supplement_frame_bytes(size_t *output) noexcept
{
	return shop_command_selected_source(output, true);
}
bool shop_trade_command_decode_payload_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !shop_command_source_supported())
		return false;
	// Exact real first budget/decode workspace/linked owner; all nested copies
	// and allocations remain covered by their unchanged prospective admissions.
	*output = sizeof(shop_codec_budget) + sizeof(shop_codec_decode_workspace) +
		  sizeof(shop_codec_live_owner);
	return true;
}
