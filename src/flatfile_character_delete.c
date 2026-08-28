#include "flatfile_character_delete.h"

#include "flatfile_authority_transaction.h"
#include "flatfile_boon_repository.h"
#include "flatfile_identity_repository.h"
#include "flatfile_item_repository.h"
#include "flatfile_player_domain_repository.h"
#include "flatfile_player_repository.h"
#include "flatfile_recipe_repository.h"
#include "flatfile_spellbook_repository.h"

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
		operations.reserve(7);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_character_delete_result::io_error;
	}

	flatfile_authority_operation operation;
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

	const auto item = flatfile_item_repository_prepare_player_remove(
		root, authority_lock, static_cast<uint32_t>(pid), &operation, error);
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
