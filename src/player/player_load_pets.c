#include "player/player_load_pets.h"

#include "player/player_load_items.h"
#include "player/pet_restore_runtime.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"

#include <algorithm>
#include <new>
#include <unordered_set>
#include <ctime>

namespace
{
bool fail(player_load_pet_materialize_metrics *metrics, player_load_pet_materialize_outcome outcome)
{
	if (metrics)
		metrics->outcome = outcome;
	return false;
}

bool valid_pet(const player_pet_snapshot &pet)
{
	return pet.mob_vnum > 0 && pet.order >= 0 &&
	       pet.order < static_cast<int32_t>(PLAYER_LOAD_PET_MAX) && pet.max_hit > 0 &&
	       pet.hit >= 0 && pet.hit <= pet.max_hit && pet.max_mana >= 0 && pet.mana >= 0 &&
	       pet.mana <= pet.max_mana && pet.max_vitality >= 0 && pet.vitality >= 0 &&
	       pet.vitality <= pet.max_vitality && pet.charm_duration >= -1 && pet.room_vnum > 0;
}
}

void player_load_pets_discard(std::vector<P_char> *pets)
{
	if (!pets)
		return;
	for (P_char pet : *pets)
		if (pet)
		{
			player_load_items_discard(pet);
			extract_char(pet);
		}
	pets->clear();
}

bool player_load_pets_stage(P_char owner, const player_load_result &result,
			    std::vector<P_char> *pets, player_load_pet_materialize_metrics *metrics)
{
	player_load_pet_materialize_metrics local_metrics = {};
	if (!metrics)
		metrics = &local_metrics;
	*metrics = {};
	if (!owner || !IS_PC(owner) || !pets || !pets->empty() || result.pid <= 0 ||
	    result.snapshot.pets.size() != result.pet_identities.size())
		return fail(metrics, player_load_pet_materialize_outcome::invalid_snapshot);
	metrics->pet_count = result.snapshot.pets.size();
	if (metrics->pet_count > PLAYER_LOAD_PET_MAX)
		return fail(metrics, player_load_pet_materialize_outcome::limit_exceeded);
	try
	{
		pets->reserve(metrics->pet_count);
	}
	catch (const std::bad_alloc &)
	{
		return fail(metrics, player_load_pet_materialize_outcome::allocation_failure);
	}
	std::unordered_set<uint64_t> database_ids;
	std::unordered_set<int32_t> orders;
	int remaining_power = summoned_pet_capacity(owner);
	int remaining_golem_power = IS_TRUSTED(owner) ? remaining_power : GET_LEVEL(owner) / 3;
	try
	{
		database_ids.reserve(metrics->pet_count);
		orders.reserve(metrics->pet_count);
	}
	catch (const std::bad_alloc &)
	{
		return fail(metrics, player_load_pet_materialize_outcome::allocation_failure);
	}
	for (size_t index = 0; index < result.snapshot.pets.size(); ++index)
	{
		const player_pet_snapshot &snapshot = result.snapshot.pets[index];
		const player_load_pet_identity &identity = result.pet_identities[index];
		bool unique = false;
		try
		{
			unique = database_ids.insert(identity.database_id).second &&
				 orders.insert(snapshot.order).second;
		}
		catch (const std::bad_alloc &)
		{
			player_load_pets_discard(pets);
			return fail(metrics,
				    player_load_pet_materialize_outcome::allocation_failure);
		}
		if (!identity.database_id || !valid_pet(snapshot) || !unique ||
		    snapshot.room_vnum != result.snapshot.room_vnum ||
		    snapshot.items.size() != identity.item_identities.size() ||
		    snapshot.items.size() > PLAYER_LOAD_ITEM_MAX - metrics->item_count)
		{
			player_load_pets_discard(pets);
			return fail(metrics, player_load_pet_materialize_outcome::invalid_snapshot);
		}
		const int mobile_number = real_mobile(snapshot.mob_vnum);
		pet_restore_state state;
		const bool has_state = !snapshot.restore_state.empty();
		pet_hold_reason reason = snapshot.hold_reason;
		if (reason == pet_hold_reason::none && has_state &&
		    !pet_restore_state_decode(snapshot.restore_state, &state))
			reason = pet_hold_reason::invalid_state;
		if (reason == pet_hold_reason::none && !has_state &&
		    legacy_summon_prototype(snapshot.mob_vnum))
			reason = pet_hold_reason::legacy_summon;
		if (reason == pet_hold_reason::none && has_state &&
		    (!summoned_pet_matches_prototype(state.kind, snapshot.mob_vnum) ||
		     state.race > LAST_RACE || !(state.act & ACT_ISNPC)))
			reason = pet_hold_reason::invalid_state;
		if (reason == pet_hold_reason::none && mobile_number < 0)
			reason = pet_hold_reason::missing_prototype;
		const int cost = has_state ? summoned_pet_cost(state.kind) : 0;
		const auto kind = static_cast<uint32_t>(state.kind);
		const bool golem = has_state && kind >= 15 && kind <= 18;
		const int64_t now = time(nullptr);
		if (reason == pet_hold_reason::none && has_state &&
		    ((state.charm_expires_at && state.charm_expires_at <= now) ||
		     (state.death_expires_at && state.death_expires_at <= now)))
			reason = pet_hold_reason::expired;
		if (reason == pet_hold_reason::none &&
		    (cost > remaining_power || (golem && cost > remaining_golem_power)))
			reason = pet_hold_reason::over_capacity;
		if (reason != pet_hold_reason::none)
		{
			try
			{
				if (!owner->only.pc->held_pets)
					owner->only.pc->held_pets = new player_held_pet_state;
				auto held = snapshot;
				held.hold_reason = reason;
				owner->only.pc->held_pets->pets.push_back(std::move(held));
				pets->push_back(
					nullptr); // retain snapshot/identity index correspondence
				metrics->item_count += snapshot.items.size();
			}
			catch (const std::bad_alloc &)
			{
				player_load_pets_discard(pets);
				return fail(
					metrics,
					player_load_pet_materialize_outcome::allocation_failure);
			}
			continue;
		}
		remaining_power -= cost;
		if (golem)
			remaining_golem_power -= cost;
		P_char pet = read_mobile(mobile_number, REAL, false);
		if (!pet)
		{
			player_load_pets_discard(pets);
			return fail(metrics,
				    player_load_pet_materialize_outcome::allocation_failure);
		}
		try
		{
			pets->push_back(pet);
		}
		catch (const std::bad_alloc &)
		{
			extract_char(pet);
			player_load_pets_discard(pets);
			return fail(metrics,
				    player_load_pet_materialize_outcome::allocation_failure);
		}
		player_load_item_materialize_metrics item_metrics = {};
		if (has_state && !summoned_pet_apply(pet, state))
		{
			player_load_pets_discard(pets);
			return fail(metrics, player_load_pet_materialize_outcome::invalid_snapshot);
		}
		if (has_state)
			affect_total(pet, FALSE);
		if (!player_load_item_graph_materialize(
			    pet, snapshot.items, identity.item_identities, result.pid,
			    result.item_owner_revision, false, &item_metrics))
		{
			player_load_pets_discard(pets);
			return fail(metrics, player_load_pet_materialize_outcome::item_failure);
		}
		metrics->item_count += item_metrics.item_count;
		metrics->operation_count += item_metrics.operation_count;
		metrics->maximum_depth =
			std::max(metrics->maximum_depth, item_metrics.maximum_depth);
		if (!has_state)
		{
			GET_MAX_HIT(pet) = snapshot.max_hit;
			GET_MAX_MANA(pet) = snapshot.max_mana;
			GET_MAX_VITALITY(pet) = snapshot.max_vitality;
		}
		GET_HIT(pet) = std::min(snapshot.hit, GET_MAX_HIT(pet));
		GET_MANA(pet) = std::min<int>(snapshot.mana, GET_MAX_MANA(pet));
		GET_VITALITY(pet) = std::min<int>(snapshot.vitality, GET_MAX_VITALITY(pet));
		metrics->operation_count += PLAYER_LOAD_PET_OPERATIONS_PER_PET;
	}
	metrics->outcome = player_load_pet_materialize_outcome::applied;
	return true;
}

