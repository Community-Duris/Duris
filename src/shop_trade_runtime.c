#include "shop_trade_runtime.h"

#include "item_ownership_runtime.h"
#include "player_snapshot_capture.h"
#include "player_snapshot_codec.h"
#include "prototypes.h"
#include "utils.h"

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
