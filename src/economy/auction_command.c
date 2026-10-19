#include "economy/auction_command.h"
#include "item/item_transfer_command.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <openssl/sha.h>

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

template <size_t Size>
bool append_string(std::vector<uint8_t> *output, const std::array<char, Size> &value)
{
	const size_t length = strnlen(value.data(), value.size());
	if (length >= value.size() || length > UINT16_MAX)
		return false;
	append_le<uint16_t>(output, static_cast<uint16_t>(length));
	output->insert(output->end(), value.begin(), value.begin() + length);
	return true;
}

template <size_t Size>
bool read_string(const uint8_t **cursor, const uint8_t *end, std::array<char, Size> *value)
{
	uint16_t length = 0;
	if (!read_le(cursor, end, &length) || length >= value->size() ||
	    static_cast<size_t>(end - *cursor) < length)
		return false;
	value->fill(0);
	memcpy(value->data(), *cursor, length);
	*cursor += length;
	return true;
}

uint64_t listing_key(const critical_operation_id &operation_id)
{
	uint64_t key = 0;
	for (size_t index = 0; index < sizeof(key); ++index)
		key |= static_cast<uint64_t>(operation_id.bytes[index]) << (index * 8);
	return key ? key : 1;
}

bool valid_payload(const auction_command_payload &payload)
{
	if (payload.action <= auction_action::unknown || payload.action > auction_action::remove ||
	    payload.item_count > AUCTION_COMMAND_MAX_ITEMS ||
	    payload.object_blob_size >= payload.object_blob.size())
		return false;
	if ((payload.action == auction_action::list ||
	     payload.action == auction_action::claim_item) &&
	    !payload.item_count)
		return false;
	// finalize and remove are authoritative closures: they stage items back to the
	// seller (or to the recorded winner) and never touch an actor wallet, so both
	// run actor-less from expiry and from the DurisWeb removal path.
	if (payload.action != auction_action::finalize &&
	    payload.action != auction_action::remove && !payload.actor_pid)
		return false;
	if (payload.action != auction_action::list && !payload.auction_id &&
	    payload.action != auction_action::claim_money)
		return false;
	if (payload.actor_pid &&
	    (!payload.account_name[0] ||
	     strnlen(payload.account_name.data(), payload.account_name.size()) >=
		     payload.account_name.size()))
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
		if (!payload.items[index].item_uid || payload.items[index].vnum < 0)
			return false;
	return true;
}

bool matching_fences(const critical_command &left, const critical_command &right)
{
	if (left.keys.size() != right.keys.size() ||
	    left.expected_revisions.size() != right.expected_revisions.size())
		return false;
	for (size_t index = 0; index < left.keys.size(); ++index)
		if (left.keys[index].type != right.keys[index].type ||
		    left.keys[index].id != right.keys[index].id)
			return false;
	for (size_t index = 0; index < left.expected_revisions.size(); ++index)
		if (left.expected_revisions[index].key.type !=
			    right.expected_revisions[index].key.type ||
		    left.expected_revisions[index].key.id !=
			    right.expected_revisions[index].key.id ||
		    left.expected_revisions[index].revision !=
			    right.expected_revisions[index].revision)
			return false;
	return true;
}
} // namespace

