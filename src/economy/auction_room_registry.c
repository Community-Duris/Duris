#include "economy/auction_room_registry.h"

#include <array>
#include <algorithm>

namespace
{
constexpr std::array<int, AUCTION_HOUSE_REGISTERED_ROOM_COUNT> room_vnums = {
	16885, 83117, 97756, 17736, 55193, 888, 1200, 69, 420, 132821,
};
}

bool auction_house_registered_room_vnum(std::size_t index, int *room_vnum)
{
	if (!room_vnum || index >= room_vnums.size())
		return false;
	*room_vnum = room_vnums[index];
	return true;
}

bool auction_house_is_registered_room_vnum(int room_vnum)
{
	return std::find(room_vnums.begin(), room_vnums.end(), room_vnum) != room_vnums.end();
}
