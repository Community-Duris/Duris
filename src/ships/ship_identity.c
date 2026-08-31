#include "core/prototypes.h"
#include "ships/ships.h"

struct ship_runtime_slot
{
	P_ship ship;
	uint64_t generation;
};

static ship_runtime_slot ship_runtime_slots[MAXSHIPS] = {};
static uint64_t next_ship_runtime_generation = 0;

ShipRuntimeRef register_ship_runtime(P_ship ship)
{
	if (!ship)
		return {};
	if (ship->runtime_ref.slot || ship->runtime_ref.generation)
	{
		panic_corruption("ship", "attempted to register an existing runtime identity");
		return {};
	}

	for (uint64_t index = 0; index < MAXSHIPS; index++)
	{
		ship_runtime_slot &slot = ship_runtime_slots[index];
		if (slot.ship)
			continue;

		const uint64_t generation = ++next_ship_runtime_generation;
		if (!generation)
		{
			panic_corruption("ship", "process-local ship identity exhausted");
			return {};
		}
		slot.ship = ship;
		slot.generation = generation;
		ship->runtime_ref = { index + 1, generation };
		return ship->runtime_ref;
	}

	panic_corruption("ship", "runtime identity registry exhausted");
	return {};
}

void unregister_ship_runtime(P_ship ship)
{
	if (!ship)
		return;

	const ShipRuntimeRef ref = ship->runtime_ref;
	if (!ref.slot || ref.slot > MAXSHIPS || !ref.generation)
	{
		panic_corruption("ship", "attempted to unregister an invalid runtime identity");
		return;
	}

	ship_runtime_slot &slot = ship_runtime_slots[ref.slot - 1];
	if (slot.ship != ship || slot.generation != ref.generation)
	{
		panic_corruption("ship", "runtime identity registry mismatch during deletion");
		return;
	}
	slot.ship = NULL;
	ship->runtime_ref = {};
}

P_ship find_ship_by_runtime_ref(ShipRuntimeRef ref)
{
	if (!ref.slot || ref.slot > MAXSHIPS || !ref.generation)
		return NULL;

	const ship_runtime_slot &slot = ship_runtime_slots[ref.slot - 1];
	if (slot.generation != ref.generation)
		return NULL;
	return slot.ship;
}

bool resolve_volley_endpoints(const VolleyData *volley, P_ship *attacker, P_ship *target)
{
	if (!attacker || !target)
		return false;
	*attacker = NULL;
	*target = NULL;
	if (!volley)
		return false;

	P_ship resolved_attacker = find_ship_by_runtime_ref(volley->attacker);
	P_ship resolved_target = find_ship_by_runtime_ref(volley->target);
	if (!resolved_attacker || !resolved_target)
		return false;

	*attacker = resolved_attacker;
	*target = resolved_target;
	return true;
}
