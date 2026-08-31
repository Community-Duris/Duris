#include "flatfile/flatfile_character_delete.h"

#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_account_reward_summon_repository.h"
#include "flatfile/flatfile_artifact_repository.h"
#include "flatfile/flatfile_association_repository.h"
#include "flatfile/flatfile_auction_repository.h"
#include "flatfile/flatfile_boon_repository.h"
#include "flatfile/flatfile_frag_leaderboard_repository.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_locker_repository.h"
#include "flatfile/flatfile_offline_message_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_player_repository.h"
#include "flatfile/flatfile_recipe_repository.h"
#include "flatfile/flatfile_ship_repository.h"
#include "flatfile/flatfile_spellbook_repository.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "flatfile/flatfile_world_quest_history.h"

#include <ctime>
#include <new>
#include <vector>

namespace
{
void set_error(std::string *error, const char *message)
{
	if (error && error->empty())
		*error = message;
}

flatfile_character_delete_result map_identity(flatfile_identity_result result)
{
	switch (result)
	{
	case flatfile_identity_result::not_found:
		return flatfile_character_delete_result::not_found;
	case flatfile_identity_result::conflict:
		return flatfile_character_delete_result::conflict;
	case flatfile_identity_result::io_error:
		return flatfile_character_delete_result::io_error;
	default:
		return flatfile_character_delete_result::invalid;
	}
}

template <typename Result>
flatfile_character_delete_result map_authority(Result result, Result not_found, Result io_error)
{
	if (result == not_found)
		return flatfile_character_delete_result::not_found;
	if (result == io_error)
		return flatfile_character_delete_result::io_error;
	return flatfile_character_delete_result::invalid;
}

/*
 * Maximum authority operations one character deletion can stage: one per
 * append_operation() call site below (account reward summon, artifact,
 * frag leaderboard, association, ship, player snapshot, player domain,
 * world quest, item repository, locker removal, world-item removal, shop
 * trade materialization, boon, recipe, spellbook, offline message, identity).
 * Keep this in step with the call sites so the transaction encoder can never
 * reject a fully populated character.
 */
constexpr size_t character_delete_maximum_operations = 17;
static_assert(character_delete_maximum_operations <=
		      flatfile_authority_transaction_maximum_operations,
	      "character deletion can stage more operations than one authority transaction holds");

bool append_operation(std::vector<flatfile_authority_operation> *operations,
		      flatfile_authority_operation *operation)
{
	try
	{
		operations->push_back(std::move(*operation));
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
} // namespace

flatfile_character_delete_result flatfile_character_delete(const std::string &root, int32_t pid,
							   const std::string &expected_name,
							   std::string *error)
{
	if (error)
		error->clear();
	if (root.empty() || pid <= 0 || expected_name.empty())
		return flatfile_character_delete_result::invalid;

	flatfile_player_snapshot_lock snapshot_lock;
	if (!snapshot_lock.acquire(root, pid, error))
		return flatfile_character_delete_result::io_error;
	flatfile_identity_lock identity_lock;
	if (!identity_lock.acquire(root, error))
		return flatfile_character_delete_result::io_error;
	flatfile_authority_lock authority_lock;
	if (!authority_lock.acquire(root, error))
		return flatfile_character_delete_result::io_error;

	flatfile_authority_operation identity_operation;
	const auto identity = flatfile_identity_prepare_remove(root, identity_lock, authority_lock,
							       pid, expected_name,
							       &identity_operation, error);
	const bool retry = identity == flatfile_identity_result::unchanged;
	if (identity != flatfile_identity_result::ok && !retry)
		return map_identity(identity);

	std::vector<flatfile_authority_operation> operations;
	try
	{
		operations.reserve(character_delete_maximum_operations);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_character_delete_result::io_error;
	}

	flatfile_authority_operation operation;
	const auto auction = flatfile_auction_check_player_unreferenced(
		root, authority_lock, static_cast<uint32_t>(pid), error);
	if (auction == flatfile_auction_player_reference_result::referenced)
		return flatfile_character_delete_result::conflict;
	if (auction != flatfile_auction_player_reference_result::clear)
		return auction == flatfile_auction_player_reference_result::io_error ?
			       flatfile_character_delete_result::io_error :
			       flatfile_character_delete_result::invalid;

	const auto reward_summon = flatfile_account_reward_summon_prepare_player_remove(
		root, authority_lock, static_cast<uint32_t>(pid), &operation, error);
	if (reward_summon == flatfile_account_reward_summon_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (reward_summon != flatfile_account_reward_summon_result::unchanged)
	{
		return map_authority(reward_summon,
				     flatfile_account_reward_summon_result::not_found,
				     flatfile_account_reward_summon_result::io_error);
	}

	flatfile_world_item_player_removal world_item_removal;
	const auto world_item = flatfile_world_item_prepare_player_remove(
		root, authority_lock, static_cast<uint32_t>(pid), expected_name,
		&world_item_removal, error);
	const bool world_item_changed = world_item == flatfile_world_item_result::ok;
	if (world_item == flatfile_world_item_result::conflict)
		return flatfile_character_delete_result::conflict;
	if (!world_item_changed && world_item != flatfile_world_item_result::unchanged)
		return map_authority(world_item, flatfile_world_item_result::not_found,
				     flatfile_world_item_result::io_error);

	const auto artifact = flatfile_artifact_prepare_player_release(
		root, authority_lock, static_cast<uint32_t>(pid), &operation, error);
	if (artifact == flatfile_artifact_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (artifact == flatfile_artifact_result::conflict)
	{
		return flatfile_character_delete_result::conflict;
	}
	else if (artifact != flatfile_artifact_result::unchanged)
	{
		return map_authority(artifact, flatfile_artifact_result::not_found,
				     flatfile_artifact_result::io_error);
	}

	const auto leaderboard = flatfile_frag_leaderboard_prepare_tombstone(
		root, authority_lock, static_cast<uint32_t>(pid),
		static_cast<int64_t>(time(nullptr)), &operation, error);
	if (leaderboard == flatfile_frag_leaderboard_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (leaderboard != flatfile_frag_leaderboard_result::unchanged)
	{
		return map_authority(leaderboard, flatfile_frag_leaderboard_result::not_found,
				     flatfile_frag_leaderboard_result::io_error);
	}

	flatfile_locker_player_removal locker_removal;
	const auto locker = flatfile_locker_prepare_player_remove(root, authority_lock,
								  static_cast<uint32_t>(pid),
								  expected_name, &locker_removal,
								  error);
	const bool locker_changed = locker == flatfile_locker_result::ok;
	if (locker == flatfile_locker_result::conflict)
		return flatfile_character_delete_result::conflict;
	if (!locker_changed && locker != flatfile_locker_result::unchanged)
		return map_authority(locker, flatfile_locker_result::not_found,
				     flatfile_locker_result::io_error);

	const auto association = flatfile_association_prepare_player_remove(
		root, authority_lock, static_cast<uint32_t>(pid), expected_name, &operation, error);
	if (association == flatfile_association_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (association == flatfile_association_result::conflict)
	{
		return flatfile_character_delete_result::conflict;
	}
	else if (association != flatfile_association_result::unchanged)
	{
		return map_authority(association, flatfile_association_result::not_found,
				     flatfile_association_result::io_error);
	}

	const auto ship = flatfile_ship_prepare_player_remove(
		root, authority_lock, static_cast<uint32_t>(pid), expected_name, &operation, error);
	if (ship == flatfile_ship_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (ship == flatfile_ship_result::conflict)
	{
		return flatfile_character_delete_result::conflict;
	}
	else if (ship != flatfile_ship_result::unchanged)
	{
		return map_authority(ship, flatfile_ship_result::not_found,
				     flatfile_ship_result::io_error);
	}

	const auto snapshot = flatfile_player_snapshot_prepare_remove(
		root, snapshot_lock, authority_lock, pid, &operation, error);
	if (snapshot == flatfile_player_load_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (!(retry && snapshot == flatfile_player_load_result::not_found))
	{
		return map_authority(snapshot, flatfile_player_load_result::not_found,
				     flatfile_player_load_result::io_error);
	}

	const auto domain = flatfile_player_domain_prepare_remove(
		root, authority_lock, static_cast<uint32_t>(pid), &operation, error);
	if (domain == flatfile_player_domain_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (!(retry && domain == flatfile_player_domain_result::not_found))
	{
		return map_authority(domain, flatfile_player_domain_result::not_found,
				     flatfile_player_domain_result::io_error);
	}

	const auto world_quest = flatfile_world_quest_prepare_remove(
		root, authority_lock, static_cast<uint32_t>(pid), &operation, error);
	if (world_quest == flatfile_world_quest_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (world_quest != flatfile_world_quest_result::not_found)
	{
		return map_authority(world_quest, flatfile_world_quest_result::not_found,
				     flatfile_world_quest_result::io_error);
	}

	const auto item = locker_changed || world_item_changed ?
				  flatfile_item_repository_prepare_player_and_custody_remove(
					  root, authority_lock, static_cast<uint32_t>(pid),
					  locker_removal.custody, world_item_removal.custody,
					  &operation, error) :
				  flatfile_item_repository_prepare_player_remove(
					  root, authority_lock, static_cast<uint32_t>(pid),
					  &operation, error);
	if (item == flatfile_item_repository_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (item != flatfile_item_repository_result::unchanged)
	{
		return map_authority(item, flatfile_item_repository_result::not_found,
				     flatfile_item_repository_result::io_error);
	}
	if (locker_changed && !append_operation(&operations, &locker_removal.operation))
		return flatfile_character_delete_result::io_error;
	if (world_item_changed && !append_operation(&operations, &world_item_removal.operation))
		return flatfile_character_delete_result::io_error;

	const auto shop_materialization = flatfile_shop_trade_materialization_prepare_player_remove(
		root, authority_lock, static_cast<uint32_t>(pid), &operation, error);
	if (shop_materialization == flatfile_shop_trade_materialization_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (shop_materialization != flatfile_shop_trade_materialization_result::unchanged)
	{
		return shop_materialization ==
				       flatfile_shop_trade_materialization_result::io_error ?
			       flatfile_character_delete_result::io_error :
			       flatfile_character_delete_result::invalid;
	}

	const auto boon = flatfile_boon_prepare_player_remove(
		root, authority_lock, static_cast<uint32_t>(pid), &operation, error);
	if (boon == flatfile_boon_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (boon != flatfile_boon_result::unchanged)
	{
		return map_authority(boon, flatfile_boon_result::not_found,
				     flatfile_boon_result::io_error);
	}

	const auto recipe = flatfile_recipe_prepare_clear(
		root, authority_lock, static_cast<uint32_t>(pid), &operation, error);
	if (recipe == flatfile_recipe_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (recipe != flatfile_recipe_result::unchanged)
	{
		return map_authority(recipe, flatfile_recipe_result::not_found,
				     flatfile_recipe_result::io_error);
	}

	const auto spellbook = flatfile_spellbook_prepare_clear(
		root, authority_lock, static_cast<uint32_t>(pid), &operation, error);
	if (spellbook == flatfile_spellbook_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (spellbook != flatfile_spellbook_result::unchanged)
	{
		return map_authority(spellbook, flatfile_spellbook_result::not_found,
				     flatfile_spellbook_result::io_error);
	}

	const auto offline = flatfile_offline_message_prepare_remove(
		root, authority_lock, static_cast<uint32_t>(pid), &operation, error);
	if (offline == flatfile_offline_message_result::ok)
	{
		if (!append_operation(&operations, &operation))
			return flatfile_character_delete_result::io_error;
	}
	else if (offline != flatfile_offline_message_result::unchanged)
	{
		return map_authority(offline, flatfile_offline_message_result::not_found,
				     flatfile_offline_message_result::io_error);
	}

	if (!retry && !append_operation(&operations, &identity_operation))
		return flatfile_character_delete_result::io_error;
	if (operations.empty())
		return flatfile_character_delete_result::already_deleted;
	if (retry)
	{
		set_error(error, "tombstoned identity still has character authority");
		return flatfile_character_delete_result::invalid;
	}

	const auto committed = flatfile_authority_transaction_commit_operations(
		root, authority_lock, operations, error);
	if (committed == flatfile_authority_transaction_result::ok)
		return flatfile_character_delete_result::ok;
	return committed == flatfile_authority_transaction_result::io_error ?
		       flatfile_character_delete_result::io_error :
		       flatfile_character_delete_result::invalid;
}
