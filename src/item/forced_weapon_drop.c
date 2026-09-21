#include "item/forced_weapon_drop.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "item/item_command_policy.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "net/comm.h"
#include "persistence/persistence_checkpoint.h"
#include "redis/redis_floor_runtime.h"

#include <cstring>

extern P_obj object_list;
extern P_room world;
extern const int top_of_world;

namespace
{
struct forced_weapon_drop_context
{
	uint64_t item_uid;
	int32_t room;
	uint8_t cause;
	uint8_t floor_hint;
	uint8_t reserved[2];
};

static_assert(sizeof(forced_weapon_drop_context) <= ITEM_MOVEMENT_CONTEXT_MAX_BYTES);

int equipped_slot(P_char actor, P_obj weapon)
{
	if (!actor || !weapon)
		return MAX_WEAR;
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (actor->equipment[slot] == weapon)
			return slot;
	return MAX_WEAR;
}

P_obj find_live_weapon(uint64_t item_uid)
{
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == item_uid)
			return object;
	return NULL;
}

bool valid_room(int room)
{
	return room >= 0 && room <= top_of_world;
}

void announce_recovery(P_char actor, P_obj weapon, int room, forced_weapon_drop_cause cause)
{
	if (!actor || !weapon || cause != forced_weapon_drop_cause::combat_fumble)
		return;
	act("&-L&+YYou lose control of your&n $q&-L&+Y, but recover it before it hits "
	    "the ground!&n\r\n",
	    FALSE, actor, weapon, 0, TO_CHAR);
	if (actor->in_room == room)
		act("$n stumbles with $s attack, but recovers $s weapon!", TRUE, actor, 0, 0,
		    TO_ROOM);
}

void announce_drop(P_char actor, P_obj weapon, int room, forced_weapon_drop_cause cause)
{
	if (!actor || !weapon || cause != forced_weapon_drop_cause::combat_fumble)
		return;
	if (actor->in_room == room)
	{
		act("&-L&+YYou swing at your foe _really_ badly, sending your&n "
		    "$q&-L&+Y flying!&n\r\n",
		    FALSE, actor, weapon, 0, TO_CHAR);
		act("$n stumbles with $s attack, sending $s weapon flying!", TRUE, actor, 0, 0,
		    TO_ROOM);
		return;
	}
	act("&-L&+YThe&n $q&-L&+Y you fumbled lands where you lost it.&n\r\n", FALSE, actor, weapon,
	    0, TO_CHAR);
	act("$p clatters to the ground after being sent flying!", TRUE, 0, weapon, 0, TO_ROOM);
}

bool stage_in_inventory(P_char actor, P_obj weapon, int slot)
{
	P_obj removed = unequip_char(actor, slot);
	if (removed != weapon)
	{
		logit(LOG_FILE,
		      "forced_weapon_drop: outcome=stage_failed cause=unequip actor_pid=%d "
		      "item_uid=%llu",
		      IS_PC(actor) ? GET_PID(actor) : 0, (unsigned long long)weapon->obj_uid);
		return false;
	}
	obj_to_char(weapon, actor);
	if (OBJ_CARRIED_BY(weapon, actor))
		return true;
	logit(LOG_FILE,
	      "forced_weapon_drop: outcome=stage_failed cause=inventory actor_pid=%d "
	      "item_uid=%llu",
	      IS_PC(actor) ? GET_PID(actor) : 0, (unsigned long long)weapon->obj_uid);
	return false;
}

bool player_owns_root(P_char actor, P_obj weapon, item_owner_identity *owner)
{
	if (!actor || !weapon || !owner || !IS_PC(actor) || GET_PID(actor) <= 0)
		return false;
	*owner = { item_owner_type::player, static_cast<uint64_t>(GET_PID(actor)), 0 };
	item_ownership_runtime_entry runtime = {};
	return item_ownership_runtime_lookup(weapon->obj_uid, &runtime) &&
	       runtime.state == item_custody_state::active &&
	       item_owner_identity_equal(runtime.owner, *owner) &&
	       runtime.root_item_uid == weapon->obj_uid && runtime.parent_item_uid == 0;
}

