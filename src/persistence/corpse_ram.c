#include "persistence/corpse_ram.h"
#include "economy/collector_ram.h"
#include "economy/economic_state.h"
#include "item/item_ownership_runtime.h"
#include "player/artifact_restitution_ram.h"
#include "core/prototypes.h"
#include "classes/necromancy.h"
#include "core/utils.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <cerrno>
#include <map>
#include <set>
#include <strings.h>

extern P_obj object_list;

namespace
{
struct corpse_state
{
	corpse_lifecycle_payload metadata;
	uint64_t revision = 0;
};
std::map<uint64_t, corpse_state> corpses;
uint64_t catalog_revision = 1, next_catalog_revision = 1;
bool flatfile = false;
corpse_state *target = nullptr;
corpse_state staged;
std::vector<artifact_restitution_state> staged_artifacts;

uint64_t revision(const item_owner_identity &owner)
{
	uint64_t value = 0;
	item_ownership_runtime_peek_owner_revision(owner, &value);
	return value;
}
P_obj object(uint64_t uid)
{
	for (P_obj current = object_list; current; current = current->next)
		if (current->obj_uid == uid)
			return current;
	return nullptr;
}
P_obj corpse_object(uint32_t pid, uint32_t save_id)
{
	for (P_obj current = object_list; current; current = current->next)
		if (GET_ITEM_TYPE(current) == ITEM_CORPSE &&
		    current->value[CORPSE_PID] == static_cast<int32_t>(pid) &&
		    current->value[CORPSE_SAVEID] == static_cast<int32_t>(save_id))
			return current;
	return nullptr;
}
critical_apply_result rejected(unsigned int code)
{
	return { critical_apply_outcome::terminal_failure, 0, code };
}
critical_apply_result encoded(const corpse_lifecycle_result &result)
{
	std::array<uint8_t, CORPSE_LIFECYCLE_RESULT_BYTES> bytes{};
	if (!corpse_lifecycle_command_encode_result(result, &bytes))
		return rejected(EINVAL);
	const uint64_t clock =
		flatfile ? result.catalog_revision :
			   std::max({ result.corpse_revision, result.catalog_revision,
				      result.corpse_owner_revision, result.room_owner_revision,
				      result.player_owner_revision, result.wallet_revision,
				      result.bank_revision, result.max_item_revision,
				      result.collector_catalog_changed ? collector_ram_revision() :
									 0 });
	critical_apply_result prepared{ critical_apply_outcome::applied, clock, 0 };
	prepared.result_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), prepared.result_payload.begin());
	return prepared;
}

