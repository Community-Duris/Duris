#include "item/item_ownership_runtime.h"
#include "item/craft_pouch_mutation.h"
#include "core/prototypes.h"

#include "economy/collector_command.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <cerrno>
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

unsigned int item_ownership_runtime_snapshot_all_active(
	size_t limit, std::vector<item_ownership_runtime_entry> *output) noexcept
{
	if (!output || !limit || limit > ITEM_OWNERSHIP_RUNTIME_MAX || !nevent_is_game_thread())
		return EINVAL;
	try
	{
		size_t count = 0;
		for (const auto &[uid, entry] : entries)
		{
			(void)uid;
			if (entry.state == item_custody_state::active && ++count > limit)
				return E2BIG;
		}
		std::vector<item_ownership_runtime_entry> candidate;
		candidate.reserve(count);
		for (const auto &[uid, entry] : entries)
		{
			(void)uid;
			if (entry.state == item_custody_state::active)
				candidate.push_back(entry);
		}
		std::sort(candidate.begin(), candidate.end(),
			  [](const auto &a, const auto &b) { return a.item_uid < b.item_uid; });
		output->swap(candidate);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EINVAL;
	}
}

static bool snapshot_root(uint64_t root_item_uid, size_t limit, bool active_only,
			  std::vector<item_ownership_runtime_entry> *snapshot)
{
	if (!snapshot)
		return false;
	snapshot->clear();
	if (!root_item_uid || !limit)
		return false;
	try
	{
		// Count before allocating. The serialized game-thread registry cannot
		// change between passes, and unrelated roots never consume the budget.
		size_t count = 0;
		for (const auto &[uid, entry] : entries)
		{
			(void)uid;
			if (entry.root_item_uid != root_item_uid ||
			    (active_only && entry.state != item_custody_state::active))
				continue;
			if (count >= limit)
				return false;
			++count;
		}
		std::vector<item_ownership_runtime_entry> captured;
		captured.reserve(count);
		for (const auto &[uid, entry] : entries)
		{
			(void)uid;
			if (entry.root_item_uid == root_item_uid &&
			    (!active_only || entry.state == item_custody_state::active))
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

bool item_ownership_runtime_snapshot_root(uint64_t root_item_uid, size_t limit,
					  std::vector<item_ownership_runtime_entry> *snapshot)
{
	return snapshot_root(root_item_uid, limit, false, snapshot);
}

bool item_ownership_runtime_snapshot_active_root(
	uint64_t root_item_uid, size_t limit, std::vector<item_ownership_runtime_entry> *snapshot)
{
	return snapshot_root(root_item_uid, limit, true, snapshot);
}

bool item_ownership_runtime_published_native_observer::snapshot_links(
	std::span<const uint64_t> selected_uids, size_t limit,
	std::vector<item_ownership_runtime_entry> *output) noexcept
{
	if (!output || !limit || limit > ITEM_OWNERSHIP_RUNTIME_MAX ||
	    selected_uids.size() > PLAYER_SNAPSHOT_MAX_ROWS)
		return false;
	uint64_t previous = 0;
	for (const auto uid : selected_uids)
	{
		if (!uid || uid == UINT64_MAX || uid <= previous)
			return false;
		previous = uid;
	}
	try
	{
		const auto selected = [&](uint64_t uid) noexcept
		{ return std::binary_search(selected_uids.begin(), selected_uids.end(), uid); };
		const auto matches = [&](const item_ownership_runtime_entry &entry) noexcept
		{
			return entry.state == item_custody_state::active &&
			       (selected(entry.item_uid) || selected(entry.root_item_uid) ||
				(entry.parent_item_uid && selected(entry.parent_item_uid)));
		};
		size_t count = 0;
		for (const auto &[uid, entry] : entries)
		{
			(void)uid;
			if (matches(entry))
			{
				if (count >= limit)
					return false;
				++count;
			}
		}
		std::vector<item_ownership_runtime_entry> captured;
		captured.reserve(count);
		for (const auto &[uid, entry] : entries)
		{
			(void)uid;
			if (matches(entry))
				captured.push_back(entry);
		}
		std::sort(captured.begin(), captured.end(),
			  [](const auto &a, const auto &b) { return a.item_uid < b.item_uid; });
		*output = std::move(captured);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool item_ownership_runtime_published_native_observer::snapshot_links_bounded(
    std::span<const uint64_t> selected_uids, size_t limit,
    std::vector<item_ownership_runtime_entry> *output,
    bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
    size_t outer_live_scratch, size_t *retained_output_payload_bytes) noexcept
{
	if (!output || !reserve_scratch_peak || !limit || limit > ITEM_OWNERSHIP_RUNTIME_MAX ||
	    selected_uids.size() > PLAYER_SNAPSHOT_MAX_ROWS)
		return false;
	uint64_t previous = 0;
	for (const auto uid : selected_uids)
	{
		if (!uid || uid == UINT64_MAX || uid <= previous)
			return false;
		previous = uid;
	}
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
    !_GLIBCXX_USE_CXX11_ABI
	(void)context;
	(void)outer_live_scratch;
	(void)retained_output_payload_bytes;
	errno = ENOTSUP;
	return false;
#else
	// The original scalar-only registry traversal counts before its first
	// allocation. The serialized source must stay unchanged through both passes.
	const auto selected = [&](uint64_t uid) noexcept
	{ return std::binary_search(selected_uids.begin(), selected_uids.end(), uid); };
	size_t count = 0;
	for (const auto &[uid, entry] : entries)
	{
		(void)uid;
		if (entry.state == item_custody_state::active &&
		    (selected(entry.item_uid) || selected(entry.root_item_uid) ||
		     (entry.parent_item_uid && selected(entry.parent_item_uid))))
		{
			if (count >= limit)
				return false;
			++count;
		}
	}
	constexpr size_t fixed = sizeof(std::vector<item_ownership_runtime_entry>);
	if (count > SIZE_MAX / sizeof(item_ownership_runtime_entry))
	{
		errno = ENOBUFS;
		return false;
	}
	const size_t retained = count * sizeof(item_ownership_runtime_entry);
	if (fixed > SIZE_MAX - outer_live_scratch ||
	    retained > SIZE_MAX - outer_live_scratch - fixed ||
	    !reserve_scratch_peak(outer_live_scratch + fixed + retained, context))
	{
		errno = ENOBUFS;
		return false;
	}
	// GCC13 fresh reserve requests exactly count rows; push_back copies directly
	// into them with no growth or nested allocations. Original sorting/UID-link
	// semantics and its strong transfer/refusal behavior remain authoritative.
	if (!snapshot_links(selected_uids, limit, output))
		return false;
	if (retained_output_payload_bytes)
		*retained_output_payload_bytes = retained;
	return true;
#endif
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

bool item_ownership_runtime_peek_owner_revision(const item_owner_identity &owner,
						uint64_t *revision) noexcept
{
	if (!revision || !item_owner_identity_valid(owner))
		return false;
	const auto found = owner_revisions.find(owner);
	if (found == owner_revisions.end())
		return false;
	*revision = found->second;
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

bool item_ownership_runtime_apply_craft(const item_transfer_payload &payload,
					const item_transfer_result &result)
{
	if (payload.reason != item_transfer_reason::craft || !payload.item_count ||
	    result.item_count != payload.item_count ||
	    result.root_item_uid != payload.selected_item_uid ||
	    result.from_owner_revision != result.to_owner_revision ||
	    !item_owner_identity_equal(payload.from_owner, payload.to_owner))
		return false;
	std::vector<player_item_snapshot> outputs;
	craft_pouch_mutation pouch;
	if (!craft_pouch_mutation_from_payload(payload, &pouch))
		return false;
	if (payload.item_blob_size &&
	    player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &outputs) != player_snapshot_codec_result::ok)
		return false;
	if (!outputs.empty() && outputs[0].object_uid != payload.selected_item_uid)
		return false;
	std::vector<item_ownership_runtime_entry> changes;
	try
	{
		changes.reserve(payload.item_count + outputs.size());
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const item_transfer_entry &expected = payload.items[index];
			const bool retained = expected.item_uid == pouch.before.object_uid;
			const item_owner_identity after_owner =
				retained ?
					payload.from_owner :
					item_owner_identity{ item_owner_type::destruction, 0, 0 };
			const auto after_state = retained ? item_custody_state::active :
							    item_custody_state::destroyed;
			auto found = entries.find(expected.item_uid);
			uint64_t target_root = 0, target_parent = 0;
			if (!item_transfer_target_topology(payload, expected.item_uid, &target_root,
							   &target_parent) ||
			    expected.expected_item_revision == UINT64_MAX)
				return false;
			if (found != entries.end() &&
			    (found->second.vnum != expected.vnum ||
			     found->second.root_item_uid != expected.root_item_uid ||
			     found->second.parent_item_uid != expected.parent_item_uid ||
			     !((found->second.item_revision == expected.expected_item_revision &&
				item_owner_identity_equal(found->second.owner, payload.from_owner) &&
				found->second.state == item_custody_state::active) ||
			       (found->second.item_revision == expected.expected_item_revision + 1 &&
				item_owner_identity_equal(found->second.owner, after_owner) &&
				found->second.state == after_state))))
				return false;
			changes.push_back({ expected.item_uid, target_root, target_parent,
					    after_owner, expected.expected_item_revision + 1,
					    result.from_owner_revision, expected.vnum,
					    after_state });
		}
		std::unordered_set<uint64_t> output_uids;
		output_uids.reserve(outputs.size());
		for (size_t index = 0; index < outputs.size(); ++index)
		{
			const player_item_snapshot &output = outputs[index];
			if (!output.object_uid || output.vnum <= 0 ||
			    !output_uids.insert(output.object_uid).second)
				return false;
			uint64_t root = output.object_uid;
			int32_t parent = output.parent_index;
			for (size_t depth = 0;
			     parent != PLAYER_SNAPSHOT_NO_PARENT && depth < outputs.size(); ++depth)
			{
				if (parent < 0 || static_cast<size_t>(parent) >= outputs.size())
					return false;
				root = outputs[static_cast<size_t>(parent)].object_uid;
				parent = outputs[static_cast<size_t>(parent)].parent_index;
			}
			if (parent != PLAYER_SNAPSHOT_NO_PARENT)
				return false;
			const uint64_t parent_uid =
				output.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
					0 :
					outputs[static_cast<size_t>(output.parent_index)].object_uid;
			auto existing = entries.find(output.object_uid);
			if (existing != entries.end() &&
			    (existing->second.item_revision != 1 ||
			     existing->second.root_item_uid != root ||
			     existing->second.parent_item_uid != parent_uid ||
			     existing->second.vnum != output.vnum ||
			     existing->second.state != item_custody_state::active ||
			     !item_owner_identity_equal(existing->second.owner, payload.to_owner)))
				return false;
			changes.push_back({ output.object_uid, root, parent_uid, payload.to_owner,
					    1, result.to_owner_revision, output.vnum,
					    item_custody_state::active });
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (!item_ownership_runtime_hydrate_many_atomic(changes.data(), changes.size()))
		return false;
	return item_ownership_runtime_hydrate_owner(payload.from_owner,
						    result.from_owner_revision) &&
	       item_ownership_runtime_hydrate_owner({ item_owner_type::destruction, 0, 0 },
						    result.from_owner_revision);
}

bool item_ownership_runtime_native_quest_publication_owner::apply(
	const item_transfer_payload &payload, const item_transfer_result &result,
	publication_result phase, std::span<const item_ownership_runtime_entry> current_custody,
	uint64_t current_from_revision, uint64_t current_to_revision,
	uint64_t current_player_revision) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	try
	{
		if (phase != publication_result::applied && phase != publication_result::rejected)
			return false;
		const bool applied = phase == publication_result::applied;
		const bool consumption = payload.native_mobile.action ==
					 item_native_mobile_action::consumption;
		if (!item_transfer_native_mobile_recovery_shape_valid(payload) ||
		    current_custody.size() >
			    2 * PLAYER_SNAPSHOT_MAX_OBJECTS + ITEM_TRANSFER_MAX_ITEMS ||
		    (applied && (result.item_count != payload.item_count ||
				 result.root_item_uid != item_transfer_result_root(payload) ||
				 payload.expected_from_revision == UINT64_MAX ||
				 payload.expected_to_revision == UINT64_MAX ||
				 result.from_owner_revision != payload.expected_from_revision + 1 ||
				 result.to_owner_revision != payload.expected_to_revision + 1 ||
				 current_from_revision != result.from_owner_revision ||
				 (consumption ? current_to_revision < result.to_owner_revision :
						current_to_revision != result.to_owner_revision) ||
				 result.corpse_revision || result.collector_catalog_changed)))
			return false;
		const item_owner_identity player{ item_owner_type::player,
						  payload.native_recovery.player_pid, 0 };
		auto from = owner_revisions.find(payload.from_owner);
		auto to = owner_revisions.find(payload.to_owner);
		auto giver = owner_revisions.find(player);
		if (from == owner_revisions.end() || to == owner_revisions.end() ||
		    giver == owner_revisions.end() ||
		    from->second !=
			    (applied ? payload.expected_from_revision : current_from_revision) ||
		    (consumption ? (to->second > current_to_revision ||
				    (applied && to->second < payload.expected_to_revision)) :
				   to->second != (applied ? payload.expected_to_revision :
							    current_to_revision)) ||
		    (consumption ? giver->second > current_player_revision :
				   current_player_revision != current_from_revision))
			return false;
		struct projected
		{
			item_ownership_runtime_entry *entry;
			item_ownership_runtime_entry after;
		};
		std::vector<projected> changes;
		changes.reserve(current_custody.size());
		std::unordered_set<uint64_t> seen;
		seen.reserve(current_custody.size());
		size_t selected_count = 0;
		uint64_t maximum = 0;
		for (const auto &after : current_custody)
		{
			if (!after.item_uid || !seen.insert(after.item_uid).second)
				return false;
			auto original = std::lower_bound(payload.items.begin(),
							 payload.items.begin() + payload.item_count,
							 after.item_uid,
							 [](const auto &item, uint64_t uid)
							 { return item.item_uid < uid; });
			const bool selected = original !=
						      payload.items.begin() + payload.item_count &&
					      original->item_uid == after.item_uid;
			auto before = entries.find(after.item_uid);
			auto current_owner = owner_revisions.find(after.owner);
			const bool known_owner =
				item_owner_identity_equal(after.owner, player) ||
				item_owner_identity_equal(after.owner, payload.from_owner) ||
				item_owner_identity_equal(after.owner, payload.to_owner);
			const uint64_t owner_revision =
				item_owner_identity_equal(after.owner, player) ?
					current_player_revision :
				item_owner_identity_equal(after.owner, payload.from_owner) ?
					current_from_revision :
					current_to_revision;
			if (!known_owner || after.owner_revision != owner_revision ||
			    before == entries.end() || current_owner == owner_revisions.end())
				return false;
			if (selected && applied)
			{
				uint64_t root = 0, parent = 0;
				if (!original->expected_item_revision ||
				    original->expected_item_revision == UINT64_MAX ||
				    !item_transfer_target_topology(payload, original->item_uid,
								   &root, &parent) ||
				    before->second.item_revision !=
					    original->expected_item_revision ||
				    before->second.vnum != original->vnum ||
				    before->second.root_item_uid != original->root_item_uid ||
				    before->second.parent_item_uid != original->parent_item_uid ||
				    before->second.state != item_custody_state::active ||
				    !item_owner_identity_equal(before->second.owner,
							       payload.from_owner) ||
				    before->second.owner_revision > from->second ||
				    after.root_item_uid != root ||
				    after.parent_item_uid != parent ||
				    after.item_revision != original->expected_item_revision + 1 ||
				    after.vnum != original->vnum ||
				    after.state != (consumption ? item_custody_state::destroyed :
								  item_custody_state::active) ||
				    !item_owner_identity_equal(after.owner, payload.to_owner))
					return false;
				++selected_count;
				maximum = std::max(maximum, after.item_revision);
			}
			else if (before->second.root_item_uid != after.root_item_uid ||
				 before->second.parent_item_uid != after.parent_item_uid ||
				 before->second.item_revision != after.item_revision ||
				 before->second.vnum != after.vnum ||
				 before->second.state != after.state ||
				 !item_owner_identity_equal(before->second.owner, after.owner) ||
				 before->second.owner_revision > current_owner->second)
				return false;
			changes.push_back({ &before->second, after });
		}
		if (applied &&
		    (selected_count != payload.item_count || maximum != result.max_item_revision))
			return false;
		// Original full physical/held-player proof and complete active cache
		// census are required by the sole friend before this value projection.
		// Every allocation/comparison precedes serialized nonfailing node writes.
		for (const auto &change : changes)
			*change.entry = change.after;
		from->second = current_from_revision;
		to->second = current_to_revision;
		giver->second = current_player_revision;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool item_ownership_runtime_apply(const item_transfer_payload &payload,
				  const item_transfer_result &result)
{
	if (payload.reason == item_transfer_reason::craft)
		return item_ownership_runtime_apply_craft(payload, result);
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
					       uint32_t player_pid, uint64_t pet_uid,
					       const corpse_lifecycle_result &result)
{
	const uint64_t corpse_owner_id = (static_cast<uint64_t>(owner_pid) << 32) |
					 static_cast<uint64_t>(save_id);
	if (!owner_pid || !save_id || !player_pid || result.owner_pid != owner_pid ||
	    result.save_id != save_id || result.action != corpse_lifecycle_action::raise_follower ||
	    result.corpse_revision || !result.corpse_owner_revision || result.room_owner_revision ||
	    !(pet_uid ? result.pet_owner_revision : result.player_owner_revision) ||
	    !result.wallet_revision ||
	    ((!result.item_count && result.max_item_revision) ||
	     (result.item_count && !result.max_item_revision)))
		return false;
	const item_owner_identity corpse = { item_owner_type::corpse, corpse_owner_id, 0 };
	const item_owner_identity destination =
		pet_uid ? item_owner_identity{ item_owner_type::pet, pet_uid, player_pid } :
			  item_owner_identity{ item_owner_type::player, player_pid, 0 };
	const uint64_t destination_revision = pet_uid ? result.pet_owner_revision :
							result.player_owner_revision;
	const auto corpse_revision = owner_revisions.find(corpse);
	const auto player_revision = owner_revisions.find(destination);
	if ((corpse_revision == owner_revisions.end() ? 0 : corpse_revision->second) !=
		    result.corpse_owner_revision - 1 ||
	    (player_revision == owner_revisions.end() ? 0 : player_revision->second) !=
		    destination_revision - 1)
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
		owner_revisions.insert_or_assign(destination, destination_revision);
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
		entry.owner = destination;
		entry.owner_revision = destination_revision;
	}
	return true;
}

bool item_ownership_runtime_apply_corpse_discarded(uint32_t owner_pid, uint32_t save_id,
						   const std::vector<uint64_t> &item_uids,
						   const corpse_lifecycle_result &result)
{
	if ((result.action != corpse_lifecycle_action::release &&
	     result.action != corpse_lifecycle_action::release_nested &&
	     result.action != corpse_lifecycle_action::destroy &&
	     result.action != corpse_lifecycle_action::resurrect &&
	     result.action != corpse_lifecycle_action::raise_follower) ||
	    result.owner_pid != owner_pid || result.save_id != save_id ||
	    item_uids.size() != result.discarded_item_count)
		return false;
	if (item_uids.empty())
		return !result.destruction_owner_revision && !result.max_discarded_item_revision;
	if (!owner_pid || !save_id || result.corpse_owner_revision < 2 ||
	    !result.destruction_owner_revision || !result.max_discarded_item_revision)
		return false;
	const item_owner_identity corpse = { item_owner_type::corpse,
					     item_corpse_owner_id(owner_pid, save_id), 0 };
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	const auto source = owner_revisions.find(corpse);
	const auto destination = owner_revisions.find(destruction);
	if ((source == owner_revisions.end() ? 0 : source->second) !=
		    result.corpse_owner_revision - 2 ||
	    (destination != owner_revisions.end() &&
	     destination->second != result.destruction_owner_revision - 1))
		return false;
	std::unordered_set<uint64_t> selected;
	std::vector<uint64_t> target_roots;
	uint64_t max_revision = 0;
	try
	{
		selected.reserve(item_uids.size());
		target_roots.reserve(item_uids.size());
		for (uint64_t uid : item_uids)
		{
			const auto found = entries.find(uid);
			if (!uid || !selected.insert(uid).second || found == entries.end() ||
			    !item_owner_identity_equal(found->second.owner, corpse) ||
			    found->second.state != item_custody_state::active ||
			    found->second.item_revision == UINT64_MAX)
				return false;
			max_revision = std::max(max_revision, found->second.item_revision + 1);
		}
		for (uint64_t uid : item_uids)
		{
			uint64_t root = uid;
			uint64_t parent = entries.find(uid)->second.parent_item_uid;
			size_t depth = 0;
			while (selected.contains(parent) && depth++ < selected.size())
			{
				root = parent;
				parent = entries.find(parent)->second.parent_item_uid;
			}
			if (selected.contains(parent))
				return false;
			target_roots.push_back(root);
		}
		owner_revisions.reserve(owner_revisions.size() + 2);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (max_revision != result.max_discarded_item_revision)
		return false;
	for (size_t index = 0; index < item_uids.size(); ++index)
	{
		auto &entry = entries.find(item_uids[index])->second;
		entry.root_item_uid = target_roots[index];
		if (!selected.contains(entry.parent_item_uid))
			entry.parent_item_uid = 0;
		++entry.item_revision;
		entry.owner = destruction;
		entry.owner_revision = result.destruction_owner_revision;
		entry.state = item_custody_state::destroyed;
	}
	owner_revisions.insert_or_assign(corpse, result.corpse_owner_revision - 1);
	owner_revisions.insert_or_assign(destruction, result.destruction_owner_revision);
	return true;
}

bool item_ownership_runtime_apply_world_corpse_raise(uint64_t source_uid, int32_t room_vnum,
						     uint32_t player_pid, uint64_t pet_uid,
						     const std::vector<uint64_t> &durable_uids,
						     const std::vector<uint64_t> &discarded_uids,
						     const corpse_lifecycle_result &result)
{
	const uint64_t result_source = (static_cast<uint64_t>(result.owner_pid) << 32) |
				       static_cast<uint64_t>(result.save_id);
	if (!source_uid || source_uid != result_source || room_vnum <= 0 || !player_pid ||
	    result.action != corpse_lifecycle_action::raise_world_follower ||
	    result.corpse_revision || result.catalog_revision != result.corpse_owner_revision ||
	    !result.corpse_owner_revision || result.room_owner_revision ||
	    result.player_owner_revision || result.wallet_revision || result.bank_revision ||
	    result.item_count != durable_uids.size() ||
	    result.discarded_item_count != discarded_uids.size() || !result.discarded_item_count ||
	    (pet_uid ? (!result.pet_owner_revision || pet_uid != source_uid) :
		       result.pet_owner_revision))
		return false;
	const item_owner_identity room = { item_owner_type::room, static_cast<uint64_t>(room_vnum),
					   0 };
	const item_owner_identity pet = { item_owner_type::pet, pet_uid, player_pid };
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	std::unordered_set<uint64_t> durable;
	std::unordered_set<uint64_t> discarded;
	struct planned_entry
	{
		uint64_t uid = 0;
		uint64_t root = 0;
		uint64_t parent = 0;
		uint64_t revision = 0;
		bool destroy = false;
	};
	std::vector<planned_entry> planned;
	size_t boundary_count = 0;
	uint64_t durable_max = 0;
	uint64_t discarded_max = 0;
	try
	{
		durable.reserve(durable_uids.size());
		discarded.reserve(discarded_uids.size());
		planned.reserve(durable_uids.size() + discarded_uids.size());
		for (uint64_t uid : durable_uids)
			if (!uid || !durable.insert(uid).second)
				return false;
		for (uint64_t uid : discarded_uids)
			if (!uid || durable.contains(uid) || !discarded.insert(uid).second)
				return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (!discarded.contains(source_uid))
		return false;
	for (const auto &[uid, entry] : entries)
	{
		if (!item_owner_identity_equal(entry.owner, room) ||
		    entry.root_item_uid != source_uid)
			continue;
		const bool destroy = discarded.contains(uid);
		if ((!destroy && !durable.contains(uid)) ||
		    entry.state != item_custody_state::active || entry.item_revision == UINT64_MAX)
			return false;
		const bool parent_selected = entry.parent_item_uid &&
					     (durable.contains(entry.parent_item_uid) ||
					      discarded.contains(entry.parent_item_uid));
		if (uid == source_uid ? entry.parent_item_uid != 0 : !parent_selected)
			return false;
		const bool parent_destroyed = discarded.contains(entry.parent_item_uid);
		const bool boundary = entry.parent_item_uid && parent_destroyed != destroy;
		boundary_count += boundary ? 1 : 0;
		bool detached = false;
		uint64_t ancestor_uid = entry.parent_item_uid;
		for (size_t depth = 0; ancestor_uid; ++depth)
		{
			if (depth > durable.size() + discarded.size())
				return false;
			const auto ancestor = entries.find(ancestor_uid);
			if (ancestor == entries.end())
				return false;
			if (discarded.contains(ancestor_uid) != destroy)
				detached = true;
			ancestor_uid = ancestor->second.parent_item_uid;
		}
		uint64_t root = uid;
		uint64_t parent = entry.parent_item_uid;
		if (parent && (destroy ? discarded.contains(parent) : durable.contains(parent)))
		{
			root = parent;
			for (size_t depth = 0; depth <= durable.size() + discarded.size(); ++depth)
			{
				const auto ancestor = entries.find(root);
				if (ancestor == entries.end())
					return false;
				const uint64_t next = ancestor->second.parent_item_uid;
				if (!next ||
				    !(destroy ? discarded.contains(next) : durable.contains(next)))
					break;
				root = next;
				if (depth == durable.size() + discarded.size())
					return false;
			}
		}
		else
			parent = 0;
		const uint64_t revision =
			entry.item_revision + (detached ? 1 : 0) + ((destroy || pet_uid) ? 1 : 0);
		if (revision < entry.item_revision)
			return false;
		if (destroy)
			discarded_max = std::max(discarded_max, revision);
		else
			durable_max = std::max(durable_max, revision);
		planned.push_back({ uid, root, parent, revision, destroy });
	}
	if (planned.size() != durable.size() + discarded.size() ||
	    durable_max != result.max_item_revision ||
	    discarded_max != result.max_discarded_item_revision)
		return false;
	const uint64_t source_steps = boundary_count + 1 + (pet_uid ? 1 : 0);
	if (result.corpse_owner_revision < source_steps)
		return false;
	const auto source_revision = owner_revisions.find(room);
	const auto pet_revision = pet_uid ? owner_revisions.find(pet) : owner_revisions.end();
	const auto destroy_revision = owner_revisions.find(destruction);
	if ((source_revision == owner_revisions.end() ? 0 : source_revision->second) !=
		    result.corpse_owner_revision - source_steps ||
	    (pet_uid && (pet_revision == owner_revisions.end() ? 0 : pet_revision->second) !=
				result.pet_owner_revision - 1) ||
	    (destroy_revision != owner_revisions.end() &&
	     destroy_revision->second != result.destruction_owner_revision - 1))
		return false;
	try
	{
		owner_revisions.reserve(owner_revisions.size() + 3);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (const planned_entry &change : planned)
	{
		auto found = entries.find(change.uid);
		if (found == entries.end())
			return false;
		found->second.root_item_uid = change.root;
		found->second.parent_item_uid = change.parent;
		found->second.item_revision = change.revision;
		found->second.owner = change.destroy ? destruction : (pet_uid ? pet : room);
		found->second.owner_revision = change.destroy ? result.destruction_owner_revision :
					       pet_uid	      ? result.pet_owner_revision :
								result.corpse_owner_revision;
		if (change.destroy)
			found->second.state = item_custody_state::destroyed;
	}
	owner_revisions.insert_or_assign(room, result.corpse_owner_revision);
	owner_revisions.insert_or_assign(destruction, result.destruction_owner_revision);
	if (pet_uid)
		owner_revisions.insert_or_assign(pet, result.pet_owner_revision);
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
		       (owner.type == item_owner_type::pet && owner.context_id == player_pid) ||
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

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
bool hydrate_storage_add(size_t &bytes, size_t amount) noexcept
{
	if (amount > SIZE_MAX - bytes)
		return false;
	bytes += amount;
	return true;
}
bool hydrate_storage_rows(size_t &bytes, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) && hydrate_storage_add(bytes, count * width);
}
template <typename Container> size_t hydrate_node_bytes() noexcept
{
	using node =
		std::__detail::_Hash_node<typename Container::value_type,
					  std::__cache_default<typename Container::key_type,
							       typename Container::hasher>::value>;
	return sizeof(node);
}
template <typename Container>
bool hydrate_hash_heap(const Container &container, size_t &bytes) noexcept
{
	// GCC13's bucket_count==1 uses its embedded singleton bucket. Every allocated
	// prime bucket array has at least two slots, including an empty reserved map.
	return hydrate_storage_rows(bytes, container.size(), hydrate_node_bytes<Container>()) &&
	       (container.bucket_count() == 1 ||
		hydrate_storage_rows(bytes, container.bucket_count(),
				     sizeof(std::__detail::_Hash_node_base *)));
}
bool hydrate_cache_bytes(size_t &bytes) noexcept
{
	bytes = sizeof(entries) + sizeof(owner_revisions);
	return hydrate_hash_heap(entries, bytes) && hydrate_hash_heap(owner_revisions, bytes);
}
struct hydrate_previous_entry
{
	uint64_t item_uid;
	bool existed;
	item_ownership_runtime_entry value;
};
struct hydrate_previous_owner
{
	item_owner_identity owner;
	bool existed;
	uint64_t revision;
};
struct hydrate_workspace
{
	std::vector<hydrate_previous_entry> previous_entries;
	std::vector<hydrate_previous_owner> previous_owners;
	std::unordered_set<uint64_t> item_uids;
	std::unordered_map<item_owner_identity, uint64_t, owner_hash, owner_equal> incoming_owners;
	hydrate_previous_entry saved_entry{};
	hydrate_previous_owner saved_owner{};
	// The owning maps have their original, private, unchanged default load factor
	// of one. This actual policy object models reserve's real prime bucket request.
	std::__detail::_Prime_rehash_policy policy;
};
struct hydrate_live_storage
{
	hydrate_workspace &work;
	const size_t &fixed;
	bool bytes(size_t &output) const noexcept
	{
		size_t cache = 0;
		output = fixed;
		return hydrate_cache_bytes(cache) && hydrate_storage_add(output, cache) &&
		       hydrate_storage_rows(output, work.previous_entries.capacity(),
					    sizeof(hydrate_previous_entry)) &&
		       hydrate_storage_rows(output, work.previous_owners.capacity(),
					    sizeof(hydrate_previous_owner)) &&
		       hydrate_hash_heap(work.item_uids, output) &&
		       hydrate_hash_heap(work.incoming_owners, output);
	}
	bool admit(size_t extra, bool (*reserve)(size_t, void *) noexcept,
		   void *context) const noexcept
	{
		size_t current = 0;
		return bytes(current) && hydrate_storage_add(current, extra) && reserve &&
		       reserve(current, context);
	}
};
template <typename Container>
bool hydrate_reserve_bucket_request(const Container &container, size_t count,
				    std::__detail::_Prime_rehash_policy &policy,
				    size_t &request) noexcept
{
	// Exact GCC13 _Rehash_base::reserve -> _Hashtable::rehash sequence. Existing
	// buckets are already in the live census and survive until fresh allocation
	// and rehash complete. No insertion rehash occurs after these complete reserves.
	if (container.size() == SIZE_MAX || container.max_load_factor() != policy.max_load_factor())
		return false;
	policy._M_reset();
	const size_t buckets =
		policy._M_next_bkt(std::max(policy._M_bkt_for_elements(count),
					    policy._M_bkt_for_elements(container.size() + 1)));
	request = 0;
	return buckets == container.bucket_count() ||
	       hydrate_storage_rows(request, buckets, sizeof(std::__detail::_Hash_node_base *));
}
#endif
} // namespace

bool item_ownership_runtime_cache_storage_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
	if (!output)
		return false;
	size_t candidate = 0;
	if (!hydrate_cache_bytes(candidate))
		return false;
	*output = candidate;
	return true;
#else
	(void)output;
	return false;
#endif
}

bool item_ownership_runtime_hydrate_many_atomic_bounded(const item_ownership_runtime_entry *batch,
							size_t count,
							bool (*reserve)(size_t, void *) noexcept,
							void *context, size_t outer_live) noexcept
{
	if ((!batch && count) || count > ITEM_OWNERSHIP_RUNTIME_MAX)
		return false;
	if (!count)
		return true;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)reserve;
	(void)context;
	(void)outer_live;
	return false;
#else
	size_t initial_cache = 0;
	if (!hydrate_cache_bytes(initial_cache) || outer_live < initial_cache)
		return false;
	size_t fixed = outer_live - initial_cache;
	if (!hydrate_storage_add(fixed, sizeof(hydrate_workspace)) ||
	    !hydrate_storage_add(fixed, sizeof(hydrate_live_storage)))
		return false;
	size_t initial = fixed;
	if (!hydrate_storage_add(initial, initial_cache) || !reserve || !reserve(initial, context))
		return false;
	hydrate_workspace work;
	hydrate_live_storage live{ work, fixed };
	size_t new_entries = 0;
	try
	{
		size_t request = 0;
		if (!hydrate_storage_rows(request, count, sizeof(hydrate_previous_entry)) ||
		    !live.admit(request, reserve, context))
			return false;
		work.previous_entries.reserve(count);
		if (!hydrate_reserve_bucket_request(work.item_uids, count, work.policy, request) ||
		    !live.admit(request, reserve, context))
			return false;
		work.item_uids.reserve(count);
		if (!hydrate_reserve_bucket_request(work.incoming_owners, count, work.policy,
						    request) ||
		    !live.admit(request, reserve, context))
			return false;
		work.incoming_owners.reserve(count);
		for (size_t index = 0; index < count; ++index)
		{
			const item_ownership_runtime_entry &entry = batch[index];
			if (!entry.item_uid || !entry.root_item_uid ||
			    !item_owner_identity_valid(entry.owner) ||
			    entry.state == item_custody_state::absent || entry.vnum < 0)
				return false;
			// Unique-key insertion does not allocate for a duplicate. Complete reserve
			// already prevents rehash; the request is the actual owning hash node.
			request = work.item_uids.find(entry.item_uid) == work.item_uids.end() ?
					  hydrate_node_bytes<decltype(work.item_uids)>() :
					  0;
			if (!live.admit(request, reserve, context) ||
			    !work.item_uids.insert(entry.item_uid).second)
				return false;
			const auto incoming_owner = work.incoming_owners.find(entry.owner);
			if (incoming_owner != work.incoming_owners.end())
			{
				if (incoming_owner->second != entry.owner_revision)
					return false;
			}
			else
			{
				if (!live.admit(hydrate_node_bytes<decltype(work.incoming_owners)>(),
						reserve, context))
					return false;
				work.incoming_owners.emplace(entry.owner, entry.owner_revision);
			}
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
			work.saved_entry.item_uid = entry.item_uid;
			work.saved_entry.existed = found != entries.end();
			if (found != entries.end())
				work.saved_entry.value = found->second;
			else
			{
				if (!live.admit(sizeof(item_ownership_runtime_entry), reserve,
						context))
					return false;
				work.saved_entry.value = item_ownership_runtime_entry{};
			}
			work.previous_entries.push_back(work.saved_entry);
			if (found == entries.end())
				++new_entries;
		}
		request = 0;
		if (!hydrate_storage_rows(request, work.incoming_owners.size(),
					  sizeof(hydrate_previous_owner)) ||
		    !live.admit(request, reserve, context))
			return false;
		work.previous_owners.reserve(work.incoming_owners.size());
		for (const auto &[owner, revision] : work.incoming_owners)
		{
			const auto found = owner_revisions.find(owner);
			if (found != owner_revisions.end() && found->second > revision)
				return false;
			work.saved_owner.owner = owner;
			work.saved_owner.existed = found != owner_revisions.end();
			work.saved_owner.revision = found != owner_revisions.end() ? found->second :
										     0;
			work.previous_owners.push_back(work.saved_owner);
		}
		if (entries.size() > ITEM_OWNERSHIP_RUNTIME_MAX - new_entries)
			return false;
		size_t entry_target = entries.size(), owner_target = owner_revisions.size();
		if (!hydrate_storage_add(entry_target, new_entries) ||
		    !hydrate_storage_add(owner_target, work.incoming_owners.size()) ||
		    !hydrate_reserve_bucket_request(entries, entry_target, work.policy, request) ||
		    !live.admit(request, reserve, context))
			return false;
		entries.reserve(entry_target);
		if (!hydrate_reserve_bucket_request(owner_revisions, owner_target, work.policy,
						    request) ||
		    !live.admit(request, reserve, context))
			return false;
		owner_revisions.reserve(owner_target);
		// Admit the exact final monotonic node peak BEFORE the first data mutation.
		// Reserves cover every possible insertion, including already-present owners;
		// inserts need no further budget callback and cannot leave partial effects on
		// a resource refusal. Allocation failure uses the original content rollback.
		size_t new_owners = 0;
		for (const auto &owner : work.previous_owners)
			if (!owner.existed)
				++new_owners;
		request = 0;
		if (!hydrate_storage_rows(request, new_entries,
					  hydrate_node_bytes<decltype(entries)>()) ||
		    !hydrate_storage_rows(request, new_owners,
					  hydrate_node_bytes<decltype(owner_revisions)>()) ||
		    !live.admit(request, reserve, context))
			return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	catch (...)
	{
		return false;
	}
	try
	{
		for (size_t index = 0; index < count; ++index)
			entries.insert_or_assign(batch[index].item_uid, batch[index]);
		for (const auto &[owner, revision] : work.incoming_owners)
			owner_revisions.insert_or_assign(owner, revision);
	}
	catch (...)
	{
		// POD/noexcept hash/equality operations make allocation the ordinary failure;
		// the noexcept companion also restores contents for any library refusal.
		for (const hydrate_previous_entry &entry : work.previous_entries)
			if (entry.existed)
				entries[entry.item_uid] = entry.value;
			else
				entries.erase(entry.item_uid);
		for (const hydrate_previous_owner &owner : work.previous_owners)
			if (owner.existed)
				owner_revisions[owner.owner] = owner.revision;
			else
				owner_revisions.erase(owner.owner);
		return false;
	}
	return true;
#endif
}

bool item_ownership_runtime_hydrate_owner_bounded(const item_owner_identity &owner,
						  uint64_t revision,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer_live) noexcept
{
	if (!item_owner_identity_valid(owner))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)revision;
	(void)reserve;
	(void)context;
	(void)outer_live;
	return false;
#else
	// Match the original owner's monotonic revision law, including a valid zero.
	// A zero-item batch does not establish this real owner cache entry.
	const auto found = owner_revisions.find(owner);
	if (found != owner_revisions.end() && found->second > revision)
		return false;
	struct workspace
	{
		size_t initial_cache = 0, fixed = 0, current = 0, target = 0, request = 0;
		std::__detail::_Prime_rehash_policy policy;
	};
	size_t initial_cache = 0, initial = outer_live;
	if (!hydrate_cache_bytes(initial_cache) || outer_live < initial_cache ||
	    !hydrate_storage_add(initial, sizeof(workspace)) || !reserve ||
	    !reserve(initial, context))
		return false;
	workspace work;
	work.initial_cache = initial_cache;
	work.fixed = outer_live - initial_cache;
	if (!hydrate_storage_add(work.fixed, sizeof(workspace)))
		return false;
	if (found != owner_revisions.end())
	{
		// Admission precedes the only content change; no allocation or callback
		// follows the assignment. This preserves the original existing-owner path.
		found->second = revision;
		return true;
	}
	try
	{
		work.target = owner_revisions.size();
		if (!hydrate_storage_add(work.target, 1) ||
		    !hydrate_reserve_bucket_request(owner_revisions, work.target, work.policy,
						    work.request))
			return false;
		work.current = work.fixed;
		if (!hydrate_storage_add(work.current, work.initial_cache) ||
		    !hydrate_storage_add(work.current, work.request) ||
		    !reserve(work.current, context))
			return false;
		// Charge the actual fresh GCC13 bucket request before reserve. Old buckets
		// remain counted until reserve completes; growth survives a later refusal.
		owner_revisions.reserve(work.target);
		if (!hydrate_cache_bytes(work.current) ||
		    !hydrate_storage_add(work.current, work.fixed) ||
		    !hydrate_storage_add(work.current,
					 hydrate_node_bytes<decltype(owner_revisions)>()) ||
		    !reserve(work.current, context))
			return false;
		// The complete reserve prevents insertion rehash. A failed node allocation
		// leaves map contents unchanged; no synthetic item or owner is introduced.
		return owner_revisions.emplace(owner, revision).second;
	}
	catch (...)
	{
		return false;
	}
#endif
}
