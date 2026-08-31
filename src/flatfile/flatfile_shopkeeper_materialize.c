#include "flatfile/flatfile_shopkeeper_materialize.h"

#include "flatfile/flatfile_shopkeeper_ownership.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"

#include <climits>
#include <cstdint>
#include <limits>
#include <vector>

namespace
{
constexpr size_t shopkeeper_affect_maximum = 4096;

class staged_shopkeeper
{
    public:
	explicit staged_shopkeeper(P_char value)
		: character(value)
	{
	}
	staged_shopkeeper(const staged_shopkeeper &) = delete;
	staged_shopkeeper &operator=(const staged_shopkeeper &) = delete;
	~staged_shopkeeper()
	{
		if (character)
			extract_char(character);
	}
	P_char release()
	{
		P_char value = character;
		character = nullptr;
		return value;
	}

    private:
	P_char character;
};

bool valid_affects(const std::vector<flatfile_shopkeeper_affect_record> &affects)
{
	if (affects.size() > shopkeeper_affect_maximum)
		return false;
	for (const auto &affect : affects)
	{
		if (affect.type < std::numeric_limits<sh_int>::min() ||
		    affect.type > std::numeric_limits<sh_int>::max() || affect.location < 0 ||
		    affect.location > std::numeric_limits<ubyte>::max())
			return false;
		for (uint64_t bitvector : affect.bitvectors)
			if (bitvector > ULONG_MAX)
				return false;
	}
	return true;
}
}

flatfile_shopkeeper_materialize_result
flatfile_shopkeeper_materialize(const std::string &root, const flatfile_shopkeeper_record &record,
				flatfile_materialized_shopkeeper *materialized,
				player_load_item_materialize_metrics *item_metrics,
				std::string *error)
{
	if (!materialized || root.empty() || record.mob_vnum <= 0 || record.room_vnum <= 0 ||
	    !record.revision || record.saved_at < 0 || !valid_affects(record.affects))
		return flatfile_shopkeeper_materialize_result::invalid;
	const int mobile_rnum = real_mobile(record.mob_vnum);
	if (mobile_rnum < 0)
		return flatfile_shopkeeper_materialize_result::unknown_mobile;
	const int room_rnum = real_room(record.room_vnum);
	if (room_rnum == NOWHERE)
		return flatfile_shopkeeper_materialize_result::unknown_room;

	uint64_t owner_revision = 0;
	std::vector<player_load_item_identity> identities;
	const auto ownership = flatfile_shopkeeper_load_item_ownership(
		root, record, &owner_revision, &identities, error);
	if (ownership != flatfile_shopkeeper_ownership_result::ok)
		return ownership == flatfile_shopkeeper_ownership_result::not_found ?
			       flatfile_shopkeeper_materialize_result::not_found :
		       ownership == flatfile_shopkeeper_ownership_result::io_error ?
			       flatfile_shopkeeper_materialize_result::io_error :
			       flatfile_shopkeeper_materialize_result::invalid;

	P_char character = read_mobile(mobile_rnum, REAL);
	if (!character)
		return flatfile_shopkeeper_materialize_result::allocation_failure;
	staged_shopkeeper staged(character);
	if (!IS_SHOPKEEPER(character))
		return flatfile_shopkeeper_materialize_result::invalid;
	GET_BIRTHPLACE(character) = record.room_vnum;
	for (const auto &saved : record.affects)
	{
		struct affected_type affect = {};
		affect.type = static_cast<sh_int>(saved.type);
		affect.duration = saved.duration;
		affect.modifier = saved.modifier;
		affect.location = static_cast<ubyte>(saved.location);
		affect.bitvector = static_cast<unsigned long>(saved.bitvectors[0]);
		affect.bitvector2 = static_cast<unsigned long>(saved.bitvectors[1]);
		affect.bitvector3 = static_cast<unsigned long>(saved.bitvectors[2]);
		affect.bitvector4 = static_cast<unsigned long>(saved.bitvectors[3]);
		affect.bitvector5 = static_cast<unsigned long>(saved.bitvectors[4]);
		affect_to_char(character, &affect);
	}
	if (!player_load_item_graph_materialize_for_owner(
		    character, record.items, identities,
		    flatfile_shopkeeper_item_owner(record.shop_id), owner_revision, true, true,
		    item_metrics))
		return flatfile_shopkeeper_materialize_result::item_failure;

	flatfile_materialized_shopkeeper output = {};
	output.character = staged.release();
	output.room_rnum = room_rnum;
	output.owner_revision = owner_revision;
	*materialized = output;
	return flatfile_shopkeeper_materialize_result::ok;
}
