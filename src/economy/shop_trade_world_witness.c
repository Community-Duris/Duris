#include "economy/shop_trade_world_witness.h"
#include "economy/shop_trade_command.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include <algorithm>
#include <array>
#include <climits>
#include <new>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

extern P_obj object_list;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_room world;
extern const int top_of_world;
extern P_index mob_index;
extern int top_of_mobt;

namespace
{
constexpr size_t world_limit = 1000000;
// Each tree capture estimate includes the player envelope. Charge it once for
// an aggregate forest, and still verify the independent whole-list codec bound.
bool add_tree_estimate(size_t &total, size_t estimate)
{
	if (estimate < sizeof(player_snapshot))
		return false;
	const size_t body = estimate - sizeof(player_snapshot);
	if (body > PLAYER_SNAPSHOT_MAX_BYTES - total)
		return false;
	total += body;
	return true;
}
enum class link_kind : uint8_t
{
	none,
	room,
	inside,
	carried,
	equipment
};
struct physical_link
{
	size_t count = 0;
	link_kind kind = link_kind::none;
	P_char body = nullptr;
	P_obj parent = nullptr;
	int room = -1;
	int slot = 0;
};
struct uid_occurrence
{
	P_obj object = nullptr;
	size_t count = 0;
};
struct census
{
	size_t visits = 0;
	std::unordered_map<P_obj, physical_link> objects;
	std::unordered_map<uint64_t, uid_occurrence> uids;
	std::unordered_set<P_char> seen_bodies;
	std::vector<P_char> bodies;
	bool charge() { return ++visits <= world_limit; }
	bool add_link(P_obj object, physical_link value)
	{
		auto found = objects.find(object);
		if (found == objects.end() || !charge())
			return false;
		value.count = found->second.count + 1;
		if (!found->second.count)
			found->second = value;
		else
			found->second.count = value.count;
		return true;
	}
	bool links(P_obj head, physical_link value)
	{
		std::unordered_set<P_obj> seen;
		for (P_obj object = head; object; object = object->next_content)
			if (!seen.insert(object).second || !add_link(object, value))
				return false;
		return true;
	}
	bool body(P_char ch)
	{
		if (!ch || !seen_bodies.insert(ch).second)
			return true;
		if (!charge() || (IS_NPC(ch) ? !ch->only.npc : !ch->only.pc))
			return false;
		bodies.push_back(ch);
		physical_link link;
		link.kind = link_kind::carried;
		link.body = ch;
		if (!links(ch->carrying, link))
			return false;
		link.kind = link_kind::equipment;
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			if (!ch->equipment[slot])
				continue; // Empty slots do not consume graph budget.
			link.slot = slot + 1;
			if (!add_link(ch->equipment[slot], link))
				return false;
		}
		return true;
	}
	bool scan()
	{
		P_obj previous = nullptr;
		for (P_obj object = object_list; object; object = object->next)
		{
			if (!charge() || object->prev != previous ||
			    !objects.emplace(object, physical_link{}).second)
				return false;
			if (object->obj_uid)
			{
				auto &occurrence = uids[object->obj_uid];
				occurrence.object = object;
				++occurrence.count;
			}
			previous = object;
		}
		for (const auto &[object, ignored] : objects)
		{
			physical_link link;
			link.kind = link_kind::inside;
			link.parent = object;
			if (!links(object->contains, link))
				return false;
		}
		if (!world || top_of_world < 0)
			return false;
		for (int room = 0; room <= top_of_world; ++room)
		{
			physical_link link;
			link.kind = link_kind::room;
			link.room = room;
			if (!charge() || !links(world[room].contents, link))
				return false;
			std::unordered_set<P_char> room_people;
			for (P_char ch = world[room].people; ch; ch = ch->next_in_room)
				if (!charge() || !room_people.insert(ch).second || !body(ch))
					return false;
		}
		std::unordered_set<P_char> chain;
		for (P_char ch = character_list; ch; ch = ch->next)
			if (!chain.insert(ch).second || !body(ch))
				return false;
		std::unordered_set<P_desc> descriptors;
		for (P_desc d = descriptor_list; d; d = d->next)
			if (!charge() || !descriptors.insert(d).second || !body(d->character) ||
			    !body(d->original))
				return false;
		for (size_t index = 0; index < bodies.size(); ++index)
		{
			P_char ch = bodies[index];
			if ((IS_NPC(ch) && !body(ch->only.npc->orig_char)) || !body(GET_PLYR(ch)))
				return false;
		}
		// Detect containment cycles even when each sibling chain alone is finite.
		std::unordered_map<P_obj, uint8_t> colors;
		for (const auto &[object, ignored] : objects)
		{
			std::vector<P_obj> path;
			P_obj current = object;
			while (current && !colors[current])
			{
				if (!charge())
					return false;
				colors[current] = 1;
				path.push_back(current);
				const auto &link = objects.at(current);
				current = link.kind == link_kind::inside ? link.parent : nullptr;
			}
			if (current && colors[current] == 1)
				return false;
			for (P_obj node : path)
				colors[node] = 2;
		}
		return true;
	}
	P_obj unique(uint64_t uid) const
	{
		auto found = uids.find(uid);
		return found != uids.end() && found->second.count == 1 ? found->second.object :
									 nullptr;
	}
	bool reciprocal(P_obj object, const physical_link &link) const
	{
		if (link.count != 1)
			return false;
		switch (link.kind)
		{
		case link_kind::room:
			return object->loc_p == LOC_ROOM && object->loc.room == link.room;
		case link_kind::inside:
			return object->loc_p == LOC_INSIDE && object->loc.inside == link.parent;
		case link_kind::carried:
			return object->loc_p == LOC_CARRIED && object->loc.carrying == link.body;
		case link_kind::equipment:
			return object->loc_p == LOC_WORN && object->loc.wearing == link.body &&
			       !object->next_content;
		default:
			return false;
		}
	}
	bool tree(P_obj object, P_obj parent, size_t depth, std::unordered_set<P_obj> &seen)
	{
		if (depth > PLAYER_SNAPSHOT_MAX_DEPTH ||
		    seen.size() >= PLAYER_SNAPSHOT_MAX_OBJECTS || !seen.insert(object).second)
			return false;
		auto found = objects.find(object);
		if (found == objects.end() ||
		    (object->obj_uid && unique(object->obj_uid) != object))
			return false;
		if (parent &&
		    (found->second.kind != link_kind::inside || found->second.parent != parent ||
		     !reciprocal(object, found->second)))
			return false;
		for (P_obj child = object->contains; child; child = child->next_content)
			if (!tree(child, object, depth + 1, seen))
				return false;
		return true;
	}
	bool forest(P_char ch)
	{
		std::unordered_set<P_obj> seen;
		auto root = [&](P_obj object, link_kind kind, int slot)
		{
			auto found = objects.find(object);
			return found != objects.end() && found->second.kind == kind &&
			       found->second.body == ch && found->second.slot == slot &&
			       reciprocal(object, found->second) && tree(object, nullptr, 1, seen);
		};
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			if (ch->equipment[slot] &&
			    !root(ch->equipment[slot], link_kind::equipment, slot + 1))
				return false;
		for (P_obj object = ch->carrying; object; object = object->next_content)
			if (!root(object, link_kind::carried, 0))
				return false;
		return true;
	}
};

