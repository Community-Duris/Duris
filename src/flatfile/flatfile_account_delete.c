#include "flatfile/flatfile_account_delete.h"

#include "flatfile/flatfile_account_repository.h"
#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_character_delete.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_locker_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"

#include <new>
#include <utility>
#include <vector>

namespace
{
constexpr int8_t account_deletion_fence = 2;

flatfile_account_delete_result map_account(flatfile_account_result result)
{
	switch (result)
	{
	case flatfile_account_result::not_found:
		return flatfile_account_delete_result::not_found;
	case flatfile_account_result::conflict:
		return flatfile_account_delete_result::conflict;
	case flatfile_account_result::io_error:
		return flatfile_account_delete_result::io_error;
	default:
		return flatfile_account_delete_result::invalid;
	}
}

flatfile_account_delete_result map_identity(flatfile_identity_result result)
{
	if (result == flatfile_identity_result::conflict)
		return flatfile_account_delete_result::conflict;
	if (result == flatfile_identity_result::io_error)
		return flatfile_account_delete_result::io_error;
	return flatfile_account_delete_result::invalid;
}

flatfile_account_delete_result map_character(flatfile_character_delete_result result)
{
	if (result == flatfile_character_delete_result::not_found)
		return flatfile_account_delete_result::not_found;
	if (result == flatfile_character_delete_result::conflict)
		return flatfile_account_delete_result::conflict;
	if (result == flatfile_character_delete_result::io_error)
		return flatfile_account_delete_result::io_error;
	return flatfile_account_delete_result::invalid;
}

flatfile_account_delete_result map_domain(flatfile_player_domain_result result)
{
	if (result == flatfile_player_domain_result::not_found)
		return flatfile_account_delete_result::not_found;
	if (result == flatfile_player_domain_result::conflict)
		return flatfile_account_delete_result::conflict;
	if (result == flatfile_player_domain_result::io_error)
		return flatfile_account_delete_result::io_error;
	return flatfile_account_delete_result::invalid;
}

flatfile_account_delete_result map_locker(flatfile_locker_result result)
{
	if (result == flatfile_locker_result::not_found)
		return flatfile_account_delete_result::not_found;
	if (result == flatfile_locker_result::conflict)
		return flatfile_account_delete_result::conflict;
	if (result == flatfile_locker_result::io_error)
		return flatfile_account_delete_result::io_error;
	return flatfile_account_delete_result::invalid;
}

flatfile_account_delete_result map_item(flatfile_item_repository_result result)
{
	if (result == flatfile_item_repository_result::not_found)
		return flatfile_account_delete_result::not_found;
	if (result == flatfile_item_repository_result::io_error)
		return flatfile_account_delete_result::io_error;
	return flatfile_account_delete_result::invalid;
}
} // namespace

