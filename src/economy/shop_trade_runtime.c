#include "economy/shop_trade_runtime.h"

#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "economy/economic_gameplay_authority.h"
#include "player/player_save_pipeline.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <limits>
#include <new>
#include <set>
#include <unordered_map>
#ifndef __NO_MYSQL__
#include "persistence/economic_sql_shop_trade_transaction.h"
#endif

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

extern struct shop_data *shop_index;
extern int number_of_shops;

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

static shop_trade_payload_build_result
build_payload(P_char player, P_char keeper, P_obj selected, P_obj stock, P_obj destination,
	      uint32_t shop_id, shop_trade_action action, int64_t price,
	      shop_trade_payload *payload,
	      const player_shop_checkpoint_stage *player_status = nullptr,
	      uint64_t native_shop_revision = 0)
{
	if (!player || IS_NPC(player) || !player->only.pc || GET_PID(player) <= 0 || !keeper ||
	    !IS_NPC(keeper) || GET_VNUM(keeper) <= 0 || !shop_index || number_of_shops < 0 ||
	    shop_id >= static_cast<uint32_t>(number_of_shops) ||
	    GET_RNUM(keeper) != shop_index[shop_id].keeper || !selected || !selected->obj_uid ||
	    !payload || price < 0 || price > INT_MAX ||
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
	uint64_t shop_revision = native_shop_revision;
	if (player_status)
	{
		if (!economic_gameplay_authority::active_regular_sql() ||
		    !player_status->save_revision || !player_status->level ||
		    player_status->level > 255 ||
		    static_cast<uint32_t>(GET_LEVEL(player)) != player_status->level ||
		    !shop_revision || shop_revision == std::numeric_limits<uint64_t>::max())
			return shop_trade_payload_build_result::unavailable;
	}
	else if (!shop_trade_runtime_revision(shop_id, &shop_revision))
		return shop_trade_payload_build_result::unavailable;

	std::vector<player_item_snapshot> snapshots;
	const auto captured =
		player_status ?
			player_item_snapshot_tree_capture_literal(selected, &snapshots, nullptr) :
			player_item_snapshot_tree_capture(selected, &snapshots, nullptr);
	if (captured != player_snapshot_capture_result::ok || snapshots.empty() ||
	    snapshots.size() > SHOP_TRADE_MAX_ITEMS ||
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
	built.keeper_vnum = GET_VNUM(keeper);
	built.keeper_roaming = shop_index[shop_id].shop_is_roaming != 0;
	built.expected_keeper_cash = static_cast<int64_t>(GET_COPPER(keeper)) +
				     10LL * GET_SILVER(keeper) + 100LL * GET_GOLD(keeper) +
				     1000LL * GET_PLATINUM(keeper);
	if (built.expected_keeper_cash < 0 || built.expected_keeper_cash > INT_MAX)
		return shop_trade_payload_build_result::unavailable;
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
	if (player_status)
	{
		built.expected_player_save_revision = player_status->save_revision;
		built.expected_player_level = player_status->level;
		if (!shop_trade_command_encode_accounted_payload(built, &validated))
			return shop_trade_payload_build_result::invalid;
	}
	else if (!shop_trade_command_encode_payload(built, &validated))
		return shop_trade_payload_build_result::invalid;
	*payload = std::move(built);
	return shop_trade_payload_build_result::ok;
}

shop_trade_payload_build_result
shop_trade_runtime_build_payload(P_char player, P_char keeper, P_obj selected, P_obj stock,
				 P_obj destination, uint32_t shop_id, shop_trade_action action,
				 int64_t price, shop_trade_payload *payload)
{
	return build_payload(player, keeper, selected, stock, destination, shop_id, action, price,
			     payload);
}

shop_trade_payload_build_result shop_trade_runtime_build_accounted_payload(
	P_char player, P_char keeper, P_obj selected, P_obj stock, P_obj destination,
	uint32_t shop_id, shop_trade_action action, int64_t price,
	const player_shop_checkpoint_stage &player_status, uint64_t native_shop_revision,
	shop_trade_payload *payload)
{
	return build_payload(player, keeper, selected, stock, destination, shop_id, action, price,
			     payload, &player_status, native_shop_revision);
}

bool shop_trade_runtime_object_matches_accounted_payload(P_obj selected,
							 const shop_trade_payload &payload)
{
	if (!selected || selected->obj_uid != payload.selected_item_uid ||
	    !payload.expected_player_save_revision || !payload.expected_player_level ||
	    payload.expected_player_level > 255 || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size())
		return false;
	std::vector<player_item_snapshot> snapshots;
	std::vector<uint8_t> encoded;
	return player_item_snapshot_tree_capture_literal(selected, &snapshots, nullptr) ==
		       player_snapshot_capture_result::ok &&
	       player_item_snapshot_list_encode(snapshots, &encoded) ==
		       player_snapshot_codec_result::ok &&
	       encoded.size() == payload.item_blob_size &&
	       std::equal(encoded.begin(), encoded.end(), payload.item_blob.begin());
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

#ifndef __NO_MYSQL__
bool shop_trade_current_runtime_owner::publish(
	MYSQL *connection, const shop_trade_payload &payload,
	const economic_sql_shop_trade_publication &current) noexcept
{
	if (!nevent_is_game_thread() || !connection || !current.session_id ||
	    mysql_thread_id(connection) != current.session_id ||
	    !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    current.custody.size() != current.custody_vnums.size() || !current.shop_revision)
		return false;
	try
	{
		const item_owner_identity player{ item_owner_type::player, payload.player_pid, 0 };
		const item_owner_identity keeper{ item_owner_type::shopkeeper,
						  item_shopkeeper_owner_id(payload.shop_id), 0 };
		const item_owner_identity counterparty =
			payload.action == shop_trade_action::buy_produced ?
				item_owner_identity{ item_owner_type::system, 0, 0 } :
			(payload.action == shop_trade_action::sell_destroy ||
			 payload.action == shop_trade_action::discard_invalid) ?
				item_owner_identity{ item_owner_type::destruction, 0, 0 } :
				keeper;
		auto revision_for = [&](const item_owner_identity &owner, uint64_t *revision)
		{
			if (item_owner_identity_equal(owner, player))
				*revision = current.wallet_owner_revision;
			else if (item_owner_identity_equal(owner, keeper))
				*revision = current.keeper_owner_revision;
			else if (item_owner_identity_equal(owner, counterparty))
				*revision = current.counterparty_owner_revision;
			else
				return false;
			return true;
		};
		uint64_t cached = 0;
		if ((item_ownership_runtime_peek_owner_revision(player, &cached) &&
		     cached > current.wallet_owner_revision) ||
		    (item_ownership_runtime_peek_owner_revision(keeper, &cached) &&
		     cached > current.keeper_owner_revision))
			return false;
		const auto old_shop = shop_revisions.find(payload.shop_id);
		if (old_shop == shop_revisions.end() || old_shop->second > current.shop_revision)
			return false;
		// Rejected produced outputs have no SQL rows and therefore are not in
		// current.custody. The original output IDs must also be absent in cache.
		if (current.rejected && payload.action == shop_trade_action::buy_produced)
			for (size_t index = 0; index < payload.item_count; ++index)
			{
				item_ownership_runtime_entry existing{};
				if (item_ownership_runtime_lookup(payload.items[index].item_uid,
								  &existing))
					return false;
			}
		std::vector<item_ownership_runtime_entry> batch;
		batch.reserve(current.custody.size());
		for (size_t index = 0; index < current.custody.size(); ++index)
		{
			const auto &entry = current.custody[index];
			const auto &position = entry.position;
			if (position.state == item_custody_state::absent)
			{
				item_ownership_runtime_entry existing{};
				if (item_ownership_runtime_lookup(entry.uid, &existing))
					return false;
				continue;
			}
			uint64_t owner_revision = 0;
			if (!revision_for(position.owner, &owner_revision))
			{
				// Historical descendants may belong to an owner outside this
				// locked owner cut. Never fabricate or hydrate its revision.
				if (position.state == item_custody_state::active)
					return false;
				item_ownership_runtime_entry existing{};
				if (item_ownership_runtime_lookup(entry.uid, &existing) &&
				    (existing.root_item_uid != position.root_uid ||
				     existing.parent_item_uid != position.parent_uid ||
				     !item_owner_identity_equal(existing.owner, position.owner) ||
				     existing.item_revision != position.revision ||
				     existing.vnum != current.custody_vnums[index] ||
				     existing.state != position.state))
					return false;
				continue;
			}
			batch.push_back({ entry.uid, position.root_uid, position.parent_uid,
					  position.owner, position.revision, owner_revision,
					  current.custody_vnums[index], position.state });
		}
		if (!item_ownership_runtime_hydrate_many_atomic(batch.data(), batch.size()) ||
		    !item_ownership_runtime_hydrate_owner(player, current.wallet_owner_revision) ||
		    !item_ownership_runtime_hydrate_owner(keeper, current.keeper_owner_revision))
			return false;
		for (const auto &entry : batch)
		{
			item_ownership_runtime_entry actual{};
			if (!item_ownership_runtime_lookup(entry.item_uid, &actual) ||
			    actual.root_item_uid != entry.root_item_uid ||
			    actual.parent_item_uid != entry.parent_item_uid ||
			    !item_owner_identity_equal(actual.owner, entry.owner) ||
			    actual.item_revision != entry.item_revision ||
			    actual.owner_revision != entry.owner_revision ||
			    actual.vnum != entry.vnum || actual.state != entry.state)
				return false;
		}
		// Active-root census includes foreign/malformed active claims. Omitted
		// unrelated history remains retained; explicit history was checked above.
		std::set<uint64_t> roots;
		for (const auto &entry : current.custody)
			roots.insert(entry.position.root_uid);
		for (uint64_t root : roots)
		{
			std::vector<item_ownership_runtime_entry> actual;
			if (!item_ownership_runtime_snapshot_active_root(root, batch.size() + 1,
									 &actual))
				return false;
			size_t expected_count = 0;
			for (const auto &entry : batch)
				if (entry.root_item_uid == root &&
				    entry.state == item_custody_state::active)
					++expected_count;
			if (actual.size() != expected_count)
				return false;
			for (const auto &entry : actual)
				if (std::none_of(batch.begin(), batch.end(),
						 [&](const auto &value)
						 {
							 return value.item_uid == entry.item_uid &&
								value.root_item_uid == root &&
								value.state ==
									item_custody_state::active;
						 }))
					return false;
		}
		if (!item_ownership_runtime_peek_owner_revision(player, &cached) ||
		    cached != current.wallet_owner_revision ||
		    !item_ownership_runtime_peek_owner_revision(keeper, &cached) ||
		    cached != current.keeper_owner_revision)
			return false;
		shop_revisions.find(payload.shop_id)->second = current.shop_revision;
		return mysql_thread_id(connection) == current.session_id &&
		       (connection->server_status & SERVER_STATUS_IN_TRANS);
	}
	catch (...)
	{
		return false;
	}
}
#endif
