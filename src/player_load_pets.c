#include "player_load_pets.h"

#include "player_load_items.h"
#include "prototypes.h"
#include "structs.h"
#include "utils.h"

#include <algorithm>
#include <new>
#include <unordered_set>

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
		if (mobile_number < 0)
		{
			player_load_pets_discard(pets);
			return fail(metrics,
				    player_load_pet_materialize_outcome::unknown_prototype);
		}
		P_char pet = read_mobile(mobile_number, REAL);
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
		GET_HIT(pet) = snapshot.hit;
		GET_MAX_HIT(pet) = snapshot.max_hit;
		GET_MANA(pet) = snapshot.mana;
		GET_MAX_MANA(pet) = snapshot.max_mana;
		GET_VITALITY(pet) = snapshot.vitality;
		GET_MAX_VITALITY(pet) = snapshot.max_vitality;
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
		setup_pet(pet, owner, result.snapshot.pets[index].charm_duration, PET_NOAGGRO);
		add_follower(pet, owner);
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
