#include "item/item_transfer_room_parents.h"

#include <algorithm>
#include <new>
#include <unordered_map>
#include <unordered_set>

item_transfer_room_parent_error
item_transfer_room_parent_candidates(const item_transfer_payload &payload,
				     const item_owner_identity &owner, uint64_t owner_revision,
				     std::span<const player_load_item_identity> before,
				     std::vector<player_load_item_identity> *output) noexcept
{
	using error = item_transfer_room_parent_error;
	const bool put = payload.reason == item_transfer_reason::player_put &&
			 payload.from_owner.type == item_owner_type::player &&
			 payload.to_owner.type == item_owner_type::room;
	const bool get = payload.reason == item_transfer_reason::player_get &&
			 payload.from_owner.type == item_owner_type::room &&
			 payload.to_owner.type == item_owner_type::player;
	if (!put && !get)
		return error::unsupported;
	if (!output || owner.type != item_owner_type::room || !item_owner_identity_valid(owner) ||
	    !payload.item_count || payload.item_count > payload.items.size() ||
	    before.size() > PLAYER_LOAD_ITEM_MAX)
		return error::invalid;
	if (!item_owner_identity_equal(owner, put ? payload.to_owner : payload.from_owner) ||
	    owner_revision != (put ? payload.expected_to_revision : payload.expected_from_revision))
		return error::stale;
	try
	{
		std::unordered_map<uint64_t, const player_load_item_identity *> rows;
		std::unordered_set<uint64_t> selected, ancestors;
		rows.reserve(before.size());
		selected.reserve(payload.item_count);
		for (const auto &row : before)
		{
			if (!row.item_uid || !row.root_item_uid || !row.item_revision ||
			    row.state != item_custody_state::active ||
			    !item_owner_identity_equal(row.owner, owner) ||
			    row.owner_revision > owner_revision ||
			    !rows.emplace(row.item_uid, &row).second)
				return error::malformed;
		}
		for (size_t index = 0; index < payload.item_count; ++index)
			if (!payload.items[index].item_uid ||
			    !selected.insert(payload.items[index].item_uid).second ||
			    (put && rows.contains(payload.items[index].item_uid)))
				return error::malformed;
		// Validate even selected paths and untouched siblings: a cycle entirely
		// within selected rows must not disappear when finding outside parents.
		for (const auto &row : before)
		{
			const auto *current = &row;
			size_t depth = 0;
			while (true)
			{
				if (++depth > PLAYER_SNAPSHOT_MAX_DEPTH ||
				    current->root_item_uid != row.root_item_uid)
					return error::malformed;
				if (!current->parent_item_uid)
				{
					if (current->item_uid != row.root_item_uid)
						return error::malformed;
					break;
				}
				const auto parent = rows.find(current->parent_item_uid);
				if (parent == rows.end())
					return error::malformed;
				current = parent->second;
			}
		}
		std::vector<player_load_item_identity> result;
		const auto walk = [&](uint64_t uid, uint64_t root) -> error
		{
			size_t depth = 0;
			while (uid)
			{
				if (++depth > before.size() || selected.contains(uid))
					return error::malformed;
				const auto found = rows.find(uid);
				if (found == rows.end() || found->second->root_item_uid != root)
					return error::malformed;
				const auto &row = *found->second;
				if (!row.parent_item_uid && row.item_uid != root)
					return error::malformed;
				if (ancestors.insert(uid).second)
					result.push_back(row);
				uid = row.parent_item_uid;
			}
			return error::ok;
		};
		if (put)
		{
			if (!payload.target_parent_item_uid || !payload.target_root_item_uid)
				return error::invalid;
			const auto parent = rows.find(payload.target_parent_item_uid);
			if (parent == rows.end() || parent->second->item_revision !=
							    payload.expected_target_parent_revision)
				return error::stale;
			const auto status =
				walk(payload.target_parent_item_uid, payload.target_root_item_uid);
			if (status != error::ok)
				return status;
		}
		else
			for (size_t index = 0; index < payload.item_count; ++index)
			{
				const auto &entry = payload.items[index];
				const auto found = rows.find(entry.item_uid);
				if (found == rows.end())
					return error::stale;
				const auto &row = *found->second;
				if (row.root_item_uid != entry.root_item_uid ||
				    row.parent_item_uid != entry.parent_item_uid ||
				    row.item_revision != entry.expected_item_revision ||
				    row.state != entry.expected_state)
					return error::stale;
				if (selected.contains(row.parent_item_uid))
					continue;
				const auto status = walk(row.parent_item_uid, row.root_item_uid);
				if (status != error::ok)
					return status;
			}
		std::sort(result.begin(), result.end(), [](const auto &left, const auto &right)
			  { return left.item_uid < right.item_uid; });
		*output = std::move(result);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::allocation_failure;
	}
}