bool auction_command_encode_payload(const auction_command_payload &payload,
				    std::vector<uint8_t> *encoded)
{
	if (!encoded || !valid_payload(payload))
		return false;
	try
	{
		encoded->clear();
		encoded->reserve(512 + payload.object_blob_size +
				 strnlen(payload.object_info.data(), payload.object_info.size()));
		append_le<uint8_t>(encoded, static_cast<uint8_t>(payload.action));
		append_le<uint32_t>(encoded, payload.actor_pid);
		append_le<uint32_t>(encoded, payload.auction_id);
		append_le<uint8_t>(encoded, payload.racewar);
		append_le<uint64_t>(encoded, payload.expected_wallet_revision);
		append_le<uint64_t>(encoded, payload.expected_bank_revision);
		append_le<int64_t>(encoded, payload.value);
		append_le<int64_t>(encoded, payload.start_price);
		append_le<int64_t>(encoded, payload.buy_price);
		append_le<int64_t>(encoded, payload.listing_fee);
		append_le<uint32_t>(encoded, payload.closing_fee_basis_points);
		append_le<uint32_t>(encoded, payload.bid_extension_seconds);
		append_le<uint64_t>(encoded, payload.end_time);
		append_le<uint16_t>(encoded, payload.item_count);
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			append_le<uint64_t>(encoded, payload.items[index].item_uid);
			append_le<uint64_t>(encoded, payload.items[index].expected_item_revision);
			append_le<int32_t>(encoded, payload.items[index].vnum);
		}
		if (!append_string(encoded, payload.account_name) ||
		    !append_string(encoded, payload.actor_name) ||
		    !append_string(encoded, payload.object_short) ||
		    !append_string(encoded, payload.id_keywords) ||
		    !append_string(encoded, payload.object_info))
			return false;
		append_le<uint32_t>(encoded, payload.object_blob_size);
		encoded->insert(encoded->end(), payload.object_blob.begin(),
				payload.object_blob.begin() + payload.object_blob_size);
	}
	catch (const std::bad_alloc &)
	{
		encoded->clear();
		return false;
	}
	return encoded->size() <= CRITICAL_COMMAND_MAX_PAYLOAD_BYTES;
}

bool auction_command_decode_payload(const critical_command &command,
				    auction_command_payload *payload)
{
	if (!payload || command.type != critical_command_type::auction ||
	    command.payload_version != AUCTION_COMMAND_PAYLOAD_VERSION)
		return false;
	*payload = {};
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	uint8_t action = 0;
	if (!read_le(&cursor, end, &action) || !read_le(&cursor, end, &payload->actor_pid) ||
	    !read_le(&cursor, end, &payload->auction_id) ||
	    !read_le(&cursor, end, &payload->racewar) ||
	    !read_le(&cursor, end, &payload->expected_wallet_revision) ||
	    !read_le(&cursor, end, &payload->expected_bank_revision) ||
	    !read_le(&cursor, end, &payload->value) ||
	    !read_le(&cursor, end, &payload->start_price) ||
	    !read_le(&cursor, end, &payload->buy_price) ||
	    !read_le(&cursor, end, &payload->listing_fee) ||
	    !read_le(&cursor, end, &payload->closing_fee_basis_points) ||
	    !read_le(&cursor, end, &payload->bid_extension_seconds) ||
	    !read_le(&cursor, end, &payload->end_time) ||
	    !read_le(&cursor, end, &payload->item_count) ||
	    payload->item_count > payload->items.size())
		return false;
	payload->action = static_cast<auction_action>(action);
	for (size_t index = 0; index < payload->item_count; ++index)
		if (!read_le(&cursor, end, &payload->items[index].item_uid) ||
		    !read_le(&cursor, end, &payload->items[index].expected_item_revision) ||
		    !read_le(&cursor, end, &payload->items[index].vnum))
			return false;
	if (!read_string(&cursor, end, &payload->account_name) ||
	    !read_string(&cursor, end, &payload->actor_name) ||
	    !read_string(&cursor, end, &payload->object_short) ||
	    !read_string(&cursor, end, &payload->id_keywords) ||
	    !read_string(&cursor, end, &payload->object_info) ||
	    !read_le(&cursor, end, &payload->object_blob_size) ||
	    payload->object_blob_size > payload->object_blob.size() ||
	    static_cast<size_t>(end - cursor) != payload->object_blob_size)
		return false;
	memcpy(payload->object_blob.data(), cursor, payload->object_blob_size);
	if (!valid_payload(*payload))
		return false;
	critical_command expected = {};
	if (!auction_command_build(&expected, command.operation_id, *payload, command.source_site,
				   command.deadline_class) ||
	    !matching_fences(expected, command))
		return false;
	return true;
}

