#include "economy/shop_trade_runtime.h"

#include "flatfile/flatfile_accounting_shop_transaction.h"
#include "flatfile/flatfile_accounting_native_mobile_birth_shared_shop_transaction.h"
#include "economy/shop_trade_accounting.h"
#include <map>
#include <tuple>

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
#include <utility>
#ifndef __NO_MYSQL__
#include "persistence/economic_sql_shop_trade_transaction.h"
#include "persistence/economic_sql_native_mobile_birth_transaction.h"
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
		if (old_shop != shop_revisions.end() && old_shop->second > current.shop_revision)
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
		// Allocate the missing authenticated SQL revision before any cache hydration.
		// Keep its node private until the complete owner readback succeeds.
		decltype(shop_revisions)::node_type staged_shop;
		if (old_shop == shop_revisions.end())
		{
			decltype(shop_revisions) staged;
			staged.emplace(payload.shop_id, current.shop_revision);
			staged_shop = staged.extract(payload.shop_id);
			shop_revisions.reserve(shop_revisions.size() + 1);
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
		if (mysql_thread_id(connection) != current.session_id ||
		    !(connection->server_status & SERVER_STATUS_IN_TRANS))
			return false;
		if (staged_shop)
		{
			if (!shop_revisions.insert(std::move(staged_shop)).inserted)
				return false;
		}
		else
			shop_revisions.find(payload.shop_id)->second = current.shop_revision;
		return mysql_thread_id(connection) == current.session_id &&
		       (connection->server_status & SERVER_STATUS_IN_TRANS);
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_current_runtime_owner::publish_native_birth(
	MYSQL *connection, const critical_command &command,
	const flatfile_shopkeeper_record &original_checkpoint,
	const critical_completion &completion) noexcept
{
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active_regular_sql() ||
	    !connection || !(connection->server_status & SERVER_STATUS_IN_TRANS))
		return false;
	try
	{
		const auto session = mysql_thread_id(connection);
		quest_mobile_native_image image;
		std::vector<item_ownership_runtime_entry> batch;
		// Reauthenticate the complete original command, retained receipt, checkpoint
		// and actual still-born SQL forest on this reconnect-off IN_TRANS session.
		// Passive participant values alone never grant projection permission.
		if (economic_sql_native_mobile_birth_shared_shop_lock_publication(
			    connection, command, original_checkpoint, completion, &image, &batch))
			return false;
		native_mobile_birth_cash_role_result result{};
		if (!native_mobile_birth_cash_role_result_decode({ completion.result_payload.data(),
								   completion.result_size },
								 &result) ||
		    result.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    result.wallet_mapping_id ||
		    result.mobile_instance_id != image.reference.mobile_instance_id ||
		    result.shared.shop_id != original_checkpoint.shop_id ||
		    !result.shared.shop_after_present || result.shared.shop_revision_after != 1 ||
		    original_checkpoint.revision != result.shared.shop_revision_after ||
		    result.item_owner_id != item_shopkeeper_owner_id(result.shared.shop_id) ||
		    result.item_owner_revision != result.shared.owner_revision_after ||
		    image.items.size() != batch.size())
			return false;
		const item_owner_identity keeper{ item_owner_type::shopkeeper, result.item_owner_id,
						  0 };
		const item_owner_identity ordinary{ item_owner_type::native_mobile,
						    result.mobile_instance_id, 0 };
		uint64_t cached = 0;
		const bool owner_present =
			item_ownership_runtime_peek_owner_revision(keeper, &cached);
		if ((owner_present && (!result.shared.owner_after_present ||
				       cached > result.shared.owner_revision_after)) ||
		    item_ownership_runtime_peek_owner_revision(ordinary, &cached) ||
		    (!batch.empty() && !result.shared.owner_after_present))
			return false;
		const auto old_shop = shop_revisions.find(result.shared.shop_id);
		if (old_shop != shop_revisions.end() &&
		    old_shop->second != result.shared.shop_revision_after)
			return false;
		auto same = [](const item_ownership_runtime_entry &left,
			       const item_ownership_runtime_entry &right)
		{
			return left.item_uid == right.item_uid &&
			       left.root_item_uid == right.root_item_uid &&
			       left.parent_item_uid == right.parent_item_uid &&
			       item_owner_identity_equal(left.owner, right.owner) &&
			       left.item_revision == right.item_revision &&
			       left.owner_revision == right.owner_revision &&
			       left.vnum == right.vnum && left.state == right.state;
		};
		std::map<uint64_t, const item_ownership_runtime_entry *> born;
		for (const auto &entry : batch)
		{
			if (!entry.item_uid || !entry.root_item_uid ||
			    !item_owner_identity_equal(entry.owner, keeper) ||
			    entry.item_revision != 1 ||
			    entry.owner_revision != result.shared.owner_revision_after ||
			    entry.state != item_custody_state::active ||
			    !born.emplace(entry.item_uid, &entry).second)
				return false;
			item_ownership_runtime_entry existing{};
			if (item_ownership_runtime_lookup(entry.item_uid, &existing))
			{
				// Only exact born topology/owner may be present. An older selected
				// owner clock may advance to the genuine SQL clock; no foreign,
				// inactive, ahead or historical item row is overwritten.
				if (existing.owner_revision > entry.owner_revision)
					return false;
				existing.owner_revision = entry.owner_revision;
				if (!same(existing, entry))
					return false;
			}
		}
		std::vector<item_ownership_runtime_entry> before;
		if (item_ownership_runtime_snapshot_all_active(262144, &before))
			return false;
		for (const auto &entry : before)
		{
			const auto found = born.find(entry.item_uid);
			const bool selected_owner = entry.owner.type ==
							    item_owner_type::shopkeeper &&
						    entry.owner.id == keeper.id;
			const bool wrong_ordinary = entry.owner.type ==
							    item_owner_type::native_mobile &&
						    entry.owner.id == ordinary.id;
			const bool related = selected_owner || wrong_ordinary ||
					     found != born.end() ||
					     born.count(entry.root_item_uid) ||
					     born.count(entry.parent_item_uid);
			if (related)
			{
				if (found == born.end() ||
				    entry.owner_revision > found->second->owner_revision)
					return false;
				auto expected = entry;
				expected.owner_revision = found->second->owner_revision;
				if (!same(expected, *found->second))
					return false;
			}
		}
		// All caller-side allocations and the complete active owner/link census
		// precede cache effects. Reserve does not publish a SHOP clock.
		decltype(shop_revisions)::node_type staged_shop;
		if (old_shop == shop_revisions.end())
		{
			decltype(shop_revisions) staged;
			staged.emplace(result.shared.shop_id, result.shared.shop_revision_after);
			staged_shop = staged.extract(result.shared.shop_id);
			shop_revisions.reserve(shop_revisions.size() + 1);
		}
		if (mysql_thread_id(connection) != session ||
		    !(connection->server_status & SERVER_STATUS_IN_TRANS))
			return false;
		// The existing atomic primitive preflights its own allocations and restores
		// its exact previous rows/counters on allocation refusal. Nonempty stock
		// supplies the actual owner clock in the same batch. Empty stock preserves
		// absence or hydrates genuine present zero/nonzero, without creating defaults.
		if (!item_ownership_runtime_hydrate_many_atomic(batch.data(), batch.size()) ||
		    (batch.empty() && result.shared.owner_after_present &&
		     !item_ownership_runtime_hydrate_owner(keeper,
							   result.shared.owner_revision_after)))
			return false;
		// No callbacks or intervening writer run on the serialized game thread.
		// Full preflight census plus the atomic batch's bounded mutation set forms
		// the complete post-cut census. Read back every original active row and
		// every born row without allocating another snapshot after effects.
		for (const auto &entry : before)
		{
			const auto found = born.find(entry.item_uid);
			const auto &expected = found == born.end() ? entry : *found->second;
			item_ownership_runtime_entry actual{};
			if (!item_ownership_runtime_lookup(entry.item_uid, &actual) ||
			    !same(actual, expected))
				return false;
		}
		for (const auto &entry : batch)
		{
			item_ownership_runtime_entry actual{};
			if (!item_ownership_runtime_lookup(entry.item_uid, &actual) ||
			    !same(actual, entry))
				return false;
		}
		const bool actual_present =
			item_ownership_runtime_peek_owner_revision(keeper, &cached);
		if (actual_present != result.shared.owner_after_present ||
		    (actual_present && cached != result.shared.owner_revision_after) ||
		    item_ownership_runtime_peek_owner_revision(ordinary, &cached) ||
		    mysql_thread_id(connection) != session ||
		    !(connection->server_status & SERVER_STATUS_IN_TRANS))
			return false;
		if (staged_shop && !shop_revisions.insert(std::move(staged_shop)).inserted)
			return false;
		const auto actual_shop = shop_revisions.find(result.shared.shop_id);
		return actual_shop != shop_revisions.end() &&
		       actual_shop->second == result.shared.shop_revision_after &&
		       mysql_thread_id(connection) == session &&
		       (connection->server_status & SERVER_STATUS_IN_TRANS);
	}
	catch (...)
	{
		// Any refusal after cache effects remains held/unknown at the original
		// owner. Never erase or synthesize state to claim physical publication.
		return false;
	}
}