critical_apply_result world_raise(const critical_command &command,
				  const corpse_lifecycle_payload &payload)
{
	const uint64_t uid = (static_cast<uint64_t>(payload.owner_pid) << 32) | payload.save_id;
	const item_owner_identity room{ item_owner_type::room,
					static_cast<uint64_t>(payload.room_vnum), 0 };
	const item_owner_identity player{ item_owner_type::player, payload.destination_player_pid,
					  0 };
	const item_owner_identity pet{ item_owner_type::pet, payload.pet_uid,
				       payload.destination_player_pid };
	const item_owner_identity destruction{ item_owner_type::destruction, 0, 0 };
	item_ownership_runtime_entry root;
	if (!item_ownership_runtime_lookup(uid, &root) ||
	    !item_owner_identity_equal(root.owner, room) ||
	    root.item_revision != payload.expected_corpse_revision ||
	    revision(room) != payload.expected_room_revision ||
	    revision(player) != payload.expected_player_revision ||
	    (payload.pet_uid && revision(pet)))
		return rejected(ESTALE);
	std::vector<item_ownership_runtime_entry> owned;
	if (!item_ownership_runtime_snapshot_owner(room, SIZE_MAX, &owned))
		return rejected(ENOMEM);
	std::map<uint64_t, item_ownership_runtime_entry> tree;
	std::set<uint64_t> discarded;
	for (const auto &entry : owned)
	{
		if (entry.root_item_uid != uid)
			continue;
		P_obj live = object(entry.item_uid);
		if (!live)
			return rejected(ENOENT);
		if (!flatfile && IS_SET(live->extra_flags, ITEM_ARTIFACT) &&
		    !IS_SET(live->extra_flags, ITEM_TRANSIENT))
			return rejected(EOPNOTSUPP);
		tree.emplace(entry.item_uid, entry);
		if (entry.item_uid == uid || IS_SET(live->extra_flags, ITEM_TRANSIENT))
			discarded.insert(entry.item_uid);
	}
	if (tree.empty() || tree.size() > ITEM_TRANSFER_MAX_ITEMS)
		return rejected(E2BIG);
	size_t boundaries = 0;
	for (const auto &[id, entry] : tree)
		if (entry.parent_item_uid &&
		    discarded.contains(id) != discarded.contains(entry.parent_item_uid))
			++boundaries;
	const uint64_t steps = boundaries + 1 + (payload.pet_uid != 0);
	if (revision(room) > UINT64_MAX - steps || revision(destruction) == UINT64_MAX)
		return rejected(ERANGE);
	corpse_lifecycle_result result;
	result.owner_pid = payload.owner_pid;
	result.save_id = payload.save_id;
	result.action = payload.action;
	result.corpse_owner_revision = revision(room) + steps;
	result.catalog_revision = flatfile ? catalog_revision + 1 : result.corpse_owner_revision;
	result.pet_owner_revision = payload.pet_uid ? 1 : 0;
	result.destruction_owner_revision = revision(destruction) + 1;
	std::vector<collector_custody_boundary_item> claims;
	for (const auto &[id, entry] : tree)
	{
		const bool destroyed = discarded.contains(id);
		bool detached = false;
		uint64_t ancestor = id;
		for (size_t depth = 0; ancestor; ++depth)
		{
			if (depth >= tree.size())
				return rejected(EINVAL);
			const auto found = tree.find(ancestor);
			if (found == tree.end())
				return rejected(EINVAL);
			const uint64_t parent = found->second.parent_item_uid;
			if (parent && discarded.contains(ancestor) != discarded.contains(parent))
				detached = true;
			ancestor = parent;
		}
		const uint64_t increments = detached + (destroyed || payload.pet_uid != 0);
		if (entry.item_revision > UINT64_MAX - increments)
			return rejected(ERANGE);
		const uint64_t next = entry.item_revision + increments;
		if (destroyed)
		{
			++result.discarded_item_count;
			result.max_discarded_item_revision =
				std::max(result.max_discarded_item_revision, next);
		}
		else
		{
			++result.item_count;
			result.max_item_revision = std::max(result.max_item_revision, next);
			if (payload.pet_uid)
				claims.push_back({ id, next });
		}
	}
	if (auto error = collector_ram_prepare_boundary(command, collector::reason::claimed, claims,
							&result.collector_catalog_changed))
		return rejected(error);
	if (flatfile)
		next_catalog_revision = catalog_revision + 1;
	return encoded(result);
}
}

void corpse_ram_reset(uint64_t revision_value, bool flatfile_value)
{
	corpses.clear();
	flatfile = flatfile_value;
	catalog_revision = revision_value;
	corpse_ram_begin();
}
bool corpse_ram_seed(const corpse_lifecycle_payload &metadata, uint64_t value)
{
	return metadata.owner_pid && metadata.save_id && value &&
	       corpses.emplace(item_corpse_owner_id(metadata.owner_pid, metadata.save_id),
			       corpse_state{ metadata, value })
		       .second;
}
void corpse_ram_begin()
{
	staged_artifacts.clear();
	target = nullptr;
	next_catalog_revision = catalog_revision;
}
void corpse_ram_note_world_change()
{
	if (flatfile)
		++next_catalog_revision;
}

