#ifndef FORCED_WEAPON_DROP_H
#define FORCED_WEAPON_DROP_H

#include "core/structs.h"

#include <cstdint>

enum class forced_weapon_drop_cause : uint8_t
{
	combat_fumble,
	critical_disarm,
};

enum class forced_weapon_drop_result : uint8_t
{
	rejected,
	retained,
	dropped,
	pending,
};

/*
 * Remove an equipped weapon from the actor's grasp and, when durable ownership
 * permits it, transfer the weapon to the room through the item movement
 * authority. A retained result means the weapon was safely moved to inventory
 * instead of being published to the floor.
 */
forced_weapon_drop_result forced_weapon_drop(P_char actor, P_obj weapon,
					     forced_weapon_drop_cause cause);

#endif
