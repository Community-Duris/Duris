#include "flatfile/flatfile_corpse_ownership.h"

#include <limits>
#include <new>
#include <unordered_map>
#include <utility>

item_owner_identity flatfile_corpse_item_owner(uint32_t owner_pid, uint32_t save_id)
{
	return { item_owner_type::corpse, item_corpse_owner_id(owner_pid, save_id), 0 };
}

flatfile_corpse_ownership_result
flatfile_world_reconcile_item_ownership(const std::vector<player_item_snapshot> &items,
					const item_owner_identity &owner, uint64_t owner_revision,
					const std::vector<flatfile_item_ownership_record> &custody,
					std::vector<player_load_item_identity> *identities)
{
	if (!identities || !item_owner_identity_valid(owner) || !owner_revision ||
	    items.size() != custody.size() ||
	    items.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
		return flatfile_corpse_ownership_result::invalid;
	std::unordered_map<uint64_t, const flatfile_item_ownership_record *> by_uid;
	std::vector<player_load_item_identity> reconciled;
	try
	{
		by_uid.reserve(custody.size());
		reconciled.reserve(items.size());
		for (const auto &entry : custody)
			if (!entry.item_uid || entry.state != item_custody_state::active ||
			    !item_owner_identity_equal(entry.owner, owner) ||
			    !by_uid.emplace(entry.item_uid, &entry).second)
				return flatfile_corpse_ownership_result::invalid;
		for (size_t index = 0; index < items.size(); ++index)
		{
			const player_item_snapshot &item = items[index];
			const auto found = by_uid.find(item.object_uid);
			if (!item.object_uid || item.equipment_slot != -1 ||
			    found == by_uid.end() || found->second->vnum != item.vnum)
				return flatfile_corpse_ownership_result::invalid;
			uint64_t parent_uid = 0;
			uint64_t root_uid = item.object_uid;
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index)
					return flatfile_corpse_ownership_result::invalid;
				const auto &parent =
					reconciled[static_cast<size_t>(item.parent_index)];
				parent_uid = parent.item_uid;
				root_uid = parent.root_item_uid;
			}
			const flatfile_item_ownership_record &entry = *found->second;
			if (entry.root_item_uid != root_uid ||
			    entry.parent_item_uid != parent_uid || !entry.item_revision)
				return flatfile_corpse_ownership_result::invalid;
			player_load_item_identity identity = {};
			identity.database_id = index + 1;
			identity.serialized_parent_id =
				item.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
					0 :
					static_cast<uint64_t>(item.parent_index) + 1;
			identity.quantity = 1;
			identity.override_mask = PLAYER_LOAD_ITEM_OVERRIDE_ALL;
			identity.item_uid = entry.item_uid;
			identity.root_item_uid = entry.root_item_uid;
			identity.parent_item_uid = entry.parent_item_uid;
			identity.owner = owner;
			identity.item_revision = entry.item_revision;
			identity.owner_revision = owner_revision;
			identity.state = entry.state;
			reconciled.push_back(identity);
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_corpse_ownership_result::io_error;
	}
	*identities = std::move(reconciled);
	return flatfile_corpse_ownership_result::ok;
}

flatfile_corpse_ownership_result
flatfile_corpse_reconcile_item_ownership(const flatfile_corpse_record &record,
					 uint64_t owner_revision,
					 const std::vector<flatfile_item_ownership_record> &custody,
					 std::vector<player_load_item_identity> *identities)
{
	if (!record.revision)
		return flatfile_corpse_ownership_result::invalid;
	return flatfile_world_reconcile_item_ownership(
		record.items, flatfile_corpse_item_owner(record.owner_pid, record.save_id),
		owner_revision, custody, identities);
}

flatfile_corpse_ownership_result flatfile_corpse_load_item_ownership(
	const std::string &root, const flatfile_corpse_record &record, uint64_t *owner_revision,
	std::vector<player_load_item_identity> *identities, std::string *error)
{
	if (!owner_revision || !identities)
		return flatfile_corpse_ownership_result::invalid;
	uint64_t loaded_revision = 0;
	std::vector<flatfile_item_ownership_record> custody;
	const auto loaded = flatfile_item_repository_load_owner(
		root, flatfile_corpse_item_owner(record.owner_pid, record.save_id),
		&loaded_revision, &custody, error);
	if (loaded == flatfile_item_repository_result::not_found && record.items.empty())
	{
		*owner_revision = 0;
		identities->clear();
		return flatfile_corpse_ownership_result::ok;
	}
	if (loaded != flatfile_item_repository_result::ok)
		return loaded == flatfile_item_repository_result::not_found ?
			       flatfile_corpse_ownership_result::not_found :
		       loaded == flatfile_item_repository_result::io_error ?
			       flatfile_corpse_ownership_result::io_error :
			       flatfile_corpse_ownership_result::invalid;
	if (record.items.empty())
	{
		if (!custody.empty() || !loaded_revision)
			return flatfile_corpse_ownership_result::invalid;
		*owner_revision = loaded_revision;
		identities->clear();
		return flatfile_corpse_ownership_result::ok;
	}
	const auto reconciled = flatfile_corpse_reconcile_item_ownership(record, loaded_revision,
									 custody, identities);
	if (reconciled == flatfile_corpse_ownership_result::ok)
		*owner_revision = loaded_revision;
	return reconciled;
}

flatfile_corpse_ownership_result flatfile_room_load_item_ownership(
	const std::string &root, const flatfile_room_item_record &record, uint64_t *owner_revision,
	std::vector<player_load_item_identity> *identities, std::string *error)
{
	if (!owner_revision || !identities || record.room_vnum <= 0 || !record.revision)
		return flatfile_corpse_ownership_result::invalid;
	const item_owner_identity owner = { item_owner_type::room,
					    static_cast<uint64_t>(record.room_vnum), 0 };
	uint64_t loaded_revision = 0;
	std::vector<flatfile_item_ownership_record> custody;
	const auto loaded =
		flatfile_item_repository_load_owner(root, owner, &loaded_revision, &custody, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded == flatfile_item_repository_result::not_found ?
			       flatfile_corpse_ownership_result::not_found :
		       loaded == flatfile_item_repository_result::io_error ?
			       flatfile_corpse_ownership_result::io_error :
			       flatfile_corpse_ownership_result::invalid;
	if (record.items.empty())
	{
		if (!custody.empty() || !loaded_revision)
			return flatfile_corpse_ownership_result::invalid;
		*owner_revision = loaded_revision;
		identities->clear();
		return flatfile_corpse_ownership_result::ok;
	}
	const auto reconciled = flatfile_world_reconcile_item_ownership(
		record.items, owner, loaded_revision, custody, identities);
	if (reconciled == flatfile_corpse_ownership_result::ok)
		*owner_revision = loaded_revision;
	return reconciled;
}
