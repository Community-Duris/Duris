#include "item/item_ownership_runtime.h"

#include "economy/collector_command.h"

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
	size_t operator()(const item_owner_identity &owner) const noexcept
	{
		return static_cast<size_t>(owner.id ^ (owner.context_id << 1) ^
					   (static_cast<uint64_t>(owner.type) << 56));
	}
};
struct owner_equal
{
	bool operator()(const item_owner_identity &left,
			const item_owner_identity &right) const noexcept
	{
		return item_owner_identity_equal(left, right);
	}
};
std::unordered_map<item_owner_identity, uint64_t, owner_hash, owner_equal> owner_revisions;
}

bool item_ownership_runtime_snapshot_owner(const item_owner_identity &owner, size_t limit,
					   std::vector<item_ownership_runtime_entry> *snapshot)
{
	if (!snapshot || !item_owner_identity_valid(owner))
		return false;
	try
	{
		std::vector<item_ownership_runtime_entry> captured;
		for (const auto &[uid, entry] : entries)
		{
			(void)uid;
			if (!item_owner_identity_equal(entry.owner, owner))
				continue;
			if (captured.size() >= limit)
				return false;
			captured.push_back(entry);
		}
		std::sort(captured.begin(), captured.end(), [](const auto &left, const auto &right)
			  { return left.item_uid < right.item_uid; });
		*snapshot = std::move(captured);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
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
			    entry.state == item_custody_state::absent || entry.vnum < 0 ||
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

bool item_ownership_runtime_reconcile_collector(const item_ownership_runtime_entry *batch,
						size_t count)
{
	if ((!batch && count) || count > ITEM_OWNERSHIP_RUNTIME_MAX)
		return false;
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
	using entry_node = decltype(entries)::node_type;
	using owner_node = decltype(owner_revisions)::node_type;
	std::unordered_set<uint64_t> incoming_uids;
	std::unordered_set<uint64_t> incoming_owner_ids;
	std::vector<previous_entry> previous_entries;
	std::vector<previous_owner> previous_owners;
	std::vector<entry_node> removed_entries;
	std::vector<owner_node> removed_owners;
	std::vector<uint64_t> inserted_entries;
	std::vector<item_owner_identity> inserted_owners;
	size_t stale_entry_count = 0, stale_owner_count = 0, new_entry_count = 0,
	       new_owner_count = 0;
	try
	{
		incoming_uids.reserve(count);
		incoming_owner_ids.reserve(count);
		previous_entries.reserve(count);
		previous_owners.reserve(count);
		inserted_entries.reserve(count);
		inserted_owners.reserve(count);
		for (size_t index = 0; index < count; ++index)
		{
			const item_ownership_runtime_entry &entry = batch[index];
			if (!entry.item_uid || entry.root_item_uid != entry.item_uid ||
			    entry.parent_item_uid ||
			    entry.owner.type != item_owner_type::collector || entry.owner.id == 0 ||
			    entry.owner.context_id || !entry.item_revision ||
			    !entry.owner_revision || entry.vnum <= 0 ||
			    entry.state != item_custody_state::active ||
			    !incoming_uids.insert(entry.item_uid).second ||
			    !incoming_owner_ids.insert(entry.owner.id).second)
				return false;
			const auto current = entries.find(entry.item_uid);
			if (current != entries.end() &&
			    (current->second.item_revision > entry.item_revision ||
			     (current->second.item_revision == entry.item_revision &&
			      (current->second.root_item_uid != entry.root_item_uid ||
			       current->second.parent_item_uid != entry.parent_item_uid ||
			       !item_owner_identity_equal(current->second.owner, entry.owner) ||
			       current->second.owner_revision != entry.owner_revision ||
			       current->second.vnum != entry.vnum ||
			       current->second.state != entry.state))))
				return false;
			previous_entries.push_back({ entry.item_uid, current != entries.end(),
						     current != entries.end() ?
							     current->second :
							     item_ownership_runtime_entry{} });
			if (current == entries.end())
				++new_entry_count;
			const auto owner = owner_revisions.find(entry.owner);
			if (owner != owner_revisions.end() && owner->second > entry.owner_revision)
				return false;
			previous_owners.push_back(
				{ entry.owner, owner != owner_revisions.end(),
				  owner != owner_revisions.end() ? owner->second : 0 });
			if (owner == owner_revisions.end())
				++new_owner_count;
		}
		for (const auto &[uid, entry] : entries)
			if (entry.owner.type == item_owner_type::collector &&
			    !incoming_uids.count(uid))
				++stale_entry_count;
		for (const auto &[owner, revision] : owner_revisions)
		{
			(void)revision;
			if (owner.type == item_owner_type::collector &&
			    !incoming_owner_ids.count(owner.id))
				++stale_owner_count;
		}
		if (entries.size() - stale_entry_count >
		    ITEM_OWNERSHIP_RUNTIME_MAX - new_entry_count)
			return false;
		removed_entries.reserve(stale_entry_count);
		removed_owners.reserve(stale_owner_count);
		entries.reserve(entries.size() + new_entry_count);
		owner_revisions.reserve(owner_revisions.size() + new_owner_count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}

	for (auto current = entries.begin(); current != entries.end();)
		if (current->second.owner.type == item_owner_type::collector &&
		    !incoming_uids.count(current->first))
		{
			auto stale = current++;
			removed_entries.push_back(entries.extract(stale));
		}
		else
			++current;
	for (auto current = owner_revisions.begin(); current != owner_revisions.end();)
		if (current->first.type == item_owner_type::collector &&
		    !incoming_owner_ids.count(current->first.id))
		{
			auto stale = current++;
			removed_owners.push_back(owner_revisions.extract(stale));
		}
		else
			++current;

	try
	{
		for (size_t index = 0; index < count; ++index)
		{
			const item_ownership_runtime_entry &entry = batch[index];
			auto current = entries.find(entry.item_uid);
			if (current == entries.end())
			{
				entries.emplace(entry.item_uid, entry);
				inserted_entries.push_back(entry.item_uid);
			}
			else
				current->second = entry;
			auto owner = owner_revisions.find(entry.owner);
			if (owner == owner_revisions.end())
			{
				owner_revisions.emplace(entry.owner, entry.owner_revision);
				inserted_owners.push_back(entry.owner);
			}
			else
				owner->second = entry.owner_revision;
		}
	}
	catch (const std::bad_alloc &)
	{
		for (uint64_t uid : inserted_entries)
			entries.erase(uid);
		for (const item_owner_identity &owner : inserted_owners)
			owner_revisions.erase(owner);
		for (const previous_entry &entry : previous_entries)
			if (entry.existed)
				entries.find(entry.item_uid)->second = entry.value;
		for (const previous_owner &owner : previous_owners)
			if (owner.existed)
				owner_revisions.find(owner.owner)->second = owner.revision;
		for (entry_node &entry : removed_entries)
			entries.insert(std::move(entry));
		for (owner_node &owner : removed_owners)
			owner_revisions.insert(std::move(owner));
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
	    result.root_item_uid != item_transfer_result_root(payload))
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
		std::vector<item_ownership_runtime_entry> created;
		try
		{
			created.reserve(payload.item_count);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const item_transfer_entry &item = payload.items[index];
			uint64_t target_root = 0, target_parent = 0;
			if (item.expected_state != item_custody_state::absent ||
			    item.expected_item_revision != ITEM_TRANSFER_ABSENT_REVISION ||
			    entries.find(item.item_uid) != entries.end() ||
			    !item_transfer_target_topology(payload, item.item_uid, &target_root,
							   &target_parent) ||
			    item.vnum <= 0)
				return false;
			try
			{
				created.push_back({ item.item_uid, target_root, target_parent,
						    payload.to_owner, 1, result.to_owner_revision,
						    item.vnum, item_custody_state::active });
			}
			catch (const std::bad_alloc &)
			{
				return false;
			}
		}
		if (!item_ownership_runtime_hydrate_batch(created.data(), created.size()))
			return false;
		owner_revisions[payload.from_owner] = result.from_owner_revision;
		return true;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		auto found = entries.find(payload.items[index].item_uid);
		uint64_t target_root = 0, target_parent = 0;
		if (found == entries.end() ||
		    found->second.item_revision != payload.items[index].expected_item_revision ||
		    !item_owner_identity_equal(found->second.owner, payload.from_owner) ||
		    found->second.item_revision == std::numeric_limits<uint64_t>::max() ||
		    !item_transfer_target_topology(payload, payload.items[index].item_uid,
						   &target_root, &target_parent))
			return false;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		item_ownership_runtime_entry &entry = entries[payload.items[index].item_uid];
		uint64_t target_root = 0, target_parent = 0;
		if (!item_transfer_target_topology(payload, payload.items[index].item_uid,
						   &target_root, &target_parent))
			return false;
		++entry.item_revision;
		entry.root_item_uid = target_root;
		entry.parent_item_uid = target_parent;
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

namespace
{
const item_transfer_entry *collector_payload_item(const collector_command_payload &payload,
						  uint64_t item_uid)
{
	auto found = std::lower_bound(payload.items.begin(),
				      payload.items.begin() + payload.item_count, item_uid,
				      [](const item_transfer_entry &entry, uint64_t sought)
				      { return entry.item_uid < sought; });
	return found != payload.items.begin() + payload.item_count && found->item_uid == item_uid ?
		       &*found :
		       nullptr;
}

uint64_t collector_root_after_detach(const collector_command_payload &payload,
				     const item_transfer_entry &item)
{
	if (payload.items[0].root_item_uid != payload.selected_item_uid ||
	    item.item_uid == payload.selected_item_uid)
		return item.item_uid == payload.selected_item_uid ? item.item_uid :
								    item.root_item_uid;
	const item_transfer_entry *cursor = &item;
	for (size_t depth = 0; depth <= payload.item_count; ++depth)
	{
		if (cursor->parent_item_uid == payload.selected_item_uid)
			return cursor->item_uid;
		cursor = collector_payload_item(payload, cursor->parent_item_uid);
		if (!cursor)
			return 0;
	}
	return 0;
}

bool collector_runtime_entry_matches(const item_ownership_runtime_entry &left,
				     const item_ownership_runtime_entry &right)
{
	return left.item_uid == right.item_uid && left.root_item_uid == right.root_item_uid &&
	       left.parent_item_uid == right.parent_item_uid &&
	       item_owner_identity_equal(left.owner, right.owner) &&
	       left.item_revision == right.item_revision && left.vnum == right.vnum &&
	       left.state == right.state;
}

bool collector_publish_authority(const collector_command_payload &payload,
				 const collector_command_result &result,
				 std::vector<item_ownership_runtime_entry> desired)
{
	if (result.from_owner_revision != payload.expected_from_owner_revision + 1 ||
	    result.to_owner_revision != payload.expected_to_owner_revision + 1)
		return false;
	uint64_t from_revision = 0, to_revision = 0;
	if (!item_ownership_runtime_owner_revision(payload.from_owner, &from_revision) ||
	    !item_ownership_runtime_owner_revision(payload.to_owner, &to_revision) ||
	    from_revision < payload.expected_from_owner_revision ||
	    to_revision < payload.expected_to_owner_revision)
		return false;
	const uint64_t published_from_revision =
		std::max(from_revision, result.from_owner_revision);
	const uint64_t published_to_revision = std::max(to_revision, result.to_owner_revision);

	std::vector<item_ownership_runtime_entry> changes;
	try
	{
		changes.reserve(desired.size());
		for (item_ownership_runtime_entry &target : desired)
		{
			item_ownership_runtime_entry current = {};
			const item_transfer_entry *expected =
				collector_payload_item(payload, target.item_uid);
			if (!expected ||
			    !item_ownership_runtime_lookup(target.item_uid, &current) ||
			    current.item_revision < expected->expected_item_revision)
				return false;
			if (current.item_revision == expected->expected_item_revision)
			{
				if (current.root_item_uid != expected->root_item_uid ||
				    current.parent_item_uid != expected->parent_item_uid ||
				    !item_owner_identity_equal(current.owner, payload.from_owner) ||
				    current.vnum != expected->vnum ||
				    current.state != expected->expected_state)
					return false;
				target.owner_revision = item_owner_identity_equal(
								target.owner, payload.from_owner) ?
								published_from_revision :
								published_to_revision;
				changes.push_back(target);
				continue;
			}
			if (current.item_revision == target.item_revision)
			{
				if (!collector_runtime_entry_matches(current, target))
					return false;
				continue;
			}
			// A later committed custody operation is authoritative. An old
			// completion may finish publication without rolling it back.
			if (current.item_revision < target.item_revision)
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (!changes.empty() &&
	    !item_ownership_runtime_hydrate_many_atomic(changes.data(), changes.size()))
		return false;
	return item_ownership_runtime_hydrate_owner(payload.from_owner, published_from_revision) &&
	       item_ownership_runtime_hydrate_owner(payload.to_owner, published_to_revision);
}
}

bool item_ownership_runtime_apply_collector(const collector_command_payload &payload,
					    const collector_command_result &result)
{
	if (result.action != payload.action || !result.record_present ||
	    result.entry.listing != payload.listing)
		return false;
	const bool collection = payload.action == collector_action::collect;
	const bool held = payload.action == collector_action::purchase ||
			  payload.action == collector_action::expire ||
			  (payload.action == collector_action::cancel && payload.item_count);
	if (!collection && !held)
		return !payload.item_count && !result.from_owner_revision &&
		       !result.to_owner_revision;
	if (!payload.item_count || payload.item_count > payload.items.size() ||
	    result.entry.uid != payload.selected_item_uid)
		return false;
	const item_transfer_entry *selected =
		collector_payload_item(payload, payload.selected_item_uid);
	if (!selected)
		return false;
	std::vector<item_ownership_runtime_entry> desired;
	try
	{
		desired.reserve(collection ? payload.item_count : 1);
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const item_transfer_entry &item = payload.items[index];
			if (item.expected_item_revision == std::numeric_limits<uint64_t>::max())
				return false;
			if (collection && item.item_uid != payload.selected_item_uid)
			{
				const uint64_t root = collector_root_after_detach(payload, item);
				const uint64_t parent = item.parent_item_uid ==
									payload.selected_item_uid ?
								selected->parent_item_uid :
								item.parent_item_uid;
				if (!root)
					return false;
				desired.push_back({ item.item_uid, root, parent, payload.from_owner,
						    item.expected_item_revision + 1,
						    result.from_owner_revision, item.vnum,
						    item_custody_state::active });
				continue;
			}
			if (!collection && item.item_uid != payload.selected_item_uid)
				return false;
			desired.push_back({ item.item_uid, item.item_uid, 0, payload.to_owner,
					    item.expected_item_revision + 1,
					    result.to_owner_revision, item.vnum,
					    payload.target_state });
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (held && desired.size() != 1)
		return false;
	if (result.entry.item_revision != selected->expected_item_revision + 1)
		return false;
	return collector_publish_authority(payload, result, std::move(desired));
}

static bool item_ownership_runtime_apply_corpse_disposition(
	uint32_t owner_pid, uint32_t save_id, const item_owner_identity &destination,
	corpse_lifecycle_action action, const corpse_lifecycle_result &result,
	uint64_t target_root_item_uid = 0, uint64_t target_parent_item_uid = 0,
	uint64_t expected_target_parent_revision = 0)
{
	const auto corpse_owner_id = [](uint32_t player_pid, uint32_t corpse_save_id)
	{
		return player_pid && corpse_save_id ?
			       (static_cast<uint64_t>(player_pid) << 32) | corpse_save_id :
			       0;
	};
	const bool nested = action == corpse_lifecycle_action::release_nested;
	const uint64_t destination_result_revision = destination.type == item_owner_type::player ?
							     result.player_owner_revision :
							     result.room_owner_revision;
	if (!owner_pid || !save_id || !item_owner_identity_valid(destination) ||
	    result.owner_pid != owner_pid || result.save_id != save_id || result.action != action ||
	    result.corpse_revision || !result.corpse_owner_revision ||
	    !destination_result_revision ||
	    (nested !=
	     (target_root_item_uid && target_parent_item_uid && expected_target_parent_revision)) ||
	    ((!result.item_count && result.max_item_revision) ||
	     (result.item_count && !result.max_item_revision)))
		return false;
	const item_owner_identity corpse = { item_owner_type::corpse,
					     corpse_owner_id(owner_pid, save_id), 0 };
	const auto corpse_revision = owner_revisions.find(corpse);
	const auto destination_revision = owner_revisions.find(destination);
	const bool corpse_existed = corpse_revision != owner_revisions.end();
	const bool destination_existed = destination_revision != owner_revisions.end();
	const uint64_t expected_corpse_revision = result.corpse_owner_revision - 1;
	const uint64_t expected_destination_revision = destination_result_revision - 1;
	if ((corpse_existed ? corpse_revision->second : 0) != expected_corpse_revision ||
	    (destination_existed ? destination_revision->second : 0) !=
		    expected_destination_revision)
		return false;
	if (nested)
	{
		const auto parent = entries.find(target_parent_item_uid);
		if (parent == entries.end() ||
		    parent->second.root_item_uid != target_root_item_uid ||
		    !item_owner_identity_equal(parent->second.owner, destination) ||
		    parent->second.item_revision != expected_target_parent_revision ||
		    parent->second.state != item_custody_state::active)
			return false;
	}
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
		owner_revisions.insert_or_assign(destination, destination_result_revision);
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
		entry.owner_revision = destination_result_revision;
		if (nested)
		{
			entry.root_item_uid = target_root_item_uid;
			if (!entry.parent_item_uid)
				entry.parent_item_uid = target_parent_item_uid;
		}
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

bool item_ownership_runtime_apply_corpse_nested_release(uint32_t owner_pid, uint32_t save_id,
							const item_owner_identity &destination,
							uint64_t target_root_item_uid,
							uint64_t target_parent_item_uid,
							uint64_t expected_target_parent_revision,
							const corpse_lifecycle_result &result)
{
	if (destination.type != item_owner_type::player &&
	    destination.type != item_owner_type::room)
		return false;
	return item_ownership_runtime_apply_corpse_disposition(
		owner_pid, save_id, destination, corpse_lifecycle_action::release_nested, result,
		target_root_item_uid, target_parent_item_uid, expected_target_parent_revision);
}

bool item_ownership_runtime_apply_corpse_resurrection(uint32_t owner_pid, uint32_t save_id,
						      uint32_t player_pid, int32_t old_room_vnum,
						      const corpse_lifecycle_result &result)
{
	const uint64_t corpse_owner_id = (static_cast<uint64_t>(owner_pid) << 32) |
					 static_cast<uint64_t>(save_id);
	if (!owner_pid || !save_id || !player_pid || old_room_vnum <= 0 ||
	    result.owner_pid != owner_pid || result.save_id != save_id ||
	    result.action != corpse_lifecycle_action::resurrect || result.corpse_revision ||
	    !result.corpse_owner_revision || !result.room_owner_revision ||
	    !result.player_owner_revision || !result.wallet_revision ||
	    ((!result.item_count && result.max_item_revision) ||
	     (result.item_count && !result.max_item_revision)))
		return false;
	const item_owner_identity corpse = { item_owner_type::corpse, corpse_owner_id, 0 };
	const item_owner_identity player = { item_owner_type::player, player_pid, 0 };
	const item_owner_identity room = { item_owner_type::room,
					   static_cast<uint64_t>(old_room_vnum), 0 };
	const auto corpse_revision = owner_revisions.find(corpse);
	const auto player_revision = owner_revisions.find(player);
	const auto room_revision = owner_revisions.find(room);
	if ((corpse_revision == owner_revisions.end() ? 0 : corpse_revision->second) !=
		    result.corpse_owner_revision - 1 ||
	    (player_revision == owner_revisions.end() ? 0 : player_revision->second) !=
		    result.player_owner_revision - 1 ||
	    (room_revision == owner_revisions.end() ? 0 : room_revision->second) !=
		    result.room_owner_revision - 1)
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
		owner_revisions.reserve(owner_revisions.size() + 3);
		owner_revisions.insert_or_assign(corpse, result.corpse_owner_revision);
		owner_revisions.insert_or_assign(player, result.player_owner_revision);
		owner_revisions.insert_or_assign(room, result.room_owner_revision);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &[uid, entry] : entries)
	{
		(void)uid;
		if (!item_owner_identity_equal(entry.owner, corpse))
			continue;
		++entry.item_revision;
		entry.owner = player;
		entry.owner_revision = result.player_owner_revision;
	}
	return true;
}

bool item_ownership_runtime_apply_corpse_raise(uint32_t owner_pid, uint32_t save_id,
					       uint32_t player_pid,
					       const corpse_lifecycle_result &result)
{
	const uint64_t corpse_owner_id = (static_cast<uint64_t>(owner_pid) << 32) |
					 static_cast<uint64_t>(save_id);
	if (!owner_pid || !save_id || !player_pid || result.owner_pid != owner_pid ||
	    result.save_id != save_id || result.action != corpse_lifecycle_action::raise_follower ||
	    result.corpse_revision || !result.corpse_owner_revision || result.room_owner_revision ||
	    !result.player_owner_revision || !result.wallet_revision ||
	    ((!result.item_count && result.max_item_revision) ||
	     (result.item_count && !result.max_item_revision)))
		return false;
	const item_owner_identity corpse = { item_owner_type::corpse, corpse_owner_id, 0 };
	const item_owner_identity player = { item_owner_type::player, player_pid, 0 };
	const auto corpse_revision = owner_revisions.find(corpse);
	const auto player_revision = owner_revisions.find(player);
	if ((corpse_revision == owner_revisions.end() ? 0 : corpse_revision->second) !=
		    result.corpse_owner_revision - 1 ||
	    (player_revision == owner_revisions.end() ? 0 : player_revision->second) !=
		    result.player_owner_revision - 1)
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
		owner_revisions.reserve(owner_revisions.size() + 2);
		owner_revisions.insert_or_assign(corpse, result.corpse_owner_revision);
		owner_revisions.insert_or_assign(player, result.player_owner_revision);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &[uid, entry] : entries)
	{
		(void)uid;
		if (!item_owner_identity_equal(entry.owner, corpse))
			continue;
		++entry.item_revision;
		entry.owner = player;
		entry.owner_revision = result.player_owner_revision;
	}
	return true;
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

void item_ownership_runtime_forget_player_domain(uint32_t player_pid)
{
	if (!player_pid)
		return;
	const auto belongs_to_player = [player_pid](const item_owner_identity &owner)
	{
		return (owner.type == item_owner_type::player && owner.id == player_pid) ||
		       (owner.type == item_owner_type::corpse &&
			static_cast<uint32_t>(owner.id >> 32) == player_pid);
	};
	for (auto entry = entries.begin(); entry != entries.end();)
		if (belongs_to_player(entry->second.owner))
			entry = entries.erase(entry);
		else
			++entry;
	for (auto owner = owner_revisions.begin(); owner != owner_revisions.end();)
		if (belongs_to_player(owner->first))
			owner = owner_revisions.erase(owner);
		else
			++owner;
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
