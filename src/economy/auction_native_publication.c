#include "economy/auction_native_publication.h"
#include "world/db.h"
#include "economy/auction_player_forest_detail.h"
#include "economy/auction_native_command_context.h"
#include "economy/auction_repository.h"
#include "economy/economic_gameplay_authority.h"
#include "player/player_save_pipeline.h"
#ifndef __NO_MYSQL__
#include "player/player_sql_transaction_cleanup.h"
#endif
#include <algorithm>
#include <set>
#include <utility>

bool auction_original_item_stage::prepare(const object_template &prototype,
					  const player_item_snapshot &literal,
					  auction_original_item_stage &out) noexcept
{
	// Preserve the original allocator's strong output and complete supported
	// metadata, spellbook, dynamic-affect, strings and trap reload policy.
	return shop_trade_original_item_stage::prepare(prototype, literal, out.original_);
}
bool auction_original_item_stage::reload_step(P_obj object, const object_template &prototype,
					      unsigned int step,
					      shop_trade_original_reload_effect &effect) noexcept
{
	return shop_trade_original_item_stage::reload_step(object, prototype, step, effect);
}
bool auction_original_item_stage::proclib_probe(P_obj object, const object_template &prototype,
						size_t description,
						shop_trade_original_reload_effect &effect) noexcept
{
	return shop_trade_original_item_stage::proclib_probe(object, prototype, description,
							     effect);
}
P_obj auction_original_item_stage::get() const noexcept
{
	return original_.object_;
}
P_obj auction_original_item_stage::consume() noexcept
{
	P_obj result = std::exchange(original_.object_, nullptr);
	original_.pool_ = nullptr;
	original_.affect_pool_ = nullptr;
	return result;
}

namespace
{
using auction_player_forest_detail::selected_ranges;
}

#include "core/prototypes.h"
#include "core/utils.h"
#include "player/player_snapshot_capture.h"
#include "world/world_singletons.h"
#include <climits>
#include <map>
#include <unordered_map>
#include <unordered_set>
extern P_obj object_list;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_room world;
extern const int top_of_world;
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
	bool forest(P_char ch, size_t *count = nullptr)
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
		if (count)
			*count = seen.size();
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

bool auction_native_world_observe(uint32_t pid, uint64_t runtime,
				  std::span<const player_item_snapshot> expected,
				  std::span<const auction_native_world_uid> selected,
				  auction_native_world_observation *out) noexcept
{
	try
	{
		if (!out || !nevent_is_game_thread() || pid > INT_MAX || (!pid && runtime) ||
		    (!runtime && !expected.empty()) || selected.size() > AUCTION_COMMAND_MAX_ITEMS)
			return false;
		std::unordered_set<uint64_t> identities;
		if (!validate_forest(expected, false, false, identities))
			return false;
		std::set<uint64_t> required;
		for (const auto &uid : selected)
			if (!uid.uid || uid.uid == UINT64_MAX || !required.insert(uid.uid).second ||
			    static_cast<uint8_t>(uid.location) > 2)
				return false;
		census observed;
		if (!observed.scan())
			return false;
		auction_native_world_observation value;
		for (P_char body : observed.bodies)
		{
			if (body->runtime_id == runtime && runtime &&
			    (IS_NPC(body) || GET_PID(body) <= 0 ||
			     static_cast<uint32_t>(GET_PID(body)) != pid))
				return false;
			if (!IS_NPC(body) && GET_PID(body) > 0 &&
			    static_cast<uint32_t>(GET_PID(body)) == pid)
			{
				if (!runtime || value.actor || body->runtime_id != runtime)
					return false;
				value.actor = body;
			}
		}
		if (runtime && !value.actor)
			return false;
		std::vector<uint64_t> literal_roots;
		for (const auto &uid : selected)
			if (uid.location == auction_native_world_location::carried)
			{
				if (!std::any_of(expected.begin(), expected.end(),
						 [&](const auto &row)
						 {
							 return row.object_uid == uid.uid &&
								row.parent_index ==
									PLAYER_SNAPSHOT_NO_PARENT &&
								row.string_mask == 15;
						 }))
					return false;
				literal_roots.push_back(uid.uid);
			}
		if (value.actor &&
		    (!observed.forest(value.actor) ||
		     !capture_body(value.actor, false, literal_roots, value.player_items) ||
		     !same_items(value.player_items, expected)))
			return false;
		value.selected.reserve(selected.size());
		value.selected_trees.reserve(selected.size());
		size_t selected_bytes = sizeof(player_snapshot), selected_rows = 0;
		for (const auto &uid : selected)
		{
			auto occurrence = observed.uids.find(uid.uid);
			if (uid.location == auction_native_world_location::absent)
			{
				if (occurrence != observed.uids.end())
					return false;
				value.selected.push_back(nullptr);
				value.selected_trees.emplace_back();
				continue;
			}
			if (occurrence == observed.uids.end() || occurrence->second.count != 1)
				return false;
			P_obj object = occurrence->second.object;
			auto link = observed.objects.find(object);
			if (link == observed.objects.end())
				return false;
			if (uid.location == auction_native_world_location::carried)
			{
				if (!value.actor || !observed.reciprocal(object, link->second) ||
				    link->second.kind != link_kind::carried ||
				    link->second.body != value.actor || link->second.count != 1)
					return false;
			}
			else if (link->second.kind != link_kind::none || link->second.count ||
				 !OBJ_NOWHERE(object) || object->next_content)
				return false;
			std::unordered_set<P_obj> subtree;
			std::vector<player_item_snapshot> tree;
			size_t bytes = 0;
			if (!observed.tree(object, nullptr, 1, subtree) ||
			    player_item_snapshot_tree_capture_literal(object, &tree, &bytes) !=
				    player_snapshot_capture_result::ok ||
			    tree.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - selected_rows ||
			    !add_tree_estimate(selected_bytes, bytes))
				return false;
			selected_rows += tree.size();
			value.selected.push_back(object);
			value.selected_trees.push_back(std::move(tree));
		}
		*out = std::move(value);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

class auction_preparation_owner final
{
	friend class auction_native_publication_owner;
	// Values-only preparation under the original player hold. Outputs remain
	// unchanged until the full supported command/context is frozen. This method
	// neither admits, materializes nor releases a publication reservation.
	static bool freeze(P_char actor, const player_auction_checkpoint_token &token,
			   const critical_command &original, critical_command *command_out,
			   auction_recovery_context *context_out) noexcept
	{
#ifdef __NO_MYSQL__
		(void)actor;
		(void)token;
		(void)original;
		(void)command_out;
		(void)context_out;
		return false;
#else
		try
		{
			auction_command_payload payload{};
			if (!command_out || !context_out || !nevent_is_game_thread() ||
			    original.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
			    original.publication_required || original.accepted_at_usec ||
			    !auction_command_decode_payload(original, &payload))
				return false;
			critical_command command = original;
			auction_recovery_context context;
			context.actor_pid = payload.actor_pid;
			if (payload.actor_pid)
			{
				if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0 ||
				    static_cast<uint32_t>(GET_PID(actor)) != payload.actor_pid ||
				    actor->runtime_id != token.actor_runtime_id)
					return false;
				player_auction_checkpoint_stage stage{};
				if (!player_save_auction_checkpoint_owner::observe_held(
					    token, actor, original.operation_id, &stage,
					    &context.player_before))
					return false;
				context.before_present = true;
				context.before_save_revision = stage.save_revision;
				context.original_level = stage.level;
			}
			else if (actor || (payload.action != auction_action::finalize &&
					   payload.action != auction_action::remove))
				return false;
			std::vector<auction_native_world_uid> requirements;
			for (size_t i = 0; i < payload.item_count; ++i)
				requirements.push_back(
					{ payload.items[i].item_uid,
					  payload.action == auction_action::list ?
						  auction_native_world_location::carried :
						  auction_native_world_location::absent });
			auction_native_world_observation observed;
			if (!auction_native_world_observe(
				    payload.actor_pid, actor ? actor->runtime_id : 0,
				    context.player_before, requirements, &observed))
				return false;
			if (payload.action == auction_action::list)
			{
				for (size_t i = 0; i < payload.item_count; ++i)
				{
					std::vector<player_item_snapshot> literal;
					if (player_item_snapshot_tree_capture_literal(
						    observed.selected[i], &literal, nullptr) !=
						    player_snapshot_capture_result::ok ||
					    literal.empty() || (!i && literal.size() != 1) ||
					    literal[0].object_uid != payload.items[i].item_uid ||
					    literal[0].vnum != payload.items[i].vnum)
						return false;
					const int32_t offset = static_cast<int32_t>(
						context.selected_literals.size());
					for (auto &row : literal)
					{
						if (row.parent_index >= 0)
							row.parent_index += offset;
						context.selected_literals.push_back(std::move(row));
					}
				}
			}
			if (payload.action == auction_action::claim_item)
			{
				MYSQL *connection = sql_pool_acquire();
				player_sql_pool_lease lease(connection);
				if (!connection || player_sql_idle_error(connection))
					return false;
				player_sql_cleanup cleanup;
				player_sql_transaction_cleanup transaction(connection, cleanup);
				transaction.starting();
				bool proven = false;
				try
				{
					if (mysql_real_query(connection, "START TRANSACTION", 17))
						throw EIO;
					const std::string sql =
						"SELECT HEX(listing_operation_id) FROM auctions WHERE id=" +
						std::to_string(payload.auction_id);
					if (mysql_real_query(connection, sql.data(), sql.size()))
						throw EIO;
					MYSQL_RES *rows = mysql_store_result(connection);
					MYSQL_ROW row = rows ? mysql_fetch_row(rows) : nullptr;
					critical_operation_id listing{};
					const bool found =
						rows && mysql_num_fields(rows) == 1 &&
						mysql_num_rows(rows) == 1 && row && row[0] &&
						critical_operation_id_from_hex(row[0], &listing);
					if (rows)
						mysql_free_result(rows);
					if (!found || critical_operation_id_is_zero(listing))
						throw EILSEQ;
					const auto code =
						auction_repository_read_original_native_selected(
							connection, original, listing,
							&context.selected_literals);
					if (code || !auction_native_selected_forest_valid(
							    payload, context.selected_literals))
						throw EILSEQ;
					proven = transaction.same_session() &&
						 (connection->server_status &
						  SERVER_STATUS_IN_TRANS);
				}
				catch (...)
				{
					proven = false;
				}
				transaction.finish();
				if (!proven || !cleanup.rollback_confirmed ||
				    cleanup.cleanup_error ||
				    cleanup.disposition !=
					    player_sql_cleanup_disposition::idle_verified)
					return false;
				lease.reuse(cleanup);
			}
			std::vector<player_item_snapshot> after;
			if (!auction_native_expected_player_forest(
				    payload, context.player_before, context.selected_literals,
				    false, context.original_level, &after) ||
			    auction_native_command_bind(&command, context.player_before, after,
							context.selected_literals,
							context.original_level,
							context.before_save_revision) !=
				    economic_accounting_error::ok ||
			    economic_gameplay_authority::prepare_auction(&command) !=
				    economic_accounting_error::ok)
				return false;
			context.reload.resize(context.selected_literals.size());
			context.proclib.resize(context.selected_literals.size());
			for (size_t i = 0; i < context.selected_literals.size(); ++i)
				context.proclib[i].resize(
					context.selected_literals[i].extra_descriptions.size());
			// The domain owner assigns the original acceptance timestamp and bit
			// only after this strong preparation, then encodes the same NAR1 body.
			*command_out = std::move(command);
			*context_out = std::move(context);
			return true;
		}
		catch (...)
		{
			return false;
		}
#endif
	}
};

#include "economy/currency_transaction.h"
#include "core/utility.h"
#include "persistence/critical_command_coordinator.h"
#include "item/item_ownership_runtime.h"
#include "persistence/economic_sql_auction_retained.h"
#include "persistence/persistence_checkpoint.h"
#include <tuple>
#include "persistence/shop_item_runtime_payload.h"
#include "world/object_template.h"
#include "world/handler.h"
#include <chrono>
#include <charconv>
#include <cstring>
#include <memory>
#include <optional>
#include <strings.h>
extern P_index obj_index;
extern int top_of_objt;

namespace
{
struct auction_effect_key
{
	uint8_t kind = 0;
	size_t node = 0, step = 0;
	bool operator==(const auction_effect_key &) const = default;
};
struct auction_native_pending
{
	critical_command command;
	auction_command_payload payload{};
	auction_completion_fn notification = nullptr;
	player_auction_checkpoint_token token{};
	uint64_t runtime = 0;
	bool held = false, submitted = false, restored = false, ready = false;
	bool running = false, blocked = false, cancelled = false, context_dirty = false,
	     continuation_released = false;
	size_t retained_bytes = 0;
	critical_completion completion{};
	critical_native_recovery_envelope envelope;
	auction_recovery_context context;
	// Only an actual in-process callback return can finish this original started
	// effect. A replayed state1 has no such permit and remains unresolved.
	std::optional<auction_effect_key> pending_start;
	std::vector<auction_original_item_stage> staged;
	std::vector<shop_trade_original_procedure_binding_stage> bindings;
	bool staging_ready = false;
};
std::map<std::string, std::unique_ptr<auction_native_pending>> auction_native_pending_operations;
size_t auction_native_retained_bytes = 0;
constexpr size_t auction_native_total_bytes = CRITICAL_COORDINATOR_MAX_BYTES;
std::string auction_native_key(const critical_operation_id &id)
{
	return std::string(reinterpret_cast<const char *>(id.bytes.data()), id.bytes.size());
}
bool auction_native_same_completion(const critical_completion &a, const critical_completion &b)
{
	return a.operation_id.bytes == b.operation_id.bytes && a.error_code == b.error_code &&
	       a.failure_stage == b.failure_stage && a.durable_revision == b.durable_revision &&
	       a.result_size == b.result_size && a.result_payload == b.result_payload &&
	       a.disposition == b.disposition &&
	       (a.outcome == b.outcome || ((a.outcome == critical_apply_outcome::applied ||
					    a.outcome == critical_apply_outcome::already_applied) &&
					   (b.outcome == critical_apply_outcome::applied ||
					    b.outcome == critical_apply_outcome::already_applied)));
}
struct auction_current_cut
{
	currency_vector wallet{}, bank{};
	uint64_t wallet_revision = 0, bank_revision = 0, player_owner_revision = 0,
		 auction_owner_revision = 0;
	std::vector<item_ownership_runtime_entry> custody;
};
#ifndef __NO_MYSQL__
bool auction_sql_rows(MYSQL *connection, const std::string &sql, size_t fields,
		      std::vector<std::vector<std::string>> *out)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		return false;
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	if (!result || mysql_num_fields(result.get()) != fields ||
	    mysql_num_rows(result.get()) > PLAYER_SNAPSHOT_MAX_OBJECTS * 2 + 1)
		return false;
	std::vector<std::vector<std::string>> values;
	for (MYSQL_ROW row = mysql_fetch_row(result.get()); row;
	     row = mysql_fetch_row(result.get()))
	{
		const auto *lengths = mysql_fetch_lengths(result.get());
		if (!lengths)
			return false;
		std::vector<std::string> record;
		record.reserve(fields);
		for (size_t i = 0; i < fields; ++i)
		{
			if (!row[i])
				return false;
			record.emplace_back(row[i], lengths[i]);
		}
		values.push_back(std::move(record));
	}
	if (mysql_errno(connection))
		return false;
	*out = std::move(values);
	return true;
}
template <class T> bool auction_sql_number(const std::string &text, T *out)
{
	T value{};
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
	if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
		return false;
	*out = value;
	return true;
}
std::string auction_sql_operation(const critical_operation_id &id)
{
	char text[33]{};
	if (!critical_operation_id_to_hex(id, text, sizeof(text)))
		return {};
	return "UNHEX('" + std::string(text) + "')";
}
#endif
}

