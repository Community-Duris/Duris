#include "item/item_get_policy.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "classes/necromancy.h"
#include "item/item_ownership_runtime.h"

extern P_room world;
extern const int top_of_world;
extern int top_of_objt;

namespace
{
constexpr const char *MALFORMED_CONTAINER_MESSAGE = "That container has a malformed item.\r\n";

bool actor_room_is_valid(P_char actor)
{
	return actor && actor->in_room > NOWHERE && actor->in_room <= top_of_world;
}

bool live_placement_outer(P_char actor, P_obj object, P_obj container, P_obj *outer_out)
{
	if (!actor || !object || !outer_out || !actor_room_is_valid(actor))
		return false;

	P_obj outer = container ? container : object;
	if (container && (!OBJ_INSIDE(object) || object->loc.inside != container))
		return false;

	int safety = top_of_objt + 1;
	while (outer && OBJ_INSIDE(outer) && outer->loc.inside)
	{
		if (safety-- <= 0)
		{
			send_to_char(MALFORMED_CONTAINER_MESSAGE, actor);
			return false;
		}
		outer = outer->loc.inside;
	}
	if (!outer)
		return false;
	*outer_out = outer;
	return true;
}

bool live_placement_is_accessible(P_char actor, P_obj outer)
{
	return actor && outer &&
	       ((OBJ_ROOM(outer) && outer->loc.room == actor->in_room) ||
		(OBJ_CARRIED_BY(outer, actor) || OBJ_WORN_BY(outer, actor)));
}

bool live_placement_owner(P_char actor, P_obj object, P_obj container, item_owner_identity *owner)
{
	if (!owner)
		return false;
	*owner = {};
	P_obj outer = NULL;
	if (!live_placement_outer(actor, object, container, &outer))
		return false;
	if (!live_placement_is_accessible(actor, outer))
		return false;

	if (outer->type == ITEM_CORPSE && IS_SET(outer->value[CORPSE_FLAGS], PC_CORPSE) &&
	    outer->value[CORPSE_PID] > 0 && outer->value[CORPSE_SAVEID] > 0)
	{
		*owner = item_owner_identity{
			item_owner_type::corpse,
			item_corpse_owner_id(static_cast<uint32_t>(outer->value[CORPSE_PID]),
					     static_cast<uint32_t>(outer->value[CORPSE_SAVEID])),
			0
		};
		return item_owner_identity_valid(*owner);
	}

	if (OBJ_ROOM(outer) && outer->loc.room == actor->in_room)
	{
		*owner = { item_owner_type::room,
			   static_cast<uint64_t>(world[actor->in_room].number), 0 };
		return item_owner_identity_valid(*owner);
	}

	if (OBJ_CARRIED_BY(outer, actor) || OBJ_WORN_BY(outer, actor))
	{
		*owner = { item_owner_type::player, static_cast<uint64_t>(GET_PID(actor)), 0 };
		return item_owner_identity_valid(*owner);
	}

	/* NPC custody and objects in nowhere have no durable source authority. */
	return false;
}

bool owner_is_virtual_source(item_owner_type type)
{
	return type == item_owner_type::locker || type == item_owner_type::auction ||
	       type == item_owner_type::shopkeeper || type == item_owner_type::collector;
}

bool runtime_owner_matches_live_placement(P_char actor, P_obj object, P_obj container,
					  const item_owner_identity &runtime_owner)
{
	P_obj outer = NULL;
	if (!live_placement_outer(actor, object, container, &outer) ||
	    !live_placement_is_accessible(actor, outer))
		return false;
	if (owner_is_virtual_source(runtime_owner.type))
		/* Locker/auction/shopkeeper/collector authorities are explicit virtual
		 * boundaries and are intentionally not reduced to room ownership. */
		return true;
	if (runtime_owner.type != item_owner_type::player &&
	    runtime_owner.type != item_owner_type::room &&
	    runtime_owner.type != item_owner_type::corpse)
		return false;

	item_owner_identity live_owner = {};
	return live_placement_owner(actor, object, container, &live_owner) &&
	       item_owner_identity_equal(live_owner, runtime_owner);
}
} // namespace

bool item_get_source_owner(P_char actor, P_obj object, P_obj container, item_owner_identity *source)
{
	if (!actor || !object || !source)
		return false;
	*source = {};

	item_ownership_runtime_entry runtime = {};
	if (item_ownership_runtime_lookup(object->obj_uid, &runtime))
	{
		if (runtime.state != item_custody_state::active ||
		    !item_owner_identity_valid(runtime.owner) ||
		    !runtime_owner_matches_live_placement(actor, object, container, runtime.owner))
			return false;
		*source = runtime.owner;
		return true;
	}

	if (container)
	{
		if (container->type == ITEM_CORPSE &&
		    IS_SET(container->value[CORPSE_FLAGS], PC_CORPSE) &&
		    container->value[CORPSE_PID] > 0 && container->value[CORPSE_SAVEID] > 0)
		{
			if (!live_placement_owner(actor, object, container, source))
				return false;
			return source->type == item_owner_type::corpse;
		}

		if (item_ownership_runtime_lookup(container->obj_uid, &runtime))
		{
			if (runtime.state != item_custody_state::active ||
			    !item_owner_identity_valid(runtime.owner) ||
			    !runtime_owner_matches_live_placement(actor, object, container,
								  runtime.owner))
				return false;
			*source = runtime.owner;
			return true;
		}
	}

	return live_placement_owner(actor, object, container, source);
}
