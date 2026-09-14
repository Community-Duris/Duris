#include "economy/collector_collection_preparation.h"

#include "classes/necromancy.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <limits>
#include <new>
#include <unordered_set>
#include <utility>
#include <vector>

extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern const int top_of_world;
extern int top_of_objt;

namespace
{
P_obj find_live_item(uint64_t uid)
{
	P_obj found = nullptr;
	std::unordered_set<P_obj> seen;
	try
	{
		for (P_obj object = object_list; object; object = object->next)
		{
			if (!seen.insert(object).second)
				return nullptr;
			if (object->obj_uid == uid)
			{
				if (found)
					return nullptr;
				found = object;
			}
		}
	}
	catch (const std::bad_alloc &)
	{
		return nullptr;
	}
	return found;
}

bool list_contains(P_obj head, P_obj sought)
{
	std::unordered_set<P_obj> seen;
	try
	{
		for (P_obj current = head; current; current = current->next_content)
		{
			if (!seen.insert(current).second)
				return false;
			if (current == sought)
				return true;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return false;
}

P_obj ownership_root(P_obj selected, uint64_t root_uid)
{
	P_obj current = selected;
	for (size_t depth = 0; current && depth <= COLLECTOR_COMMAND_MAX_ITEMS; ++depth)
	{
		if (current->obj_uid == root_uid)
			return current;
		if (!OBJ_INSIDE(current) || !current->loc.inside ||
		    !list_contains(current->loc.inside->contains, current))
			return nullptr;
		current = current->loc.inside;
	}
	return nullptr;
}

bool root_location_matches(P_obj root, const item_owner_identity &owner)
{
	if (!root || owner.context_id)
		return false;
	if (owner.type == item_owner_type::room)
		return OBJ_ROOM(root) && root->loc.room >= 0 && root->loc.room <= top_of_world &&
		       world[root->loc.room].number > 0 &&
		       static_cast<uint64_t>(world[root->loc.room].number) == owner.id &&
		       list_contains(world[root->loc.room].contents, root);
	if (owner.type != item_owner_type::corpse || !OBJ_INSIDE(root) || !root->loc.inside ||
	    !list_contains(root->loc.inside->contains, root))
		return false;
	P_obj corpse = root->loc.inside;
	if (GET_ITEM_TYPE(corpse) != ITEM_CORPSE ||
	    !IS_SET(corpse->value[CORPSE_FLAGS], PC_CORPSE) || corpse->value[CORPSE_PID] <= 0 ||
	    corpse->value[CORPSE_SAVEID] <= 0)
		return false;
	return owner.id ==
	       item_corpse_owner_id(static_cast<uint32_t>(corpse->value[CORPSE_PID]),
				    static_cast<uint32_t>(corpse->value[CORPSE_SAVEID]));
}

bool capture_authority_tree(P_obj object, uint64_t root_uid, uint64_t parent_uid,
			    const item_owner_identity &owner, uint64_t owner_revision,
			    std::unordered_set<P_obj> *seen,
			    std::vector<item_transfer_entry> *items)
{
	if (!object || !seen || !items || object->R_num < 0 || object->R_num > top_of_objt ||
	    !object->obj_uid || items->size() >= COLLECTOR_COMMAND_MAX_ITEMS ||
	    !seen->insert(object).second)
		return false;
	item_ownership_runtime_entry runtime = {};
	if (!item_ownership_runtime_lookup(object->obj_uid, &runtime) ||
	    runtime.item_uid != object->obj_uid || runtime.root_item_uid != root_uid ||
	    runtime.parent_item_uid != parent_uid ||
	    !item_owner_identity_equal(runtime.owner, owner) ||
	    runtime.owner_revision != owner_revision || !runtime.item_revision ||
	    runtime.item_revision == std::numeric_limits<uint64_t>::max() ||
	    runtime.vnum != OBJ_VNUM(object) || runtime.vnum <= 0 ||
	    runtime.state != item_custody_state::active)
		return false;
	items->push_back({ runtime.item_uid, runtime.root_item_uid, runtime.parent_item_uid,
			   runtime.item_revision, runtime.vnum, runtime.state });
	for (P_obj child = object->contains; child; child = child->next_content)
		if (!OBJ_INSIDE(child) || child->loc.inside != object ||
		    !capture_authority_tree(child, root_uid, object->obj_uid, owner, owner_revision,
					    seen, items))
			return false;
	return true;
}

bool capture_exact_blob(P_obj selected, std::vector<uint8_t> *blob)
{
	if (!selected || !blob)
		return false;
	std::vector<player_item_snapshot> snapshots;
	if (player_item_snapshot_tree_capture(selected, &snapshots, nullptr) !=
		    player_snapshot_capture_result::ok ||
	    snapshots.empty())
		return false;
	int64_t children_weight = 0;
	for (P_obj child = selected->contains; child; child = child->next_content)
	{
		if (child->weight < 0 ||
		    child->weight > std::numeric_limits<int64_t>::max() - children_weight)
			return false;
		children_weight += child->weight;
	}
	if (selected->weight < children_weight)
		return false;
	const int64_t own_weight = static_cast<int64_t>(selected->weight) - children_weight;
	if (own_weight < 0 || own_weight > std::numeric_limits<int32_t>::max())
		return false;
	player_item_snapshot singleton = std::move(snapshots.front());
	singleton.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	singleton.equipment_slot = 0;
	singleton.weight = static_cast<int32_t>(own_weight);
	std::vector<player_item_snapshot> exact;
	try
	{
		exact.push_back(std::move(singleton));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return player_item_snapshot_list_encode(exact, blob) == player_snapshot_codec_result::ok &&
	       !blob->empty() && blob->size() <= COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES;
}

bool capture_live(const collector_command_payload &payload, P_obj *selected_out,
		  std::vector<item_transfer_entry> *items_out, std::vector<uint8_t> *blob_out)
{
	if (payload.action != collector_action::collect || !payload.selected_item_uid ||
	    !payload.item_count || payload.item_count > COLLECTOR_COMMAND_MAX_ITEMS ||
	    !selected_out || !items_out)
		return false;
	P_obj selected = find_live_item(payload.selected_item_uid);
	if (!selected)
		return false;
	P_obj root = ownership_root(selected, payload.items[0].root_item_uid);
	if (!root || !root_location_matches(root, payload.from_owner))
		return false;
	std::unordered_set<P_obj> seen;
	std::vector<item_transfer_entry> items;
	try
	{
		seen.reserve(payload.item_count);
		items.reserve(payload.item_count);
		if (!capture_authority_tree(root, root->obj_uid, 0, payload.from_owner,
					    payload.expected_from_owner_revision, &seen, &items))
			return false;
		std::sort(items.begin(), items.end(), [](const auto &left, const auto &right)
			  { return left.item_uid < right.item_uid; });
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (items.size() != payload.item_count ||
	    !std::equal(items.begin(), items.end(), payload.items.begin(),
			[](const auto &left, const auto &right)
			{
				return left.item_uid == right.item_uid &&
				       left.root_item_uid == right.root_item_uid &&
				       left.parent_item_uid == right.parent_item_uid &&
				       left.expected_item_revision ==
					       right.expected_item_revision &&
				       left.vnum == right.vnum &&
				       left.expected_state == right.expected_state;
			}))
		return false;
	if (blob_out && !capture_exact_blob(selected, blob_out))
		return false;
	*selected_out = selected;
	*items_out = std::move(items);
	return true;
}
}

bool collector_collection_item_eligible(P_obj object)
{
	return object && object->obj_uid && object->R_num >= 0 && object->R_num <= top_of_objt &&
	       GET_ITEM_TYPE(object) != ITEM_MONEY && GET_ITEM_TYPE(object) != ITEM_CORPSE &&
	       !IS_ARTIFACT(object) && !(object->name && IS_UNIQUE(object)) &&
	       !IS_SET(object->extra_flags, ITEM_TRANSIENT) &&
	       !IS_SET(object->extra_flags, ITEM_NORENT) &&
	       !IS_SET(object->extra_flags, ITEM_NOSELL) &&
	       !IS_OBJ_STAT2(object, ITEM2_ACCOUNT_BOUND) && CAN_WEAR(object, ITEM_TAKE);
}

collector_collection_prepare_outcome
collector_collection_prepare(const collector::record &entry, uint64_t observed_at,
			     std::unique_ptr<collector_command_payload> *payload)
{
	if (!payload || !observed_at || !collector::valid_record(entry) ||
	    entry.status != collector::state::candidate)
		return collector_collection_prepare_outcome::invalid_request;
	if (observed_at < entry.collect_at)
		return collector_collection_prepare_outcome::not_due;
	P_obj selected = find_live_item(entry.uid);
	if (!selected)
		return collector_collection_prepare_outcome::missing_item;
	if (!collector_collection_item_eligible(selected))
		return collector_collection_prepare_outcome::excluded;
	item_ownership_runtime_entry selected_runtime = {};
	if (!item_ownership_runtime_lookup(entry.uid, &selected_runtime))
		return collector_collection_prepare_outcome::stale_custody;
	if (selected_runtime.state == item_custody_state::destroyed)
		return collector_collection_prepare_outcome::destroyed;
	if (selected_runtime.state == item_custody_state::quarantined)
		return collector_collection_prepare_outcome::stale_custody;
	if (selected_runtime.owner.type != item_owner_type::room &&
	    selected_runtime.owner.type != item_owner_type::corpse)
		return collector_collection_prepare_outcome::claimed;
	if (selected_runtime.item_revision < entry.item_revision ||
	    selected_runtime.item_revision == std::numeric_limits<uint64_t>::max())
		return collector_collection_prepare_outcome::stale_custody;
	P_obj root = ownership_root(selected, selected_runtime.root_item_uid);
	if (!root || !root_location_matches(root, selected_runtime.owner))
		return collector_collection_prepare_outcome::invalid_topology;

	std::vector<item_transfer_entry> items;
	std::vector<uint8_t> blob;
	try
	{
		std::unordered_set<P_obj> seen;
		seen.reserve(32);
		items.reserve(32);
		if (!capture_authority_tree(root, root->obj_uid, 0, selected_runtime.owner,
					    selected_runtime.owner_revision, &seen, &items))
			return items.size() >= COLLECTOR_COMMAND_MAX_ITEMS ?
				       collector_collection_prepare_outcome::limit_exceeded :
				       collector_collection_prepare_outcome::invalid_topology;
		std::sort(items.begin(), items.end(), [](const auto &left, const auto &right)
			  { return left.item_uid < right.item_uid; });
		if (!capture_exact_blob(selected, &blob))
			return collector_collection_prepare_outcome::invalid_topology;
		auto candidate = std::make_unique<collector_command_payload>();
		candidate->action = collector_action::collect;
		candidate->target_state = item_custody_state::active;
		candidate->listing = entry.listing;
		candidate->expected_listing_revision = entry.revision;
		candidate->observed_at = observed_at;
		candidate->from_owner = selected_runtime.owner;
		candidate->to_owner = { item_owner_type::collector,
					item_collector_owner_id(entry.listing), 0 };
		candidate->expected_from_owner_revision = selected_runtime.owner_revision;
		if (!item_ownership_runtime_owner_revision(candidate->to_owner,
							   &candidate->expected_to_owner_revision))
			return collector_collection_prepare_outcome::stale_custody;
		candidate->selected_item_uid = entry.uid;
		candidate->item_count = static_cast<uint16_t>(items.size());
		std::copy(items.begin(), items.end(), candidate->items.begin());
		candidate->item_blob_size = static_cast<uint32_t>(blob.size());
		std::copy(blob.begin(), blob.end(), candidate->item_blob.begin());
		*payload = std::move(candidate);
	}
	catch (const std::bad_alloc &)
	{
		return collector_collection_prepare_outcome::allocation_failure;
	}
	return collector_collection_prepare_outcome::prepared;
}

bool collector_collection_live_matches(const collector_command_payload &payload, P_obj *selected)
{
	if (!selected)
		return false;
	std::vector<item_transfer_entry> items;
	std::vector<uint8_t> blob;
	P_obj candidate = nullptr;
	if (!capture_live(payload, &candidate, &items, &blob) ||
	    blob.size() != payload.item_blob_size ||
	    !std::equal(blob.begin(), blob.end(), payload.item_blob.begin()))
		return false;
	*selected = candidate;
	return true;
}

bool collector_collection_detach_live(P_obj selected)
{
	if (!selected || (!OBJ_INSIDE(selected) && !OBJ_ROOM(selected)))
		return false;
	P_obj parent = OBJ_INSIDE(selected) ? selected->loc.inside : nullptr;
	const int room = OBJ_ROOM(selected) ? selected->loc.room : NOWHERE;
	while (selected->contains)
	{
		P_obj child = selected->contains;
		obj_from_obj(child);
		if (!OBJ_NOWHERE(child))
			return false;
		if (parent)
			obj_to_obj(child, parent);
		else
			obj_to_room(child, room);
		if ((parent && !OBJ_INSIDE_OBJ(child, parent)) ||
		    (!parent && !OBJ_IN_ROOM(child, room)))
			return false;
	}
	extract_obj(selected, FALSE);
	return true;
}
