#include "economy/collector_purchase_preparation.h"

#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>
#include <vector>

namespace
{
bool same_record(const collector::record &left, const collector::record &right)
{
	std::array<uint8_t, collector::encoded_record_bytes> left_bytes = {}, right_bytes = {};
	return collector::record_encode(left, &left_bytes) == collector::codec_result::ok &&
	       collector::record_encode(right, &right_bytes) == collector::codec_result::ok &&
	       left_bytes == right_bytes;
}

bool valid_account_name(const std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> &account_name)
{
	const size_t length = strnlen(account_name.data(), account_name.size());
	if (!length || length >= account_name.size())
		return false;
	return std::all_of(account_name.begin(), account_name.begin() + length,
			   [](char value) { return static_cast<unsigned char>(value) >= 0x20; });
}

bool has_capacity(const collector_purchase_actor_snapshot &actor, const player_item_snapshot &item)
{
	if (actor.carried_weight < 0 || actor.carry_weight_limit < 0 ||
	    actor.carried_items >= actor.carry_item_limit)
		return false;
	const int64_t item_weight = std::max<int64_t>(0, item.weight);
	return item_weight <= actor.carry_weight_limit &&
	       actor.carried_weight <= actor.carry_weight_limit - item_weight;
}
}

collector_purchase_prepare_outcome
collector_purchase_prepare(const collector::record &runtime_entry,
			   const collector_listing_detail &detail,
			   const item_ownership_runtime_entry &held_item,
			   const collector_purchase_actor_snapshot &actor,
			   collector_command_payload *payload, player_item_snapshot *decoded_item)
{
	if (!payload || !decoded_item || !actor.pid || actor.racewar > 4 || !actor.observed_at ||
	    !valid_account_name(actor.account_name))
		return collector_purchase_prepare_outcome::invalid_actor;
	if (runtime_entry.status != collector::state::available || runtime_entry.holding_paused ||
	    actor.observed_at < runtime_entry.available_at ||
	    actor.observed_at >= runtime_entry.expires_at)
		return collector_purchase_prepare_outcome::unavailable;
	if (runtime_entry.beneficiary != actor.pid)
		return collector_purchase_prepare_outcome::forbidden;
	if (actor.carried_currency_value < runtime_entry.price_value)
		return collector_purchase_prepare_outcome::insufficient_funds;
	player_item_snapshot item;
	switch (collector_listing_decode_item(runtime_entry, detail, &item))
	{
	case collector_listing_decode_outcome::decoded:
		break;
	case collector_listing_decode_outcome::stale_listing:
		return collector_purchase_prepare_outcome::stale_listing;
	case collector_listing_decode_outcome::invalid_item:
		return collector_purchase_prepare_outcome::invalid_item;
	case collector_listing_decode_outcome::allocation_failure:
		return collector_purchase_prepare_outcome::allocation_failure;
	}

	const item_owner_identity expected_collector = {
		item_owner_type::collector, item_collector_owner_id(runtime_entry.listing), 0
	};
	if (held_item.item_uid != runtime_entry.uid ||
	    held_item.root_item_uid != runtime_entry.uid || held_item.parent_item_uid ||
	    !item_owner_identity_equal(held_item.owner, expected_collector) ||
	    held_item.item_revision != runtime_entry.item_revision || held_item.vnum != item.vnum ||
	    held_item.state != item_custody_state::active)
		return collector_purchase_prepare_outcome::stale_custody;
	if (!has_capacity(actor, item))
		return collector_purchase_prepare_outcome::capacity;

	collector_command_payload candidate = {};
	candidate.action = collector_action::purchase;
	candidate.target_state = item_custody_state::active;
	candidate.capacity_admitted = true;
	candidate.listing = runtime_entry.listing;
	candidate.expected_listing_revision = runtime_entry.revision;
	candidate.observed_at = actor.observed_at;
	candidate.actor_pid = actor.pid;
	candidate.racewar = actor.racewar;
	candidate.account_name = actor.account_name;
	candidate.expected_wallet_revision = actor.wallet_revision;
	candidate.expected_bank_revision = actor.bank_revision;
	candidate.from_owner = expected_collector;
	candidate.to_owner = { item_owner_type::player, actor.pid, 0 };
	candidate.expected_from_owner_revision = held_item.owner_revision;
	candidate.expected_to_owner_revision = actor.item_owner_revision;
	candidate.selected_item_uid = runtime_entry.uid;
	candidate.item_count = 1;
	candidate.items[0] = {
		held_item.item_uid,	 held_item.root_item_uid, held_item.parent_item_uid,
		held_item.item_revision, held_item.vnum,	  held_item.state
	};
	candidate.item_blob_size = static_cast<uint32_t>(detail.item_blob.size());
	std::copy(detail.item_blob.begin(), detail.item_blob.end(), candidate.item_blob.begin());
	*payload = std::move(candidate);
	*decoded_item = std::move(item);
	return collector_purchase_prepare_outcome::prepared;
}

collector_listing_decode_outcome
collector_listing_decode_item(const collector::record &runtime_entry,
			      const collector_listing_detail &detail,
			      player_item_snapshot *decoded_item)
{
	if (!decoded_item)
		return collector_listing_decode_outcome::invalid_item;
	if (!same_record(runtime_entry, detail.entry))
		return collector_listing_decode_outcome::stale_listing;
	if (detail.item_blob.empty() ||
	    detail.item_blob.size() > COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES)
		return collector_listing_decode_outcome::invalid_item;
	std::vector<player_item_snapshot> decoded;
	try
	{
		const auto result = player_item_snapshot_list_decode(
			detail.item_blob.data(), detail.item_blob.size(), &decoded);
		if (result == player_snapshot_codec_result::allocation_failure)
			return collector_listing_decode_outcome::allocation_failure;
		if (result != player_snapshot_codec_result::ok || decoded.size() != 1 ||
		    decoded[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
		    decoded[0].equipment_slot != 0 || !decoded[0].object_uid ||
		    decoded[0].object_uid != runtime_entry.uid || decoded[0].vnum <= 0)
			return collector_listing_decode_outcome::invalid_item;
	}
	catch (const std::bad_alloc &)
	{
		return collector_listing_decode_outcome::allocation_failure;
	}
	*decoded_item = std::move(decoded[0]);
	return collector_listing_decode_outcome::decoded;
}
