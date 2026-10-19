#include "economy/shop_trade_runtime.h"

#include "item/item_ownership_runtime.h"
#include "item/item_movement_transaction.h"
#include "economy/economic_state.h"
#include "persistence/persistence_mode.h"
#include <cerrno>
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <unordered_map>

namespace
{
std::unordered_map<uint32_t, uint64_t> shop_revisions;

bool is_buy(shop_trade_action action)
{
	return action == shop_trade_action::buy_existing ||
	       action == shop_trade_action::buy_produced;
}

bool shop_owned(shop_trade_action action)
{
	return is_buy(action) || action == shop_trade_action::discard_invalid;
}
} // namespace

bool shop_trade_runtime_replace_revisions(const std::vector<flatfile_shopkeeper_record> &records)
{
	std::unordered_map<uint32_t, uint64_t> replacement;
	try
	{
		replacement.reserve(records.size());
		for (const auto &record : records)
			if (!record.revision ||
			    !replacement.emplace(record.shop_id, record.revision).second)
				return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	shop_revisions.swap(replacement);
	return true;
}

bool shop_trade_runtime_revision(uint32_t shop_id, uint64_t *revision)
{
	if (!revision)
		return false;
	const auto found = shop_revisions.find(shop_id);
	if (found == shop_revisions.end())
		return false;
	*revision = found->second;
	return true;
}

bool shop_trade_runtime_can_advance(uint32_t shop_id, uint64_t expected_revision,
				    uint64_t new_revision)
{
	const auto found = shop_revisions.find(shop_id);
	return found != shop_revisions.end() && expected_revision &&
	       expected_revision != std::numeric_limits<uint64_t>::max() &&
	       found->second == expected_revision && new_revision == expected_revision + 1;
}

bool shop_trade_runtime_advance(uint32_t shop_id, uint64_t expected_revision, uint64_t new_revision)
{
	if (!shop_trade_runtime_can_advance(shop_id, expected_revision, new_revision))
		return false;
	shop_revisions[shop_id] = new_revision;
	return true;
}

void shop_trade_runtime_reset_for_tests(void)
{
	shop_revisions.clear();
}

shop_trade_payload_build_result
shop_trade_runtime_build_payload(P_char player, P_obj selected, P_obj stock, P_obj destination,
				 uint32_t shop_id, shop_trade_action action, int64_t price,
				 shop_trade_payload *payload)
{
	if (!player || IS_NPC(player) || !player->only.pc || GET_PID(player) <= 0 || !selected ||
	    !selected->obj_uid || !payload || price < 0 || price > INT_MAX ||
	    (action == shop_trade_action::discard_invalid ? price != 0 :
							    (!is_buy(action) && price == 0)) ||
	    action <= shop_trade_action::unknown || action > shop_trade_action::discard_invalid ||
	    ((action == shop_trade_action::buy_produced) != (stock != nullptr)) ||
	    (destination && action != shop_trade_action::buy_produced))
		return shop_trade_payload_build_result::invalid;
	const char *account_name = get_account_name_safe(player);
	if (!account_name || !strcmp(account_name, "Unknown") ||
	    strlen(account_name) > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return shop_trade_payload_build_result::unavailable;
	uint64_t shop_revision = 0;
	if (!shop_trade_runtime_revision(shop_id, &shop_revision))
		return shop_trade_payload_build_result::unavailable;

	if (economic_state_active() && persistence_mode_requires_mysql())
	{
		// Flat-file stock is adopted with its shop catalog. Its generic item
		// projector does not support creating shopkeeper stock independently.
		P_obj exemplar = stock ? stock : shop_owned(action) ? selected : nullptr;
		item_ownership_runtime_entry entry;
		if (exemplar && !item_ownership_runtime_lookup(exemplar->obj_uid, &entry))
		{
			const item_owner_identity owner{ item_owner_type::shopkeeper,
							 item_shopkeeper_owner_id(shop_id), 0 };
			(void)item_movement_transaction_submit(player, exemplar, nullptr, owner,
							       owner,
							       item_transfer_reason::creation,
							       shop_id, nullptr, nullptr, 0);
			return shop_trade_payload_build_result::unavailable;
		}
	}
	std::vector<player_item_snapshot> snapshots;
	if (player_item_snapshot_tree_capture(selected, &snapshots, nullptr) !=
		    player_snapshot_capture_result::ok ||
	    snapshots.empty() || snapshots.size() > SHOP_TRADE_MAX_ITEMS ||
	    snapshots.front().object_uid != selected->obj_uid)
		return shop_trade_payload_build_result::capture_failure;
	std::vector<uint8_t> blob;
	if (player_item_snapshot_list_encode(snapshots, &blob) !=
		    player_snapshot_codec_result::ok ||
	    blob.size() > SHOP_TRADE_ITEM_BLOB_MAX_BYTES)
		return shop_trade_payload_build_result::capture_failure;

	const bool creates = action == shop_trade_action::buy_produced;
	const item_owner_identity player_owner = { item_owner_type::player,
						   static_cast<uint32_t>(GET_PID(player)), 0 };
	const item_owner_identity shop_owner = { item_owner_type::shopkeeper,
						 item_shopkeeper_owner_id(shop_id), 0 };
	const item_owner_identity expected_owner = shop_owned(action) ? shop_owner : player_owner;
	shop_trade_payload built = {};
	built.action = action;
	built.player_pid = static_cast<uint32_t>(GET_PID(player));
	built.shop_id = shop_id;
	built.racewar = static_cast<uint8_t>(GET_RACEWAR(player));
	strcpy(built.account_name.data(), account_name);
	built.price = price;
	built.expected_wallet_revision = player->only.pc->wallet_revision;
	built.expected_bank_revision = player->only.pc->bank_revision;
	built.expected_shop_revision = shop_revision;
	built.selected_item_uid = selected->obj_uid;
	built.target_root_item_uid = selected->obj_uid;
	built.item_count = static_cast<uint16_t>(snapshots.size());
	built.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), built.item_blob.begin());

	for (size_t index = 0; index < snapshots.size(); ++index)
	{
		const auto &snapshot = snapshots[index];
		const uint64_t parent_uid =
			snapshot.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
				0 :
				snapshots[static_cast<size_t>(snapshot.parent_index)].object_uid;
		item_ownership_runtime_entry runtime = {};
		const bool adopted = item_ownership_runtime_lookup(snapshot.object_uid, &runtime);
		if ((creates && adopted) ||
		    (!creates &&
		     (!adopted || !item_owner_identity_equal(runtime.owner, expected_owner) ||
		      runtime.root_item_uid != selected->obj_uid ||
		      runtime.parent_item_uid != parent_uid || runtime.vnum != snapshot.vnum ||
		      runtime.state != item_custody_state::active)))
			return shop_trade_payload_build_result::unavailable;
		built.items[index] = {
			snapshot.object_uid,
			selected->obj_uid,
			parent_uid,
			creates ? ITEM_TRANSFER_ABSENT_REVISION : runtime.item_revision,
			snapshot.vnum,
			creates ? item_custody_state::absent : item_custody_state::active,
		};
	}
	std::sort(built.items.begin(), built.items.begin() + built.item_count,
		  [](const auto &left, const auto &right)
		  { return left.item_uid < right.item_uid; });

	if (creates)
	{
		item_ownership_runtime_entry exemplar = {};
		if (!stock->obj_uid || stock->obj_uid == selected->obj_uid ||
		    !item_ownership_runtime_lookup(stock->obj_uid, &exemplar) ||
		    !item_owner_identity_equal(exemplar.owner, shop_owner) ||
		    exemplar.root_item_uid != exemplar.item_uid || exemplar.parent_item_uid ||
		    exemplar.state != item_custody_state::active ||
		    exemplar.vnum != OBJ_VNUM(stock) || exemplar.vnum != snapshots.front().vnum)
			return shop_trade_payload_build_result::unavailable;
		built.stock_item_uid = exemplar.item_uid;
		built.expected_stock_item_revision = exemplar.item_revision;
		built.stock_vnum = exemplar.vnum;
		if (destination)
		{
			item_ownership_runtime_entry target = {};
			if (!destination->obj_uid || destination->obj_uid == selected->obj_uid ||
			    !item_ownership_runtime_lookup(destination->obj_uid, &target) ||
			    !item_owner_identity_equal(target.owner, player_owner) ||
			    target.root_item_uid != target.item_uid || target.parent_item_uid ||
			    target.state != item_custody_state::active ||
			    target.vnum != OBJ_VNUM(destination))
				return shop_trade_payload_build_result::unavailable;
			built.target_root_item_uid = target.root_item_uid;
			built.target_parent_item_uid = target.item_uid;
			built.expected_target_parent_revision = target.item_revision;
		}
	}
	else if (action == shop_trade_action::buy_existing ||
		 action == shop_trade_action::discard_invalid)
	{
		const auto selected_entry = std::find_if(
			built.items.begin(), built.items.begin() + built.item_count,
			[&](const auto &item) { return item.item_uid == selected->obj_uid; });
		if (selected_entry == built.items.begin() + built.item_count)
			return shop_trade_payload_build_result::unavailable;
		built.stock_item_uid = selected_entry->item_uid;
		built.expected_stock_item_revision = selected_entry->expected_item_revision;
		built.stock_vnum = selected_entry->vnum;
	}
	std::vector<uint8_t> validated;
	if (!shop_trade_command_encode_payload(built, &validated))
		return shop_trade_payload_build_result::invalid;
	*payload = std::move(built);
	return shop_trade_payload_build_result::ok;
}