bool auction_command_encode_result(const auction_command_result &result,
				   std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> *encoded)
{
	if (!encoded || result.item_count > result.item_uids.size())
		return false;
	encoded->fill(0);
	std::vector<uint8_t> bytes;
	try
	{
		append_le<uint8_t>(&bytes, static_cast<uint8_t>(result.action));
		append_le<uint8_t>(&bytes, static_cast<uint8_t>(result.event_type));
		append_le<uint32_t>(&bytes, result.auction_id);
		append_le<uint32_t>(&bytes, result.status);
		append_le<uint32_t>(&bytes, result.seller_pid);
		append_le<uint32_t>(&bytes, result.winner_pid);
		append_le<uint32_t>(&bytes, result.previous_bidder_pid);
		append_le<int64_t>(&bytes, result.final_price);
		append_le<int64_t>(&bytes, result.wallet_value_delta);
		for (int64_t value : result.wallet.amount)
			append_le<int64_t>(&bytes, value);
		for (int64_t value : result.bank.amount)
			append_le<int64_t>(&bytes, value);
		append_le<uint64_t>(&bytes, result.wallet_revision);
		append_le<uint64_t>(&bytes, result.bank_revision);
		append_le<uint64_t>(&bytes, result.auction_revision);
		append_le<uint64_t>(&bytes, result.player_owner_revision);
		append_le<uint64_t>(&bytes, result.auction_owner_revision);
		append_le<uint16_t>(&bytes, result.item_count);
		for (size_t index = 0; index < result.item_count; ++index)
		{
			append_le<uint64_t>(&bytes, result.item_uids[index]);
			append_le<uint64_t>(&bytes, result.item_revisions[index]);
		}
		bytes.resize(288, 0);
		append_le<uint64_t>(&bytes, result.end_time);
		append_le<int64_t>(&bytes, result.refund);
		append_le<int64_t>(&bytes, result.seller_proceeds);
		append_le<uint64_t>(&bytes, result.catalog_revision);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (bytes.size() > encoded->size())
		return false;
	std::copy(bytes.begin(), bytes.end(), encoded->begin());
	return true;
}

bool auction_command_decode_result(const uint8_t *encoded, size_t size,
				   auction_command_result *result)
{
	if (!encoded || !result || size != AUCTION_RESULT_PAYLOAD_BYTES)
		return false;
	*result = {};
	const uint8_t *cursor = encoded;
	const uint8_t *end = encoded + size;
	uint8_t action = 0, event = 0;
	if (!read_le(&cursor, end, &action) || !read_le(&cursor, end, &event) ||
	    !read_le(&cursor, end, &result->auction_id) ||
	    !read_le(&cursor, end, &result->status) ||
	    !read_le(&cursor, end, &result->seller_pid) ||
	    !read_le(&cursor, end, &result->winner_pid) ||
	    !read_le(&cursor, end, &result->previous_bidder_pid) ||
	    !read_le(&cursor, end, &result->final_price) ||
	    !read_le(&cursor, end, &result->wallet_value_delta))
		return false;
	result->action = static_cast<auction_action>(action);
	result->event_type = static_cast<auction_event_type>(event);
	for (int64_t &value : result->wallet.amount)
		if (!read_le(&cursor, end, &value))
			return false;
	for (int64_t &value : result->bank.amount)
		if (!read_le(&cursor, end, &value))
			return false;
	if (!read_le(&cursor, end, &result->wallet_revision) ||
	    !read_le(&cursor, end, &result->bank_revision) ||
	    !read_le(&cursor, end, &result->auction_revision) ||
	    !read_le(&cursor, end, &result->player_owner_revision) ||
	    !read_le(&cursor, end, &result->auction_owner_revision) ||
	    !read_le(&cursor, end, &result->item_count) ||
	    result->item_count > result->item_uids.size())
		return false;
	for (size_t index = 0; index < result->item_count; ++index)
		if (!read_le(&cursor, end, &result->item_uids[index]) ||
		    !read_le(&cursor, end, &result->item_revisions[index]))
			return false;
	cursor = encoded + 288;
	return read_le(&cursor, end, &result->end_time) && read_le(&cursor, end, &result->refund) &&
	       read_le(&cursor, end, &result->seller_proceeds) &&
	       read_le(&cursor, end, &result->catalog_revision) &&
	       result->action > auction_action::unknown && result->action <= auction_action::remove;
}

uint32_t auction_command_listing_id(const critical_operation_id &operation_id)
{
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
	SHA256(operation_id.bytes.data(), operation_id.bytes.size(), digest.data());
	uint32_t id = 0;
	for (size_t index = 0; index < sizeof(id); ++index)
		id |= static_cast<uint32_t>(digest[index]) << (8 * index);
	// auctions.id is a signed SQL INT in both supported engines.
	id &= INT32_MAX;
	return id ? id : 1;
}

bool auction_blob_parts(const auction_command_payload &payload, const uint8_t **legacy,
			size_t *legacy_size, const uint8_t **runtime, size_t *runtime_size)
{
	if (!legacy || !legacy_size || !runtime || !runtime_size || !payload.object_blob_size ||
	    payload.object_blob_size >= payload.object_blob.size())
		return false;
	*legacy = payload.object_blob.data();
	*legacy_size = payload.object_blob_size;
	*runtime = nullptr;
	*runtime_size = 0;
	if (payload.object_blob_size >= 8 && !memcmp(*legacy, "DURSAUC2", 8))
	{
		const uint8_t *cursor = *legacy + 8,
			      *end = payload.object_blob.data() + payload.object_blob_size;
		uint32_t old_size = 0, new_size = 0;
		if (!read_le(&cursor, end, &old_size) || !read_le(&cursor, end, &new_size) ||
		    !old_size || !new_size ||
		    static_cast<size_t>(end - cursor) != static_cast<size_t>(old_size) + new_size)
			return false;
		*legacy = cursor;
		*legacy_size = old_size;
		*runtime = cursor + old_size;
		*runtime_size = new_size;
	}
	// write_one_object is a binary format, including embedded zero bytes.
	// Its stored length, rather than strlen or a terminator, bounds the envelope.
	return *legacy_size != 0;
}
bool auction_blob_pack(auction_command_payload *payload, const std::vector<uint8_t> &runtime)
{
	const uint8_t *legacy = nullptr, *old_runtime = nullptr;
	size_t legacy_size = 0, old_runtime_size = 0;
	if (!payload || runtime.empty() ||
	    !auction_blob_parts(*payload, &legacy, &legacy_size, &old_runtime, &old_runtime_size) ||
	    16 + legacy_size + runtime.size() >= payload->object_blob.size())
		return false;
	std::vector<uint8_t> encoded{ 'D', 'U', 'R', 'S', 'A', 'U', 'C', '2' };
	append_le<uint32_t>(&encoded, legacy_size);
	append_le<uint32_t>(&encoded, runtime.size());
	encoded.insert(encoded.end(), legacy, legacy + legacy_size);
	encoded.insert(encoded.end(), runtime.begin(), runtime.end());
	payload->object_blob_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), payload->object_blob.begin());
	return true;
}
bool auction_item_payload(const auction_command_payload &payload, uint32_t auction_id,
			  item_transfer_payload *transfer)
{
	const uint8_t *legacy = nullptr, *runtime = nullptr;
	size_t legacy_size = 0, runtime_size = 0;
	std::vector<player_item_snapshot> captured, selected;
	if (!transfer || !payload.item_count ||
	    !auction_blob_parts(payload, &legacy, &legacy_size, &runtime, &runtime_size) ||
	    !runtime_size ||
	    player_item_snapshot_list_decode(runtime, runtime_size, &captured) !=
		    player_snapshot_codec_result::ok)
		return false;
	if (captured.size() > AUCTION_COMMAND_MAX_ITEMS ||
	    (payload.action == auction_action::list && captured.size() != payload.item_count))
		return false;
	for (size_t i = 0; i < captured.size(); ++i)
	{
		if (!captured[i].object_uid ||
		    captured[i].parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			return false;
		for (size_t j = 0; j < i; ++j)
			if (captured[i].object_uid == captured[j].object_uid)
				return false;
	}
	*transfer = {};
	const item_owner_identity player{ item_owner_type::player, payload.actor_pid, 0 };
	const item_owner_identity auction{ item_owner_type::auction, auction_id, 0 };
	const bool listing = payload.action == auction_action::list;
	transfer->from_owner = listing ? player : auction;
	transfer->to_owner = listing ? auction : player;
	transfer->reason = listing ? item_transfer_reason::auction_list :
				     item_transfer_reason::auction_claim;
	transfer->multi_root = true;
	transfer->selected_item_uid = payload.items[0].item_uid;
	transfer->target_root_item_uid = payload.items[0].item_uid;
	transfer->item_count = payload.item_count;
	for (size_t i = 0; i < payload.item_count; ++i)
	{
		const auto &item = payload.items[i];
		for (size_t j = 0; j < i; ++j)
			if (item.item_uid == payload.items[j].item_uid)
				return false;
		const auto found = std::find_if(captured.begin(), captured.end(), [&](const auto &s)
						{ return s.object_uid == item.item_uid; });
		if (found == captured.end() || found->vnum != item.vnum ||
		    found->parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			return false;
		selected.push_back(*found);
		selected.back().equipment_slot = 0;
		transfer->items[i] = { item.item_uid,
				       item.item_uid,
				       0,
				       item.expected_item_revision,
				       item.vnum,
				       item_custody_state::active };
	}
	std::vector<uint8_t> bytes;
	if (player_item_snapshot_list_encode(selected, &bytes) !=
		    player_snapshot_codec_result::ok ||
	    bytes.size() > transfer->item_blob.size())
		return false;
	transfer->item_blob_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), transfer->item_blob.begin());
	return true;
}