bool validate_forest(std::span<const player_item_snapshot> items, bool literal, bool detached,
		     std::unordered_set<uint64_t> &identities)
{
	if (items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	size_t bytes = 0, rows = items.size();
	auto add_bytes = [&](size_t count)
	{
		if (count > PLAYER_SNAPSHOT_MAX_BYTES - bytes)
			return false;
		bytes += count;
		return true;
	};
	auto text = [&](const std::string &value)
	{ return value.size() <= PLAYER_SNAPSHOT_MAX_STRING_BYTES && add_bytes(value.size()); };
	// Bound caller-owned nested values before copying them into canonical codecs.
	for (const auto &item : items)
	{
		if (!add_bytes(sizeof(player_item_snapshot)) || !text(item.name) ||
		    !text(item.short_description) || !text(item.description) ||
		    !text(item.action_description) ||
		    item.dynamic_affects.size() > PLAYER_SNAPSHOT_MAX_ROWS - rows)
			return false;
		rows += item.dynamic_affects.size();
		if (item.extra_descriptions.size() > PLAYER_SNAPSHOT_MAX_ROWS - rows ||
		    !add_bytes(item.dynamic_affects.size() *
			       sizeof(player_item_dynamic_affect_snapshot)))
			return false;
		rows += item.extra_descriptions.size();
		for (const auto &extra : item.extra_descriptions)
		{
			if (!add_bytes(sizeof(player_item_extra_description_snapshot)) ||
			    !text(extra.keyword) || !text(extra.description) ||
			    extra.spell_ids.size() > PLAYER_SNAPSHOT_MAX_ROWS - rows)
				return false;
			rows += extra.spell_ids.size();
			if (!add_bytes(extra.spell_ids.size() * sizeof(int32_t)))
				return false;
		}
	}
	std::array<size_t, PLAYER_SNAPSHOT_MAX_DEPTH> ancestors{};
	size_t depth = 0;
	int last_equipment = 0;
	bool inventory = false;
	for (size_t index = 0; index < items.size(); ++index)
	{
		const auto &item = items[index];
		if (!item.object_uid || item.object_uid == UINT64_MAX ||
		    (item.object_uid && !identities.insert(item.object_uid).second) ||
		    item.string_mask > 15 || (literal && item.string_mask != 15))
			return false;
		if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			if (item.equipment_slot < 0 || item.equipment_slot > MAX_WEAR ||
			    (detached && item.equipment_slot))
				return false;
			if (item.equipment_slot)
			{
				if (inventory || item.equipment_slot <= last_equipment)
					return false;
				last_equipment = item.equipment_slot;
			}
			else
				inventory = true;
			depth = 1;
			ancestors[0] = index;
		}
		else
		{
			if (item.equipment_slot || item.parent_index < 0 ||
			    static_cast<size_t>(item.parent_index) >= index)
				return false;
			while (depth &&
			       ancestors[depth - 1] != static_cast<size_t>(item.parent_index))
				--depth;
			if (!depth || depth == ancestors.size())
				return false;
			ancestors[depth++] = index;
		}
	}
	std::vector<player_item_snapshot> copy(items.begin(), items.end());
	std::vector<uint8_t> encoded;
	return player_item_snapshot_list_encode(copy, &encoded) ==
		       player_snapshot_codec_result::ok &&
	       encoded.size() <= PLAYER_SNAPSHOT_MAX_BYTES;
}
bool append_tree(std::vector<player_item_snapshot> tree, int slot,
		 std::vector<player_item_snapshot> &out)
{
	if (tree.empty() || tree.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - out.size())
		return false;
	const int32_t offset = out.size();
	tree[0].equipment_slot = slot;
	for (auto &row : tree)
	{
		if (row.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
			row.parent_index += offset;
		out.push_back(std::move(row));
	}
	return true;
}
bool capture_body(P_char body, bool keeper, std::span<const uint64_t> literal_roots,
		  std::vector<player_item_snapshot> &out)
{
	if (keeper)
	{
		size_t captured_bytes = sizeof(player_snapshot);
		auto root = [&](P_obj object, int slot)
		{
			if (!object)
				return true;
			std::vector<player_item_snapshot> tree;
			size_t bytes = 0;
			if (player_item_snapshot_tree_capture_literal(object, &tree, &bytes) !=
				    player_snapshot_capture_result::ok ||
			    !add_tree_estimate(captured_bytes, bytes))
				return false;
			return append_tree(std::move(tree), slot, out);
		};
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			if (!root(body->equipment[slot], slot + 1))
				return false;
		for (P_obj object = body->carrying; object; object = object->next_content)
			if (!root(object, 0))
				return false;
		return true;
	}
	std::vector<player_item_snapshot> ordinary;
	if (player_item_snapshot_list_capture(body, true, true, true, &ordinary, nullptr) !=
	    player_snapshot_capture_result::ok)
		return false;
	std::unordered_map<uint64_t, size_t> saved;
	for (size_t index = 0; index < ordinary.size(); ++index)
		if (!ordinary[index].object_uid ||
		    !saved.emplace(ordinary[index].object_uid, index).second)
			return false;
	struct node
	{
		P_obj object;
		P_obj parent;
		int slot;
	};
	std::vector<node> ordered;
	auto visit = [&](auto &&self, P_obj object, P_obj parent, int slot, size_t depth) -> bool
	{
		if (depth > PLAYER_SNAPSHOT_MAX_DEPTH ||
		    ordered.size() >= PLAYER_SNAPSHOT_MAX_OBJECTS)
			return false;
		ordered.push_back({ object, parent, slot });
		for (P_obj child = object->contains; child; child = child->next_content)
			if (!self(self, child, object, 0, depth + 1))
				return false;
		return true;
	};
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (body->equipment[slot] &&
		    !visit(visit, body->equipment[slot], nullptr, slot + 1, 1))
			return false;
	for (P_obj object = body->carrying; object; object = object->next_content)
		if (!visit(visit, object, nullptr, 0, 1))
			return false;
	std::unordered_map<uint64_t, player_item_snapshot> literals;
	size_t literal_bytes = sizeof(player_snapshot);
	for (uint64_t uid : literal_roots)
	{
		auto found = std::find_if(ordered.begin(), ordered.end(), [uid](const auto &entry)
					  { return entry.object->obj_uid == uid; });
		if (found == ordered.end())
			return false;
		std::vector<player_item_snapshot> tree;
		size_t bytes = 0;
		if (player_item_snapshot_tree_capture_literal(found->object, &tree, &bytes) !=
			    player_snapshot_capture_result::ok ||
		    !add_tree_estimate(literal_bytes, bytes))
			return false;
		for (auto &row : tree)
			if (!literals.emplace(row.object_uid, std::move(row)).second)
				return false;
	}
	std::unordered_map<P_obj, int32_t> positions;
	for (const auto &entry : ordered)
	{
		const auto literal = literals.find(entry.object->obj_uid);
		const auto ordinary_row = saved.find(entry.object->obj_uid);
		if (literal == literals.end() && ordinary_row == saved.end())
			continue;
		player_item_snapshot row = literal != literals.end() ?
						   std::move(literal->second) :
						   ordinary[ordinary_row->second];
		row.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		if (entry.parent)
		{
			auto parent = positions.find(entry.parent);
			if (parent == positions.end())
				return false;
			row.parent_index = parent->second;
		}
		row.equipment_slot = entry.slot;
		positions.emplace(entry.object, out.size());
		out.push_back(std::move(row));
	}
	// Merge literal subtree rows, including a nested produced root, without
	// promoting the destination or its siblings or inventing a persisted parent.
	return true;
}

bool same_items(const std::vector<player_item_snapshot> &actual,
		std::span<const player_item_snapshot> expected)
{
	std::vector<player_item_snapshot> copy(expected.begin(), expected.end());
	std::vector<uint8_t> a, b;
	return player_item_snapshot_list_encode(actual, &a) == player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(copy, &b) == player_snapshot_codec_result::ok &&
	       a == b;
}
}