void player_load_pets_commit(P_char owner, std::vector<P_char> *pets,
			     const player_load_result &result)
{
	if (!owner || !pets || pets->size() != result.snapshot.pets.size())
		return;
	for (size_t index = 0; index < pets->size(); ++index)
	{
		P_char pet = (*pets)[index];
		if (!pet)
			continue;
		pet_restore_state state;
		if (pet_restore_state_decode(result.snapshot.pets[index].restore_state, &state))
			summoned_pet_restore_lifetime(pet, owner, state);
		else
			setup_pet(pet, owner, result.snapshot.pets[index].charm_duration,
				  PET_NOAGGRO | PET_RESTORE);
		add_follower(pet, owner);
	}
	if (owner->only.pc->held_pets && !owner->only.pc->held_pets->pets.empty())
	{
		logit(LOG_FILE, "pet recovery held pid=%d pets=%zu; saved equipment retained",
		      GET_PID(owner), owner->only.pc->held_pets->pets.size());
		if (owner->desc)
			send_to_char(
				"Some saved pets require staff review. Their saved equipment has been retained.\r\n",
				owner);
	}
	pets->clear();
}

void player_load_pets_place(P_char owner)
{
	if (!owner || owner->in_room == NOWHERE)
		return;
	for (follow_type *follow = owner->followers; follow; follow = follow->next)
		if (follow->follower && IS_NPC(follow->follower) &&
		    follow->follower->in_room == NOWHERE)
		{
			char_to_room(follow->follower, owner->in_room, FALSE);
			player_load_items_activate_equipment(follow->follower);
		}
}