class auction_native_publication_owner final
{
	static P_char actor(const auction_native_pending &entry) noexcept
	{
		P_char found = nullptr;
		size_t count = 0;
		for (P_char ch = character_list; ch; ch = ch->next)
		{
			if (++count > world_limit)
				return nullptr;
			if (!IS_NPC(ch) && GET_PID(ch) > 0 &&
			    static_cast<uint32_t>(GET_PID(ch)) == entry.payload.actor_pid)
			{
				if (found || (entry.runtime && entry.runtime != ch->runtime_id))
					return nullptr;
				found = ch;
			}
		}
		return found;
	}
	static auction_recovery_effect &effect(auction_recovery_context &context,
					       auction_effect_key key)
	{
		if (key.kind == 0)
			return context.roots.at(key.node).at(key.step);
		if (key.kind == 1)
			return context.reload.at(key.node).at(key.step);
		if (key.kind == 2)
			return context.proclib.at(key.node).at(key.step);
		if (key.kind == 5)
			return context.ownership;
		return key.kind == 3 ? context.balances : context.notice;
	}
	static bool retain_size(auction_native_pending &entry, size_t bytes) noexcept
	{
		if (bytes > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
		    auction_native_retained_bytes - entry.retained_bytes >
			    auction_native_total_bytes - bytes)
			return false;
		auction_native_retained_bytes =
			auction_native_retained_bytes - entry.retained_bytes + bytes;
		entry.retained_bytes = bytes;
		return true;
	}
	static bool synchronize(auction_native_pending &entry) noexcept
	{
		try
		{
			if (!entry.context_dirty)
				return true;
			if (entry.envelope.revision == UINT64_MAX)
				return false;
			critical_native_recovery_envelope next = entry.envelope;
			++next.revision;
			if (auction_recovery_context_encode(entry.command, entry.context,
							    &next.attachment) !=
				    player_snapshot_codec_result::ok ||
			    !retain_size(entry, next.attachment.size()))
				return false;
			const bool continuation = entry.continuation_released;
			const bool stored =
				continuation ?
					critical_native_auction_continuation_owner::
						checkpoint_context(entry.envelope, next) :
					(entry.payload.actor_pid ?
						 player_save_auction_publication_owner::
							 checkpoint_recovery_context(entry.envelope,
										     next) :
						 critical_native_auction_background_publication_owner::
							 checkpoint_context(entry.envelope, next));
			if (!stored)
				return false;
			entry.envelope = std::move(next);
			entry.context_dirty = false;
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
	static bool refresh(auction_native_pending &entry) noexcept
	{
		try
		{
			if (!synchronize(entry))
				return false;
			critical_native_recovery_envelope original;
			const bool released =
				critical_native_auction_continuation_owner::copy_context(
					entry.command, &original);
			bool copied = released;
			if (!copied)
				copied =
					entry.payload.actor_pid ?
						player_save_auction_publication_owner::
							copy_recovery_context(entry.command,
									      &original) :
						critical_native_auction_background_publication_owner::
							copy_context(entry.command, &original);
			if (!copied || !critical_command_equal(entry.command, original.command) ||
			    auction_recovery_context_decode(original.command, original.attachment,
							    &entry.context) !=
				    player_snapshot_codec_result::ok ||
			    !retain_size(entry, original.attachment.size()))
				return false;
			entry.envelope = std::move(original);
			entry.continuation_released = released;
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
	static bool sql_cut(auction_native_pending &, bool, auction_current_cut &) noexcept;
	static bool working_world(auction_native_pending &, auction_native_world_observation &,
				  std::vector<size_t> &) noexcept;
	static bool after_world(auction_native_pending &, const auction_current_cut &,
				auction_native_world_observation &) noexcept;
	static bool runtime_matches(auction_native_pending &, const auction_current_cut &,
				    const auction_native_world_observation &) noexcept;
#ifndef __NO_MYSQL__
	static bool stage_claim(auction_native_pending &) noexcept;
#endif
	static bool physical(auction_native_pending &) noexcept;
	static bool guarded(const critical_command &command, const critical_completion &receipt,
			    void *opaque) noexcept
	{
		if (!opaque)
			return false;
		auto &entry = *static_cast<auction_native_pending *>(opaque);
		return critical_command_equal(command, entry.command) &&
		       auction_native_same_completion(receipt, entry.completion) && physical(entry);
	}
	static bool refusal(const critical_command &command, const critical_completion &receipt,
			    void *opaque) noexcept
	{
		if (!opaque || !critical_completion_disposition_valid(receipt) ||
		    receipt.disposition != critical_completion_disposition::never_admitted)
			return false;
		auto &entry = *static_cast<auction_native_pending *>(opaque);
		auction_current_cut cut;
		auction_native_world_observation world;
		std::vector<size_t> roots;
		return critical_command_equal(command, entry.command) &&
		       auction_native_same_completion(receipt, entry.completion) &&
		       sql_cut(entry, true, cut) && working_world(entry, world, roots) &&
		       runtime_matches(entry, cut, world);
	}
	template <class F> static bool run_effect(auction_native_pending &entry,
						  auction_effect_key key, F &&callback) noexcept
	{
		try
		{
			if (!synchronize(entry))
				return false;
			auto &progress = effect(entry.context, key);
			if (progress.state == 2)
				return true;
			if (progress.state == 1 &&
			    (!entry.pending_start || *entry.pending_start != key))
				return false;
			if (!progress.state)
			{
				progress.state = 1;
				entry.pending_start = key;
				entry.context_dirty = true;
				if (!synchronize(entry))
					return false;
			}
			// Consume the local invocation permit before crossing any native callback.
			// A thrown/unreturned/false callback is never invoked again.
			entry.pending_start.reset();
			if (!callback(progress))
				return false;
			progress.state = 2;
			entry.context_dirty = true;
			return synchronize(entry);
		}
		catch (...)
		{
			return false;
		}
	}
	static bool prepare(auction_native_pending &) noexcept;
	static bool notify(auction_native_pending &) noexcept;
	static bool advance(auction_native_pending &) noexcept;

    public:
	static bool current_storage_bytes(size_t *) noexcept;
	static size_t current_storage_observer_frame_bytes() noexcept;
	static bool submit(P_char, const auction_command_payload &, auction_completion_fn,
			   critical_source_site, critical_deadline_class) noexcept;
	static void completions(const critical_completion *, size_t) noexcept;
	static void pulse() noexcept;
	static void ready(P_char) noexcept;
	static bool busy(P_char) noexcept;
	static bool restore(const critical_native_recovery_envelope &) noexcept;
	static bool replayed(const critical_command &) noexcept;
	static bool replay_entry_storage_bytes(const auction_native_pending &, size_t *) noexcept;
};

bool auction_native_publication_owner::sql_cut(auction_native_pending &entry, bool never,
					       auction_current_cut &out) noexcept
{
#ifdef __NO_MYSQL__
	(void)entry;
	(void)never;
	(void)out;
	return false;
#else
	try
	{
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		auction_current_cut observed;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17))
				throw EIO;
			const auto op = auction_sql_operation(entry.command.operation_id);
			if (op.empty())
				throw EILSEQ;
			std::vector<std::vector<std::string>> rows;
			if (never)
			{
				if (!critical_completion_disposition_valid(entry.completion) ||
				    entry.completion.disposition !=
					    critical_completion_disposition::never_admitted ||
				    !auction_recovery_initial_valid(entry.envelope))
					throw EILSEQ;
				// Genuine delivered coordinator refusal is necessary but not sufficient:
				// independently prove that no execution/source/native/outbox row exists.
				for (const auto *table :
				     { "critical_operation_inbox", "critical_outbox",
				       "economic_accounting_intent",
				       "economic_accounting_operation", "auction_ledger",
				       "item_ownership_ledger" })
				{
					if (!auction_sql_rows(connection,
							      "SELECT COUNT(*) FROM " +
								      std::string(table) +
								      " WHERE operation_id=" + op +
								      " FOR UPDATE",
							      1, &rows) ||
					    rows.size() != 1 || rows[0][0] != "0")
						throw EILSEQ;
				}
				for (const auto &[table, predicate] :
				     std::array<std::pair<const char *, std::string>, 2>{
					     std::pair{ "economic_pending_claim_source",
							"source_operation_id=" + op +
								" OR claim_operation_id=" + op },
					     std::pair{ "economic_pending_claim_consumption",
							"spending_operation_id=" + op } })
					if (!auction_sql_rows(
						    connection,
						    "SELECT COUNT(*) FROM " + std::string(table) +
							    " WHERE " + predicate + " FOR UPDATE",
						    1, &rows) ||
					    rows.size() != 1 || rows[0][0] != "0")
						throw EILSEQ;
			}
			else
			{
				if (!entry.ready ||
				    entry.completion.disposition !=
					    critical_completion_disposition::execution ||
				    economic_sql_auction_verify_retained(
					    connection, entry.command, entry.completion.error_code,
					    entry.completion.failure_stage,
					    entry.completion.durable_revision,
					    std::span<const uint8_t>(
						    entry.completion.result_payload.data(),
						    entry.completion.result_size)))
					throw EILSEQ;
			}
			auction_command_result receipt{};
			if (!never &&
			    !auction_command_decode_result(entry.completion.result_payload.data(),
							   entry.completion.result_size, &receipt))
				throw EILSEQ;
			const bool applied = !never && !entry.completion.error_code;
			std::vector<player_item_snapshot> expected;
			if (!auction_native_expected_player_forest(
				    entry.payload, entry.context.player_before,
				    entry.context.selected_literals, !applied,
				    entry.context.original_level, &expected))
				throw EILSEQ;
			if (entry.payload.actor_pid)
			{
				if (!auction_sql_rows(
					    connection,
					    "SELECT level,save_revision,account_name,racewar,copper,silver,gold,platinum,wallet_revision FROM player_data WHERE pid=" +
						    std::to_string(entry.payload.actor_pid) +
						    " FOR UPDATE",
					    9, &rows) ||
				    rows.size() != 1)
					throw EILSEQ;
				uint64_t level = 0, save = 0, race = 0;
				if (!auction_sql_number(rows[0][0], &level) ||
				    !auction_sql_number(rows[0][1], &save) ||
				    !auction_sql_number(rows[0][3], &race) ||
				    level != entry.context.original_level ||
				    save != entry.context.before_save_revision ||
				    race != entry.payload.racewar ||
				    strcasecmp(rows[0][2].c_str(),
					       entry.payload.account_name.data()))
					throw ESTALE;
				for (size_t i = 0; i < 4; ++i)
					if (!auction_sql_number(rows[0][4 + i],
								&observed.wallet.amount[i]) ||
					    observed.wallet.amount[i] < 0)
						throw EILSEQ;
				if (!auction_sql_number(rows[0][8], &observed.wallet_revision))
					throw EILSEQ;
				std::string account(entry.payload.account_name.data());
				std::vector<char> escaped(account.size() * 2 + 1);
				auto length = mysql_real_escape_string(
					connection, escaped.data(), account.data(), account.size());
				if (!auction_sql_rows(
					    connection,
					    "SELECT bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision FROM account_banks WHERE account_name='" +
						    std::string(escaped.data(), length) +
						    "' AND racewar=" +
						    std::to_string(entry.payload.racewar) +
						    " FOR UPDATE",
					    5, &rows) ||
				    rows.size() != 1)
					throw EILSEQ;
				for (size_t i = 0; i < 4; ++i)
					if (!auction_sql_number(rows[0][i],
								&observed.bank.amount[i]) ||
					    observed.bank.amount[i] < 0)
						throw EILSEQ;
				if (!auction_sql_number(rows[0][4], &observed.bank_revision) ||
				    (never && (observed.wallet_revision !=
						       entry.payload.expected_wallet_revision ||
					       observed.bank_revision !=
						       entry.payload.expected_bank_revision)) ||
				    (!never &&
				     (observed.wallet_revision < receipt.wallet_revision ||
				      observed.bank_revision < receipt.bank_revision)))
					throw ESTALE;
			}
			const uint64_t auction = applied ? receipt.auction_id :
							   entry.payload.auction_id;
			if (auction)
			{
				if (!auction_sql_rows(
					    connection,
					    "SELECT seller_pid,winning_bidder_pid,status+0,auction_revision FROM auctions WHERE id=" +
						    std::to_string(auction) + " FOR UPDATE",
					    4, &rows) ||
				    rows.size() != 1)
					throw EILSEQ;
				uint64_t seller = 0, winner = 0, status = 0, revision = 0;
				if (!auction_sql_number(rows[0][0], &seller) ||
				    !auction_sql_number(rows[0][1], &winner) ||
				    !auction_sql_number(rows[0][2], &status) ||
				    !auction_sql_number(rows[0][3], &revision))
					throw EILSEQ;
				if (applied &&
				    (revision != receipt.auction_revision ||
				     seller != receipt.seller_pid || winner != receipt.winner_pid ||
				     status != receipt.status))
					throw ESTALE;
				if (never)
				{
					auto expected = std::find_if(
						entry.command.expected_revisions.begin(),
						entry.command.expected_revisions.end(),
						[&](const auto &value) {
							return value.key.type ==
								       critical_entity_type::auction &&
							       value.key.id == auction;
						});
					if (expected == entry.command.expected_revisions.end() ||
					    expected->revision != revision)
						throw ESTALE;
				}
			}
			if (never && entry.payload.action == auction_action::claim_item)
			{
				if (!auction_sql_rows(
					    connection,
					    "SELECT HEX(listing_operation_id) FROM auctions WHERE id=" +
						    std::to_string(auction) + " FOR UPDATE",
					    1, &rows) ||
				    rows.size() != 1)
					throw EILSEQ;
				critical_operation_id listing{};
				std::vector<player_item_snapshot> selected_source;
				if (!critical_operation_id_from_hex(rows[0][0].c_str(), &listing) ||
				    auction_repository_read_original_native_selected(
					    connection, entry.command, listing, &selected_source) ||
				    !same_items(selected_source, entry.context.selected_literals))
					throw EILSEQ;
			}

			for (const auto &[type, id, destination] :
			     { std::tuple{ item_owner_type::player,
					   uint64_t(entry.payload.actor_pid),
					   &observed.player_owner_revision },
			       std::tuple{ item_owner_type::auction, auction,
					   &observed.auction_owner_revision } })
			{
				if (!id)
					continue;
				if (!auction_sql_rows(
					    connection,
					    "SELECT revision FROM item_owner_revision WHERE owner_type=" +
						    std::to_string(static_cast<unsigned>(type)) +
						    " AND owner_id=" + std::to_string(id) +
						    " AND owner_context_id=0 FOR UPDATE",
					    1, &rows) ||
				    rows.size() != 1 ||
				    !auction_sql_number(rows[0][0], destination))
					throw EILSEQ;
			}
			if (applied &&
			    (observed.player_owner_revision < receipt.player_owner_revision ||
			     observed.auction_owner_revision < receipt.auction_owner_revision))
				throw ESTALE;
			std::string selected, selected_roots;
			for (const auto &item : entry.context.selected_literals)
				if (item.parent_index < 0)
				{
					if (!selected_roots.empty())
						selected_roots += ',';
					selected_roots += std::to_string(item.object_uid);
				}
			for (const auto &item : entry.context.selected_literals)
			{
				if (!selected.empty())
					selected += ',';
				selected += std::to_string(item.object_uid);
			}
			const std::string where =
				"(owner_type=1 AND owner_id=" +
				std::to_string(entry.payload.actor_pid) +
				" AND owner_context_id=0 AND state=1 AND coin_payload IS NULL)" +
				(selected.empty() ?
					 "" :
					 " OR item_uid IN(" + selected +
						 ") OR (state=1 AND (root_item_uid IN(" +
						 selected_roots + ") OR parent_item_uid IN(" +
						 selected + ")))");
			if (!auction_sql_rows(connection,
					      "SELECT item_uid FROM item_current_owner WHERE " +
						      where + " ORDER BY item_uid FOR UPDATE",
					      1, &rows))
				throw EILSEQ;
			if (entry.context.selected_literals.empty())
			{
				std::set<uint64_t> source_uids;
				for (const auto &key : entry.command.keys)
					if (key.type == critical_entity_type::item)
						source_uids.insert(key.id);
				if (!source_uids.empty())
				{
					if (!auction ||
					    !auction_sql_rows(
						    connection,
						    "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),item_revision,vnum FROM item_current_owner WHERE owner_type=" +
							    std::to_string(static_cast<unsigned>(
								    item_owner_type::auction)) +
							    " AND owner_id=" +
							    std::to_string(auction) +
							    " AND owner_context_id=0 AND state=1 AND coin_payload IS NULL ORDER BY item_uid FOR UPDATE",
						    5, &rows) ||
					    rows.size() != source_uids.size())
						throw ESTALE;
					for (const auto &record : rows)
					{
						uint64_t uid = 0, root_uid = 0, parent_uid = 0,
							 revision = 0;
						int32_t vnum = 0;
						if (!auction_sql_number(record[0], &uid) ||
						    !auction_sql_number(record[1], &root_uid) ||
						    !auction_sql_number(record[2], &parent_uid) ||
						    !auction_sql_number(record[3], &revision) ||
						    !auction_sql_number(record[4], &vnum) ||
						    !source_uids.erase(uid))
							throw EILSEQ;
						auto fence = std::find_if(
							entry.command.expected_revisions.begin(),
							entry.command.expected_revisions.end(),
							[&](const auto &value) {
								return value.key.type ==
									       critical_entity_type::
										       item &&
								       value.key.id == uid;
							});
						if (fence ==
							    entry.command.expected_revisions.end() ||
						    fence->revision != revision)
							throw ESTALE;
						observed.custody.push_back(
							{ uid,
							  root_uid,
							  parent_uid,
							  { item_owner_type::auction, auction, 0 },
							  revision,
							  observed.auction_owner_revision,
							  vnum,
							  item_custody_state::active });
					}
					if (!source_uids.empty())
						throw ESTALE;
				}
			}
			if (!selected.empty())
			{
				if (!auction_sql_rows(
					    connection,
					    "SELECT item_uid FROM item_current_owner WHERE state=1 AND root_item_uid IN(" +
						    selected_roots +
						    ") ORDER BY item_uid FOR UPDATE",
					    1, &rows) ||
				    rows.size() != entry.context.selected_literals.size())
					throw ESTALE;
				std::set<uint64_t> expected_uids;
				for (const auto &item : entry.context.selected_literals)
					expected_uids.insert(item.object_uid);
				for (const auto &record : rows)
				{
					uint64_t uid = 0;
					if (!auction_sql_number(record[0], &uid) ||
					    !expected_uids.erase(uid))
						throw ESTALE;
				}
				if (!expected_uids.empty())
					throw ESTALE;
			}
			if (entry.payload.actor_pid)
			{
				shop_item_runtime_image image;
				if (!shop_item_runtime_lock_player_image(
					    connection, entry.payload.actor_pid,
					    std::span<const player_item_snapshot>(expected),
					    &image))
					throw EILSEQ;
				std::vector<player_item_snapshot> actual;
				actual.reserve(expected.size());
				for (const auto &item : expected)
				{
					auto found = image.find(item.object_uid);
					if (found == image.end() || !found->second.payload_present)
						throw EILSEQ;
					actual.push_back(found->second.item);
				}
				if (!same_items(actual, expected))
					throw ESTALE;
			}
			size_t root = 0;
			for (size_t i = 0; i < entry.context.selected_literals.size(); ++i)
			{
				const auto &item = entry.context.selected_literals[i];
				if (item.parent_index < 0)
					root = i;
				if (!auction_sql_rows(
					    connection,
					    "SELECT root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot,coin_payload IS NULL FROM item_current_owner WHERE item_uid=" +
						    std::to_string(item.object_uid) + " FOR UPDATE",
					    10, &rows) ||
				    rows.size() != 1)
					throw EILSEQ;
				uint64_t value[10]{};
				int32_t vnum = 0;
				for (size_t n = 0; n < 10; ++n)
					if (n != 6 && !auction_sql_number(rows[0][n], &value[n]))
						throw EILSEQ;
				auto fence = std::find_if(
					entry.command.expected_revisions.begin(),
					entry.command.expected_revisions.end(),
					[&](const auto &revision) {
						return revision.key.type ==
							       critical_entity_type::item &&
						       revision.key.id == item.object_uid;
					});
				if (fence == entry.command.expected_revisions.end() ||
				    (applied && fence->revision == UINT64_MAX))
					throw EILSEQ;
				const auto owner =
					entry.payload.action == auction_action::list ?
						(applied ?
							 item_owner_identity{
								 item_owner_type::auction, auction,
								 0 } :
							 item_owner_identity{
								 item_owner_type::player,
								 entry.payload.actor_pid, 0 }) :
						(applied ?
							 item_owner_identity{
								 item_owner_type::player,
								 entry.payload.actor_pid, 0 } :
							 item_owner_identity{
								 item_owner_type::auction, auction,
								 0 });
				const uint64_t parent =
					item.parent_index < 0 ?
						0 :
						entry.context.selected_literals[item.parent_index]
							.object_uid;
				if (!auction_sql_number(rows[0][6], &vnum) || vnum != item.vnum ||
				    value[0] != entry.context.selected_literals[root].object_uid ||
				    value[1] != parent ||
				    value[2] != static_cast<unsigned>(owner.type) ||
				    value[3] != owner.id || value[4] ||
				    value[5] != fence->revision + (applied ? 1 : 0) ||
				    value[7] != static_cast<unsigned>(item_custody_state::active) ||
				    value[8] || value[9] != 1)
					throw ESTALE;
				observed.custody.push_back(
					{ item.object_uid, value[0], parent, owner, value[5],
					  owner.type == item_owner_type::player ?
						  observed.player_owner_revision :
						  observed.auction_owner_revision,
					  vnum, item_custody_state::active });
			}
			proven = transaction.same_session() &&
				 (connection->server_status & SERVER_STATUS_IN_TRANS);
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		if (!proven || !cleanup.rollback_confirmed || cleanup.cleanup_error ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		lease.reuse(cleanup);
		out = std::move(observed);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool auction_native_publication_owner::working_world(auction_native_pending &entry,
						     auction_native_world_observation &out,
						     std::vector<size_t> &root_positions) noexcept
{
	try
	{
		std::vector<player_item_snapshot> working = entry.context.player_before;
		std::vector<std::pair<size_t, size_t>> ranges;
		const bool listing = entry.payload.action == auction_action::list,
			   claiming = entry.payload.action == auction_action::claim_item;
		if ((listing || claiming) &&
		    !selected_ranges(entry.payload, entry.context.selected_literals, &ranges))
			return false;
		if (listing)
		{
			for (size_t i = 0; i < ranges.size(); ++i)
				if (entry.context.roots[i][0].state == 2)
				{
					std::vector<player_item_snapshot> removed, remaining;
					if (player_item_snapshot_extract_subtree(
						    working, entry.payload.items[i].item_uid,
						    &removed,
						    &remaining) != player_snapshot_codec_result::ok)
						return false;
					working = std::move(remaining);
				}
		}
		if (claiming)
		{
			for (size_t i = 0; i < ranges.size(); ++i)
				if (entry.context.roots[i][3].state == 2)
				{
					auto one = entry.payload;
					one.item_count = 1;
					one.items[0] = entry.payload.items[i];
					std::vector<player_item_snapshot> selected(
						entry.context.selected_literals.begin() +
							ranges[i].first,
						entry.context.selected_literals.begin() +
							ranges[i].second),
						next;
					for (auto &item : selected)
						if (item.parent_index >= 0)
							item.parent_index -= static_cast<int32_t>(
								ranges[i].first);
					if (!auction_native_expected_player_forest(
						    one, working, selected, false,
						    entry.context.original_level, &next))
						return false;
					working = std::move(next);
				}
		}
		std::vector<auction_native_world_uid> requirements;
		std::vector<size_t> positions;
		for (size_t i = 0; i < ranges.size(); ++i)
		{
			positions.push_back(requirements.size());
			auto location =
				listing ?
					(entry.context.roots[i][1].state == 2 ?
						 auction_native_world_location::absent :
						 (entry.context.roots[i][0].state == 2 ?
							  auction_native_world_location::detached :
							  auction_native_world_location::carried)) :
					(entry.context.roots[i][3].state == 2 ?
						 auction_native_world_location::carried :
						 (entry.context.roots[i][2].state == 2 ?
							  auction_native_world_location::detached :
							  auction_native_world_location::absent));
			requirements.push_back({ entry.payload.items[i].item_uid, location });
			if (location == auction_native_world_location::absent)
				for (size_t j = ranges[i].first + 1; j < ranges[i].second; ++j)
					requirements.push_back(
						{ entry.context.selected_literals[j].object_uid,
						  location });
		}
		if (ranges.empty())
			for (const auto &key : entry.command.keys)
				if (key.type == critical_entity_type::item)
					requirements.push_back(
						{ key.id, auction_native_world_location::absent });
		auction_native_world_observation observed;
		if (!auction_native_world_observe(entry.payload.actor_pid, entry.runtime, working,
						  requirements, &observed))
			return false;
		for (size_t i = 0; i < ranges.size(); ++i)
		{
			if (!observed.selected[positions[i]])
				continue;
			std::vector<player_item_snapshot> selected(
				entry.context.selected_literals.begin() + ranges[i].first,
				entry.context.selected_literals.begin() + ranges[i].second);
			for (auto &item : selected)
				if (item.parent_index >= 0)
					item.parent_index -= static_cast<int32_t>(ranges[i].first);
			if (claiming && entry.context.roots[i][3].state == 2 &&
			    !selected[0].generated_key && entry.context.original_level < 57 &&
			    entry.payload.actor_pid < 10000000)
				selected[0].generated_key = 1;
			if (!same_items(selected, observed.selected_trees[positions[i]]))
				return false;
		}
		out = std::move(observed);
		root_positions = std::move(positions);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

#ifndef __NO_MYSQL__
bool auction_native_publication_owner::stage_claim(auction_native_pending &entry) noexcept
{
	try
	{
		if (entry.staging_ready)
			return true;
		if (std::any_of(entry.context.roots.begin(), entry.context.roots.end(),
				[](const auto &root) { return root[2].state != 0; }))
			return false;
		std::vector<std::pair<size_t, size_t>> ranges;
		if (!selected_ranges(entry.payload, entry.context.selected_literals, &ranges))
			return false;
		std::vector<auction_original_item_stage> stages(
			entry.context.selected_literals.size());
		std::vector<shop_trade_original_procedure_binding_stage> bindings(ranges.size());
		std::map<int, size_t> counts;
		for (size_t i = 0; i < stages.size(); ++i)
		{
			const auto *prototype = find_recovery_object_template(
				entry.context.selected_literals[i].vnum);
			if (!prototype ||
			    !auction_original_item_stage::prepare(
				    *prototype, entry.context.selected_literals[i], stages[i]) ||
			    !stages[i].get())
				return false;
			++counts[prototype->R_num];
		}
		for (const auto &[number, count] : counts)
			if (!obj_index || number < 0 || number > top_of_objt ||
			    obj_index[number].number < 0 ||
			    count > static_cast<size_t>(INT_MAX - obj_index[number].number))
				return false;
		for (size_t root = 0; root < ranges.size(); ++root)
		{
			auto [begin, end] = ranges[root];
			std::vector<P_obj> objects;
			std::vector<player_item_snapshot> literals;
			for (size_t i = begin; i < end; ++i)
			{
				objects.push_back(stages[i].get());
				auto value = entry.context.selected_literals[i];
				if (value.parent_index >= 0)
					value.parent_index -= static_cast<int32_t>(begin);
				literals.push_back(std::move(value));
			}
			if (!shop_trade_original_procedure_binding_stage::prepare(objects, literals,
										  bindings[root]))
				return false;
		}
		// Original full literal forest is linked privately, preserving sibling order.
		// No global list, count, constructor, UID or callback effect has occurred.
		for (size_t i = stages.size(); i-- > 0;)
			if (entry.context.selected_literals[i].parent_index >= 0)
			{
				P_obj child = stages[i].get(),
				      parent =
					      stages[entry.context.selected_literals[i].parent_index]
						      .get();
				child->loc_p = LOC_INSIDE;
				child->loc.inside = parent;
				child->next_content = parent->contains;
				parent->contains = child;
			}
		entry.staged = std::move(stages);
		entry.bindings = std::move(bindings);
		entry.staging_ready = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
}
#endif

bool auction_native_publication_owner::physical(auction_native_pending &entry) noexcept
{
#ifdef __NO_MYSQL__
	(void)entry;
	return false;
#else
	try
	{
		if (!refresh(entry) || !entry.ready || entry.blocked)
			return false;
		auction_current_cut current;
		auction_native_world_observation observed;
		std::vector<size_t> positions;
		const auto witness = [&]() {
			return sql_cut(entry, false, current) &&
			       working_world(entry, observed, positions);
		};
		const bool cold = entry.restored && sql_cut(entry, false, current) &&
				  after_world(entry, current, observed);
		if (!cold && !witness())
			return false;
		if (!entry.context.receipt_present)
		{
			if (entry.completion.result_size != entry.context.result.size() ||
			    entry.completion.failure_stage != critical_failure_stage::none)
				return false;
			entry.context.receipt_present = true;
			entry.context.outcome = entry.completion.outcome;
			entry.context.result_code = entry.completion.error_code;
			entry.context.failure_stage = entry.completion.failure_stage;
			entry.context.durable_revision = entry.completion.durable_revision;
			std::copy_n(entry.completion.result_payload.begin(),
				    entry.context.result.size(), entry.context.result.begin());
			if (!auction_native_expected_player_forest(
				    entry.payload, entry.context.player_before,
				    entry.context.selected_literals,
				    entry.completion.error_code != 0, entry.context.original_level,
				    &entry.context.player_after))
				return false;
			entry.context.after_present = entry.payload.actor_pid != 0;
			entry.context.stage = auction_recovery_stage::publishing;
			entry.context_dirty = true;
			if (!synchronize(entry))
				return false;
		}
		if (entry.context.stage == auction_recovery_stage::restored_after_proven)
			return entry.restored && sql_cut(entry, false, current) &&
			       after_world(entry, current, observed);
		if (cold && entry.context.stage != auction_recovery_stage::physically_proven)
		{
			// This distinct source kind preserves every original callback progress byte.
			// Actual private pipeline CAS requires a genuinely replayed, rebound slot.
			entry.context.stage = auction_recovery_stage::restored_after_proven;
			entry.context_dirty = true;
			return synchronize(entry) && sql_cut(entry, false, current) &&
			       after_world(entry, current, observed);
		}
		if (entry.context.stage == auction_recovery_stage::physically_proven)
			return witness() && runtime_matches(entry, current, observed);
		const bool applied = entry.context.result_code == 0;
		std::vector<std::pair<size_t, size_t>> ranges;
		if (applied &&
		    (entry.payload.action == auction_action::list ||
		     entry.payload.action == auction_action::claim_item) &&
		    !selected_ranges(entry.payload, entry.context.selected_literals, &ranges))
			return false;
		if (applied && entry.payload.action == auction_action::claim_item)
		{
			if (!stage_claim(entry))
				return false;
			for (size_t root = 0; root < ranges.size(); ++root)
			{
				auto [begin, end] = ranges[root];
				if (!run_effect(
					    entry, { 0, root, 2 },
					    [&](auto &)
					    {
						    if (!witness() || !entry.staging_ready ||
							root >= entry.bindings.size() ||
							!entry.bindings[root].valid())
							    return false;
						    if (!quest_mobile_native_item_cold_prepend_cut_ready(
								end - begin))
							    return false;
						    std::map<int, size_t> counts;
						    for (size_t i = begin; i < end; ++i)
						    {
							    P_obj object = entry.staged[i].get();
							    if (!object ||
								!quest_mobile_native_item_cold_prepend_body_ready(
									object))
								    return false;
							    ++counts[object->R_num];
						    }
						    for (const auto &[number, count] : counts)
							    if (!obj_index || number < 0 ||
								number > top_of_objt ||
								obj_index[number].number < 0 ||
								count > static_cast<size_t>(
										INT_MAX -
										obj_index[number]
											.number))
								    return false;
						    entry.bindings[root].commit_unchecked();
						    for (size_t i = begin; i < end; ++i)
						    {
							    P_obj object =
								    entry.staged[i].consume();
							    quest_mobile_native_item_observe_native_prepend(
								    object);
							    object->next = object_list;
							    if (object_list)
								    object_list->prev = object;
							    object_list = object;
							    ++obj_index[object->R_num].number;
						    }
						    return true;
					    }))
					return false;
				for (size_t node = begin; node < end; ++node)
					for (size_t step = 0; step < 4; ++step)
					{
						if (step == 2)
							for (size_t description = 0;
							     description <
							     entry.context.proclib[node].size();
							     ++description)
								if (!run_effect(
									    entry,
									    { 2, node,
									      description },
									    [&](auto &progress)
									    {
										    if (!witness())
											    return false;
										    P_obj object =
											    observed.selected
												    [positions[root]];
										    // Reacquire the exact node from the newly observed root census each time.
										    std::vector<P_obj>
											    todo{
												    object
											    };
										    object =
											    nullptr;
										    while (!todo.empty())
										    {
											    auto value =
												    todo.back();
											    todo.pop_back();
											    if (value->obj_uid ==
												entry.context
													.selected_literals
														[node]
													.object_uid)
											    {
												    object =
													    value;
												    break;
											    }
											    for (P_obj child =
													 value->contains;
												 child;
												 child = child->next_content)
												    todo.push_back(
													    child);
										    }
										    auto *prototype = find_recovery_object_template(
											    entry.context
												    .selected_literals
													    [node]
												    .vnum);
										    shop_trade_original_reload_effect
											    actual;
										    if (!object ||
											!prototype ||
											!auction_original_item_stage::proclib_probe(
												object,
												*prototype,
												description,
												actual) ||
											!actual.started ||
											!actual.returned ||
											!actual.succeeded)
											    return false;
										    progress.periodic =
											    actual.periodic;
										    return true;
									    }))
									return false;
						if (!run_effect(
							    entry, { 1, node, step },
							    [&](auto &progress)
							    {
								    if (!witness())
									    return false;
								    P_obj object =
									    observed.selected
										    [positions[root]];
								    std::vector<P_obj> todo{
									    object
								    };
								    object = nullptr;
								    while (!todo.empty())
								    {
									    auto value =
										    todo.back();
									    todo.pop_back();
									    if (value->obj_uid ==
										entry.context
											.selected_literals
												[node]
											.object_uid)
									    {
										    object = value;
										    break;
									    }
									    for (P_obj child =
											 value->contains;
										 child;
										 child = child->next_content)
										    todo.push_back(
											    child);
								    }
								    auto *prototype = find_recovery_object_template(
									    entry.context
										    .selected_literals
											    [node]
										    .vnum);
								    shop_trade_original_reload_effect
									    actual;
								    if (step == 1)
									    actual.periodic =
										    entry.context
											    .reload[node]
												   [0]
											    .periodic;
								    if (step == 2)
									    actual.periodic = std::any_of(
										    entry.context
											    .proclib[node]
											    .begin(),
										    entry.context
											    .proclib[node]
											    .end(),
										    [](const auto &
											       probe)
										    {
											    return probe
												    .periodic;
										    });
								    if (!object || !prototype ||
									!auction_original_item_stage::
										reload_step(
											object,
											*prototype,
											step,
											actual) ||
									!actual.started ||
									!actual.returned ||
									!actual.succeeded)
									    return false;
								    progress.periodic =
									    actual.periodic;
								    return true;
							    }))
							return false;
					}
				if (!run_effect(entry, { 0, root, 3 },
						[&](auto &)
						{
							if (!witness() || !observed.actor ||
							    !observed.selected[positions[root]])
								return false;
							obj_to_char(
								observed.selected[positions[root]],
								observed.actor);
							return true;
						}))
					return false;
			}
		}
		if (applied && entry.payload.action == auction_action::list)
			for (size_t root = 0; root < ranges.size(); ++root)
			{
				if (!run_effect(entry, { 0, root, 0 },
						[&](auto &)
						{
							if (!witness() || !observed.actor ||
							    !observed.selected[positions[root]])
								return false;
							obj_from_char(
								observed.selected[positions[root]]);
							return true;
						}))
					return false;
				if (!run_effect(entry, { 0, root, 1 },
						[&](auto &)
						{
							if (!witness() ||
							    !observed.selected[positions[root]])
								return false;
							extract_obj(
								observed.selected[positions[root]]);
							return true;
						}))
					return false;
			}
		if (applied)
			for (size_t root = 0; root < ranges.size(); ++root)
				if (!run_effect(
					    entry, { 0, root, 4 },
					    [&](auto &)
					    {
						    if (!witness())
							    return false;
						    auto [begin, end] = ranges[root];
						    auction_command_result actual_receipt{};
						    if (!auction_command_decode_result(
								entry.context.result.data(),
								entry.context.result.size(),
								&actual_receipt) ||
							!actual_receipt.auction_id)
							    return false;
						    return item_ownership_runtime_hydrate_owner(
								   { item_owner_type::player,
								     entry.payload.actor_pid, 0 },
								   current.player_owner_revision) &&
							   item_ownership_runtime_hydrate_owner(
								   { item_owner_type::auction,
								     actual_receipt.auction_id, 0 },
								   current.auction_owner_revision) &&
							   item_ownership_runtime_hydrate_many_atomic(
								   current.custody.data() + begin,
								   end - begin);
					    }))
					return false;
		auction_command_result receipt{};
		if (!auction_command_decode_result(entry.context.result.data(),
						   entry.context.result.size(), &receipt))
			return false;
		if (applied && (entry.payload.action == auction_action::bid ||
				entry.payload.action == auction_action::finalize ||
				entry.payload.action == auction_action::remove))
			if (!run_effect(entry, { 5, 0, 0 },
					[&](auto &)
					{
						if (!witness() || !receipt.auction_id)
							return false;
						return item_ownership_runtime_hydrate_owner(
							       { item_owner_type::auction,
								 receipt.auction_id, 0 },
							       current.auction_owner_revision) &&
						       item_ownership_runtime_hydrate_many_atomic(
							       current.custody.data(),
							       current.custody.size());
					}))
				return false;
		if (applied && entry.payload.actor_pid && receipt.wallet_revision)
			if (!run_effect(entry, { 3, 0, 0 },
					[&](auto &)
					{
						return witness() && observed.actor &&
						       currency_transaction_publish_balances(
							       observed.actor,
							       entry.payload.account_name.data(),
							       entry.payload.racewar,
							       current.wallet, current.bank,
							       current.wallet_revision,
							       current.bank_revision);
					}))
				return false;
		if (!witness() || !runtime_matches(entry, current, observed) ||
		    (entry.payload.actor_pid &&
		     (!observed.actor ||
		      !same_items(observed.player_items, entry.context.player_after))))
			return false;
		entry.context.stage = auction_recovery_stage::physically_proven;
		entry.context_dirty = true;
		return synchronize(entry);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool auction_native_publication_owner::prepare(auction_native_pending &entry) noexcept
{
	try
	{
		if (entry.submitted || entry.restored)
			return true;
		P_char ch = entry.payload.actor_pid ? actor(entry) : nullptr;
		if (entry.payload.actor_pid)
		{
			if (!ch)
				return false;
			if (!entry.held)
			{
				const auto state = player_save_pipeline_auction_checkpoint_poll(
					entry.token, ch);
				if (state == player_literal_inventory_state::pending)
					return true;
				if (state !=
					    player_literal_inventory_state::database_acknowledged ||
				    !player_save_pipeline_auction_checkpoint_hold(
					    entry.token, entry.command.operation_id))
					return false;
				entry.held = true;
			}
		}
		critical_command prepared;
		auction_recovery_context context;
		if (!auction_preparation_owner::freeze(ch, entry.token, entry.command, &prepared,
						       &context))
			return false;
		auto now = std::chrono::duration_cast<std::chrono::microseconds>(
				   std::chrono::system_clock::now().time_since_epoch())
				   .count();
		if (now <= 0)
			return false;
		prepared.accepted_at_usec = static_cast<uint64_t>(now);
		prepared.publication_required = true;
		critical_native_recovery_envelope envelope;
		envelope.command = prepared;
		envelope.revision = 1;
		if (!critical_command_envelope_valid(prepared) ||
		    auction_recovery_context_encode(prepared, context, &envelope.attachment) !=
			    player_snapshot_codec_result::ok ||
		    !auction_recovery_initial_valid(envelope) ||
		    !retain_size(entry, envelope.attachment.size()))
			return false;
		// Strong original retention precedes submission. After crossing this boundary
		// an ambiguous append, overload or delivered refusal cannot create a fresh ID.
		entry.command = std::move(prepared);
		entry.context = std::move(context);
		entry.envelope = std::move(envelope);
		entry.submitted = true;
		bool released = false;
		const auto submitted =
			entry.payload.actor_pid ?
				player_save_auction_checkpoint_owner::submit_owned(
					entry.token, entry.command, entry.envelope.attachment,
					&released) :
				critical_native_auction_background_publication_owner::submit(
					entry.envelope);
		if (released || (!entry.payload.actor_pid &&
				 !critical_submit_result_keeps_operation(submitted)))
		{
			entry.held = false;
			entry.cancelled = true;
			return true;
		}
		// Non-kept outcomes still retain the original local preparation/held slot.
		// Only actual typed never-admitted cancellation may subsequently retire it.
		return critical_submit_result_keeps_operation(submitted) || entry.submitted;
	}
	catch (...)
	{
		return entry.submitted;
	}
}

bool auction_native_publication_owner::notify(auction_native_pending &entry) noexcept
{
	try
	{
		if (!entry.continuation_released ||
		    entry.envelope.phase != critical_native_recovery_phase::continuation_pending ||
		    !entry.context.receipt_present)
			return false;
		auction_command_result receipt{};
		if (!auction_command_decode_result(entry.context.result.data(),
						   entry.context.result.size(), &receipt) ||
		    (entry.payload.actor_pid && !actor(entry)))
			return false;
		if (!run_effect(
			    entry, { 4, 0, 0 },
			    [&](auto &)
			    {
				    P_char ch = entry.payload.actor_pid ? actor(entry) : nullptr;
				    if (entry.payload.actor_pid && !ch)
					    return false;
				    const bool committed = !entry.context.result_code;
				    if (entry.payload.action == auction_action::list)
				    {
					    if (committed)
					    {
						    mark_player_dirty_components(
							    GET_PID(ch),
							    PLAYER_COMPONENT_STATUS |
								    PLAYER_COMPONENT_EQUIPMENT |
								    PLAYER_COMPONENT_INVENTORY);
						    send_to_char_f(
							    ch,
							    "&+W%s is now listed as auction %u.&n\r\n",
							    entry.payload.object_short.data(),
							    receipt.auction_id);
					    }
					    else
						    send_to_char(
							    "The auction was not listed; your item and money are unchanged.\r\n",
							    ch);
				    }
				    else if (entry.payload.action == auction_action::claim_item)
				    {
					    if (committed)
					    {
						    for (const auto &item :
							 entry.context.selected_literals)
							    if (item.parent_index < 0)
								    send_to_char_f(
									    ch,
									    "&+WYou pick up &n%s&+W.&n\r\n",
									    item.short_description
										    .c_str());
						    mark_player_dirty_components(
							    GET_PID(ch),
							    PLAYER_COMPONENT_STATUS |
								    PLAYER_COMPONENT_EQUIPMENT |
								    PLAYER_COMPONENT_INVENTORY);
					    }
					    else
						    send_to_char(
							    "Your auction items remain available for pickup.\r\n",
							    ch);
				    }
				    else if (entry.payload.action == auction_action::bid)
				    {
					    if (!committed)
						    send_to_char(
							    "Your bid did not commit; your money is unchanged.\r\n",
							    ch);
					    else if (receipt.claim_credit_used > 0)
						    send_to_char_f(
							    ch,
							    "&+WYour bid of &n%s&+W on auction %u committed. It used &n%s&+W in pending auction credit and &n%s&+W from your wallet.&n\r\n",
							    coin_stringv(static_cast<int>(
								    receipt.final_price)),
							    receipt.auction_id,
							    coin_stringv(static_cast<int>(
								    receipt.claim_credit_used)),
							    coin_stringv(static_cast<int>(
								    -receipt.wallet_value_delta)));
					    else
						    send_to_char_f(
							    ch,
							    "&+WYour bid of &n%s&+W on auction %u committed.&n\r\n",
							    coin_stringv(static_cast<int>(
								    receipt.final_price)),
							    receipt.auction_id);
				    }
				    else if (entry.payload.action == auction_action::remove && ch)
				    {
					    if (!committed)
						    send_to_char(
							    "That auction could not be removed.\r\n",
							    ch);
					    else
					    {
						    send_to_char_f(ch,
								   "&+WAuction %u removed.&n\r\n",
								   receipt.auction_id);
						    logit(LOG_WIZ, "Auction [%u] removed by %s",
							  receipt.auction_id,
							  entry.payload.actor_name.data());
					    }
				    }
				    else if (entry.payload.action == auction_action::claim_money)
				    {
					    if (!committed)
						    send_to_char(
							    "Your auction money remains available for pickup.\r\n",
							    ch);
					    else
						    send_to_char_f(
							    ch, "&+WYou pick up &n%s&+W.&n\r\n",
							    coin_stringv(static_cast<int>(
								    receipt.wallet_value_delta)));
				    }
				    // Finalize has no original player notice. No legacy mutation callback runs.
				    return true;
			    }))
			return false;
		return auction_recovery_terminal_valid(entry.envelope) &&
		       critical_native_auction_continuation_owner::retire_continuation(
			       entry.envelope);
	}
	catch (...)
	{
		return false;
	}
}

bool auction_native_publication_owner::advance(auction_native_pending &entry) noexcept
{
	try
	{
		if (entry.running || entry.blocked)
			return false;
		if (entry.restored && entry.ready && entry.payload.actor_pid && !entry.runtime)
		{
			P_char ch = actor(entry);
			if (ch)
				ready(ch);
		}
		struct running_guard
		{
			bool &flag;
			explicit running_guard(bool &value)
				: flag(value)
			{
				flag = true;
			}
			~running_guard() { flag = false; }
		} guard(entry.running);
		if (!entry.submitted && !entry.restored)
		{
			if (!prepare(entry))
			{
				const bool released =
					entry.payload.actor_pid ?
						(entry.held ?
							 player_save_pipeline_auction_checkpoint_release(
								 entry.token,
								 entry.command.operation_id) :
							 player_save_pipeline_auction_checkpoint_cancel(
								 entry.token)) :
						true;
				if (!released || entry.submitted)
				{
					entry.blocked = true;
					return false;
				}
				entry.held = false;
				entry.cancelled = true;
			}
			if (!entry.cancelled)
				return false;
		}
		if (entry.cancelled)
		{
			P_char ch = entry.payload.actor_pid ? actor(entry) : nullptr;
			if (entry.payload.actor_pid && !ch)
				return false;
			entry.blocked = true;
			if (entry.notification)
				entry.notification(ch, false, {},
						   entry.ready ? entry.completion.error_code :
								 EAGAIN,
						   entry.payload);
			return true;
		}
		if (!entry.ready)
			return false;
		if (entry.completion.disposition == critical_completion_disposition::never_admitted)
		{
			bool retired = false;
			if (entry.payload.actor_pid)
				retired = player_save_auction_publication_owner::publish_auction(
					entry.command, entry.completion, refusal, &entry);
			else
			{
				uint64_t generation = 0;
				retired = critical_native_auction_background_publication_owner::
						  observe_generation(entry.envelope, &generation) &&
					  critical_native_auction_background_publication_owner::
						  cancel_refusal(entry.envelope, entry.completion,
								 generation, refusal, &entry);
			}
			if (retired)
			{
				entry.cancelled = true;
				entry.held = false;
			}
			return false;
		}
		if (!refresh(entry))
			return false;
		if (entry.continuation_released)
			return notify(entry);
		if (entry.payload.actor_pid)
		{
			if (!player_save_auction_publication_owner::publish_auction(
				    entry.command, entry.completion, guarded, &entry))
				return false;
			entry.held = false;
		}
		else
		{
			if (!physical(entry))
				return false;
			uint64_t generation = 0;
			if (!critical_native_auction_background_publication_owner::observe_generation(
				    entry.envelope, &generation) ||
			    !critical_native_auction_background_publication_owner::acknowledge(
				    entry.envelope, entry.completion, generation))
				return false;
		}
		return refresh(entry) && entry.continuation_released && notify(entry);
	}
	catch (...)
	{
		return false;
	}
}

bool auction_native_publication_owner::submit(P_char ch, const auction_command_payload &payload,
					      auction_completion_fn notification,
					      critical_source_site site,
					      critical_deadline_class deadline) noexcept
{
#ifdef __NO_MYSQL__
	(void)ch;
	(void)payload;
	(void)notification;
	(void)site;
	(void)deadline;
	return false;
#else
	try
	{
		if (!nevent_is_game_thread() ||
		    auction_native_pending_operations.size() >= AUCTION_PENDING_MAX ||
		    (payload.actor_pid ? (!ch || IS_NPC(ch) || GET_PID(ch) <= 0 ||
					  static_cast<uint32_t>(GET_PID(ch)) != payload.actor_pid ||
					  !ch->runtime_id || busy(ch)) :
					 (ch || (payload.action != auction_action::finalize &&
						 payload.action != auction_action::remove))))
			return false;
		auto entry = std::make_unique<auction_native_pending>();
		critical_operation_id operation{};
		if (!critical_operation_id_generate(&operation) ||
		    !auction_command_build(&entry->command, operation, payload, site, deadline))
			return false;
		entry->payload = payload;
		entry->notification = notification;
		entry->runtime = ch ? ch->runtime_id : 0;
		const auto key = auction_native_key(operation);
		std::vector<uint64_t> roots;
		if (ch && payload.action == auction_action::list)
			for (size_t i = 0; i < payload.item_count; ++i)
				roots.push_back(payload.items[i].item_uid);
		auto [found, inserted] =
			auction_native_pending_operations.emplace(key, std::move(entry));
		if (!inserted)
			return false;
		auto &retained = *found->second;
		if (ch)
		{
			if (ch->in_room < 0 || ch->in_room > top_of_world ||
			    player_save_pipeline_auction_checkpoint_begin(
				    ch, world[ch->in_room].number, roots, &retained.token) ==
				    player_literal_inventory_state::refused)
			{
				auction_native_pending_operations.erase(found);
				return false;
			}
		}
		if (!prepare(retained))
		{
			// No native or submission effect occurred before submitted became true.
			// Release only the exact original unadmitted checkpoint; failure retains it.
			if (!retained.submitted &&
			    (retained.held ?
				     player_save_pipeline_auction_checkpoint_release(
					     retained.token, retained.command.operation_id) :
				     (!ch || player_save_pipeline_auction_checkpoint_cancel(
						     retained.token))))
			{
				auction_native_retained_bytes -= retained.retained_bytes;
				auction_native_pending_operations.erase(found);
				return false;
			}
			retained.blocked = true;
		}
		if (retained.cancelled)
		{
			auction_native_retained_bytes -= retained.retained_bytes;
			auction_native_pending_operations.erase(found);
			return false;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

void auction_native_publication_owner::completions(const critical_completion *values,
						   size_t count) noexcept
{
	if (!nevent_is_game_thread() || (!values && count))
		return;
	try
	{
		for (size_t i = 0; i < count; ++i)
		{
			auto found = auction_native_pending_operations.find(
				auction_native_key(values[i].operation_id));
			if (found == auction_native_pending_operations.end())
				continue;
			auto &entry = *found->second;
			const auto &receipt = values[i];
			if (!critical_completion_disposition_valid(receipt) ||
			    (receipt.outcome != critical_apply_outcome::applied &&
			     receipt.outcome != critical_apply_outcome::already_applied &&
			     receipt.outcome != critical_apply_outcome::terminal_failure))
				continue;
			if (entry.ready &&
			    !auction_native_same_completion(entry.completion, receipt))
			{
				entry.blocked = true;
				continue;
			}
			entry.completion = receipt;
			entry.ready = true;
		}
	}
	catch (...)
	{
	}
}

void auction_native_publication_owner::pulse() noexcept
{
	if (!nevent_is_game_thread())
		return;
	try
	{
		for (auto found = auction_native_pending_operations.begin();
		     found != auction_native_pending_operations.end();)
		{
			auto current = found++;
			auto &entry = *current->second;
			if (advance(entry))
			{
				auction_native_retained_bytes -= entry.retained_bytes;
				auction_native_pending_operations.erase(current);
			}
		}
	}
	catch (...)
	{
	}
}

void auction_native_publication_owner::ready(P_char ch) noexcept
{
	if (!nevent_is_game_thread() || !ch || IS_NPC(ch) || GET_PID(ch) <= 0 || !ch->runtime_id)
		return;
	try
	{
		for (auto &[id, value] : auction_native_pending_operations)
		{
			(void)id;
			auto &entry = *value;
			if (entry.payload.actor_pid != static_cast<uint32_t>(GET_PID(ch)) ||
			    entry.running)
				continue;
			if (entry.cancelled)
			{
				entry.runtime = ch->runtime_id;
				continue;
			}
			if (!entry.restored)
				continue;
			if (entry.envelope.phase ==
			    critical_native_recovery_phase::continuation_pending)
			{
				entry.runtime = ch->runtime_id;
				continue;
			}
			// Registration/rebind is passive and follows genuine SQL/whole-world proof,
			// never a PID-only identity replacement or an admission attempt.
			auction_current_cut cut;
			if (!entry.ready ||
			    !sql_cut(entry,
				     entry.completion.disposition ==
					     critical_completion_disposition::never_admitted,
				     cut))
				continue;
			const uint64_t old = entry.runtime;
			entry.runtime = ch->runtime_id;
			auction_native_world_observation observed;
			std::vector<size_t> roots;
			const bool world_proven =
				working_world(entry, observed, roots) ||
				(entry.completion.disposition ==
					 critical_completion_disposition::execution &&
				 after_world(entry, cut, observed));
			if (!world_proven ||
			    !player_save_auction_publication_owner::rebind_recovery_checkpoint(
				    entry.envelope, ch->runtime_id))
				entry.runtime = old;
		}
	}
	catch (...)
	{
	}
}

bool auction_native_publication_owner::busy(P_char ch) noexcept
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0)
		return false;
	if (!nevent_is_game_thread())
		return true;
	return std::any_of(auction_native_pending_operations.begin(),
			   auction_native_pending_operations.end(),
			   [&](const auto &value)
			   {
				   return value.second->payload.actor_pid ==
						  static_cast<uint32_t>(GET_PID(ch)) &&
					  !value.second->cancelled;
			   });
}

bool auction_native_publication_owner::restore(
	const critical_native_recovery_envelope &original) noexcept
{
	try
	{
		if (!nevent_is_game_thread() || !auction_recovery_envelope_valid(original))
			return false;
		auto key = auction_native_key(original.command.operation_id);
		auto found = auction_native_pending_operations.find(key);
		if (found != auction_native_pending_operations.end() &&
		    !critical_command_equal(found->second->command, original.command))
			return false;
		if (found != auction_native_pending_operations.end() &&
		    found->second->envelope.revision)
			return found->second->envelope.revision == original.revision &&
			       found->second->envelope.phase == original.phase &&
			       found->second->envelope.attachment == original.attachment;
		if (found == auction_native_pending_operations.end() &&
		    auction_native_pending_operations.size() >= AUCTION_PENDING_MAX)
			return false;
		auto entry = std::make_unique<auction_native_pending>();
		entry->command = original.command;
		entry->envelope = original;
		entry->restored = true;
		entry->submitted = true;
		if (!auction_command_decode_payload(entry->command, &entry->payload) ||
		    auction_recovery_context_decode(entry->command, original.attachment,
						    &entry->context) !=
			    player_snapshot_codec_result::ok)
			return false;
		if (entry->context.receipt_present)
		{
			entry->completion.operation_id = entry->command.operation_id;
			entry->completion.outcome = entry->context.outcome;
			entry->completion.error_code = entry->context.result_code;
			entry->completion.failure_stage = entry->context.failure_stage;
			entry->completion.durable_revision = entry->context.durable_revision;
			entry->completion.result_size = entry->context.result.size();
			std::copy(entry->context.result.begin(), entry->context.result.end(),
				  entry->completion.result_payload.begin());
			entry->ready = true;
		}
		if (original.attachment.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
		    auction_native_retained_bytes -
				    (found == auction_native_pending_operations.end() ?
					     0 :
					     found->second->retained_bytes) >
			    auction_native_total_bytes - original.attachment.size())
			return false;
		std::unique_ptr<auction_native_pending> previous;
		const bool inserted = found == auction_native_pending_operations.end();
		if (inserted)
		{
			auto result =
				auction_native_pending_operations.emplace(key, std::move(entry));
			if (!result.second)
				return false;
			found = result.first;
		}
		else
		{
			previous = std::move(found->second);
			found->second = std::move(entry);
		}
		auto &installed = *found->second;
		if (installed.payload.actor_pid &&
		    original.phase == critical_native_recovery_phase::execution_pending &&
		    !player_save_auction_publication_owner::restore_recovery_checkpoint(original))
		{
			if (inserted)
				auction_native_pending_operations.erase(found);
			else
				found->second = std::move(previous);
			return false;
		}
		if (previous)
			auction_native_retained_bytes -= previous->retained_bytes;
		installed.retained_bytes = original.attachment.size();
		auction_native_retained_bytes += installed.retained_bytes;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool auction_native_publication_owner::replayed(const critical_command &command) noexcept
{
	try
	{
		if (!nevent_is_game_thread() ||
		    !auction_repository_frozen_accounting_valid(command))
			return false;
		auction_native_command_context context;
		if (auction_native_command_decode(command, &context) !=
		    economic_accounting_error::ok)
			return false;
		auto key = auction_native_key(command.operation_id);
		auto found = auction_native_pending_operations.find(key);
		if (found != auction_native_pending_operations.end())
			return critical_command_equal(found->second->command, command);
		if (auction_native_pending_operations.size() >= AUCTION_PENDING_MAX)
			return false;
		auto entry = std::make_unique<auction_native_pending>();
		entry->command = command;
		entry->payload = context.payload;
		entry->restored = true;
		// Original values await the actual NAR replay callback. This shell neither
		// recaptures history nor admits/executes/release/ACKs a command-only record.
		auction_native_pending_operations.emplace(key, std::move(entry));
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool auction_native_publication_submit(P_char ch, const auction_command_payload &payload,
				       auction_completion_fn callback, critical_source_site site,
				       critical_deadline_class deadline) noexcept
{
	return auction_native_publication_owner::submit(ch, payload, callback, site, deadline);
}
void auction_native_publication_completions(const critical_completion *values,
					    size_t count) noexcept
{
	auction_native_publication_owner::completions(values, count);
}
void auction_native_publication_pulse() noexcept
{
	auction_native_publication_owner::pulse();
}
void auction_native_publication_player_ready(P_char ch) noexcept
{
	auction_native_publication_owner::ready(ch);
}
bool auction_native_publication_player_busy(P_char ch) noexcept
{
	return auction_native_publication_owner::busy(ch);
}
bool auction_native_publication_restore(const critical_native_recovery_envelope &value) noexcept
{
	return auction_native_publication_owner::restore(value);
}
bool auction_native_publication_restore_replayed_command(const critical_command &value) noexcept
{
	return auction_native_publication_owner::replayed(value);
}

bool auction_native_publication_owner::after_world(auction_native_pending &entry,
						   const auction_current_cut &cut,
						   auction_native_world_observation &out) noexcept
{
	try
	{
		if (!entry.restored || !entry.payload.actor_pid || !entry.runtime)
			return false;
		std::vector<player_item_snapshot> after;
		if (!auction_native_expected_player_forest(
			    entry.payload, entry.context.player_before,
			    entry.context.selected_literals, entry.completion.error_code != 0,
			    entry.context.original_level, &after))
			return false;
		const bool claiming = entry.payload.action == auction_action::claim_item &&
				      !entry.completion.error_code;
		std::vector<auction_native_world_uid> requirements;
		for (const auto &item : entry.context.selected_literals)
		{
			if (claiming || (entry.payload.action == auction_action::list &&
					 entry.completion.error_code))
			{
				if (item.parent_index < 0)
					requirements.push_back(
						{ item.object_uid,
						  auction_native_world_location::carried });
			}
			else
				requirements.push_back(
					{ item.object_uid,
					  entry.payload.action == auction_action::list &&
							  !entry.completion.error_code ?
						  auction_native_world_location::absent :
						  (item.parent_index < 0 &&
								   entry.payload.action ==
									   auction_action::list ?
							   auction_native_world_location::carried :
							   auction_native_world_location::absent) });
		}
		if (entry.context.selected_literals.empty())
			for (const auto &key : entry.command.keys)
				if (key.type == critical_entity_type::item)
					requirements.push_back(
						{ key.id, auction_native_world_location::absent });
		auction_native_world_observation observed;
		if (!auction_native_world_observe(entry.payload.actor_pid, entry.runtime, after,
						  requirements, &observed) ||
		    !runtime_matches(entry, cut, observed))
			return false;
		out = std::move(observed);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool auction_native_publication_owner::runtime_matches(
	auction_native_pending &entry, const auction_current_cut &cut,
	const auction_native_world_observation &world) noexcept
{
	if (entry.payload.actor_pid &&
	    (!world.actor || !world.actor->only.pc ||
	     IS_SET(world.actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
	     world.actor->only.pc->wallet_revision != cut.wallet_revision ||
	     world.actor->only.pc->bank_revision != cut.bank_revision ||
	     std::array<int64_t, 4>{ GET_COPPER(world.actor), GET_SILVER(world.actor),
				     GET_GOLD(world.actor),
				     GET_PLATINUM(world.actor) } != cut.wallet.amount ||
	     std::array<int64_t, 4>{ GET_BALANCE_COPPER(world.actor),
				     GET_BALANCE_SILVER(world.actor), GET_BALANCE_GOLD(world.actor),
				     GET_BALANCE_PLATINUM(world.actor) } != cut.bank.amount))
		return false;
	for (const auto &expected : cut.custody)
	{
		item_ownership_runtime_entry actual{};
		if (!item_ownership_runtime_lookup(expected.item_uid, &actual) ||
		    actual.item_uid != expected.item_uid ||
		    actual.root_item_uid != expected.root_item_uid ||
		    actual.parent_item_uid != expected.parent_item_uid ||
		    actual.owner.type != expected.owner.type ||
		    actual.owner.id != expected.owner.id ||
		    actual.owner.context_id != expected.owner.context_id ||
		    actual.item_revision != expected.item_revision ||
		    actual.owner_revision != expected.owner_revision ||
		    actual.vnum != expected.vnum || actual.state != expected.state)
			return false;
	}
	return true;
}

namespace
{
bool auction_current_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
template <class T> bool auction_current_vector(const std::vector<T> &values, size_t &bytes) noexcept
{
	return values.capacity() <= SIZE_MAX / sizeof(T) &&
	       auction_current_add(bytes, values.capacity() * sizeof(T));
}
bool auction_current_string(const std::string &value, size_t &bytes) noexcept
{
	const size_t capacity = value.capacity();
	return capacity <= 15 || (capacity < SIZE_MAX && auction_current_add(bytes, capacity + 1));
}
bool auction_current_command(const critical_command &command, size_t &bytes) noexcept
{
	return auction_current_vector(command.keys, bytes) &&
	       auction_current_vector(command.expected_revisions, bytes) &&
	       auction_current_vector(command.payload, bytes) &&
	       auction_current_vector(command.accounting_intent, bytes);
}
bool auction_current_items(const std::vector<player_item_snapshot> &items, size_t &bytes) noexcept
{
	if (!auction_current_vector(items, bytes))
		return false;
	for (const auto &item : items)
	{
		size_t heap = 0;
		if (!player_item_snapshot_current_heap_bytes(item, &heap) ||
		    !auction_current_add(bytes, heap))
			return false;
	}
	return true;
}
bool auction_current_context(const auction_recovery_context &context, size_t &bytes) noexcept
{
	if (!auction_current_items(context.player_before, bytes) ||
	    !auction_current_items(context.player_after, bytes) ||
	    !auction_current_items(context.selected_literals, bytes) ||
	    !auction_current_vector(context.reload, bytes) ||
	    !auction_current_vector(context.proclib, bytes))
		return false;
	for (const auto &effects : context.proclib)
		if (!auction_current_vector(effects, bytes))
			return false;
	return true;
}
}

bool auction_original_item_stage::current_private_heap_bytes(const player_item_snapshot &literal,
							     size_t *output) const noexcept
{
	return original_.current_private_heap_bytes(literal, output);
}
size_t auction_original_item_stage::current_private_heap_observer_frame_bytes() noexcept
{
	// Actual forwarding method this/literal/output and return boolean.
	return 3 * sizeof(void *) + sizeof(bool) +
	       shop_trade_original_item_stage::current_private_heap_observer_frame_bytes();
}

bool auction_native_publication_owner::current_storage_bytes(size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	if (!output || !nevent_is_game_thread() || sizeof(void *) != 8 || sizeof(size_t) != 8 ||
	    sizeof(std::string) != 32)
		return false;
	using registry = decltype(auction_native_pending_operations);
	using node = std::_Rb_tree_node<registry::value_type>;
	size_t bytes =
		sizeof(auction_native_pending_operations) + sizeof(auction_native_retained_bytes);
	if (auction_native_pending_operations.size() > SIZE_MAX / sizeof(node) ||
	    !auction_current_add(bytes, auction_native_pending_operations.size() * sizeof(node)))
		return false;
	for (auto position = auction_native_pending_operations.cbegin();
	     position != auction_native_pending_operations.cend(); ++position)
	{
		const auto &[key, owned] = *position;
		if (!owned || !auction_current_string(key, bytes))
			return false;
		// Original make_unique/transfer laws provide distinct retained bodies.
		// Authenticate that actual ownership without an allocating address set.
		for (auto earlier = auction_native_pending_operations.cbegin(); earlier != position;
		     ++earlier)
			if (earlier->second.get() == owned.get())
				return false;
		const auto &entry = *owned;
		if (!auction_current_add(bytes, sizeof(entry)) ||
		    !auction_current_command(entry.command, bytes) ||
		    !auction_current_command(entry.envelope.command, bytes) ||
		    !auction_current_vector(entry.envelope.attachment, bytes) ||
		    !auction_current_context(entry.context, bytes) ||
		    !auction_current_vector(entry.staged, bytes) ||
		    !auction_current_vector(entry.bindings, bytes))
			return false;
		for (size_t index = 0; index < entry.staged.size(); ++index)
		{
			size_t heap = 0;
			if (index >= entry.context.selected_literals.size() ||
			    !entry.staged[index].current_private_heap_bytes(
				    entry.context.selected_literals[index], &heap) ||
			    !auction_current_add(bytes, heap))
				return false;
		}
		for (const auto &binding : entry.bindings)
		{
			// This genuine allocation-free DB getter includes the inline stage.
			// Its vector already owns that inline object, including chain/scopes.
			const size_t retained = binding.retained_bytes();
			if (retained < sizeof(binding) ||
			    !auction_current_add(bytes, retained - sizeof(binding)))
				return false;
		}
	}
	*output = bytes;
	return true;
#else
	(void)output;
	return false;
#endif
}

size_t auction_native_publication_owner::current_storage_observer_frame_bytes() noexcept
{
	// Source-declared carriers only. Caller admits these before observing;
	// output excludes these frames, which are absent from retained CURRENT.
	using map = decltype(auction_native_pending_operations);
	constexpr size_t own =
		// Public wrapper output/result; owner output/bytes, position/earlier,
		// key/owned/entry/binding references, index/heap/retained/result.
		2 * sizeof(void *) + 2 * sizeof(bool) + 4 * sizeof(size_t) +
		2 * sizeof(map::const_iterator) + 5 * sizeof(void *) +
		2 * sizeof(std::vector<shop_trade_original_procedure_binding_stage>::const_iterator) +
		// Current command/context/items/vector/string/add source parameters,
		// references, heap/capacity locals, range iterators and checked result.
		13 * sizeof(void *) + 5 * sizeof(size_t) + 6 * sizeof(bool) +
		2 * sizeof(std::vector<player_item_snapshot>::const_iterator) +
		2 * sizeof(std::vector<std::vector<auction_recovery_effect>>::const_iterator) +
		// Map cbegin/cend -> tree begin/end -> iterator ctor(this,node),
		// each actual returned iterator and both map/tree size signatures.
		2 * (2 * (sizeof(void *) + sizeof(map::const_iterator)) + 2 * sizeof(void *)) +
		2 * (sizeof(void *) + sizeof(size_t)) +
		// Iterator deref/arrow -> node valptr -> buffer ptr -> buffer addr,
		// increment(this,returned-ref) and compiled increment(node,result),
		// comparison(two iterator references,returned boolean).
		2 * (4 * (2 * sizeof(void *))) + 4 * sizeof(void *) + 2 * sizeof(void *) +
		sizeof(bool) +
		// Genuine unique_ptr get -> impl ptr -> get<0> -> get_helper ->
		// Tuple_impl::_M_head -> Head_base::_M_head: each input/this+result.
		// operator* and operator bool add their own this/result signatures;
		// these three actual sibling call closures form an upper envelope.
		3 * (6 * (2 * sizeof(void *))) + 2 * sizeof(void *) + sizeof(void *) +
		sizeof(bool) +
		// vector begin/end and normal-iterator ctor, increment/deref/compare
		// -> base; vector capacity/size/index and shared_ptr bool/get used
		// by the real binding getter. No constructor or heap request here.
		2 * (sizeof(void *) + sizeof(std::vector<player_item_snapshot>::const_iterator) +
		     2 * sizeof(void *)) +
		2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(bool) +
		2 * (2 * sizeof(void *)) + 2 * (sizeof(void *) + sizeof(size_t)) +
		2 * sizeof(void *) + sizeof(size_t) + 3 * sizeof(void *) + sizeof(bool) +
		// basic_string capacity -> is_local -> data/local_data ->
		// pointer_traits pointer_to -> addressof, authentic SSO capacity 15.
		sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(bool) +
		4 * (2 * sizeof(void *)) +
		// Snapshot's genuine published snapshot_clone_observation_frames
		// plus its public row/output/bytes/result forwarding carrier values.
		13 * sizeof(void *) + 8 * sizeof(size_t) + 6 * sizeof(bool) +
		8 * (sizeof(void *) + sizeof(size_t)) + 8 * (2 * sizeof(void *)) +
		2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
		// Genuine DB binding CURRENT getter: this/chain/bytes/total/retained,
		// scope iterators/ref and chain getter's this/bytes; vector capacity.
		4 * sizeof(void *) + 7 * sizeof(size_t) + 3 * sizeof(bool) +
		2 * sizeof(std::vector<std::shared_ptr<
				   const quest_mobile_native_flat_factory_scope>>::const_iterator);
	return own + auction_original_item_stage::current_private_heap_observer_frame_bytes();
}
bool auction_native_publication_current_storage_bytes(size_t *output) noexcept
{
	return auction_native_publication_owner::current_storage_bytes(output);
}
size_t auction_native_publication_current_storage_observer_frame_bytes() noexcept
{
	return auction_native_publication_owner::current_storage_observer_frame_bytes();
}

bool auction_native_publication_owner::replay_entry_storage_bytes(
	const auction_native_pending &entry, size_t *output) noexcept
{
	if (!output)
		return false;
	size_t bytes = 0;

	if (!auction_current_add(bytes, sizeof(entry)) ||
	    !auction_current_command(entry.command, bytes) ||
	    !auction_current_command(entry.envelope.command, bytes) ||
	    !auction_current_vector(entry.envelope.attachment, bytes) ||
	    !auction_current_context(entry.context, bytes) ||
	    !auction_current_vector(entry.staged, bytes) ||
	    !auction_current_vector(entry.bindings, bytes))
		return false;
	for (size_t i = 0; i < entry.staged.size(); ++i)
	{
		size_t heap = 0;
		if (i >= entry.context.selected_literals.size() ||
		    !entry.staged[i].current_private_heap_bytes(entry.context.selected_literals[i],
								&heap) ||
		    !auction_current_add(bytes, heap))
			return false;
	}
	for (const auto &binding : entry.bindings)
	{
		const size_t retained = binding.retained_bytes();
		if (retained < sizeof(binding) ||
		    !auction_current_add(bytes, retained - sizeof(binding)))
			return false;
	}
	*output = bytes;
	return true;
}

#include "economy/auction_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "economy/auction_listing_accounting.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_money_claim_accounting.h"
namespace
{
struct auction_frozen_validator_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer;
	const economic_frozen_intent *intent = nullptr;
	bool denied = false;
	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &b = *static_cast<auction_frozen_validator_budget *>(opaque);
		if (b.denied || !b.reserve || !b.reserve(amount, b.context))
		{
			b.denied = true;
			return false;
		}
		return true;
	}
	bool prefix(size_t &bytes) noexcept
	{
		bytes = outer;
		// All actual original reusable typed outputs, not guessed v1 payload
		// or maximum intent bytes. They coexist through the five-way OR.
		const size_t frames =
			sizeof(economic_frozen_intent) + sizeof(auction_command_payload) +
			sizeof(auction_bid_accounting_listing) +
			sizeof(auction_bid_accounting_accounts) +
			sizeof(auction_settlement_listing) + sizeof(auction_settlement_accounts) +
			sizeof(auction_item_claim_state) + 3 * sizeof(economic_account_key) +
			8 * sizeof(void *) + 3 * sizeof(size_t) + 5 * sizeof(bool) + sizeof(*this);
		if (!auction_current_add(bytes, frames) ||
		    (intent && !auction_current_add(bytes, intent->admission.facts.capacity())))
		{
			denied = true;
			return false;
		}
		return !denied;
	}
};
}
bool auction_repository_frozen_accounting_valid_bounded(const critical_command &command,
							bool (*reserve)(size_t, void *) noexcept,
							void *context, size_t outer) noexcept
{
	auction_frozen_validator_budget budget{ reserve, context, outer };
	size_t nested = 0;
	if (!budget.prefix(nested) || !auction_frozen_validator_budget::forward(nested, &budget))
		return false;
	try
	{
		economic_frozen_intent intent;
		auction_command_payload payload{};
		auction_bid_accounting_listing bid;
		auction_bid_accounting_accounts bid_accounts;
		auction_settlement_listing settlement;
		auction_settlement_accounts settlement_accounts;
		auction_item_claim_state claim;
		economic_account_key wallet, bank, claim_account;
		budget.intent = &intent;
		// Complete original five-decoder short circuit and returned semantic
		// failures survive. Real refusal remains sticky through later OR arms.
		return (budget.prefix(nested) &&
			auction_bid_accounting_decode_bounded(
				command, &intent, &payload, &bid, &bid_accounts,
				auction_frozen_validator_budget::forward, &budget,
				nested) == economic_accounting_error::ok) ||
		       (budget.prefix(nested) &&
			auction_settlement_accounting_decode_bounded(
				command, &intent, &payload, &settlement, &settlement_accounts,
				auction_frozen_validator_budget::forward, &budget,
				nested) == economic_accounting_error::ok) ||
		       (budget.prefix(nested) &&
			auction_listing_accounting_decode_bounded(
				command, &intent, &payload, &wallet, &bank,
				auction_frozen_validator_budget::forward, &budget,
				nested) == economic_accounting_error::ok) ||
		       (budget.prefix(nested) &&
			auction_item_claim_accounting_decode_bounded(
				command, &intent, &payload, &claim, &wallet, &bank,
				auction_frozen_validator_budget::forward, &budget,
				nested) == economic_accounting_error::ok) ||
		       (budget.prefix(nested) &&
			auction_money_claim_accounting_decode_bounded(
				command, &intent, &payload, &wallet, &bank, &claim_account,
				auction_frozen_validator_budget::forward, &budget,
				nested) == economic_accounting_error::ok);
	}
	catch (...)
	{
		return false;
	}
}

// Appended privately in auction_native_publication.c, never selected here.
#include <algorithm>
#include <array>
#include <initializer_list>
#include <set>
#include <type_traits>

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) && \
	!defined(_GLIBCXX_PARALLEL) && __cplusplus >= 202002L
namespace
{
using auction_forest_rows = std::vector<player_item_snapshot>;
using auction_forest_ranges = std::vector<std::pair<size_t, size_t>>;
using auction_forest_set = std::set<uint64_t>;
using auction_forest_node = std::_Rb_tree_node<uint64_t>;
constexpr size_t auction_forest_P = sizeof(void *);
constexpr size_t auction_forest_N = sizeof(size_t);
constexpr size_t auction_forest_B = sizeof(bool);

bool auction_forest_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}

// Sum of genuine named source scopes. Sequential scopes may conservatively
// coexist in this inventory; no multiplicative row/container census is used.
// Inline workspace objects and live recursive set cleanup are separate below.
constexpr size_t auction_forest_caller_frames =
	// add(ref,amount), prepare(callback,context,outer,output; initial,frames,
	// copy,vector,observation), forward(amount,opaque; w reference).
	(auction_forest_P + auction_forest_N + auction_forest_B) +
	(3 * auction_forest_P + 6 * auction_forest_N + auction_forest_B) +
	(2 * auction_forest_P + auction_forest_N + auction_forest_B) +
	// current(this,total,extra,prospective; four-row initializer list/backing
	// pointer array/begin/end/value/heap, rowheap,nodes), peak(this,extra,
	// prospective;total), growth(this,v,count,request;increment,capacity).
	(9 * auction_forest_P + 6 * auction_forest_N + auction_forest_B +
	 sizeof(std::initializer_list<const auction_forest_rows *>)) +
	(auction_forest_P + 3 * auction_forest_N + auction_forest_B) +
	(3 * auction_forest_P + 3 * auction_forest_N + auction_forest_B) +
	// push(this,v,value;request), uid(this,value), fresh copy(this,span,target;
	// request, hidden range ref/iterators/item/heap), encode(this,source,out;
	// total), append_row(this,target;request).
	(2 * auction_forest_P + sizeof(auction_forest_ranges::value_type) + auction_forest_N +
	 auction_forest_B) +
	(auction_forest_P + sizeof(uint64_t) + auction_forest_B) +
	(4 * auction_forest_P + sizeof(std::span<const player_item_snapshot>) +
	 2 * sizeof(std::span<const player_item_snapshot>::iterator) + 2 * auction_forest_N +
	 auction_forest_B) +
	(3 * auction_forest_P + auction_forest_N + auction_forest_B) +
	(2 * auction_forest_P + auction_forest_N + auction_forest_B) +
	// insert_tree(this,position;request), both real row range scopes, heap
	// locals. assignment_inline_capacity: two true initializer lists/backing
	// arrays/range endpoints/text/capacity plus description range scopes.
	(5 * auction_forest_P + 4 * sizeof(auction_forest_rows::const_iterator) +
	 4 * auction_forest_N + auction_forest_B) +
	(19 * auction_forest_P + 2 * auction_forest_N + auction_forest_B +
	 2 * sizeof(std::initializer_list<const std::string *>) +
	 2 * sizeof(std::vector<player_item_extra_description_snapshot>::const_iterator)) +
	// selected_ranges arguments/profile scalar, full ancestry array,
	// depth/i/slot and original row reference.
	(5 * auction_forest_P + sizeof(std::span<const player_item_snapshot>) +
	 5 * auction_forest_N + auction_forest_B +
	 sizeof(std::array<size_t, PLAYER_SNAPSHOT_MAX_DEPTH>)) +
	// same_selected_values arguments/profile scalar; observed range/row,
	// found iterator; actual parent IDs/first/second/request; original two
	// singleton vector objects, two backing rows and initializer lists.
	(7 * auction_forest_P + 2 * sizeof(std::span<const player_item_snapshot>) +
	 3 * sizeof(std::span<const player_item_snapshot>::iterator) + 5 * auction_forest_N +
	 2 * sizeof(uint64_t) + auction_forest_B + 2 * sizeof(auction_forest_rows) +
	 2 * sizeof(player_item_snapshot) +
	 2 * sizeof(std::initializer_list<player_item_snapshot>)) +
	// selected public wrapper arguments/frame/nested and boolean result.
	(3 * auction_forest_P + sizeof(std::span<const player_item_snapshot>) +
	 3 * auction_forest_N + auction_forest_B) +
	// expected public body arguments/flags/frame/nested/sorting, original
	// before range, listing ranges, claim structured binding/root, head/same/
	// i/position/count, shifted-row range and per-tree row loop.
	(13 * auction_forest_P + 2 * sizeof(std::span<const player_item_snapshot>) +
	 11 * auction_forest_N + 4 * auction_forest_B + sizeof(uint32_t) +
	 4 * sizeof(auction_forest_rows::iterator) +
	 4 * sizeof(auction_forest_ranges::const_iterator)) +
	// sort profile body count/output/result, six genuine constexpr automatic
	// scalar objects, logarithm/n/depth/leaf and max initializer-list backing.
	(auction_forest_P + 15 * auction_forest_N + auction_forest_B +
	 sizeof(std::initializer_list<size_t>)) +
	// find_if/__find_if/_Iter_pred actual capture/ref scopes, unrolled
	// trip_count and first/last/next/result; lambda this/x/capture/result.
	(9 * sizeof(std::span<const player_item_snapshot>::iterator) + 2 * sizeof(std::ptrdiff_t) +
	 11 * auction_forest_P + 4 * auction_forest_B) +
	// Full byte equality/equal/aux/aux1/__equal<true>/__memcmp declaration.
	(20 * auction_forest_P + 4 * sizeof(std::ptrdiff_t) + 6 * auction_forest_B + sizeof(int)) +
	// Actual scalar max/min (references,returned ref,bool), array/pair/span
	// index/data/size and singleton-list constructor/accessor declarations.
	(24 * auction_forest_P + 8 * auction_forest_N + 6 * auction_forest_B +
	 2 * sizeof(std::allocator<player_item_snapshot>)) +
	// Actual lower enum results, three pure profile-return scalars, plus the
	// list-CURRENT wrapper own value/output/bytes/policy/result around the
	// already exported complete per-row CURRENT observation leaf.
	(3 * sizeof(player_snapshot_codec_result) + 4 * auction_forest_P + 5 * auction_forest_N +
	 3 * auction_forest_B);

// stl_set.h:insert(const value_type&) -> _M_insert_unique (not map::emplace).
// Exact typed return/position pairs and one actual _Alloc_node are retained.
// Lookup is iterative. Native RB rebalance/decrement implementation is outside
// the source-declared library boundary, as in the shared owner contracts.
constexpr size_t auction_forest_set_frames =
	2 * auction_forest_P + sizeof(std::pair<auction_forest_set::iterator, bool>) +
	sizeof(std::pair<std::_Rb_tree_iterator<uint64_t>, bool>) +
	// _M_insert_unique/get_insert_unique_pos and actual __res/__x/__y/__comp/__j.
	6 * auction_forest_P +
	2 * sizeof(std::pair<std::_Rb_tree_node_base *, std::_Rb_tree_node_base *>) +
	sizeof(std::_Rb_tree_iterator<uint64_t>) + auction_forest_B + auction_forest_P +
	2 * auction_forest_P +
	// begin/end/key/identity/less, iterator construction/decrement/equality,
	// pair construction/forwarding and actual node's aligned-buffer address.
	4 * auction_forest_P + 11 * auction_forest_P + 3 * auction_forest_P + 3 * auction_forest_P +
	auction_forest_B + 6 * auction_forest_P + 2 * auction_forest_P + auction_forest_B +
	7 * auction_forest_P +
	// _M_insert_ arguments/insert_left/__z/result, node generator and
	// create/construct/get-node, placement-new and construct_at forwarding.
	7 * auction_forest_P + auction_forest_B + 3 * auction_forest_P + 4 * auction_forest_P +
	3 * auction_forest_P + 2 * auction_forest_P + 3 * auction_forest_P + auction_forest_N +
	3 * auction_forest_P + 3 * auction_forest_P +
	// Node allocator traits/allocator/new_allocator allocate/deallocate and
	// real operator new/delete size/pointer; no allocator metadata estimate.
	3 * (2 * auction_forest_P + auction_forest_N) + 3 * auction_forest_P + auction_forest_N +
	auction_forest_P + auction_forest_N + 4 * (2 * auction_forest_P + auction_forest_N) +
	auction_forest_P + auction_forest_N +
	// _M_get_Node_allocator, node value/destroy/drop/put chain, allocator
	// destroy/destroy_at, set/tree/header/default-comparator constructors.
	2 * auction_forest_P + 11 * auction_forest_P + 8 * auction_forest_P + 3 * auction_forest_P +
	auction_forest_B +
	// _Rb_tree_insert_and_rebalance declaration's bool +3 node/header refs.
	3 * auction_forest_P + auction_forest_B;

// Fitting row insertion reaches generated row and description COPY ASSIGNMENT,
// not merely fresh constructors. Shared copy/vector profiles own their true
// allocator/copy/move/destroy leaves; these are the missing assignment bodies.
constexpr size_t auction_forest_assignment_frames =
	2 * auction_forest_P + 2 * auction_forest_P +
	// Four row strings + two description strings: operator=/assign/_M_assign
	// this/source, __rsize/__capacity/__new_capacity and actual __tmp.
	6 * (7 * auction_forest_P + 3 * auction_forest_N) +
	// dynamic-affect/extra-description/spell vectors: operator= this/source,
	// __xlen/__tmp/addressof; std::allocator propagation branch is inactive.
	3 * (5 * auction_forest_P + auction_forest_N + 2 * auction_forest_B) +
	// Nontrivial copy_m<false> iterators, difference __n, element references;
	// generated assignments for actual arrays and trivial dynamic-affect row.
	6 * auction_forest_P + sizeof(std::ptrdiff_t) + 18 * auction_forest_P;

// Settled GNU13 default uint64 sort profile adapted to this actual roots vector.
// The full original sort is kept. No unique/erase operation exists here.
bool auction_forest_sort_frames(size_t count, size_t *output) noexcept
{
	using iterator = std::vector<uint64_t>::iterator;
	using difference = std::vector<uint64_t>::difference_type;
	using compare = __gnu_cxx::__ops::_Iter_less_iter;
	using value_compare = __gnu_cxx::__ops::_Val_less_iter;
	using iter_value_compare = __gnu_cxx::__ops::_Iter_less_val;
	constexpr size_t setup = 4 * sizeof(iterator) + 2 * sizeof(compare) + 2 * auction_forest_P +
				 3 * sizeof(difference) + sizeof(int) +
				 8 * (auction_forest_P + sizeof(iterator)) + 4 * auction_forest_B;
	constexpr size_t recursive = 3 * sizeof(iterator) + sizeof(difference) + sizeof(compare);
	constexpr size_t partition = 12 * sizeof(iterator) + 3 * sizeof(compare) +
				     2 * auction_forest_B + 7 * auction_forest_P + sizeof(uint64_t);
	constexpr size_t insertion = 10 * sizeof(iterator) + 2 * sizeof(compare) +
				     2 * sizeof(value_compare) + sizeof(iter_value_compare) +
				     3 * sizeof(uint64_t) + 2 * sizeof(difference) +
				     12 * auction_forest_P + 3 * auction_forest_B;
	constexpr size_t heap = 12 * sizeof(iterator) + 14 * sizeof(difference) +
				5 * sizeof(compare) + 2 * sizeof(value_compare) +
				2 * sizeof(iter_value_compare) + 4 * sizeof(uint64_t) +
				18 * auction_forest_P + 3 * auction_forest_B;
	constexpr size_t comparator =
		2 * sizeof(iterator) + 2 * auction_forest_P + 3 * auction_forest_B;
	size_t logarithm = 0;
	for (size_t n = count; n > 1; n >>= 1)
		++logarithm;
	const size_t depth = count <= 16 ? 1 : std::min(logarithm * 2 + 1, count - 16 + 1);
	const size_t leaf = std::max({ partition, insertion, heap });
	if (!output || depth > (SIZE_MAX - setup - leaf - comparator) / recursive)
		return false;
	*output = setup + depth * recursive + leaf + comparator;
	return true;
}

struct auction_forest_workspace
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	auction_forest_rows a = {}, b = {}, c = {}, d = {};
	std::vector<uint8_t> x = {}, y = {};
	auction_forest_ranges ranges = {};
	std::vector<uint64_t> roots = {};
	auction_forest_set identities = {};
	player_item_snapshot row{};

	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &w = *static_cast<auction_forest_workspace *>(opaque);
		return w.reserve && w.reserve(amount, w.context);
	}
	bool current(size_t &total, size_t extra = 0, size_t prospective_nodes = 0) const noexcept
	{
		total = outer;
		if (!auction_forest_add(total, sizeof(*this)) ||
		    !auction_forest_add(total, frames) || !auction_forest_add(total, extra))
			return false;
		for (const auto *rows : { &a, &b, &c, &d })
		{
			size_t heap = 0;
			if (!player_item_snapshot_list_current_heap_bytes(*rows, &heap) ||
			    !auction_forest_add(total, heap))
				return false;
		}
		size_t heap = 0;
		if (!player_item_snapshot_current_heap_bytes(row, &heap) ||
		    !auction_forest_add(total, heap) || !auction_forest_add(total, x.capacity()) ||
		    !auction_forest_add(total, y.capacity()) ||
		    ranges.capacity() > SIZE_MAX / sizeof(auction_forest_ranges::value_type) ||
		    !auction_forest_add(total, ranges.capacity() *
						       sizeof(auction_forest_ranges::value_type)) ||
		    roots.capacity() > SIZE_MAX / sizeof(uint64_t) ||
		    !auction_forest_add(total, roots.capacity() * sizeof(uint64_t)) ||
		    identities.size() > SIZE_MAX / sizeof(auction_forest_node) ||
		    !auction_forest_add(total, identities.size() * sizeof(auction_forest_node)))
			return false;
		// Actual _M_erase recursively visits right subtrees, while the left is
		// iterative. At most n+1 genuine (this,__x,__y) source frames coexist,
		// including the null leaf. Admission prices this cleanup BEFORE insert.
		size_t nodes = identities.size();
		if (!auction_forest_add(nodes, prospective_nodes) || nodes == SIZE_MAX ||
		    nodes + 1 > SIZE_MAX / (3 * sizeof(void *)))
			return false;
		return auction_forest_add(total, (nodes + 1) * 3 * sizeof(void *));
	}
	bool peak(size_t extra = 0, size_t prospective_nodes = 0) const noexcept
	{
		size_t total = 0;
		return current(total, extra, prospective_nodes) && reserve &&
		       reserve(total, context);
	}
	template <class T>
	bool growth(const std::vector<T> &v, size_t count, size_t &request) const noexcept
	{
		request = 0;
		if (count > v.max_size() - v.size())
			return false;
		if (count <= v.capacity() - v.size())
			return true;
		const size_t increment = std::max(v.size(), count);
		const size_t capacity = increment > v.max_size() - v.size() ? v.max_size() :
									      v.size() + increment;
		if (capacity > SIZE_MAX / sizeof(T))
			return false;
		request = capacity * sizeof(T);
		return true;
	}
	template <class T> bool push(std::vector<T> &v, T value)
	{
		size_t request = 0;
		if (!growth(v, 1, request) || !peak(request))
			return false;
		v.push_back(std::move(value));
		return true;
	}
	bool uid(uint64_t value)
	{
		// Duplicate insert does not construct/replace a node. Prospective
		// admission permits the one real node on the successful new-key path.
		return peak(sizeof(auction_forest_node), 1) && identities.insert(value).second;
	}
	bool copy(std::span<const player_item_snapshot> source, auction_forest_rows &target)
	{
		// Only the original fresh initial vector copies select this helper.
		size_t request = 0;
		if (!target.empty() || target.capacity() ||
		    source.size() > SIZE_MAX / sizeof(player_item_snapshot))
			return false;
		request = source.size() * sizeof(player_item_snapshot);
		for (const auto &item : source)
		{
			size_t heap = 0;
			if (!player_item_snapshot_fresh_copy_request_bytes(item, &heap) ||
			    !auction_forest_add(request, heap))
				return false;
		}
		if (!peak(request))
			return false;
		target.assign(source.begin(), source.end());
		return true;
	}
	bool encode(const auction_forest_rows &source, std::vector<uint8_t> &output) noexcept
	{
		size_t total = 0;
		return current(total) && player_item_snapshot_list_encode_bounded(
						 source, &output, forward, this, total) ==
						 player_snapshot_codec_result::ok;
	}
	bool append_row(auction_forest_rows &target)
	{
		size_t request = 0;
		if (!growth(target, 1, request) || !peak(request))
			return false;
		target.push_back(std::move(row));
		return true;
	}
	bool insert_tree(size_t position)
	{
		size_t request = 0;
		if (!growth(b, c.size(), request))
			return false;
		// Actual fitting branch can assign into live nested descriptions and
		// strings. _M_create(req,oldcap) is <=req+2*oldcap; vector copy assign
		// allocates the source size when capacity is insufficient. Each old
		// nested buffer has one owner after noexcept relocation, and each
		// source row is copied once. This prices the complete fitting branch,
		// retaining old buffers in CURRENT until their admitted disposal.
		if (c.size() <= b.capacity() - b.size())
			for (const auto &item : b)
			{
				size_t heap = 0;
				if (!player_item_snapshot_current_heap_bytes(item, &heap) ||
				    heap > SIZE_MAX / 2 || !auction_forest_add(request, 2 * heap) ||
				    !assignment_inline_capacity(item, request))
					return false;
			}
		for (const auto &item : c)
		{
			size_t heap = 0;
			if (!player_item_snapshot_fresh_copy_request_bytes(item, &heap) ||
			    !auction_forest_add(request, heap))
				return false;
		}
		if (!peak(request))
			return false;
		b.insert(b.begin() + position, c.begin(), c.end());
		return true;
	}
	static bool assignment_inline_capacity(const player_item_snapshot &value,
					       size_t &bytes) noexcept
	{
		// CURRENT heap excludes SSO capacity. An assignment from 16..29 bytes
		// into SSO can nevertheless request 31 bytes via _M_create(n,15).
		// Include the actual inline capacities as prospective growth inputs.
		for (const auto *text : { &value.name, &value.short_description, &value.description,
					  &value.action_description })
		{
			const size_t capacity = text->capacity();
			if (capacity <= 15 && !auction_forest_add(bytes, 2 * (capacity + 1)))
				return false;
		}
		for (const auto &description : value.extra_descriptions)
			for (const auto *text : { &description.keyword, &description.description })
			{
				const size_t capacity = text->capacity();
				if (capacity <= 15 &&
				    !auction_forest_add(bytes, 2 * (capacity + 1)))
					return false;
			}
		return true;
	}
};

bool auction_forest_prepare(bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
			    size_t *output) noexcept
{
	size_t initial = outer, frames = auction_forest_caller_frames;
	// This executes BEFORE construction of the workspace's genuine strings,
	// vectors, set or row. No CURRENT/heap scan precedes the first admission.
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || !reserve || !output ||
	    !auction_forest_add(initial, sizeof(auction_forest_workspace)) ||
	    !auction_forest_add(initial, frames) ||
	    !auction_forest_add(initial, 3 * sizeof(size_t)) || !reserve(initial, context))
		return false;
	const size_t copy = player_item_snapshot_copy_frame_bytes();
	const size_t vector = player_item_snapshot_vector_operation_frame_bytes();
	const size_t observation = player_item_snapshot_current_heap_observer_frame_bytes();
	if (!copy || !vector || !observation || !auction_forest_add(frames, copy) ||
	    !auction_forest_add(frames, vector) || !auction_forest_add(frames, observation) ||
	    !auction_forest_add(frames, auction_forest_assignment_frames) ||
	    !auction_forest_add(frames, auction_forest_set_frames))
		return false;
	initial = outer;
	if (!auction_forest_add(initial, sizeof(auction_forest_workspace)) ||
	    !auction_forest_add(initial, frames) || !reserve(initial, context))
		return false;
	*output = frames;
	return true;
}

bool auction_forest_selected_ranges(const auction_command_payload &payload,
				    std::span<const player_item_snapshot> selected,
				    auction_forest_ranges *out,
				    bool (*reserve)(size_t, void *) noexcept, void *context,
				    size_t outer)
{
	if (!out || selected.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	size_t frames = 0;
	if (!auction_forest_prepare(reserve, context, outer, &frames))
		return false;
	auction_forest_workspace w{ reserve, context, outer, frames };
	if (!w.copy(selected, w.a) || !w.encode(w.a, w.x))
		return false;
	std::array<size_t, PLAYER_SNAPSHOT_MAX_DEPTH> ancestors{};
	size_t depth = 0;
	for (size_t i = 0; i < selected.size(); ++i)
	{
		const auto &row = selected[i];
		if (!row.object_uid || row.object_uid == UINT64_MAX || row.equipment_slot ||
		    row.string_mask != 15 || !w.uid(row.object_uid))
			return false;
		if (row.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			const size_t slot = w.ranges.size();
			if (slot >= payload.item_count ||
			    row.object_uid != payload.items[slot].item_uid ||
			    row.vnum != payload.items[slot].vnum)
				return false;
			if (!w.ranges.empty())
				w.ranges.back().second = i;
			if (!w.push(w.ranges, std::pair<size_t, size_t>{ i, selected.size() }))
				return false;
			depth = 1;
			ancestors[0] = i;
		}
		else
		{
			if (row.parent_index < 0 || static_cast<size_t>(row.parent_index) >= i)
				return false;
			while (depth &&
			       ancestors[depth - 1] != static_cast<size_t>(row.parent_index))
				--depth;
			if (!depth || depth == ancestors.size())
				return false;
			ancestors[depth++] = i;
		}
	}
	if (w.ranges.size() != payload.item_count || !w.peak())
		return false;
	*out = std::move(w.ranges);
	return true;
}

bool auction_forest_same_selected_values(std::span<const player_item_snapshot> observed,
					 std::span<const player_item_snapshot> selected,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer)
{
	if (observed.size() != selected.size())
		return false;
	size_t frames = 0;
	if (!auction_forest_prepare(reserve, context, outer, &frames))
		return false;
	auction_forest_workspace w{ reserve, context, outer, frames };
	for (const auto &row : observed)
	{
		auto found = std::find_if(selected.begin(), selected.end(), [&](const auto &x)
					  { return x.object_uid == row.object_uid; });
		if (found == selected.end())
			return false;
		const uint64_t a_parent =
			row.parent_index < 0 ? 0 : observed[row.parent_index].object_uid;
		const uint64_t b_parent =
			found->parent_index < 0 ? 0 : selected[found->parent_index].object_uid;
		if (a_parent != b_parent)
			return false;
		size_t first = 0, second = 0, request = 2 * sizeof(player_item_snapshot);
		// The original TWO singleton initializer lists each copy a backing row
		// and then a vector row. Both backing objects survive the original full
		// declaration expression. Admit all four genuine nested fresh copies.
		if (!player_item_snapshot_fresh_copy_request_bytes(row, &first) ||
		    !player_item_snapshot_fresh_copy_request_bytes(*found, &second) ||
		    first > SIZE_MAX / 2 || second > SIZE_MAX / 2 ||
		    !auction_forest_add(request, 2 * first) ||
		    !auction_forest_add(request, 2 * second) || !w.peak(request))
			return false;
		{
			std::vector<player_item_snapshot> a{ row }, b{ *found };
			// Equal-allocator moves are allocation free. Both source vectors
			// are already priced by request until the following real census.
			w.a = std::move(a);
			w.b = std::move(b);
		}
		w.a[0].parent_index = w.b[0].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		if (!w.encode(w.a, w.x) || !w.encode(w.b, w.y) || w.x != w.y)
			return false;
		if (!w.peak())
			return false;
		// Original singleton rows/wires die each iteration. This preserves that
		// lifetime; no accumulating capacity substitutes for fresh copies.
		auction_forest_rows{}.swap(w.a);
		auction_forest_rows{}.swap(w.b);
		std::vector<uint8_t>{}.swap(w.x);
		std::vector<uint8_t>{}.swap(w.y);
	}
	return true;
}
} // namespace
#endif

bool auction_native_selected_forest_valid_bounded(const auction_command_payload &payload,
						  std::span<const player_item_snapshot> selected,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) && \
	!defined(_GLIBCXX_PARALLEL) && __cplusplus >= 202002L
	try
	{
		size_t frames = 0;
		if (!auction_forest_prepare(reserve, context, outer, &frames))
			return false;
		auction_forest_workspace w{ reserve, context, outer, frames };
		size_t nested = 0;
		return w.current(nested) &&
		       auction_forest_selected_ranges(payload, selected, &w.ranges,
						      auction_forest_workspace::forward, &w,
						      nested);
	}
	catch (...)
	{
		return false;
	}
#else
	(void)payload;
	(void)selected;
	(void)reserve;
	(void)context;
	(void)outer;
	return false;
#endif
}

bool auction_native_expected_player_forest_bounded(
	const auction_command_payload &payload,
	std::span<const player_item_snapshot> original_before,
	std::span<const player_item_snapshot> original_selected, bool rejected,
	uint32_t original_actor_level, std::vector<player_item_snapshot> *out,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) && \
	!defined(_GLIBCXX_PARALLEL) && __cplusplus >= 202002L
	try
	{
		if (!out || original_before.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
		    payload.item_count > AUCTION_COMMAND_MAX_ITEMS)
			return false;
		size_t frames = 0;
		if (!auction_forest_prepare(reserve, context, outer, &frames))
			return false;
		auction_forest_workspace w{ reserve, context, outer, frames };
		if (!w.copy(original_before, w.a) || !w.encode(w.a, w.x))
			return false;
		for (const auto &row : w.a)
			if (!row.object_uid || !w.uid(row.object_uid))
				return false;
		const bool listing = payload.action == auction_action::list,
			   claiming = payload.action == auction_action::claim_item;
		if ((!listing && !claiming) || rejected)
		{
			if (!w.peak())
				return false;
			*out = std::move(w.a);
			return true;
		}
		if (!payload.actor_pid || !original_actor_level || !payload.item_count)
			return false;
		size_t nested = 0;
		if (!w.current(nested) ||
		    !auction_forest_selected_ranges(payload, original_selected, &w.ranges,
						    auction_forest_workspace::forward, &w, nested))
			return false;
		if (listing)
		{
			for (const auto &range : w.ranges)
				if (!w.push(w.roots, original_selected[range.first].object_uid))
					return false;
			size_t sorting = 0;
			if (!auction_forest_sort_frames(w.roots.size(), &sorting) ||
			    !w.peak(sorting))
				return false;
			std::sort(w.roots.begin(), w.roots.end());
			if (!w.current(nested) ||
			    player_item_snapshot_extract_forest_bounded(
				    w.a, w.roots, &w.d, &w.b, auction_forest_workspace::forward, &w,
				    nested) != player_snapshot_codec_result::ok ||
			    !w.current(nested) ||
			    !auction_forest_same_selected_values(w.d, original_selected,
								 auction_forest_workspace::forward,
								 &w, nested) ||
			    !w.peak())
				return false;
			*out = std::move(w.b);
			return true;
		}
		if (w.a.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - original_selected.size() ||
		    !w.peak())
			return false;
		w.b = std::move(w.a);
		for (const auto &[begin, end] : w.ranges)
		{
			const auto &root = original_selected[begin];
			size_t head = w.b.size(), same = w.b.size();
			for (size_t i = 0; i < w.b.size(); ++i)
				if (w.b[i].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
				    !w.b[i].equipment_slot)
				{
					if (head == w.b.size())
						head = i;
					if (w.b[i].vnum == root.vnum)
					{
						same = i;
						break;
					}
				}
			const size_t position = same != w.b.size() ? same : head;
			const size_t count = end - begin;
			for (auto &row : w.b)
				if (row.parent_index >= 0 &&
				    static_cast<size_t>(row.parent_index) >= position)
					row.parent_index += static_cast<int32_t>(count);
			if (count > w.c.max_size() ||
			    count > SIZE_MAX / sizeof(player_item_snapshot) ||
			    !w.peak(count * sizeof(player_item_snapshot)))
				return false;
			w.c.reserve(count);
			for (size_t i = begin; i < end; ++i)
			{
				if (!w.current(nested) ||
				    player_item_snapshot_clone_bounded(
					    original_selected[i], &w.row,
					    auction_forest_workspace::forward, &w,
					    nested) != player_snapshot_codec_result::ok ||
				    !w.uid(w.row.object_uid))
					return false;
				if (w.row.parent_index >= 0)
					w.row.parent_index = static_cast<int32_t>(position) +
							     w.row.parent_index -
							     static_cast<int32_t>(begin);
				if (i == begin && !w.row.generated_key &&
				    original_actor_level < 57 && payload.actor_pid < 10000000)
					w.row.generated_key = 1;
				if (!w.append_row(w.c))
					return false;
			}
			if (!w.insert_tree(position) || !w.peak())
				return false;
			// The original private tree dies after each range insertion.
			auction_forest_rows{}.swap(w.c);
		}
		if (!w.encode(w.b, w.x) || !w.peak())
			return false;
		*out = std::move(w.b);
		return true;
	}
	catch (...)
	{
		return false;
	}
#else
	(void)payload;
	(void)original_before;
	(void)original_selected;
	(void)rejected;
	(void)original_actor_level;
	(void)out;
	(void)reserve;
	(void)context;
	(void)outer;
	return false;
#endif
}

#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)
size_t auction_native_publication_replay_source_frame_bytes() noexcept
{
	using map = decltype(auction_native_pending_operations);
	using iterator = map::iterator;
	using const_iterator = map::const_iterator;
	using pointer = std::unique_ptr<auction_native_pending>;
	using node_pointer = std::_Rb_tree_node<map::value_type> *;
	// Real restore/replayed parameters and locals: budget/work inline owners
	// are charged separately. This closes nested prefix/peak/forward,
	// equal/copy/entry_heap, key construction and receipt-copy call carriers.
	constexpr size_t owning_calls =
		// v9 checkpoint_query/checkpoint_frames/checkpoint_peak are genuine
		// provider locals, retained across its exact preflight/checkpoint cut.
		13 * sizeof(void *) + 11 * sizeof(size_t) + 5 * sizeof(bool) + sizeof(iterator) +
		sizeof(std::pair<iterator, bool>) + 3 * sizeof(void *) + sizeof(bool) +
		4 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(bool) + 2 * sizeof(void *) +
		4 * sizeof(size_t) + 3 * sizeof(bool) + 4 * sizeof(void *) + sizeof(size_t) +
		sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
	// Binary operation key: public helper id/ref + returned string, genuine
	// basic_string(data,16) ctor allocator/guard, forward _M_construct's
	// first/last/distance/guard, _M_create's capacity/old-capacity/page values,
	// allocator allocation and traits copy/memcpy parameters and results.
	// Heap requests (17 bytes) are admitted separately before this path.
	constexpr size_t string_construct =
		sizeof(void *) + sizeof(std::string) + 5 * sizeof(void *) + sizeof(size_t) +
		sizeof(std::allocator<char>) + 5 * sizeof(void *) + 4 * sizeof(size_t) +
		4 * sizeof(void *) + 5 * sizeof(size_t) + 8 * sizeof(void *) + 5 * sizeof(size_t) +
		7 * sizeof(void *) + 2 * sizeof(size_t);
	// make_unique: returned pointer plus forwarding/new location; actual
	// unique_ptr/impl/tuple/head constructors and move assignment/reset.
	// The pending object and all its inline DTO members occupy the genuine
	// heap block, not another source frame. No hidden pending DTO is copied.
	constexpr size_t unique_construct_move =
		sizeof(pointer) + sizeof(void *) + sizeof(size_t) + 13 * sizeof(void *) +
		3 * sizeof(pointer) + 12 * sizeof(void *) + sizeof(bool) + 6 * (2 * sizeof(void *));
	// find -> tree find -> lower_bound, actual node/end/result carriers;
	// _S_key/valptr/buffer access, comparator refs and string compare's
	// lengths/rlen/result plus traits::compare/memcmp signatures. The loops
	// are iterative; this profile does not multiply by registry cardinality.
	constexpr size_t lookup = 7 * sizeof(void *) + sizeof(iterator) + sizeof(const_iterator) +
				  3 * sizeof(node_pointer) + 4 * (2 * sizeof(void *)) +
				  5 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(int) +
				  4 * sizeof(void *) + sizeof(size_t) + sizeof(int);
	// map::emplace -> _M_emplace_unique -> _Auto_node: real forwarding
	// refs, two-pointer _Auto_node and result pairs; _M_create_node,
	// _M_construct_node, get/traits allocate, pair string-copy/unique-move.
	// The actual node and its copied key heap are priced before emplace.
	constexpr size_t insert =
		7 * sizeof(void *) + sizeof(std::pair<iterator, bool>) + 2 * sizeof(void *) +
		2 * sizeof(node_pointer) + 4 * sizeof(void *) +
		sizeof(std::pair<node_pointer, node_pointer>) + 10 * sizeof(void *) +
		sizeof(size_t) + 8 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(map::value_type) +
		5 * sizeof(void *) +
		// _M_get_insert_unique_pos x/y/j/comp and returned node pair;
		// _Auto_node::_M_insert and _M_insert_node x/p/z/insert_left.
		5 * sizeof(node_pointer) + sizeof(iterator) + 2 * sizeof(bool) +
		sizeof(std::pair<node_pointer, node_pointer>) + 5 * sizeof(void *) +
		sizeof(iterator) + sizeof(bool) +
		// Compiled rebalance call arguments only. Emitted/native stack
		// qualification remains a separate major-plan check.
		4 * sizeof(void *) + sizeof(bool);
	// Rollback is erase(iterator), never whole-map erase: const conversion,
	// map/tree erase result, _M_erase_aux and compiled rebalance arguments,
	// drop/destroy node/traits destructor and put-node deallocation.
	constexpr size_t rollback = 5 * sizeof(void *) + 3 * sizeof(iterator) +
				    2 * sizeof(const_iterator) + 4 * sizeof(void *) +
				    sizeof(node_pointer) + 9 * sizeof(void *) + 4 * sizeof(size_t) +
				    4 * (2 * sizeof(void *));
	// The real pending destructor releases the two command vectors and
	// envelope, three forests, nested proclib and stages/bindings. Actual
	// retained heap stays charged by CURRENT/entry until destruction.
	// Destructor closures are sequential; these typed vector/row/string
	// scopes conservatively sum their real call-path carriers, not their
	// already owned element heaps or a fabricated envelope baseline.
	constexpr size_t cleanup =
		5 * sizeof(void *) + sizeof(pointer) +
		2 * (sizeof(void *) + sizeof(std::vector<uint8_t>)) + 14 * sizeof(void *) +
		6 * sizeof(size_t) + 2 * sizeof(std::vector<player_item_snapshot>::iterator) +
		2 * sizeof(std::vector<std::vector<auction_recovery_effect>>::iterator) +
		4 * sizeof(void *) + 3 * sizeof(size_t) +
		3 * (3 * sizeof(void *) + sizeof(size_t)) + 4 * (2 * sizeof(void *));
	// std::copy receipt bytes/equality bytevectors: complete source-bound
	// iterator/normal-iterator wrappers and memmove/memcmp carrier values.
	constexpr size_t byte_algorithms = 8 * (4 * sizeof(void *)) + 4 * sizeof(size_t) +
					   3 * sizeof(std::ptrdiff_t) + 3 * sizeof(bool) +
					   8 * (2 * sizeof(void *));
	return 8 * sizeof(size_t) + owning_calls + string_construct + unique_construct_move +
	       lookup + insert + rollback + cleanup + byte_algorithms +
	       // The owned replay observer joins the complete integrated178 row
	       // CURRENT closure; pure54's original profile remains byte unchanged.
	       player_item_snapshot_current_heap_observer_frame_bytes();
}

// Private unselected complete registration body. Owning codec and typed source
// profiles are joined separately before this is eligible for maintained source.
struct auction_replay_work
{
	std::string key;
	std::unique_ptr<auction_native_pending> entry, previous;
	auction_native_command_context native;
};
struct auction_replay_equal_work
{
	std::vector<uint8_t> left, right;
};
struct auction_replay_budget
{
	player_save_coin_replay_budget_scope_owner &pipeline;
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, source_frames;
	auction_replay_work *work = nullptr;
	auction_replay_equal_work *duplicate = nullptr;
	size_t observed_auction = 0, observed_pipeline = 0;
	bool observing_pipeline = false;
	bool denied = false;

	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &b = *static_cast<auction_replay_budget *>(opaque);
		// Replace only the actual owned CURRENT terms captured by this
		// specific prefix. The pipeline child owns its own CURRENT when
		// observing_pipeline is false; its fresh term must not be removed.
		size_t auction = 0, pipeline = 0;
		if (b.denied || !b.reserve || amount < b.observed_auction ||
		    !auction_native_publication_current_storage_bytes(&auction))
		{
			b.denied = true;
			return false;
		}
		amount -= b.observed_auction;
		if (!auction_current_add(amount, auction) ||
		    (b.observing_pipeline &&
		     (amount < b.observed_pipeline ||
		      !player_save_auction_replay_owner::current_storage_bytes(b.pipeline,
									       &pipeline))))
		{
			b.denied = true;
			return false;
		}
		if (b.observing_pipeline)
		{
			amount -= b.observed_pipeline;
			if (!auction_current_add(amount, pipeline))
			{
				b.denied = true;
				return false;
			}
		}
		if (!b.reserve(amount, b.context))
		{
			b.denied = true;
			return false;
		}
		return true;
	}
	bool entry_heap(const auction_native_pending &entry, size_t &bytes) const noexcept
	{
		size_t heap = 0;
		return auction_native_publication_owner::replay_entry_storage_bytes(entry, &heap) &&
		       auction_current_add(bytes, heap);
	}
	bool prefix(size_t &output, bool include_pipeline = true, size_t extra = 0) noexcept
	{
		size_t bytes = outer, current = 0;
		if (!auction_current_add(bytes, sizeof(*this)) ||
		    !auction_current_add(bytes, sizeof(auction_replay_work)) ||
		    !auction_current_add(bytes, source_frames) ||
		    !auction_native_publication_current_storage_bytes(&current) ||
		    !auction_current_add(bytes, current))
		{
			denied = true;
			return false;
		}
		observed_auction = current;
		observing_pipeline = include_pipeline;
		observed_pipeline = 0;
		if (include_pipeline &&
		    (!player_save_auction_replay_owner::current_storage_bytes(pipeline, &current) ||
		     !auction_current_add(bytes, current)))
		{
			denied = true;
			return false;
		}
		if (include_pipeline)
			observed_pipeline = current;
		if (work && (!auction_current_string(work->key, bytes) ||
			     !auction_current_vector(work->native.base_v1_payload, bytes) ||
			     !auction_current_vector(work->native.before_item_uids, bytes) ||
			     (work->entry && !entry_heap(*work->entry, bytes)) ||
			     (work->previous && !entry_heap(*work->previous, bytes))))
		{
			denied = true;
			return false;
		}
		if (duplicate && (!auction_current_add(bytes, sizeof(*duplicate)) ||
				  !auction_current_vector(duplicate->left, bytes) ||
				  !auction_current_vector(duplicate->right, bytes)))
		{
			denied = true;
			return false;
		}
		if (!auction_current_add(bytes, extra))
		{
			denied = true;
			return false;
		}
		output = bytes;
		return true;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t bytes = 0;
		return prefix(bytes, true, extra) && forward(bytes, this);
	}
	bool equal(const critical_command &left, const critical_command &right) noexcept
	{
		if (!peak(sizeof(auction_replay_equal_work)))
			return false;
		size_t nested = 0;
		auction_replay_equal_work candidate;
		duplicate = &candidate;
		const bool equal = prefix(nested) &&
				   critical_command_encode_bounded(left, &candidate.left, forward,
								   this, nested) ==
					   critical_command_codec_result::ok &&
				   prefix(nested) &&
				   critical_command_encode_bounded(right, &candidate.right, forward,
								   this, nested) ==
					   critical_command_codec_result::ok &&
				   candidate.left == candidate.right;
		duplicate = nullptr;
		return equal;
	}
	bool copy(critical_command &destination, const critical_command &source)
	{
		size_t request = 0;
		if (!critical_command_fresh_copy_request_bytes(source, &request) ||
		    !auction_current_add(request, critical_command_copy_frame_bytes()) ||
		    !peak(request))
			return false;
		destination = source;
		return true;
	}
};
static bool auction_replay_prepare(bool (*reserve)(size_t, void *) noexcept, void *context,
				   size_t outer, size_t *output) noexcept
{
	// Actual restore/replayed and prepare parameters/returns/locals and add/
	// reserve scopes; fourteen getter size_t carriers include integrated178
	// CURRENT's nested getter return beside the original pure54 query.
	constexpr size_t own = 8 * sizeof(void *) + 9 * sizeof(size_t) + 4 * sizeof(bool);
	size_t initial = outer, frames = own;
	if (!reserve || !output || sizeof(void *) != 8 || sizeof(size_t) != 8 ||
	    !auction_current_add(initial, sizeof(auction_replay_budget)) ||
	    !auction_current_add(initial, sizeof(auction_replay_work)) ||
	    !auction_current_add(initial, own + 14 * sizeof(size_t)) || !reserve(initial, context))
		return false;
	const size_t replay = auction_native_publication_replay_source_frame_bytes();
	const size_t observation =
		auction_native_publication_current_storage_observer_frame_bytes();
	const size_t pipeline = player_save_auction_replay_owner::current_observer_frame_bytes();
	if (!auction_current_add(frames, replay) || !auction_current_add(frames, observation) ||
	    !auction_current_add(frames, pipeline))
		return false;
	initial = outer;
	if (!auction_current_add(initial, sizeof(auction_replay_budget)) ||
	    !auction_current_add(initial, sizeof(auction_replay_work)) ||
	    !auction_current_add(initial, frames) || !reserve(initial, context))
		return false;
	*output = frames;
	return true;
}
bool auction_native_publication_restore_bounded(const critical_native_recovery_envelope &original,
						player_save_coin_replay_budget_scope_owner &pipeline,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer) noexcept
{
	size_t frames = 0;
	if (!auction_replay_prepare(reserve, context, outer, &frames))
		return false;
	auction_replay_budget budget{ pipeline, reserve, context, outer, frames };
	if (!reserve || !budget.peak() || !nevent_is_game_thread())
		return false;
	auction_replay_work work;
	budget.work = &work;
	try
	{
		size_t nested = 0;
		if (!budget.prefix(nested) ||
		    !auction_recovery_envelope_valid_bounded(
			    original, auction_replay_budget::forward, &budget, nested))
			return false;
		// The genuine binary 16-byte operation key has a 17-byte fresh request
		// under GNU13's 15-byte SSO. It remains live through node-key copying.
		if (!budget.peak(original.command.operation_id.bytes.size() + 1))
			return false;
		work.key = auction_native_key(original.command.operation_id);
		auto found = auction_native_pending_operations.find(work.key);
		if (found != auction_native_pending_operations.end() &&
		    !budget.equal(found->second->command, original.command))
			return false;
		if (found != auction_native_pending_operations.end() &&
		    found->second->envelope.revision)
			return found->second->envelope.revision == original.revision &&
			       found->second->envelope.phase == original.phase &&
			       found->second->envelope.attachment == original.attachment;
		if (found == auction_native_pending_operations.end() &&
		    auction_native_pending_operations.size() >= AUCTION_PENDING_MAX)
			return false;
		if (!budget.peak(sizeof(auction_native_pending)))
			return false;
		work.entry = std::make_unique<auction_native_pending>();
		if (!budget.copy(work.entry->command, original.command) ||
		    !budget.copy(work.entry->envelope.command, original.command) ||
		    !budget.peak(original.attachment.size()))
			return false;
		work.entry->envelope.revision = original.revision;
		work.entry->envelope.phase = original.phase;
		work.entry->envelope.attachment = original.attachment;
		work.entry->restored = true;
		work.entry->submitted = true;
		if (!budget.prefix(nested) ||
		    !auction_command_decode_payload_bounded(
			    work.entry->command, &work.entry->payload,
			    auction_replay_budget::forward, &budget, nested) ||
		    !budget.prefix(nested) ||
		    auction_recovery_context_decode_bounded(
			    work.entry->command, original.attachment, &work.entry->context,
			    auction_replay_budget::forward, &budget,
			    nested) != player_snapshot_codec_result::ok)
			return false;
		if (work.entry->context.receipt_present)
		{
			work.entry->completion.operation_id = work.entry->command.operation_id;
			work.entry->completion.outcome = work.entry->context.outcome;
			work.entry->completion.error_code = work.entry->context.result_code;
			work.entry->completion.failure_stage = work.entry->context.failure_stage;
			work.entry->completion.durable_revision =
				work.entry->context.durable_revision;
			work.entry->completion.result_size = work.entry->context.result.size();
			std::copy(work.entry->context.result.begin(),
				  work.entry->context.result.end(),
				  work.entry->completion.result_payload.begin());
			work.entry->ready = true;
		}
		if (original.attachment.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
		    auction_native_retained_bytes -
				    (found == auction_native_pending_operations.end() ?
					     0 :
					     found->second->retained_bytes) >
			    auction_native_total_bytes - original.attachment.size())
			return false;
		const bool inserted = found == auction_native_pending_operations.end();
		if (inserted)
		{
			using map = decltype(auction_native_pending_operations);
			using node = std::_Rb_tree_node<map::value_type>;
			size_t request = sizeof(node);
			if (!auction_current_add(request, work.key.size() + 1) ||
			    !budget.peak(request))
				return false;
			auto result = auction_native_pending_operations.emplace(
				work.key, std::move(work.entry));
			if (!result.second)
				return false;
			found = result.first;
		}
		else
		{
			work.previous = std::move(found->second);
			found->second = std::move(work.entry);
		}
		auto &installed = *found->second;
		if (installed.payload.actor_pid &&
		    original.phase == critical_native_recovery_phase::execution_pending)
		{
			const size_t checkpoint_query =
				player_save_auction_replay_owner::source_profile_query_frames();
			size_t checkpoint_frames = 0, checkpoint_peak = checkpoint_query;
			// The v9 checkpoint obtains its source profile before its first
			// child callback. Admit that exact query, then its returned whole
			// profile and the repeated query before crossing the SAME scope.
			if (!budget.peak(checkpoint_query) ||
			    !player_save_auction_replay_owner::restore_source_frames(
				    &checkpoint_frames) ||
			    !auction_current_add(checkpoint_peak, checkpoint_frames) ||
			    !budget.peak(checkpoint_peak) || !budget.prefix(nested, false) ||
			    !player_save_auction_replay_owner::restore_recovery_checkpoint(
				    original, pipeline, auction_replay_budget::forward, &budget,
				    nested))
			{
				if (inserted)
					auction_native_pending_operations.erase(found);
				else
					found->second = std::move(work.previous);
				return false;
			}
		}
		// The original checkpoint's successful hold is followed only by scalar
		// bookkeeping and original nonthrowing local destruction.
		if (work.previous)
			auction_native_retained_bytes -= work.previous->retained_bytes;
		installed.retained_bytes = original.attachment.size();
		auction_native_retained_bytes += installed.retained_bytes;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool auction_native_publication_restore_replayed_command_bounded(
	const critical_command &command, player_save_coin_replay_budget_scope_owner &pipeline,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	size_t frames = 0;
	if (!auction_replay_prepare(reserve, context, outer, &frames))
		return false;
	auction_replay_budget budget{ pipeline, reserve, context, outer, frames };
	if (!reserve || !budget.peak() || !nevent_is_game_thread())
		return false;
	auction_replay_work work;
	budget.work = &work;
	try
	{
		size_t nested = 0;
		if (!budget.prefix(nested) ||
		    !auction_repository_frozen_accounting_valid_bounded(
			    command, auction_replay_budget::forward, &budget, nested) ||
		    !budget.prefix(nested) ||
		    auction_native_command_decode_bounded(
			    command, &work.native, auction_replay_budget::forward, &budget,
			    nested) != economic_accounting_error::ok ||
		    !budget.peak(command.operation_id.bytes.size() + 1))
			return false;
		work.key = auction_native_key(command.operation_id);
		auto found = auction_native_pending_operations.find(work.key);
		if (found != auction_native_pending_operations.end())
			return budget.equal(found->second->command, command);
		if (auction_native_pending_operations.size() >= AUCTION_PENDING_MAX ||
		    !budget.peak(sizeof(auction_native_pending)))
			return false;
		work.entry = std::make_unique<auction_native_pending>();
		if (!budget.copy(work.entry->command, command))
			return false;
		work.entry->payload = work.native.payload;
		work.entry->restored = true;
		using map = decltype(auction_native_pending_operations);
		using node = std::_Rb_tree_node<map::value_type>;
		size_t request = sizeof(node);
		if (!auction_current_add(request, work.key.size() + 1) || !budget.peak(request))
			return false;
		auction_native_pending_operations.emplace(work.key, std::move(work.entry));
		return true;
	}
	catch (...)
	{
		return false;
	}
}

#else

size_t auction_native_publication_replay_source_frame_bytes() noexcept
{
	return 0;
}
bool auction_native_publication_restore_bounded(const critical_native_recovery_envelope &,
						player_save_coin_replay_budget_scope_owner &,
						bool (*)(size_t, void *) noexcept, void *,
						size_t) noexcept
{
	return false;
}
bool auction_native_publication_restore_replayed_command_bounded(
	const critical_command &, player_save_coin_replay_budget_scope_owner &,
	bool (*)(size_t, void *) noexcept, void *, size_t) noexcept
{
	return false;
}

#endif

// Additive complete original five-arm fixed validator/Source candidate.
namespace
{
enum class auction_frozen_fixed_child : uint8_t;
}

// PRIVATE owned validator Source draft. Original value-init expressions are kept;
// containing implicit constructors are distinct from brace-initialized ID/array
// members, whose initialization is aggregate initialization rather than a call.
namespace
{
template <class T> constexpr size_t auction_frozen_fixed_vector_default = 6 * sizeof(void *);
// Vector destructor -> _Destroy trivial dispatch -> base destructor,
// _M_deallocate -> traits/allocator/new_allocator -> sized delete, followed
// by actual allocator and new_allocator base cleanup. No nontrivial T here.
template <class T> constexpr size_t auction_frozen_fixed_vector_cleanup =
	sizeof(std::vector<T> *) + 2 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(void *) + 4 * (2 * sizeof(void *) + sizeof(size_t)) +
	// Sized delete plus real _Vector_impl, allocator, new_allocator and
	// _Vector_impl_data cleanup receivers (base destructor above owns P).
	sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) +
	// Genuine C++20 _Destroy and allocator::deallocate runtime false
	// constant-evaluation result carriers; both selected calls still occur.
	2 * sizeof(bool);

constexpr size_t auction_frozen_fixed_outputs_inline =
	sizeof(economic_frozen_intent) + sizeof(auction_command_payload) +
	sizeof(auction_bid_accounting_listing) + sizeof(auction_bid_accounting_accounts) +
	sizeof(auction_settlement_listing) + sizeof(auction_settlement_accounts) +
	sizeof(auction_item_claim_state) + 3 * sizeof(economic_account_key);
constexpr size_t auction_frozen_fixed_frozen_lifetime_source =
	// Actual intent -> admission -> metadata implicit default calls. ID and
	// digest members use their existing brace-valued DMIs, so no extra ID/
	// array constructor invocation is invented. Destruction traverses the
	// genuine frozen/admission/metadata and ID-wrapper/member-array descendants.
	3 * sizeof(void *) + 3 * sizeof(void *) + 4 * 2 * sizeof(void *) + 2 * sizeof(void *) +
	// Actual _Storage() default-initializes _Empty_byte: eight default
	// receivers; seven trivial cleanup receivers, no union member traversal.
	(8 + 7) * sizeof(void *) + auction_frozen_fixed_vector_default<uint8_t> +
	auction_frozen_fixed_vector_cleanup<uint8_t>;
constexpr size_t auction_frozen_fixed_dto_lifetime_source =
	// payload{} is genuine aggregate initialization, with its original seven
	// array members/nine item values; cleanup receivers are separate scopes.
	(1 + 7 + AUCTION_COMMAND_MAX_ITEMS) * sizeof(void *) +
	// bid default containing receiver; two original ID-wrapper/array cleanups.
	sizeof(void *) + (1 + 2 * 2) * sizeof(void *) +
	// bid_accounts default containing receiver; six key/ID/array cleanups.
	sizeof(void *) + (1 + 6 * 3) * sizeof(void *) +
	// settlement default containing receiver; two ID-wrapper/arrays plus
	// actual items array and nine trivial settlement-row cleanup receivers.
	sizeof(void *) + (1 + 2 * 2 + 1 + AUCTION_COMMAND_MAX_ITEMS) * sizeof(void *) +
	// settlement_accounts default receiver, four key/ID/array cleanups.
	sizeof(void *) + (1 + 4 * 3) * sizeof(void *) +
	// claim default receiver, its two IDs and array/nine row cleanup scopes.
	sizeof(void *) + (1 + 2 * 2 + 1 + AUCTION_COMMAND_MAX_ITEMS) * sizeof(void *) +
	// Three directly default-initialized keys: each containing default receiver
	// and distinct key/ID-wrapper/array cleanup receivers. Their lineage DMI
	// remains aggregate initialization, rather than a default constructor call.
	3 * (sizeof(void *) + 3 * sizeof(void *));
constexpr size_t auction_frozen_fixed_observer_source =
	// prefix(this,total-ref,extra;bool), peak(this,extra,total;bool),
	// forward(amount,opaque,b-ref;bool), checked-add(total-ref,value;bool).
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	// Reused output facts vector's fresh physical capacity observation.
	sizeof(void *) + sizeof(size_t) +
	// Actual aggregate budget cleanup, no called constructor for brace-init.
	sizeof(void *);
constexpr size_t auction_frozen_fixed_caller_source =
	// Public command/callback/context; outer/source/initial/query_peak/
	// entry_peak/nested and two required constexpr size_t locals, bool result.
	3 * sizeof(void *) + 8 * sizeof(size_t) + sizeof(bool) +
	// child_entry: budget/out refs, actual enum argument, source/initial/
	// supplement and one selected constexpr query, admitted/result bools.
	2 * sizeof(void *) + sizeof(auction_frozen_fixed_child) + 4 * sizeof(size_t) +
	2 * sizeof(bool) +
	// Pure own/initial getter output/result and selected policy result.
	sizeof(void *) + 2 * sizeof(bool);
}

// PRIVATE candidate: no complete profile selection before actual five decoder joins.
namespace
{
enum class auction_frozen_fixed_child : uint8_t
{
	bid,
	settlement,
	listing,
	item_claim,
	money_claim
};
struct auction_frozen_fixed_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, source;
	const economic_frozen_intent *intent = nullptr;
	bool denied = false;
	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &b = *static_cast<auction_frozen_fixed_budget *>(opaque);
		if (b.denied || !b.reserve || !b.reserve(amount, b.context))
		{
			b.denied = true;
			return false;
		}
		return true;
	}
	bool prefix(size_t &total, size_t extra = 0) noexcept
	{
		total = outer;
		if (denied || !auction_current_add(total, source) ||
		    !auction_current_add(total, sizeof(*this)) ||
		    !auction_current_add(total, auction_frozen_fixed_outputs_inline) ||
		    (intent && !auction_current_add(total, intent->admission.facts.capacity())) ||
		    !auction_current_add(total, extra))
		{
			denied = true;
			return false;
		}
		return true;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && forward(total, this);
	}
};
bool auction_frozen_fixed_child_entry(auction_frozen_fixed_budget &budget,
				      auction_frozen_fixed_child kind, size_t &nested) noexcept
{
	size_t source = 0, initial = 0, supplement = 0;
	bool admitted = false;
	if (budget.denied)
		return false;
	switch (kind)
	{
	case auction_frozen_fixed_child::bid:
	{
		constexpr size_t query = auction_bid_accounting_decode_source_query_frame_bytes();
		admitted = budget.peak(query) &&
			   auction_bid_accounting_decode_source_frame_bytes(&source) &&
			   auction_bid_accounting_decode_initial_inline_bytes(&initial) &&
			   auction_bid_accounting_decode_source_supplement_frame_bytes(&supplement);
		break;
	}
	case auction_frozen_fixed_child::settlement:
	{
		constexpr size_t query =
			auction_settlement_accounting_decode_source_query_frame_bytes();
		admitted = budget.peak(query) &&
			   auction_settlement_accounting_decode_source_frame_bytes(&source) &&
			   auction_settlement_accounting_decode_initial_inline_bytes(&initial) &&
			   auction_settlement_accounting_decode_source_supplement_frame_bytes(
				   &supplement);
		break;
	}
	case auction_frozen_fixed_child::listing:
	{
		constexpr size_t query =
			auction_listing_accounting_decode_source_query_frame_bytes();
		admitted = budget.peak(query) &&
			   auction_listing_accounting_decode_source_frame_bytes(&source) &&
			   auction_listing_accounting_decode_initial_inline_bytes(&initial) &&
			   auction_listing_accounting_decode_source_supplement_frame_bytes(
				   &supplement);
		break;
	}
	case auction_frozen_fixed_child::item_claim:
	{
		constexpr size_t query =
			auction_item_claim_accounting_decode_source_query_frame_bytes();
		admitted = budget.peak(query) &&
			   auction_item_claim_accounting_decode_source_frame_bytes(&source) &&
			   auction_item_claim_accounting_decode_initial_inline_bytes(&initial) &&
			   auction_item_claim_accounting_decode_source_supplement_frame_bytes(
				   &supplement);
		break;
	}
	case auction_frozen_fixed_child::money_claim:
	{
		constexpr size_t query =
			auction_money_claim_accounting_decode_source_query_frame_bytes();
		admitted = budget.peak(query) &&
			   auction_money_claim_accounting_decode_source_frame_bytes(&source) &&
			   auction_money_claim_accounting_decode_initial_inline_bytes(&initial) &&
			   auction_money_claim_accounting_decode_source_supplement_frame_bytes(
				   &supplement);
		break;
	}
	}
	if (!admitted || !auction_current_add(source, initial) || !budget.peak(source) ||
	    !budget.prefix(nested) || !auction_current_add(nested, supplement))
	{
		budget.denied = true;
		return false;
	}
	return true;
}
}

namespace
{
// Same genuine finite byte-vector/source policy as all five original bounded
// accounting codecs. It grants no writer, admission, or activation authority.
bool auction_frozen_fixed_source_policy() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)
	return sizeof(void *) == 8 && sizeof(size_t) == 8;
#else
	return false;
#endif
}
}
bool auction_repository_frozen_accounting_valid_own_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !auction_frozen_fixed_source_policy())
		return false;
	*output = auction_frozen_fixed_frozen_lifetime_source +
		  auction_frozen_fixed_dto_lifetime_source + auction_frozen_fixed_observer_source +
		  auction_frozen_fixed_caller_source;
	return true;
}
bool auction_repository_frozen_accounting_valid_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !auction_frozen_fixed_source_policy())
		return false;
	*output = sizeof(auction_frozen_fixed_budget) + auction_frozen_fixed_outputs_inline;
	return true;
}
bool auction_repository_frozen_accounting_valid_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !auction_frozen_fixed_source_policy())
		return false;
	*output = 0;
	return true;
}
bool auction_repository_frozen_accounting_valid_source_frame_bytes(size_t *output) noexcept
{
	size_t own = 0, bid = 0, settlement = 0, listing = 0, item = 0, money = 0, total = 0;
	if (!output || !auction_repository_frozen_accounting_valid_own_source_frame_bytes(&own) ||
	    !auction_bid_accounting_decode_source_frame_bytes(&bid) ||
	    !auction_settlement_accounting_decode_source_frame_bytes(&settlement) ||
	    !auction_listing_accounting_decode_source_frame_bytes(&listing) ||
	    !auction_item_claim_accounting_decode_source_frame_bytes(&item) ||
	    !auction_money_claim_accounting_decode_source_frame_bytes(&money))
		return false;
	// Genuine five original short-circuit child phases are sequential. Every
	// actual reusable parent DTO/Source remains owned throughout each child.
	total = bid;
	if (settlement > total)
		total = settlement;
	if (listing > total)
		total = listing;
	if (item > total)
		total = item;
	if (money > total)
		total = money;
	if (!auction_current_add(total, own))
		return false;
	*output = total;
	return true;
}

