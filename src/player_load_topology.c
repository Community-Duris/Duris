#include "player_load_repository.h"

#include <new>
#include <unordered_map>
#include <vector>

bool player_load_reconcile_item_topology(std::vector<player_item_snapshot> *items,
					 std::vector<player_load_item_identity> *identities,
					 size_t *promoted_item_rows, size_t *repaired_item_rows)
{
	if (!items || !identities || !promoted_item_rows || !repaired_item_rows ||
	    items->size() != identities->size() || items->size() > PLAYER_LOAD_ITEM_MAX)
		return false;

	constexpr size_t no_parent = static_cast<size_t>(-1);
	const size_t item_count = items->size();
	std::unordered_map<uint64_t, size_t> database_indices;
	std::unordered_map<uint64_t, size_t> uid_indices;
	std::vector<size_t> parents;
	std::vector<bool> promoted;
	std::vector<uint8_t> visit_state;
	std::vector<size_t> depth;
	try
	{
		database_indices.reserve(item_count);
		uid_indices.reserve(item_count);
		parents.assign(item_count, no_parent);
		promoted.assign(item_count, false);
		visit_state.assign(item_count, 0);
		depth.assign(item_count, 0);
		for (size_t index = 0; index < item_count; ++index)
		{
			const player_item_snapshot &item = (*items)[index];
			const player_load_item_identity &identity = (*identities)[index];
			if (!identity.database_id || !identity.item_uid ||
			    identity.item_uid != item.object_uid ||
			    !database_indices.emplace(identity.database_id, index).second ||
			    !uid_indices.emplace(identity.item_uid, index).second)
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}

	for (size_t index = 0; index < item_count; ++index)
	{
		player_item_snapshot &item = (*items)[index];
		player_load_item_identity &identity = (*identities)[index];
		if (!identity.parent_item_uid)
		{
			if (identity.serialized_parent_id)
				++*repaired_item_rows;
			identity.serialized_parent_id = 0;
			item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			continue;
		}

		const auto parent = uid_indices.find(identity.parent_item_uid);
		if (parent == uid_indices.end())
		{
			identity.serialized_parent_id = 0;
			identity.parent_item_uid = 0;
			identity.root_item_uid = identity.item_uid;
			item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			promoted[index] = true;
			++*promoted_item_rows;
			continue;
		}
		if (parent->second == index || parent->second > static_cast<size_t>(INT32_MAX))
			return false;
		parents[index] = parent->second;
		const uint64_t resolved_database_id = (*identities)[parent->second].database_id;
		if (identity.serialized_parent_id != resolved_database_id)
			++*repaired_item_rows;
		identity.serialized_parent_id = resolved_database_id;
		item.parent_index = static_cast<int32_t>(parent->second);
	}

	// Resolve parents before children even when a repaired parent appears later in the
	// serialized row order. This also rejects cycles and unreasonably deep graphs.
	for (size_t start = 0; start < item_count; ++start)
	{
		if (visit_state[start] == 2)
			continue;
		std::vector<size_t> path;
		size_t current = start;
		try
		{
			while (current != no_parent && visit_state[current] == 0)
			{
				visit_state[current] = 1;
				path.push_back(current);
				current = parents[current];
			}
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		if (current != no_parent && visit_state[current] == 1)
			return false;
		while (!path.empty())
		{
			const size_t node = path.back();
			path.pop_back();
			const size_t parent = parents[node];
			depth[node] = parent == no_parent ? 1 : depth[parent] + 1;
			if (depth[node] > PLAYER_SNAPSHOT_MAX_DEPTH)
				return false;
			if (parent == no_parent)
			{
				if ((*identities)[node].root_item_uid !=
				    (*identities)[node].item_uid)
					return false;
			}
			else if (promoted[parent])
			{
				promoted[node] = true;
				(*identities)[node].root_item_uid =
					(*identities)[parent].root_item_uid;
			}
			else if ((*identities)[node].root_item_uid !=
				 (*identities)[parent].root_item_uid)
				return false;
			visit_state[node] = 2;
		}
	}
	return true;
}
