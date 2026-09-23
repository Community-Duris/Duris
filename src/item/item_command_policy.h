#ifndef ITEM_COMMAND_POLICY_H
#define ITEM_COMMAND_POLICY_H

#include "core/structs.h"
#include "item/item_transfer_command.h"

#include <cstdint>

/*
 * Command-facing item policy.  These functions decide whether a command
 * enters the durable item pipeline and resolve its authoritative boundary.
 * They do not mutate live objects or retain pointers across an asynchronous
 * operation.
 */
bool item_command_uses_durable_ownership(P_obj object);
// A raw subtree removal is safe only when neither its root nor any descendant
// can have a generic custody row. This intentionally classifies unadmitted
// nontransient UIDs conservatively so callers fail closed.
bool item_tree_has_durable_ownership(P_obj root);
// Unlike the conservative command predicate, this asks whether any node is
// already present in the active custody registry.
bool item_tree_has_active_custody(P_obj root);
bool item_command_object_is_takeable(P_char actor, P_obj object);
bool item_command_container_is_valid(P_obj container);

struct item_put_destination
{
	P_obj target_container;
	item_owner_identity owner;
	item_transfer_reason reason;
	int64_t reason_id;
};

bool item_command_resolve_put_destination(P_char actor, P_obj container,
					  item_put_destination *destination);
bool item_command_resolve_drop_destination(P_char actor, item_owner_identity *destination,
					   item_transfer_reason *reason, int64_t *reason_id);

#endif
