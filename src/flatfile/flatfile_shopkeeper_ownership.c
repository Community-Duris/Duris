#include "flatfile/flatfile_shopkeeper_ownership.h"

#include <limits>
#include <new>
#include <unordered_map>
#include <utility>

item_owner_identity flatfile_shopkeeper_item_owner(uint32_t shop_id)
{
	return { item_owner_type::shopkeeper, item_shopkeeper_owner_id(shop_id), 0 };
}

flatfile_shopkeeper_ownership_result flatfile_shopkeeper_reconcile_item_ownership(
	const flatfile_shopkeeper_record &record, uint64_t owner_revision,
	const std::vector<flatfile_item_ownership_record> &custody,
	std::vector<player_load_item_identity> *identities)
{
	if (!identities || !record.revision || !owner_revision ||
	    record.items.size() != custody.size() ||
	    record.items.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
		return flatfile_shopkeeper_ownership_result::invalid;
	const item_owner_identity owner = flatfile_shopkeeper_item_owner(record.shop_id);
	std::unordered_map<uint64_t, const flatfile_item_ownership_record *> by_uid;
	std::vector<player_load_item_identity> reconciled;
	try
	{
		by_uid.reserve(custody.size());
		reconciled.reserve(record.items.size());
		for (const auto &entry : custody)
			if (!entry.item_uid || entry.state != item_custody_state::active ||
			    !item_owner_identity_equal(entry.owner, owner) ||
			    !by_uid.emplace(entry.item_uid, &entry).second)
				return flatfile_shopkeeper_ownership_result::invalid;
		for (size_t index = 0; index < record.items.size(); ++index)
		{
			const player_item_snapshot &item = record.items[index];
			const auto found = by_uid.find(item.object_uid);
			if (!item.object_uid || found == by_uid.end() ||
			    found->second->vnum != item.vnum)
				return flatfile_shopkeeper_ownership_result::invalid;
			uint64_t parent_uid = 0;
			uint64_t root_uid = item.object_uid;
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 ||
				    static_cast<size_t>(item.parent_index) >= index)
					return flatfile_shopkeeper_ownership_result::invalid;
				const player_load_item_identity &parent =
					reconciled[static_cast<size_t>(item.parent_index)];
				parent_uid = parent.item_uid;
				root_uid = parent.root_item_uid;
			}
			const flatfile_item_ownership_record &entry = *found->second;
			if (entry.root_item_uid != root_uid ||
			    entry.parent_item_uid != parent_uid || !entry.item_revision)
				return flatfile_shopkeeper_ownership_result::invalid;
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
		return flatfile_shopkeeper_ownership_result::io_error;
	}
	*identities = std::move(reconciled);
	return flatfile_shopkeeper_ownership_result::ok;
}

flatfile_shopkeeper_ownership_result flatfile_shopkeeper_load_item_ownership(
	const std::string &root, const flatfile_shopkeeper_record &record, uint64_t *owner_revision,
	std::vector<player_load_item_identity> *identities, std::string *error)
{
	if (!owner_revision || !identities)
		return flatfile_shopkeeper_ownership_result::invalid;
	uint64_t loaded_revision = 0;
	std::vector<flatfile_item_ownership_record> custody;
	const auto loaded = flatfile_item_repository_load_owner(
		root, flatfile_shopkeeper_item_owner(record.shop_id), &loaded_revision, &custody,
		error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded == flatfile_item_repository_result::not_found ?
			       flatfile_shopkeeper_ownership_result::not_found :
		       loaded == flatfile_item_repository_result::io_error ?
			       flatfile_shopkeeper_ownership_result::io_error :
			       flatfile_shopkeeper_ownership_result::invalid;
	const auto reconciled = flatfile_shopkeeper_reconcile_item_ownership(
		record, loaded_revision, custody, identities);
	if (reconciled == flatfile_shopkeeper_ownership_result::ok)
		*owner_revision = loaded_revision;
	return reconciled;
}