unsigned int corpse_ram_prepare_item(const item_transfer_payload &payload,
				     item_transfer_result *result)
{
	const bool from = payload.from_owner.type == item_owner_type::corpse;
	const bool to = payload.to_owner.type == item_owner_type::corpse;
	if (!from && !to)
	{
		if (flatfile && (payload.from_owner.type == item_owner_type::room ||
				 payload.to_owner.type == item_owner_type::room))
		{
			if (catalog_revision == UINT64_MAX)
				return ERANGE;
			corpse_ram_note_world_change();
		}
		return 0;
	}
	if (!payload.corpse.present || !result || catalog_revision == UINT64_MAX)
		return EINVAL;
	const uint64_t key = from ? payload.from_owner.id : payload.to_owner.id;
	auto [found, inserted] = corpses.try_emplace(key);
	if (from && !found->second.revision)
		return ENOENT;
	if (found->second.revision == UINT64_MAX)
		return ERANGE;
	const auto &metadata = payload.corpse;
	if (found->second.revision &&
	    (strcasecmp(found->second.metadata.owner_name.c_str(), metadata.owner_name.c_str()) ||
	     found->second.metadata.values[CORPSE_RACEWAR] != metadata.values[CORPSE_RACEWAR]))
		return ESTALE;
	staged = found->second;
	staged.metadata.owner_pid = static_cast<uint32_t>(key >> 32);
	staged.metadata.save_id = static_cast<uint32_t>(key);
	staged.metadata.owner_name = metadata.owner_name;
	staged.metadata.room_vnum = metadata.room_vnum;
	staged.metadata.weight = metadata.weight;
	staged.metadata.values = metadata.values;
	staged.metadata.short_description = metadata.short_description;
	staged.metadata.description = metadata.description;
	staged.metadata.keywords = metadata.keywords;
	++staged.revision;
	target = &found->second;
	result->corpse_revision = staged.revision;
	next_catalog_revision = catalog_revision + 1;
	return 0;
}

