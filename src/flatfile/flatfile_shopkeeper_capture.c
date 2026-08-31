#include "flatfile/flatfile_shopkeeper_capture.h"

#include "player/player_snapshot_capture.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"

#include <new>
#include <unordered_set>
#include <utility>

extern P_index mob_index;
extern P_room world;
extern int top_of_mobt;
extern int top_of_world;

player_snapshot_capture_result flatfile_shopkeeper_capture(P_char shopkeeper, uint32_t shop_id,
							   uint64_t revision, int64_t saved_at,
							   flatfile_shopkeeper_record *record_out)
{
	if (!shopkeeper || !record_out || IS_PC(shopkeeper) || !revision || saved_at < 0)
		return player_snapshot_capture_result::invalid_identity;
	const int mob_rnum = GET_RNUM(shopkeeper);
	if (mob_rnum < 0 || mob_rnum > top_of_mobt || !IS_SHOPKEEPER(shopkeeper) ||
	    shopkeeper->in_room <= NOWHERE || shopkeeper->in_room > top_of_world)
		return player_snapshot_capture_result::invalid_identity;
	try
	{
		flatfile_shopkeeper_record record = {};
		record.shop_id = shop_id;
		record.mob_vnum = mob_index[mob_rnum].virtual_number;
		record.room_vnum = world[shopkeeper->in_room].number;
		record.saved_at = saved_at;
		record.revision = revision;
		std::unordered_set<const struct affected_type *> seen;
		for (const struct affected_type *affect = shopkeeper->affected; affect;
		     affect = affect->next)
		{
			if (!seen.insert(affect).second)
				return player_snapshot_capture_result::object_cycle;
			if (IS_SET(affect->flags, AFFTYPE_NOSAVE))
				continue;
			record.affects.push_back(
				{ affect->type,
				  affect->duration,
				  affect->modifier,
				  affect->location,
				  { affect->bitvector, affect->bitvector2, affect->bitvector3,
				    affect->bitvector4, affect->bitvector5 } });
		}
		const auto item_result = player_item_snapshot_list_capture(
			shopkeeper, true, true, false, &record.items, nullptr);
		if (item_result != player_snapshot_capture_result::ok)
			return item_result;
		for (const auto &item : record.items)
			if (!item.object_uid)
				return player_snapshot_capture_result::malformed_source;
		*record_out = std::move(record);
	}
	catch (const std::bad_alloc &)
	{
		return player_snapshot_capture_result::retryable_allocation_failure;
	}
	return player_snapshot_capture_result::ok;
}
