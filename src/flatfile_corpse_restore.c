#include "flatfile_corpse_restore.h"

#include "corpse_lifecycle_transaction.h"
#include "flatfile_corpse_ownership.h"
#include "flatfile_world_item_repository.h"
#include "item_ownership_runtime.h"
#include "necromancy.h"
#include "player_load_items.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"

#include <algorithm>
#include <new>
#include <utility>
#include <vector>

extern int skip_corpse_save;

namespace
{
class corpse_save_guard
{
    public:
	corpse_save_guard()
		: previous(skip_corpse_save)
	{
		skip_corpse_save = 1;
	}
	~corpse_save_guard() { skip_corpse_save = previous; }

    private:
	int previous;
};

struct staged_corpse
{
	P_obj object = nullptr;
	int room_rnum = NOWHERE;
	uint32_t owner_pid = 0;
	uint32_t save_id = 0;
	bool lifecycle_hydrated = false;
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
	const auto ownership = flatfile_corpse_load_item_ownership(root, record, &owner_revision,
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
} // namespace

flatfile_corpse_restore_result flatfile_corpse_restore_catalog(const std::string &root,
							       std::string *error)
{
	if (root.empty())
		return flatfile_corpse_restore_result::invalid;
	corpse_save_guard guard;
	std::vector<flatfile_corpse_record> records;
	std::vector<flatfile_saved_world_item_record> saved_items;
	const auto listed = flatfile_world_item_list(root, &records, &saved_items, error);
	if (listed != flatfile_world_item_result::ok)
		return listed == flatfile_world_item_result::not_found ?
			       flatfile_corpse_restore_result::not_found :
		       listed == flatfile_world_item_result::io_error ?
			       flatfile_corpse_restore_result::io_error :
			       flatfile_corpse_restore_result::invalid;

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
		const auto materialized = materialize_corpse(root, record, &corpse, error);
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
	for (size_t index = 0; index < staged.size(); ++index)
	{
		obj_to_room(staged[index].object, staged[index].room_rnum);
		if (!OBJ_ROOM(staged[index].object) ||
		    staged[index].object->loc.room != staged[index].room_rnum)
		{
			discard_staged(&staged, records);
			return flatfile_corpse_restore_result::publish_failure;
		}
		persistence_refresh_restored_corpse(staged[index].object,
						    "flatfile_corpse_restore_catalog");
	}
	for (auto &entry : staged)
		entry.object = nullptr;
	return flatfile_corpse_restore_result::ok;
}
