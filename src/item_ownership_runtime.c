#include "item_ownership_runtime.h"

#include <algorithm>
#include <limits>
#include <new>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
constexpr size_t ITEM_OWNERSHIP_RUNTIME_MAX = 262144;
std::unordered_map<uint64_t, item_ownership_runtime_entry> entries;
struct owner_hash
{
	size_t operator()(const item_owner_identity &owner) const
	{
		return static_cast<size_t>(owner.id ^ (owner.context_id << 1) ^
					   (static_cast<uint64_t>(owner.type) << 56));
	}
};
struct owner_equal
{
	bool operator()(const item_owner_identity &left, const item_owner_identity &right) const
	{
		return item_owner_identity_equal(left, right);
	}
};
std::unordered_map<item_owner_identity, uint64_t, owner_hash, owner_equal> owner_revisions;
}

bool item_ownership_runtime_hydrate(const item_ownership_runtime_entry &entry)
{
	if (!entry.item_uid || !entry.root_item_uid || !item_owner_identity_valid(entry.owner) ||
	    entry.state == item_custody_state::absent || entry.vnum < 0)
		return false;
	auto found = entries.find(entry.item_uid);
	if (!item_ownership_runtime_hydrate_owner(entry.owner, entry.owner_revision))
		return false;
	if (found != entries.end())
	{
		if (found->second.item_revision > entry.item_revision)
			return false;
		found->second = entry;
		return true;
	}
	if (entries.size() >= ITEM_OWNERSHIP_RUNTIME_MAX)
		return false;
	try
	{
		entries.emplace(entry.item_uid, entry);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool item_ownership_runtime_hydrate_batch(const item_ownership_runtime_entry *batch, size_t count)
{
	if ((!batch && count) || count > ITEM_OWNERSHIP_RUNTIME_MAX)
		return false;
	if (!count)
		return true;
	const item_owner_identity owner = batch[0].owner;
	const uint64_t owner_revision = batch[0].owner_revision;
	struct previous_entry
	{
		uint64_t item_uid;
		bool existed;
		item_ownership_runtime_entry value;
	};
	std::vector<previous_entry> previous;
	std::unordered_set<uint64_t> item_uids;
	try
	{
		previous.reserve(count);
		item_uids.reserve(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (size_t index = 0; index < count; ++index)
	{
		const item_ownership_runtime_entry &entry = batch[index];
		if (!entry.item_uid || !entry.root_item_uid ||
		    !item_owner_identity_equal(entry.owner, owner) ||
		    entry.owner_revision != owner_revision ||
		    entry.state != item_custody_state::active || entry.vnum < 0)
			return false;
		try
		{
			if (!item_uids.insert(entry.item_uid).second)
				return false;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		const auto found = entries.find(entry.item_uid);
		if (found != entries.end() &&
		    (found->second.item_revision > entry.item_revision ||
		     (found->second.item_revision == entry.item_revision &&
		      (found->second.root_item_uid != entry.root_item_uid ||
		       found->second.parent_item_uid != entry.parent_item_uid ||
		       !item_owner_identity_equal(found->second.owner, entry.owner) ||
		       found->second.vnum != entry.vnum || found->second.state != entry.state))))
			return false;
		previous.push_back({ entry.item_uid, found != entries.end(),
				     found != entries.end() ? found->second :
							      item_ownership_runtime_entry{} });
	}
	const auto previous_owner = owner_revisions.find(owner);
	if (previous_owner != owner_revisions.end() && previous_owner->second > owner_revision)
		return false;
	const bool owner_existed = previous_owner != owner_revisions.end();
	const uint64_t old_owner_revision = owner_existed ? previous_owner->second : 0;
	size_t new_count = 0;
	for (const previous_entry &entry : previous)
		if (!entry.existed)
			++new_count;
	if (entries.size() > ITEM_OWNERSHIP_RUNTIME_MAX - new_count)
		return false;
	try
	{
		entries.reserve(entries.size() + new_count);
		owner_revisions.reserve(owner_revisions.size() + 1);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (size_t index = 0; index < count; ++index)
		if (!item_ownership_runtime_hydrate(batch[index]))
		{
			for (const previous_entry &entry : previous)
				if (entry.existed)
					entries[entry.item_uid] = entry.value;
				else
					entries.erase(entry.item_uid);
			if (owner_existed)
				owner_revisions[owner] = old_owner_revision;
			else
				owner_revisions.erase(owner);
			return false;
		}
	return true;
}

bool item_ownership_runtime_hydrate_many_atomic(const item_ownership_runtime_entry *batch,
						size_t count)
{
	if ((!batch && count) || count > ITEM_OWNERSHIP_RUNTIME_MAX)
		return false;
	if (!count)
		return true;
	struct previous_entry
	{
		uint64_t item_uid;
		bool existed;
		item_ownership_runtime_entry value;
	};
	struct previous_owner
	{
		item_owner_identity owner;
		bool existed;
		uint64_t revision;
	};
	std::vector<previous_entry> previous_entries;
	std::vector<previous_owner> previous_owners;
	std::unordered_set<uint64_t> item_uids;
	std::unordered_map<item_owner_identity, uint64_t, owner_hash, owner_equal> incoming_owners;
	size_t new_entries = 0;
	try
	{
		previous_entries.reserve(count);
		item_uids.reserve(count);
		incoming_owners.reserve(count);
		for (size_t index = 0; index < count; ++index)
		{
			const item_ownership_runtime_entry &entry = batch[index];
			if (!entry.item_uid || !entry.root_item_uid ||
			    !item_owner_identity_valid(entry.owner) ||
			    entry.state != item_custody_state::active || entry.vnum < 0 ||
			    !item_uids.insert(entry.item_uid).second)
				return false;
			const auto incoming_owner = incoming_owners.find(entry.owner);
			if (incoming_owner != incoming_owners.end())
			{
				if (incoming_owner->second != entry.owner_revision)
					return false;
			}
			else
				incoming_owners.emplace(entry.owner, entry.owner_revision);
			const auto found = entries.find(entry.item_uid);
			if (found != entries.end() &&
			    (found->second.item_revision > entry.item_revision ||
			     (found->second.item_revision == entry.item_revision &&
			      (found->second.root_item_uid != entry.root_item_uid ||
			       found->second.parent_item_uid != entry.parent_item_uid ||
			       !item_owner_identity_equal(found->second.owner, entry.owner) ||
			       found->second.vnum != entry.vnum ||
			       found->second.state != entry.state))))
				return false;
			previous_entries.push_back({ entry.item_uid, found != entries.end(),
						     found != entries.end() ?
							     found->second :
							     item_ownership_runtime_entry{} });
			if (found == entries.end())
				++new_entries;
		}
		previous_owners.reserve(incoming_owners.size());
		for (const auto &[owner, revision] : incoming_owners)
		{
			const auto found = owner_revisions.find(owner);
			if (found != owner_revisions.end() && found->second > revision)
				return false;
			previous_owners.push_back(
				{ owner, found != owner_revisions.end(),
				  found != owner_revisions.end() ? found->second : 0 });
		}
		if (entries.size() > ITEM_OWNERSHIP_RUNTIME_MAX - new_entries)
			return false;
		entries.reserve(entries.size() + new_entries);
		owner_revisions.reserve(owner_revisions.size() + incoming_owners.size());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	try
	{
		for (size_t index = 0; index < count; ++index)
			entries.insert_or_assign(batch[index].item_uid, batch[index]);
		for (const auto &[owner, revision] : incoming_owners)
			owner_revisions.insert_or_assign(owner, revision);
	}
	catch (const std::bad_alloc &)
	{
		for (const previous_entry &entry : previous_entries)
			if (entry.existed)
				entries[entry.item_uid] = entry.value;
			else
				entries.erase(entry.item_uid);
		for (const previous_owner &owner : previous_owners)
			if (owner.existed)
				owner_revisions[owner.owner] = owner.revision;
			else
				owner_revisions.erase(owner.owner);
		return false;
	}
	return true;
}

bool item_ownership_runtime_hydrate_owner(const item_owner_identity &owner, uint64_t revision)
{
	if (!item_owner_identity_valid(owner))
		return false;
	auto found = owner_revisions.find(owner);
	if (found != owner_revisions.end())
	{
		if (found->second > revision)
			return false;
		found->second = revision;
		return true;
	}
	try
	{
		owner_revisions.emplace(owner, revision);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

bool item_ownership_runtime_lookup(uint64_t item_uid, item_ownership_runtime_entry *entry)
{
	if (!entry)
		return false;
	const auto found = entries.find(item_uid);
	if (found == entries.end())
		return false;
	*entry = found->second;
	return true;
}

bool item_ownership_runtime_owner_revision(const item_owner_identity &owner, uint64_t *revision)
{
	if (!revision)
		return false;
	const auto found = owner_revisions.find(owner);
	if (found == owner_revisions.end())
	{
		if (!item_owner_identity_valid(owner) ||
		    !item_ownership_runtime_hydrate_owner(owner, 0))
			return false;
		*revision = 0;
		return true;
	}
	*revision = found->second;
	return true;
}

bool item_ownership_runtime_apply(const item_transfer_payload &payload,
				  const item_transfer_result &result)
{
	if (!payload.item_count || payload.item_count > ITEM_TRANSFER_MAX_ITEMS ||
	    result.item_count != payload.item_count ||
	    result.root_item_uid != payload.selected_item_uid)
		return false;
	const bool creation = payload.from_owner.type == item_owner_type::system;
	if (payload.target_parent_item_uid)
	{
		const auto parent = entries.find(payload.target_parent_item_uid);
		if (parent == entries.end() ||
		    parent->second.root_item_uid != payload.target_root_item_uid ||
		    !item_owner_identity_equal(parent->second.owner, payload.to_owner) ||
		    parent->second.item_revision != payload.expected_target_parent_revision ||
		    parent->second.state != item_custody_state::active)
			return false;
	}
	if (creation)
	{
		for (size_t index = 0; index < payload.item_count; ++index)
			if (entries.find(payload.items[index].item_uid) != entries.end() ||
			    !item_ownership_runtime_hydrate(
				    { payload.items[index].item_uid, payload.target_root_item_uid,
				      payload.items[index].item_uid == payload.selected_item_uid ?
					      payload.target_parent_item_uid :
					      payload.items[index].parent_item_uid,
				      payload.to_owner, 1, result.to_owner_revision,
				      payload.items[index].vnum, item_custody_state::active }))
				return false;
		owner_revisions[payload.from_owner] = result.from_owner_revision;
		owner_revisions[payload.to_owner] = result.to_owner_revision;
		return true;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		auto found = entries.find(payload.items[index].item_uid);
		if (found == entries.end() ||
		    found->second.item_revision != payload.items[index].expected_item_revision ||
		    !item_owner_identity_equal(found->second.owner, payload.from_owner))
			return false;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		item_ownership_runtime_entry &entry = entries[payload.items[index].item_uid];
		if (entry.item_revision == std::numeric_limits<uint64_t>::max())
			return false;
		++entry.item_revision;
		entry.root_item_uid = payload.target_root_item_uid;
		entry.parent_item_uid = payload.items[index].item_uid == payload.selected_item_uid ?
						payload.target_parent_item_uid :
						payload.items[index].parent_item_uid;
		entry.owner = payload.to_owner;
		entry.owner_revision = result.to_owner_revision;
		entry.state = payload.to_owner.type == item_owner_type::destruction ?
				      item_custody_state::destroyed :
				      item_custody_state::active;
	}
	owner_revisions[payload.from_owner] = result.from_owner_revision;
	owner_revisions[payload.to_owner] = result.to_owner_revision;
	return true;
}

static bool item_ownership_runtime_apply_corpse_disposition(uint32_t owner_pid, uint32_t save_id,
							    const item_owner_identity &destination,
							    corpse_lifecycle_action action,
							    const corpse_lifecycle_result &result)
{
	if (!owner_pid || !save_id || !item_owner_identity_valid(destination) ||
	    result.owner_pid != owner_pid || result.save_id != save_id || result.action != action ||
	    result.corpse_revision || !result.corpse_owner_revision ||
	    !result.room_owner_revision ||
	    ((!result.item_count && result.max_item_revision) ||
	     (result.item_count && !result.max_item_revision)))
		return false;
	const item_owner_identity corpse = { item_owner_type::corpse,
					     item_corpse_owner_id(owner_pid, save_id), 0 };
	const auto corpse_revision = owner_revisions.find(corpse);
	const auto destination_revision = owner_revisions.find(destination);
	const bool corpse_existed = corpse_revision != owner_revisions.end();
	const bool destination_existed = destination_revision != owner_revisions.end();
	const uint64_t expected_corpse_revision = result.corpse_owner_revision - 1;
	const uint64_t expected_destination_revision = result.room_owner_revision - 1;
	if ((corpse_existed ? corpse_revision->second : 0) != expected_corpse_revision ||
	    (destination_existed ? destination_revision->second : 0) !=
		    expected_destination_revision)
		return false;
	size_t item_count = 0;
	uint64_t max_item_revision = 0;
	for (const auto &[uid, entry] : entries)
	{
		(void)uid;
		if (!item_owner_identity_equal(entry.owner, corpse))
			continue;
		if (entry.state != item_custody_state::active ||
		    entry.item_revision == std::numeric_limits<uint64_t>::max())
			return false;
		++item_count;
		max_item_revision = std::max(max_item_revision, entry.item_revision + 1);
	}
	if (item_count != result.item_count || max_item_revision != result.max_item_revision)
		return false;
	try
	{
		owner_revisions.reserve(owner_revisions.size() + (corpse_existed ? 0 : 1) +
					(destination_existed ? 0 : 1));
		owner_revisions.insert_or_assign(corpse, result.corpse_owner_revision);
		owner_revisions.insert_or_assign(destination, result.room_owner_revision);
	}
	catch (const std::bad_alloc &)
	{
		if (!corpse_existed)
			owner_revisions.erase(corpse);
		else
			owner_revisions[corpse] = expected_corpse_revision;
		if (!destination_existed)
			owner_revisions.erase(destination);
		else
			owner_revisions[destination] = expected_destination_revision;
		return false;
	}
	for (auto &[uid, entry] : entries)
	{
		(void)uid;
		if (!item_owner_identity_equal(entry.owner, corpse))
			continue;
		++entry.item_revision;
		entry.owner = destination;
		entry.owner_revision = result.room_owner_revision;
		if (action == corpse_lifecycle_action::destroy)
			entry.state = item_custody_state::destroyed;
	}
	return true;
}

bool item_ownership_runtime_apply_corpse_release(uint32_t owner_pid, uint32_t save_id,
						 int32_t room_vnum,
						 const corpse_lifecycle_result &result)
{
	if (room_vnum <= 0)
		return false;
	return item_ownership_runtime_apply_corpse_disposition(
		owner_pid, save_id, { item_owner_type::room, static_cast<uint64_t>(room_vnum), 0 },
		corpse_lifecycle_action::release, result);
}

bool item_ownership_runtime_apply_corpse_destruction(uint32_t owner_pid, uint32_t save_id,
						     const corpse_lifecycle_result &result)
{
	return item_ownership_runtime_apply_corpse_disposition(
		owner_pid, save_id, { item_owner_type::destruction, 0, 0 },
		corpse_lifecycle_action::destroy, result);
}

void item_ownership_runtime_forget(uint64_t item_uid)
{
	if (item_uid)
		entries.erase(item_uid);
}

void item_ownership_runtime_forget_owner(const item_owner_identity &owner)
{
	if (item_owner_identity_valid(owner))
		owner_revisions.erase(owner);
}

void item_ownership_runtime_reset(void)
{
	entries.clear();
	owner_revisions.clear();
}

size_t item_ownership_runtime_size(void)
{
	return entries.size();
}
