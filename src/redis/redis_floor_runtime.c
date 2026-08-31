#include "redis/redis_floor_runtime.h"

#include "core/prototypes.h"
#include "redis/redis_floor_store.h"
#include "redis/redis_key_registry.h"
#include "redis/redis_namespace.h"
#include "core/utils.h"
#include "world/world_recovery_codec.h"
#include "world/world_recovery_pipeline.h"

#include <ctime>
#include <new>
#include <vector>

extern int _pwipe;

namespace
{
constexpr int max_floor_drop_batch = 1024;
constexpr size_t floor_drop_record_max_bytes =
	sizeof(world_recovery_object_record) +
	WORLD_RECOVERY_MAX_ITEM_TREE * sizeof(world_recovery_item_snapshot);

struct pending_floor_drop
{
	unsigned long uid;
	size_t record_size;
	unsigned char record[floor_drop_record_max_bytes];
};

bool floor_runtime_enabled = false;
bool floor_runtime_quiesced = false;
bool floor_runtime_materializing = false;
char floor_key[128] = {};
char floor_index_key[128] = {};
pending_floor_drop floor_drop_batch[max_floor_drop_batch] = {};
int floor_drop_batch_count = 0;
unsigned long floor_drop_removes[max_floor_drop_batch] = {};
int floor_drop_remove_count = 0;
} // namespace

bool redis_floor_runtime_configure(const char *key_namespace, uint64_t epoch)
{
	floor_runtime_enabled = false;
	if (!key_namespace || !*key_namespace || !epoch ||
	    !redis_namespace_season_key(key_namespace, epoch, REDIS_FLOOR_DROPS_SUFFIX, floor_key,
					sizeof floor_key) ||
	    !redis_namespace_season_key(key_namespace, epoch, REDIS_FLOOR_DROP_INDEX_SUFFIX,
					floor_index_key, sizeof floor_index_key))
		return false;
	return true;
}

void redis_floor_runtime_reset(void)
{
	floor_runtime_enabled = false;
	floor_runtime_quiesced = false;
	floor_runtime_materializing = false;
	floor_key[0] = '\0';
	floor_index_key[0] = '\0';
	floor_drop_batch_count = 0;
	floor_drop_remove_count = 0;
}

void redis_floor_runtime_set_enabled(bool enabled)
{
	floor_runtime_enabled = enabled;
}

void redis_floor_runtime_set_quiesced(bool quiesced)
{
	floor_runtime_quiesced = quiesced;
}

void redis_floor_runtime_set_materializing(bool active)
{
	floor_runtime_materializing = active;
}

bool redis_floor_runtime_enabled(void)
{
	return floor_runtime_enabled;
}

void redis_log_floor_drop(P_obj obj, int room_vnum)
{
	if (_pwipe)
		return;
#ifndef __NO_REDIS__
	if (!floor_runtime_enabled || floor_runtime_quiesced || !obj || obj->obj_uid == 0)
		return;

	if (OBJ_VNUM(obj) == 2 && obj->value[6] > 0)
	{
		const time_t corpse_time = static_cast<time_t>(obj->value[6]);
		if (time(NULL) - corpse_time > 86400)
			return;
	}

	if (floor_drop_batch_count >= max_floor_drop_batch)
	{
		redis_flush_floor_drops();
		if (floor_drop_batch_count >= max_floor_drop_batch)
		{
			logit(LOG_SYS, "redis: floor delta retry buffer is full");
			return;
		}
	}

	const int index = floor_drop_batch_count;
	const int size = world_recovery_write_object_to_buffer(
		obj, room_vnum, reinterpret_cast<char *>(floor_drop_batch[index].record),
		sizeof(floor_drop_batch[index].record));
	if (size <= 0)
		return;
	floor_drop_batch[index].uid = obj->obj_uid;
	floor_drop_batch[index].record_size = static_cast<size_t>(size);
	++floor_drop_batch_count;
#endif
}

bool redis_flush_floor_drops(void)
{
#ifndef __NO_REDIS__
	if (!floor_runtime_enabled || floor_runtime_quiesced)
		return true;
	if (!redis_floor_store_health_copy().initialized || !floor_key[0] || !floor_index_key[0])
		return false;

	if (floor_drop_batch_count == 0 && floor_drop_remove_count == 0)
		return true;
	std::vector<redis_floor_mutation> mutations;
	try
	{
		mutations.reserve(floor_drop_remove_count + floor_drop_batch_count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (int index = 0; index < floor_drop_remove_count; ++index)
		mutations.push_back({ floor_drop_removes[index], NULL, 0, true });

	for (int index = 0; index < floor_drop_batch_count; ++index)
		mutations.push_back({ floor_drop_batch[index].uid, floor_drop_batch[index].record,
				      floor_drop_batch[index].record_size, false, true });

	if (!redis_floor_store_submit(floor_key, floor_index_key, mutations.data(),
				      mutations.size()))
		return false;

	floor_drop_remove_count = 0;
	floor_drop_batch_count = 0;
	return true;
#else
	return false;
#endif
}

void redis_remove_floor_drop(unsigned long obj_uid)
{
#ifndef __NO_REDIS__
	if (!floor_runtime_enabled || floor_runtime_quiesced || floor_runtime_materializing ||
	    obj_uid == 0)
		return;

	for (int index = 0; index < floor_drop_batch_count; ++index)
	{
		if (floor_drop_batch[index].uid == obj_uid)
		{
			--floor_drop_batch_count;
			if (index != floor_drop_batch_count)
				floor_drop_batch[index] = floor_drop_batch[floor_drop_batch_count];
			return;
		}
	}

	if (floor_drop_remove_count < max_floor_drop_batch)
		floor_drop_removes[floor_drop_remove_count++] = obj_uid;
#else
	(void)obj_uid;
#endif
}
