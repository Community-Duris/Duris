#ifndef DURIS_COLLECTOR_CUSTODY_BOUNDARY_H
#define DURIS_COLLECTOR_CUSTODY_BOUNDARY_H

#include "economy/collector_policy.h"
#include "item/item_transfer_command.h"
#include "persistence/corpse_lifecycle_command.h"

#include <cstdint>
#include <limits>

// Classify only successful durable custody boundaries. Environmental movement
// (corpse decay, room relocation, and restoration) deliberately returns none:
// it must not disqualify an otherwise unclaimed death item. Every player or
// shopkeeper boundary is proof of acquisition, even if a stale candidate is
// only discovered on a later drop/put, and the command's full item list makes
// container cancellation transitive without a history scan.
inline collector::reason
collector_item_transfer_boundary_reason(const item_transfer_payload &payload)
{
	// Mob and pet inventory is intentionally represented by the existing room,
	// corpse, or player aggregate. The explicit reason is durable evidence that
	// the live object nevertheless changed hands.
	if (payload.reason == item_transfer_reason::mobile_claim)
		return collector::reason::claimed;
	if (payload.reason == item_transfer_reason::corpse_raise_pet &&
	    payload.to_owner.type == item_owner_type::pet)
		return collector::reason::claimed;
	if (payload.to_owner.type == item_owner_type::destruction ||
	    payload.reason == item_transfer_reason::destruction)
		return collector::reason::destroyed;
	if (payload.from_owner.type == item_owner_type::player ||
	    payload.to_owner.type == item_owner_type::player ||
	    payload.from_owner.type == item_owner_type::shopkeeper ||
	    payload.to_owner.type == item_owner_type::shopkeeper)
		return collector::reason::claimed;
	return collector::reason::none;
}

inline uint32_t collector_item_transfer_actor_pid(const item_transfer_payload &payload)
{
	if (payload.reason == item_transfer_reason::mobile_claim && payload.reason_id > 0 &&
	    static_cast<uint64_t>(payload.reason_id) <= std::numeric_limits<uint32_t>::max())
		return static_cast<uint32_t>(payload.reason_id);
	if (payload.reason == item_transfer_reason::corpse_raise_pet &&
	    payload.to_owner.type == item_owner_type::pet &&
	    payload.to_owner.context_id <= std::numeric_limits<uint32_t>::max())
		return static_cast<uint32_t>(payload.to_owner.context_id);
	const uint64_t actor =
		payload.to_owner.type == item_owner_type::player   ? payload.to_owner.id :
		payload.from_owner.type == item_owner_type::player ? payload.from_owner.id :
								     0;
	return actor <= std::numeric_limits<uint32_t>::max() ? static_cast<uint32_t>(actor) : 0;
}

// A corpse disappearing is not, by itself, proof that its contents were
// claimed.  Decay/release into a room only relocates environmental custody.
// Resurrection and a raised follower hand every durable item to a player
// aggregate, while destruction is terminal.  A corpse decaying inside a
// player-owned container is likewise the first durable player boundary for
// any candidate that survived acquisition of the outer corpse.
inline collector::reason
collector_corpse_lifecycle_boundary_reason(const corpse_lifecycle_payload &payload)
{
	switch (payload.action)
	{
	case corpse_lifecycle_action::destroy:
		return collector::reason::destroyed;
	case corpse_lifecycle_action::resurrect:
	case corpse_lifecycle_action::raise_follower:
	case corpse_lifecycle_action::raise_world_follower:
		return collector::reason::claimed;
	case corpse_lifecycle_action::release_nested:
		return payload.destination_player_pid ? collector::reason::claimed :
							collector::reason::none;
	case corpse_lifecycle_action::upsert:
	case corpse_lifecycle_action::remove:
	case corpse_lifecycle_action::release:
		return collector::reason::none;
	}
	return collector::reason::none;
}

inline uint32_t collector_corpse_lifecycle_actor_pid(const corpse_lifecycle_payload &payload)
{
	return collector_corpse_lifecycle_boundary_reason(payload) == collector::reason::claimed ?
		       payload.destination_player_pid :
		       0;
}

struct collector_custody_boundary_item
{
	uint64_t item_uid = 0;
	uint64_t post_item_revision = 0;
};

#endif
