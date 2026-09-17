#include "item/item_command_policy.h"

#include "core/utils.h"
#include "classes/necromancy.h"
#include "item/item_ownership_runtime.h"
#include "item/storage_lockers.h"

extern P_room world;

bool item_command_uses_durable_ownership(P_obj object)
{
	if (!object || object->obj_uid == 0 || object->type == ITEM_MONEY ||
	    (object->type == ITEM_CORPSE && IS_SET(object->value[CORPSE_FLAGS], PC_CORPSE)))
		return false;
	if (!IS_SET(object->extra_flags, ITEM_TRANSIENT))
		return true;

	item_ownership_runtime_entry ownership = {};
	return item_ownership_runtime_lookup(object->obj_uid, &ownership) &&
	       ownership.state == item_custody_state::active;
}

bool item_command_object_is_takeable(P_char actor, P_obj object)
{
	return actor && object &&
	       (CAN_WEAR(object, ITEM_TAKE) || ((GET_LEVEL(actor) >= 60) && !IS_NPC(actor)));
}

bool item_command_container_is_valid(P_obj container)
{
	return container && ((GET_ITEM_TYPE(container) == ITEM_CONTAINER) ||
			     (GET_ITEM_TYPE(container) == ITEM_STORAGE) ||
			     (GET_ITEM_TYPE(container) == ITEM_QUIVER) ||
			     (GET_ITEM_TYPE(container) == ITEM_CORPSE));
}

bool item_command_resolve_put_destination(P_char actor, P_obj container,
					  item_put_destination *destination)
{
	if (!actor || !container || !destination || !container->obj_uid ||
	    !item_command_container_is_valid(container))
		return false;

	*destination = {};
	if (locker_owner_for_container(actor, container, &destination->owner))
	{
		destination->target_container = NULL;
		destination->reason = item_transfer_reason::locker_deposit;
		destination->reason_id = static_cast<int64_t>(destination->owner.context_id);
		return true;
	}

	item_ownership_runtime_entry runtime = {};
	if (!item_ownership_runtime_lookup(container->obj_uid, &runtime) ||
	    runtime.state != item_custody_state::active ||
	    !item_owner_identity_valid(runtime.owner))
		return false;

	destination->target_container = container;
	destination->owner = runtime.owner;
	destination->reason = item_transfer_reason::player_put;
	destination->reason_id = static_cast<int64_t>(container->obj_uid);
	return true;
}

bool item_command_resolve_drop_destination(P_char actor, item_owner_identity *destination,
					   item_transfer_reason *reason, int64_t *reason_id)
{
	if (!actor || !destination || !reason || !reason_id || actor->in_room == NOWHERE)
		return false;

	*destination = { item_owner_type::room, static_cast<uint64_t>(world[actor->in_room].number),
			 0 };
	*reason = item_transfer_reason::player_drop;
	*reason_id = world[actor->in_room].number;
	if (locker_owner_for_room(actor, destination))
	{
		*reason = item_transfer_reason::locker_deposit;
		*reason_id = 0;
	}
	return true;
}