bool shop_trade_world_witness_observe(const shop_trade_world_expectation &expected,
				      shop_trade_world_witness *output) noexcept
{
	if (!output || !nevent_is_game_thread() || !expected.actor_pid ||
	    expected.actor_pid > INT_MAX || !expected.keeper_runtime_id ||
	    expected.shop_id > INT_MAX || expected.keeper_vnum < 0 ||
	    expected.literal_player_root_uids.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    expected.uid_locations.size() > 3 * PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    (!expected.actor_runtime_id &&
	     (!expected.player_items.empty() || !expected.literal_player_root_uids.empty())))
		return false;
	try
	{
		std::unordered_set<uint64_t> identities;
		if (!validate_forest(expected.player_items, false, false, identities) ||
		    !validate_forest(expected.keeper_items, true, false, identities) ||
		    !validate_forest(expected.detached_items, true, true, identities))
			return false;
		std::unordered_set<uint64_t> literal;
		for (uint64_t uid : expected.literal_player_root_uids)
		{
			if (!uid || uid == UINT64_MAX || !literal.insert(uid).second)
				return false;
			auto root = std::find_if(expected.player_items.begin(),
						 expected.player_items.end(), [uid](const auto &row)
						 { return row.object_uid == uid; });
			if (root == expected.player_items.end() || root->string_mask != 15)
				return false;
			const int32_t index = root - expected.player_items.begin();
			for (auto row = root + 1;
			     row != expected.player_items.end() && row->parent_index >= index;
			     ++row)
				if (row->string_mask != 15)
					return false;
		}

		census world_census;
		if (!world_census.scan())
			return false;
		shop_trade_world_witness candidate;
		for (P_char body : world_census.bodies)
		{
			if (body->runtime_id == expected.keeper_runtime_id)
			{
				if (candidate.keeper || !IS_NPC(body) || !body->only.npc ||
				    !mob_index || body->only.npc->R_num < 0 ||
				    body->only.npc->R_num > top_of_mobt ||
				    mob_index[body->only.npc->R_num].virtual_number !=
					    expected.keeper_vnum ||
				    body->only.npc->shopkeeper_shop_id < 0 ||
				    static_cast<uint32_t>(body->only.npc->shopkeeper_shop_id) !=
					    expected.shop_id)
					return false;
				candidate.keeper = body;
			}
			if (body->runtime_id == expected.actor_runtime_id &&
			    expected.actor_runtime_id &&
			    (!IS_PC(body) || !body->only.pc ||
			     static_cast<uint32_t>(GET_PID(body)) != expected.actor_pid))
				return false;
			if (IS_PC(body) && body->only.pc && GET_PID(body) > 0 &&
			    static_cast<uint32_t>(GET_PID(body)) == expected.actor_pid)
			{
				if (!expected.actor_runtime_id || candidate.actor ||
				    body->runtime_id != expected.actor_runtime_id)
					return false;
				candidate.actor = body;
			}
		}
		if (!candidate.keeper || (expected.actor_runtime_id && !candidate.actor) ||
		    !world_census.forest(candidate.keeper) ||
		    !capture_body(candidate.keeper, true, {}, candidate.keeper_items) ||
		    !same_items(candidate.keeper_items, expected.keeper_items))
			return false;
		if (candidate.actor &&
		    (!world_census.forest(candidate.actor) ||
		     !capture_body(candidate.actor, false, expected.literal_player_root_uids,
				   candidate.player_items)))
			return false;
		if (!same_items(candidate.player_items, expected.player_items))
			return false;
		std::unordered_set<P_obj> detached_seen;
		size_t detached_bytes = sizeof(player_snapshot);
		for (const auto &row : expected.detached_items)
		{
			if (row.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				continue;
			P_obj root = world_census.unique(row.object_uid);
			if (!root || world_census.objects.at(root).count ||
			    root->loc_p != LOC_NOWHERE || root->next_content ||
			    !world_census.tree(root, nullptr, 1, detached_seen))
				return false;
			std::vector<player_item_snapshot> tree;
			size_t bytes = 0;
			if (player_item_snapshot_tree_capture_literal(root, &tree, &bytes) !=
				    player_snapshot_capture_result::ok ||
			    !add_tree_estimate(detached_bytes, bytes) ||
			    !append_tree(std::move(tree), 0, candidate.detached_items))
				return false;
		}
		if (!same_items(candidate.detached_items, expected.detached_items))
			return false;
		std::unordered_set<uint64_t> requirements;
		for (const auto &request : expected.uid_locations)
		{
			if (!request.uid || request.uid == UINT64_MAX ||
			    !requirements.insert(request.uid).second ||
			    request.native_item_id < -1 || request.equipment_slot < 0 ||
			    request.equipment_slot > MAX_WEAR)
				return false;
			P_obj object = world_census.unique(request.uid);
			if (request.location == shop_trade_world_location::absent)
			{
				if (world_census.uids.count(request.uid) ||
				    identities.count(request.uid) || request.parent_uid ||
				    request.equipment_slot || request.native_item_id != -1)
					return false;
				candidate.objects.push_back(nullptr);
				continue;
			}
			if (!object || !identities.count(request.uid) ||
			    (request.native_item_id != -1 &&
			     object->db_item_id != request.native_item_id))
				return false;
			const auto &link = world_census.objects.at(object);
			bool match = false;
			switch (request.location)
			{
			case shop_trade_world_location::actor_inventory:
			case shop_trade_world_location::keeper_inventory:
				match = link.kind == link_kind::carried &&
					link.body ==
						(request.location == shop_trade_world_location::
									     actor_inventory ?
							 candidate.actor :
							 candidate.keeper) &&
					!request.parent_uid && !request.equipment_slot;
				break;
			case shop_trade_world_location::actor_equipment:
			case shop_trade_world_location::keeper_equipment:
				match = link.kind == link_kind::equipment &&
					link.body ==
						(request.location == shop_trade_world_location::
									     actor_equipment ?
							 candidate.actor :
							 candidate.keeper) &&
					!request.parent_uid && request.equipment_slot &&
					link.slot == request.equipment_slot;
				break;
			case shop_trade_world_location::inside:
				match = link.kind == link_kind::inside && link.parent &&
					link.parent->obj_uid == request.parent_uid &&
					request.parent_uid && !request.equipment_slot;
				break;
			case shop_trade_world_location::detached:
				match = !link.count && object->loc_p == LOC_NOWHERE &&
					!object->next_content && !request.parent_uid &&
					!request.equipment_slot && detached_seen.count(object);
				break;
			default:
				return false;
			}
			if (!match || (request.location != shop_trade_world_location::detached &&
				       !world_census.reciprocal(object, link)))
				return false;
			candidate.objects.push_back(object);
		}
		*output = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool shop_trade_world_expected_player_order(const shop_trade_payload &payload, P_char actor,
					    P_obj selected, P_obj destination,
					    const std::vector<player_item_snapshot> &values,
					    std::vector<player_item_snapshot> *output) noexcept
{
	if (!output || !nevent_is_game_thread() || !actor || !IS_PC(actor) || !actor->only.pc ||
	    static_cast<uint32_t>(GET_PID(actor)) != payload.player_pid || !selected ||
	    selected->obj_uid != payload.selected_item_uid || selected->R_num < 0 ||
	    (payload.action != shop_trade_action::buy_existing &&
	     payload.action != shop_trade_action::buy_produced) ||
	    values.empty() || values.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    (payload.target_parent_item_uid ?
		     (!destination || destination->obj_uid != payload.target_parent_item_uid ||
		      !OBJ_CARRIED_BY(destination, actor)) :
		     destination != nullptr))
		return false;
	try
	{
		std::map<uint64_t, size_t> indices;
		std::vector<std::vector<size_t>> children(values.size());
		std::vector<size_t> roots;
		for (size_t index = 0; index < values.size(); ++index)
		{
			const auto &row = values[index];
			if (!row.object_uid || row.object_uid == UINT64_MAX ||
			    !indices.emplace(row.object_uid, index).second ||
			    row.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    row.parent_index >= static_cast<int32_t>(index))
				return false;
			if (row.parent_index < 0)
				roots.push_back(index);
			else
			{
				if (row.equipment_slot)
					return false;
				children[static_cast<size_t>(row.parent_index)].push_back(index);
			}
		}
		const auto picked = indices.find(payload.selected_item_uid);
		if (picked == indices.end() || values[picked->second].equipment_slot ||
		    real_object(values[picked->second].vnum) != selected->R_num)
			return false;
		const size_t selected_index = picked->second;
		const int32_t parent = values[selected_index].parent_index;
		if (payload.target_parent_item_uid ?
			    (parent < 0 || values[static_cast<size_t>(parent)].object_uid !=
						   payload.target_parent_item_uid) :
			    parent != PLAYER_SNAPSHOT_NO_PARENT)
			return false;
		auto &siblings = parent < 0 ? roots : children[static_cast<size_t>(parent)];
		const auto removed = std::find(siblings.begin(), siblings.end(), selected_index);
		if (removed == siblings.end())
			return false;
		siblings.erase(removed);
		// Existing handler grouping uses actual R_num, including ephemeral
		// NORENT siblings omitted from the saved forest. Observe the complete
		// target chain, skipping this selected node on a resumed placement.
		std::vector<P_obj> physical;
		std::set<P_obj> seen;
		std::vector<size_t> observed_saved;
		for (P_obj object = destination ? destination->contains : actor->carrying; object;
		     object = object->next_content)
		{
			if (physical.size() >= PLAYER_SNAPSHOT_MAX_OBJECTS ||
			    !seen.insert(object).second ||
			    (destination ?
				     (!OBJ_INSIDE(object) || object->loc.inside != destination) :
				     !OBJ_CARRIED_BY(object, actor)))
				return false;
			if (object == selected)
				continue;
			physical.push_back(object);
			const auto saved = indices.find(object->obj_uid);
			if (saved != indices.end())
			{
				if (std::find(siblings.begin(), siblings.end(), saved->second) ==
				    siblings.end())
					return false;
				observed_saved.push_back(saved->second);
			}
		}
		const auto inventory_begin =
			parent < 0 ?
				std::find_if(siblings.begin(), siblings.end(), [&](size_t index)
					     { return !values[index].equipment_slot; }) :
				siblings.begin();
		if (observed_saved != std::vector<size_t>(inventory_begin, siblings.end()))
			return false; // The original remaining sibling order must still agree.
		const auto same_template =
			std::find_if(physical.begin(), physical.end(), [&](P_obj object)
				     { return object->R_num == selected->R_num; });
		const auto physical_anchor = same_template == physical.end() ? physical.begin() :
									       same_template;
		auto insertion = siblings.end();
		for (auto node = physical_anchor; node != physical.end(); ++node)
		{
			const auto saved = indices.find((*node)->obj_uid);
			if (saved != indices.end())
			{
				insertion =
					std::find(inventory_begin, siblings.end(), saved->second);
				if (insertion == siblings.end())
					return false;
				break;
			}
		}
		siblings.insert(insertion, selected_index);
		std::vector<player_item_snapshot> ordered;
		ordered.reserve(values.size());
		const auto append = [&](auto &&self, size_t index, int32_t parent_index,
					size_t depth) -> bool
		{
			if (depth > PLAYER_SNAPSHOT_MAX_DEPTH || ordered.size() >= values.size())
				return false;
			const int32_t placed = static_cast<int32_t>(ordered.size());
			ordered.push_back(values[index]);
			ordered.back().parent_index = parent_index;
			for (size_t child : children[index])
				if (!self(self, child, placed, depth + 1))
					return false;
			return true;
		};
		for (size_t root : roots)
			if (!append(append, root, PLAYER_SNAPSHOT_NO_PARENT, 1))
				return false;
		std::vector<uint8_t> canonical;
		if (ordered.size() != values.size() ||
		    player_item_snapshot_list_encode(ordered, &canonical) !=
			    player_snapshot_codec_result::ok ||
		    canonical.size() > PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t))
			return false;
		*output = std::move(ordered);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
