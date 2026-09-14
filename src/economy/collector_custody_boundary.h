#ifndef DURIS_COLLECTOR_CUSTODY_BOUNDARY_H
#define DURIS_COLLECTOR_CUSTODY_BOUNDARY_H

#include "economy/collector_policy.h"
#include "item/item_transfer_command.h"

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
	const uint64_t actor = payload.to_owner.type == item_owner_type::player ?
				       payload.to_owner.id :
			       payload.from_owner.type == item_owner_type::player ?
				       payload.from_owner.id :
				       0;
	return actor <= std::numeric_limits<uint32_t>::max() ? static_cast<uint32_t>(actor) : 0;
}

#endif
