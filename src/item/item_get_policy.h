#ifndef ITEM_GET_POLICY_H
#define ITEM_GET_POLICY_H

#include "core/structs.h"
#include "item/item_transfer_command.h"

/*
 * Resolve the durable authority for a pickup from the live object topology.
 * The returned identity is a value; callers must still revalidate the object
 * and container before publishing an asynchronous movement.
 */
bool item_get_source_owner(P_char actor, P_obj object, P_obj container,
			   item_owner_identity *source);

#endif
