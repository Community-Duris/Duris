#include "combat/attack_continuation.h"

#include "core/prototypes.h"
#include "core/utils.h"

extern P_obj object_list;

namespace
{
int locate_weapon_slot(P_char actor, P_obj weapon)
{
	if (!actor || !weapon)
		return -1;
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (actor->equipment[slot] == weapon)
			return slot;
	return -1;
}

P_char resolve_character(P_char expected, uint64_t runtime_id)
{
	if (!expected || !runtime_id)
		return nullptr;
	P_char live = find_character_by_runtime_id(runtime_id);
	return live == expected ? live : nullptr;
}

P_obj resolve_live_object(P_obj expected, uint64_t uid) noexcept
{
	if (!expected)
		return nullptr;
	for (P_obj object = object_list; object; object = object->next)
		if (object == expected && object->obj_uid == uid)
			return object;
	return nullptr;
}

attack_continuation_result rejected(attack_continuation_outcome outcome)
{
	return { outcome, nullptr, nullptr, nullptr };
}
} // namespace

attack_continuation begin_attack_continuation(P_char actor, P_char target, P_obj weapon,
					      int weapon_slot) noexcept
{
	attack_continuation continuation{};
	continuation.actor = actor;
	continuation.target = target;
	continuation.weapon = weapon;
	if (!actor || !target)
		return continuation;

	continuation.actor_runtime_id = actor->runtime_id;
	continuation.target_runtime_id = target->runtime_id;
	continuation.room = actor->in_room;
	continuation.height = actor->specials.z_cord;
	continuation.weapon_slot = weapon_slot >= 0 ? weapon_slot :
						      locate_weapon_slot(actor, weapon);
	if (weapon)
		continuation.weapon_uid = weapon->obj_uid;
	return continuation;
}

attack_continuation_result
check_attack_continuation(const attack_continuation &continuation) noexcept
{
	if (!continuation.actor || !continuation.target || !continuation.actor_runtime_id ||
	    !continuation.target_runtime_id)
		return rejected(attack_continuation_outcome::cancelled);

	P_char actor = resolve_character(continuation.actor, continuation.actor_runtime_id);
	if (!actor || !IS_ALIVE(actor))
		return rejected(attack_continuation_outcome::actor_gone);

	P_char target = resolve_character(continuation.target, continuation.target_runtime_id);
	if (!target || !IS_ALIVE(target))
		return rejected(attack_continuation_outcome::target_gone);

	if (actor->in_room != continuation.room || target->in_room != continuation.room ||
	    actor->specials.z_cord != continuation.height ||
	    target->specials.z_cord != continuation.height)
		return rejected(attack_continuation_outcome::relocated);

	if (continuation.weapon_slot < -1 || continuation.weapon_slot >= MAX_WEAR)
		return rejected(attack_continuation_outcome::cancelled);

	P_obj weapon = continuation.weapon;
	if (continuation.weapon_slot >= 0)
	{
		P_obj selected = actor->equipment[continuation.weapon_slot];
		if (selected != continuation.weapon)
			return rejected(attack_continuation_outcome::weapon_changed);
		if (selected && resolve_live_object(selected, continuation.weapon_uid) != selected)
			return rejected(attack_continuation_outcome::weapon_changed);
		weapon = selected;
	}
	else if (continuation.weapon &&
		 resolve_live_object(continuation.weapon, continuation.weapon_uid) !=
			 continuation.weapon)
	{
		return rejected(attack_continuation_outcome::weapon_changed);
	}

	return { attack_continuation_outcome::continue_attack, actor, target, weapon };
}
