#ifndef SHOP_ITEM_RUNTIME_PAYLOAD_H
#define SHOP_ITEM_RUNTIME_PAYLOAD_H

#include "player/player_snapshot.h"
#include <map>

struct char_data;
// Pure physical capture only: no SQL/custody proof, adoption or live mutation.
// Failure preserves the supplied output, including for an invalid forest.
bool shop_item_runtime_capture_keeper_literal(char_data *,
					      std::vector<player_item_snapshot> *) noexcept;

#ifndef __NO_MYSQL__
#include <mysql/mysql.h>
#include <span>

struct obj_data;

// Borrow the caller's original reconnect-disabled transaction. This sidecar
// has no ownership/topology authority; physical rows and current custody do.
struct shop_item_runtime_row
{
	uint64_t id = 0, parent_id = 0, root_uid = 0, revision = 0;
	int16_t slot = 0;
	player_item_snapshot item{};
	// Original locked sidecar presence, retained for exact before/after readback.
	// It is a value observation and grants no checkpoint/write authority.
	bool payload_present = false;
};
using shop_item_runtime_image = std::map<uint64_t, shop_item_runtime_row>;
bool shop_item_runtime_storage_available(MYSQL *, bool *available) noexcept;
bool shop_item_runtime_read(MYSQL *, bool keeper, uint64_t item_id, player_item_snapshot *,
			    bool *present) noexcept;
bool shop_item_runtime_verify(MYSQL *, bool keeper, uint64_t item_id, uint64_t owner_id,
			      uint64_t parent_id, const player_item_snapshot &) noexcept;
bool shop_item_runtime_write(MYSQL *, bool keeper, uint64_t item_id,
			     const player_item_snapshot &) noexcept;
// Read-only value proof in the caller's original transaction. The caller must
// already hold authority/player/bank, keeper and item-owner revision locks.
// No checkpoint write/commit/adoption authority is granted by this image.
// Legacy rows prove SQL-represented values only, not unstored runtime/order.
bool shop_item_runtime_lock_checkpoint_image(
	MYSQL *, uint64_t keeper_id, uint32_t shop_id, int32_t keeper_vnum,
	std::span<const player_item_snapshot> original_keeper_items,
	shop_item_runtime_image *output) noexcept;
// Complete current EQ/INV proof against the original holder's saved forest,
// preserving each item's actual captured string policy and sidecar bytes.
// Borrowed reconnect-disabled transaction only; caller locks the player and all
// player custody before any keeper/physical-image locks. No mutation/adoption,
// publication or ACK authority. Missing literal sidecars refuse ENODATA; legacy
// base rows alone cannot prove generated keys, secondary timers or native order.
// Empty forest is valid. Failure preserves the supplied output.
bool shop_item_runtime_lock_player_image(MYSQL *, uint32_t pid,
					 std::span<const player_item_snapshot> original_player_items,
					 shop_item_runtime_image *output) noexcept;
// Cold recovery counterpart: retained canonical UID order supplies identity,
// never item values. Read actual canonical sidecars and prove the complete current
// forest under the same original global custody-before-physical lock cut.
// Reconstruct parent_index from matching custody/physical parents in this order;
// preserve stored string policy, slot and all other values. Caller must compare
// the reconstructed complete canonical body to its retained role-bound manifest.
// This reader does not authenticate that manifest, admission or publication.
// Empty forest is valid; missing sidecars refuse. Failure preserves output.
bool shop_item_runtime_lock_player_image(MYSQL *, uint32_t pid,
					 std::span<const uint64_t> retained_player_uids,
					 shop_item_runtime_image *output) noexcept;
bool shop_item_runtime_keeper_image(MYSQL *, uint64_t keeper_id, uint32_t shop_id,
				    int32_t keeper_vnum, shop_item_runtime_image *) noexcept;
// Cold native loader counterpart. Borrow the caller's original reconnect-disabled
// transaction after its keeper serialization lock. Retain the unchanged full
// keeper-image proof and lock the actual owner counter/current custody. These
// values grant no transaction/session cleanup, native placement/enrollment,
// registry adoption/mutation or ACK. Wholly legacy image stays empty. Failure
// preserves both outputs. Caller hydrates only its successfully restored native
// stage atomically after original transaction finish, before keeper publication.
struct item_ownership_runtime_entry;
bool shop_item_runtime_keeper_image(MYSQL *, uint64_t keeper_id, uint32_t shop_id,
				    int32_t keeper_vnum, shop_item_runtime_image *,
				    std::vector<item_ownership_runtime_entry> *) noexcept;
bool shop_item_runtime_refresh_image(
	MYSQL *, uint64_t keeper_id, uint32_t shop_id, int32_t keeper_vnum, char_data *,
	shop_item_runtime_image *, bool *complete_out = nullptr,
	std::vector<player_item_snapshot> *literal_out = nullptr) noexcept;
// Only an unpublished native object. Properties already applied dynamic
// affects once; this restores missing runtime fields and descriptor order.
bool shop_item_runtime_restore_stage(obj_data *, const player_item_snapshot &) noexcept;
#endif
#endif
