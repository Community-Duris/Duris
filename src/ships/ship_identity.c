/*****************************************************
 * ship_identity.c
 *
 * Process-local ship identity registry
 *****************************************************/

/*
 * OVERVIEW -- where this file sits in the ship system
 * ---------------------------------------------------
 * Ships are heap objects (struct ShipData) that can be deleted at any time --
 * they sink, they get scuttled, the owner deletes them.  Anything that has to
 * remember "that ship over there" *across a delay* therefore cannot hold a raw
 * P_ship: by the time the delay fires the pointer may dangle, or worse, the
 * allocator may have handed the same address to a brand new ship.
 *
 * This file is the answer to that.  It hands every live ship a ShipRuntimeRef
 * -- a (slot, generation) pair -- that can be stored safely and later resolved
 * back to a P_ship, returning NULL if the ship it named is gone.  The
 * generation counter is what makes address reuse harmless: a recycled slot
 * always carries a fresh generation, so a stale ref fails to match.
 *
 * The registry is deliberately *process-local*: refs are meaningful only
 * inside one running server and are never persisted.  The database identity of
 * a ship is ShipData::db_id, and its player-facing identity is SHIP_ID()
 * (the two-letter contact designation assigned in ship_utils.c).  Do not
 * confuse the three.
 *
 * Who uses this
 * -------------
 * The main consumer is delayed weapon fire.  fire_weapon() (ship_combat.c)
 * builds a VolleyData holding refs to the attacker and the target, and schedules
 * volley_hit_event() to land some pulses later; that handler calls
 * resolve_volley_endpoints() and simply drops the volley if either ship sank in
 * the meantime.
 *
 * Registration is owned by ship_base.c: new_ship() calls
 * register_ship_runtime(), delete_ship() calls unregister_ship_runtime().
 * Nothing else should call either.
 *
 * Invariants
 * ----------
 *   - ShipRuntimeRef::slot is 1-BASED.  Slot 0 means "no identity", which is
 *     what a zeroed ShipData carries, so `!ref.slot` is the null test and the
 *     array subscript is always `ref.slot - 1`.
 *   - ShipRuntimeRef::generation is never 0 for a live ref; the counter starts
 *     at 1 and only ever increases.
 *   - Every deviation from those invariants is a corruption bug, not a
 *     recoverable condition, so it reports through panic_corruption() rather
 *     than being silently tolerated.
 */

#include "core/prototypes.h"
#include "ships/ships.h"

/*
 * One entry in the registry.
 *
 * `ship` is NULL for a free slot.  `generation` is the stamp handed out when
 * the slot was last claimed; it is left in place after the slot is freed so
 * that a stale ShipRuntimeRef naming this slot still fails the comparison in
 * find_ship_by_runtime_ref().
 */
struct ship_runtime_slot
{
	P_ship ship;
	uint64_t generation;
};

/*
 * The registry itself, sized to the same MAXSHIPS ceiling the rest of the ship
 * system uses, and the monotonically increasing generation counter.  Both are
 * zero-initialised, which is exactly the "empty registry, no generations
 * issued" state.
 */
static ship_runtime_slot ship_runtime_slots[MAXSHIPS] = {};
static uint64_t next_ship_runtime_generation = 0;

/*
 * Claim a registry slot for `ship` and return the ref that names it.
 *
 * Called once per ship, by new_ship() in ship_base.c, before the ship is
 * handed to any other subsystem.  The ref is stored on the ship itself
 * (ShipData::runtime_ref) as well as returned, so callers that do not need it
 * can ignore the return value.
 *
 * Returns a zeroed ref -- slot 0, generation 0, the "no identity" value -- for
 * a NULL ship, and reports through panic_corruption() (also returning a zeroed
 * ref) if the ship already holds an identity, if the generation counter has
 * wrapped, or if all MAXSHIPS slots are in use.
 */