#endif

bool shop_trade_current_runtime_owner::publish_native_birth_flat(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	const critical_completion &completion) noexcept
{
	if (!nevent_is_game_thread() || !economic_gameplay_authority::active_regular_flat() ||
	    !lock.matches(root))
		return false;
	try
	{
		flatfile_shared_native_birth_publication_projection current;
		if (flatfile_shared_native_birth_publication_storage::read_locked(
			    root, lock, original, completion, &current))
			return false;
		const auto &image = current.native;
		const auto &original_checkpoint = current.keeper;
		const auto &batch = current.custody;
		native_mobile_birth_cash_role_result result{};
		if (!native_mobile_birth_cash_role_result_decode({ completion.result_payload.data(),
								   completion.result_size },
								 &result) ||
		    result.role != native_mobile_birth_cash_role::shared_shopkeeper ||
		    result.wallet_mapping_id ||
		    result.mobile_instance_id != image.reference.mobile_instance_id ||
		    result.shared.shop_id != original_checkpoint.shop_id ||
		    !result.shared.shop_after_present || result.shared.shop_revision_after != 1 ||
		    original_checkpoint.revision != result.shared.shop_revision_after ||
		    result.item_owner_id != item_shopkeeper_owner_id(result.shared.shop_id) ||
		    result.item_owner_revision != result.shared.owner_revision_after ||
		    image.items.size() != batch.size())
			return false;
		const item_owner_identity keeper{ item_owner_type::shopkeeper, result.item_owner_id,
						  0 };
		const item_owner_identity ordinary{ item_owner_type::native_mobile,
						    result.mobile_instance_id, 0 };
		uint64_t cached = 0;
		const bool owner_present =
			item_ownership_runtime_peek_owner_revision(keeper, &cached);
		if ((owner_present && (!result.shared.owner_after_present ||
				       cached > result.shared.owner_revision_after)) ||
		    item_ownership_runtime_peek_owner_revision(ordinary, &cached) ||
		    (!batch.empty() && !result.shared.owner_after_present))
			return false;
		const auto old_shop = shop_revisions.find(result.shared.shop_id);
		if (old_shop != shop_revisions.end() &&
		    old_shop->second != result.shared.shop_revision_after)
			return false;
		auto same = [](const item_ownership_runtime_entry &left,
			       const item_ownership_runtime_entry &right)
		{
			return left.item_uid == right.item_uid &&
			       left.root_item_uid == right.root_item_uid &&
			       left.parent_item_uid == right.parent_item_uid &&
			       item_owner_identity_equal(left.owner, right.owner) &&
			       left.item_revision == right.item_revision &&
			       left.owner_revision == right.owner_revision &&
			       left.vnum == right.vnum && left.state == right.state;
		};
		std::map<uint64_t, const item_ownership_runtime_entry *> born;
		for (const auto &entry : batch)
		{
			if (!entry.item_uid || !entry.root_item_uid ||
			    !item_owner_identity_equal(entry.owner, keeper) ||
			    entry.item_revision != 1 ||
			    entry.owner_revision != result.shared.owner_revision_after ||
			    entry.state != item_custody_state::active ||
			    !born.emplace(entry.item_uid, &entry).second)
				return false;
			item_ownership_runtime_entry existing{};
			if (item_ownership_runtime_lookup(entry.item_uid, &existing))
			{
				// Only exact born topology/owner may be present. An older selected
				// owner clock may advance to the genuine native clock; no foreign,
				// inactive, ahead or historical item row is overwritten.
				if (existing.owner_revision > entry.owner_revision)
					return false;
				existing.owner_revision = entry.owner_revision;
				if (!same(existing, entry))
					return false;
			}
		}
		std::vector<item_ownership_runtime_entry> before;
		if (item_ownership_runtime_snapshot_all_active(262144, &before))
			return false;
		for (const auto &entry : before)
		{
			const auto found = born.find(entry.item_uid);
			const bool selected_owner = entry.owner.type ==
							    item_owner_type::shopkeeper &&
						    entry.owner.id == keeper.id;
			const bool wrong_ordinary = entry.owner.type ==
							    item_owner_type::native_mobile &&
						    entry.owner.id == ordinary.id;
			const bool related = selected_owner || wrong_ordinary ||
					     found != born.end() ||
					     born.count(entry.root_item_uid) ||
					     born.count(entry.parent_item_uid);
			if (related)
			{
				if (found == born.end() ||
				    entry.owner_revision > found->second->owner_revision)
					return false;
				auto expected = entry;
				expected.owner_revision = found->second->owner_revision;
				if (!same(expected, *found->second))
					return false;
			}
		}
		// All caller-side allocations and the complete active owner/link census
		// precede cache effects. Reserve does not publish a SHOP clock.
		decltype(shop_revisions)::node_type staged_shop;
		if (old_shop == shop_revisions.end())
		{
			decltype(shop_revisions) staged;
			staged.emplace(result.shared.shop_id, result.shared.shop_revision_after);
			staged_shop = staged.extract(result.shared.shop_id);
			shop_revisions.reserve(shop_revisions.size() + 1);
		}
		if (!lock.matches(root))
			return false;
		// The existing atomic primitive preflights its own allocations and restores
		// its exact previous rows/counters on allocation refusal. Nonempty stock
		// supplies the actual owner clock in the same batch. Empty stock preserves
		// absence or hydrates genuine present zero/nonzero, without creating defaults.
		if (!item_ownership_runtime_hydrate_many_atomic(batch.data(), batch.size()) ||
		    (batch.empty() && result.shared.owner_after_present &&
		     !item_ownership_runtime_hydrate_owner(keeper,
							   result.shared.owner_revision_after)))
			return false;
		// No callbacks or intervening writer run on the serialized game thread.
		// Full preflight census plus the atomic batch's bounded mutation set forms
		// the complete post-cut census. Read back every original active row and
		// every born row without allocating another snapshot after effects.
		for (const auto &entry : before)
		{
			const auto found = born.find(entry.item_uid);
			const auto &expected = found == born.end() ? entry : *found->second;
			item_ownership_runtime_entry actual{};
			if (!item_ownership_runtime_lookup(entry.item_uid, &actual) ||
			    !same(actual, expected))
				return false;
		}
		for (const auto &entry : batch)
		{
			item_ownership_runtime_entry actual{};
			if (!item_ownership_runtime_lookup(entry.item_uid, &actual) ||
			    !same(actual, entry))
				return false;
		}
		const bool actual_present =
			item_ownership_runtime_peek_owner_revision(keeper, &cached);
		if (actual_present != result.shared.owner_after_present ||
		    (actual_present && cached != result.shared.owner_revision_after) ||
		    item_ownership_runtime_peek_owner_revision(ordinary, &cached) ||
		    !lock.matches(root))
			return false;
		if (staged_shop && !shop_revisions.insert(std::move(staged_shop)).inserted)
			return false;
		const auto actual_shop = shop_revisions.find(result.shared.shop_id);
		return actual_shop != shop_revisions.end() &&
		       actual_shop->second == result.shared.shop_revision_after &&
		       lock.matches(root);
	}
	catch (...)
	{
		// Any refusal after cache effects remains held/unknown at the original
		// owner. Never erase or synthesize state to claim physical publication.
		return false;
	}
}

