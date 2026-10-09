#include "flatfile/flatfile_corpse_restore.h"

#include "persistence/corpse_lifecycle_transaction.h"
#include "flatfile/flatfile_corpse_ownership.h"
#include "flatfile/flatfile_accounting_coin_transaction.h"
#include "flatfile/flatfile_identity_repository.h"
#include "economy/coin_physical_recovery.h"
#include "player/player_snapshot_codec.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "item/item_ownership_runtime.h"
#include "classes/necromancy.h"
#include "player/player_load_items.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"

#include <algorithm>
#include <new>
#include <map>
#include <set>
#include <utility>
#include <vector>

extern int skip_corpse_save;
extern bool updateArtis;

namespace
{
class restore_side_effect_guard
{
    public:
	restore_side_effect_guard()
		: previous_corpse_save(skip_corpse_save)
		, previous_artifact_update(updateArtis)
	{
		skip_corpse_save = 1;
		updateArtis = false;
	}
	~restore_side_effect_guard()
	{
		skip_corpse_save = previous_corpse_save;
		updateArtis = previous_artifact_update;
	}

    private:
	int previous_corpse_save;
	bool previous_artifact_update;
};

struct staged_corpse
{
	P_obj object = nullptr;
	int room_rnum = NOWHERE;
	uint32_t owner_pid = 0;
	uint32_t save_id = 0;
	bool lifecycle_hydrated = false;
};

struct staged_room
{
	std::vector<P_obj> roots;
	P_obj money = nullptr;
	int room_rnum = NOWHERE;
};

void clear_item_uids(P_obj object)
{
	for (; object; object = object->next_content)
	{
		object->obj_uid = 0;
		clear_item_uids(object->contains);
	}
}

void forget_record_items(const flatfile_corpse_record &record)
{
	for (const auto &item : record.items)
		item_ownership_runtime_forget(item.object_uid);
	item_ownership_runtime_forget_owner(
		flatfile_corpse_item_owner(record.owner_pid, record.save_id));
}

void forget_record_items(const flatfile_room_item_record &record)
{
	for (const auto &item : record.items)
		item_ownership_runtime_forget(item.object_uid);
	item_ownership_runtime_forget_owner(
		{ item_owner_type::room, static_cast<uint64_t>(record.room_vnum), 0 });
}

void discard_staged(std::vector<staged_corpse> *staged,
		    const std::vector<flatfile_corpse_record> &records)
{
	if (!staged)
		return;
	for (size_t index = 0; index < staged->size(); ++index)
	{
		staged_corpse &entry = (*staged)[index];
		forget_record_items(records[index]);
		if (entry.lifecycle_hydrated)
			(void)corpse_lifecycle_transaction_forget(entry.owner_pid, entry.save_id);
		if (entry.object)
		{
			clear_item_uids(entry.object->contains);
			extract_obj(entry.object, FALSE);
			entry.object = nullptr;
		}
	}
}

void discard_staged(std::vector<staged_room> *staged,
		    const std::vector<flatfile_room_item_record> &records)
{
	if (!staged)
		return;
	for (size_t index = 0; index < staged->size(); ++index)
	{
		staged_room &entry = (*staged)[index];
		forget_record_items(records[index]);
		for (P_obj item : entry.roots)
		{
			clear_item_uids(item);
			extract_obj(item, FALSE);
		}
		entry.roots.clear();
		if (entry.money)
		{
			extract_obj(entry.money, FALSE);
			entry.money = nullptr;
		}
	}
}

bool valid_record(const flatfile_corpse_record &record)
{
	return record.owner_pid && record.owner_pid <= INT32_MAX && record.save_id &&
	       record.save_id <= INT32_MAX && record.room_vnum > 0 && record.revision &&
	       record.values[CORPSE_PID] == static_cast<int32_t>(record.owner_pid) &&
	       record.values[CORPSE_SAVEID] == static_cast<int32_t>(record.save_id) &&
	       record.values[CORPSE_RACEWAR] >= 0 && record.values[CORPSE_RACEWAR] <= 4 &&
	       IS_SET(record.values[CORPSE_FLAGS], PC_CORPSE) &&
	       std::all_of(record.money.begin(), record.money.end(),
			   [](int32_t amount) { return amount >= 0; });
}

void set_corpse_identity(P_obj corpse, const flatfile_corpse_record &record)
{
	char keywords[MAX_STRING_LENGTH];
	if (!record.keywords.empty())
		set_keywords(corpse, record.keywords.c_str());
	else
	{
		checked_snprintf(keywords, sizeof(keywords), "%s corpse _pcorpse_",
				 record.owner_name.c_str());
		set_keywords(corpse, keywords);
	}
	if (!record.short_description.empty())
		set_short_description(corpse, record.short_description.c_str());
	if (!record.description.empty())
		set_long_description(corpse, record.description.c_str());
	if ((corpse->str_mask & STRUNG_DESC3) && corpse->action_description)
		FREE(corpse->action_description);
	corpse->str_mask |= STRUNG_DESC3;
	corpse->action_description = str_dup(record.owner_name.c_str());
}

void attach_root(P_obj corpse, P_obj root, P_obj *tail)
{
	root->loc_p = LOC_INSIDE;
	root->loc.inside = corpse;
	root->next_content = nullptr;
	if (*tail)
		(*tail)->next_content = root;
	else
		corpse->contains = root;
	*tail = root;
}

flatfile_corpse_restore_result materialize_corpse(const std::string &root,
		const flatfile_authority_lock &lock,
						  const flatfile_corpse_record &record,
						  staged_corpse *output, std::string *error)
{
	if (!output || !valid_record(record))
		return flatfile_corpse_restore_result::invalid;
	const int room_rnum = real_room(record.room_vnum);
	if (room_rnum == NOWHERE)
		return flatfile_corpse_restore_result::unknown_room;
	const int corpse_rnum = real_object(2);
	if (corpse_rnum < 0)
		return flatfile_corpse_restore_result::unknown_prototype;

	uint64_t owner_revision = 0;
	std::vector<player_load_item_identity> identities;
	const auto ownership = flatfile_corpse_load_item_ownership_locked(root, lock, record, &owner_revision,
								   &identities, error);
	if (ownership != flatfile_corpse_ownership_result::ok)
		return ownership == flatfile_corpse_ownership_result::not_found ?
			       flatfile_corpse_restore_result::item_failure :
		       ownership == flatfile_corpse_ownership_result::io_error ?
			       flatfile_corpse_restore_result::io_error :
			       flatfile_corpse_restore_result::invalid;

	std::vector<P_obj> roots;
	player_load_item_materialize_metrics metrics = {};
	const item_owner_identity owner =
		flatfile_corpse_item_owner(record.owner_pid, record.save_id);
	if (!record.items.empty() &&
	    !player_load_item_graph_materialize_detached(
		    record.items, identities, owner, owner_revision, true, true, &roots, &metrics))
		return metrics.outcome == player_load_item_materialize_outcome::allocation_failure ?
			       flatfile_corpse_restore_result::allocation_failure :
			       flatfile_corpse_restore_result::item_failure;
	if (record.items.empty() && owner_revision &&
	    !item_ownership_runtime_hydrate_owner(owner, owner_revision))
		return flatfile_corpse_restore_result::item_failure;

	P_obj corpse = read_object(corpse_rnum, REAL);
	if (!corpse)
	{
		forget_record_items(record);
		for (P_obj item : roots)
		{
			clear_item_uids(item);
			extract_obj(item, FALSE);
		}
		return flatfile_corpse_restore_result::allocation_failure;
	}
	corpse->type = ITEM_CORPSE;
	corpse->weight = record.weight;
	for (size_t index = 0; index < record.values.size(); ++index)
		corpse->value[index] = record.values[index];
	set_corpse_identity(corpse, record);
	P_obj tail = nullptr;
	for (P_obj item : roots)
		attach_root(corpse, item, &tail);
	if (std::any_of(record.money.begin(), record.money.end(),
			[](int32_t amount) { return amount != 0; }))
	{
		P_obj money = create_money(record.money[0], record.money[1], record.money[2],
					   record.money[3]);
		if (!money)
		{
			forget_record_items(record);
			clear_item_uids(corpse->contains);
			extract_obj(corpse, FALSE);
			return flatfile_corpse_restore_result::allocation_failure;
		}
		attach_root(corpse, money, &tail);
	}
	if (!corpse_lifecycle_transaction_hydrate(record.owner_pid, record.save_id,
						  record.revision))
	{
		forget_record_items(record);
		clear_item_uids(corpse->contains);
		extract_obj(corpse, FALSE);
		return flatfile_corpse_restore_result::item_failure;
	}
	*output = { corpse, room_rnum, record.owner_pid, record.save_id, true };
	return flatfile_corpse_restore_result::ok;
}

flatfile_corpse_restore_result materialize_room(const std::string &root,
		const flatfile_authority_lock &lock, const flatfile_room_coin_boot_view &coins,
						const flatfile_room_item_record &record,
						staged_room *output, std::string *error)
{
	if (!output || record.room_vnum <= 0 || !record.revision ||
	    !std::all_of(record.money.begin(), record.money.end(),
			 [](int32_t amount) { return amount >= 0; }))
		return flatfile_corpse_restore_result::invalid;
	const int room_rnum = real_room(record.room_vnum);
	if (room_rnum == NOWHERE)
		return flatfile_corpse_restore_result::unknown_room;
	const item_owner_identity owner = { item_owner_type::room,
					    static_cast<uint64_t>(record.room_vnum), 0 };
	uint64_t owner_revision = 0;
	std::vector<player_load_item_identity> identities;
	const auto ownership = flatfile_room_load_item_ownership_locked(root, lock, record, &owner_revision,
								 &identities, error);
	if (ownership != flatfile_corpse_ownership_result::ok)
		return ownership == flatfile_corpse_ownership_result::not_found ?
			       flatfile_corpse_restore_result::item_failure :
		       ownership == flatfile_corpse_ownership_result::io_error ?
			       flatfile_corpse_restore_result::io_error :
			       flatfile_corpse_restore_result::invalid;
	// Original complete owner census succeeded above. Only now partition
	// authenticated typed roots from the generic graph; preserve all remaining
	// literal/identity links and rebase serialized snapshot parent indices.
	std::vector<player_item_snapshot> ordinary;
	std::vector<player_load_item_identity> ordinary_identities;
	try
	{
		const bool fenced = std::binary_search(coins.fenced_rooms.begin(),
			coins.fenced_rooms.end(), record.room_vnum);
		if (fenced && (record.revision != owner_revision ||
			std::any_of(record.money.begin(), record.money.end(),
				[](int32_t amount) { return amount != 0; })))
			return flatfile_corpse_restore_result::item_failure;
		std::vector<int32_t> positions(record.items.size(), PLAYER_SNAPSHOT_NO_PARENT);
		ordinary.reserve(record.items.size());
		ordinary_identities.reserve(identities.size());
		for (size_t index = 0; index < record.items.size(); ++index)
		{
			const auto &item = record.items[index];
			if (std::binary_search(coins.history_uids.begin(), coins.history_uids.end(), item.object_uid))
			{
				const auto found = std::find_if(coins.piles.begin(), coins.piles.end(),
					[&](const flatfile_room_coin_pile &pile)
					{ return pile.identity.item_uid == item.object_uid; });
				if (found == coins.piles.end() || item.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
					found->identity.owner.id != static_cast<uint64_t>(record.room_vnum) ||
					found->identity.owner_revision != owner_revision ||
					identities[index].item_revision != found->identity.item_revision)
					return flatfile_corpse_restore_result::item_failure;
				std::vector<uint8_t> persisted, proved;
				if (player_item_snapshot_list_encode({ item }, &persisted) != player_snapshot_codec_result::ok ||
					player_item_snapshot_list_encode({ found->item }, &proved) != player_snapshot_codec_result::ok ||
					persisted != proved)
					return flatfile_corpse_restore_result::item_failure;
				continue;
			}
			auto copied = item;
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			{
				if (item.parent_index < 0 || static_cast<size_t>(item.parent_index) >= index ||
					positions[item.parent_index] == PLAYER_SNAPSHOT_NO_PARENT)
					return flatfile_corpse_restore_result::item_failure;
				copied.parent_index = positions[item.parent_index];
			}
			positions[index] = static_cast<int32_t>(ordinary.size());
			ordinary.push_back(std::move(copied));
			ordinary_identities.push_back(identities[index]);
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_corpse_restore_result::allocation_failure;
	}
	std::vector<P_obj> roots;
	player_load_item_materialize_metrics metrics = {};
	if (!ordinary.empty() &&
	    !player_load_item_graph_materialize_detached(
		    ordinary, ordinary_identities, owner, owner_revision, true, true, &roots, &metrics))
		return metrics.outcome == player_load_item_materialize_outcome::allocation_failure ?
			       flatfile_corpse_restore_result::allocation_failure :
			       flatfile_corpse_restore_result::item_failure;
	if (ordinary.empty() && !item_ownership_runtime_hydrate_owner(owner, owner_revision))
		return flatfile_corpse_restore_result::item_failure;
	P_obj money = nullptr;
	if (std::any_of(record.money.begin(), record.money.end(),
			[](int32_t amount) { return amount != 0; }))
	{
		money = create_money(record.money[0], record.money[1], record.money[2],
				     record.money[3]);
		if (!money)
		{
			forget_record_items(record);
			for (P_obj item : roots)
			{
				clear_item_uids(item);
				extract_obj(item, FALSE);
			}
			return flatfile_corpse_restore_result::allocation_failure;
		}
	}
	output->roots = std::move(roots);
	output->money = money;
	output->room_rnum = room_rnum;
	return flatfile_corpse_restore_result::ok;
}
} // namespace

flatfile_corpse_restore_result flatfile_corpse_restore_catalog(const std::string &root,
							       std::string *error)
{
	if (root.empty())
		return flatfile_corpse_restore_result::invalid;
	restore_side_effect_guard guard;
	// Native transaction ordering: identity before authority. Keep both cuts
	// through every detached stage, original publication and rollback.
	flatfile_identity_lock identity;
	flatfile_authority_lock authority;
	if (!identity.acquire(root, error) || !authority.acquire(root, error) ||
		flatfile_authority_transaction_recover(root, authority, error) !=
			flatfile_authority_transaction_result::ok)
		return flatfile_corpse_restore_result::io_error;
	flatfile_room_coin_boot_view coins;
	// Match the original physical recovery owner's existing million-visit bound.
	if (flatfile_accounting_coin_transaction::read_room_boot_locked(root, authority,
		1000000, &coins, error))
		return flatfile_corpse_restore_result::item_failure;
	std::vector<flatfile_corpse_record> records;
	std::vector<flatfile_room_item_record> room_records;
	const auto listed = flatfile_world_item_recovery_list_locked(root, authority,
		&records, &room_records, error);
	if (listed != flatfile_world_item_result::ok &&
		!(listed == flatfile_world_item_result::not_found && !coins.history_uids.empty()))
		return listed == flatfile_world_item_result::not_found ?
			flatfile_corpse_restore_result::not_found :
			listed == flatfile_world_item_result::io_error ?
			flatfile_corpse_restore_result::io_error : flatfile_corpse_restore_result::invalid;
	try
	{
		// Original native COIN writes may exist without a world-room projection.
		// Build an owned value record only from the complete native owner census;
		// every current item must be one of the independently proved typed roots.
		std::map<int32_t, std::vector<const flatfile_room_coin_pile *>> missing;
		for (const auto &pile : coins.piles)
			if (std::none_of(room_records.begin(), room_records.end(),
				[&](const flatfile_room_item_record &room)
				{ return static_cast<uint64_t>(room.room_vnum) == pile.identity.owner.id; }))
				missing[static_cast<int32_t>(pile.identity.owner.id)].push_back(&pile);
		if (listed == flatfile_world_item_result::not_found)
		{
			std::vector<flatfile_item_ownership_record> catalog;
			if (flatfile_item_repository_recovery_catalog_locked(root, authority,
				&catalog, error) != flatfile_item_repository_result::ok)
				return flatfile_corpse_restore_result::item_failure;
			for (const auto &item : catalog)
				if (item.state == item_custody_state::active &&
					(item.owner.type == item_owner_type::corpse || item.owner.type == item_owner_type::room) &&
					std::none_of(coins.piles.begin(), coins.piles.end(),
						[&](const flatfile_room_coin_pile &pile)
						{ return pile.identity.item_uid == item.item_uid; }))
					return flatfile_corpse_restore_result::item_failure;
		}
		for (const auto &[room_vnum, piles] : missing)
		{
			const item_owner_identity owner{ item_owner_type::room, static_cast<uint64_t>(room_vnum), 0 };
			uint64_t revision = 0;
			std::vector<flatfile_item_ownership_record> custody;
			if (flatfile_item_repository_load_owner_locked(root, authority, owner,
				&revision, &custody, error) != flatfile_item_repository_result::ok ||
				!revision || custody.size() != piles.size())
				return flatfile_corpse_restore_result::item_failure;
			flatfile_room_item_record room;
			room.room_vnum = room_vnum;
			room.revision = revision;
			for (const auto *pile : piles)
			{
				if (pile->identity.owner_revision != revision)
					return flatfile_corpse_restore_result::item_failure;
				room.items.push_back(pile->item);
			}
			std::vector<player_load_item_identity> reconciled;
			if (flatfile_world_reconcile_item_ownership(room.items, owner, revision,
				custody, &reconciled) != flatfile_corpse_ownership_result::ok)
				return flatfile_corpse_restore_result::item_failure;
			room_records.push_back(std::move(room));
		}
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_corpse_restore_result::allocation_failure;
	}

	std::vector<staged_corpse> staged;
	try
	{
		staged.reserve(records.size());
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_corpse_restore_result::io_error;
	}
	for (const auto &record : records)
	{
		staged_corpse corpse = {};
		const auto materialized = materialize_corpse(root, authority, record, &corpse, error);
		if (materialized != flatfile_corpse_restore_result::ok)
		{
			discard_staged(&staged, records);
			return materialized;
		}
		try
		{
			staged.push_back(corpse);
		}
		catch (const std::bad_alloc &)
		{
			forget_record_items(record);
			(void)corpse_lifecycle_transaction_forget(record.owner_pid, record.save_id);
			clear_item_uids(corpse.object->contains);
			extract_obj(corpse.object, FALSE);
			discard_staged(&staged, records);
			return flatfile_corpse_restore_result::io_error;
		}
	}
	std::vector<staged_room> staged_rooms;
	try
	{
		staged_rooms.reserve(room_records.size());
	}
	catch (const std::bad_alloc &)
	{
		discard_staged(&staged, records);
		return flatfile_corpse_restore_result::io_error;
	}
	for (const auto &record : room_records)
	{
		staged_room room = {};
		const auto materialized = materialize_room(root, authority, coins, record, &room, error);
		if (materialized != flatfile_corpse_restore_result::ok)
		{
			discard_staged(&staged_rooms, room_records);
			discard_staged(&staged, records);
			return materialized;
		}
		try
		{
			staged_rooms.push_back(std::move(room));
		}
		catch (const std::bad_alloc &)
		{
			forget_record_items(record);
			for (P_obj item : room.roots)
			{
				clear_item_uids(item);
				extract_obj(item, FALSE);
			}
			if (room.money)
				extract_obj(room.money, FALSE);
			discard_staged(&staged_rooms, room_records);
			discard_staged(&staged, records);
			return flatfile_corpse_restore_result::io_error;
		}
	}
	std::vector<flatfile_coin_boot_stage> staged_coins;
	try
	{
		staged_coins.reserve(coins.piles.size());
		for (const auto &pile : coins.piles)
		{
			flatfile_coin_boot_stage stage;
			if (!flatfile_coin_boot_stage::prepare(root, authority, pile.identity.item_uid, stage))
			{
				discard_staged(&staged_rooms, room_records);
				discard_staged(&staged, records);
				return flatfile_corpse_restore_result::item_failure;
			}
			staged_coins.push_back(std::move(stage));
		}
	}
	catch (const std::bad_alloc &)
	{
		discard_staged(&staged_rooms, room_records);
		discard_staged(&staged, records);
		return flatfile_corpse_restore_result::allocation_failure;
	}
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	uint64_t destruction_revision = 0;
	std::vector<flatfile_item_ownership_record> destruction_items;
	const auto destruction_loaded = flatfile_item_repository_load_owner_locked(
		root, authority, destruction, &destruction_revision, &destruction_items, error);
	if ((destruction_loaded != flatfile_item_repository_result::ok &&
	     destruction_loaded != flatfile_item_repository_result::not_found) ||
	    !destruction_items.empty() ||
	    !item_ownership_runtime_hydrate_owner(
		    destruction, destruction_loaded == flatfile_item_repository_result::ok ?
					 destruction_revision :
					 0))
	{
		discard_staged(&staged_rooms, room_records);
		discard_staged(&staged, records);
		return destruction_loaded == flatfile_item_repository_result::io_error ?
			       flatfile_corpse_restore_result::io_error :
			       flatfile_corpse_restore_result::item_failure;
	}
	for (size_t index = 0; index < staged.size(); ++index)
	{
		obj_to_room(staged[index].object, staged[index].room_rnum);
		if (!OBJ_ROOM(staged[index].object) ||
		    staged[index].object->loc.room != staged[index].room_rnum)
		{
			item_ownership_runtime_forget_owner(destruction);
			discard_staged(&staged_rooms, room_records);
			discard_staged(&staged, records);
			return flatfile_corpse_restore_result::publish_failure;
		}
		persistence_refresh_restored_corpse(staged[index].object,
						    "flatfile_corpse_restore_catalog");
	}
	for (size_t index = 0; index < staged_rooms.size(); ++index)
	{
		for (P_obj item : staged_rooms[index].roots)
		{
			obj_to_room(item, staged_rooms[index].room_rnum);
			if (!OBJ_ROOM(item) || item->loc.room != staged_rooms[index].room_rnum)
			{
				item_ownership_runtime_forget_owner(destruction);
				discard_staged(&staged_rooms, room_records);
				discard_staged(&staged, records);
				return flatfile_corpse_restore_result::publish_failure;
			}
		}
	}
	for (auto &coin : staged_coins)
		if (!coin.publish())
		{
			item_ownership_runtime_forget_owner(destruction);
			discard_staged(&staged_rooms, room_records);
			discard_staged(&staged, records);
			return flatfile_corpse_restore_result::publish_failure;
		}
	for (size_t index = 0; index < staged_rooms.size(); ++index)
	{
		if (staged_rooms[index].money)
		{
			obj_to_room(staged_rooms[index].money, staged_rooms[index].room_rnum);
			staged_rooms[index].money = nullptr;
		}
	}
	for (auto &coin : staged_coins)
		coin.finish();
	for (auto &entry : staged)
		entry.object = nullptr;
	for (auto &entry : staged_rooms)
		entry.roots.clear();
	return flatfile_corpse_restore_result::ok;
}