bool shop_trade_runtime_object_matches_payload(P_obj selected, const shop_trade_payload &payload)
{
	if (!selected || selected->obj_uid != payload.selected_item_uid)
		return false;
	std::vector<player_item_snapshot> snapshots;
	std::vector<uint8_t> encoded;
	return player_item_snapshot_tree_capture(selected, &snapshots, nullptr) ==
		       player_snapshot_capture_result::ok &&
	       player_item_snapshot_list_encode(snapshots, &encoded) ==
		       player_snapshot_codec_result::ok &&
	       encoded.size() == payload.item_blob_size &&
	       std::equal(encoded.begin(), encoded.end(), payload.item_blob.begin());
}

bool shop_trade_runtime_seed_revision(uint32_t shop_id, uint64_t revision)
{
	return revision && shop_revisions.emplace(shop_id, revision).second;
}

unsigned int shop_trade_runtime_prepare(const shop_trade_payload &payload,
					shop_trade_result *result)
{
	if (!result)
		return EINVAL;
	uint64_t revision = 0;
	if (!shop_trade_runtime_revision(payload.shop_id, &revision))
		return ENOENT;
	if (revision != payload.expected_shop_revision)
		return ESTALE;
	if (revision == UINT64_MAX)
		return ERANGE;
	currency_command_result wallet;
	if (!economic_state_balances(payload.player_pid, payload.account_name.data(),
				     payload.racewar, &wallet))
		return ENOENT;
	if (wallet.wallet_revision != payload.expected_wallet_revision ||
	    wallet.bank_revision != payload.expected_bank_revision)
		return ESTALE;
	std::vector<player_item_snapshot> snapshots;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &snapshots) != player_snapshot_codec_result::ok ||
	    snapshots.size() != payload.item_count || snapshots.empty() ||
	    snapshots.front().object_uid != payload.selected_item_uid)
		return EINVAL;
	if (payload.action == shop_trade_action::sell_store &&
	    std::any_of(snapshots.begin(), snapshots.end(),
			[](const auto &snapshot) { return !snapshot.dynamic_affects.empty(); }))
		return EOPNOTSUPP;
	if (payload.action == shop_trade_action::buy_produced)
	{
		item_ownership_runtime_entry stock;
		if (!item_ownership_runtime_lookup(payload.stock_item_uid, &stock) ||
		    stock.owner.type != item_owner_type::shopkeeper ||
		    stock.owner.id != item_shopkeeper_owner_id(payload.shop_id) ||
		    stock.root_item_uid != stock.item_uid || stock.parent_item_uid ||
		    stock.item_revision != payload.expected_stock_item_revision ||
		    stock.vnum != payload.stock_vnum || stock.state != item_custody_state::active ||
		    snapshots.front().vnum != payload.stock_vnum)
			return ESTALE;
	}
	item_transfer_payload transfer;
	if (!shop_trade_item_payload(payload, &transfer))
		return EINVAL;
	item_ownership_runtime_peek_owner_revision(transfer.from_owner,
						   &transfer.expected_from_revision);
	item_ownership_runtime_peek_owner_revision(transfer.to_owner,
						   &transfer.expected_to_revision);
	item_transfer_result moved;
	if (auto error = item_ownership_runtime_prepare(transfer, &moved))
		return error;
	if (payload.action != shop_trade_action::discard_invalid)
	{
		const int64_t delta = is_buy(payload.action) ? -payload.price : payload.price;
		const auto &amount = wallet.wallet.amount;
		const int64_t current =
			amount[0] + 10 * amount[1] + 100 * amount[2] + 1000 * amount[3];
		if (delta < 0 && current < -delta)
			return ENOSPC;
		if (delta > 0 && current > INT64_MAX - delta)
			return ERANGE;
		const int64_t value = current + delta;
		if (value / 1000 > INT_MAX || wallet.wallet_revision == UINT64_MAX ||
		    wallet.bank_revision == UINT64_MAX)
			return ERANGE;
		wallet.wallet = { { value % 10, (value / 10) % 10, (value / 100) % 10,
				    value / 1000 } };
		++wallet.wallet_revision;
		++wallet.bank_revision;
	}
	*result = {};
	result->action = payload.action;
	result->wallet = wallet.wallet;
	result->bank = wallet.bank;
	result->wallet_revision = wallet.wallet_revision;
	result->bank_revision = wallet.bank_revision;
	result->shop_revision = revision + 1;
	result->player_owner_revision = is_buy(payload.action) ? moved.to_owner_revision :
								 moved.from_owner_revision;
	result->counterparty_owner_revision = is_buy(payload.action) ? moved.from_owner_revision :
								       moved.to_owner_revision;
	result->item_count = payload.item_count;
	for (size_t i = 0; i < payload.item_count; ++i)
	{
		result->item_uids[i] = payload.items[i].item_uid;
		result->item_revisions[i] = payload.action == shop_trade_action::buy_produced ?
						    1 :
						    payload.items[i].expected_item_revision + 1;
	}
	return 0;
}
