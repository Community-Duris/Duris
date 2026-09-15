#include "economy/auction_room_registry.h"

#include <array>
#include <cassert>
#include <unordered_set>

int main()
{
	constexpr std::array<int, AUCTION_HOUSE_REGISTERED_ROOM_COUNT> expected = {
		16885, 83117, 97756, 17736, 55193, 888, 1200, 69, 420, 132821,
	};
	std::unordered_set<int> unique;
	for (std::size_t index = 0; index < expected.size(); ++index)
	{
		int room_vnum = -1;
		assert(auction_house_registered_room_vnum(index, &room_vnum));
		assert(room_vnum == expected[index]);
		assert(room_vnum > 0);
		assert(unique.insert(room_vnum).second);
		assert(auction_house_is_registered_room_vnum(room_vnum));
	}
	int untouched = 73;
	assert(!auction_house_registered_room_vnum(expected.size(), &untouched));
	assert(untouched == 73);
	assert(!auction_house_registered_room_vnum(0, nullptr));
	assert(!auction_house_is_registered_room_vnum(0));
	assert(!auction_house_is_registered_room_vnum(999999));
}