// PRIVATE candidate; genuine complete own/lower Source definitions remain unselected until sealed.
bool auction_repository_frozen_accounting_valid_fixed_bounded(const critical_command &command,
							      bool (*reserve)(size_t,
									      void *) noexcept,
							      void *context, size_t outer) noexcept
{
	size_t source = 0, initial = 0, query_peak = outer, entry_peak = outer, nested = 0;
	constexpr size_t query =
		auction_repository_frozen_accounting_valid_own_source_query_frame_bytes();
	constexpr size_t entry_source =
		3 * sizeof(void *) + 8 * sizeof(size_t) + sizeof(bool) +
		// Genuine checked-add(ref,value,result) before first callback.
		sizeof(void *) + sizeof(size_t) + sizeof(bool);
	if (!reserve || !auction_current_add(query_peak, entry_source) ||
	    !auction_current_add(query_peak, query) || !reserve(query_peak, context) ||
	    !auction_repository_frozen_accounting_valid_own_source_frame_bytes(&source) ||
	    !auction_repository_frozen_accounting_valid_initial_inline_bytes(&initial) ||
	    !auction_current_add(entry_peak, source) || !auction_current_add(entry_peak, initial) ||
	    !reserve(entry_peak, context))
		return false;
	auction_frozen_fixed_budget budget{ reserve, context, outer, source };
	try
	{
		economic_frozen_intent intent;
		auction_command_payload payload{};
		auction_bid_accounting_listing bid;
		auction_bid_accounting_accounts bid_accounts;
		auction_settlement_listing settlement;
		auction_settlement_accounts settlement_accounts;
		auction_item_claim_state claim;
		economic_account_key wallet, bank, claim_account;
		budget.intent = &intent;
		// Complete original five-decoder short circuit and returned semantic
		// failures survive. Real refusal remains sticky through later OR arms.
		return (auction_frozen_fixed_child_entry(budget, auction_frozen_fixed_child::bid,
							 nested) &&
			auction_bid_accounting_decode_fixed_bounded(
				command, &intent, &payload, &bid, &bid_accounts,
				auction_frozen_fixed_budget::forward, &budget,
				nested) == economic_accounting_error::ok) ||
		       (auction_frozen_fixed_child_entry(
				budget, auction_frozen_fixed_child::settlement, nested) &&
			auction_settlement_accounting_decode_fixed_bounded(
				command, &intent, &payload, &settlement, &settlement_accounts,
				auction_frozen_fixed_budget::forward, &budget,
				nested) == economic_accounting_error::ok) ||
		       (auction_frozen_fixed_child_entry(
				budget, auction_frozen_fixed_child::listing, nested) &&
			auction_listing_accounting_decode_fixed_bounded(
				command, &intent, &payload, &wallet, &bank,
				auction_frozen_fixed_budget::forward, &budget,
				nested) == economic_accounting_error::ok) ||
		       (auction_frozen_fixed_child_entry(
				budget, auction_frozen_fixed_child::item_claim, nested) &&
			auction_item_claim_accounting_decode_fixed_bounded(
				command, &intent, &payload, &claim, &wallet, &bank,
				auction_frozen_fixed_budget::forward, &budget,
				nested) == economic_accounting_error::ok) ||
		       (auction_frozen_fixed_child_entry(
				budget, auction_frozen_fixed_child::money_claim, nested) &&
			auction_money_claim_accounting_decode_fixed_bounded(
				command, &intent, &payload, &wallet, &bank, &claim_account,
				auction_frozen_fixed_budget::forward, &budget,
				nested) == economic_accounting_error::ok);
	}
	catch (...)
	{
		return false;
	}
}
