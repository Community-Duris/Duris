#include "cmd/lockpick_retirement.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "economy/economic_gameplay_authority.h"
#include "item/item_ownership_runtime.h"
#include "player/player_revision_state.h"
#include "persistence/persistence_checkpoint.h"
#include "net/comm.h"

extern P_obj object_list;

namespace
{
bool exact_actor(P_char actor, uint32_t pid) noexcept
{
	return actor && IS_PC(actor) && actor->only.pc && GET_PID(actor) > 0 &&
	       static_cast<uint32_t>(GET_PID(actor)) == pid && actor->runtime_id &&
	       find_character_by_runtime_id(actor->runtime_id) == actor;
}
void refused(P_char actor)
{
	send_to_char("Your lockpick cracks, but remains intact.\r\n", actor);
}
}

bool lockpick_retirement_publish_physical(const critical_operation_id &operation, P_char actor,
					  bool committed, const item_transfer_result &result,
					  const lockpick_retirement_terms &terms,
					  bool notify) noexcept
try
{
	if (!exact_actor(actor, terms.actor_pid) || critical_operation_id_is_zero(operation) ||
	    !terms.item_uid || terms.item_uid == UINT64_MAX || terms.item_vnum <= 0 ||
	    terms.branch < lockpick_retirement_branch::failed_container ||
	    terms.branch > lockpick_retirement_branch::wear)
		return false;
	if (!committed)
	{
		if (notify)
			refused(actor);
		return true;
	}
	if (result.root_item_uid != terms.item_uid || result.item_count != 1 ||
	    !result.max_item_revision || result.max_item_revision == UINT64_MAX)
		return false;
	item_ownership_runtime_entry retired = {};
	// Live apply or the shared owner's authenticated cold AFTER must precede this.
	// Missing runtime evidence is never inferred from an absent physical object.
	if (!item_ownership_runtime_lookup(terms.item_uid, &retired) ||
	    retired.item_uid != terms.item_uid || retired.root_item_uid != terms.item_uid ||
	    retired.parent_item_uid || retired.vnum != terms.item_vnum ||
	    retired.state != item_custody_state::destroyed ||
	    retired.owner.type != item_owner_type::destruction || retired.owner.id ||
	    retired.owner.context_id || retired.item_revision != result.max_item_revision)
		return false;
	P_obj pick = nullptr;
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == terms.item_uid)
		{
			if (pick)
				return false;
			pick = object;
		}
	// ACK retry or authentic omission on cold load: never replay messages/RNG.
	if (!pick)
		return true;
	if (pick->type != ITEM_PICK || OBJ_VNUM(pick) != terms.item_vnum || pick->contains ||
	    actor->equipment[HOLD] != pick || !OBJ_WORN_BY(pick, actor))
		return false;
	if (notify)
	{
		act(terms.branch == lockpick_retirement_branch::wear ?
			    "Damn!  But you broke your $p!" :
			    "Damn!  You broke your $p too!",
		    FALSE, actor, pick, 0, TO_CHAR);
		act("$n begins cursing under $s breath as $s $p snaps.", FALSE, actor, pick, 0,
		    TO_ROOM);
	}
	unequip_char(actor, HOLD);
	extract_obj(pick, TRUE);
	mark_player_dirty_components(GET_PID(actor), PLAYER_COMPONENT_STATUS |
							     PLAYER_COMPONENT_EQUIPMENT |
							     PLAYER_COMPONENT_INVENTORY);
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == terms.item_uid)
			return false;
	return true;
}
catch (...)
{
	return false;
}

bool lockpick_retirement_publication(const critical_operation_id &operation, P_char actor,
				     bool committed, const item_transfer_result &result,
				     unsigned int, const uint8_t *encoded,
				     size_t encoded_size) noexcept
{
	lockpick_retirement_terms terms;
	return encoded && lockpick_retirement_decode({ encoded, encoded_size }, &terms) &&
	       lockpick_retirement_publish_physical(operation, actor, committed, result, terms,
						    true);
}

bool lockpick_retirement_submit(P_char actor, P_obj pick,
				lockpick_retirement_branch branch) noexcept
try
{
	if (!economic_gameplay_authority::active() || !actor || !IS_PC(actor) || !actor->only.pc ||
	    GET_PID(actor) <= 0 || !actor->runtime_id ||
	    find_character_by_runtime_id(actor->runtime_id) != actor || !pick ||
	    actor->equipment[HOLD] != pick || !OBJ_WORN_BY(pick, actor) ||
	    pick->type != ITEM_PICK || pick->contains || !pick->obj_uid ||
	    pick->obj_uid == UINT64_MAX || OBJ_VNUM(pick) <= 0)
		return false;
	const item_owner_identity owner = { item_owner_type::player,
					    static_cast<uint32_t>(GET_PID(actor)), 0 };
	item_ownership_runtime_entry original = {};
	if (!item_ownership_runtime_lookup(pick->obj_uid, &original) ||
	    original.item_uid != pick->obj_uid || original.root_item_uid != pick->obj_uid ||
	    original.parent_item_uid || !item_owner_identity_equal(original.owner, owner) ||
	    original.vnum != OBJ_VNUM(pick) || original.state != item_custody_state::active ||
	    !original.item_revision || original.item_revision == UINT64_MAX)
	{
		refused(actor);
		return false;
	}
	item_transfer_continuation continuation;
	// Reserved shared kind8; the unextended shared validator refuses it safely.
	continuation.kind = static_cast<item_transfer_continuation_kind>(8);
	if (!lockpick_retirement_encode({ branch, pick->obj_uid,
					  static_cast<uint32_t>(GET_PID(actor)), OBJ_VNUM(pick) },
					&continuation.data))
	{
		refused(actor);
		return false;
	}
	item_movement_reject reject = item_movement_reject::none;
	if (!item_movement_transaction_prepare_sql_lockpick_retirement(
		    actor, pick, continuation, lockpick_retirement_publication, &reject))
	{
		refused(actor);
		return false;
	}
	return true;
}
catch (...)
{
	// A shared submit may have admitted the original before throwing. Never
	// extract, retry with a new operation, or label this ambiguous cut rejected.
	return false;
}
