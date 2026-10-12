#ifndef DURIS_ITEM_TRANSFER_ROOM_PARENTS_H
#define DURIS_ITEM_TRANSFER_ROOM_PARENTS_H

#include "player/player_load_repository.h"
#include <span>

enum class item_transfer_room_parent_error : uint8_t
{
	ok,
	unsupported,
	invalid,
	stale,
	malformed,
	allocation_failure,
};

// Pure identity enumeration from a complete BEFORE graph. The caller still
// authenticates the original session/owner cut and computes complete literal
// AFTER values. Listing an ancestor neither authorizes mutation nor requires
// an item revision change when its literal is unchanged after clamping.
// Cached item owner revisions may precede the fenced current owner clock.
// Returns each affected ancestor once, sorted by UID, with strong output.
item_transfer_room_parent_error
item_transfer_room_parent_candidates(const item_transfer_payload &, const item_owner_identity &,
				     uint64_t owner_revision,
				     std::span<const player_load_item_identity>,
				     std::vector<player_load_item_identity> *) noexcept;

#endif