bool auction_command_build(critical_command *command, critical_operation_id operation_id,
			   const auction_command_payload &payload, critical_source_site source_site,
			   critical_deadline_class deadline_class)
{
	if (!command || !valid_payload(payload))
		return false;
	critical_entity_key account_key = {};
	std::vector<critical_entity_key> keys;
	std::vector<critical_expected_revision> revisions;
	try
	{
		if (payload.actor_pid)
		{
			const critical_entity_key player = { critical_entity_type::player,
							     payload.actor_pid };
			if (!currency_account_key(payload.account_name.data(), payload.racewar,
						  &account_key))
				return false;
			keys.push_back(player);
			keys.push_back(account_key);
			revisions.push_back({ player, payload.expected_wallet_revision });
			revisions.push_back({ account_key, payload.expected_bank_revision });
		}
		keys.push_back(
			{ critical_entity_type::auction,
			  payload.auction_id ? payload.auction_id : listing_key(operation_id) });
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const critical_entity_key item = { critical_entity_type::item,
							   payload.items[index].item_uid };
			keys.push_back(item);
			revisions.push_back({ item, payload.items[index].expected_item_revision });
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::auction,
		     .payload_version = AUCTION_COMMAND_PAYLOAD_VERSION,
		     .source_site = source_site,
		     .deadline_class = deadline_class,
		     .accepted_at_usec = 0,
		     .keys = std::move(keys),
		     .expected_revisions = std::move(revisions),
		     .payload = {} };
	if (!auction_command_encode_payload(payload, &command->payload))
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
