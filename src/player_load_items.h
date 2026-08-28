#ifndef PLAYER_LOAD_ITEMS_H
#define PLAYER_LOAD_ITEMS_H

#include "player_load_repository.h"

#include <cstddef>
#include <cstdint>

struct char_data;
typedef struct char_data *P_char;
struct obj_data;
typedef struct obj_data *P_obj;

enum class player_load_item_materialize_outcome : uint8_t
{
	applied,
	invalid_snapshot,
	limit_exceeded,
	allocation_failure,
	unknown_prototype,
	ownership_failure,
};

struct player_load_item_materialize_metrics
{
	player_load_item_materialize_outcome outcome =
		player_load_item_materialize_outcome::invalid_snapshot;
	size_t item_count = 0;
	size_t operation_count = 0;
	size_t maximum_depth = 0;
};

bool player_load_items_materialize(P_char character, const player_load_result &result,
				   player_load_item_materialize_metrics *metrics);
bool player_load_item_graph_materialize(P_char character,
					const std::vector<player_item_snapshot> &items,
					const std::vector<player_load_item_identity> &identities,
					int32_t pid, uint64_t owner_revision,
					bool hydrate_ownership,
					player_load_item_materialize_metrics *metrics);
bool player_load_item_graph_materialize_for_owner(
	P_char character, const std::vector<player_item_snapshot> &items,
	const std::vector<player_load_item_identity> &identities,
	const item_owner_identity &expected_owner, uint64_t owner_revision, bool hydrate_ownership,
	bool complete_snapshot_state, player_load_item_materialize_metrics *metrics);
bool player_load_item_graph_materialize_detached(
	const std::vector<player_item_snapshot> &items,
	const std::vector<player_load_item_identity> &identities,
	const item_owner_identity &expected_owner, uint64_t owner_revision, bool hydrate_ownership,
	bool complete_snapshot_state, std::vector<P_obj> *roots,
	player_load_item_materialize_metrics *metrics);
void player_load_items_activate_equipment(P_char character);
void player_load_items_discard(P_char character);

#endif
