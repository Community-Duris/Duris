#ifndef DURIS_REMOVE_CURSE_CUSTODY_H
#define DURIS_REMOVE_CURSE_CUSTODY_H

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"

#include <climits>
#include <cstdint>
#include <new>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

extern P_obj object_list;
extern P_char character_list;
extern P_room world;
extern const int top_of_world;

// Preparation and native observation only. These helpers never admit a command,
// move an object, remove a curse, or authorize publication/ACK. The producer and
// shared exact custody/status proof contract remain unwired.
namespace remove_curse_custody
{
struct selected_root
{
	uint64_t item_uid = 0;
	// Physical source slot+1, independently observed from the equipment array.
	// Generic tree capture always sets its root slot to zero.
	uint16_t source_slot = 0;
	uint32_t first_item = 0;
	uint32_t item_count = 0;
};

struct prepared_selection
{
	uint32_t victim_pid = 0;
	int32_t room_index = -1;
	int32_t room_vnum = 0;
	uint64_t source_owner_revision = 0;
	// Spell traversal order: ascending equipment slots, then carrying order.
	std::vector<selected_root> roots;
	std::vector<player_item_snapshot> literal_items;
	std::vector<item_ownership_runtime_entry> source_custody;
	std::vector<uint8_t> encoded_literal;
};

struct native_root_match
{
	bool source_graph = false;
	bool selected_room_graph = false;
};

namespace detail
{
inline bool registered_victim(P_char victim)
{
	P_char slow = character_list, fast = character_list;
	while (fast && fast->next)
	{
		slow = slow->next;
		fast = fast->next->next;
		if (slow == fast)
			return false;
	}
	bool found = false;
	for (P_char current = character_list; current; current = current->next)
	{
		if (IS_NPC(current) || !current->only.pc || GET_PID(current) != GET_PID(victim))
			continue;
		if (current != victim || found)
			return false;
		found = true;
	}
	return found;
}

struct physical_audit
{
	std::unordered_set<P_obj> objects;
	std::unordered_set<uint64_t> uids;
};

inline bool audit_tree(P_obj object, P_obj parent, P_char owner, uint16_t source_slot, size_t depth,
		       physical_audit *audit)
{
	if (!object || !audit || depth > PLAYER_SNAPSHOT_MAX_DEPTH ||
	    audit->objects.size() >= PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    !audit->objects.insert(object).second || object->obj_uid == UINT64_MAX ||
	    (object->obj_uid && !audit->uids.insert(object->obj_uid).second))
		return false;
	if (parent ? !OBJ_INSIDE_OBJ(object, parent) :
		     (source_slot ? !OBJ_WORN_BY(object, owner) : !OBJ_CARRIED_BY(object, owner)))
		return false;
	for (P_obj child = object->contains; child; child = child->next_content)
		if (!audit_tree(child, object, owner, 0, depth + 1, audit))
			return false;
	return true;
}

inline bool audit_owner(P_char owner, physical_audit *audit)
{
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (owner->equipment[slot] &&
		    !audit_tree(owner->equipment[slot], nullptr, owner,
				static_cast<uint16_t>(slot + 1), 1, audit))
			return false;
	for (P_obj object = owner->carrying; object; object = object->next_content)
		if (!audit_tree(object, nullptr, owner, 0, 1, audit))
			return false;
	return true;
}

// Constant-space registry cycle detection; selection-sized lookup storage only.
// All selected UIDs must have exactly one registered live object.
inline bool registered_objects(const std::vector<player_item_snapshot> &items,
			       std::unordered_map<uint64_t, P_obj> *found)
{
	P_obj slow = object_list, fast = object_list;
	while (fast && fast->next)
	{
		slow = slow->next;
		fast = fast->next->next;
		if (slow == fast)
			return false;
	}
	for (const auto &item : items)
		if (!item.object_uid || item.object_uid == UINT64_MAX ||
		    !found->emplace(item.object_uid, nullptr).second)
			return false;
	for (P_obj object = object_list; object; object = object->next)
	{
		auto selected = found->find(object->obj_uid);
		if (selected == found->end())
			continue;
		if (selected->second)
			return false;
		selected->second = object;
	}
	for (const auto &[uid, object] : *found)
	{
		(void)uid;
		if (!object)
			return false;
	}
	return true;
}

inline bool room_roots(const prepared_selection &selection,
		       const std::unordered_map<uint64_t, P_obj> &registered,
		       std::unordered_set<P_obj> *found)
{
	std::unordered_set<uint64_t> root_uids;
	for (const auto &root : selection.roots)
		root_uids.insert(root.item_uid);
	P_obj slow = world[selection.room_index].contents, fast = slow;
	while (fast && fast->next_content)
	{
		slow = slow->next_content;
		fast = fast->next_content->next_content;
		if (slow == fast)
			return false;
	}
	for (P_obj object = world[selection.room_index].contents; object;
	     object = object->next_content)
	{
		auto selected = registered.find(object->obj_uid);
		if (selected == registered.end())
			continue;
		if (!root_uids.contains(object->obj_uid) || selected->second != object ||
		    !OBJ_ROOM(object) || object->loc.room != selection.room_index ||
		    !found->insert(object).second)
			return false;
	}
	return true;
}

inline bool audit_selected_tree(P_obj root, physical_audit *audit)
{
	if (!root || !audit || audit->objects.size() >= ITEM_TRANSFER_MAX_ITEMS ||
	    !audit->objects.insert(root).second || !root->obj_uid || root->obj_uid == UINT64_MAX ||
	    !audit->uids.insert(root->obj_uid).second)
		return false;
	for (P_obj child = root->contains; child; child = child->next_content)
		if (!audit_tree(child, root, nullptr, 0, 2, audit))
			return false;
	return audit->objects.size() <= ITEM_TRANSFER_MAX_ITEMS;
}

inline bool append_root(P_obj object, uint16_t slot, const item_owner_identity &owner,
			prepared_selection *selection)
{
	if (!object || !IS_SET(object->extra_flags, ITEM_NODROP) ||
	    selection->roots.size() >= ITEM_TRANSFER_MAX_ITEMS)
		return false;
	std::vector<player_item_snapshot> tree;
	if (player_item_snapshot_tree_capture_literal(object, &tree, nullptr) !=
		    player_snapshot_capture_result::ok ||
	    tree.empty() || tree.size() > ITEM_TRANSFER_MAX_ITEMS ||
	    selection->literal_items.size() > ITEM_TRANSFER_MAX_ITEMS - tree.size() ||
	    tree.front().object_uid != object->obj_uid ||
	    tree.front().parent_index != PLAYER_SNAPSHOT_NO_PARENT)
		return false;
	// Owned snapshot only; neither the live equipment nor flags are changed.
	tree.front().equipment_slot = static_cast<int16_t>(slot);
	const size_t offset = selection->literal_items.size();
	for (size_t index = 0; index < tree.size(); ++index)
	{
		auto &item = tree[index];
		if (!item.object_uid || item.object_uid == UINT64_MAX ||
		    (index && (item.equipment_slot || item.parent_index < 0 ||
			       item.parent_index >= static_cast<int32_t>(index))))
			return false;
		item_ownership_runtime_entry custody = {};
		const uint64_t parent_uid =
			index ? tree[static_cast<size_t>(item.parent_index)].object_uid : 0;
		if (!item_ownership_runtime_lookup(item.object_uid, &custody) ||
		    custody.item_uid != item.object_uid ||
		    custody.root_item_uid != object->obj_uid ||
		    custody.parent_item_uid != parent_uid || custody.vnum != item.vnum ||
		    custody.state != item_custody_state::active || !custody.item_revision ||
		    custody.item_revision == UINT64_MAX ||
		    !item_owner_identity_equal(custody.owner, owner))
			return false;
		selection->source_custody.push_back(custody);
	}
	selection->roots.push_back({ object->obj_uid, slot, static_cast<uint32_t>(offset),
				     static_cast<uint32_t>(tree.size()) });
	for (auto &item : tree)
	{
		if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			item.parent_index += static_cast<int32_t>(offset);
		selection->literal_items.push_back(std::move(item));
	}
	return true;
}

inline bool valid_selection(const prepared_selection &selection)
{
	if (!selection.victim_pid || selection.victim_pid > INT32_MAX || selection.room_index < 0 ||
	    selection.room_index > top_of_world ||
	    world[selection.room_index].number != selection.room_vnum || selection.room_vnum <= 0 ||
	    !selection.source_owner_revision || selection.source_owner_revision == UINT64_MAX ||
	    selection.literal_items.size() > ITEM_TRANSFER_MAX_ITEMS ||
	    selection.source_custody.size() != selection.literal_items.size() ||
	    selection.roots.size() > ITEM_TRANSFER_MAX_ITEMS)
		return false;
	size_t next = 0;
	uint16_t last_slot = 0;
	bool carrying_started = false;
	for (const auto &root : selection.roots)
	{
		if (!root.item_count || root.first_item != next ||
		    next > selection.literal_items.size() ||
		    root.item_count > selection.literal_items.size() - next ||
		    root.source_slot > MAX_WEAR ||
		    selection.literal_items[next].object_uid != root.item_uid ||
		    selection.literal_items[next].parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
		    selection.literal_items[next].equipment_slot != root.source_slot)
			return false;
		if (root.source_slot)
		{
			if (carrying_started || root.source_slot <= last_slot)
				return false;
			last_slot = root.source_slot;
		}
		else
			carrying_started = true;
		for (size_t index = next; index < next + root.item_count; ++index)
		{
			const auto &item = selection.literal_items[index];
			const auto &custody = selection.source_custody[index];
			if (index != next && (item.parent_index < static_cast<int32_t>(next) ||
					      item.parent_index >= static_cast<int32_t>(index) ||
					      item.equipment_slot))
				return false;
			const uint64_t parent_uid =
				index == next ? 0 :
						selection
							.literal_items[static_cast<size_t>(
								item.parent_index)]
							.object_uid;
			const item_owner_identity owner = { item_owner_type::player,
							    selection.victim_pid, 0 };
			if (!item.object_uid || item.object_uid == UINT64_MAX ||
			    custody.item_uid != item.object_uid ||
			    custody.root_item_uid != root.item_uid ||
			    custody.parent_item_uid != parent_uid || custody.vnum != item.vnum ||
			    custody.state != item_custody_state::active || !custody.item_revision ||
			    custody.item_revision == UINT64_MAX ||
			    !item_owner_identity_equal(custody.owner, owner))
				return false;
		}
		next += root.item_count;
	}
	return next == selection.literal_items.size();
}
} // namespace detail

// Refusal leaves the caller's output and all live state unchanged. Permission,
// ordinary SPELL_CURSE removal and special67243 effects belong to the producer.
// Runtime custody has no equipment-slot field: ledger before-slot must still be
// checked by the shared authority adapter against the explicit physical slot.
inline bool prepare(P_char victim, prepared_selection *output)
try
{
	if (!victim || !output || IS_NPC(victim) || !victim->only.pc || GET_PID(victim) <= 0 ||
	    victim->in_room < 0 || victim->in_room > top_of_world ||
	    world[victim->in_room].number <= 0 || !detail::registered_victim(victim))
		return false;
	detail::physical_audit audit;
	if (!detail::audit_owner(victim, &audit))
		return false;
	prepared_selection candidate;
	candidate.victim_pid = static_cast<uint32_t>(GET_PID(victim));
	candidate.room_index = victim->in_room;
	candidate.room_vnum = world[victim->in_room].number;
	const item_owner_identity owner = { item_owner_type::player, candidate.victim_pid, 0 };
	if (!item_ownership_runtime_owner_revision(owner, &candidate.source_owner_revision) ||
	    !candidate.source_owner_revision || candidate.source_owner_revision == UINT64_MAX)
		return false;
	for (int slot = 0; slot < MAX_WEAR; ++slot)
	{
		P_obj object = victim->equipment[slot];
		if (object && IS_SET(object->extra_flags, ITEM_NODROP) &&
		    !detail::append_root(object, static_cast<uint16_t>(slot + 1), owner,
					 &candidate))
			return false;
	}
	for (P_obj object = victim->carrying; object; object = object->next_content)
		if (IS_SET(object->extra_flags, ITEM_NODROP) &&
		    !detail::append_root(object, 0, owner, &candidate))
			return false;
	std::unordered_map<uint64_t, P_obj> registered;
	if (!detail::registered_objects(candidate.literal_items, &registered) ||
	    !detail::valid_selection(candidate))
		return false;
	for (const auto &[uid, object] : registered)
	{
		(void)uid;
		if (!audit.objects.contains(object))
			return false;
	}
	for (const auto &root : candidate.roots)
	{
		P_obj object = registered.at(root.item_uid);
		if (root.source_slot ? victim->equipment[root.source_slot - 1] != object :
				       !OBJ_CARRIED_BY(object, victim))
			return false;
	}
	if (!candidate.literal_items.empty() &&
	    (player_item_snapshot_list_encode(candidate.literal_items,
					      &candidate.encoded_literal) !=
		     player_snapshot_codec_result::ok ||
	     candidate.encoded_literal.empty() ||
	     candidate.encoded_literal.size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES))
		return false;
	*output = std::move(candidate);
	return true;
}
catch (const std::bad_alloc &)
{
	return false;
}

// Exact native observation only. Even an exact room match is not a historical
// receipt, status-effect proof, or permission to ACK. The shared owner supplies
// those operation-bound proofs. No custody revision is inferred from location.
inline bool compare_native(P_char victim, const prepared_selection &selection,
			   std::vector<native_root_match> *output)
try
{
	if (!output || !detail::valid_selection(selection) ||
	    (victim && (IS_NPC(victim) || !victim->only.pc ||
			GET_PID(victim) != static_cast<int>(selection.victim_pid) ||
			!detail::registered_victim(victim))))
		return false;
	detail::physical_audit owner_audit;
	if (victim && !detail::audit_owner(victim, &owner_audit))
		return false;
	std::unordered_map<uint64_t, P_obj> registered;
	if (!detail::registered_objects(selection.literal_items, &registered))
		return false;
	std::unordered_set<P_obj> room_objects;
	if (!detail::room_roots(selection, registered, &room_objects))
		return false;
	std::vector<uint8_t> sealed_literal;
	if (!selection.literal_items.empty() &&
	    (player_item_snapshot_list_encode(selection.literal_items, &sealed_literal) !=
		     player_snapshot_codec_result::ok ||
	     sealed_literal.empty() || sealed_literal.size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES))
		return false;
	if (sealed_literal != selection.encoded_literal)
		return false;
	std::vector<native_root_match> candidate;
	candidate.reserve(selection.roots.size());
	detail::physical_audit selected_audit;
	for (const auto &root : selection.roots)
	{
		P_obj object = registered.at(root.item_uid);
		const bool source = victim && owner_audit.objects.contains(object) &&
				    (root.source_slot ?
					     (OBJ_WORN_BY(object, victim) &&
					      victim->equipment[root.source_slot - 1] == object) :
					     OBJ_CARRIED_BY(object, victim));
		const bool room = room_objects.contains(object) && OBJ_ROOM(object) &&
				  object->loc.room == selection.room_index;
		if ((!source && !room) || !detail::audit_selected_tree(object, &selected_audit))
			return false;
		std::vector<player_item_snapshot> actual;
		if (player_item_snapshot_tree_capture_literal(object, &actual, nullptr) !=
			    player_snapshot_capture_result::ok ||
		    actual.size() != root.item_count)
			return false;
		actual.front().equipment_slot = source ? static_cast<int16_t>(root.source_slot) : 0;
		std::vector<player_item_snapshot> expected(
			selection.literal_items.begin() + root.first_item,
			selection.literal_items.begin() + root.first_item + root.item_count);
		for (auto &item : expected)
			if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				item.parent_index -= static_cast<int32_t>(root.first_item);
		if (room)
			expected.front().equipment_slot = 0;
		std::vector<uint8_t> actual_bytes, expected_bytes;
		if (player_item_snapshot_list_encode(actual, &actual_bytes) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode(expected, &expected_bytes) !=
			    player_snapshot_codec_result::ok ||
		    actual_bytes != expected_bytes)
			return false;
		candidate.push_back({ source, room });
	}
	for (const auto &[uid, object] : registered)
	{
		(void)uid;
		if (!selected_audit.objects.contains(object))
			return false;
	}
	*output = std::move(candidate);
	return true;
}
catch (const std::bad_alloc &)
{
	return false;
}
} // namespace remove_curse_custody

#endif
