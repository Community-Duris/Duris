// This component fixture cannot provide a native world.
// Unexpected world access aborts; these seams never certify publication.
#include <cstdlib>
#include "economy/auction_command.h"
#include "core/prototypes.h"
int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}
bool nevent_is_game_thread()
{
	std::abort();
}
bool auction_native_expected_player_forest(const auction_command_payload &,
					   std::span<const player_item_snapshot>,
					   std::span<const player_item_snapshot>, bool, uint32_t,
					   std::vector<player_item_snapshot> *) noexcept
{
	std::abort();
}