critical_apply_result corpse_ram_prepare(const critical_command &command)
{
	corpse_lifecycle_payload payload;
	if (!corpse_lifecycle_command_decode_payload(command, &payload))
		return rejected(EINVAL);
	if (catalog_revision == UINT64_MAX)
		return rejected(ERANGE);
	if (payload.action == corpse_lifecycle_action::raise_world_follower)
		return world_raise(command, payload);
	const uint64_t key = item_corpse_owner_id(payload.owner_pid, payload.save_id);
	auto [found, inserted] = corpses.try_emplace(key);
	const auto &current = found->second;
	if (current.revision != payload.expected_corpse_revision ||
	    (current.revision &&
	     strcasecmp(current.metadata.owner_name.c_str(), payload.owner_name.c_str())))
		return rejected(ESTALE);
	if (payload.action != corpse_lifecycle_action::upsert && !current.revision)
		return rejected(ENOENT);
	corpse_lifecycle_result result;
	result.owner_pid = payload.owner_pid;
	result.save_id = payload.save_id;
	result.action = payload.action;
	result.catalog_revision = catalog_revision + 1;
	staged = current;
	const item_owner_identity source{ item_owner_type::corpse, key, 0 };
	std::vector<item_ownership_runtime_entry> owned;
	if (!item_ownership_runtime_snapshot_owner(source, SIZE_MAX, &owned))
		return rejected(ENOMEM);
	if (payload.action == corpse_lifecycle_action::upsert)
	{
		if (current.revision == UINT64_MAX ||
		    (current.revision &&
		     (current.metadata.values[CORPSE_PID] != payload.values[CORPSE_PID] ||
		      current.metadata.values[CORPSE_RACEWAR] != payload.values[CORPSE_RACEWAR] ||
		      current.metadata.values[CORPSE_SAVEID] != payload.values[CORPSE_SAVEID])))
			return rejected(ESTALE);
		staged.metadata = payload;
		result.corpse_revision = ++staged.revision;
	}
	else if (payload.action == corpse_lifecycle_action::remove)
	{
		if (!owned.empty() ||
		    std::any_of(current.metadata.money.begin(), current.metadata.money.end(),
				[](auto value) { return value != 0; }))
			return rejected(ENOTEMPTY);
		staged.revision = 0;
	}
	else
	{
		const bool resurrect = payload.action == corpse_lifecycle_action::resurrect;
		const bool raised = payload.action == corpse_lifecycle_action::raise_follower;
		const bool player_action =
			resurrect || raised ||
			(payload.action == corpse_lifecycle_action::release_nested &&
			 payload.destination_player_pid);
		const bool destroy = payload.action == corpse_lifecycle_action::destroy;
		const bool pet_action = raised && payload.pet_uid;
		item_owner_identity destination =
			destroy	   ? item_owner_identity{ item_owner_type::destruction, 0, 0 } :
			pet_action ? item_owner_identity{ item_owner_type::pet, payload.pet_uid,
							  payload.destination_player_pid } :
			player_action ?
				     item_owner_identity{ item_owner_type::player,
							  payload.destination_player_pid, 0 } :
				     item_owner_identity{ item_owner_type::room,
							  static_cast<uint64_t>(payload.room_vnum),
							  0 };
		const uint64_t expected = pet_action	? 0 :
					  player_action ? payload.expected_player_revision :
							  payload.expected_room_revision;
		if (revision(destination) != expected ||
		    (pet_action &&
		     revision({ item_owner_type::player, payload.destination_player_pid, 0 }) !=
			     payload.expected_player_revision))
			return rejected(ESTALE);
		if (current.metadata.room_vnum != payload.room_vnum)
			return rejected(ESTALE);
		if (payload.target_parent_item_uid)
		{
			item_ownership_runtime_entry parent;
			if (!item_ownership_runtime_lookup(payload.target_parent_item_uid,
							   &parent) ||
			    !item_owner_identity_equal(parent.owner, destination) ||
			    parent.item_revision != payload.expected_target_parent_revision ||
			    parent.root_item_uid != payload.target_root_item_uid ||
			    parent.state != item_custody_state::active)
				return rejected(ESTALE);
		}
		std::array<int64_t, 4> money{};
		if (flatfile)
			std::copy(current.metadata.money.begin(), current.metadata.money.end(),
				  money.begin());
		else
		{
			P_obj live = corpse_object(payload.owner_pid, payload.save_id);
			if (!live)
				return rejected(ENOENT);
			std::vector<P_obj> pending;
			for (P_obj child = live->contains; child; child = child->next_content)
				pending.push_back(child);
			while (!pending.empty())
			{
				P_obj child = pending.back();
				pending.pop_back();
				if (GET_ITEM_TYPE(child) == ITEM_MONEY)
					for (size_t i = 0; i < money.size(); ++i)
						money[i] += child->value[i];
				for (P_obj nested = child->contains; nested;
				     nested = nested->next_content)
					pending.push_back(nested);
			}
		}
		const item_owner_identity destruction{ item_owner_type::destruction, 0, 0 };
		uint64_t source_clock = revision(source), destination_clock = revision(destination),
			 destruction_clock = revision(destruction);
		std::vector<collector_custody_boundary_item> claims;
		for (const auto &entry : owned)
		{
			if (entry.item_revision == UINT64_MAX)
				return rejected(ERANGE);
			P_obj live = object(entry.item_uid);
			if (!live)
				return rejected(ENOENT);
			const bool discarded =
				!flatfile &&
				(IS_SET(live->extra_flags, ITEM_TRANSIENT) ||
				 (GET_ITEM_TYPE(live) == ITEM_MONEY && (player_action || destroy)));
			if (discarded)
			{
				++result.discarded_item_count;
				result.max_discarded_item_revision =
					std::max(result.max_discarded_item_revision,
						 entry.item_revision + 1);
			}
			else
			{
				if (!flatfile && IS_SET(live->extra_flags, ITEM_ARTIFACT))
				{
					const auto proof =
						artifact_restitution_ram_lookup(entry.vnum);
					if (!proof || !proof->domain.present ||
					    !proof->god.present || !proof->god.owned ||
					    proof->god.location_type != 5 ||
					    proof->god.location !=
						    static_cast<int32_t>(payload.owner_pid) ||
					    proof->revision == UINT64_MAX ||
					    (proof->uid_present && proof->uid != entry.item_uid) ||
					    (proof->item_revision_present &&
					     proof->item_revision != entry.item_revision) ||
					    std::any_of(staged_artifacts.begin(),
							staged_artifacts.end(),
							[&](const auto &a) {
								return a.vnum ==
								       static_cast<uint32_t>(
									       entry.vnum);
							}))
						return rejected(ESTALE);
					auto next = *proof;
					const bool player = player_action || pet_action;
					next.domain.owned = !destroy;
					next.domain.location_type = destroy ? 1 : player ? 3 : 4;
					next.domain.location =
						destroy ? -1 :
						player	? static_cast<int32_t>(
								  payload.destination_player_pid) :
							 payload.room_vnum;
					for (auto *legacy : { &next.god, &next.mortal })
						if (legacy->present)
						{
							legacy->owned = next.domain.owned;
							legacy->location_type =
								next.domain.location_type;
							legacy->location = next.domain.location;
						}
					if (destroy)
					{
						next.bind_owner = next.bind_row_owner = -1;
						next.bind_timer = next.bind_row_timer = 0;
						next.bind_present = true;
					}
					next.uid_present = next.item_revision_present = true;
					next.uid = entry.item_uid;
					next.item_revision = entry.item_revision + 1;
					++next.revision;
					staged_artifacts.push_back(next);
				}
				++result.item_count;
				result.max_item_revision =
					std::max(result.max_item_revision, entry.item_revision + 1);
				claims.push_back({ entry.item_uid, entry.item_revision + 1 });
			}
		}
		if (result.discarded_item_count)
		{
			if (source_clock == UINT64_MAX || destruction_clock == UINT64_MAX)
				return rejected(ERANGE);
			++source_clock;
			++destruction_clock;
			result.destruction_owner_revision = destruction_clock;
			if (destroy)
				destination_clock = destruction_clock;
		}
		if (source_clock == UINT64_MAX || destination_clock == UINT64_MAX)
			return rejected(ERANGE);
		result.corpse_owner_revision = source_clock + 1;
		if (pet_action)
			result.pet_owner_revision = destination_clock + 1;
		else if (player_action)
			result.player_owner_revision = destination_clock + 1;
		else
			result.room_owner_revision = destination_clock + 1;
		if (resurrect)
		{
			const auto room_clock =
				revision({ item_owner_type::room,
					   static_cast<uint64_t>(payload.old_room_vnum), 0 });
			if (room_clock != payload.expected_room_revision ||
			    room_clock == UINT64_MAX)
				return rejected(ESTALE);
			result.room_owner_revision = room_clock + 1;
		}
		if (player_action)
		{
			economic_player_state player;
			if (!economic_state_find(payload.destination_player_pid, &player))
				return rejected(ENOENT);
			if (player.balances.wallet_revision != payload.expected_wallet_revision ||
			    player.balances.wallet_revision == UINT64_MAX ||
			    (!flatfile && player.balances.bank_revision == UINT64_MAX))
				return rejected(ESTALE);
			for (size_t i = 0; i < money.size(); ++i)
			{
				if (player.balances.wallet.amount[i] != payload.money[i])
					return rejected(ESTALE);
				const auto next = money[i] + (resurrect ? 0 : payload.money[i]);
				if (next < 0 || next > INT32_MAX)
					return rejected(EOVERFLOW);
				result.wallet[i] = static_cast<int32_t>(next);
			}
			result.wallet_revision = player.balances.wallet_revision + 1;
			result.bank_revision = flatfile ? 0 : player.balances.bank_revision + 1;
		}
		if (auto error = collector_ram_prepare_boundary(
			    command, collector_corpse_lifecycle_boundary_reason(payload), claims,
			    &result.collector_catalog_changed))
			return rejected(error);
		staged.revision = 0;
	}
	target = &found->second;
	next_catalog_revision = catalog_revision + 1;
	return encoded(result);
}

void corpse_ram_publish() noexcept
{
	for (const auto &artifact : staged_artifacts)
		artifact_restitution_ram_update(artifact);
	staged_artifacts.clear();
	if (target)
		std::swap(*target, staged);
	catalog_revision = next_catalog_revision;
	target = nullptr;
}
