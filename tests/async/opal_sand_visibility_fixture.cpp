#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <strings.h>

// Actor/lighting policy is fixed; visibility, aliases and indexed lookup are production code.
index_data indexes[1]{};
P_index obj_index = indexes;
bool has_innate(P_char, int)
{
	return false;
}
int get_vis_mode(P_char, int)
{
	return 2;
}
int ship_obj_proc(P_obj, P_char, int, char *)
{
	return false;
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

// PRODUCTION_FUNCTIONS

int main(int argc, char **argv)
{
	assert(argc == 2);
	char_data owner{};
	owner.player.level = 8;
	owner.specials.position = POS_STANDING + STAT_NORMAL;
	char keywords[] = "large container fine sand";
	obj_data sand{};
	sand.name = keywords;
	sand.type = ITEM_OTHER;
	sand.extra_flags = static_cast<unsigned>(std::strtoul(argv[1], nullptr, 10));
	sand.loc_p = LOC_CARRIED;
	sand.loc.carrying = &owner;
	owner.carrying = &sand;
	indexes[0].virtual_number = 70823;
	assert(IS_SET(sand.extra_flags, ITEM_NORESET));
	for (const char *alias : { "large", "container", "fine", "sand", "1.sand" })
	{
		if (!CAN_SEE_OBJ(&owner, &sand) ||
		    get_obj_in_list_vis(&owner, alias, owner.carrying, false) != &sand ||
		    get_obj_in_list_vis(&owner, alias, owner.carrying, true) != &sand)
		{
			std::fprintf(stderr, "Sand reward cannot be selected by its player: %s\n",
				     alias);
			return 1;
		}
	}
	assert(!get_obj_in_list_vis(&owner, "quill", owner.carrying, false));
	assert(!get_obj_in_list_vis(&owner, "2.sand", owner.carrying, false));
	SET_BIT(sand.extra_flags, ITEM_SECRET);
	assert(!CAN_SEE_OBJ(&owner, &sand));
	assert(!get_obj_in_list_vis(&owner, "sand", owner.carrying, false));
	REMOVE_BIT(sand.extra_flags, ITEM_SECRET);
	SET_BIT(owner.specials.affected_by, AFF_BLIND);
	assert(!get_obj_in_list_vis(&owner, "sand", owner.carrying, false));
	std::puts(
		"Production visibility and alias lookup accept the sand reward; hidden/blind/wrong/duplicate selection is rejected.");
}