flatfile_account_delete_result flatfile_account_delete(const std::string &root,
						       const std::string &account_name,
						       std::string *error)
{
	if (error)
		error->clear();
	if (root.empty() || account_name.empty())
		return flatfile_account_delete_result::invalid;

	/* Finish a previously published character/finalization journal before
	 * deciding whether this is a new request or an idempotent retry. */
	{
		flatfile_authority_lock authority_lock;
		if (!authority_lock.acquire(root, error))
			return flatfile_account_delete_result::io_error;
		const auto recovered =
			flatfile_authority_transaction_recover(root, authority_lock, error);
		if (recovered != flatfile_authority_transaction_result::ok)
			return recovered == flatfile_authority_transaction_result::io_error ?
				       flatfile_account_delete_result::io_error :
				       flatfile_account_delete_result::invalid;
	}

	flatfile_account_record account;
	const auto loaded = flatfile_account_load(root, account_name, &account, error);
	if (loaded == flatfile_account_result::not_found)
	{
		std::vector<flatfile_identity_record> remaining;
		const auto identities =
			flatfile_identity_list_account(root, account_name, &remaining, error);
		if (identities != flatfile_identity_result::ok)
			return map_identity(identities);
		return remaining.empty() ? flatfile_account_delete_result::already_deleted :
					   flatfile_account_delete_result::invalid;
	}
	if (loaded != flatfile_account_result::ok)
		return map_account(loaded);
	if (account.blocked != account_deletion_fence)
		return flatfile_account_delete_result::conflict;

	std::vector<flatfile_identity_record> characters;
	const auto listed = flatfile_identity_list_account(root, account_name, &characters, error);
	if (listed != flatfile_identity_result::ok)
		return map_identity(listed);
	for (const auto &character : characters)
	{
		const auto deleted =
			flatfile_character_delete(root, character.pid, character.name, error);
		if (deleted != flatfile_character_delete_result::ok &&
		    deleted != flatfile_character_delete_result::already_deleted)
			return map_character(deleted);
	}

	/* Account membership saves acquire locks in this order. Holding all three
	 * prevents a new membership revision from racing credential removal. */
	{
		flatfile_identity_lock identity_lock;
		if (!identity_lock.acquire(root, error))
			return flatfile_account_delete_result::io_error;
		flatfile_authority_lock authority_lock;
		if (!authority_lock.acquire(root, error))
			return flatfile_account_delete_result::io_error;
		flatfile_account_lock account_lock;
		if (!account_lock.acquire(root, error))
			return flatfile_account_delete_result::io_error;

		std::vector<flatfile_authority_operation> operations;
		const auto banks = flatfile_player_domain_prepare_account_remove(
			root, authority_lock, account_name, &operations, error);
		if (banks != flatfile_player_domain_result::ok)
			return map_domain(banks);
		try
		{
			flatfile_locker_player_removal locker_removal;
			const auto locker = flatfile_locker_prepare_account_remove(
				root, authority_lock, account_name, &locker_removal, error);
			if (locker == flatfile_locker_result::ok)
			{
				flatfile_authority_operation item_operation;
				const auto item = flatfile_item_repository_prepare_locker_remove(
					root, authority_lock, locker_removal.custody,
					&item_operation, error);
				if (item == flatfile_item_repository_result::ok)
					operations.push_back(std::move(item_operation));
				else if (item != flatfile_item_repository_result::unchanged)
					return map_item(item);
				operations.push_back(std::move(locker_removal.operation));
			}
			else if (locker != flatfile_locker_result::unchanged)
			{
				return map_locker(locker);
			}

			flatfile_authority_operation identity_operation;
			const auto identities = flatfile_identity_prepare_sync_account(
				root, identity_lock, authority_lock, account_name, {},
				&identity_operation, error);
			if (identities != flatfile_identity_result::ok)
				return map_identity(identities);
			operations.push_back(std::move(identity_operation));

			/* Credential/account removal is deliberately the final operation. */
			flatfile_authority_operation account_operation;
			const auto prepared = flatfile_account_prepare_remove(
				root, account_lock, authority_lock, account_name, account.revision,
				&account_operation, error);
			if (prepared != flatfile_account_result::ok)
				return map_account(prepared);
			operations.push_back(std::move(account_operation));
		}
		catch (const std::bad_alloc &)
		{
			return flatfile_account_delete_result::io_error;
		}

		const auto committed = flatfile_authority_transaction_commit_operations(
			root, authority_lock, operations, error);
		if (committed != flatfile_authority_transaction_result::ok)
			return committed == flatfile_authority_transaction_result::io_error ?
				       flatfile_account_delete_result::io_error :
				       flatfile_account_delete_result::invalid;
	}

	bool exists = true;
	if (flatfile_account_exists(root, account_name, &exists, error) !=
		    flatfile_account_result::ok ||
	    exists)
		return flatfile_account_delete_result::io_error;
	std::vector<flatfile_identity_record> remaining;
	const auto identities =
		flatfile_identity_list_account(root, account_name, &remaining, error);
	if (identities != flatfile_identity_result::ok)
		return map_identity(identities);
	if (!remaining.empty())
		return flatfile_account_delete_result::conflict;
	return flatfile_account_delete_result::ok;
}
