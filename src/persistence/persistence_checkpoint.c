#include "persistence/persistence_checkpoint.h"

#include "core/files.h"
#include "player/player_save_pipeline.h"
#include "player/player_save_worker.h"
#include "core/prototypes.h"
#include "redis/redis_floor_runtime.h"
#include "core/utils.h"

#include <new>
#include <vector>

extern int _pwipe;
extern const int top_of_world;
extern struct room_data *world;
extern P_char character_list;

namespace
{
int room_vnum(P_char character)
{
	if (!character || character->in_room < 0 || character->in_room > top_of_world)
		return NOWHERE;
	return world[character->in_room].number;
}
} // namespace

void mark_player_dirty(int pid)
{
	if (_pwipe)
		return;
	mark_player_dirty_components(pid, PLAYER_CHECKPOINT_COMPONENT_ALL);
}

void mark_player_dirty_components(int pid, player_component_mask_t components)
{
	if (_pwipe)
		return;
	player_save_pipeline_mark(pid, components);
}

void flush_dirty_players(void)
{
	for (P_char character = character_list; character; character = character->next)
		if (IS_PC(character) && GET_PID(character) > 0)
			player_save_pipeline_checkpoint_dirty(character, RENT_CRASH,
							      room_vnum(character));
}

int get_dirty_player_count(void)
{
	return static_cast<int>(player_save_pipeline_dirty_count());
}

struct persistence_dirty_save_snapshot persistence_dirty_save_snapshot_copy(void)
{
	struct persistence_dirty_save_snapshot snapshot = {};
	const player_save_pipeline_health pipeline = player_save_pipeline_health_copy();
	const player_save_worker_health worker = player_save_worker_health_copy();
	snapshot.enabled = 1;
	snapshot.available = pipeline.initialized ? 1 : 0;
	snapshot.active_count = player_save_pipeline_dirty_count();
	snapshot.inflight_count = worker.inflight_pids;
	snapshot.inflight_oldest_age_msec = worker.oldest_age_msec;
	return snapshot;
}

void event_flush_dirty_players(P_char /*ch*/, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	constexpr size_t DIRTY_PLAYER_BATCH_SIZE = 8;
	static std::vector<uint64_t> character_ids;
	static size_t cursor = 0;

	if (character_ids.empty())
	{
		try
		{
			for (P_char character = character_list; character;
			     character = character->next)
				if (IS_PC(character) && GET_PID(character) > 0 &&
				    character->runtime_id)
					character_ids.push_back(character->runtime_id);
		}
		catch (const std::bad_alloc &)
		{
			character_ids.clear();
			cursor = 0;
			nevent_periodic_retry_after(WAIT_SEC,
						    "dirty-player snapshot allocation failed");
			return;
		}
	}

	size_t processed = 0;
	while (cursor < character_ids.size() && processed < DIRTY_PLAYER_BATCH_SIZE)
	{
		P_char character = find_character_by_runtime_id(character_ids[cursor++]);
		processed++;
		if (character && IS_PC(character) && GET_PID(character) > 0)
			player_save_pipeline_checkpoint_dirty(character, RENT_CRASH,
							      room_vnum(character));
	}

	if (cursor < character_ids.size())
	{
		nevent_periodic_continue_after(1);
		return;
	}

	character_ids.clear();
	cursor = 0;
	if (redis_floor_runtime_enabled())
		redis_flush_floor_drops();
}
