#include "flatfile/flatfile_shopkeeper_restore.h"

#include "flatfile/flatfile_shopkeeper_materialize.h"
#include "item/item_ownership_runtime.h"
#include "shop_trade_runtime.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"

#include <new>
#include <unordered_set>
#include <vector>

extern P_char character_list;
extern P_index mob_index;
extern P_index obj_index;
extern struct shop_data *shop_index;
extern int number_of_shops;
extern int top_of_objt;

namespace
{
bool valid_shop_binding(const flatfile_shopkeeper_record &record)
{
	if (number_of_shops < 0 || record.shop_id >= static_cast<uint32_t>(number_of_shops))
		return false;
	const int mobile_rnum = real_mobile(record.mob_vnum);
	if (mobile_rnum < 0 || shop_index[record.shop_id].keeper != mobile_rnum)
		return false;
	const int produced_count = shop_index[record.shop_id].number_items_produced;
	if (produced_count < 0 || produced_count > MAX_PROD)
		return false;
	for (int produced = 0; produced < produced_count; ++produced)
	{
		const int object_rnum = shop_index[record.shop_id].producing[produced];
		if (object_rnum < 0 || object_rnum > top_of_objt)
			return false;
		const int object_vnum = obj_index[object_rnum].virtual_number;
		bool found = false;
		for (const auto &item : record.items)
			if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
			    item.equipment_slot == 0 && item.vnum == object_vnum)
			{
				found = true;
				break;
			}
		if (!found)
			return false;
	}
	return true;
}

void forget_record_items(const flatfile_shopkeeper_record &record)
{
	for (const auto &item : record.items)
		item_ownership_runtime_forget(item.object_uid);
}

void discard_staged(std::vector<flatfile_materialized_shopkeeper> *staged,
		    const std::vector<flatfile_shopkeeper_record> &records)
{
	if (!staged)
		return;
	for (size_t index = 0; index < staged->size(); ++index)
	{
		forget_record_items(records[index]);
		if ((*staged)[index].character)
			extract_char((*staged)[index].character);
		(*staged)[index].character = nullptr;
	}
}
}

flatfile_shopkeeper_restore_result flatfile_shopkeeper_restore_catalog(const std::string &root,
								       std::string *error)
{
	std::vector<flatfile_shopkeeper_record> records;
	const auto listed = flatfile_shopkeeper_list(root, &records, error);
	if (listed != flatfile_shopkeeper_result::ok)
		return listed == flatfile_shopkeeper_result::not_found ?
			       flatfile_shopkeeper_restore_result::not_found :
		       listed == flatfile_shopkeeper_result::io_error ?
			       flatfile_shopkeeper_restore_result::io_error :
			       flatfile_shopkeeper_restore_result::invalid;

	std::unordered_set<int32_t> mobile_vnums;
	std::vector<flatfile_materialized_shopkeeper> staged;
	try
	{
		mobile_vnums.reserve(records.size());
		staged.reserve(records.size());
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_shopkeeper_restore_result::io_error;
	}
	for (const auto &record : records)
	{
		try
		{
			if (!mobile_vnums.insert(record.mob_vnum).second ||
			    !valid_shop_binding(record))
			{
				discard_staged(&staged, records);
				return flatfile_shopkeeper_restore_result::invalid;
			}
		}
		catch (const std::bad_alloc &)
		{
			discard_staged(&staged, records);
			return flatfile_shopkeeper_restore_result::io_error;
		}
		flatfile_materialized_shopkeeper materialized = {};
		if (flatfile_shopkeeper_materialize(root, record, &materialized, nullptr, error) !=
		    flatfile_shopkeeper_materialize_result::ok)
		{
			discard_staged(&staged, records);
			return flatfile_shopkeeper_restore_result::materialize_failure;
		}
		staged.push_back(materialized);
	}

	for (size_t index = 0; index < staged.size(); ++index)
		if (!char_to_room(staged[index].character, staged[index].room_rnum, 0))
		{
			forget_record_items(records[index]);
			staged[index].character = nullptr;
			discard_staged(&staged, records);
			return flatfile_shopkeeper_restore_result::publish_failure;
		}
		else if (staged[index].character->in_room != staged[index].room_rnum)
		{
			discard_staged(&staged, records);
			return flatfile_shopkeeper_restore_result::publish_failure;
		}
	std::unordered_set<P_char> replacements;
	try
	{
		replacements.reserve(staged.size());
		for (const auto &entry : staged)
			replacements.insert(entry.character);
	}
	catch (const std::bad_alloc &)
	{
		discard_staged(&staged, records);
		return flatfile_shopkeeper_restore_result::io_error;
	}
	if (!shop_trade_runtime_replace_revisions(records))
	{
		discard_staged(&staged, records);
		return flatfile_shopkeeper_restore_result::io_error;
	}
	for (P_char existing = character_list; existing;)
	{
		P_char next = existing->next;
		if (IS_NPC(existing) && replacements.find(existing) == replacements.end() &&
		    mobile_vnums.find(mob_index[GET_RNUM(existing)].virtual_number) !=
			    mobile_vnums.end())
			extract_char(existing);
		existing = next;
	}
	for (const auto &record : records)
		shop_index[record.shop_id].dirty = 1;
	return flatfile_shopkeeper_restore_result::ok;
}
