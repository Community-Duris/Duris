#include "flatfile/flatfile_native_mobile_birth_ordinary_physical.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "flatfile/flatfile_locker_repository.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "economy/native_mobile_birth_cash_role_command.h"

#include <algorithm>
#include <cerrno>
#include <new>

unsigned int
flatfile_native_mobile_birth_ordinary_physical_storage::verify_catalog_namespaces_locked(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &authority_lock,
	const critical_native_recovery_envelope &original,
	flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence *output,
	std::string *error) noexcept
{
	if (root.empty() || !output || !identity_lock.matches(root) ||
	    !authority_lock.matches(root))
		return EINVAL;
	try
	{
		if (!native_mobile_birth_cash_role_recovery_valid(original))
			return EINVAL;
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		economic_frozen_intent intent;
		if (native_mobile_birth_cash_role_command_decode(original.command, &image, &recipes,
								 &role) !=
			    economic_accounting_error::ok ||
		    role.role != native_mobile_birth_cash_role::ordinary_wallet ||
		    economic_intent_decode(original.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(original.command, intent) !=
			    economic_accounting_error::ok ||
		    !intent.admission.metadata.source_event)
			return EINVAL;
		std::vector<uint64_t> born;
		born.reserve(image.items.size());
		for (const auto &literal : image.items)
			born.push_back(literal.object_uid);
		std::sort(born.begin(), born.end());
		if (std::adjacent_find(born.begin(), born.end()) != born.end() ||
		    (!born.empty() && !born.front()))
			return EINVAL;
		flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence observed;
		auto status =
			flatfile_native_mobile_birth_ordinary_player_physical_storage::verify_locked(
				root, identity_lock, authority_lock, original, &observed.players,
				error);
		if (status)
			return status;
		status =
			flatfile_native_mobile_birth_ordinary_auction_physical_storage::verify_locked(
				root, authority_lock, original, &observed.auction, error);
		if (status)
			return status;
		status = flatfile_native_mobile_birth_ordinary_collector_physical_storage::
			verify_locked(root, authority_lock, original, &observed.collector, error);
		if (status)
			return status;

		{
			std::vector<flatfile_corpse_record> corpses;
			std::vector<flatfile_room_item_record> rooms;
			std::vector<flatfile_saved_world_item_record> saved;
			const auto loaded = flatfile_world_item_recovery_list_all_locked(
				root, authority_lock, &corpses, &rooms, &saved, error);
			if (loaded != flatfile_world_item_result::ok &&
			    loaded != flatfile_world_item_result::not_found)
				return loaded == flatfile_world_item_result::io_error ? EIO :
											EILSEQ;
			observed.corpses = corpses.size();
			observed.rooms = rooms.size();
			observed.saved_world_records = saved.size();
			for (const auto &corpse : corpses)
				for (const auto &item : corpse.items)
				{
					if (std::binary_search(born.begin(), born.end(),
							       item.object_uid))
						return EEXIST;
					++observed.world_item_rows;
				}
			for (const auto &room : rooms)
				for (const auto &item : room.items)
				{
					if (std::binary_search(born.begin(), born.end(),
							       item.object_uid))
						return EEXIST;
					++observed.world_item_rows;
				}
			for (const auto &record : saved)
				for (const auto &item : record.items)
				{
					if (std::binary_search(born.begin(), born.end(),
							       item.object_uid))
						return EEXIST;
					++observed.world_item_rows;
				}
		}
		{
			std::vector<flatfile_locker_record> lockers;
			const auto loaded = flatfile_locker_recovery_list_locked(
				root, authority_lock, &lockers, error);
			if (loaded != flatfile_locker_result::ok &&
			    loaded != flatfile_locker_result::not_found)
				return loaded == flatfile_locker_result::io_error ? EIO : EILSEQ;
			observed.lockers = lockers.size();
			for (const auto &locker : lockers)
				for (const auto &chest : locker.chests)
				{
					++observed.locker_chests;
					for (const auto &item : chest.items)
					{
						if (std::binary_search(born.begin(), born.end(),
								       item.object_uid))
							return EEXIST;
						++observed.locker_item_rows;
					}
				}
		}
		{
			std::vector<flatfile_shopkeeper_record> keepers;
			const auto loaded = flatfile_shopkeeper_list_locked(root, authority_lock,
									    &keepers, error);
			if (loaded != flatfile_shopkeeper_result::ok &&
			    loaded != flatfile_shopkeeper_result::not_found)
				return loaded == flatfile_shopkeeper_result::io_error ? EIO :
											EILSEQ;
			observed.shopkeepers = keepers.size();
			for (const auto &keeper : keepers)
				for (const auto &item : keeper.items)
				{
					if (std::binary_search(born.begin(), born.end(),
							       item.object_uid))
						return EEXIST;
					++observed.shop_item_rows;
				}
		}
		if (!identity_lock.matches(root) || !authority_lock.matches(root))
			return EINVAL;
		// The genuine caller separately joins all remaining original namespaces,
		// custody UID/root/parent scopes, source/history and the atomic root bundle.
		*output = observed;
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}
