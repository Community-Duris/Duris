#include "flatfile_shopkeeper_save.h"

#include "flatfile_shopkeeper_capture.h"
#include "flatfile_shopkeeper_ownership.h"
#include "flatfile_shopkeeper_repository.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"

#include <algorithm>
#include <limits>
#include <vector>

extern P_room world;
extern struct shop_data *shop_index;
extern int number_of_shops;
extern int top_of_world;

flatfile_shopkeeper_save_result flatfile_shopkeeper_save(const std::string &root, P_char shopkeeper,
							 uint32_t shop_id, int64_t saved_at,
							 std::string *error)
{
	if (root.empty() || !shopkeeper || saved_at < 0)
		return flatfile_shopkeeper_save_result::invalid;
	std::vector<flatfile_shopkeeper_record> records;
	const auto listed = flatfile_shopkeeper_list(root, &records, error);
	if (listed != flatfile_shopkeeper_result::ok)
		return listed == flatfile_shopkeeper_result::not_found ?
			       flatfile_shopkeeper_save_result::not_found :
		       listed == flatfile_shopkeeper_result::io_error ?
			       flatfile_shopkeeper_save_result::io_error :
			       flatfile_shopkeeper_save_result::invalid;
	auto existing = std::lower_bound(records.begin(), records.end(), shop_id,
					 [](const flatfile_shopkeeper_record &record, uint32_t id)
					 { return record.shop_id < id; });
	if (existing == records.end() || existing->shop_id != shop_id)
		return flatfile_shopkeeper_save_result::not_found;
	if (!existing->revision || existing->revision == std::numeric_limits<uint64_t>::max())
		return flatfile_shopkeeper_save_result::invalid;

	flatfile_shopkeeper_record captured = {};
	if (flatfile_shopkeeper_capture(shopkeeper, shop_id, existing->revision + 1, saved_at,
					&captured) != player_snapshot_capture_result::ok)
		return flatfile_shopkeeper_save_result::capture_failure;
	if (captured.mob_vnum != existing->mob_vnum)
		return flatfile_shopkeeper_save_result::invalid;
	uint64_t owner_revision = 0;
	std::vector<player_load_item_identity> identities;
	const auto custody = flatfile_shopkeeper_load_item_ownership(
		root, captured, &owner_revision, &identities, error);
	if (custody != flatfile_shopkeeper_ownership_result::ok)
		return custody == flatfile_shopkeeper_ownership_result::io_error ?
			       flatfile_shopkeeper_save_result::io_error :
			       flatfile_shopkeeper_save_result::custody_mismatch;
	const auto replaced =
		flatfile_shopkeeper_replace(root, captured, existing->revision, error);
	if (replaced == flatfile_shopkeeper_result::ok)
		return flatfile_shopkeeper_save_result::ok;
	if (replaced == flatfile_shopkeeper_result::stale)
		return flatfile_shopkeeper_save_result::stale;
	return replaced == flatfile_shopkeeper_result::io_error ?
		       flatfile_shopkeeper_save_result::io_error :
		       flatfile_shopkeeper_save_result::invalid;
}

bool flatfile_shopkeeper_save_dirty(const std::string &root, int64_t saved_at, std::string *error)
{
	if (root.empty() || saved_at < 0 || number_of_shops < 0)
		return false;
	bool complete = true;
	for (int shop_id = 0; shop_id < number_of_shops; ++shop_id)
	{
		if (!shop_index[shop_id].dirty)
			continue;
		const int room_rnum = real_room(shop_index[shop_id].in_room);
		P_char keeper = nullptr;
		if (room_rnum >= 0 && room_rnum <= top_of_world)
			for (P_char candidate = world[room_rnum].people; candidate;
			     candidate = candidate->next_in_room)
				if (IS_NPC(candidate) &&
				    GET_RNUM(candidate) == shop_index[shop_id].keeper)
				{
					keeper = candidate;
					break;
				}
		if (!keeper || flatfile_shopkeeper_save(root, keeper, shop_id, saved_at, error) !=
				       flatfile_shopkeeper_save_result::ok)
		{
			complete = false;
			continue;
		}
		shop_index[shop_id].dirty = 0;
	}
	return complete;
}