bool forced_weapon_drop_publication(P_char actor, bool committed, const item_transfer_result &,
				    unsigned int, const uint8_t *encoded, size_t encoded_size)
{
	forced_weapon_drop_context context = {};
	if (!actor || !encoded || encoded_size != sizeof(context))
		return false;
	memcpy(&context, encoded, sizeof(context));
	if (!valid_room(context.room) ||
	    context.cause > static_cast<uint8_t>(forced_weapon_drop_cause::critical_disarm))
		return false;

	P_obj weapon = find_live_weapon(context.item_uid);
	if (!weapon)
		return false;
	const auto cause = static_cast<forced_weapon_drop_cause>(context.cause);
	if (!committed)
	{
		if (!OBJ_CARRIED_BY(weapon, actor))
			return false;
		announce_recovery(actor, weapon, context.room, cause);
		return true;
	}

	// A failed publication acknowledgement retries this callback. Environmental
	// room logic may already have redirected or started dropping the weapon, so
	// any floor placement is the idempotent post-publication state.
	if (OBJ_ROOM(weapon))
		return true;
	if (!OBJ_CARRIED_BY(weapon, actor))
		return false;

	obj_from_char(weapon);
	obj_to_room(weapon, context.room);
	if (!OBJ_ROOM(weapon))
		return false;

	const int landing_room = weapon->loc.room;
	if (context.floor_hint)
		redis_log_floor_drop(weapon, world[landing_room].number);
	mark_player_dirty_components(GET_PID(actor), PLAYER_COMPONENT_STATUS |
							     PLAYER_COMPONENT_EQUIPMENT |
							     PLAYER_COMPONENT_INVENTORY);
	char_light(actor);
	if (valid_room(actor->in_room))
		room_light(actor->in_room, REAL);
	if (landing_room != actor->in_room)
		room_light(landing_room, REAL);
	announce_drop(actor, weapon, context.room, cause);
	return true;
}
} // namespace

forced_weapon_drop_result forced_weapon_drop(P_char actor, P_obj weapon,
					     forced_weapon_drop_cause cause)
{
	const int room = actor ? actor->in_room : NOWHERE;
	const int slot = equipped_slot(actor, weapon);
	if (!actor || !weapon || slot == MAX_WEAR || !valid_room(room))
	{
		announce_recovery(actor, weapon, room, cause);
		return forced_weapon_drop_result::rejected;
	}

	const bool durable_player_weapon = IS_PC(actor) && GET_PID(actor) > 0 &&
					   item_command_uses_durable_ownership(weapon);
	if (!durable_player_weapon)
	{
		P_obj removed = unequip_char(actor, slot);
		if (removed != weapon)
		{
			announce_recovery(actor, weapon, room, cause);
			return forced_weapon_drop_result::rejected;
		}
		obj_to_room(weapon, room);
		char_light(actor);
		room_light(room, REAL);
		announce_drop(actor, weapon, room, cause);
		return forced_weapon_drop_result::dropped;
	}

	item_owner_identity source = {};
	if (!player_owns_root(actor, weapon, &source))
	{
		logit(LOG_FILE,
		      "forced_weapon_drop: outcome=rejected cause=ownership actor_pid=%d "
		      "item_uid=%llu",
		      GET_PID(actor), (unsigned long long)weapon->obj_uid);
		announce_recovery(actor, weapon, room, cause);
		return forced_weapon_drop_result::rejected;
	}
	if (IS_OBJ_STAT2(weapon, ITEM2_CRUMBLELOOT) && !IS_TRUSTED(actor))
	{
		logit(LOG_FILE,
		      "forced_weapon_drop: outcome=rejected cause=crumbleloot actor_pid=%d "
		      "item_uid=%llu",
		      GET_PID(actor), (unsigned long long)weapon->obj_uid);
		announce_recovery(actor, weapon, room, cause);
		return forced_weapon_drop_result::rejected;
	}

	if (!stage_in_inventory(actor, weapon, slot))
	{
		announce_recovery(actor, weapon, room, cause);
		return forced_weapon_drop_result::rejected;
	}

	item_owner_identity destination = {};
	item_transfer_reason reason = item_transfer_reason::unknown;
	int64_t reason_id = 0;
	item_movement_reject reject = item_movement_reject::none;
	if (!item_command_resolve_drop_destination(actor, &destination, &reason, &reason_id))
	{
		announce_recovery(actor, weapon, room, cause);
		return forced_weapon_drop_result::retained;
	}
	const forced_weapon_drop_context context = {
		weapon->obj_uid,
		room,
		static_cast<uint8_t>(cause),
		static_cast<uint8_t>(reason == item_transfer_reason::player_drop),
		{},
	};
	if (!item_movement_transaction_submit(actor, weapon, NULL, source, destination, reason,
					      reason_id, NULL, &context, sizeof(context), NULL,
					      &reject, forced_weapon_drop_publication))
	{
		logit(LOG_FILE, "forced_weapon_drop: outcome=%s actor_pid=%d item_uid=%llu",
		      item_movement_reject_name(reject), GET_PID(actor),
		      (unsigned long long)weapon->obj_uid);
		announce_recovery(actor, weapon, room, cause);
		return forced_weapon_drop_result::retained;
	}
	return forced_weapon_drop_result::pending;
}