bool shop_trade_current_runtime_owner::publish_flat(const std::string &root,
						    const flatfile_authority_lock &lock,
						    const critical_command &command,
						    const critical_completion &completion) noexcept
{
	if (!nevent_is_game_thread() || root.empty() || !lock.matches(root) ||
	    !critical_completion_disposition_valid(completion))
		return false;
	try
	{
		economic_frozen_intent intent;
		shop_trade_payload payload{};
		economic_account_key wallet{}, bank{}, counterparty_account{};
		if (shop_trade_accounting_decode(command, &intent, &payload, &wallet, &bank,
						 &counterparty_account) !=
		    economic_accounting_error::ok)
			return false;
		flatfile_accounting_shop_projection current;
		const bool never_admitted = completion.disposition ==
					    critical_completion_disposition::never_admitted;
		if (!critical_operation_id_equal(command.operation_id, completion.operation_id) ||
		    (never_admitted ?
			     flatfile_accounting_shop_transaction::read_never_admitted_before_locked(
				     root, lock, command, &current, nullptr) :
			     flatfile_accounting_shop_transaction::read_current_locked(
				     root, lock, command, completion, &current, nullptr)))
			return false;
		// The pipeline owns the genuine never-admitted decision. This method grants
		// neither a refusal receipt nor permission to release its original hold.
		const bool before = never_admitted ||
				    completion.outcome == critical_apply_outcome::terminal_failure;
		if (!current.keeper.revision)
			return false;
		const item_owner_identity player{ item_owner_type::player, payload.player_pid, 0 };
		const item_owner_identity keeper{ item_owner_type::shopkeeper,
						  item_shopkeeper_owner_id(payload.shop_id), 0 };
		const item_owner_identity primary =
			payload.action == shop_trade_action::discard_invalid ? keeper : player;
		const item_owner_identity counterparty =
			payload.action == shop_trade_action::buy_produced ?
				item_owner_identity{ item_owner_type::system, 0, 0 } :
			(payload.action == shop_trade_action::sell_destroy ||
			 payload.action == shop_trade_action::discard_invalid) ?
				item_owner_identity{ item_owner_type::destruction, 0, 0 } :
				keeper;
		using owner_key = std::tuple<uint8_t, uint64_t, uint64_t>;
		const auto key = [](const item_owner_identity &owner) {
			return owner_key{ static_cast<uint8_t>(owner.type), owner.id,
					  owner.context_id };
		};
		std::map<owner_key, uint64_t> clocks;
		const auto add_clock = [&](const item_owner_identity &owner, uint64_t revision)
		{
			if (!item_owner_identity_valid(owner))
				return false;
			const auto [at, inserted] = clocks.emplace(key(owner), revision);
			uint64_t cached = 0;
			return (inserted || at->second == revision) &&
			       (!item_ownership_runtime_peek_owner_revision(owner, &cached) ||
				cached <= revision);
		};
		if (!add_clock(player, current.player_owner_revision) ||
		    !add_clock(keeper, current.keeper_owner_revision) ||
		    !add_clock(primary, current.primary_owner_revision) ||
		    !add_clock(counterparty, current.counterparty_owner_revision))
			return false;
		std::set<uint64_t> native_pet_uids;
		for (const auto &pet : current.player.pets)
			if (pet.pet_uid && !native_pet_uids.insert(pet.pet_uid).second)
				return false;
		if (native_pet_uids.size() != current.pet_owner_revisions.size())
			return false;
		std::vector<item_owner_identity> complete_owners{ player, keeper };
		for (const auto &pet : current.pet_owner_revisions)
		{
			if (pet.owner.type != item_owner_type::pet ||
			    pet.owner.context_id != payload.player_pid ||
			    !native_pet_uids.erase(pet.owner.id) ||
			    !add_clock(pet.owner, pet.owner_revision))
				return false;
			complete_owners.push_back(pet.owner);
		}
		if (!native_pet_uids.empty())
			return false;
		const auto equal_row = [](const flatfile_item_ownership_record &a,
					  const flatfile_item_ownership_record &b)
		{
			return a.item_uid == b.item_uid && a.root_item_uid == b.root_item_uid &&
			       a.parent_item_uid == b.parent_item_uid &&
			       item_owner_identity_equal(a.owner, b.owner) &&
			       a.item_revision == b.item_revision && a.vnum == b.vnum &&
			       a.state == b.state && a.equipment_slot == b.equipment_slot &&
			       a.coin_payload == b.coin_payload;
		};
		std::unordered_map<uint64_t, const flatfile_item_ownership_record *> expected;
		const auto append = [&](const std::vector<flatfile_item_ownership_record> &rows,
					const item_owner_identity *owner, bool repeated_selected)
		{
			for (const auto &row : rows)
			{
				if (!row.item_uid || !row.root_item_uid || !row.item_revision ||
				    !item_owner_identity_valid(row.owner) || row.vnum < 0 ||
				    (owner && (row.state != item_custody_state::active ||
					       !item_owner_identity_equal(row.owner, *owner))))
					return false;
				const auto [at, inserted] = expected.emplace(row.item_uid, &row);
				if (!inserted &&
				    (!repeated_selected || !equal_row(*at->second, row)))
					return false;
			}
			return true;
		};
		if (!append(current.player_custody, &player, false) ||
		    !append(current.keeper_custody, &keeper, false))
			return false;
		for (const auto &row : current.pet_custody)
		{
			if (row.owner.type != item_owner_type::pet ||
			    row.owner.context_id != payload.player_pid ||
			    !clocks.count(key(row.owner)) ||
			    row.state != item_custody_state::active)
				return false;
		}
		if (!append(current.pet_custody, nullptr, false) ||
		    !append(current.selected_custody, nullptr, true))
			return false;
		std::vector<uint64_t> links;
		links.reserve(expected.size() + payload.item_count * 3 + 3);
		for (const auto &[uid, row] : expected)
		{
			links.push_back(uid);
			links.push_back(row->root_item_uid);
			if (row->parent_item_uid)
				links.push_back(row->parent_item_uid);
		}
		for (size_t n = 0; n < payload.item_count; ++n)
		{
			links.push_back(payload.items[n].item_uid);
			if (payload.items[n].root_item_uid)
				links.push_back(payload.items[n].root_item_uid);
			if (payload.items[n].parent_item_uid)
				links.push_back(payload.items[n].parent_item_uid);
		}
		if (payload.stock_item_uid)
			links.push_back(payload.stock_item_uid);
		if (payload.target_root_item_uid)
			links.push_back(payload.target_root_item_uid);
		if (payload.target_parent_item_uid)
			links.push_back(payload.target_parent_item_uid);
		std::sort(links.begin(), links.end());
		links.erase(std::unique(links.begin(), links.end()), links.end());
		const auto selected = [&](uint64_t uid)
		{ return std::binary_search(links.begin(), links.end(), uid); };
		std::unordered_map<uint64_t, const shop_trade_item_entry *> original_selected;
		for (size_t n = 0; n < payload.item_count; ++n)
			if (!original_selected.emplace(payload.items[n].item_uid, &payload.items[n])
				     .second)
				return false;
		std::vector<flatfile_item_ownership_record> catalog;
		if (flatfile_item_repository_recovery_catalog_locked(
			    root, lock, &catalog, nullptr) != flatfile_item_repository_result::ok)
			return false;
		size_t matched_rows = 0;
		for (const auto &row : catalog)
		{
			const auto found = expected.find(row.item_uid);
			if (found != expected.end())
			{
				if (!equal_row(row, *found->second))
					return false;
				++matched_rows;
			}
			const bool complete_owner = row.owner.type == item_owner_type::player ||
						    row.owner.type == item_owner_type::shopkeeper ||
						    row.owner.type == item_owner_type::pet;
			if (row.state == item_custody_state::active &&
			    ((complete_owner && clocks.count(key(row.owner))) ||
			     selected(row.item_uid) || selected(row.root_item_uid) ||
			     (row.parent_item_uid && selected(row.parent_item_uid))) &&
			    found == expected.end())
				return false;
			if (before && payload.action == shop_trade_action::buy_produced &&
			    original_selected.count(row.item_uid))
				return false;
		}
		if (matched_rows != expected.size())
			return false;
		const item_owner_identity original_owner = shop_owned(payload.action) ? keeper :
											player;
		const auto cache_supported = [&](const item_ownership_runtime_entry &cached)
		{
			const auto found = expected.find(cached.item_uid);
			if (found == expected.end())
				return false;
			const auto &row = *found->second;
			const auto owner_clock = clocks.find(key(cached.owner));
			if (owner_clock == clocks.end() ||
			    cached.owner_revision > owner_clock->second)
				return false;
			if (cached.root_item_uid == row.root_item_uid &&
			    cached.parent_item_uid == row.parent_item_uid &&
			    item_owner_identity_equal(cached.owner, row.owner) &&
			    cached.item_revision == row.item_revision && cached.vnum == row.vnum &&
			    cached.state == row.state)
				return true;
			// Only the original selected BEFORE witness authorizes a changed item.
			// Stock/target ancestors and unrelated native rows must match CURRENT exactly.
			const auto original = original_selected.find(cached.item_uid);
			if (before || payload.action == shop_trade_action::buy_produced ||
			    original == original_selected.end())
				return false;
			const auto &witness = *original->second;
			return cached.root_item_uid == witness.root_item_uid &&
			       cached.parent_item_uid == witness.parent_item_uid &&
			       item_owner_identity_equal(cached.owner, original_owner) &&
			       cached.item_revision == witness.expected_item_revision &&
			       cached.vnum == witness.vnum &&
			       cached.state == witness.expected_state;
		};
		const size_t cache_limit = std::max<size_t>(1, item_ownership_runtime_size());
		// Keep the observer's original per-call UID bound. The complete union may
		// include retained selected history in addition to two maximum native forests.
		for (size_t offset = 0; offset < links.size(); offset += PLAYER_SNAPSHOT_MAX_ROWS)
		{
			const size_t count =
				std::min(PLAYER_SNAPSHOT_MAX_ROWS, links.size() - offset);
			std::vector<item_ownership_runtime_entry> observed;
			if (!item_ownership_runtime_published_native_observer::snapshot_links(
				    std::span<const uint64_t>(links.data() + offset, count),
				    cache_limit, &observed))
				return false;
			for (const auto &row : observed)
				if (!cache_supported(row))
					return false;
		}
		for (const auto &owner : complete_owners)
		{
			std::vector<item_ownership_runtime_entry> observed;
			if (!item_ownership_runtime_snapshot_owner(owner, cache_limit, &observed))
				return false;
			for (const auto &row : observed)
				if (row.state == item_custody_state::active &&
				    !cache_supported(row))
					return false;
		}
		if (before && payload.action == shop_trade_action::buy_produced)
			for (size_t n = 0; n < payload.item_count; ++n)
			{
				item_ownership_runtime_entry existing{};
				if (item_ownership_runtime_lookup(payload.items[n].item_uid,
								  &existing))
					return false;
			}
		std::vector<item_ownership_runtime_entry> batch;
		batch.reserve(expected.size());
		for (const auto &[uid, row] : expected)
		{
			const auto clock = clocks.find(key(row->owner));
			if (clock == clocks.end())
			{
				if (row->state == item_custody_state::active)
					return false;
				item_ownership_runtime_entry existing{};
				if (item_ownership_runtime_lookup(uid, &existing) &&
				    (existing.root_item_uid != row->root_item_uid ||
				     existing.parent_item_uid != row->parent_item_uid ||
				     !item_owner_identity_equal(existing.owner, row->owner) ||
				     existing.item_revision != row->item_revision ||
				     existing.vnum != row->vnum || existing.state != row->state))
					return false;
				continue;
			}
			item_ownership_runtime_entry existing{};
			if (item_ownership_runtime_lookup(uid, &existing) &&
			    !cache_supported(existing))
				return false;
			batch.push_back({ uid, row->root_item_uid, row->parent_item_uid, row->owner,
					  row->item_revision, clock->second, row->vnum,
					  row->state });
		}
		const auto old_shop = shop_revisions.find(payload.shop_id);
		if (old_shop != shop_revisions.end() && old_shop->second > current.keeper.revision)
			return false;
		decltype(shop_revisions)::node_type staged_shop;
		if (old_shop == shop_revisions.end())
		{
			decltype(shop_revisions) staged;
			staged.emplace(payload.shop_id, current.keeper.revision);
			staged_shop = staged.extract(payload.shop_id);
			shop_revisions.reserve(shop_revisions.size() + 1);
		}
		if (!lock.matches(root) ||
		    !item_ownership_runtime_hydrate_many_atomic(batch.data(), batch.size()))
			return false;
		for (const auto &owner : complete_owners)
			if (!item_ownership_runtime_hydrate_owner(owner, clocks.at(key(owner))))
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
		for (const auto &owner : complete_owners)
		{
			uint64_t revision = 0;
			if (!item_ownership_runtime_peek_owner_revision(owner, &revision) ||
			    revision != clocks.at(key(owner)))
				return false;
		}
		if (!lock.matches(root))
			return false;
		if (staged_shop)
			return shop_revisions.insert(std::move(staged_shop)).inserted;
		shop_revisions.find(payload.shop_id)->second = current.keeper.revision;
		return true;
	}
	catch (...)
	{
		return false;
	}
}