ShipRuntimeRef register_ship_runtime(P_ship ship)
{
	if (!ship)
		return {};
	/*
	 * Re-registering would strand the previous slot forever: nothing else
	 * remembers it, so unregister_ship_runtime() could never free it.
	 */
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
		/*
		 * Generation 0 is reserved for "no identity", so a wrap would
		 * start minting refs that compare equal to stale ones.  At one
		 * ship per nanosecond this takes ~584 years; treat it as
		 * unreachable rather than recoverable.
		 */
		if (!generation)
		{
			panic_corruption("ship", "process-local ship identity exhausted");
			return {};
		}
		slot.ship = ship;
		slot.generation = generation;
		/* Note the +1: refs are 1-based so that 0 can mean "none". */
		ship->runtime_ref = { index + 1, generation };
		return ship->runtime_ref;
	}

	panic_corruption("ship", "runtime identity registry exhausted");
	return {};
}

/*
 * Release the registry slot `ship` holds, invalidating every outstanding ref
 * to it.
 *
 * Called from delete_ship() (ship_base.c) as the ship is torn down.  After
 * this returns, find_ship_by_runtime_ref() on any previously issued ref for
 * this ship yields NULL, because the slot's generation stamp no longer
 * matches.  The ship's own runtime_ref is zeroed so a double unregister is
 * caught rather than freeing someone else's slot.
 *
 * A NULL ship is ignored.  A ref that is out of range, or that does not match
 * what the registry believes, is reported through panic_corruption() and the
 * slot is left alone.
 */
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
	/*
	 * Both halves matter: a wrong ->ship means two ships think they own the
	 * slot, a wrong generation means this ref is stale and the slot has
	 * already been reissued to someone else.
	 */
	if (slot.ship != ship || slot.generation != ref.generation)
	{
		panic_corruption("ship", "runtime identity registry mismatch during deletion");
		return;
	}
	slot.ship = NULL;
	ship->runtime_ref = {};
}

/*
 * Resolve a stored ShipRuntimeRef back to a live ship.
 *
 * This is the whole point of the module: it is safe to call with a ref of any
 * age.  Returns NULL when the ref is the zeroed "no identity" value, when it is
 * out of range, or when the ship it named has since been deleted (detected by
 * the generation stamp no longer matching).  Otherwise returns the ship.
 *
 * The returned pointer is only good until the next opportunity for the ship to
 * be deleted -- resolve, use, discard; never cache it.
 */
P_ship find_ship_by_runtime_ref(ShipRuntimeRef ref)
{
	if (!ref.slot || ref.slot > MAXSHIPS || !ref.generation)
		return NULL;

	const ship_runtime_slot &slot = ship_runtime_slots[ref.slot - 1];
	/*
	 * Comparing generations alone is sufficient: a freed slot keeps its old
	 * stamp and a reused one gets a strictly newer stamp, so a stale ref can
	 * never match.  slot.ship is NULL for a freed slot, which is exactly the
	 * value we want to hand back.
	 */
	if (slot.generation != ref.generation)
		return NULL;
	return slot.ship;
}

/*
 * Resolve both endpoints of an in-flight weapon volley at the moment it lands.
 *
 * `volley` is the VolleyData that fire_weapon() (ship_combat.c) attached to the
 * delayed volley_hit_event(); it stores the attacker and target as refs rather
 * than pointers precisely so that either may sink before the shot arrives.
 *
 * `attacker` and `target` are out-parameters and are always written -- they are
 * set to NULL first, so a caller that ignores the return value still sees NULL
 * rather than stale garbage.
 *
 * Returns true only when BOTH ships are still alive, which is the caller's
 * signal to apply the damage.  Returns false -- with both out-parameters NULL
 * -- when the out-parameters are missing, when `volley` is NULL, or when either
 * endpoint has been deleted, in which case the volley is simply dropped.
 */
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
	/*
	 * All-or-nothing: a volley with only one live endpoint has nobody to
	 * credit the damage to, or nobody to apply it to.
	 */
	if (!resolved_attacker || !resolved_target)
		return false;

	*attacker = resolved_attacker;
	*target = resolved_target;
	return true;
}
