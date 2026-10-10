#include "world/economic_initialized_world_snapshot.h"
#include "account/account.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <map>
#include <new>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <utility>

extern P_obj object_list;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_room world;
extern const int top_of_world;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern P_index mob_index;
extern int top_of_mobt;

namespace
{
struct failure
{
	unsigned int code;
};
void require(bool value, unsigned int code = EILSEQ)
{
	if (!value)
		throw failure{ code };
}
struct budget
{
	const economic_sql_source_limits &limit;
	economic_initialized_world_snapshot &value;
	void add(uint64_t &target, uint64_t count, uint64_t maximum)
	{
		require(target <= maximum && count <= maximum - target, E2BIG);
		target += count;
	}
	void row() { add(value.rows, 1, limit.maximum_rows); }
	void cell(uint64_t bytes)
	{
		require(bytes <= limit.maximum_single_cell_bytes, E2BIG);
		add(value.cells, 1, limit.maximum_cells);
		add(value.cell_bytes, bytes, limit.maximum_cell_bytes);
	}
	void scalars(size_t count, size_t width)
	{
		for (size_t i = 0; i < count; ++i)
			cell(width);
	}
	void text(const std::string &s) { cell(s.size()); }
	void literal(const player_item_snapshot &s)
	{
		// The item row was charged during registry enumeration. Every retained
		// primitive/array element is a cell; variable child records are separate rows.
		scalars(1, 4);
		scalars(1, 2);
		scalars(2, 8);
		scalars(1, 4);
		scalars(2, 1);
		text(s.name);
		text(s.short_description);
		text(s.description);
		text(s.action_description);
		scalars(s.values.size(), 4);
		scalars(s.timers.size(), 8);
		scalars(6, 4);
		scalars(1, 1);
		scalars(1, 4);
		scalars(2, 2);
		scalars(s.bitvectors.size(), 8);
		scalars(s.affects.size() * 2, 2);
		for (size_t index = 0; index < s.dynamic_affects.size(); ++index)
		{
			row();
			scalars(2, 2);
			scalars(1, 8);
		}
		for (const auto &extra : s.extra_descriptions)
		{
			row();
			text(extra.keyword);
			text(extra.description);
			scalars(1, 1);
			for (int32_t ignored : extra.spell_ids)
			{
				(void)ignored;
				row();
				scalars(1, 4);
			}
		}
	}
};
struct physical_link
{
	size_t count = 0;
	economic_world_item_location kind = {};
	P_char body = nullptr;
	P_obj parent = nullptr;
	int room = -1, slot = 0;
};
struct census
{
	budget &charge;
	economic_initialized_world_snapshot &out;
	std::unordered_map<P_obj, physical_link> objects;
	std::vector<P_obj> object_order;
	std::unordered_map<uint64_t, P_obj> uids;
	std::unordered_map<P_char, uint32_t> body_indices;
	std::vector<P_char> bodies;
	std::unordered_map<P_desc, uint32_t> descriptor_indices;
	std::vector<P_desc> descriptors;
	std::unordered_map<P_char, int> body_rooms;
	std::unordered_set<P_char> registered;
	std::unordered_map<P_obj, uint32_t> item_indices;
	census(budget &b, economic_initialized_world_snapshot &v)
		: charge(b)
		, out(v)
	{
	}
	void account(const char *name, std::optional<std::string> &output)
	{
		if (!name)
		{
			charge.cell(0);
			return;
		}
		const size_t cap = static_cast<size_t>(charge.limit.maximum_single_cell_bytes);
		const size_t length = strnlen(name, cap + 1);
		require(length <= cap, E2BIG);
		charge.cell(length);
		output = std::string(name, length);
	}
	uint32_t body(P_char ch)
	{
		if (!ch)
			return ECONOMIC_WORLD_NO_INDEX;
		auto found = body_indices.find(ch);
		if (found != body_indices.end())
			return found->second;
		require(IS_NPC(ch) ? ch->only.npc != nullptr : ch->only.pc != nullptr);
		charge.row();
		const uint32_t index = static_cast<uint32_t>(bodies.size());
		require(index < ECONOMIC_WORLD_NO_INDEX, E2BIG);
		body_indices.emplace(ch, index);
		bodies.push_back(ch);
		out.bodies.emplace_back();
		physical_link link;
		link.kind = economic_world_item_location::carried;
		link.body = ch;
		links(ch->carrying, link);
		link.kind = economic_world_item_location::equipment;
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			if (ch->equipment[slot])
			{
				link.slot = slot + 1;
				add_link(ch->equipment[slot], link);
			}
		return index;
	}
	void add_link(P_obj object, physical_link link)
	{
		auto found = objects.find(object);
		require(found != objects.end() && !found->second.count);
		link.count = 1;
		found->second = link;
	}
	void links(P_obj head, physical_link link)
	{
		std::unordered_set<P_obj> seen;
		for (P_obj object = head; object; object = object->next_content)
		{
			require(seen.insert(object).second);
			add_link(object, link);
		}
	}
	bool reciprocal(P_obj object, const physical_link &link) const
	{
		if (link.count != 1)
			return false;
		switch (link.kind)
		{
		case economic_world_item_location::room:
			return object->loc_p == LOC_ROOM && object->loc.room == link.room;
		case economic_world_item_location::inside:
			return object->loc_p == LOC_INSIDE && object->loc.inside == link.parent;
		case economic_world_item_location::carried:
			return object->loc_p == LOC_CARRIED && object->loc.carrying == link.body;
		case economic_world_item_location::equipment:
			return object->loc_p == LOC_WORN && object->loc.wearing == link.body &&
			       !object->next_content;
		}
		return false;
	}
	void scan()
	{
		P_obj previous = nullptr;
		for (P_obj object = object_list; object; object = object->next)
		{
			require(object->prev == previous && !objects.contains(object));
			require(object->obj_uid && object->obj_uid != UINT64_MAX &&
				!uids.contains(object->obj_uid));
			charge.row();
			objects.emplace(object, physical_link{});
			uids.emplace(object->obj_uid, object);
			object_order.push_back(object);
			previous = object;
		}
		for (P_obj object : object_order)
		{
			physical_link link;
			link.kind = economic_world_item_location::inside;
			link.parent = object;
			links(object->contains, link);
		}
		require(world && top_of_world >= 0 && zone_table && top_of_zone_table >= 0);
		// Check the full empty-room population before reserving its owning rows.
		require(static_cast<uint64_t>(top_of_world) + 1 <=
				charge.limit.maximum_rows - out.rows,
			E2BIG);
		std::unordered_set<int> room_vnums;
		for (int r = 0; r <= top_of_world; ++r)
		{
			require(world[r].zone <= top_of_zone_table &&
				room_vnums.insert(world[r].number).second);
			charge.row();
			economic_world_room_observation room;
			room.room_rnum = r;
			room.room_vnum = world[r].number;
			room.zone_rnum = world[r].zone;
			room.zone_vnum = zone_table[world[r].zone].number;
			charge.scalars(4, 4);
			physical_link link;
			link.kind = economic_world_item_location::room;
			link.room = r;
			links(world[r].contents, link);
			std::unordered_set<P_char> room_people;
			for (P_char ch = world[r].people; ch; ch = ch->next_in_room)
			{
				require(room_people.insert(ch).second && !body_rooms.contains(ch) &&
					ch->in_room == r);
				body_rooms.emplace(ch, r);
				charge.cell(4);
				room.people.push_back(body(ch));
			}
			out.rooms.push_back(std::move(room));
		}
		for (P_char ch = character_list; ch; ch = ch->next)
		{
			require(registered.insert(ch).second);
			body(ch);
		}
		std::unordered_map<P_char, P_desc> descriptor_body_owners;
		for (P_desc d = descriptor_list; d; d = d->next)
		{
			require(!descriptor_indices.contains(d));
			auto claim = [&](P_char ch)
			{
				if (!ch)
					return;
				const auto [owner, inserted] =
					descriptor_body_owners.emplace(ch, d);
				require(inserted || owner->second == d);
			};
			if (d->character)
			{
				require(d->character->desc == d);
				claim(d->character);
			}
			if (d->original)
			{
				// Silent switch/morph originals legitimately have no descriptor.
				// An original may still retain this same descriptor, never another.
				require(d->character && d->original != d->character &&
					(!d->original->desc || d->original->desc == d));
				claim(d->original);
			}
			charge.row();
			charge.scalars(2, 4);
			const uint32_t index = static_cast<uint32_t>(descriptors.size());
			descriptor_indices.emplace(d, index);
			descriptors.push_back(d);
			economic_world_descriptor_observation descriptor;
			descriptor.character_index = body(d->character);
			descriptor.original_index = body(d->original);
			account(d->account ? d->account->acct_name : nullptr,
				descriptor.account_name);
			out.descriptors.push_back(std::move(descriptor));
		}
		// Extend the complete closure until no original/controller body is new.
		for (size_t i = 0; i < bodies.size(); ++i)
		{
			P_char ch = bodies[i];
			if (ch->desc)
				require(descriptor_indices.contains(ch->desc));
			if (IS_NPC(ch))
				body(ch->only.npc->orig_char);
			body(GET_PLYR(ch));
		}
		std::unordered_map<P_obj, uint8_t> colors;
		for (P_obj object : object_order)
		{
			require(reciprocal(object, objects.at(object)));
			std::vector<P_obj> path;
			P_obj current = object;
			while (current && !colors[current])
			{
				colors[current] = 1;
				path.push_back(current);
				const auto &link = objects.at(current);
				current = link.kind == economic_world_item_location::inside ?
						  link.parent :
						  nullptr;
			}
			require(!current || colors[current] != 1);
			for (P_obj node : path)
				colors[node] = 2;
		}
	}
	uint32_t body_index(P_char ch) const
	{
		return ch ? body_indices.at(ch) : ECONOMIC_WORLD_NO_INDEX;
	}
	void freeze_bodies()
	{
		std::unordered_set<uint64_t> runtimes;
		std::unordered_set<int32_t> pids;
		for (size_t i = 0; i < bodies.size(); ++i)
		{
			P_char ch = bodies[i];
			auto &v = out.bodies[i];
			require(!ch->runtime_id || runtimes.insert(ch->runtime_id).second);
			v.npc = IS_NPC(ch);
			v.globally_registered = registered.contains(ch);
			v.runtime_id = ch->runtime_id;
			v.room_rnum = ch->in_room;
			const auto room = body_rooms.find(ch);
			if (ch->in_room != NOWHERE)
			{
				require(ch->in_room >= 0 && ch->in_room <= top_of_world &&
					room != body_rooms.end() && room->second == ch->in_room);
				v.room_index = static_cast<uint32_t>(ch->in_room);
			}
			else
				require(room == body_rooms.end() && !ch->next_in_room);
			if (ch->desc)
			{
				require(ch->desc->character == ch || ch->desc->original == ch);
				v.descriptor_index = descriptor_indices.at(ch->desc);
			}
			v.player_body_index = body_index(GET_PLYR(ch));
			v.cash = { GET_COPPER(ch), GET_SILVER(ch), GET_GOLD(ch), GET_PLATINUM(ch) };
			v.durable_pet_uid = ch->durable_pet_uid;
			v.durable_pet_owner_pid = ch->durable_pet_owner_pid;
			charge.scalars(2, 1);
			charge.scalars(3, 8);
			charge.scalars(10, 4);
			charge.scalars(4, 8);
			if (v.npc)
			{
				v.mobile_rnum = ch->only.npc->R_num;
				v.native_pet_id = ch->only.npc->idnum;
				v.shop_id = ch->only.npc->shopkeeper_shop_id;
				v.original_body_index = body_index(ch->only.npc->orig_char);
				if (mob_index && v.mobile_rnum >= 0 && v.mobile_rnum <= top_of_mobt)
					v.mobile_vnum = mob_index[v.mobile_rnum].virtual_number;
				quest_mobile_native_reference reference;
				if (quest_mobile_native_reference_copy(ch, ch->runtime_id,
								       &reference))
				{
					charge.cell(QUEST_MOBILE_NATIVE_REFERENCE_BYTES);
					v.native_reference = reference;
				}
				else
					charge.cell(0);
				quest_mobile_native_cash_reference cash;
				if (quest_mobile_native_cash_reference_copy(ch, ch->runtime_id,
									    &cash))
				{
					charge.cell(QUEST_MOBILE_NATIVE_REFERENCE_BYTES);
					charge.scalars(2, 16);
					charge.scalars(6, 8);
					v.native_cash_reference = cash;
				}
				else
					charge.cell(0);
			}
			else
			{
				v.player_pid = ch->only.pc->pid;
				require(v.player_pid >= 0 &&
					(!v.player_pid || pids.insert(v.player_pid).second));
				v.wallet_revision = ch->only.pc->wallet_revision;
				charge.row();
				economic_world_bank_projection bank;
				bank.body_index = static_cast<uint32_t>(i);
				bank.racewar = GET_RACEWAR(ch);
				bank.denominations = { GET_BALANCE_COPPER(ch),
						       GET_BALANCE_SILVER(ch), GET_BALANCE_GOLD(ch),
						       GET_BALANCE_PLATINUM(ch) };
				bank.revision = ch->only.pc->bank_revision;
				charge.scalars(1, 4);
				charge.scalars(1, 1);
				charge.scalars(5, 8);
				// Do not call get_account_name_safe: its "Unknown" fallback is not identity.
				account(ch->desc && ch->desc->account ?
						ch->desc->account->acct_name :
						nullptr,
					bank.account_name);
				out.bank_projections.push_back(std::move(bank));
				charge.cell(0); // Unavailable native reference on an actual PC.
				charge.cell(
					0); // Unavailable native cash reference on an actual PC.
			}
			charge.cell(v.mobile_vnum ? 4 : 0);
		}
		// Controller/original cycles are malformed; GET_PLYR's normal self edge is not.
		std::vector<uint8_t> colors(out.bodies.size(), 0);
		for (uint32_t start = 0; start < out.bodies.size(); ++start)
		{
			if (colors[start])
				continue;
			std::vector<std::pair<uint32_t, uint8_t>> path;
			path.emplace_back(start, 0);
			colors[start] = 1;
			while (!path.empty())
			{
				auto &step = path.back();
				if (step.second == 2)
				{
					colors[step.first] = 2;
					path.pop_back();
					continue;
				}
				const auto &value = out.bodies[step.first];
				const uint8_t edge = step.second++;
				const uint32_t next = edge == 0 ? value.original_body_index :
								  value.player_body_index;
				if (next == ECONOMIC_WORLD_NO_INDEX ||
				    (edge == 1 && next == step.first))
					continue;
				require(next < colors.size() && colors[next] != 1);
				if (!colors[next])
				{
					colors[next] = 1;
					path.emplace_back(next, 0);
				}
			}
		}
	}
	uint32_t tree(P_obj root, int slot)
	{
		require(root && !item_indices.contains(root));
		std::vector<player_item_snapshot> literals;
		size_t estimated = 0;
		const auto captured =
			player_item_snapshot_tree_capture_literal(root, &literals, &estimated);
		require(captured == player_snapshot_capture_result::ok,
			captured == player_snapshot_capture_result::retryable_allocation_failure ?
				ENOMEM :
			captured == player_snapshot_capture_result::limit_exceeded ? E2BIG :
										     EILSEQ);
		require(!literals.empty() && literals.front().object_uid == root->obj_uid);
		require(out.items.size() <= INT32_MAX &&
				literals.size() <= INT32_MAX - out.items.size(),
			E2BIG);
		const uint32_t root_index = static_cast<uint32_t>(out.items.size());
		for (auto &literal : literals)
		{
			const auto uid = uids.find(literal.object_uid);
			require(uid != uids.end());
			P_obj object = uid->second;
			const auto &link = objects.at(object);
			require(!item_indices.contains(object));
			const uint32_t index = static_cast<uint32_t>(out.items.size());
			economic_world_item_observation item;
			item.location = link.kind;
			item.root_index = root_index;
			if (literal.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
			{
				require(object == root &&
					link.kind != economic_world_item_location::inside);
				literal.equipment_slot = static_cast<int16_t>(slot);
			}
			else
			{
				require(literal.parent_index >= 0 &&
					static_cast<size_t>(literal.parent_index) <
						index - root_index &&
					link.kind == economic_world_item_location::inside &&
					item_indices.contains(link.parent) &&
					item_indices.at(link.parent) ==
						root_index + static_cast<uint32_t>(
								     literal.parent_index));
				literal.parent_index += static_cast<int32_t>(root_index);
			}
			switch (link.kind)
			{
			case economic_world_item_location::room:
				item.owner_index = static_cast<uint32_t>(link.room);
				break;
			case economic_world_item_location::inside:
				item.owner_index = item_indices.at(link.parent);
				break;
			case economic_world_item_location::carried:
			case economic_world_item_location::equipment:
				item.owner_index = body_indices.at(link.body);
				break;
			}
			require((link.kind == economic_world_item_location::equipment ?
					 link.slot :
					 0) == literal.equipment_slot);
			charge.literal(literal);
			charge.scalars(1, 1);
			charge.scalars(2, 4);
			item.literal = std::move(literal);
			item_indices.emplace(object, index);
			out.items.push_back(std::move(item));
		}
		return root_index;
	}
	void freeze_items()
	{
		for (size_t r = 0; r < out.rooms.size(); ++r)
			for (P_obj object = world[r].contents; object;
			     object = object->next_content)
			{
				charge.cell(4);
				out.rooms[r].roots.push_back(tree(object, 0));
			}
		for (size_t i = 0; i < bodies.size(); ++i)
		{
			P_char ch = bodies[i];
			auto &v = out.bodies[i];
			for (int slot = 0; slot < MAX_WEAR; ++slot)
				if (ch->equipment[slot])
				{
					charge.cell(4);
					v.equipment_roots.push_back(
						tree(ch->equipment[slot], slot + 1));
				}
			for (P_obj object = ch->carrying; object; object = object->next_content)
			{
				charge.cell(4);
				v.inventory_roots.push_back(tree(object, 0));
			}
		}
		require(out.items.size() == objects.size() &&
			item_indices.size() == objects.size());
		for (P_obj object : object_order)
		{
			charge.cell(4);
			out.object_registry_order.push_back(item_indices.at(object));
		}
	}
};
} // namespace

unsigned int
economic_initialized_world_snapshot_capture(const economic_sql_source_limits &limits,
					    economic_initialized_world_snapshot *output) noexcept
{
	if (!output || !nevent_is_game_thread())
		return EINVAL;
	const economic_sql_source_limits hard{};
	if (!limits.maximum_rows || limits.maximum_rows > hard.maximum_rows ||
	    !limits.maximum_cells || limits.maximum_cells > hard.maximum_cells ||
	    !limits.maximum_cell_bytes || limits.maximum_cell_bytes > hard.maximum_cell_bytes ||
	    !limits.maximum_single_cell_bytes ||
	    limits.maximum_single_cell_bytes > hard.maximum_single_cell_bytes)
		return EINVAL;
	try
	{
		economic_initialized_world_snapshot candidate;
		budget charged{ limits, candidate };
		census world_census{ charged, candidate };
		world_census.scan();
		world_census.freeze_bodies();
		world_census.freeze_items();
		static_assert(
			std::is_nothrow_move_assignable_v<economic_initialized_world_snapshot>);
		*output = std::move(candidate);
		return 0;
	}
	catch (const failure &failed)
	{
		return failed.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
}

#include <memory>
namespace
{
// Full owner-scoped allocation interception for scratch containers. Rebound
// libstdc++ nodes/buckets reach allocate(n) with their genuine installed type.
// No allocator metadata/RSS/system-library allocation claim follows from this.
struct world_memory
{
	bool (*reserve)(size_t, void *) noexcept = nullptr;
	void *context = nullptr;
	size_t outer = 0, frames = 0, scratch_heap = 0, default_heap = 0;
	bool refused = false;
	static size_t sum(size_t a, size_t b)
	{
		if (b > SIZE_MAX - a)
			throw failure{ ENOBUFS };
		return a + b;
	}
	size_t live() const { return sum(sum(sum(outer, frames), scratch_heap), default_heap); }
	void request(size_t bytes)
	{
		const size_t peak =
			sum(sum(live(), bytes), sizeof(bytes) + sizeof(size_t) + sizeof(this));
		if (refused || !reserve || !reserve(peak, context))
		{
			refused = true;
			throw failure{ ENOBUFS };
		}
	}
	void default_heap_add(size_t bytes) { default_heap = sum(default_heap, bytes); }
	void default_heap_drop(size_t bytes)
	{
		require(bytes <= default_heap, ENOTSUP);
		default_heap -= bytes;
	}
};
struct world_frame
{
	world_memory &memory;
	size_t bytes;
	world_frame(world_memory &m, size_t actual)
		: memory(m)
		, bytes(actual)
	{
		memory.request(world_memory::sum(actual, sizeof(*this)));
		bytes = world_memory::sum(actual, sizeof(*this));
		memory.frames = world_memory::sum(memory.frames, bytes);
	}
	~world_frame() noexcept { memory.frames -= bytes; }
};
bool world_nested_reserve(size_t requested, void *context) noexcept
{
	auto &m = *static_cast<world_memory *>(context);
	if (requested > SIZE_MAX - (sizeof(requested) + sizeof(context) + sizeof(void *)) ||
	    m.refused || !m.reserve ||
	    !m.reserve(requested + sizeof(requested) + sizeof(context) + sizeof(void *), m.context))
	{
		m.refused = true;
		return false;
	}
	return true;
}
template <class T> class world_allocator
{
    public:
	using value_type = T;
	world_memory *memory = nullptr;
	explicit world_allocator(world_memory &m) noexcept
		: memory(&m)
	{
	}
	template <class U> world_allocator(const world_allocator<U> &other) noexcept
		: memory(other.memory)
	{
	}
	T *allocate(size_t count)
	{
		world_frame carriers(*memory, sizeof(count) + sizeof(size_t) + sizeof(T *) +
						      sizeof(std::allocator<T>));
		require(count <= SIZE_MAX / sizeof(T), ENOBUFS);
		const size_t bytes = count * sizeof(T);
		memory->request(bytes);
		T *result = std::allocator<T>{}.allocate(count);
		memory->scratch_heap = world_memory::sum(memory->scratch_heap, bytes);
		return result;
	}
	void deallocate(T *pointer, size_t count) noexcept
	{
		memory->scratch_heap -= count * sizeof(T);
		std::allocator<T>{}.deallocate(pointer, count);
	}
	template <class U> bool operator==(const world_allocator<U> &rhs) const noexcept
	{
		return memory == rhs.memory;
	}
};
template <class T> using world_vector = std::vector<T, world_allocator<T>>;
template <class K, class V> using world_map =
	std::unordered_map<K, V, std::hash<K>, std::equal_to<K>,
			   world_allocator<std::pair<const K, V>>>;
template <class K> using world_set =
	std::unordered_set<K, std::hash<K>, std::equal_to<K>, world_allocator<K>>;
template <class K, class V> world_map<K, V> world_map_make(world_memory &m)
{
	return world_map<K, V>(0, std::hash<K>{}, std::equal_to<K>{},
			       world_allocator<std::pair<const K, V>>(m));
}
template <class K> world_set<K> world_set_make(world_memory &m)
{
	return world_set<K>(0, std::hash<K>{}, std::equal_to<K>{}, world_allocator<K>(m));
}
template <class T, class... Args>
void world_emplace(world_memory &m, std::vector<T> &output, Args &&...args)
{
	world_frame carriers(m, sizeof(void *) + sizeof(size_t) * 4 +
					sizeof...(Args) * sizeof(void *));
	if (output.size() == output.capacity())
	{
		const size_t old = output.capacity();
		const size_t add = output.size() ? output.size() : 1;
		require(add <= output.max_size() - output.size(), ENOBUFS);
		const size_t next = output.size() + add;
		require(next <= SIZE_MAX / sizeof(T), ENOBUFS);
		const size_t bytes = next * sizeof(T);
		m.request(bytes);
		output.reserve(next);
		require(output.capacity() == next, ENOTSUP);
		m.default_heap_drop(old * sizeof(T));
		m.default_heap_add(bytes);
	}
	output.emplace_back(std::forward<Args>(args)...);
}
void world_text(world_memory &m, const char *source, size_t length,
		std::optional<std::string> &output)
{
	world_frame carriers(m, sizeof(source) + sizeof(length) + sizeof(void *) +
					sizeof(std::string) + sizeof(size_t));
	require(!output, EILSEQ);
	const size_t request = length > 15 ? world_memory::sum(length, 1) : 0;
	m.request(request);
	std::string text(source, length);
	require(text.capacity() == (length > 15 ? length : 15), ENOTSUP);
	output = std::move(text);
	m.default_heap_add(request);
}
struct bounded_budget : budget
{
	world_memory &memory;
	bounded_budget(const economic_sql_source_limits &l, economic_initialized_world_snapshot &v,
		       world_memory &m)
		: budget{ l, v }
		, memory(m)
	{
	}
};
struct bounded_census
{
	bounded_budget &charge;
	economic_initialized_world_snapshot &out;
	world_map<P_obj, physical_link> objects;
	world_vector<P_obj> object_order;
	world_map<uint64_t, P_obj> uids;
	world_map<P_char, uint32_t> body_indices;
	world_vector<P_char> bodies;
	world_map<P_desc, uint32_t> descriptor_indices;
	world_vector<P_desc> descriptors;
	world_map<P_char, int> body_rooms;
	world_set<P_char> registered;
	world_map<P_obj, uint32_t> item_indices;
	bounded_census(bounded_budget &b, economic_initialized_world_snapshot &v)
		: charge(b)
		, out(v)
		, objects(world_map_make<P_obj, physical_link>(b.memory))
		, object_order(world_vector<P_obj>(world_allocator<P_obj>(b.memory)))
		, uids(world_map_make<uint64_t, P_obj>(b.memory))
		, body_indices(world_map_make<P_char, uint32_t>(b.memory))
		, bodies(world_vector<P_char>(world_allocator<P_char>(b.memory)))
		, descriptor_indices(world_map_make<P_desc, uint32_t>(b.memory))
		, descriptors(world_vector<P_desc>(world_allocator<P_desc>(b.memory)))
		, body_rooms(world_map_make<P_char, int>(b.memory))
		, registered(world_set_make<P_char>(b.memory))
		, item_indices(world_map_make<P_obj, uint32_t>(b.memory))
	{
	}
	void account(const char *name, std::optional<std::string> &output)
	{
		world_frame admitted_frame(charge.memory,
					   sizeof(name) + sizeof(void *) + 2 * sizeof(size_t));
		if (!name)
		{
			charge.cell(0);
			return;
		}
		const size_t cap = static_cast<size_t>(charge.limit.maximum_single_cell_bytes);
		const size_t length = strnlen(name, cap + 1);
		require(length <= cap, E2BIG);
		charge.cell(length);
		world_text(charge.memory, name, length, output);
	}
	uint32_t body(P_char ch)
	{
		world_frame admitted_frame(charge.memory,
					   sizeof(ch) + sizeof(uint32_t) + sizeof(int) +
						   sizeof(physical_link) +
						   sizeof(decltype(body_indices)::iterator));
		if (!ch)
			return ECONOMIC_WORLD_NO_INDEX;
		auto found = body_indices.find(ch);
		if (found != body_indices.end())
			return found->second;
		require(IS_NPC(ch) ? ch->only.npc != nullptr : ch->only.pc != nullptr);
		charge.row();
		const uint32_t index = static_cast<uint32_t>(bodies.size());
		require(index < ECONOMIC_WORLD_NO_INDEX, E2BIG);
		body_indices.emplace(ch, index);
		bodies.push_back(ch);
		world_emplace(charge.memory, out.bodies);
		physical_link link;
		link.kind = economic_world_item_location::carried;
		link.body = ch;
		links(ch->carrying, link);
		link.kind = economic_world_item_location::equipment;
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			if (ch->equipment[slot])
			{
				link.slot = slot + 1;
				add_link(ch->equipment[slot], link);
			}
		return index;
	}
	void add_link(P_obj object, physical_link link)
	{
		world_frame admitted_frame(charge.memory,
					   sizeof(object) + sizeof(link) +
						   sizeof(decltype(objects)::iterator));
		auto found = objects.find(object);
		require(found != objects.end() && !found->second.count);
		link.count = 1;
		found->second = link;
	}
	void links(P_obj head, physical_link link)
	{
		world_frame admitted_frame(charge.memory, 2 * sizeof(P_obj) + sizeof(link) +
								  sizeof(world_set<P_obj>));
		auto seen = world_set_make<P_obj>(charge.memory);
		for (P_obj object = head; object; object = object->next_content)
		{
			require(seen.insert(object).second);
			add_link(object, link);
		}
	}
	bool reciprocal(P_obj object, const physical_link &link) const
	{
		if (link.count != 1)
			return false;
		switch (link.kind)
		{
		case economic_world_item_location::room:
			return object->loc_p == LOC_ROOM && object->loc.room == link.room;
		case economic_world_item_location::inside:
			return object->loc_p == LOC_INSIDE && object->loc.inside == link.parent;
		case economic_world_item_location::carried:
			return object->loc_p == LOC_CARRIED && object->loc.carrying == link.body;
		case economic_world_item_location::equipment:
			return object->loc_p == LOC_WORN && object->loc.wearing == link.body &&
			       !object->next_content;
		}
		return false;
	}
	void scan()
	{
		world_frame admitted_frame(
			charge.memory,
			5 * sizeof(P_obj) + 4 * sizeof(P_char) + sizeof(P_desc) + sizeof(int) +
				sizeof(uint32_t) + 3 * sizeof(physical_link) +
				sizeof(economic_world_room_observation) +
				sizeof(economic_world_descriptor_observation) +
				sizeof(world_set<int>) + sizeof(world_set<P_char>) +
				sizeof(world_map<P_char, P_desc>) +
				sizeof(world_map<P_obj, uint8_t>) + sizeof(world_vector<P_obj>) +
				sizeof(void *) +
				sizeof(std::pair<typename world_map<P_char, P_desc>::iterator,
						 bool>));
		P_obj previous = nullptr;
		for (P_obj object = object_list; object; object = object->next)
		{
			require(object->prev == previous && !objects.contains(object));
			require(object->obj_uid && object->obj_uid != UINT64_MAX &&
				!uids.contains(object->obj_uid));
			charge.row();
			objects.emplace(object, physical_link{});
			uids.emplace(object->obj_uid, object);
			object_order.push_back(object);
			previous = object;
		}
		for (P_obj object : object_order)
		{
			physical_link link;
			link.kind = economic_world_item_location::inside;
			link.parent = object;
			links(object->contains, link);
		}
		require(world && top_of_world >= 0 && zone_table && top_of_zone_table >= 0);
		// Check the full empty-room population before reserving its owning rows.
		require(static_cast<uint64_t>(top_of_world) + 1 <=
				charge.limit.maximum_rows - out.rows,
			E2BIG);
		auto room_vnums = world_set_make<int>(charge.memory);
		for (int r = 0; r <= top_of_world; ++r)
		{
			require(world[r].zone <= top_of_zone_table &&
				room_vnums.insert(world[r].number).second);
			charge.row();
			economic_world_room_observation room;
			room.room_rnum = r;
			room.room_vnum = world[r].number;
			room.zone_rnum = world[r].zone;
			room.zone_vnum = zone_table[world[r].zone].number;
			charge.scalars(4, 4);
			physical_link link;
			link.kind = economic_world_item_location::room;
			link.room = r;
			links(world[r].contents, link);
			auto room_people = world_set_make<P_char>(charge.memory);
			for (P_char ch = world[r].people; ch; ch = ch->next_in_room)
			{
				require(room_people.insert(ch).second && !body_rooms.contains(ch) &&
					ch->in_room == r);
				body_rooms.emplace(ch, r);
				charge.cell(4);
				world_emplace(charge.memory, room.people, body(ch));
			}
			world_emplace(charge.memory, out.rooms, std::move(room));
		}
		for (P_char ch = character_list; ch; ch = ch->next)
		{
			require(registered.insert(ch).second);
			body(ch);
		}
		auto descriptor_body_owners = world_map_make<P_char, P_desc>(charge.memory);
		for (P_desc d = descriptor_list; d; d = d->next)
		{
			require(!descriptor_indices.contains(d));
			auto claim = [&](P_char ch)
			{
				if (!ch)
					return;
				const auto [owner, inserted] =
					descriptor_body_owners.emplace(ch, d);
				require(inserted || owner->second == d);
			};
			if (d->character)
			{
				require(d->character->desc == d);
				claim(d->character);
			}
			if (d->original)
			{
				// Silent switch/morph originals legitimately have no descriptor.
				// An original may still retain this same descriptor, never another.
				require(d->character && d->original != d->character &&
					(!d->original->desc || d->original->desc == d));
				claim(d->original);
			}
			charge.row();
			charge.scalars(2, 4);
			const uint32_t index = static_cast<uint32_t>(descriptors.size());
			descriptor_indices.emplace(d, index);
			descriptors.push_back(d);
			economic_world_descriptor_observation descriptor;
			descriptor.character_index = body(d->character);
			descriptor.original_index = body(d->original);
			account(d->account ? d->account->acct_name : nullptr,
				descriptor.account_name);
			world_emplace(charge.memory, out.descriptors, std::move(descriptor));
		}
		// Extend the complete closure until no original/controller body is new.
		for (size_t i = 0; i < bodies.size(); ++i)
		{
			P_char ch = bodies[i];
			if (ch->desc)
				require(descriptor_indices.contains(ch->desc));
			if (IS_NPC(ch))
				body(ch->only.npc->orig_char);
			body(GET_PLYR(ch));
		}
		auto colors = world_map_make<P_obj, uint8_t>(charge.memory);
		for (P_obj object : object_order)
		{
			require(reciprocal(object, objects.at(object)));
			world_vector<P_obj> path(world_allocator<P_obj>(charge.memory));
			P_obj current = object;
			while (current && !colors[current])
			{
				colors[current] = 1;
				path.push_back(current);
				const auto &link = objects.at(current);
				current = link.kind == economic_world_item_location::inside ?
						  link.parent :
						  nullptr;
			}
			require(!current || colors[current] != 1);
			for (P_obj node : path)
				colors[node] = 2;
		}
	}
	uint32_t body_index(P_char ch) const
	{
		return ch ? body_indices.at(ch) : ECONOMIC_WORLD_NO_INDEX;
	}
	void freeze_bodies()
	{
		world_frame admitted_frame(
			charge.memory,
			sizeof(world_set<uint64_t>) + sizeof(world_set<int32_t>) +
				sizeof(world_vector<uint8_t>) +
				sizeof(world_vector<std::pair<uint32_t, uint8_t>>) +
				sizeof(economic_world_bank_projection) +
				sizeof(quest_mobile_native_reference) +
				sizeof(quest_mobile_native_cash_reference) + sizeof(size_t) +
				3 * sizeof(uint32_t) + 2 * sizeof(uint8_t) + sizeof(P_char) +
				3 * sizeof(void *) + sizeof(decltype(body_rooms)::iterator));
		auto runtimes = world_set_make<uint64_t>(charge.memory);
		auto pids = world_set_make<int32_t>(charge.memory);
		for (size_t i = 0; i < bodies.size(); ++i)
		{
			P_char ch = bodies[i];
			auto &v = out.bodies[i];
			require(!ch->runtime_id || runtimes.insert(ch->runtime_id).second);
			v.npc = IS_NPC(ch);
			v.globally_registered = registered.contains(ch);
			v.runtime_id = ch->runtime_id;
			v.room_rnum = ch->in_room;
			const auto room = body_rooms.find(ch);
			if (ch->in_room != NOWHERE)
			{
				require(ch->in_room >= 0 && ch->in_room <= top_of_world &&
					room != body_rooms.end() && room->second == ch->in_room);
				v.room_index = static_cast<uint32_t>(ch->in_room);
			}
			else
				require(room == body_rooms.end() && !ch->next_in_room);
			if (ch->desc)
			{
				require(ch->desc->character == ch || ch->desc->original == ch);
				v.descriptor_index = descriptor_indices.at(ch->desc);
			}
			v.player_body_index = body_index(GET_PLYR(ch));
			v.cash = { GET_COPPER(ch), GET_SILVER(ch), GET_GOLD(ch), GET_PLATINUM(ch) };
			v.durable_pet_uid = ch->durable_pet_uid;
			v.durable_pet_owner_pid = ch->durable_pet_owner_pid;
			charge.scalars(2, 1);
			charge.scalars(3, 8);
			charge.scalars(10, 4);
			charge.scalars(4, 8);
			if (v.npc)
			{
				v.mobile_rnum = ch->only.npc->R_num;
				v.native_pet_id = ch->only.npc->idnum;
				v.shop_id = ch->only.npc->shopkeeper_shop_id;
				v.original_body_index = body_index(ch->only.npc->orig_char);
				if (mob_index && v.mobile_rnum >= 0 && v.mobile_rnum <= top_of_mobt)
					v.mobile_vnum = mob_index[v.mobile_rnum].virtual_number;
				quest_mobile_native_reference reference;
				if (quest_mobile_native_reference_copy(ch, ch->runtime_id,
								       &reference))
				{
					charge.cell(QUEST_MOBILE_NATIVE_REFERENCE_BYTES);
					v.native_reference = reference;
				}
				else
					charge.cell(0);
				quest_mobile_native_cash_reference cash;
				if (quest_mobile_native_cash_reference_copy(ch, ch->runtime_id,
									    &cash))
				{
					charge.cell(QUEST_MOBILE_NATIVE_REFERENCE_BYTES);
					charge.scalars(2, 16);
					charge.scalars(6, 8);
					v.native_cash_reference = cash;
				}
				else
					charge.cell(0);
			}
			else
			{
				v.player_pid = ch->only.pc->pid;
				require(v.player_pid >= 0 &&
					(!v.player_pid || pids.insert(v.player_pid).second));
				v.wallet_revision = ch->only.pc->wallet_revision;
				charge.row();
				economic_world_bank_projection bank;
				bank.body_index = static_cast<uint32_t>(i);
				bank.racewar = GET_RACEWAR(ch);
				bank.denominations = { GET_BALANCE_COPPER(ch),
						       GET_BALANCE_SILVER(ch), GET_BALANCE_GOLD(ch),
						       GET_BALANCE_PLATINUM(ch) };
				bank.revision = ch->only.pc->bank_revision;
				charge.scalars(1, 4);
				charge.scalars(1, 1);
				charge.scalars(5, 8);
				// Do not call get_account_name_safe: its "Unknown" fallback is not identity.
				account(ch->desc && ch->desc->account ?
						ch->desc->account->acct_name :
						nullptr,
					bank.account_name);
				world_emplace(charge.memory, out.bank_projections, std::move(bank));
				charge.cell(0); // Unavailable native reference on an actual PC.
				charge.cell(
					0); // Unavailable native cash reference on an actual PC.
			}
			charge.cell(v.mobile_vnum ? 4 : 0);
		}
		// Controller/original cycles are malformed; GET_PLYR's normal self edge is not.
		world_vector<uint8_t> colors(out.bodies.size(), 0,
					     world_allocator<uint8_t>(charge.memory));
		for (uint32_t start = 0; start < out.bodies.size(); ++start)
		{
			if (colors[start])
				continue;
			world_vector<std::pair<uint32_t, uint8_t>> path(
				world_allocator<std::pair<uint32_t, uint8_t>>(charge.memory));
			path.emplace_back(start, 0);
			colors[start] = 1;
			while (!path.empty())
			{
				auto &step = path.back();
				if (step.second == 2)
				{
					colors[step.first] = 2;
					path.pop_back();
					continue;
				}
				const auto &value = out.bodies[step.first];
				const uint8_t edge = step.second++;
				const uint32_t next = edge == 0 ? value.original_body_index :
								  value.player_body_index;
				if (next == ECONOMIC_WORLD_NO_INDEX ||
				    (edge == 1 && next == step.first))
					continue;
				require(next < colors.size() && colors[next] != 1);
				if (!colors[next])
				{
					colors[next] = 1;
					path.emplace_back(next, 0);
				}
			}
		}
	}
	uint32_t tree(P_obj root, int slot)
	{
		world_frame admitted_frame(
			charge.memory,
			sizeof(root) + sizeof(slot) + sizeof(std::vector<player_item_snapshot>) +
				2 * sizeof(size_t) + sizeof(player_snapshot_capture_result) +
				2 * sizeof(uint32_t) + sizeof(P_obj) + 2 * sizeof(void *) +
				sizeof(economic_world_item_observation) +
				sizeof(decltype(uids)::iterator));
		require(root && !item_indices.contains(root));
		std::vector<player_item_snapshot> literals;
		size_t estimated = 0;
		size_t retained_literals = 0;
		const auto captured = player_item_snapshot_tree_capture_literal_bounded(
			root, &literals, &estimated, &world_nested_reserve, &charge.memory,
			charge.memory.live(), &retained_literals);
		if (charge.memory.refused)
			throw failure{ ENOBUFS };
		if (captured == player_snapshot_capture_result::ok)
			charge.memory.default_heap_add(retained_literals);
		require(captured == player_snapshot_capture_result::ok,
			captured == player_snapshot_capture_result::retryable_allocation_failure ?
				ENOMEM :
			captured == player_snapshot_capture_result::limit_exceeded ? E2BIG :
										     EILSEQ);
		require(!literals.empty() && literals.front().object_uid == root->obj_uid);
		require(out.items.size() <= INT32_MAX &&
				literals.size() <= INT32_MAX - out.items.size(),
			E2BIG);
		const uint32_t root_index = static_cast<uint32_t>(out.items.size());
		for (auto &literal : literals)
		{
			const auto uid = uids.find(literal.object_uid);
			require(uid != uids.end());
			P_obj object = uid->second;
			const auto &link = objects.at(object);
			require(!item_indices.contains(object));
			const uint32_t index = static_cast<uint32_t>(out.items.size());
			economic_world_item_observation item;
			item.location = link.kind;
			item.root_index = root_index;
			if (literal.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
			{
				require(object == root &&
					link.kind != economic_world_item_location::inside);
				literal.equipment_slot = static_cast<int16_t>(slot);
			}
			else
			{
				require(literal.parent_index >= 0 &&
					static_cast<size_t>(literal.parent_index) <
						index - root_index &&
					link.kind == economic_world_item_location::inside &&
					item_indices.contains(link.parent) &&
					item_indices.at(link.parent) ==
						root_index + static_cast<uint32_t>(
								     literal.parent_index));
				literal.parent_index += static_cast<int32_t>(root_index);
			}
			switch (link.kind)
			{
			case economic_world_item_location::room:
				item.owner_index = static_cast<uint32_t>(link.room);
				break;
			case economic_world_item_location::inside:
				item.owner_index = item_indices.at(link.parent);
				break;
			case economic_world_item_location::carried:
			case economic_world_item_location::equipment:
				item.owner_index = body_indices.at(link.body);
				break;
			}
			require((link.kind == economic_world_item_location::equipment ?
					 link.slot :
					 0) == literal.equipment_slot);
			charge.literal(literal);
			charge.scalars(1, 1);
			charge.scalars(2, 4);
			item.literal = std::move(literal);
			item_indices.emplace(object, index);
			world_emplace(charge.memory, out.items, std::move(item));
		}
		// Nested payload ownership moved into out.items. Only the emptied literal
		// row block dies here; its inline vector was charged by tree_frame.
		charge.memory.default_heap_drop(literals.capacity() * sizeof(player_item_snapshot));
		return root_index;
	}
	void freeze_items()
	{
		world_frame admitted_frame(charge.memory, 2 * sizeof(size_t) + 2 * sizeof(P_obj) +
								  sizeof(P_char) + sizeof(int) +
								  sizeof(void *));
		for (size_t r = 0; r < out.rooms.size(); ++r)
			for (P_obj object = world[r].contents; object;
			     object = object->next_content)
			{
				charge.cell(4);
				world_emplace(charge.memory, out.rooms[r].roots, tree(object, 0));
			}
		for (size_t i = 0; i < bodies.size(); ++i)
		{
			P_char ch = bodies[i];
			auto &v = out.bodies[i];
			for (int slot = 0; slot < MAX_WEAR; ++slot)
				if (ch->equipment[slot])
				{
					charge.cell(4);
					world_emplace(charge.memory, v.equipment_roots,
						      tree(ch->equipment[slot], slot + 1));
				}
			for (P_obj object = ch->carrying; object; object = object->next_content)
			{
				charge.cell(4);
				world_emplace(charge.memory, v.inventory_roots, tree(object, 0));
			}
		}
		require(out.items.size() == objects.size() &&
			item_indices.size() == objects.size());
		for (P_obj object : object_order)
		{
			charge.cell(4);
			world_emplace(charge.memory, out.object_registry_order,
				      item_indices.at(object));
		}
	}
};

bool world_snapshot_heap(const economic_initialized_world_snapshot &v, size_t *output)
{
	size_t bytes = 0;
	const auto add = [&bytes](size_t n) { bytes = world_memory::sum(bytes, n); };
	const auto rows = [&add](const auto &container)
	{
		using T = typename std::decay_t<decltype(container)>::value_type;
		require(container.capacity() <= SIZE_MAX / sizeof(T), ENOBUFS);
		add(container.capacity() * sizeof(T));
	};
	const auto text = [&add](const std::optional<std::string> &s)
	{
		if (s && s->capacity() > 15)
			add(world_memory::sum(s->capacity(), 1));
	};
	rows(v.rooms);
	rows(v.bodies);
	rows(v.descriptors);
	rows(v.items);
	rows(v.object_registry_order);
	rows(v.bank_projections);
	for (const auto &r : v.rooms)
	{
		rows(r.roots);
		rows(r.people);
	}
	for (const auto &b : v.bodies)
	{
		rows(b.equipment_roots);
		rows(b.inventory_roots);
	}
	for (const auto &d : v.descriptors)
		text(d.account_name);
	for (const auto &b : v.bank_projections)
		text(b.account_name);
	for (const auto &i : v.items)
	{
		size_t literal_heap = 0;
		require(player_item_snapshot_current_heap_bytes(i.literal, &literal_heap), ENOTSUP);
		add(literal_heap);
	}
	*output = bytes;
	return true;
}
} // namespace

bool economic_initialized_world_snapshot_current_heap_bytes(
	const economic_initialized_world_snapshot &value, size_t *output) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	if (!output)
		return false;
	try
	{
		size_t bytes = 0;
		world_snapshot_heap(value, &bytes);
		*output = bytes;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

unsigned int economic_initialized_world_snapshot_capture_bounded(
	const economic_sql_source_limits &limits, economic_initialized_world_snapshot *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live,
	size_t *retained_heap) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return ENOTSUP;
#else
	if (!output || !reserve || !nevent_is_game_thread())
		return EINVAL;
	const economic_sql_source_limits hard{};
	if (!limits.maximum_rows || limits.maximum_rows > hard.maximum_rows ||
	    !limits.maximum_cells || limits.maximum_cells > hard.maximum_cells ||
	    !limits.maximum_cell_bytes || limits.maximum_cell_bytes > hard.maximum_cell_bytes ||
	    !limits.maximum_single_cell_bytes ||
	    limits.maximum_single_cell_bytes > hard.maximum_single_cell_bytes)
		return EINVAL;
	struct workspace
	{
		economic_initialized_world_snapshot candidate;
		world_memory memory;
		size_t actual_heap = 0;
	};
	constexpr size_t frame = sizeof(workspace) + sizeof(bounded_budget) +
				 sizeof(bounded_census) + sizeof(hard) + sizeof(void *) * 3 +
				 sizeof(size_t) * 2;
	if (outer_live > SIZE_MAX - frame || !reserve(outer_live + frame, context))
		return ENOBUFS;
	try
	{
		workspace work;
		work.memory.reserve = reserve;
		work.memory.context = context;
		work.memory.outer = outer_live + frame;
		bounded_budget charged{ limits, work.candidate, work.memory };
		bounded_census world_census{ charged, work.candidate };
		world_census.scan();
		world_census.freeze_bodies();
		world_census.freeze_items();
		world_snapshot_heap(work.candidate, &work.actual_heap);
		require(work.actual_heap == work.memory.default_heap, ENOTSUP);
		static_assert(
			std::is_nothrow_move_assignable_v<economic_initialized_world_snapshot>);
		*output = std::move(work.candidate);
		if (retained_heap)
			*retained_heap = work.actual_heap;
		return 0;
	}
	catch (const failure &f)
	{
		return f.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
#endif
}
