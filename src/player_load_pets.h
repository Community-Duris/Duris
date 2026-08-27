#ifndef PLAYER_LOAD_PETS_H
#define PLAYER_LOAD_PETS_H

#include "player_load_repository.h"

#include <cstddef>
#include <vector>

struct char_data;
typedef struct char_data *P_char;

enum class player_load_pet_materialize_outcome : uint8_t
{
	applied,
	invalid_snapshot,
	limit_exceeded,
	allocation_failure,
	unknown_prototype,
	item_failure,
};

struct player_load_pet_materialize_metrics
{
	player_load_pet_materialize_outcome outcome =
		player_load_pet_materialize_outcome::invalid_snapshot;
	size_t pet_count = 0;
	size_t item_count = 0;
	size_t operation_count = 0;
	size_t maximum_depth = 0;
};

bool player_load_pets_stage(P_char owner, const player_load_result &result,
			    std::vector<P_char> *pets,
			    player_load_pet_materialize_metrics *metrics);
void player_load_pets_commit(P_char owner, std::vector<P_char> *pets,
			     const player_load_result &result);
void player_load_pets_discard(std::vector<P_char> *pets);
void player_load_pets_place(P_char owner);

#endif
